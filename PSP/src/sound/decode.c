/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sound/decode.c: Audio data decoding interface.
 */

#include "../common.h"
#include "../memory.h"
#include "../sound.h"
#include "../sysdep.h"
#include "decode.h"

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/**
 * READ_BUFFER_SIZE:  ファイルからの読み込みに使うバッファのサイズ（バイト）。
 */
#define READ_BUFFER_SIZE  16384

/**
 * DECODE_INTERPOLATE:  定義すると、周波数変換時にサンプルの補間処理を行う。
 * 当然ながら、再生処理が重くなる。
 */
#define DECODE_INTERPOLATE

/*************************************************************************/

/* サポートされている音声データ形式と対応するopen()関数のマッピング。
 * システム依存処理からsound_decode_set_handler()を呼び出すことで変更
 * できる。 */

static struct {
    const SoundFormat format;
    SoundDecodeOpenFunc *open;
} decode_handlers[] = {
    {SOUND_FORMAT_WAV, decode_wav_open},
    {SOUND_FORMAT_MP3, NULL},  // システム共通ハンドラなし
    {SOUND_FORMAT_OGG, decode_ogg_open},
};

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * sound_decode_set_handler:  指定された音声データ形式のデコーダモジュール
 * を登録する。
 *
 * 【引　数】   format: 音声データ形式（SOUND_FORMAT_*）
 * 　　　　　open_func: デコード開始関数
 * 【戻り値】なし
 */
void sound_decode_set_handler(SoundFormat format,
                              SoundDecodeOpenFunc *open_func)
{
    unsigned int i;
    for (i = 0; i < lenof(decode_handlers); i++) {
        if (decode_handlers[i].format == format) {
            decode_handlers[i].open = open_func;
        }
    }
}

/*************************************************************************/

/**
 * sound_decode_open:  メモリバッファに格納されている音声データのデコードを
 * 開始する。
 *
 * 【引　数】   format: 音声データ形式（SOUND_FORMAT_*定数）
 * 　　　　　     data: 音声データバッファ
 * 　　　　　  datalen: 音声データ長（バイト）
 * 　　　　　loopstart: ループ開始位置（サンプル）
 * 　　　　　  looplen: ループ長（サンプル）、0＝ループなし、
 * 　　　　　           　　負数＝データ終端でループ
 * 　　　　　     freq: PCMデータの出力周波数
 * 【戻り値】音声デコードハンドル（エラーの場合はNULL）
 */
SoundDecodeHandle *sound_decode_open(SoundFormat format,
                                     const void *data, uint32_t datalen,
                                     uint32_t loopstart, int32_t looplen,
                                     uint32_t freq)
{
    if (!data || datalen == 0 || freq == 0) {
        DMSG("Invalid parameters: 0x%X %p %u %u %u %u",
             format, data, datalen, loopstart, looplen, freq);
        goto error_return;
    }

    SoundDecodeHandle *this = mem_alloc(sizeof(*this), 0,
                                        MEM_ALLOC_TEMP | MEM_ALLOC_CLEAR);
    this->data        = data;
    this->datalen     = datalen;
    this->loopstart   = loopstart;
    this->looplen     = looplen;
    this->output_freq = freq;

    int found = 0;
    unsigned int i;
    for (i = 0; i < lenof(decode_handlers) && !found; i++) {
        if (format == decode_handlers[i].format
         && decode_handlers[i].open != NULL
        ) {
            if (!(*decode_handlers[i].open)(this)) {
                goto error_free_this;
            }
            found = 1;
        }
    }
    if (!found) {
        DMSG("Unsupported format 0x%X", format);
        mem_free(this);
        return NULL;
    }

    /* 出力周波数が録音周波数と違う場合、get_pcm()で周波数変換を行う必要が
     * あるので、バッファ確保などの準備を行う */
    if (!(this->native_freq == 0 || this->native_freq == this->output_freq)) {
        this->need_resample = 1;
        const unsigned int sample_size = (this->stereo ? 4 : 2);
        const uint32_t bufsize = RESAMPLE_BUFLEN * sample_size;
        this->resample_buf = mem_alloc(bufsize, 0, MEM_ALLOC_TEMP);
        if (!this->resample_buf) {
            DMSG("Out of memory for resample buffer");
            goto error_close_decoder;
        }
        if (!this->get_pcm(this, this->resample_buf, RESAMPLE_BUFLEN)) {
            this->resample_eof = 1;
        }
    }

    return this;

  error_close_decoder:
    this->close(this);
  error_free_this:
    mem_free(this);
  error_return:
    return NULL;
}

/*************************************************************************/

/**
 * sound_decode_open_from_file:  データファイルに格納されている音声データの
 * デコードを開始する。渡されたファイルハンドルはデコーダが専用するので、
 * この関数を呼び出した後に再利用してはならない。ファイルハンドルはデコード
 * 終了時、またはこの関数が失敗したときに自動的にクローズされる。
 *
 * 【引　数】   format: 音声データ形式（SOUND_FORMAT_*定数）
 * 　　　　　       fp: 音声データファイルハンドル
 * 　　　　　  dataofs: ファイル内の音声データのオフセット（バイト）
 * 　　　　　  datalen: 音声データのデータ長（バイト）
 * 　　　　　loopstart: ループ開始位置（サンプル）
 * 　　　　　  looplen: ループ長（サンプル）、0＝ループなし、
 * 　　　　　           　　負数＝データ終端でループ
 * 　　　　　     freq: PCMデータの出力周波数
 * 【戻り値】音声デコードハンドル（エラーの場合はNULL）
 */
SoundDecodeHandle *sound_decode_open_from_file(
    SoundFormat format, SysFile *fp,
    uint32_t dataofs, uint32_t datalen,
    uint32_t loopstart, int32_t looplen, uint32_t freq)
{
    if (!fp || freq == 0) {
        DMSG("Invalid parameters: 0x%X %p %u %u %u %u %u",
             format, fp, dataofs, datalen, loopstart, looplen, freq);
        goto error_close_file;
    }

    SoundDecodeHandle *this = mem_alloc(sizeof(*this), 0,
                                        MEM_ALLOC_TEMP | MEM_ALLOC_CLEAR);
    this->fp          = fp;
    this->dataofs     = dataofs;
    this->datalen     = datalen;
    this->loopstart   = loopstart;
    this->looplen     = looplen;
    this->output_freq = freq;

    /* データファイルの場合、読み込みバッファが必要 */
    this->read_buffer = mem_alloc(READ_BUFFER_SIZE, 0, MEM_ALLOC_TEMP);
    if (!this->read_buffer) {
        DMSG("Out of memory for read buffer");
        goto error_free_this;
    }
    this->read_async_req = sys_file_read_async(this->fp, this->read_buffer,
                                               READ_BUFFER_SIZE, this->dataofs);
    if (!this->read_async_req) {
        DMSG("Async read failed: %s", sys_last_errstr());
        goto error_free_read_buffer;
    }
    this->read_buffer_pos = 0;
    this->read_buffer_len = 0;
    this->read_async_ofs = 0;

    int found = 0;
    unsigned int i;
    for (i = 0; i < lenof(decode_handlers) && !found; i++) {
        if (format == decode_handlers[i].format
         && decode_handlers[i].open != NULL
        ) {
            if (!(*decode_handlers[i].open)(this)) {
                goto error_free_read_buffer;
            }
            found = 1;
        }
    }
    if (!found) {
        DMSG("Unsupported format 0x%X", format);
        mem_free(this);
        return NULL;
    }

    if (!(this->native_freq == 0 || this->native_freq == this->output_freq)) {
        this->need_resample = 1;
        const unsigned int sample_size = (this->stereo ? 4 : 2);
        const uint32_t bufsize = RESAMPLE_BUFLEN * sample_size;
        this->resample_buf = mem_alloc(bufsize, 0, MEM_ALLOC_TEMP);
        if (!this->resample_buf) {
            DMSG("Out of memory for resample buffer");
            goto error_close_decoder;
        }
        if (!this->get_pcm(this, this->resample_buf, RESAMPLE_BUFLEN)) {
            this->resample_eof = 1;
        }
    }

    return this;

  error_close_decoder:
    this->close(this);
  error_free_read_buffer:
    mem_free(this->read_buffer);
  error_free_this:
    mem_free(this);
  error_close_file:
    sys_file_close(fp);
    return NULL;
}

/*************************************************************************/

/**
 * sound_decode_is_stereo:  音声データのステレオ・モノラルの別を返す。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝ステレオ、0＝モノラル
 */
int sound_decode_is_stereo(SoundDecodeHandle *this)
{
    PRECOND_SOFT(this != NULL, return 0);
    return this->stereo;
}

/*************************************************************************/

/**
 * sound_decode_reset:  音声データを初めから再生するための準備を行う。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sound_decode_reset(SoundDecodeHandle *this)
{
    PRECOND_SOFT(this != NULL, return);
    this->reset(this);
    if (this->need_resample) {
        this->resample_eof = 0;
        this->resample_pos = 0;
        if (!this->get_pcm(this, this->resample_buf, RESAMPLE_BUFLEN)) {
            this->resample_eof = 1;
        }
    }
}

/*************************************************************************/

/**
 * sound_decode_get_pcm:  現在の再生位置からPCMデータを取得し、再生位置を
 * 進める。PCMバッファが一杯になる前に音声データの終端に到達した場合、無音
 * データでバッファの残りを埋める。
 *
 * 【引　数】pcm_buffer: PCMデータ（S16LE型）を格納するバッファ
 * 　　　　　   pcm_len: 取得するサンプル数
 * 【戻り値】0以外＝成功、0＝失敗（データ終端またはデコードエラー）
 */
int sound_decode_get_pcm(SoundDecodeHandle *this, int16_t *pcm_buffer,
                         uint32_t pcm_len)
{
    PRECOND_SOFT(this != NULL, return 0);
    if (pcm_buffer == NULL || pcm_len == 0) {
        DMSG("Invalid parameters: (this=%p) %p %u", this, pcm_buffer, pcm_len);
        return 0;
    }

    const int stereo = this->stereo;
    const unsigned int sample_size = (stereo ? 4 : 2);

    if (!this->need_resample) {
        const uint32_t got = this->get_pcm(this, pcm_buffer, pcm_len);
        if (got == 0) {
            return 0;
        }
        this->samples_gotten += got;
        if (got < pcm_len) {
            mem_clear((uint8_t *)pcm_buffer + got * sample_size,
                      (pcm_len - got) * sample_size);
        }
        return 1;
    }

    if (this->resample_eof) {
        return 0;
    }

    const uint32_t native_freq = this->native_freq;
    const uint32_t output_freq = this->output_freq;

    uint32_t copied;
    for (copied = 0; copied < pcm_len; copied++) {

        if (stereo) {
            const int16_t this_l = this->resample_buf[this->resample_pos*2+0];
            const int16_t this_r = this->resample_buf[this->resample_pos*2+1];
#ifdef DECODE_INTERPOLATE
            pcm_buffer[copied*2+0] = this->last_l
                + ((this_l - this->last_l) * (int32_t)this->pos_frac
                                           / (int32_t)output_freq);
            pcm_buffer[copied*2+1] = this->last_r
                + ((this_r - this->last_r) * (int32_t)this->pos_frac
                                           / (int32_t)output_freq);
#else
            pcm_buffer[copied*2+0] = this_l;
            pcm_buffer[copied*2+1] = this_r;
#endif
        } else {
            const int16_t this_l = this->resample_buf[this->resample_pos];
#ifdef DECODE_INTERPOLATE
            pcm_buffer[copied] = this->last_l
                + ((this_l - this->last_l) * (int32_t)this->pos_frac
                                           / (int32_t)output_freq);
#else
            pcm_buffer[copied] = this_l;
#endif
        }

        this->pos_frac += native_freq;
        while (this->pos_frac >= output_freq) {
#ifdef DECODE_INTERPOLATE
            if (stereo) {
                this->last_l = this->resample_buf[this->resample_pos*2+0];
                this->last_r = this->resample_buf[this->resample_pos*2+1];
            } else {
                this->last_l = this->resample_buf[this->resample_pos];
            }
#endif
            this->pos_frac -= output_freq;
            this->resample_pos++;
            if (this->resample_pos >= RESAMPLE_BUFLEN) {
                const uint32_t got =
                    this->get_pcm(this, this->resample_buf, RESAMPLE_BUFLEN);
                if (got == 0) {
                    this->resample_eof = 1;
                    goto break_copy_loop;
                }
                if (got < RESAMPLE_BUFLEN) {
                    mem_clear(this->resample_buf + got * sample_size,
                              (RESAMPLE_BUFLEN - got) * sample_size);
                }
                this->resample_pos = 0;
            }
        }

        this->samples_gotten += copied;

    }  // for (copied = 0; copied < pcm_len; copied++)
  break_copy_loop:

    if (copied == 0) {
        return 0;
    }

    if (copied < pcm_len) {
        mem_clear((uint8_t *)pcm_buffer + copied * sample_size,
                  (pcm_len - copied) * sample_size);
    }

    return 1;
}

/*************************************************************************/

/**
 * sound_decode_get_position:  get_pcm()が次に返すPCMデータの時間的位置を
 * 秒単位で返す。
 *
 * 【引　数】なし
 * 【戻り値】デコード位置（秒）
 */
float sound_decode_get_position(SoundDecodeHandle *this)
{
    return (float)this->samples_gotten / (float)this->output_freq;
}

/*************************************************************************/

/**
 * sound_decode_close:  デコードを終了し、関連リソースを破棄する。ソース
 * データに関しては、メモリバッファは破棄されないが、ファイルからストリー
 * ミングされている場合はファイルもクローズされる。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sound_decode_close(SoundDecodeHandle *this)
{
    PRECOND_SOFT(this != NULL, return);

    this->close(this);
    mem_free(this->resample_buf);
    mem_free(this->read_buffer);
    sys_file_close(this->fp);
    mem_free(this);
}

/*************************************************************************/
/********************** デコードメソッド用補助関数 ***********************/
/*************************************************************************/

/**
 * decode_get_data:  音声データの指定部分を返す。decode.cのREAD_BUFFER_SIZE
 * 定数を超えるデータ量を一回の呼び出しで取得することはできない。
 *
 * 【引　数】    pos: 取得するデータのバイトオフセット
 * 　　　　　    len: 取得するデータ量（バイト）
 * 　　　　　ptr_ret: データへのポインタを格納する変数へのポインタ
 * 【戻り値】取得できたバイト数
 */
uint32_t decode_get_data(SoundDecodeHandle *this, uint32_t pos,
                         uint32_t len, const uint8_t **ptr_ret)
{
    PRECOND_SOFT(this != NULL, return 0);
    PRECOND_SOFT(ptr_ret != NULL, return 0);

    if (pos >= this->datalen) {
        *ptr_ret = NULL;
        return 0;
    }

    if (len > this->datalen - pos) {
        len = this->datalen - pos;
    }

    if (this->data) {

        *ptr_ret = this->data + pos;

    } else {  // !this->data（つまりファイルソース）

        /* バッファサイズ以上を一時に読み込めない */

        if (len > READ_BUFFER_SIZE) {
            len = READ_BUFFER_SIZE;
        }


        if (this->read_async_req
         && pos >= this->read_buffer_pos
         && pos + len <= this->read_buffer_pos + READ_BUFFER_SIZE
         && pos + len - this->read_buffer_pos > this->read_async_ofs
        ) {
            int32_t nread = sys_file_wait_async(this->read_async_req);
            this->read_async_req = 0;
            if (nread < 0) {
                DMSG("sys_file_wait_async(%d[%p,%u,%u]): %s",
                     this->read_async_req, this->fp,
                     this->read_buffer_pos + this->read_async_req,
                     READ_BUFFER_SIZE - this->read_async_ofs,
                     sys_last_errstr());
                nread = 0;
            }
            this->read_buffer_len = this->read_async_ofs + nread;
        }

        /* 指定された部分がバッファに入っていない場合、必要な部分を即時に
         * 読み込む */

        if (pos < this->read_buffer_pos
         || pos + len > this->read_buffer_pos + this->read_buffer_len
        ) {
            if (this->read_async_req) {
                (void) sys_file_wait_async(this->read_async_req);
                this->read_async_req = 0;
            }
            this->read_buffer_pos = pos;
            if (sys_file_seek(this->fp, pos + this->dataofs,
                              FILE_SEEK_SET) < 0) {
                DMSG("sys_file_seek(%p,%u,FILE_SEEK_SET): %s", this->fp, pos,
                     sys_last_errstr());
                this->read_buffer_len = 0;
                return 0;
            }
            int32_t nread = sys_file_read(this->fp, this->read_buffer, len);
            if (nread < 0) {
                DMSG("sys_file_read(%p,%u,%u): %s", this->fp, pos, len,
                     sys_last_errstr());
                this->read_buffer_len = 0;
                return 0;
            }
            this->read_buffer_len = nread;
            len = nread;
        }

        /* 指定された部分が先読みに掛かっている場合、先読みの完了を待つ */
        /* 指定された部分がバッファの後半から始まる場合、先頭に移動させる。
         * ただし非同期読み込み中は移動を行わない（読み込み中のデータを
         * 移動できないため） */

        if (!this->read_async_req
         && pos >= this->read_buffer_pos + READ_BUFFER_SIZE/2
        ) {
            const uint32_t ofs = pos - this->read_buffer_pos;
            memmove(this->read_buffer, this->read_buffer + ofs,
                    this->read_buffer_len - ofs);
            this->read_buffer_pos += ofs;
            this->read_buffer_len -= ofs;
        }

        /* 非同期読み込み中でなく、かつバッファが一杯になっていない場合、
         * 残っている部分を先読みで埋める */

        if (!this->read_async_req && this->read_buffer_len < READ_BUFFER_SIZE) {
            const uint32_t buffer_end =
                this->read_buffer_pos + this->read_buffer_len;
            const uint32_t toread =
                ubound(READ_BUFFER_SIZE - this->read_buffer_len,
                       this->datalen - buffer_end);
            this->read_async_req = sys_file_read_async(
                this->fp, this->read_buffer + this->read_buffer_len, toread,
                this->dataofs + this->read_buffer_pos + this->read_buffer_len
            );
            if (!this->read_async_req) {
                DMSG("sys_file_read_async(%p,%u,%u): %s", this->fp,
                     this->read_buffer_pos + this->read_buffer_len, toread,
                     sys_last_errstr());
            } else {
                this->read_async_ofs = this->read_buffer_len;
            }
        }

        /* これで必要なデータが読み込みバッファに入っているので返す */

        *ptr_ret = this->read_buffer + (pos - this->read_buffer_pos);

    }  // if (this->data)

    return len;
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
