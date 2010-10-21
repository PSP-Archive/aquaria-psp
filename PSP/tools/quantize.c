/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * tools/quantize.c: Routine to quantize 32-bit-per-pixel images into
 * 8-bit-per-pixel indexed-color images.
 */

#include "../src/common.h"
#ifdef USE_SSE2
# include "sse2.h"
#endif

#include "quantize.h"

#include <time.h>

/*************************************************************************/

/* 画像の色情報を格納する構造体 */
struct colorinfo {
    uint32_t color;
    uint32_t count;
    uint32_t index;     // 暫定的のパレットインデックス
    int32_t nextuser;   // 同じパレットインデックスを使っている次のエントリー
};
static struct colorinfo *colortable;  // 動的確保

/*************************************************************************/

/* ローカル関数宣言 */

static uint32_t generate_colortable(const uint32_t *imageptr, uint32_t width,
                                    uint32_t height, uint32_t stride,
                                    const uint32_t *palette, int fixed_colors,
                                    void (*callback)(void));
static inline uint32_t colordiff_sq(uint32_t color1, uint32_t color2);

/*************************************************************************/
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
 * 逆に、パレットだけ生成して、画像データの変換自体が不要な場合はdest引数に
 * NULLを渡すことができる。
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
int quantize_image(const uint32_t *src, const int32_t src_stride,
                   uint8_t *dest, const int32_t dest_stride,
                   const int32_t width, const int32_t height,
                   uint32_t *palette, const int fixed_colors,
                   const int slow)
{
    if (src == NULL || width <= 0 || height <= 0) {
        return 0;
    }

    /* 必要に応じて色パレットを生成する */
    if (fixed_colors < 256) {
        memset(&palette[fixed_colors], 0, 4 * (256 - fixed_colors));
        if (slow) {
            generate_palette_slow(src, width, height, src_stride, palette,
                                  fixed_colors);
        } else {
            generate_palette(src, width, height, src_stride, palette,
                             fixed_colors, NULL);
        }
    }

    /* 画像データが不要な場合はここで終了 */
    if (dest == NULL) {
        return 1;
    }

    /* 画像データを変換する */
    int y;
    for (y = 0; y < height; y++) {
        const uint32_t *srcrow = &src[y * src_stride];
        uint8_t *destrow = &dest[y * dest_stride];
        int x;
        for (x = 0; x < width; x++) {
            /* 一番近い色を探す */
            const uint32_t pixel = srcrow[x];
            uint32_t best = 0xFFFFFFFF;
            int i;
            for (i = 0; i < 256; i++) {
                const uint32_t diff = colordiff_sq(pixel, palette[i]);
                if (diff < best) {
                    destrow[x] = i;
                    if (diff == 0) {
                        break;
                    } else {
                        best = diff;
                    }
                }
            }
        }
    }

    /* 成功 */
    return 1;
}

/*************************************************************************/
/*************************************************************************/

/* 色空間を分割するボックスの情報 */
struct colorbox {
    uint8_t rmin, rmax;
    uint8_t gmin, gmax;
    uint8_t bmin, bmax;
    uint8_t amin, amax;
    uint32_t ncolors;  // 入っている色の数
    uint32_t first;    // 入っている色の先頭colortable[]添え字。既存の
                       // 　ボックスを分割することでのみ、新しいボックス
                       // 　を生成するので、ボックス内の色は必ず連続する
};

/* generate_palette()の補助関数 */
static int compare_colors(const void * const a, const void * const b);
static void shrink_box(struct colorbox * const box);
static void split_box(struct colorbox * const box,
                      struct colorbox * const newbox);
static int compare_box(const void * const a, const void * const b);

/*************************************************************************/

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
void generate_palette(const uint32_t *imageptr, uint32_t width,
                      uint32_t height, uint32_t stride,
                      uint32_t *palette, int fixed_colors,
                      void (*callback)(void))
{
    PRECOND(imageptr != NULL);
    PRECOND(palette != NULL);

    int32_t i;

    /* 画像で使われている色を調べる */

    colortable = calloc(width * height, sizeof(*colortable));
    if (!colortable) {
        fprintf(stderr, "Out of memory!\n");
        return;
    }
    const uint32_t ncolors =
        generate_colortable(imageptr, width, height, stride,
                            palette, fixed_colors, callback);

    /* 全ての色がパレット内に収まれば、そのまま格納して戻る */

    if (ncolors <= 256 - fixed_colors) {
        for (i = 0; i < ncolors; i++) {
            palette[fixed_colors+i] = colortable[i].color;
        }
        free(colortable);
        colortable = NULL;
        return;
    }

    /* 色空間分割用ボックスを初期化する */

    struct colorbox box[256];
    box[0].rmin = 0; box[0].rmax = 255;
    box[0].gmin = 0; box[0].gmax = 255;
    box[0].bmin = 0; box[0].bmax = 255;
    box[0].amin = 0; box[0].amax = 255;
    box[0].ncolors = ncolors;
    box[0].first = 0;

    /* 必要な色数ができるまで、繰り返して色空間を分割する */

    for (i = 1; i < 256 - fixed_colors; i++) {

        /* 最も色数の多いボックスを探して、ボックス内の各成分の最小・最大値
         * を検出する。1色のみを囲むボックスはスキップする。全てのボックス
         * をスキップした場合、画像は元々256色以内だったことになるので、その
         * 時点で処理を終了する。 */
        int to_split;
        for (to_split = 0; to_split < i; to_split++) {
            if (box[to_split].ncolors <= 1) {
                continue;
            }
            shrink_box(&box[to_split]);
            break;
        }
        if (to_split >= i) {
            break;
        }

        /* このボックスの最も長い辺（範囲の広い成分）を基に、含まれている色
         * の中央値（median）で分割（cut）する */
        split_box(&box[to_split], &box[i]);

        /* ボックスを、色（ピクセル）の多い順に並べ替える */
        qsort(box, i+1, sizeof(*box), compare_box);
    }
    const unsigned int nboxes = i;

    /* 各ボックスの平均色を求めてパレットに格納する。ついでに、透明ピクセル
     * があるかどうかも調べておく */

    int have_transparent_pixel = 0;
    for (i = 0; i < nboxes; i++) {
        if (box[i].amin == 0) {
            have_transparent_pixel = 1;
        }
        uint32_t atot = 0, rtot = 0, gtot = 0, btot = 0;
        uint32_t pixels = 0, alpha_pixels = 0;
        uint32_t j;
        for (j = box[i].first; j < box[i].first + box[i].ncolors; j++) {
            const uint32_t color = colortable[j].color;
            const uint32_t count = colortable[j].count;
            const uint32_t alpha_count =
                lbound(((color>>24 & 0xFF) * count) / 255, 1);
            atot   += (color>>24 & 0xFF) * count;
            rtot   += (color>>16 & 0xFF) * alpha_count;
            gtot   += (color>> 8 & 0xFF) * alpha_count;
            btot   += (color>> 0 & 0xFF) * alpha_count;
            pixels += count;
            alpha_pixels += alpha_count;
        }
        palette[fixed_colors + i] = ((atot + pixels/2) / pixels) << 24
            | ((rtot + alpha_pixels/2) / alpha_pixels) << 16
            | ((gtot + alpha_pixels/2) / alpha_pixels) <<  8
            | ((btot + alpha_pixels/2) / alpha_pixels) <<  0;
    }

    /* 透明ピクセルがある場合、必ず透明色を1つ以上確保する */

    if (have_transparent_pixel) {
        int have_transparent_color = 0;
        for (i = 0; i < fixed_colors + nboxes; i++) {
            if (palette[i]>>24 == 0) {
                have_transparent_color = 1;
                break;
            }
        }
        if (!have_transparent_color) {
            /* もっともアルファ値の低い色を透明にする */
            int best = fixed_colors;
            for (i = fixed_colors+1; i < fixed_colors + nboxes; i++) {
                if (palette[i]>>24 < palette[best]>>24) {
                    best = i;
                }
            }
            palette[best] &= 0x00FFFFFF;
        }
    }

    free(colortable);
    colortable = NULL;
}

/*************************************************************************/

/* compare_colors()用の成分比較順序。各バイトは成分のシフト量を表し、位の
 * 低いバイトから順に比較を行う。例えば 0x10000818 の場合、最初にcolor>>24
 * （つまりアルファ）成分を比較し、アルファ成分が2つの色で異なっていれば
 * そこで比較が終了する。アルファ成分が同じであれば、次にcolor>>8（つまり
 * 緑）成分を比較する、それも同じならcolor>>0、color>>16の順に比較する。 */

static uint32_t color_compare_order;

/**
 * compare_colors:  colortable[]エントリーを成分別に比較する。qsort()の比較
 * 関数。
 *
 * 【引　数】a, b: 比較するエントリーへのポインタ
 * 【戻り値】a <  b: -1
 * 　　　　　a == b:  0
 * 　　　　　a >  b: +1
 */
static int compare_colors(const void * const a, const void * const b)
{
    const uint32_t color1 = ((const struct colorinfo *)a)->color;
    const uint32_t color2 = ((const struct colorinfo *)b)->color;
    uint32_t order = color_compare_order;
    unsigned int i;
    for (i = 0; i < 4; i++, order >>= 8) {
        const unsigned int shift = order & 0xFF;
        const unsigned int component1 = (color1 >> shift) & 0xFF;
        const unsigned int component2 = (color2 >> shift) & 0xFF;
        if (component1 < component2) {
            return -1;
        } else if (component1 > component2) {
            return +1;
        }
    }
    return 0;
}

/*************************************************************************/

/**
 * shrink_box:  色空間ボックスの各成分の最小・最大値を検出する。
 * generate_palette()の補助関数。
 *
 * 【引　数】box: 処理するボックスの情報構造体へのポインタ
 * 【戻り値】なし
 */
static void shrink_box(struct colorbox * const box)
{
    PRECOND(box != NULL);

    uint8_t rmin = box->rmax, rmax = box->rmin;
    uint8_t gmin = box->gmax, gmax = box->gmin;
    uint8_t bmin = box->bmax, bmax = box->bmin;
    uint8_t amin = box->amax, amax = box->amin;
    uint32_t i;
    for (i = box->first; i < box->first + box->ncolors; i++) {
        const uint8_t R = colortable[i].color>>16 & 0xFF;
        const uint8_t G = colortable[i].color>> 8 & 0xFF;
        const uint8_t B = colortable[i].color>> 0 & 0xFF;
        const uint8_t A = colortable[i].color>>24 & 0xFF;
        rmin = min(rmin, R); rmax = max(rmax, R);
        gmin = min(gmin, G); gmax = max(gmax, G);
        bmin = min(bmin, B); bmax = max(bmax, B);
        amin = min(amin, A); amax = max(amax, A);
    }
    box->rmin = rmin; box->rmax = rmax;
    box->gmin = gmin; box->gmax = gmax;
    box->bmin = bmin; box->bmax = bmax;
    box->amin = amin; box->amax = amax;
}

/*************************************************************************/

/**
 * split_box:  色空間ボックスを分割する。generate_palette()の補助関数。
 *
 * 【引　数】box: 分割するボックスの情報構造体へのポインタ
 * 【戻り値】なし
 */
static void split_box(struct colorbox * const box,
                      struct colorbox * const newbox)
{
    PRECOND(box != NULL);

    const uint8_t adiff = box->amax - box->amin;
    const uint8_t rdiff = box->rmax - box->rmin;
    const uint8_t gdiff = box->gmax - box->gmin;
    const uint8_t bdiff = box->bmax - box->bmin;
    uint8_t order[4] = {adiff, rdiff, gdiff, bdiff};
    uint8_t shift[4] = {24, 16, 8, 0};
    unsigned int i;
    for (i = 0; i < 3; i++) {
        unsigned int best = i;
        unsigned int j;
        for (j = i+1; j < 4; j++) {
            if (order[j] > order[best]) {
                best = j;
            }
        }
        if (best != i) {
            uint8_t temp;
            temp = order[i]; order[i] = order[best]; order[best] = temp;
            temp = shift[i]; shift[i] = shift[best]; shift[best] = temp;
        }
    }

    color_compare_order = shift[0] | shift[1]<<8 | shift[2]<<16 | shift[3]<<24;
    qsort(&colortable[box->first], box->ncolors, sizeof(*colortable),
          compare_colors);

    *newbox = *box;
    box->ncolors /= 2;
    newbox->first += box->ncolors;
    newbox->ncolors -= box->ncolors;
}

/*************************************************************************/

/**
 * compare_box:  色空間ボックスを、含まれている色の数で比較する。qsort()の
 * 比較関数。
 *
 * 【引　数】a, b: 比較するボックス情報構造体へのポインタ
 * 【戻り値】a <  b: -1
 * 　　　　　a == b:  0
 * 　　　　　a >  b: +1
 */
static int compare_box(const void * const a, const void * const b)
{
    const uint32_t count1 = ((const struct colorbox *)a)->ncolors;
    const uint32_t count2 = ((const struct colorbox *)b)->ncolors;
    return count1 > count2 ? -1 : count1 < count2 ? +1 : 0;  // 多い順にソート
}

/*************************************************************************/
/*************************************************************************/

/* 各パレットインデックスを使っている最初のcolortable[]インデックス */
static int32_t firstuser[256];

/* generate_palette_slow()の補助関数 */
static int compare_colortable(const void * const a, const void * const b);
static int select_color_1(uint32_t i, uint32_t *palette, int fixed_colors,
                          uint32_t *color_ret);
static int select_color_2(uint32_t i, uint32_t *palette, int fixed_colors,
                          int *second_ret, uint32_t *color_ret,
                          uint64_t *diff_ret);

/*************************************************************************/

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
// FIXME: handle the case where adjusting a color results in entry duplication
void generate_palette_slow(const uint32_t *imageptr, uint32_t width,
                           uint32_t height, uint32_t stride,
                           uint32_t *palette, int fixed_colors)
{
    PRECOND(imageptr != NULL);
    PRECOND(palette != NULL);

    /* 画像で使われている色を調べる */
    colortable = calloc(width * height, sizeof(*colortable));
    if (!colortable) {
        fprintf(stderr, "Out of memory!\n");
        return;
    }
    const uint32_t ncolors =
        generate_colortable(imageptr, width, height, stride,
                            palette, fixed_colors, NULL);

    /* テーブルを頻度順にソートする */
    qsort(colortable, ncolors, sizeof(*colortable), compare_colortable);

    /* 最もよく使われている色から初期の色パレットを生成する。固定色と一致
     * する場合は固定色を優先 */
    memset(firstuser, -1, sizeof(firstuser));
    uint32_t i;
    int index = fixed_colors;
    for (i = 0; i < ncolors && index < 256; i++) {
        int j;
        for (j = 0; j < index; j++) {
            if (colortable[i].color == palette[j]) {
                colortable[i].index = j;
                colortable[i].nextuser = firstuser[j];
                firstuser[j] = i;
                break;
            }
        }
        if (j >= index) {
            palette[index] = colortable[i].color;
            colortable[i].index = index;
            colortable[i].nextuser = -1;
            firstuser[index] = i;
            index++;
        }
    }

    /* 他の色に応じてパレットを調整する */

    for (; i < ncolors; i++) {

        /* パレット調整には二つの方法がある。
         * 　(1) 一つの色を選んで、処理中の色（i）に近づける
         * 　(2) 二つの色を統合して、空いたインデックスを処理中の色に割り当てる
         * 両方を試して、一番効果的な方法を選ぶ。 */
        int index_1, index_2;           // 各方法で選択したインデックス
        int index_mergeto;              // (2)の場合、統合先インデックス
        uint32_t color_1, color_2;      // 各方法で生成した色
        uint64_t diff_1, diff_2;        // 各方法で生じる誤差
        int32_t j;

        /* (1)の方法でインデックスを選択する */
        index_1 = select_color_1(i, palette, fixed_colors, &color_1);
        diff_1 = 0;
        for (j = firstuser[index_1]; j != -1; j = colortable[j].nextuser) {
            diff_1 += colordiff_sq(colortable[j].color, color_1);
        }
        diff_1 += colordiff_sq(colortable[i].color, color_1);

        /* (2)の方法でインデックスを選択する */
        index_2 = select_color_2(i, palette, fixed_colors,
                                 &index_mergeto, &color_2, &diff_2);

        /* 誤差の少ない方を選んで、パレットを更新する */
        if (diff_1 <= diff_2) {
            palette[index_1] = color_1;
            if (firstuser[index_1] < 0) {
                firstuser[index_1] = i;
            } else {
                j = firstuser[index_1];
                while (colortable[j].nextuser != -1) {
                    j = colortable[j].nextuser;
                }
                colortable[j].nextuser = i;
            }
            colortable[i].index = index_1;
            colortable[i].nextuser = -1;
        } else {
            palette[index_mergeto] = color_2;
            if (firstuser[index_2] >= 0) {
                j = firstuser[index_2];
                while (colortable[j].nextuser != -1) {
                    colortable[j].index = index_2;
                    j = colortable[j].nextuser;
                }
                colortable[j].index = index_2;
                colortable[j].nextuser = firstuser[index_mergeto];
            }
            palette[index_2] = colortable[i].color;
            firstuser[index_2] = i;
            colortable[i].index = index_2;
            colortable[i].nextuser = -1;
        }

    }  // for (; i < ncolors; i++)

    /* 色頻度テーブルを解放する */
    free(colortable);
    colortable = NULL;
}

/*************************************************************************/

/**
 * compare_colortable:  colortable[]エントリーを比較する。qsort()の比較関数。
 *
 * 【引　数】a, b: 比較するエントリーへのポインタ
 * 【戻り値】a <  b: -1
 * 　　　　　a == b:  0
 * 　　　　　a >  b: +1
 */
static int compare_colortable(const void * const a, const void * const b)
{
    const uint32_t count1 = ((const struct colorinfo *)a)->count;
    const uint32_t count2 = ((const struct colorinfo *)b)->count;
    return count1 > count2 ? -1 :  // 多い順にソートする
           count1 < count2 ? +1 : 0;
}

/*************************************************************************/

/**
 * select_color_1:  第1方法で色を選択する。パレットから、調整した時に出る
 * 誤差が一番小さいインデックスを選んで、インデックスと加重平均した色を返す。
 *
 * generate_palette()の補助関数。
 *
 * 【引　数】           i: colortable[]内、現在処理している色
 * 　　　　　     palette: 256色パレットへのポインタ
 * 　　　　　fixed_colors: パレットのうち、固定されている色
 * 　　　　　   color_ret: 戻り値のインデックスに設定すべき色
 * 【戻り値】変更すべきパレットインデックス（0以外）
 */
static int select_color_1(uint32_t i, uint32_t *palette, int fixed_colors,
                          uint32_t *color_ret)
{
    PRECOND(colortable != NULL);
    PRECOND(palette != NULL);
    PRECOND(color_ret != NULL);

    int best = -1;              // 最善インデックス
    uint32_t bestcolor = 0;     // bestを使った場合の平均色
    uint32_t bestdiff = ~0;     // bestを使った場合の誤差

    int index;
    for (index = 0; index < 256; index++) {
        uint32_t color;
        if (index < fixed_colors) {
            /* 固定色なので変更はできない。そのまま利用した場合の差を計算 */
            color = palette[index];
        } else {
            /* 固定色ではない */
            uint32_t a, r, g, b, rgbdiv, adiv;  // 成分別の合計と平均時の除数
            uint32_t amult;                     // RGB係数
            a = (colortable[i].color>>24 & 0xFF) * colortable[i].count;
            amult = (a+127) / 255;
            r = (colortable[i].color>>16 & 0xFF) * amult;
            g = (colortable[i].color>> 8 & 0xFF) * amult;
            b = (colortable[i].color>> 0 & 0xFF) * amult;
            adiv = colortable[i].count;
            rgbdiv = amult;
            int32_t j;
            for (j = firstuser[index]; j >= 0; j = colortable[j].nextuser) {
                uint32_t count = colortable[j].count;
                amult = (count * (colortable[j].color>>24 & 0xFF)) / 255;
                a += (colortable[j].color>>24 & 0xFF) * count;
                r += (colortable[j].color>>16 & 0xFF) * amult;
                g += (colortable[j].color>> 8 & 0xFF) * amult;
                b += (colortable[j].color>> 0 & 0xFF) * amult;
                adiv += count;
                rgbdiv += amult;
            }
            if (adiv == 0) {
                adiv = 1;
            }
            if (rgbdiv == 0) {
                rgbdiv = 1;
            }
            a = (a + adiv/2) / adiv;
            r = (r + rgbdiv/2) / rgbdiv;
            g = (g + rgbdiv/2) / rgbdiv;
            b = (b + rgbdiv/2) / rgbdiv;
            color = a<<24 | r<<16 | g<<8 | b;
        }  // if (index < fixed_colors)
        const uint32_t diff = colordiff_sq(color, colortable[i].color);
        if (diff < bestdiff) {
            best = index;
            bestdiff = diff;
            bestcolor = color;
        }
    }

    POSTCOND(best >= 0);
    *color_ret = bestcolor;
    return best;
}

/*************************************************************************/

/**
 * select_color_2:  第2方法で色を選択する。パレットから、統合した時に出る
 * 誤差が一番小さいインデックスのペアを選んで、インデックスと加重平均した
 * 色、合計誤差を返す。
 *
 * generate_palette()の補助関数。
 *
 * 【引　数】           i: colortable[]内、現在処理している色
 * 　　　　　     palette: 256色パレットへのポインタ
 * 　　　　　fixed_colors: パレットのうち、固定されている色
 * 　　　　　  second_ret: 第2インデックス（統合先）を格納する変数へのポインタ
 * 　　　　　   color_ret: 統合した色を格納する変数へのポインタ
 * 　　　　　    diff_ret: 誤差を格納する変数へのポインタ
 * 【戻り値】統合すべきパレットインデックス（0以外）
 */
static int select_color_2(uint32_t i, uint32_t *palette, int fixed_colors,
                          int *second_ret, uint32_t *color_ret,
                          uint64_t *diff_ret)
{
    PRECOND(colortable != NULL);
    PRECOND(palette != NULL);
    PRECOND(second_ret != NULL);
    PRECOND(color_ret != NULL);
    PRECOND(diff_ret != NULL);

    int best = -1;              // 最善インデックス
    int best_second = 0;        // bestを使った場合、統合先のインデックス
    uint32_t bestcolor = 0;     // bestを使った場合の平均色
    uint64_t bestdiff = ~0ULL;  // bestを使った場合の誤差
    uint32_t maxpairdiff = 0x40000;  // 処理高速化用（以下参照）

  retry:;
    int index;
    for (index = fixed_colors; index < 256; index++) {
        int second;
        for (second = 0; second < index; second++) {
            /* 処理高速化のため、差の大きい色ペアは即諦める。余程のことが
             * ない限り、結果には影響しない */
            if (colordiff_sq(palette[index], palette[second]) > maxpairdiff) {
                continue;
            }
            /* 固定色は変更できない */
            uint32_t color;
            if (second < fixed_colors) {
                color = palette[second];
            } else {
                uint32_t a = 0, r = 0, g = 0, b = 0;
                uint32_t rgbdiv = 0, adiv = 0;
                int32_t j;
                for (j = firstuser[index]; j >= 0; j = colortable[j].nextuser){
                    uint32_t count = colortable[j].count;
                    uint32_t amult =
                        (count * (colortable[j].color>>24 & 0xFF)) / 255;
                    a += (colortable[j].color>>24 & 0xFF) * count;
                    r += (colortable[j].color>>16 & 0xFF) * amult;
                    g += (colortable[j].color>> 8 & 0xFF) * amult;
                    b += (colortable[j].color>> 0 & 0xFF) * amult;
                    adiv += count;
                    rgbdiv += amult;
                }
                for (j = firstuser[second]; j >= 0; j = colortable[j].nextuser){
                    uint32_t count = colortable[j].count;
                    uint32_t amult =
                        (count * (colortable[j].color>>24 & 0xFF)) / 255;
                    a += (colortable[j].color>>24 & 0xFF) * count;
                    r += (colortable[j].color>>16 & 0xFF) * amult;
                    g += (colortable[j].color>> 8 & 0xFF) * amult;
                    b += (colortable[j].color>> 0 & 0xFF) * amult;
                    adiv += count;
                    rgbdiv += amult;
                }
                if (adiv == 0) {
                    adiv = 1;
                }
                if (rgbdiv == 0) {
                    rgbdiv = 1;
                }
                a = (a + adiv/2) / adiv;
                r = (r + rgbdiv/2) / rgbdiv;
                g = (g + rgbdiv/2) / rgbdiv;
                b = (b + rgbdiv/2) / rgbdiv;
                color = a<<24 | r<<16 | g<<8 | b;
            }  // if (second < fixed_colors)
            uint64_t diff = 0;
            int32_t j;
            for (j = firstuser[index]; j >= 0; j = colortable[j].nextuser) {
                const uint64_t thisdiff =
                    colordiff_sq(color, colortable[j].color);
                diff += thisdiff * colortable[j].count;
            }
            for (j = firstuser[second]; j >= 0; j = colortable[j].nextuser) {
                const uint64_t thisdiff =
                    colordiff_sq(color, colortable[j].color);
                diff += thisdiff * colortable[j].count;
            }
            if (diff < bestdiff) {
                best = index;
                best_second = second;
                bestdiff = diff;
                bestcolor = color;
            }
        }
    }

    if (best < 0) {
        /* maxpairdiff以内の色ペアが無かった。候補を返さなければならないこと
         * になっているので、限界値を引き上げて再試行 */
        if (maxpairdiff >= 0x40000000) {
            maxpairdiff = ~0;  // 全て通過
        } else {
            maxpairdiff *= 4;
        }
        goto retry;
    }

    POSTCOND(best >= 0);
    *second_ret = best_second;
    *color_ret = bestcolor;
    *diff_ret = bestdiff;
    return best;
}

/*************************************************************************/
/*************************************************************************/

/**
 * generate_colortable:  画像で使われている色を調べ、色の値と使用頻度を
 * colortable[]に格納する。colortable配列のメモリは予め確保されていなければ
 * ならない（少なくともwidth*height要素分）。
 *
 * 【引　数】    imageptr: 画像データポインタ（0xAARRGGBBまたは0xAABBGGRR形式）
 * 　　　　　       width: 画像の幅（ピクセル）
 * 　　　　　      height: 画像の高さ（ピクセル）
 * 　　　　　      stride: 画像のライン長（ピクセル）
 * 　　　　　     palette: 固定色の配列
 * 　　　　　fixed_colors: 固定色の数
 * 【引　数】色の数
 */
static uint32_t generate_colortable(const uint32_t *imageptr, uint32_t width,
                                    uint32_t height, uint32_t stride,
                                    const uint32_t *palette, int fixed_colors,
                                    void (*callback)(void))
{
    PRECOND(imageptr != NULL);
    PRECOND(colortable != NULL);
    PRECOND(palette != NULL);

    time_t last_callback = time(NULL);
    uint32_t total_pixels = 0;

    /* テーブルを生成する */
    int y;
    for (y = 0; y < height; y++) {
        const uint32_t *ptr = &imageptr[y * stride];
        int x;
        for (x = 0; x < width; x++, total_pixels++) {
            uint32_t i;

            if (callback != NULL && total_pixels % 256 == 0
             && time(NULL) != last_callback
            ) {
                (*callback)();
                last_callback = time(NULL);
            }

            /* 固定色の場合は除く */
            for (i = 0; i < fixed_colors; i++) {
                if (ptr[x] == palette[i]) {
                    break;
                }
            }
            if (i < fixed_colors) {
                continue;
            }

            /* 色をカウントする。ついでに、検索を速くするために1つ前へ
             * 移動する */
            if (ptr[x] == colortable[0].color) {
                /* すでに配列の先頭にある */
                colortable[0].count++;
                continue;
            }
            for (i = 1; colortable[i].count != 0; i++) {
                if (ptr[x] == colortable[i].color) {
                    break;
                }
            }
            uint32_t save_color = colortable[i-1].color;
            uint32_t save_count = colortable[i-1].count;
            colortable[i-1].color = ptr[x];
            colortable[i-1].count = colortable[i].count + 1;
            colortable[i].color = save_color;
            colortable[i].count = save_count;
        }
    }

    /* 色を数えて返す */
    int ncolors = 0;
    while (ncolors < width * height && colortable[ncolors].count != 0) {
        ncolors++;
    }
    return ncolors;
}

/*************************************************************************/

/**
 * colordiff_sq:  2つの色の差の自乗を返す。アルファ値も考慮する。
 *
 * 【引　数】color1, color2: 差を計算する色（0xAARRGGBB）
 * 【戻り値】色の差の自乗（0〜約0xFC000000）
 */
static inline uint32_t colordiff_sq(uint32_t color1, uint32_t color2)
{
#ifdef USE_SSE2
    uint32_t result;
    asm("movd %[color1], %%xmm0\n"
        "movd %[color2], %%xmm1\n"
        "pxor %%xmm7, %%xmm7\n"
        "pcmpeqw %%xmm6, %%xmm6\n"
        "pxor %%xmm5, %%xmm5\n"
        "psubb %%xmm6, %%xmm5\n"
        "psrlw $8, %%xmm6\n"
        "psrlw $8, %%xmm5\n"              // XMM5 = {1,1,1,1,1,1,1,1}
        "pslldq $6, %%xmm6\n"             // XMM6 = {A=255, R=0, G=0, B=0}
        "punpcklbw %%xmm7, %%xmm0\n"      // XMM0 = {0,0,0,0,a1,r1,g1,b1}
        "punpcklbw %%xmm7, %%xmm1\n"      // XMM1 = {0,0,0,0,a2,r2,g2,b2}
        "pshuflw $0xFF, %%xmm0, %%xmm2\n" // XMM2 = {0,0,0,0,a1,a1,a1,a1}
        "pshuflw $0xFF, %%xmm1, %%xmm3\n" // XMM3 = {0,0,0,0,a2,a2,a2,a2}
        "por %%xmm6, %%xmm2\n"            // XMM2 = {0,0,0,0,255,a1,a1,a1}
        "por %%xmm6, %%xmm3\n"            // XMM3 = {0,0,0,0,255,a2,a2,a2}
        "psubw %%xmm1, %%xmm0\n"          // XMM0 = {0,0,0,0,Da,Dr,Dg,Db}
        "pmullw %%xmm3, %%xmm2\n"         // XMM2 = {0,0,0,0,255*255,a1*a2,...}
        "pmullw %%xmm0, %%xmm0\n"         // XMM0 = {0,0,0,0,Da*Da,Dr*Dr,...}
        "paddw %%xmm5, %%xmm2\n"          // XMM2 = {...,255*255+1,a1*a2+1,...}
        /* 16ビット符号無し乗算命令や32ビット並列乗算命令がないので、
         * ちょい面倒 */
        "punpcklwd %%xmm7, %%xmm2\n"      // XMM2 = {255*255+1,a1*a2+1,...}
        "punpcklwd %%xmm7, %%xmm0\n"      // XMM0 = {Da*Da,Dr*Dr,Dg*Dg,Db*Db}
        "movdqa %%xmm2, %%xmm3\n"         // XMM3 = {255*255,a1*a2,a1*a2,a1*a2}
        "movdqa %%xmm0, %%xmm1\n"         // XMM1 = {Da*Da,Dr*Dr,Dg*Dg,Db*Db}
        "psrldq $4, %%xmm3\n"             // XMM3 = {0,255*255,a1*a2,a1*a2}
        "psrldq $4, %%xmm1\n"             // XMM1 = {0,Da*Da,Dr*Dr,Dg*Dg}
        "pmuludq %%xmm2, %%xmm0\n"        // XMM0 = {0,Dr2,0,Db2}
        "pmuludq %%xmm3, %%xmm1\n"        // XMM1 = {0,Da2,0,Dg2}
        "psrld $2, %%xmm0\n"              // XMM0 = {0,Dr2/4,0,Db2/4}
        "psrld $2, %%xmm1\n"              // XMM1 = {0,Da2/4,0,Dg2/4}
        "paddd %%xmm1, %%xmm0\n"          // XMM0={0,Da2/4+Dr2/4,0,Db2/4+Dg2/4}
        "pshufd $0xAA, %%xmm0, %%xmm1\n"  // 最後の2つを合算して終了
        "paddd %%xmm1, %%xmm0\n"
        "movd %%xmm0, %[result]\n"
        : [result] "=r" (result)
        : [color1] "r" (color1), [color2] "r" (color2)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm5", "xmm6", "xmm7"
    );
    return result;
#else
    const uint32_t a1 = (color1>>24 & 0xFF);
    const uint32_t r1 = (color1>>16 & 0xFF);
    const uint32_t g1 = (color1>> 8 & 0xFF);
    const uint32_t b1 = (color1>> 0 & 0xFF);
    const uint32_t a2 = (color2>>24 & 0xFF);
    const uint32_t r2 = (color2>>16 & 0xFF);
    const uint32_t g2 = (color2>> 8 & 0xFF);
    const uint32_t b2 = (color2>> 0 & 0xFF);
    /* アルファ0でも色成分の差を検出できるように、アルファ値の積に1を足す。
     * 非透明ピクセルの隣にある透明ピクセルの色は補間処理で意味を持つので、
     * 同じ透明ピクセルでもピクセル値を区別する必要がある */
    return ((a2-a1)*(a2-a1) * (255*255+1)) / 4
         + ((r2-r1)*(r2-r1) * (a1*a2+1)) / 4
         + ((g2-g1)*(g2-g1) * (a1*a2+1)) / 4
         + ((b2-b1)*(b2-b1) * (a1*a2+1)) / 4;
#endif
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
