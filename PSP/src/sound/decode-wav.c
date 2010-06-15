/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sound/decode-wav.c: Audio "decoder" for RIFF WAVE-encapsulated PCM data.
 */

#include "../common.h"
#include "../memory.h"
#include "../sound.h"
#include "decode.h"

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* プライベートデータ構造体 */
struct SoundDecodePrivate_ {
    uint32_t data_offset;  // PCMデータのファイルオフセット（バイト）
    uint32_t sample_size;  // 1サンプルのサイズ（バイト）
    uint32_t len;          // 全データ長（サンプル）
    uint32_t pos;          // 現在の再生位置（サンプル）
};

/*-----------------------------------------------------------------------*/

/* ローカル関数宣言 */

static void decode_wav_reset(SoundDecodeHandle *this);
static uint32_t decode_wav_get_pcm(SoundDecodeHandle *this,
                                   int16_t *pcm_buffer, uint32_t pcm_len);
static void decode_wav_close(SoundDecodeHandle *this);

static int scan_wav_header(SoundDecodeHandle *this, const uint8_t *buffer,
                           uint32_t buflen);

/*************************************************************************/
/***************************** メソッド実装 ******************************/
/*************************************************************************/

/**
 * decode_wav_open:  WAV形式の音声データのデコードを開始する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
int decode_wav_open(SoundDecodeHandle *this)
{
    this->reset   = decode_wav_reset;
    this->get_pcm = decode_wav_get_pcm;
    this->close   = decode_wav_close;

    this->private = mem_alloc(sizeof(*this->private), 0, MEM_ALLOC_TEMP);
    if (!this->private) {
        DMSG("Out of memory");
        return 0;
    }

    /* 先頭の2kBを取得してデータ形式を確認する。WAVファイルでヘッダ情報が
     * 2kBを超す場合は、多分壊れているかPCM以外の形式なのでエラー扱い */
    const uint8_t *data;
    const uint32_t len = decode_get_data(this, 0, 2048, &data);
    if (!scan_wav_header(this, data, len)) {
        mem_free(this->private);
        return 0;
    }

    this->private->sample_size = this->stereo ? 4 : 2;
    if (this->looplen > 0
     && this->private->len > this->loopstart + this->looplen
    ) {
        this->private->len = this->loopstart + this->looplen;
    }
    this->private->pos = 0;

    return 1;
}

/*************************************************************************/

/**
 * decode_wav_reset:  音声データを初めから再生するための準備を行う。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
static void decode_wav_reset(SoundDecodeHandle *this)
{
    this->private->pos = 0;
}

/*************************************************************************/

/**
 * decode_wav_get_pcm:  現在の再生位置からPCMデータを取得し、再生位置を
 * 進める。PCMバッファが一杯になる前に音声データの終端に到達した場合、無音
 * データでバッファの残りを埋める。
 *
 * 【引　数】pcm_buffer: PCMデータ（S16LE型）を格納するバッファ
 * 　　　　　   pcm_len: 取得するサンプル数
 * 【戻り値】取得できたサンプル数
 */
static uint32_t decode_wav_get_pcm(SoundDecodeHandle *this,
                                   int16_t *pcm_buffer, uint32_t pcm_len)
{
    SoundDecodePrivate *private = this->private;
    uint32_t copied = 0;

    while (copied < pcm_len) {
        if (private->pos < private->len) {
            uint32_t to_copy =
                ubound(pcm_len - copied, private->len - private->pos);
            const uint8_t *data;
            const uint32_t len = decode_get_data(
                this,
                private->data_offset + private->pos * private->sample_size,
                to_copy * private->sample_size, &data
            );
            if (len != to_copy * private->sample_size) {
                DMSG("Short read (wanted %u, got %u)",
                     to_copy * private->sample_size, len);
                to_copy = len / private->sample_size;
                if (!to_copy) {
                    break;
                }
            }
            memcpy((uint8_t *)pcm_buffer + copied * private->sample_size,
                   data, to_copy * private->sample_size);
            copied += to_copy;
            private->pos += to_copy;
        }
        if (private->pos >= private->len) {
            if (this->looplen != 0) {
                private->pos = this->loopstart;
            } else {  // ループなし
                break;
            }
        }
    }

    return copied;
}

/*************************************************************************/

/**
 * decode_wav_close:  デコードを終了し、関連リソースを破棄する。ソース
 * データに関しては、メモリバッファは破棄されないが、ファイルからストリー
 * ミングされている場合はファイルもクローズされる。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
static void decode_wav_close(SoundDecodeHandle *this)
{
    mem_free(this->private);
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * scan_wav_header:  音声データの先頭部分をスキャンして、S16LE型PCM音声の
 * 入ったRIFF WAVEファイルであることを確認する。確認できた場合、fmtヘッダ
 * からチャンネル数と周波数をオブジェクトに記録する。
 *
 * 【引　数】buffer: スキャンするデータが入っているバッファ
 * 　　　　　buflen: データ長（バイト）
 * 【戻り値】0以外＝有効なWAVファイルである、0＝有効なWAVファイルではない
 */
static int scan_wav_header(SoundDecodeHandle *this, const uint8_t *buffer,
                           uint32_t buflen)
{
    /* ファイルヘッダをまずチェックする */

    if (memcmp(buffer, "RIFF", 4) != 0 || memcmp(buffer+8, "WAVE", 4) != 0) {
        DMSG("Data is not a RIFF WAVE");
        return 0;
    }

    /* fmtとdataチャンクを探す。処理簡略化のため、dataチャンク以降は
     * チェックせず、fmtがdataより後に来るファイルには対応しない。
     * （そんなものは見たことがないが） */

    uint32_t fmt_offset = 0, data_offset = 0;
    uint32_t fmt_size = 0, data_size = 0;
    uint32_t pos = 12;
    while (!data_offset && pos < buflen) {
        const uint32_t chunk_size = buffer[pos+4] <<  0
                                  | buffer[pos+5] <<  8
                                  | buffer[pos+6] << 16
                                  | buffer[pos+7] << 24;
        if (memcmp(buffer + pos, "fmt ", 4) == 0) {
            fmt_offset = pos + 8;
            fmt_size = chunk_size;
        } else if (memcmp(buffer + pos, "data", 4) == 0) {
            data_offset = pos + 8;
            data_size = chunk_size;
        }
        pos += 8 + chunk_size;
    }
    if (!fmt_offset || !data_offset) {
        DMSG("'%s' chunk not found in data", !fmt_offset ? "fmt " : "data");
        return 0;
    }

    /* fmtチャンクのデータを解析する */

    if (fmt_size < 16) {
        DMSG("'fmt ' chunk too small (%u, must be at least 16)", fmt_size);
        return 0;
    }
    unsigned int format   = buffer[fmt_offset+ 0] | buffer[fmt_offset+ 1]<<8;
    unsigned int channels = buffer[fmt_offset+ 2] | buffer[fmt_offset+ 3]<<8;
    unsigned int bits     = buffer[fmt_offset+14] | buffer[fmt_offset+15]<<8;
    uint32_t freq = buffer[fmt_offset+4] <<  0
                  | buffer[fmt_offset+5] <<  8
                  | buffer[fmt_offset+6] << 16
                  | buffer[fmt_offset+7] << 24;
    if (format != 0x0001) {
        DMSG("Audio format 0x%X not supported", format);
        return 0;
    }
    if (channels != 1 && channels != 2) {
        DMSG("%u channels not supported", channels);
        return 0;
    }
    if (bits != 16) {
        DMSG("%u-bit samples not supported", bits);
        return 0;
    }

    /* 成功したのでオブジェクトデータを更新する */

    this->stereo = (channels == 2);
    this->native_freq = freq;
    this->private->data_offset = data_offset;
    if (data_size > 0 && data_size < (this->datalen - data_offset)) {
        this->private->len = data_size / (2*channels);
    } else {
        this->private->len = (this->datalen - data_offset) / (2*channels);
    }

    return 1;
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
