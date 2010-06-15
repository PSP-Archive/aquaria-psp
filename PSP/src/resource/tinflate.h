/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/resource/tinflate.h: Header for "deflate"-method decompression
 * functions.
 */

#ifndef RESOURCE_TINFLATE_H
#define RESOURCE_TINFLATE_H

/*************************************************************************/

/**
 * tinflate_state_size:  tinflate_partial()で使用する状態バッファのサイズを
 * 返す。
 *
 * 【引　数】なし
 * 【戻り値】状態バッファの必要サイズ（バイト）
 */
extern int tinflate_state_size(void);

/**
 * tinflate:  deflate方式で圧縮されたデータを解凍する。
 *
 * 【引　数】compressed_data: 入力バッファ
 * 　　　　　compressed_size: 入力データ長（バイト）
 * 　　　　　  output_buffer: 出力バッファ（output_size==0の場合は無視）
 * 　　　　　    output_size: 出力バッファサイズ（バイト、0で必要サイズを計算）
 * 　　　　　        crc_ret: CRC32値を格納する変数へのポインタ（NULL可）
 * 【戻り値】0以上＝成功（出力バイト数または必要バイト数）、負数＝エラー
 */
extern long tinflate(const void *compressed_data, long compressed_size,
                     void *output_buffer, long output_size,
                     unsigned long *crc_ret);

/**
 * tinflate_partial:  deflate方式で圧縮されたデータを一部分ずつ解凍する。
 *
 * 【引　数】  compressed_data: 入力バッファ
 * 　　　　　  compressed_size: 入力データ長（バイト）
 * 　　　　　    output_buffer: 出力バッファ（output_size==0の場合は無視）
 * 　　　　　      output_size: 出力バッファサイズ（バイト）
 * 　　　　　         size_ret: 出力バイト数を格納する変数へのポインタ (NULL可)
 * 　　　　　          crc_ret: CRC32値を格納する変数へのポインタ（NULL可）
 * 　　　　　     state_buffer: 状態保持バッファ（最初の呼び出し前に0クリア）
 * 　　　　　state_buffer_size: 状態保持バッファサイズ（バイト）
 * 【戻り値】0＝成功、正数＝入力データ切れ、負数＝エラー
 */
extern int tinflate_partial(const void *compressed_data, long compressed_size,
                            void *output_buffer, long output_size,
                            unsigned long *size_ret, unsigned long *crc_ret,
                            void *state_buffer, long state_buffer_size);

/*************************************************************************/

#endif  // RESOURCE_DECOMP_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
