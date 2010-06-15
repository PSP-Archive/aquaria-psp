/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/high-level.c: High-level graphics functions for
 * the GE utility library.
 */

#include "../../common.h"
#include "../ge-util.h"
#include "../psplocal.h"

#include "ge-const.h"
#include "ge-local.h"

/*************************************************************************/

/* ローカル関数宣言 */
static inline void internal_add_color_xyz_vertex(uint32_t color, int16_t x,
                                                 int16_t y, int16_t z);
static inline void internal_add_2_uv_xy_vertex(int16_t u1, int16_t v1,
                                               int16_t x1, int16_t y1,
                                               int16_t u2, int16_t v2,
                                               int16_t x2, int16_t y2);

/*************************************************************************/
/*************************************************************************/

/**
 * ge_clear:  引数に応じて、画面と奥行きバッファをクリアする。
 *
 * 【引　数】clear_screen: 0以外＝画面をクリアする、0＝クリアしない
 * 　　　　　 clear_depth: 0以外＝奥行きバッファをクリアする、0＝クリアしない
 * 　　　　　       color: 画面をクリアする際の色（0xAABBGGRR）
 * 【戻り値】なし
 */
void ge_clear(int clear_screen, int clear_depth, uint32_t color)
{
    CHECK_GELIST(7);
    CHECK_VERTLIST(6);

    /* 0x05xx = 画面バッファと奥行きバッファをクリアする */
    const int clear_flags = GECLEAR_ON
                          | (clear_screen ? GECLEAR_DRAW  : 0)
                          | (clear_depth  ? GECLEAR_DEPTH : 0);
    internal_add_command(GECMD_CLEAR_MODE, clear_flags);
    ge_disable(GE_STATE_BLEND);
    ge_set_vertex_format(GE_VERTEXFMT_TRANSFORM_2D
                       | GE_VERTEXFMT_COLOR_8888
                       | GE_VERTEXFMT_VERTEX_16BIT);
    ge_set_vertex_pointer(NULL);
    internal_add_color_xyz_vertex(color, 0, 0, 0);
    internal_add_color_xyz_vertex(color, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
    ge_draw_primitive(GE_PRIMITIVE_SPRITES, 2);
    internal_add_command(GECMD_CLEAR_MODE, GECLEAR_OFF);
    ge_commit();
}

/*************************************************************************/

/**
 * ge_copy:  srcからdestへ画像データをコピーする。各ポインタのアライメントは
 * 1ピクセルで良いが、ライン長は8ピクセルの倍数でなければならない。
 *
 * 【引　数】          src: コピー元ポインタ
 * 　　　　　   src_stride: コピー元のライン幅（ピクセル）、2048未満
 * 　　　　　         dest: コピー先ポインタ
 * 　　　　　  dest_stride: コピー先のライン幅（ピクセル）、2048未満
 * 　　　　　width, height: コピーする領域のサイズ（ピクセル）、512以下
 * 　　　　　         mode: コピーモード（ピクセルデータのサイズ、GE_COPY_*）
 * 【戻り値】なし
 */
void ge_copy(const uint32_t *src, uint32_t src_stride, uint32_t *dest,
             uint32_t dest_stride, int width, int height, GECopyMode mode)
{
    CHECK_GELIST(8);

    const int Bpp = (mode==GE_COPY_16BIT) ? 2 : 4;
    internal_add_command(GECMD_COPY_S_ADDRESS, ((uint32_t)src &0x00FFFFC0));
    internal_add_command(GECMD_COPY_S_STRIDE,  ((uint32_t)src &0xFF000000) >> 8
                                               | src_stride);
    internal_add_command(GECMD_COPY_S_POS, ((uint32_t)src &0x0000003F) / Bpp);
    internal_add_command(GECMD_COPY_D_ADDRESS, ((uint32_t)dest&0x00FFFFC0));
    internal_add_command(GECMD_COPY_D_STRIDE,  ((uint32_t)dest&0xFF000000) >> 8
                                               | dest_stride);
    internal_add_command(GECMD_COPY_D_POS, ((uint32_t)dest&0x0000003F) / Bpp);
    internal_add_command(GECMD_COPY_SIZE, (width-1) | (height-1)<<10);
    internal_add_command(GECMD_COPY, mode);
    ge_commit();
}

/*************************************************************************/

/**
 * ge_blend:  srcからdestへ画像データをブレンドしながらコピーする。各ポインタ
 * のアライメントは1ピクセルで良いが、コピー先はVRAM内でなければならない。
 * ブレンド方法は、ge_set_blend_mode()によって事前に設定しなければならない。
 *
 * 【引　数】          src: コピー元ポインタ
 * 　　　　　   src_stride: コピー元のライン幅（ピクセル）、2048未満
 * 　　　　　   srcx, srcy: コピーする部分の左上隅の座標（ピクセル）
 * 　　　　　         dest: コピー先ポインタ（VRAM内）
 * 　　　　　  dest_stride: コピー先のライン幅（ピクセル）、2048未満
 * 　　　　　width, height: コピーする領域のサイズ（ピクセル）、height<=512
 * 　　　　　      palette: コピー元が8ビットデータの場合、色パレット。
 * 　　　　　               　　コピー元が32ビットデータの場合はNULLを指定
 * 　　　　　     swizzled: 0以外の場合、コピー元のデータがswizzleしてある
 * 【戻り値】なし
 */
void ge_blend(const uint32_t *src, uint32_t src_stride, int srcx, int srcy,
              uint32_t *dest, uint32_t dest_stride, int width, int height,
              const uint32_t *palette, int swizzled)
{
    CHECK_GELIST(19);
    CHECK_VERTLIST(32*5/2);

    /* srcy >= 512の場合、そのままテキスチャ座標として使うと誤作動を起こして
     * しまうので、予めポインタを調整する。swizzleされたデータの場合、画像が
     * 8ライン単位で記録されているので注意が必要 */
    if (palette) {
        src = (uint32_t *)((uint8_t *)src + (srcy/8) * (src_stride*8));
    } else {
        src += (srcy/8) * (src_stride*8);
    }
    srcy %= 8;

    /* GEに渡すアドレスは64バイトの倍数でなければならないので、ずれている分
     * だけX座標を調整する */
    int xofs = (uint32_t)src & 0x3F;
    if (!palette) {
        xofs /= 4;
    }
    srcx += xofs;
    const uint32_t destx = ((uint32_t)dest & 0x3F) / (display_bpp / 8);

    ge_enable(GE_STATE_TEXTURE);
    ge_set_texture_filter(GE_TEXFILTER_NEAREST, GE_TEXFILTER_NEAREST,
                          GE_TEXMIPFILTER_NONE);
    ge_set_texture_wrap_mode(GE_TEXWRAPMODE_CLAMP, GE_TEXWRAPMODE_CLAMP);
    ge_set_vertex_format(GE_VERTEXFMT_TRANSFORM_2D
                       | GE_VERTEXFMT_TEXTURE_16BIT
                       | GE_VERTEXFMT_VERTEX_16BIT);

    /* 色パレットがあれば登録する */
    if (palette) {
        ge_set_colortable(palette, 256, GE_PIXFMT_8888, 0, 0xFF);
    }

    /* テキスチャを登録する。アライメントの関係で座標を調整したりするので、
     * テキスチャサイズを最大の512に設定しておく */
    ge_set_draw_buffer(dest, dest_stride);
    ge_set_texture_data(0, src, 512, 512, src_stride);
    ge_set_texture_format(1, swizzled,
                          palette ? GE_TEXFMT_T8 : GE_TEXFMT_8888);

    /* 64バイト単位で描画する（swizzledの場合でも、一括描画が遅い模様） */
    ge_set_vertex_pointer(NULL);
    int i, nverts;
    const int strip_width = palette ? 64 : 16;
    for (i = 0, nverts = 0; i < width; i += strip_width, nverts += 2) {
        const int thisw = ubound(width-i, strip_width);
        /* X座標が512を超えないよう、必要に応じてソースアドレスを修正 */
        if (srcx+i+thisw >= 512) {
            int srcx_sub;
            if (swizzled) {
                if (palette) {
                    srcx_sub = align_down(srcx+i, 16);
                    src += (srcx_sub * 8) / 4;
                } else {
                    srcx_sub = align_down(srcx+i, 4);
                    src += (srcx_sub * 32) / 4;
                }
            } else {
                srcx_sub = align_down(srcx+i, 64);
                src += srcx_sub / 4;
            }
            srcx -= srcx_sub;
            if (nverts > 0) {
                ge_draw_primitive(GE_PRIMITIVE_SPRITES, nverts);
                nverts = 0;
            }
            ge_set_texture_data(0, src, width, height, src_stride);
            ge_flush_texture_cache();
        }
        internal_add_2_uv_xy_vertex(srcx+i, srcy,
                                    destx+i, 0,
                                    srcx+i+thisw, srcy+height,
                                    destx+i+thisw, height);
    }
    ge_draw_primitive(GE_PRIMITIVE_SPRITES, nverts);
    ge_commit();

    ge_disable(GE_STATE_TEXTURE);
    ge_set_draw_buffer(NULL, 0);
}

/*************************************************************************/

/**
 * ge_fill:  VRAM領域をフィルする。
 *
 * 【引　数】x1, y1: 左上座標（ピクセル）
 * 　　　　　x2, y2: 右下座標+1（ピクセル）※x2,y2はフィル領域には入らない。
 * 　　　　　 color: フィル色（0xAABBGGRR）
 * 【戻り値】なし
 */
void ge_fill(int x1, int y1, int x2, int y2, uint32_t color)
{
    CHECK_GELIST(7);
    CHECK_VERTLIST(6);

    ge_disable(GE_STATE_TEXTURE);
    ge_enable(GE_STATE_BLEND);
    ge_set_blend_mode(GE_BLENDSET_SRC_ALPHA);
    ge_set_vertex_format(GE_VERTEXFMT_TRANSFORM_2D
                         | GE_VERTEXFMT_COLOR_8888
                         | GE_VERTEXFMT_VERTEX_16BIT);
    ge_set_vertex_pointer(NULL);
    internal_add_color_xyz_vertex(color, x1, y1, 0);
    internal_add_color_xyz_vertex(color, x2, y2, 0);
    ge_draw_primitive(GE_PRIMITIVE_SPRITES, 2);
    ge_commit();
    ge_disable(GE_STATE_BLEND);
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * internal_add_color_xyz_vertex:  色とXYZ座標を指定した頂点を登録する。
 * リスト満杯はチェックしない。
 *
 * 【引　数】  color: 色 (0xAABBGGRR)
 * 　　　　　x, y, z: 座標
 * 【戻り値】なし
 */
static inline void internal_add_color_xyz_vertex(uint32_t color, int16_t x,
                                                 int16_t y, int16_t z)
{
    *vertlist_ptr++ = color;
    *vertlist_ptr++ = INT16_PAIR(x, y);
    *vertlist_ptr++ = INT16_PAIR(z, 0);
}

/*-----------------------------------------------------------------------*/

/**
 * internal_add_2_uv_xy_vertex:  UV座標とXY座標を指定した頂点を2つ登録する。
 * リスト満杯はチェックしない。
 *
 * 【引　数】u, v: テキスチャ座標
 * 　　　　　x, y: 座標
 * 【戻り値】なし
 */
static inline void internal_add_2_uv_xy_vertex(int16_t u1, int16_t v1,
                                               int16_t x1, int16_t y1,
                                               int16_t u2, int16_t v2,
                                               int16_t x2, int16_t y2)
{
    *vertlist_ptr++ = INT16_PAIR(u1, v1);
    *vertlist_ptr++ = INT16_PAIR(x1, y1);
    *vertlist_ptr++ = INT16_PAIR(0 /*z1*/, u2);
    *vertlist_ptr++ = INT16_PAIR(v2, x2);
    *vertlist_ptr++ = INT16_PAIR(y2, 0 /*z2*/);
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
