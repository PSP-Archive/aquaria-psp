/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/texture.c: Texture manipulation routines for the
 * GE utility library.
 */

#include "../../common.h"
#include "../ge-util.h"
#include "../psplocal.h"

#include "ge-const.h"
#include "ge-local.h"

/*************************************************************************/
/*************************************************************************/

/**
 * ge_set_colortable:  CLUT型テキスチャのための色テーブルを設定する。
 *
 * 【引　数】 table: 色テーブルへのポインタ（64バイトアライメント必須）
 * 　　　　　 count: 色の数
 * 　　　　　format: 色データ形式（GE_PIXFMT_*）
 * 　　　　　 shift: テキスチャデータの右シフト量（0〜31）
 * 　　　　　  mask: テキスチャデータのマスク値
 * 【戻り値】なし
 */
void ge_set_colortable(const void *table, int count, GEPixelFormat pixfmt,
                       int shift, uint8_t mask)
{
    CHECK_GELIST(4);
    internal_add_command(GECMD_CLUT_MODE, pixfmt | (shift & 31)<<2 | mask<<8);
    internal_add_command(GECMD_CLUT_ADDRESS_L,
                         ((uint32_t)table & 0x00FFFFFF));
    internal_add_command(GECMD_CLUT_ADDRESS_H,
                         ((uint32_t)table & 0xFF000000) >> 8);
    internal_add_command(GECMD_CLUT_LOAD,
                         (pixfmt==GE_PIXFMT_8888) ? count/8 : count/16);
}

/*************************************************************************/

/**
 * ge_flush_texture_cache:  テキスチャキャッシュをクリアする。データポインタ
 * を変更後、ge_texture_set_format()を呼び出さずに描画する場合、キャッシュを
 * クリアしなければならない。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void ge_flush_texture_cache(void)
{
    CHECK_GELIST(1);
    internal_add_command(GECMD_TEXTURE_FLUSH, 0);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_texture_data:  テキスチャのデータポインタとサイズを設定する。
 *
 * 【引　数】 index: テキスチャインデックス（0〜7）
 * 　　　　　  data: テキスチャデータ
 * 　　　　　 width: テキスチャの幅（ピクセル）
 * 　　　　　height: テキスチャの高さ（ピクセル）
 * 　　　　　stride: テキスチャデータのライン長（ピクセル）
 * 【戻り値】なし
 */
void ge_set_texture_data(int index, const void *data,
                         int width, int height, int stride)
{
    CHECK_GELIST(3);

    const int log2_width  =
        width ==1 ? 0 : ubound(32 - __builtin_clz(width -1), 9);
    const int log2_height =
        height==1 ? 0 : ubound(32 - __builtin_clz(height-1), 9);

    internal_add_command(GECMD_TEX0_ADDRESS + index,
                         ((uint32_t)data & 0x00FFFFFF));
    internal_add_command(GECMD_TEX0_STRIDE + index,
                         ((uint32_t)data & 0xFF000000)>>8 | stride);
    internal_add_command(GECMD_TEX0_SIZE + index, log2_height<<8 | log2_width);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_texture_draw_mode:  テキスチャ描画方法を設定する。
 *
 * 【引　数】 mode: 描画方法（GE_TEXDRAWMODE_*）
 * 　　　　　alpha: 0以外＝アルファも考慮する、0＝アルファを考慮しない
 * 【戻り値】なし
 */
void ge_set_texture_draw_mode(GETextureDrawMode mode, int alpha)
{
    CHECK_GELIST(1);
    internal_add_command(GECMD_TEXTURE_FUNC, mode | (alpha ? 1<<8 : 0));
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_texture_color:  GE_TEXDRAWMODE_BLENDで使われるテキスチャ色を設定
 * する。
 *
 * 【引　数】color: テキスチャ色（0xBBGGRR）
 * 【戻り値】なし
 */
void ge_set_texture_color(uint32_t color)
{
    CHECK_GELIST(1);
    internal_add_command(GECMD_TEXTURE_COLOR, color & 0xFFFFFF);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_texture_filter:  テキスチャの拡大・縮小フィルタを設定する。
 *
 * 【引　数】mag_filter: 拡大時のフィルタ（GE_TEXFILTER_*）
 * 　　　　　min_filter: 縮小時のフィルタ（GE_TEXFILTER_*）
 * 　　　　　mip_filter: 縮小時のmipmap選択フィルタ（GE_TEXMIPFILTER_*）
 * 【戻り値】なし
 */
void ge_set_texture_filter(GETextureFilter mag_filter,
                           GETextureFilter min_filter,
                           GETextureMipFilter mip_filter)
{
    CHECK_GELIST(1);
    internal_add_command(GECMD_TEXTURE_FILTER, (mag_filter | mip_filter) << 8
                                             | (min_filter | mip_filter));
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_texture_mipmap_mode:  テキスチャのmipmap選択モードとバイアスを設定
 * する。
 *
 * バイアスは、指定されたモードにより選択されたmipmapレベルに加算される値で、
 * [-8.0,8.0)の範囲内で設定できる（実際には1/16単位で設定される）。レベルが
 * 1.0上がるごとにテキスチャの解像度が半分になる。
 *
 * 【引　数】mode: mipmap選択モード（GE_MIPMAPMODE_*）
 * 　　　　　bias: mipmap選択バイアス
 * 【戻り値】なし
 */
void ge_set_texture_mipmap_mode(GEMipmapMode mode, float bias)
{
    CHECK_GELIST(1);
    const int bias_int = iroundf(bound(bias*16, -128, +127)) & 0xFF;
    internal_add_command(GECMD_TEXTURE_BIAS, bias_int<<16 | mode);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_texture_mipmap_slope:  テキスチャの傾斜型mipmap選択モード
 * （GE_MIPMAPMODE_SLOPE）において、傾斜を設定する。
 *
 * カメラ位置からテキスチャへの距離をdとした場合、mipmapレベルLは以下のよう
 * に選択される。
 * 　　L = 1 + log2(d / slope)
 * つまり、slopeを1.0に設定した場合、カメラからの距離が0.5以下でmipmapレベル
 * 0、1.0でmipmapレベル1、2.0でレベル2、4.0でレベル3などとなり、128.0以上で
 * 最高（解像度最低）のレベル7となる。
 *
 * 【引　数】slope: 傾斜
 * 【戻り値】なし
 */
void ge_set_texture_mipmap_slope(float slope)
{
    CHECK_GELIST(1);
    internal_add_commandf(GECMD_TEXTURE_SLOPE, slope);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_texture_format:  テキスチャ形式を設定する。
 *
 * 【引　数】  levels: テキスチャのレベル数（mipmap数）
 * 　　　　　swizzled: 0以外＝データがswizzleされている、0＝swizzleされていない
 * 　　　　　  format: テキスチャのデータ形式（GE_TEXFMT_*）
 * 【戻り値】なし
 */
void ge_set_texture_format(int levels, int swizzled, GETexelFormat format)
{
    CHECK_GELIST(3);
    internal_add_command(GECMD_TEXTURE_MODE, (bound(levels,1,8) - 1) << 16
                                           | (swizzled ? 1 : 0));
    internal_add_command(GECMD_TEXTURE_PIXFMT, format);
    internal_add_command(GECMD_TEXTURE_FLUSH, 0);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_texture_wrap_mode:  テキスチャ描画方法を設定する。
 *
 * 【引　数】u_mode: 描画方法（GE_TEXWRAPMODE_*）
 * 　　　　　u_mode: 描画方法（GE_TEXWRAPMODE_*）
 * 【戻り値】なし
 */
void ge_set_texture_wrap_mode(GETextureWrapMode u_mode,
                              GETextureWrapMode v_mode)
{
    CHECK_GELIST(1);
    internal_add_command(GECMD_TEXTURE_WRAP, u_mode | v_mode<<8);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_texture_scale:  テキスチャ座標倍率係数を設定する。
 *
 * 【引　数】u_scale: U座標倍率係数
 * 　　　　　v_scale: V座標倍率係数
 * 【戻り値】なし
 */
void ge_set_texture_scale(float u_scale, float v_scale)
{
    CHECK_GELIST(2);
    internal_add_commandf(GECMD_USCALE, u_scale);
    internal_add_commandf(GECMD_VSCALE, v_scale);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_texture_offset:  テキスチャ座標オフセットを設定する。
 *
 * 【引　数】u_offset: U座標オフセット
 * 　　　　　v_offset: V座標オフセット
 * 【戻り値】なし
 */
void ge_set_texture_offset(float u_offset, float v_offset)
{
    CHECK_GELIST(2);
    internal_add_commandf(GECMD_UOFFSET, u_offset);
    internal_add_commandf(GECMD_VOFFSET, v_offset);
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
