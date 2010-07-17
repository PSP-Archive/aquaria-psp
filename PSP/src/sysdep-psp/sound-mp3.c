/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/sound-mp3.c: MP3 decoding module for the PSP, making use
 * of the PSP's Media Engine.
 */

#include "../common.h"
#include "../sysdep.h"
#include "../memory.h"
#include "../sound.h"
#include "../sound/decode.h"

#include "psplocal.h"
#include "sound-mp3.h"

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* MP3のフレーム長（サンプル）と出力PCMデータのサイズ（バイト） */
#define MP3_FRAME_LEN             1152  // 最大
#define MP3_FRAME_PCMSIZE_MONO    (MP3_FRAME_LEN*2)
#define MP3_FRAME_PCMSIZE_STEREO  (MP3_FRAME_LEN*4)

/* デコードされたPCMデータを格納するバッファの数 */
#define NUM_PCM_BUFFERS    4

/* MP3フレームの最大データ量（バイト） */
#define MP3_FRAME_MAXDATA  2020  // 2016＋パディング（Version 1 Layer 1の場合）

/* MP3ストリームの再生開始時、飛ばしておくサンプル数。これらのサンプルは
 * 実際の音声データの一部ではなく、エンコード・デコード処理の副作用により
 * 発生するもので、ループ処理においてはストリームから除いて位置を計算する。
 * 拡張ヘッダがある場合はそのヘッダ内の値が優先される */
#define MP3_INITIAL_SKIP   1105  // LAME 3.97の場合

/* 拡張ヘッダ（いわゆるXingヘッダ）のサイズ（フレームヘッダを含む） */
#define XING_HEADER_SIZE   194

/*-----------------------------------------------------------------------*/

/* MPEG音声のビットレート・周波数パラメータ */

static const uint16_t mpeg_kbitrate[2][3][16] = {
    {  // MPEG Version 1
        {0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448}, // Layer 1
        {0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384}, // Layer 2
        {0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320}, // Layer 3
    },
    {  // MPEG Version 2, 2.5
        {0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256}, // Layer 1
        {0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, // Layer 2
        {0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, // Layer 3
    },
};

static const uint16_t mpeg_pcmlen[2][3] = {
    /* Layer1  Layer2  Layer3 */
    {    384,   1152,   1152   },  // Version 1
    {    384,   1152,    576   },  // Version 2/2.5
};

static const uint16_t mpeg_freq[2][3] = {
    {44100, 48000, 32000},  // Version 1
    {22050, 24000, 16000},  // Version 2/2.5
};

/*-----------------------------------------------------------------------*/

/* sceAudiocodec用制御バッファ構造体（64バイトアライメント必須） */
typedef struct MP3ControlBuffer_ {
    uint32_t unknown00[3];
    void *EDRAM_ptr;
    uint32_t EDRAM_size;
    uint32_t unknown14;
    const void *src;    // MP3データバッファ
    uint32_t src_size;  // MP3フレームサイズ（バイト）
    void *dest;         // PCMバッファ
    uint32_t dest_size; // PCMバッファサイズ（バイト）
    uint32_t unknown28; // 不明（フレームサイズを格納）
    uint32_t unknown2C[53];
} MP3ControlBuffer;

/* プライベートデータ構造体（同様に64バイトアライメント必須） */
struct SoundDecodePrivate_ {
    /* sceAudiocodec用制御バッファ */
    ALIGNED(64) MP3ControlBuffer mp3_control_buffer;

    /* デコードされたPCMデータを格納するバッファ。複数スレッドからアクセス
     * するため一応volatileだが、ポインタを外部関数に渡すだけなのでvolatile
     * 指定は不要（なおかつ、volatileで定義すると関数に渡す時に警告が発生） */
    ALIGNED(64) uint8_t pcm_buffer[NUM_PCM_BUFFERS][MP3_FRAME_PCMSIZE_STEREO];
    /* PCMバッファ状況フラグ（0以外＝PCMデータがロードされている）。デコード
     * スレッドだけが立て、リセット時以外はメインスレッドだけがクリアする */
    volatile uint8_t pcm_buffer_ok[NUM_PCM_BUFFERS];
    /* 各PCMバッファの先頭サンプルのインデックス */
    int32_t pcm_buffer_pos[NUM_PCM_BUFFERS];
    /* 各PCMバッファの有効サンプル数 */
    int32_t pcm_buffer_len[NUM_PCM_BUFFERS];
    /* 次にデータを取り出すPCMバッファのインデックス（メインスレッド専用） */
    unsigned int next_pcm_buffer;
    /* next_pcm_bufferの中で、次に取り出すサンプルのインデックス */
    uint32_t next_pcm_offset;

    /* デコードスレッドのハンドル */
    SceUID decode_thread;
    /* デコード位置をファイル先頭に戻すためのフラグ。メインスレッドが立てる
     * ことでリセットを要求し、デコードスレッドがクリアすることでリセット
     * 完了を通知する */
    volatile uint8_t thread_reset;
    /* デコードスレッドを終了させるためのフラグ。メインスレッドが立てること
     * で終了を要求し、デコードスレッドが検知すると終了する */
    volatile uint8_t thread_stop;
    /* デコードスレッドが終了したことをメインスレッドに通知するフラグ。終了
     * 要求の他、ループ無効時にファイル終端に到達した場合もスレッドが終了する */
    volatile uint8_t thread_stopped;

    /* MP3データパラメータ */
    int32_t frame_len;          // 1フレームのサンプル数
    int32_t initial_skip;       // 先頭で飛ばしておくサンプル数
    int32_t file_len;           // 合計サンプル数（initial_skip除く、0＝不明）

    /* ループ開始位置関連データ（デコードスレッド専用） */
    uint8_t loop_found;         // 0以外＝ループ開始位置が見つかった
    uint32_t loop_file_pos;     // ループから再生する際の読み込み開始位置
    int32_t loop_decode_pos;    // loop_file_posに対応するサンプルインデックス
    uint32_t frame_pos[11];     // 直前11フレームのファイルオフセット

    /* 再生用カウンター（デコードスレッド専用） */
    uint32_t file_pos;          // 読み込み位置（バイト）
    int32_t decode_pos;         // file_posが指すフレームの先頭位置（サンプル）
};

/*-----------------------------------------------------------------------*/

/* ローカル関数宣言 */

static void psp_decode_mp3_reset(SoundDecodeHandle *this);
static uint32_t psp_decode_mp3_get_pcm(SoundDecodeHandle *this,
                                       int16_t *pcm_buffer, uint32_t pcm_len);
static void psp_decode_mp3_close(SoundDecodeHandle *this);

static int decode_thread(SceSize args, void *argp);
static void parse_xing_header(SoundDecodeHandle *this, const uint8_t *data);
static CONST_FUNCTION int mp3_frame_size(const uint32_t frame_header);
static CONST_FUNCTION int mp3_frame_pcmlen(const uint32_t frame_header);
static CONST_FUNCTION int mp3_frame_freq(const uint32_t frame_header);
__attribute__((unused))
    static CONST_FUNCTION int mp3_frame_channels(const uint32_t frame_header);

/*************************************************************************/
/***************************** メソッド実装 ******************************/
/*************************************************************************/

/**
 * psp_decode_mp3_open:  MP3形式の音声データのデコードを開始する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
int psp_decode_mp3_open(SoundDecodeHandle *this)
{
    this->reset   = psp_decode_mp3_reset;
    this->get_pcm = psp_decode_mp3_get_pcm;
    this->close   = psp_decode_mp3_close;

    this->private = mem_alloc(sizeof(*this->private), 64,
                              MEM_ALLOC_TEMP | MEM_ALLOC_CLEAR);
    if (!this->private) {
        DMSG("Out of memory");
        goto error_return;
    }

    /* MP3デコーダを初期化する */

    MP3ControlBuffer *mp3ctrl = &this->private->mp3_control_buffer;
    int res;
    res = sceAudiocodecCheckNeedMem((void *)mp3ctrl, PSP_CODEC_MP3);
    if (res < 0) {
        DMSG("sceAudiocodecCheckNeedMem(): %s", psp_strerror(res));
        goto error_free_private;
    }
    res = sceAudiocodecGetEDRAM((void *)mp3ctrl, PSP_CODEC_MP3);
    if (res < 0) {
        DMSG("sceAudiocodecGetEDRAM(): %s", psp_strerror(res));
        goto error_free_private;
    }
    res = sceAudiocodecInit((void *)mp3ctrl, PSP_CODEC_MP3);
    if (res < 0) {
        DMSG("sceAudiocodecInit(): %s", psp_strerror(res));
        goto error_free_EDRAM;
    }

    /* MP3ヘッダを解析する */

    const uint8_t *data;
    if (decode_get_data(this, 0, 4, &data) != 4) {
        DMSG("Short file");
        goto error_free_EDRAM;
    }
    const uint32_t header = data[0]<<24 | data[1]<<16 | data[2]<<8 | data[3];
    if (header>>21 != 0x7FF) {
        DMSG("MP3 frame not found");
        goto error_free_EDRAM;
    }
    this->native_freq = mp3_frame_freq(header);
    this->stereo = 1;  // モノラルデータでも、sceAudiocodecはステレオPCMを返す
    this->private->frame_len = mp3_frame_pcmlen(header);
    this->private->initial_skip = MP3_INITIAL_SKIP;
    this->private->file_len = 0;  // 現時点では不明

    /* 拡張ヘッダ（Xingヘッダ）があれば、正確なサンプル数を計算できる */

    if (decode_get_data(this, 0, XING_HEADER_SIZE, &data) == XING_HEADER_SIZE) {
        parse_xing_header(this, data);
    }

    /* PSPのMP3デコーダはさらに1フレームのデコード遅延がある模様 */

    this->private->initial_skip += mp3_frame_pcmlen(header);

    /* ループ範囲指定があれば、ループ終端以降のサンプルを切り捨てる */

    if (this->looplen > 0) {
        const int32_t loopend = this->loopstart + this->looplen;
        if (this->private->file_len == 0 || this->private->file_len > loopend) {
            this->private->file_len = loopend;
        }
    }

    /* デコードスレッドを開始する */

    this->private->file_pos = 0;
    this->private->decode_pos = -(this->private->initial_skip);

    static unsigned int threadnum;
    char namebuf[28];
    snprintf(namebuf, sizeof(namebuf), "MP3DecodeThread_%u", threadnum++);
    this->private->decode_thread = psp_start_thread(namebuf, decode_thread,
                                                    THREADPRI_SOUND, 4096,
                                                    sizeof(this), &this);
    if (this->private->decode_thread < 0) {
        DMSG("psp_start_thread(%s): %s", namebuf,
             psp_strerror(this->private->decode_thread));
        goto error_free_EDRAM;
    }

    /* 成功 */

    this->private->next_pcm_buffer = 0;
    this->private->next_pcm_offset = 0;
    return 1;

    /* エラー処理 */

  error_free_EDRAM:
    sceAudiocodecReleaseEDRAM((void *)&this->private->mp3_control_buffer);
  error_free_private:
    mem_free(this->private);
  error_return:
    return 0;
}

/*************************************************************************/

/**
 * psp_decode_mp3_reset:  音声データを初めから再生するための準備を行う。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
static void psp_decode_mp3_reset(SoundDecodeHandle *this)
{
    this->private->thread_reset = 1;
    while (this->private->thread_reset) {
        sceKernelDelayThread(100);
    }
    this->private->next_pcm_buffer = 0;
    this->private->next_pcm_offset = 0;
}

/*************************************************************************/

/**
 * psp_decode_mp3_get_pcm:  現在の再生位置からPCMデータを取得し、再生位置を
 * 進める。PCMバッファが一杯になる前に音声データの終端に到達した場合、無音
 * データでバッファの残りを埋める。
 *
 * 【引　数】pcm_buffer: PCMデータ（S16LE型）を格納するバッファ
 * 　　　　　   pcm_len: 取得するサンプル数
 * 【戻り値】取得できたサンプル数
 */
static uint32_t psp_decode_mp3_get_pcm(SoundDecodeHandle *this,
                                       int16_t *pcm_buffer, uint32_t pcm_len)
{
    SoundDecodePrivate *private = this->private;
    const unsigned int sample_size = this->stereo ? 4 : 2;

    uint32_t copied = 0;
    while (copied < pcm_len) {

        /* 次のPCMバッファにデータが入っていない場合、デコードスレッドを待つ
         * （終了していない場合） */

        while (!private->pcm_buffer_ok[private->next_pcm_buffer]) {
            if (private->thread_stopped) {
                goto stopped;
            }
            sceKernelDelayThread(100);
        }
        BARRIER();

        /* デコードされたPCMデータを呼び出し側のバッファにコピーする */

        const int32_t pcm_buffer_len =
            private->pcm_buffer_len[private->next_pcm_buffer];
        const int32_t to_copy =
            min(pcm_len - copied, pcm_buffer_len - private->next_pcm_offset);

        if (to_copy > 0) {
            memcpy((uint8_t *)pcm_buffer + copied * sample_size,
                   private->pcm_buffer[private->next_pcm_buffer]
                       + (private->next_pcm_offset * sample_size),
                   to_copy * sample_size);
            copied += to_copy;
            private->next_pcm_offset += to_copy;
        }

        /* PCMデータを使い切った場合、PCMバッファを解放し、次のバッファに移る */

        if (private->next_pcm_offset >= pcm_buffer_len) {
            private->pcm_buffer_ok[private->next_pcm_buffer] = 0;
            BARRIER();
            private->next_pcm_buffer =
                (private->next_pcm_buffer + 1) % NUM_PCM_BUFFERS;
            private->next_pcm_offset = 0;
        }

    }  // while (copied < pcm_len)
  stopped:

    return copied;
}

/*************************************************************************/

/**
 * psp_decode_mp3_close:  デコードを終了し、関連リソースを破棄する。ソース
 * データに関しては、メモリバッファは破棄されないが、ファイルからストリー
 * ミングされている場合はファイルもクローズされる。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
static void psp_decode_mp3_close(SoundDecodeHandle *this)
{
    this->private->thread_stop = 1;
    SceUInt timeout = 10000;  // 10ms
    if (sceKernelWaitThreadEnd(this->private->decode_thread, &timeout) < 0) {
        sceKernelTerminateThread(this->private->decode_thread);
    }
    sceKernelDeleteThread(this->private->decode_thread);
    sceAudiocodecReleaseEDRAM((void *)&this->private->mp3_control_buffer);
    mem_free(this->private);
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * decode_thread:  MP3デコードを行うスレッド。
 *
 * 【引　数】args: 引数のサイズ（常に4）
 * 　　　　　argp: 引数ポインタ（オブジェクトポインタ）
 * 【戻り値】常に0
 */
static int decode_thread(SceSize args, void *argp)
{
    SoundDecodeHandle *this = *(SoundDecodeHandle **)argp;
    SoundDecodePrivate *private = this->private;
    MP3ControlBuffer *mp3ctrl = &private->mp3_control_buffer;
    const unsigned int sample_size = this->stereo ? 4 : 2;

    /* 次にデータを格納するPCMバッファインデックス */
    unsigned int target_pcm_buffer = 0;

    while (!private->thread_stop) {

        /* リセットを要求された場合、再生位置をリセットしてフラグをクリアする */
        if (private->thread_reset) {
            private->file_pos = 0;
            private->decode_pos = -(private->initial_skip);
            unsigned int i;
            for (i = 0; i < NUM_PCM_BUFFERS; i++) {
                private->pcm_buffer_ok[i] = 0;
            }
            target_pcm_buffer = 0;
            private->thread_reset = 0;
        }

        /* 次のバッファが空くまで待つ */

        while (private->pcm_buffer_ok[target_pcm_buffer]) {
            sceKernelDelayThread(1000);
        }
        BARRIER();

        /* ループ開始位置に到達した場合はファイルオフセットを記録する。
         * MP3音声では、直前のフレームデータ（最大511バイト）を参照する
         * 場合もあるので、その分だけ読み込み再開位置を早くする。最小
         * フレームサイズは48バイトなので、11フレーム前から確認し、511
         * バイト以上前に始まる最後のフレームをループ開始フレームとする */

        if (!private->loop_found) {
            if (private->decode_pos >= this->loopstart) {
                private->loop_decode_pos =
                    private->decode_pos - private->frame_len;
                unsigned int i;
                for (i = lenof(private->frame_pos) - 1; i > 0; i--) {
                    if (private->frame_pos[i] + 511 <= private->file_pos) {
                        break;
                    }
                    if (private->frame_pos[i] == 0) {
                        break;  // ファイルの先頭より先には戻れない
                    }
                    private->loop_decode_pos -= private->frame_len;
                }
                private->loop_file_pos = private->frame_pos[i];
                private->loop_found = 1;
            } else {
                unsigned int i;
                for (i = 0; i < lenof(private->frame_pos) - 1; i++) {
                    private->frame_pos[i] = private->frame_pos[i+1];
                }
                private->frame_pos[i] = private->file_pos;
            }
        }

        /* 次のフレームを読み込んでデコードする */

        const uint8_t *data;
        uint32_t datalen = decode_get_data(this, private->file_pos,
                                           MP3_FRAME_MAXDATA, &data);
        if (datalen < 4) {
            if (datalen != 0) {
                DMSG("Short frame header at end of file (0x%X)",
                     private->file_pos);
            }
            goto eof;
        }
        const uint32_t frame_header =
            data[0]<<24 | data[1]<<16 | data[2]<<8 | data[3];
        const uint32_t frame_size = mp3_frame_size(frame_header);
        if (datalen < frame_size) {
            DMSG("Short frame at end of file (0x%X)", private->file_pos);
            goto eof;
        }
        const int32_t frame_pos = private->decode_pos;
        const int32_t pcm_size = private->frame_len * sample_size;
        private->file_pos += frame_size;
        private->decode_pos += private->frame_len;

        mp3ctrl->src       = data;
        mp3ctrl->src_size  = frame_size;
        mp3ctrl->dest      = private->pcm_buffer[target_pcm_buffer];
        mp3ctrl->dest_size = pcm_size;
        mp3ctrl->unknown28 = frame_size;
        int res = sceAudiocodecDecode((void *)mp3ctrl, PSP_CODEC_MP3);
        if (UNLIKELY(res < 0)) {
            DMSG("MP3 decode failed at 0x%X (decode_pos %d): %s",
                 private->file_pos, private->decode_pos, psp_strerror(res));
            mem_clear(private->pcm_buffer, pcm_size);
        }

        int32_t pcm_len = private->frame_len;
        if (frame_pos < 0) {
            if (frame_pos + private->frame_len <= 0) {
                continue;
            }
            const int32_t remove = -(frame_pos);
            pcm_len -= remove;
            memmove(private->pcm_buffer[target_pcm_buffer],
                    private->pcm_buffer[target_pcm_buffer] + (remove * sample_size),
                    pcm_len * sample_size);
        }
        if (private->file_len > 0
         && pcm_len > private->file_len - frame_pos
        ) {
            pcm_len =  private->file_len - frame_pos;
        }
        private->pcm_buffer_pos[target_pcm_buffer] = frame_pos;
        private->pcm_buffer_len[target_pcm_buffer] = pcm_len;
        private->pcm_buffer_ok[target_pcm_buffer] = 1;
        BARRIER();

        /* 次のバッファに進む */

        target_pcm_buffer = (target_pcm_buffer + 1) % NUM_PCM_BUFFERS;

        /* ファイル・ループ終端チェック */

        if (private->file_len > 0 && private->decode_pos >= private->file_len) {
          eof:
            if (this->looplen != 0) {
                if (!private->loop_found) {
                    DMSG("WARNING: Failed to find loop start %d",
                         this->loopstart);
                    private->loop_file_pos = 0;
                    private->loop_decode_pos = -(private->initial_skip);
                    private->loop_found = 1;
                }
                if (private->file_pos == private->loop_file_pos) {
                    DMSG("Failed to read any bytes from file, aborting loop");
                    break;
                }
                private->file_pos = private->loop_file_pos;
                private->decode_pos = private->loop_decode_pos;
            } else {
                break;
            }
        }

    }  // while (!private->thread_stop)

    private->thread_stopped = 1;
    return 0;
}

/*************************************************************************/

/**
 * parse_xing_header:  MP3ファイルの拡張ヘッダ（Xingヘッダ）を解析し、ファ
 * イル情報を更新する。
 *
 * 【引　数】data: ヘッダデータ（少なくともXING_HEADER_SIZEバイト）
 * 【戻り値】なし
 */
static void parse_xing_header(SoundDecodeHandle *this, const uint8_t *data)
{
    const uint32_t frame_header =
        data[0]<<24 | data[1]<<16 | data[2]<<8 | data[3];
    const int mpeg_version_index = frame_header>>19 &  3;
    const int mpeg_layer_index   = frame_header>>17 &  3;
    const int mode_index         = frame_header>> 6 &  3;
    if (UNLIKELY(mpeg_version_index == 1)) {
        DMSG("Bad mpeg_version_index %d", mpeg_version_index);
        return;
    }
    if (UNLIKELY(mpeg_layer_index == 0)) {
        DMSG("Bad mpeg_layer_index %d", mpeg_layer_index);
        return;
    }

    uint32_t xing_offset;
    if (mpeg_version_index == 3) {  // Version 1
        xing_offset = (mode_index == 3) ? 4+17 : 4+32;
    } else {
        xing_offset = (mode_index == 3) ? 4+9 : 4+17;
    }
    data += xing_offset;

    if (memcmp(data, "Xing", 4) != 0 && memcmp(data, "Info", 4) != 0) {
        return;
    }
    data += 4;

    const uint32_t xing_flags =
        data[0]<<24 | data[1]<<16 | data[2]<<8 | data[3];
    data += 4;

    int32_t num_frames;
    if (xing_flags & 0x1) {  // フレーム数
        num_frames = 
            data[0]<<24 | data[1]<<16 | data[2]<<8 | data[3];
        data += 4;
    } else {
        DMSG("Xing header missing frame count, can't compute file length");
        return;
    }
    if (xing_flags & 0x2) {  // エンコードデータ長（バイト）
        data += 4;
    }
    if (xing_flags & 0x4) {  // シークインデックス
        data += 100;
    }
    if (xing_flags & 0x8) {  // VBRスケール
        data += 4;
    }
    data += 21;

    const uint32_t encoder_delay   = data[0]<<4 | data[1]>>4;
    const uint32_t encoder_padding = (data[1] & 0x0F) << 8 | data[2];
    if (encoder_padding >= 529) {
        this->private->initial_skip = encoder_delay + 529;
    } else {
        DMSG("Final padding too short (%d), decode may be corrupt",
             encoder_padding);
        this->private->initial_skip = encoder_delay + encoder_padding;
    }
    this->private->file_len = num_frames * mp3_frame_pcmlen(frame_header)
                              - (encoder_delay + encoder_padding);
}

/*************************************************************************/

/**
 * mp3_frame_size:  MP3フレームのヘッダからフレームのサイズを計算する。
 *
 * 【引　数】frame_header: フレームヘッダの4バイトをビッグエンディアン整数
 * 　　　　　              　　として捉えた値
 * 【戻り値】ヘッダを含むフレームサイズ（バイト）
 */
static CONST_FUNCTION int mp3_frame_size(const uint32_t frame_header)
{
    const int mpeg_version_index = frame_header>>19 &  3;
    const int mpeg_layer_index   = frame_header>>17 &  3;
    const int bitrate_index      = frame_header>>12 & 15;
    const int freq_index         = frame_header>>10 &  3;
    const int padding            = frame_header>> 9 &  1;
    if (UNLIKELY(mpeg_version_index == 1)) {
        DMSG("Bad mpeg_version_index %d", mpeg_version_index);
        return 1;  // 0を返すと無限ループに陥る可能性があるので回避
    }
    if (UNLIKELY(mpeg_layer_index == 0)) {
        DMSG("Bad mpeg_layer_index %d", mpeg_layer_index);
        return 1;
    }

    const int version_index = mpeg_version_index==3 ? 0 : 1;
    const int layer_index = 3 - mpeg_layer_index;
    const int kbitrate = mpeg_kbitrate[version_index][layer_index][bitrate_index];
    const int pcmlen = mpeg_pcmlen[version_index][layer_index];
    int freq = mpeg_freq[version_index][freq_index];
    if (mpeg_version_index == 0) {  // Version 2.5
        freq /= 2;
    }
    PRECOND_SOFT(freq != 0, return 1);
    return ((pcmlen/8) * (1000*kbitrate) / freq)
         + (padding ? (mpeg_layer_index==3 ? 4 : 1) : 0);
}

/*-----------------------------------------------------------------------*/

/**
 * mp3_frame_pcmlen:  MP3フレームのヘッダからフレームのPCMデータ長を計算する。
 *
 * 【引　数】frame_header: フレームヘッダの4バイトをビッグエンディアン整数
 * 　　　　　              　　として捉えた値
 * 【戻り値】PCMデータ長（サンプル）
 */
static CONST_FUNCTION int mp3_frame_pcmlen(const uint32_t frame_header)
{
    const int mpeg_version_index = frame_header>>19 &  3;
    const int mpeg_layer_index   = frame_header>>17 &  3;
    if (UNLIKELY(mpeg_version_index == 1)) {
        DMSG("Bad mpeg_version_index %d", mpeg_version_index);
        return 0;
    }
    if (UNLIKELY(mpeg_layer_index == 0)) {
        DMSG("Bad mpeg_layer_index %d", mpeg_layer_index);
        return 0;
    }

    const int version_index = mpeg_version_index==3 ? 0 : 1;
    const int layer_index = 3 - mpeg_layer_index;
    return mpeg_pcmlen[version_index][layer_index];
}

/*-----------------------------------------------------------------------*/

/**
 * mp3_frame_freq:  MP3フレームのヘッダから再生周波数を取得する。
 *
 * 【引　数】frame_header: フレームヘッダの4バイトをビッグエンディアン整数
 * 　　　　　              　　として捉えた値
 * 【戻り値】再生周波数
 */
static CONST_FUNCTION int mp3_frame_freq(const uint32_t frame_header)
{
    const int mpeg_version_index = frame_header>>19 &  3;
    const int mpeg_layer_index   = frame_header>>17 &  3;
    const int freq_index         = frame_header>>10 &  3;
    if (UNLIKELY(mpeg_version_index == 1)) {
        DMSG("Bad mpeg_version_index %d", mpeg_version_index);
        return 0;
    }
    if (UNLIKELY(mpeg_layer_index == 0)) {
        DMSG("Bad mpeg_layer_index %d", mpeg_layer_index);
        return 0;
    }

    const int version_index = mpeg_version_index==3 ? 0 : 1;
    int freq = mpeg_freq[version_index][freq_index];
    if (mpeg_version_index == 0) {  // Version 2.5
        freq /= 2;
    }
    return freq;
}

/*-----------------------------------------------------------------------*/

/**
 * mp3_frame_channels:  MP3フレームのヘッダからチャンネル数を取得する。
 *
 * 【引　数】frame_header: フレームヘッダの4バイトをビッグエンディアン整数
 * 　　　　　              　　として捉えた値
 * 【戻り値】再生周波数
 */
static CONST_FUNCTION int mp3_frame_channels(const uint32_t frame_header)
{
    return (frame_header>>6 & 3) == 3 ? 1 : 2;
}

/*************************************************************************/
/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
