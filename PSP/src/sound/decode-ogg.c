/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sound/decode-ogg.c: Audio decoder for Ogg Vorbis data.
 */

#include "../common.h"
#include "../memory.h"
#include "../sound.h"
#include "decode.h"

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>
#include <errno.h>

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* vorbisfile用コールバック */

static size_t ogg_read(void *ptr, size_t size, size_t nmemb,
                           void *datasource);
static int ogg_seek(void *datasource, ogg_int64_t offset, int whence);
static long ogg_tell(void *datasource);

static const ov_callbacks ogg_callbacks = {
    .read_func  = ogg_read,
    .seek_func  = ogg_seek,
    .close_func = NULL,
    .tell_func  = ogg_tell,
};

/*-----------------------------------------------------------------------*/

/* プライベートデータ構造体 */
struct SoundDecodePrivate_ {
    OggVorbis_File vf;
    uint32_t filepos;  // libvorbisfile用読み込み位置
};

/*-----------------------------------------------------------------------*/

/* ローカル関数宣言 */

static void decode_ogg_reset(SoundDecodeHandle *this);
static uint32_t decode_ogg_get_pcm(SoundDecodeHandle *this,
                                   int16_t *pcm_buffer, uint32_t pcm_len);
static void decode_ogg_close(SoundDecodeHandle *this);

/*************************************************************************/
/***************************** メソッド実装 ******************************/
/*************************************************************************/

/**
 * decode_ogg_open:  Ogg Vorbis形式の音声データのデコードを開始する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
int decode_ogg_open(SoundDecodeHandle *this)
{
    this->reset   = decode_ogg_reset;
    this->get_pcm = decode_ogg_get_pcm;
    this->close   = decode_ogg_close;

    this->private = mem_alloc(sizeof(*this->private), 0,
                              MEM_ALLOC_TEMP | MEM_ALLOC_CLEAR);
    if (!this->private) {
        DMSG("Out of memory");
        return 0;
    }

    if (ov_open_callbacks(this, &this->private->vf, NULL, 0,
                          ogg_callbacks) != 0) {
        DMSG("ov_open() failed");
        return 0;
    }

    vorbis_info *info = ov_info(&this->private->vf, -1);
    if (!info) {
        DMSG("ov_info() failed");
        ov_clear(&this->private->vf);
        return 0;
    }
    if (info->channels == 1) {
        this->stereo = 0;
    } else if (info->channels == 2) {
        this->stereo = 1;
    } else {
        DMSG("Bad channel count %d", info->channels);
        ov_clear(&this->private->vf);
        return 0;
    }
    this->native_freq = info->rate;

    this->private->filepos = 0;

    return 1;
}

/*************************************************************************/

/**
 * decode_ogg_reset:  音声データを初めから再生するための準備を行う。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
static void decode_ogg_reset(SoundDecodeHandle *this)
{
    ov_pcm_seek(&this->private->vf, 0);
}

/*************************************************************************/

/**
 * decode_ogg_get_pcm:  現在の再生位置からPCMデータを取得し、再生位置を
 * 進める。PCMバッファが一杯になる前に音声データの終端に到達した場合、無音
 * データでバッファの残りを埋める。
 *
 * 【引　数】pcm_buffer: PCMデータ（S16LE型）を格納するバッファ
 * 　　　　　   pcm_len: 取得するサンプル数
 * 【戻り値】取得できたサンプル数
 */
static uint32_t decode_ogg_get_pcm(SoundDecodeHandle *this,
                                   int16_t *pcm_buffer, uint32_t pcm_len)
{
    SoundDecodePrivate *private = this->private;
    const int channels = this->stereo ? 2 : 1;
    const uint32_t loopend = this->loopstart + this->looplen;

    uint32_t copied = 0;
    while (copied < pcm_len) {

        uint32_t toread = pcm_len - copied;
        if (this->looplen > 0) {
            toread = min(toread, loopend - ov_pcm_tell(&private->vf));
        }

        int bitstream_unused;
        const int32_t thisread = ov_read(
            &private->vf, (char *)(pcm_buffer + (copied * channels)),
            (pcm_len - copied) * (2*channels),
            /*bigendianp*/ 0, /*word*/ 2, /*sgned*/ 1, &bitstream_unused
        );
        if (thisread == 0 || thisread == OV_EOF) {
            /* 音声データの終端に到達した */
            if (this->looplen < 0) {
                ov_pcm_seek(&this->private->vf, this->loopstart);
            } else {
                break;
            }
        } else if (thisread == OV_HOLE) {
            DMSG("Warning: decompression error, data dropped");
            continue;
        } else if (thisread < 0) {
            DMSG("Decompression error: %d", thisread);
            break;
        } else {
            copied += thisread / (2*channels);
        }

        if (this->looplen > 0 && ov_pcm_tell(&private->vf) >= loopend)  {
            ov_pcm_seek(&this->private->vf, this->loopstart);
        }

    }  // while (copied < pcm_len)

    return copied;
}

/*************************************************************************/

/**
 * decode_ogg_close:  デコードを終了し、関連リソースを破棄する。ソース
 * データに関しては、メモリバッファは破棄されないが、ファイルからストリー
 * ミングされている場合はファイルもクローズされる。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
static void decode_ogg_close(SoundDecodeHandle *this)
{
    ov_clear(&this->private->vf);
    mem_free(this->private);
}

/*************************************************************************/
/************************ libvorbisfile用I/O関数 *************************/
/*************************************************************************/

/**
 * ogg_read:  libvorbisfile用読み込みコールバック関数。
 *
 * 【引　数】        ptr: 読み込みバッファ
 * 　　　　　size, nmemb: 読み込みデータ量（size*nmembバイト）
 * 　　　　　 datasource: データ源（ここではSoundDecodeHandle構造体ポインタ）
 * 【戻り値】読み込んだデータ量（sizeバイト単位）。エラーの場合、errnoを設定
 * 　　　　　して0を返す
 */
static size_t ogg_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    if (!ptr || size <= 0 || nmemb <= 0 || !datasource) {
        DMSG("Invalid parameters: %p %d %d %p", ptr, (int)size, (int)nmemb,
             datasource);
        errno = EINVAL;
        return 0;
    }
    SoundDecodeHandle *handle = (SoundDecodeHandle *)datasource;

    const uint8_t *data;
    const uint32_t nread =
        decode_get_data(handle, handle->private->filepos, size * nmemb, &data);
    memcpy(ptr, data, nread);
    handle->private->filepos += nread;
    errno = 0;
    return nread / size;
}

/*************************************************************************/

/**
 * ogg_seek:  libvorbisfile用読み込み位置設定コールバック関数。
 *
 * 【引　数】datasource: データ源（ここではSoundDecodeHandle構造体ポインタ）
 * 　　　　　    offset: 新しい読み込み位置
 * 　　　　　    whence: offsetの解釈方法
 * 【戻り値】0＝成功、-1＝失敗
 */
static int ogg_seek(void *datasource, ogg_int64_t offset, int whence)
{
    if (!datasource) {
        DMSG("datasource == NULL");
        return -1;
    }
    SoundDecodeHandle *handle = (SoundDecodeHandle *)datasource;

    if (whence == SEEK_CUR) {
        offset += handle->private->filepos;
    } else if (whence == SEEK_END) {
        offset += handle->datalen;
    }
    handle->private->filepos = bound(offset, 0, handle->datalen);
    return 0;
}

/*************************************************************************/

/**
 * ogg_tell:  libvorbisfile用読み込み位置取得コールバック関数。
 *
 * 【引　数】datasource: データ源（ここではSoundDecodeHandle構造体ポインタ）
 * 【戻り値】読み込み位置（バイト単位）、-1＝エラー
 */
static long ogg_tell(void *datasource)
{
    if (!datasource) {
        DMSG("datasource == NULL");
        return -1;
    }
    SoundDecodeHandle *handle = (SoundDecodeHandle *)datasource;

    return handle->private->filepos;
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
