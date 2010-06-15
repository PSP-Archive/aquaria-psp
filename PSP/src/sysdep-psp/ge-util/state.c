/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/state.c: Render state manipulation routines for
 * the GE utility library.
 */

#include "../../common.h"
#include "../ge-util.h"
#include "../psplocal.h"

#include "ge-const.h"
#include "ge-local.h"

/*************************************************************************/
/*************************************************************************/

/**
 * ge_enable:  指定された描画機能を有効にする。
 *
 * 【引　数】state: 有効にする機能（GE_STATE_*）
 * 【戻り値】なし
 */
void ge_enable(GEState state)
{
    CHECK_GELIST(1);
    switch (state) {
      case GE_STATE_LIGHTING:
        internal_add_command(GECMD_ENA_LIGHTING,   1);  return;
      case GE_STATE_CLIP_PLANES:
        internal_add_command(GECMD_ENA_ZCLIP,      1);  return;
      case GE_STATE_TEXTURE:
        internal_add_command(GECMD_ENA_TEXTURE,    1);  return;
      case GE_STATE_FOG:
        internal_add_command(GECMD_ENA_FOG,        1);  return;
      case GE_STATE_DITHER:
        internal_add_command(GECMD_ENA_DITHER,     1);  return;
      case GE_STATE_BLEND:
        internal_add_command(GECMD_ENA_BLEND,      1);  return;
      case GE_STATE_ALPHA_TEST:
        internal_add_command(GECMD_ENA_ALPHA_TEST, 1);  return;
      case GE_STATE_DEPTH_TEST:
        internal_add_command(GECMD_ENA_DEPTH_TEST, 1);  return;
      case GE_STATE_DEPTH_WRITE:
        internal_add_command(GECMD_DEPTH_MASK,     0);  return;
      case GE_STATE_STENCIL_TEST:
        internal_add_command(GECMD_ENA_STENCIL,    1);  return;
      case GE_STATE_ANTIALIAS:
        internal_add_command(GECMD_ENA_ANTIALIAS,  1);  return;
      case GE_STATE_PATCH_CULL_FACE:
        internal_add_command(GECMD_ENA_PATCH_CULL, 1);  return;
      case GE_STATE_COLOR_TEST:
        internal_add_command(GECMD_ENA_COLOR_TEST, 1);  return;
      case GE_STATE_COLOR_LOGIC_OP:
        internal_add_command(GECMD_ENA_LOGIC_OP,   1);  return;
      case GE_STATE_REVERSE_NORMALS:
        internal_add_command(GECMD_REV_NORMALS,    1);  return;
    }
    DMSG("Unknown state %d", state);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_disable:  指定された描画機能を無効にする。
 *
 * 【引　数】state: 無効にする機能（GE_STATE_*）
 * 【戻り値】なし
 */
void ge_disable(GEState state)
{
    CHECK_GELIST(1);
    switch (state) {
      case GE_STATE_LIGHTING:
        internal_add_command(GECMD_ENA_LIGHTING,   0);  return;
      case GE_STATE_CLIP_PLANES:
        internal_add_command(GECMD_ENA_ZCLIP,      0);  return;
      case GE_STATE_TEXTURE:
        internal_add_command(GECMD_ENA_TEXTURE,    0);  return;
      case GE_STATE_FOG:
        internal_add_command(GECMD_ENA_FOG,        0);  return;
      case GE_STATE_DITHER:
        internal_add_command(GECMD_ENA_DITHER,     0);  return;
      case GE_STATE_BLEND:
        internal_add_command(GECMD_ENA_BLEND,      0);  return;
      case GE_STATE_ALPHA_TEST:
        internal_add_command(GECMD_ENA_ALPHA_TEST, 0);  return;
      case GE_STATE_DEPTH_TEST:
        internal_add_command(GECMD_ENA_DEPTH_TEST, 0);  return;
      case GE_STATE_DEPTH_WRITE:
        internal_add_command(GECMD_DEPTH_MASK,     1);  return;
      case GE_STATE_STENCIL_TEST:
        internal_add_command(GECMD_ENA_STENCIL,    0);  return;
      case GE_STATE_ANTIALIAS:
        internal_add_command(GECMD_ENA_ANTIALIAS,  0);  return;
      case GE_STATE_PATCH_CULL_FACE:
        internal_add_command(GECMD_ENA_PATCH_CULL, 0);  return;
      case GE_STATE_COLOR_TEST:
        internal_add_command(GECMD_ENA_COLOR_TEST, 0);  return;
      case GE_STATE_COLOR_LOGIC_OP:
        internal_add_command(GECMD_ENA_LOGIC_OP,   0);  return;
      case GE_STATE_REVERSE_NORMALS:
        internal_add_command(GECMD_REV_NORMALS,    0);  return;
    }
    DMSG("Unknown state %d", state);
}

/*************************************************************************/

/**
 * ge_set_alpha_mask:  アルファデータの書き込みマスク値を設定する。
 *
 * 【引　数】mask: 書き込みマスク（0xFF＝全ビット書き込み不可）
 * 【戻り値】なし
 */
void ge_set_alpha_mask(uint8_t mask)
{
    CHECK_GELIST(1);
    ge_add_command(GECMD_ALPHA_MASK, mask);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_alpha_test:  アルファテストの比較関数と基準値を設定する。
 *
 * 【引　数】test: 比較関数（GE_TEST_*）
 * 　　　　　 ref: 基準値（0〜255）
 * 【戻り値】なし
 */
void ge_set_alpha_test(GETestFunc test, uint8_t ref)
{
    CHECK_GELIST(1);
    ge_add_command(GECMD_ALPHATEST, test | ref<<8 | 0xFF<<16);
}

/*************************************************************************/

/**
 * ge_set_ambient_color:  描画時の環境色を設定する。
 *
 * 【引　数】color: 環境色（0xAABBGGRR）
 * 【戻り値】なし
 */
void ge_set_ambient_color(uint32_t color)
{
    CHECK_GELIST(2);
    internal_add_command(GECMD_AMBIENT_COLOR, color & 0xFFFFFF);
    internal_add_command(GECMD_AMBIENT_ALPHA, color >> 24);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_ambient_light:  光源モデルの環境色を設定する。
 *
 * 【引　数】color: 環境色（0xAABBGGRR）
 * 【戻り値】なし
 */
void ge_set_ambient_light(uint32_t color)
{
    CHECK_GELIST(2);
    internal_add_command(GECMD_LIGHT_AMBCOLOR, color & 0xFFFFFF);
    internal_add_command(GECMD_LIGHT_AMBALPHA, color >> 24);
}

/*************************************************************************/

/**
 * ge_set_blend_mode:  ブレンド関数とパラメータを設定する。
 *
 * 【引　数】     func: ブレンド関数
 * 　　　　　src_param: ブレンド元（src）のブレンドパラメータ
 * 　　　　　dst_param: ブレンド先（dst）のブレンドパラメータ
 * 　　　　　  src_fix: src_param==GE_BLEND_FIXの場合、srcに適用する定数
 * 　　　　　  dst_fix: dst_param==GE_BLEND_FIXの場合、dstに適用する定数
 * 【戻り値】なし
 * 【注　意】src_param!=GE_BLEND_FIXの場合、src用定数レジスタを更新しない。
 * 　　　　　dst_param!=GE_BLEND_FIXの場合も同様。
 */
void ge_set_blend_mode(GEBlendFunc func,
                       GEBlendParam src_param, GEBlendParam dst_param,
                       uint32_t src_fix, uint32_t dst_fix)
{
    CHECK_GELIST(3);
    internal_add_command(GECMD_BLEND_FUNC, func<<8 | dst_param<<4 | src_param);
    if (src_param == GE_BLEND_FIX) {
        internal_add_command(GECMD_BLEND_SRCFIX, src_fix);
    }
    if (dst_param == GE_BLEND_FIX) {
        internal_add_command(GECMD_BLEND_DSTFIX, dst_fix);
    }
}

/*************************************************************************/

/**
 * ge_set_clip_area, ge_unset_clip_area:  クリップ範囲を設定または解除する。
 *
 * 【引　数】x0, y0: クリップ範囲の左上隅の座標（ピクセル）
 * 　　　　　x1, y1: クリップ範囲の右下隅の座標（ピクセル）
 * 【戻り値】なし
 */
inline void ge_set_clip_area(int x0, int y0, int x1, int y1)
{
    x0 = bound(x0, 0, 1023);
    y0 = bound(y0, 0, 1023);
    x1 = bound(x1, 0, 1023);
    y1 = bound(y1, 0, 1023);
    if (x1 < x0) {
        int tmp = x0; x0 = x1; x1 = tmp;
    }
    if (y1 < x0) {
        int tmp = y0; y0 = y1; y1 = tmp;
    }
    CHECK_GELIST(2);
    internal_add_command(GECMD_CLIP_MIN, x0 | y0<<10);
    internal_add_command(GECMD_CLIP_MAX, x1 | y1<<10);
}

void ge_unset_clip_area(void)
{
    ge_set_clip_area(0, 0, DISPLAY_WIDTH-1, DISPLAY_HEIGHT-1);
}

/*************************************************************************/

/**
 * ge_set_color_mask:  色データの書き込みマスク値を設定する。
 *
 * 【引　数】mask: 書き込みマスク（0xFFFFFF＝全ビット書き込み不可）
 * 【戻り値】なし
 */
void ge_set_color_mask(uint32_t mask)
{
    CHECK_GELIST(1);
    ge_add_command(GECMD_COLOR_MASK, mask);
}

/*************************************************************************/

/**
 * ge_set_cull_mode:  フェイス排除モードを設定する。
 *
 * 【引　数】mode: フェイス排除モード（GE_CULL_*）
 * 【戻り値】なし
 */
void ge_set_cull_mode(GECullMode mode)
{
    CHECK_GELIST(2);
    switch (mode) {
      case GE_CULL_NONE:
        internal_add_command(GECMD_ENA_FACE_CULL, 0);
        return;
      case GE_CULL_CW:
        internal_add_command(GECMD_ENA_FACE_CULL, 1);
        internal_add_command(GECMD_FACE_ORDER, GEVERT_CCW);
        return;
      case GE_CULL_CCW:
        internal_add_command(GECMD_ENA_FACE_CULL, 1);
        internal_add_command(GECMD_FACE_ORDER, GEVERT_CW);
        return;
    }
    DMSG("Unknown mode %d", mode);
}

/*************************************************************************/

/**
 * ge_set_depth_test:  奥行きテストの比較関数を設定する。
 *
 * 【引　数】test: 比較関数（GE_TEST_*）
 * 【戻り値】なし
 */
void ge_set_depth_test(GETestFunc test)
{
    CHECK_GELIST(1);
    ge_add_command(GECMD_DEPTHTEST, test);
}

/*************************************************************************/

/**
 * ge_set_depth_range:  奥行きバッファに格納する奥行き値の範囲を設定する。
 * デフォルトは65535〜0（GEは大きい値ほど近いと判定する）。
 *
 * 【引　数】near: 最も近い奥行きに与える値（0〜65535）
 * 　　　　　 far: 最も遠い奥行きに与える値（0〜65535）
 * 【戻り値】なし
 */
void ge_set_depth_range(uint16_t near, uint16_t far)
{
    CHECK_GELIST(2);
    internal_add_commandf(GECMD_ZSCALE, ((int32_t)far - (int32_t)near) / 2.0f);
    internal_add_commandf(GECMD_ZPOS,   ((int32_t)far + (int32_t)near) / 2.0f);
}

/*************************************************************************/

/**
 * ge_set_fog:  フォグの設定を行う。
 *
 * 【引　数】 near: フォグが発生し始める奥行き
 * 　　　　　  far: フォグ効果が最大に達する奥行き
 * 　　　　　color: フォグの色（0xBBGGRR）
 * 【戻り値】なし
 */
void ge_set_fog(float near, float far, uint32_t color)
{
    CHECK_GELIST(3);
    internal_add_commandf(GECMD_FOG_LIMIT, far);
    internal_add_commandf(GECMD_FOG_RANGE, 1 / (far - near));
    internal_add_command (GECMD_FOG_COLOR, color & 0xFFFFFF);
}

/*************************************************************************/

/**
 * ge_set_shade_mode:  シェードモードを設定する。
 *
 * 【引　数】mode: シェードモード（GE_SHADE_*）
 * 【戻り値】なし
 */
void ge_set_shade_mode(GEShadeMode mode)
{
    CHECK_GELIST(1);
    internal_add_command(GECMD_SHADE_MODE, mode);
}

/*************************************************************************/

/**
 * ge_set_viewport:  表示領域を設定する。
 *
 * 【引　数】         x, y: 左下隅の座標（ピクセル）
 * 　　　　　width, height: 領域のサイズ（ピクセル）
 * 【戻り値】なし
 */
void ge_set_viewport(int x, int y, int width, int height)
{
    const int x0 = bound(x, 0, 1023);
    const int y0 = bound(y, 0, 1023);
    const int x1 = bound(x+width-1, 0, 1023);
    const int y1 = bound(y+height-1, 0, 1023);

    CHECK_GELIST(2);
    internal_add_command(GECMD_DRAWAREA_LOW,  x0 | y0<<10);
    internal_add_command(GECMD_DRAWAREA_HIGH, x1 | y1<<10);
    internal_add_commandf(GECMD_XSCALE,  width /2);
    internal_add_commandf(GECMD_YSCALE, -height/2);
    internal_add_command(GECMD_XOFFSET, (2048 - width /2) << 4);
    internal_add_command(GECMD_YOFFSET, (2048 - height/2) << 4);
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
