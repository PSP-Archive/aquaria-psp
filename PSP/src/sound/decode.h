/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sound/decode.h: Internal header for audio decoding handlers.
 */

#ifndef SOUND_DECODE_H
#define SOUND_DECODE_H

/*************************************************************************/

/*
 * RESAMPLE_BUFLEN:  周波数変換を行う際の一時PCMバッファの長さ（サンプル）。
 */
#define RESAMPLE_BUFLEN  1024

/*
 * SoundDecodePrivate:  各音声データデコーダのプライベートデータを定義する
 * ための構造体。各デコーダのソースファイルで、そのデコーダ専用の構造体を
 * struct SoundDecodePrivate_ {...};で定義することで、handle->privateから
 * インスタンス毎のデータを参照できる。またデコーダソースファイル以外では
 * 定義されていないため、外部から中身を参照することはできない。
 */
typedef struct SoundDecodePrivate_ SoundDecodePrivate;

/*
 * SoundDecodeHandleの構造体定義（外部向けには定義しない）
 */
struct SoundDecodeHandle_ {
    /* 各メソッドの実装関数。get_pcm()は成功・失敗ではなく、取得できた
     * サンプル数を返す（pcm_len未満の場合はバッファをクリアする必要なし） */
    void (*reset)(SoundDecodeHandle *this);
    uint32_t (*get_pcm)(SoundDecodeHandle *this, int16_t *pcm_buffer,
                        uint32_t pcm_len);
    void (*close)(SoundDecodeHandle *this);

    /* open()に渡されたデータ */
    uint32_t dataofs;           // 音声データオフセット（バイト）
    uint32_t datalen;           // 音声データ長（バイト）
    int32_t loopstart;          // ループ開始位置（サンプル）
    int32_t looplen;            // ループ長（サンプル）、0＝ループなし、
                                // 　　負数＝データ終端でループ

    /* 音声パラメータ */
    int stereo;                 // 0以外＝ステレオ、0＝モノラル
    uint32_t native_freq;       // 音声データの録音周波数

    /* データソース（バッファやファイル） */
    const uint8_t *data;        // メモリバッファ（NULL＝ファイルソース）
    SysFile *fp;                // ファイルハンドル

    /* ファイル読み込みバッファ関連 */
    uint8_t *read_buffer;       // ファイル用読み込みバッファ
    uint32_t read_buffer_pos;   // バッファの先頭ファイルオフセット（バイト）
    uint32_t read_buffer_len;   // バッファの有効データ量（バイト）
    int read_async_req;         // 非同期読み込み要求識別子
    uint32_t read_async_ofs;    // 非同期読み込みのバッファ内開始位置（バイト）

    /* 取得済みサンプル数（get_position()用） */
    uint32_t samples_gotten;

    /* 周波数変換関連 */
    uint32_t output_freq;       // 再生周波数
    uint8_t need_resample;      // 0以外＝周波数を変換しなければならない
    uint8_t resample_eof;       // 0以外＝データ終端に到達
    uint32_t resample_pos;      // 現在の再生位置（サンプル）
    uint32_t pos_frac;          // 再生位置の端数部分（0〜output_freq-1）
    int16_t *resample_buf;      // 一時PCMバッファ（必要な場合のみ確保）
    int16_t last_l, last_r;     // 左右チャンネルの直前のサンプル（補間用）

    /* 各デコーダのプライベートデータへのポインタ */
    SoundDecodePrivate *private;
};

/*-----------------------------------------------------------------------*/

/**
 * decode_get_data:  音声データの指定部分を返す。decode.cのREAD_BUFFER_SIZE
 * 定数を超えるデータ量を一回の呼び出しで取得することはできない。
 *
 * 【引　数】    pos: 取得するデータのバイトオフセット
 * 　　　　　    len: 取得するデータ量（バイト）
 * 　　　　　ptr_ret: データへのポインタを格納する変数へのポインタ
 * 【戻り値】取得できたバイト数
 */
extern uint32_t decode_get_data(SoundDecodeHandle *this, uint32_t pos,
                                uint32_t len, const uint8_t **ptr_ret);

/**
 * decode_*_open:  各デコーダの初期化処理を行う。少なくとも、オブジェクトの
 * メソッド関数ポインタを設定しなければならない。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */

/* RIFF WAVE形式PCMデコード関数（「デコード」というより「コピー」に近い） */
extern int decode_wav_open(SoundDecodeHandle *this);

/* Ogg Vorbisデコード関数 */
extern int decode_ogg_open(SoundDecodeHandle *this);

/*************************************************************************/

#endif  // SOUND_DECODE_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
