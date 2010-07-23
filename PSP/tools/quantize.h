/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * tools/quantize.h: Header for color quantization routine.
 */

#ifndef TOOLS_QUANTIZE_H

/*************************************************************************/

/**
 * quantize_image:  32ビット／ピクセルの画像データを8ビット／ピクセルに変換
 * する。ディザーは行わない。
 *
 * 8bpp画像データで使う256色パレットのバッファ自体は必須だが、画像データを
 * 解析して、最適なパレットを自動的に生成することもできる。パレットの固定色
 * 数（fixed_colors引数）が256未満の場合、非固定のパレットインデックスを利
 * 用してパレットを生成する。なお、一部の色のみ固定する場合、パレットの先頭
 * （0番）から固定色を設定しなければならない。
 *
 * srcとdestに同じポインタを指定して構わない。
 *
 * 【引　数】          src: 32bpp画像データへのポインタ
 * 　　　　　   src_stride: srcのライン長（ピクセル単位）
 * 　　　　　         dest: 8bpp画像データバッファへのポインタ
 * 　　　　　  dest_stride: destのライン長（ピクセル単位）
 * 　　　　　width, height: 画像のサイズ（ピクセル単位）
 * 　　　　　      palette: 256色パレットバッファへのポインタ
 * 　　　　　 fixed_colors: パレットのうち、固定されている色
 * 　　　　　         slow: パレットを生成する場合、正確性と速度のバランス。
 * 　　　　　               　　0＝通常、0以外＝正確性重視だが非常に遅い
 * 【戻り値】0以外＝成功、0＝失敗
 * 【注　意】一時バッファとして、画像のピクセル数×16バイトの空きメモリが必要。
 */
extern int quantize_image(const uint32_t *src, const int32_t src_stride,
                          uint8_t *dest, const int32_t dest_stride,
                          const int32_t width, const int32_t height,
                          uint32_t *palette, const int fixed_colors,
                          const int slow);

/**
 * generate_palette:  画像データから最適な256色パレットを生成する。基本的に
 * Paul Heckbert氏が提唱したmedian cutアルゴリズムを利用するが（詳しくは同氏
 * の論文「Color Image Quantization for Frame Buffer Display」を参照）、以下
 * の点で異なる。
 * 　・色ヒストグラム生成時に色の精度を落とさない
 * 　・アルファ値を考慮する
 *
 * callback!=NULLの場合、約1秒間隔にその関数を呼び出す。
 *
 * 【引　数】    imageptr: 画像データポインタ（0xAARRGGBBまたは0xAABBGGRR形式）
 * 　　　　　       width: 画像の幅（ピクセル）
 * 　　　　　      height: 画像の高さ（ピクセル）
 * 　　　　　      stride: 画像のライン長（ピクセル）
 * 　　　　　     palette: 生成した色パレットを格納するバッファへのポインタ
 * 　　　　　fixed_colors: パレットのうち、固定されている色
 * 　　　　　    callback: 定期的に呼び出されるコールバック関数（NULL可）
 * 【引　数】なし
 */
extern void generate_palette(const uint32_t *imageptr, uint32_t width,
                             uint32_t height, uint32_t stride,
                             uint32_t *palette, int fixed_colors,
                             void (*callback)(void));

/**
 * generate_palette_slow:  画像データから最適な256色パレットを生成する。
 * generate_palette()よりかなり遅いが、生成されるパレットがより正確……
 * かもしれない。
 *
 * 【引　数】    imageptr: 画像データポインタ（0xAARRGGBBまたは0xAABBGGRR形式）
 * 　　　　　       width: 画像の幅（ピクセル）
 * 　　　　　      height: 画像の高さ（ピクセル）
 * 　　　　　      stride: 画像のライン長（ピクセル）
 * 　　　　　     palette: 生成した色パレットを格納するバッファへのポインタ
 * 　　　　　fixed_colors: パレットのうち、固定されている色
 * 【引　数】なし
 */
extern void generate_palette_slow(const uint32_t *imageptr, uint32_t width,
                                  uint32_t height, uint32_t stride,
                                  uint32_t *palette, int fixed_colors);

/*************************************************************************/

#endif  // TOOLS_QUANTIZE_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
