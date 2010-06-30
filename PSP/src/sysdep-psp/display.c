/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/display.c: PSP display management and drawing routines.
 */

#include "../common.h"
#include "../memory.h"
#include "../sysdep.h"

#include "ge-util.h"
#include "psplocal.h"

#ifdef DEBUG
# include "../debugfont.h"  // デバッグ情報表示用
# include "../timer.h"      // 同上
#endif

/*************************************************************************/

/**
 * GE_SYNC_IN_THREAD:  定義すると、sys_display_finish()はハードウェアの描画
 * 処理完了を待たずに戻る。思い描画処理を行っている場合はフレームレートの
 * 向上を多少期待できる一方、ハードウェアとCPUのメモリバス競合でCPUの処理が
 * 遅くなる可能性もある。
 */
#define GE_SYNC_IN_THREAD

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* 表示モード（ピクセル形式）とビット／ピクセル */
static uint8_t display_mode;
static uint8_t display_bpp;

/* フレーム描画中フラグ（sys_display_start()とfinish()の間のみゼロ以外） */
static uint8_t in_frame;

/* ガンマ補正値。PSPでは色データに任意の指数を適用するのは無理な話なので、
 * out = in * (k*in + (1-k)*1) として計算する。gamma_level変数がこの数式の
 * 定数kとなる。gamma_level==0では効果無し（out = in）、gamma_level==1で
 * out = in^2 となる。 */
static float gamma_level;

/* 画面バッファ情報 */
static void *surfaces[2];

/* 現在表示されているサーフェイスインデックス */
static uint8_t displayed_surface;

/* 現在の描画用サーフェイスインデックスと、ピクセルデータへのポインタ */
static uint8_t work_surface;
static uint32_t *work_pixels;

/* 3次元描画用の奥行きバッファ */
static uint16_t *depth_buffer;

/* VRAMの空き容量へのポインタ */
static uint8_t *vram_spare_ptr;

/* VRAMの上限アドレス */
static uint8_t *vram_top;

/* 現在のクリップ設定 */
static int clip_left, clip_top, clip_right, clip_bottom;

/* 描画・表示バッファ入れ替えスレッド（0＝なし） */
static int buffer_flip_thread;

/*************************************************************************/

/* 描画用サーフェイスの特定ピクセルのアドレスを返すマクロ */
#define WORK_PIXEL_ADDRESS(x,y) \
    (display_bpp==16 \
     ? (void *)((uint16_t *)work_pixels + (y)*DISPLAY_STRIDE + (x)) \
     : (void *)((uint32_t *)work_pixels + (y)*DISPLAY_STRIDE + (x)))

static void do_buffer_flip(SceSize args, void *argp);

/* デバッグ情報表示関数 */
#ifdef DEBUG
static void display_ge_debug_info(void);
#endif

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * sys_display_start:  新しいフレームを描画する準備を行う。前のフレームで
 * 描画されたデータが画面に残っているかどうかは不定。
 *
 * 【引　数】width_ret, height_ret: 画面のサイズを格納する変数へのポインタ
 * 　　　　　                       　（ピクセル単位、NULL可）
 * 【戻り値】なし
 */
void sys_display_start(int *width_ret, int *height_ret)
{
    /* 前のフレームの描画が終了していることを確認する。こうしないと、
     * 新しいフレームの描画バッファが表示されたまま描画が開始されて
     * しまう可能性がある */
    sys_display_sync();

    gamma_level = 0;

    ge_start_frame(display_mode);
    clip_left = 0;
    clip_top = 0;
    clip_right = DISPLAY_WIDTH - 1;
    clip_bottom = DISPLAY_HEIGHT - 1;

    if (width_ret) {
        *width_ret = DISPLAY_WIDTH;
    }
    if (height_ret) {
        *height_ret = DISPLAY_HEIGHT;
    }

    in_frame = 1;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_display_finish:  描画されたデータを実際に画面に出力する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sys_display_finish(void)
{
    if (gamma_level > 0) {
        /* ガンマ補正を適用する */
        ge_disable(GE_STATE_DEPTH_TEST);
        ge_disable(GE_STATE_DEPTH_WRITE);
        ge_enable(GE_STATE_TEXTURE);
        ge_enable(GE_STATE_BLEND);
        ge_set_texture_data(0, work_pixels, 512, 512, DISPLAY_STRIDE);
        ge_set_texture_format(1, 0, GE_TEXFMT_8888);
        ge_set_texture_filter(GE_TEXFILTER_NEAREST, GE_TEXFILTER_NEAREST,
                              GE_TEXMIPFILTER_NONE);
        ge_set_texture_wrap_mode(GE_TEXWRAPMODE_CLAMP, GE_TEXWRAPMODE_CLAMP);
        /* BLENDモードによって、(1-gamma)*src + gamma*1.0 を計算し…… */
        ge_set_texture_draw_mode(GE_TEXDRAWMODE_BLEND, 0);
        const uint32_t gammaval = iroundf((1 - gamma_level) * 255);
        ge_set_ambient_color(0xFF000000 | (gammaval * 0x010101));
        ge_set_texture_color(0xFFFFFF);
        /* ……その値にsrcを乗じる */
        ge_set_blend_mode(GE_BLEND_ADD, GE_BLEND_COLOR, GE_BLEND_FIX, 0, 0);
        /* 全画面に適用する */
        ge_set_vertex_format(GE_VERTEXFMT_TRANSFORM_2D
                           | GE_VERTEXFMT_TEXTURE_16BIT
                           | GE_VERTEXFMT_VERTEX_16BIT);
        ge_set_vertex_pointer(NULL);
        int x, verts;
        for (x = 0, verts = 0; x < DISPLAY_WIDTH; x += 16, verts += 2) {
            ge_add_uv_xy_vertex(x, 0, x, 0);
            ge_add_uv_xy_vertex(x+16, DISPLAY_HEIGHT, x+16, DISPLAY_HEIGHT);
        }
        ge_draw_primitive(GE_PRIMITIVE_SPRITES, verts);
    }

#ifdef DEBUG
    /* L+R+Selectでデバッグメッセージを表示 */
    static int display_DMSG = 0;
    static int last_button_0 = 0;
    if (sys_input_buttonstate(8)
     && sys_input_buttonstate(9)
     && (sys_input_buttonstate(0) && !last_button_0)
    ) {
        display_DMSG = !display_DMSG;
    }
    last_button_0 = sys_input_buttonstate(0);
    if (display_DMSG) {
        psp_display_DMSG();
    }

    /* CPU情報と一緒にGE関連情報を表示する */
    if (debug_cpu_display_flag) {
        display_ge_debug_info();
    }
#endif

    in_frame = 0;

#ifndef GE_SYNC_IN_THREAD
    ge_end_frame();
    sceDisplaySetFrameBuf(work_pixels, DISPLAY_STRIDE, display_mode,
                          PSP_DISPLAY_SETBUF_NEXTFRAME);
#endif

    buffer_flip_thread = psp_start_thread("BufferFlipThread",
                                          (void *)do_buffer_flip,
                                          THREADPRI_MAIN, 1024,
                                          sizeof(work_pixels), &work_pixels);
    if (buffer_flip_thread < 0) {
        DMSG("Failed to start buffer flip thread: %s",
             psp_strerror(buffer_flip_thread));
        buffer_flip_thread = 0;
        do_buffer_flip(sizeof(work_pixels), &work_pixels);
    }
    displayed_surface = work_surface;
    work_surface = (work_surface + 1) % lenof(surfaces);
    work_pixels = (uint32_t *)surfaces[work_surface];
}

/*-----------------------------------------------------------------------*/

/**
 * sys_display_sync:  描画処理が全て完了するのを待つ。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sys_display_sync(void)
{
    if (buffer_flip_thread) {
        SceUInt timeout = 5*1001000/60;  // 5フレーム
        if (sceKernelWaitThreadEnd(buffer_flip_thread, &timeout) < 0) {
            sceKernelTerminateThread(buffer_flip_thread);
        }
        sceKernelDeleteThread(buffer_flip_thread);
        buffer_flip_thread = 0;
    } else if (in_frame) {
        ge_sync();
    }
}

/*************************************************************************/

/**
 * sys_display_set_fullscreen:  全画面表示の有無を設定する。ウィンドウ表示の
 * 概念がないシステムでは何もしない。
 *
 * 【引　数】on: 全画面表示フラグ（0以外＝全画面表示）
 * 【戻り値】0以外＝成功、0＝失敗
 */
int sys_display_set_fullscreen(int on)
{
    /* PSPでは常に全画面 */
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_display_clip:  描画のクリップ範囲を設定する。この範囲以外の描画は
 * 抑制される。フレーム開始時には設定されていない（全画面に描画できる）。
 *
 * 【引　数】    left, top: クリップ範囲の左上隅（ピクセル）
 * 　　　　　width, height: クリップ範囲のサイズ（ピクセル）
 * 【戻り値】なし
 */
void sys_display_clip(int left, int top, int width, int height)
{
    clip_left   = left;
    clip_top    = top;
    clip_right  = left + width - 1;
    clip_bottom = top + height - 1;
    ge_set_clip_area(clip_left, clip_top, clip_right, clip_bottom);
}

/*************************************************************************/

/**
 * sys_display_clear:  画面全体を黒にクリアする。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sys_display_clear(void)
{
    ge_clear(1, 1, 0x00000000);
}

/*-----------------------------------------------------------------------*/

/**
 * sys_display_blit:  画像データを描画する。色データはBGRA形式（PSP仕様では
 * RGBA形式）とする。自動的にクリップ処理を行う（座標やサイズが画面内に収まる
 * ように調整される）。
 *
 * 【引　数】          src: 描画する画像データ
 * 　　　　　   src_stride: srcのライン幅（ピクセル）
 * 　　　　　   srcx, srcy: 描画する部分の左上隅の座標（ピクセル）
 * 　　　　　 destx, desty: 画像の(srcx,srcy)を描画する画面座標（ピクセル）
 * 　　　　　width, height: 描画する領域の大きさ（ピクセル）
 * 　　　　　      palette: 色パレット（BLIT_8BITを指定する場合は必須）
 * 　　　　　        alpha: 濃度（0〜1、0＝透明）
 * 　　　　　        flags: 動作フラグ（BLIT_*）
 * 【戻り値】なし
 */
void sys_display_blit(const void *src, int src_stride, int srcx, int srcy,
                      int destx, int desty, int width, int height,
                      const uint32_t *palette, float alpha,
                      uint32_t flags)
{
    if (UNLIKELY(!src)) {
        DMSG("src == NULL");
        return;
    }
    if (UNLIKELY(src_stride <= 0)) {
        DMSG("src_stride <= 0");
        return;
    }
    if (UNLIKELY((flags & BLIT_8BIT) && !palette)) {
        DMSG("palette == NULL for 8bit");
        return;
    }
    if (UNLIKELY(src_stride >= 2048)
     || UNLIKELY((src_stride & (flags & BLIT_8BIT ? 15 : 3)) != 0)
    ) {
        DMSG("Invalid stride %d", src_stride);
        return;
    }

    if (UNLIKELY(destx < clip_left)) {
        srcx += (clip_left - destx);
        width += (destx - clip_left);
        destx = clip_left;
    }
    if (UNLIKELY(desty < clip_top)) {
        srcy += (clip_top - desty);
        height += (desty - clip_top);
        desty = clip_top;
    }
    if (UNLIKELY(destx + width > clip_right + 1)) {
        width = (clip_right + 1) - destx;
    }
    if (UNLIKELY(desty + height > clip_bottom + 1)) {
        height = (clip_bottom + 1) - desty;
    }
    if (UNLIKELY(width <= 0 || height <= 0)) {
        return;
    }
    if (UNLIKELY(alpha <= 0)) {
        return;
    }

    ge_unset_clip_area();
    ge_disable(GE_STATE_DEPTH_TEST);
    ge_disable(GE_STATE_DEPTH_WRITE);
    ge_enable(GE_STATE_BLEND);
    ge_set_texture_draw_mode(GE_TEXDRAWMODE_MODULATE, 1);
    if (flags & BLIT_BLEND_ADD) {
        ge_set_blend_mode(GE_BLEND_ADD, GE_BLEND_SRC_ALPHA,
                          GE_BLEND_FIX, 0, 0xFFFFFF);
    } else {
        ge_set_blend_mode(GE_BLENDSET_SRC_ALPHA);
    }
    const uint32_t color = bound(iroundf(alpha*255), 0, 255) << 24 | 0xFFFFFF;
    ge_set_ambient_color(color);
    ge_blend(src, src_stride, srcx, srcy,
             WORK_PIXEL_ADDRESS(destx,desty), DISPLAY_STRIDE, width, height,
             (flags & BLIT_8BIT) ? palette : NULL,
             flags & BLIT_SWIZZLED);
    ge_disable(GE_STATE_BLEND);
    ge_set_clip_area(clip_left, clip_top, clip_right, clip_bottom);
}

/*-----------------------------------------------------------------------*/

/**
 * sys_display_blit_list:  座標リストを基に画像データを描画する。
 *
 * 【引　数】       src: 描画する画像データ
 * 　　　　　   palette: 色パレット（BLIT_8BITを指定する場合は必須）
 * 　　　　　    stride: srcのライン長（ピクセル）
 * 　　　　　    height: srcの総ライン数
 * 　　　　　 blit_list: 座標リストへのポインタ
 * 　　　　　blit_count: 座標エントリーの数
 * 　　　　　     flags: 動作フラグ（BLIT_*）
 * 【戻り値】なし
 */
void sys_display_blit_list(const void *src, const uint32_t *palette,
                           int stride, int height,
                           const SysBlitList *blit_list, int blit_count,
                           uint32_t flags)
{
    if (UNLIKELY(!src) || UNLIKELY(stride <= 0) || UNLIKELY(height <= 0)) {
        DMSG("Invalid source parameters: %p %d %d", src, stride, height);
        return;
    }
    if (UNLIKELY((flags & BLIT_8BIT) && !palette)) {
        DMSG("palette == NULL for 8bit");
        return;
    }
    if (UNLIKELY(!blit_list)) {
        DMSG("blit_list == NULL");
        return;
    }
    if (UNLIKELY(blit_count <= 0)) {
        return;
    }
    if ((uint32_t)src & 63) {
        DMSG("src(%p) not 64-byte aligned, required for blit_list", src);
        return;
    }
    if (stride > 512) {
        DMSG("stride(%d) > 512 not supported for blit_list", stride);
        return;
    }

    ge_disable(GE_STATE_DEPTH_TEST);
    ge_disable(GE_STATE_DEPTH_WRITE);

    ge_enable(GE_STATE_BLEND);
    ge_set_texture_draw_mode(GE_TEXDRAWMODE_MODULATE, 1);
    if (flags & BLIT_BLEND_ADD) {
        ge_set_blend_mode(GE_BLEND_ADD, GE_BLEND_SRC_ALPHA, GE_BLEND_FIX,
                          0, 0xFFFFFF);
    } else {
        ge_set_blend_mode(GE_BLENDSET_SRC_ALPHA);
    }
    ge_set_ambient_color(0xFFFFFFFF);

    if (flags & BLIT_8BIT) {
        ge_set_colortable(palette, 256, GE_PIXFMT_8888, 0, 0xFF);
    }

    ge_enable(GE_STATE_TEXTURE);
    int texwidth = 1, texheight = 1;
    while (texwidth < stride) {
        texwidth <<= 1;
    }
    while (texheight < height) {
        texheight <<= 1;
    }
    ge_set_texture_data(0, src, texwidth, texheight, stride);
    ge_set_texture_format(1, flags & BLIT_SWIZZLED,
                          (flags & BLIT_8BIT) ? GE_TEXFMT_T8 : GE_TEXFMT_8888);
    ge_set_texture_filter(GE_TEXFILTER_LINEAR, GE_TEXFILTER_LINEAR,
                          GE_TEXMIPFILTER_NONE);
    ge_set_texture_wrap_mode(GE_TEXWRAPMODE_CLAMP, GE_TEXWRAPMODE_CLAMP);

    ge_set_vertex_format(GE_VERTEXFMT_TRANSFORM_2D
                       | GE_VERTEXFMT_TEXTURE_16BIT
                       | GE_VERTEXFMT_COLOR_8888
                       | GE_VERTEXFMT_VERTEX_32BITF);

    struct {int16_t u, v; uint32_t color; float x, y, z;} *vertices;
    for (; blit_count > 0; blit_count--, blit_list++) {
        /* 回転などの変形（拡大・縮小以外）がなく、全頂点の変色効果が同じ
         * 場合、GE_PRIMITIVE_SPRITESを使って描画できる。可否チェックが
         * 複雑だが、それでもチェックしない場合よりは遅くならない。
         * ちなみに、x1 < x0の場合は90度回転される。（GEの隠し機能？） */
        if (blit_list->dest[0].x == blit_list->dest[2].x
         && blit_list->dest[1].x == blit_list->dest[3].x
         && blit_list->dest[1].x > blit_list->dest[0].x
         && blit_list->dest[0].y == blit_list->dest[1].y
         && blit_list->dest[2].y == blit_list->dest[3].y
         && blit_list->dest[2].y > blit_list->dest[0].y
         && blit_list->dest[0].color == blit_list->dest[1].color
         && blit_list->dest[0].color == blit_list->dest[2].color
         && blit_list->dest[0].color == blit_list->dest[3].color
        ) {
            const int stripwidth = (flags & BLIT_8BIT) ? 64 : 16;
            const int nstrips = align_up(blit_list->srcw, stripwidth)
                                / stripwidth;
            vertices = ge_reserve_vertexbytes(sizeof(*vertices) * (nstrips*2));
            if (UNLIKELY(!vertices)) {
                DMSG("Failed to get %d*%d vertex bytes", sizeof(*vertices),
                     nstrips*2);
                break;
            }
            ge_set_vertex_pointer(vertices);
            const float strip_destw =
                (blit_list->dest[1].x - blit_list->dest[0].x) / blit_list->srcw
                * stripwidth;
            int strip;
            for (strip = 0; strip < nstrips; strip++, vertices += 2) {
                vertices[0].u = blit_list->srcx + strip*stripwidth;
                vertices[0].v = blit_list->srcy;
                vertices[0].color = convert_ARGB32(blit_list->dest[0].color);
                vertices[0].x = blit_list->dest[0].x + strip*strip_destw;
                vertices[0].y = blit_list->dest[0].y;
                vertices[0].z = 0;
                vertices[1].u = vertices[0].u + stripwidth;
                vertices[1].v = vertices[0].v + blit_list->srch;
                vertices[1].color = convert_ARGB32(blit_list->dest[3].color);
                vertices[1].x = vertices[0].x + strip_destw;
                vertices[1].y = blit_list->dest[3].y;
                vertices[1].z = 0;
            }
            /* 最後のストリップの幅が狭いかも知れないので頂点を修正しておく */
            vertices[-1].u = blit_list->srcx + blit_list->srcw;
            vertices[-1].x = blit_list->dest[3].x;
            ge_draw_primitive(GE_PRIMITIVE_SPRITES, nstrips*2);
        } else {  // SPRITES描画ができない
            vertices = ge_reserve_vertexbytes(sizeof(*vertices) * 4);
            if (UNLIKELY(!vertices)) {
                DMSG("Failed to get %d*%d vertex bytes", sizeof(*vertices), 4);
                break;
            }
            ge_set_vertex_pointer(vertices);
            vertices[0].u = blit_list->srcx;
            vertices[0].v = blit_list->srcy;
            vertices[0].color = convert_ARGB32(blit_list->dest[0].color);
            vertices[0].x = blit_list->dest[0].x;
            vertices[0].y = blit_list->dest[0].y;
            vertices[0].z = 0;
            vertices[1].u = blit_list->srcx + blit_list->srcw;
            vertices[1].v = blit_list->srcy;
            vertices[1].color = convert_ARGB32(blit_list->dest[1].color);
            vertices[1].x = blit_list->dest[1].x;
            vertices[1].y = blit_list->dest[1].y;
            vertices[1].z = 0;
            vertices[2].u = blit_list->srcx;
            vertices[2].v = blit_list->srcy + blit_list->srch;
            vertices[2].color = convert_ARGB32(blit_list->dest[2].color);
            vertices[2].x = blit_list->dest[2].x;
            vertices[2].y = blit_list->dest[2].y;
            vertices[2].z = 0;
            vertices[3].u = blit_list->srcx + blit_list->srcw;
            vertices[3].v = blit_list->srcy + blit_list->srch;
            vertices[3].color = convert_ARGB32(blit_list->dest[3].color);
            vertices[3].x = blit_list->dest[3].x;
            vertices[3].y = blit_list->dest[3].y;
            vertices[3].z = 0;
            ge_draw_primitive(GE_PRIMITIVE_TRIANGLE_STRIP, 4);
        }  // if (SPRITESで描画可能)
    }

    ge_disable(GE_STATE_TEXTURE);
    ge_commit();
}

/*-----------------------------------------------------------------------*/

/**
 * sys_display_capture:  画面の一部を画像バッファにコピーする。引数無効の
 * 場合、何もせずに失敗を返す。
 *
 * 【引　数】       x, y: キャプチャする領域の左上隅（ピクセル）
 * 　　　　　       w, h: キャプチャする領域のサイズ（ピクセル）
 * 　　　　　       dest: データを格納するバッファへのポインタ
 * 　　　　　     stride: バッファのライン長（ピクセル）
 *               swizzle: 可能であれば、画像データをswizzleすべきかどうか
 * 　　　　　swizzle_ret: 成功時、swizzleの有無を格納する変数へのポインタ
 * 【戻り値】0以外＝成功、0＝失敗
 */
int sys_display_capture(int x, int y, int w, int h, uint32_t *dest,
                        int stride, int swizzle, int *swizzle_ret)
{
    if (UNLIKELY(x < 0)
     || UNLIKELY(y < 0)
     || UNLIKELY(w <= 0)
     || UNLIKELY(h <= 0)
     || UNLIKELY(x+w > DISPLAY_WIDTH)
     || UNLIKELY(y+h > DISPLAY_HEIGHT)
     || UNLIKELY(!dest)
     || UNLIKELY(stride <= 0)
    ) {
        DMSG("Invalid parameters: %d,%d+%d,%d %p %d %d %p",
             x, y, w, h, dest, stride, swizzle, swizzle_ret);
        return 0;
    }
    if (UNLIKELY(stride % 4 != 0)) {
        DMSG("Stride %d not a multiple of 4, not supported", stride);
        return 0;
    }

    ge_sync();
    const uint32_t *src = WORK_PIXEL_ADDRESS(x,y);
    if (swizzle && stride % 4 == 0 && h % 8 == 0) {
        int line;
        for (line = 0; line < h; line += 8, src += DISPLAY_STRIDE*8 - stride) {
            int xx;
            for (xx = 0; xx < stride; xx += 4, src += 4) {
                const uint32_t *thissrc = src;
                int yy;
                for (yy = 0; yy < 8; yy++, thissrc += DISPLAY_STRIDE, dest += 4) {
                    dest[0] = thissrc[0] | 0xFF000000;
                    dest[1] = thissrc[1] | 0xFF000000;
                    dest[2] = thissrc[2] | 0xFF000000;
                    dest[3] = thissrc[3] | 0xFF000000;
                }
            }
        }
        *swizzle_ret = 1;
    } else {
        const uint32_t src_skip = DISPLAY_STRIDE - align_up(w, 4);
        const uint32_t dest_skip = stride - align_up(w, 4);
        int line;
        for (line = 0; line < h; line++, src += src_skip, dest += dest_skip) {
            int xx;
            for (xx = 0; xx < w; xx += 4, src += 4, dest += 4) {
                dest[0] = src[0] | 0xFF000000;
                dest[1] = src[1] | 0xFF000000;
                dest[2] = src[2] | 0xFF000000;
                dest[3] = src[3] | 0xFF000000;
            }
        }
        *swizzle_ret = 0;
    }
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_display_line:  指定座標を結んだ直線を指定色で描画する。引数無効の場合、
 * 何もしない。
 *
 * 【引　数】x1, y1: 点1の座標
 * 　　　　　x2, y2: 点2の座標
 * 　　　　　 color: 色と濃度（0xAARRGGBB）
 * 【戻り値】なし
 */
void sys_display_line(int x1, int y1, int x2, int y2, uint32_t color)
{
    if (UNLIKELY(x1 < 0)
     || UNLIKELY(y1 < 0)
     || UNLIKELY(x2 >= DISPLAY_WIDTH)
     || UNLIKELY(y2 >= DISPLAY_HEIGHT)
    ) {
        DMSG("Invalid parameters: %d %d %d %d %08X", x1, y1, x2, y2, color);
        return;
    }
    color = convert_ARGB32(color);

    /* 注：実験の結果、GEは座標を浮動小数点のように扱うものの、端数を切り捨
     * てるようである。このため、始点・終点のX座標またはY座標が異なる場合、
     * 正しく描画するために高い方の座標に1を加えなくてはならない。 */
    if (x1 > x2) {
        x1++;
    } else if (x2 > x1) {
        x2++;
    }
    if (y1 > y2) {
        y1++;
    } else if (y2 > y1) {
        y2++;
    }

    ge_disable(GE_STATE_DEPTH_TEST);
    ge_disable(GE_STATE_DEPTH_WRITE);
    ge_enable(GE_STATE_BLEND);
    ge_set_blend_mode(GE_BLENDSET_SRC_ALPHA);
    ge_set_vertex_format(GE_VERTEXFMT_TRANSFORM_2D
                       | GE_VERTEXFMT_COLOR_8888
                       | GE_VERTEXFMT_VERTEX_16BIT);
    ge_set_vertex_pointer(NULL);
    ge_add_color_xy_vertex(color, x1, y1);
    ge_add_color_xy_vertex(color, x2, y2);
    ge_draw_primitive(GE_PRIMITIVE_LINES, 2);
    ge_commit();
}

/*-----------------------------------------------------------------------*/

/**
 * sys_display_fill:  指定領域を指定色でフィルする。x1 > x2 または y1 > y2
 * の場合、何もしない。
 *
 * 【引　数】x1, y1: 左上隅座標
 * 　　　　　x2, y2: 右下隅座標
 * 　　　　　 color: 色と濃度（0xAARRGGBB）
 * 【戻り値】なし
 */
void sys_display_fill(int x1, int y1, int x2, int y2, uint32_t color)
{
    if (UNLIKELY(x1 < 0)
     || UNLIKELY(y1 < 0)
     || UNLIKELY(x1 > x2)
     || UNLIKELY(y1 > y2)
     || UNLIKELY(x2 >= DISPLAY_WIDTH)
     || UNLIKELY(y2 >= DISPLAY_HEIGHT)
    ) {
        DMSG("Invalid parameters: %d %d %d %d %08X", x1, y1, x2, y2, color);
        return;
    }
    color = convert_ARGB32(color);
    ge_disable(GE_STATE_DEPTH_TEST);
    ge_disable(GE_STATE_DEPTH_WRITE);
    ge_fill(x1, y1, x2+1, y2+1, color);
    ge_commit();
}

/*-----------------------------------------------------------------------*/

/**
 * sys_display_fill_light:  指定領域に指定色の光を加える。各ピクセルの赤成分
 * に r を、緑成分に g を、青成分に b を加算する。x1 > x2 または y1 > y2 の
 * 場合、何もしない。
 *
 * 【引　数】 x1, y1: 左上隅座標
 * 　　　　　 x2, y2: 右下隅座標
 * 　　　　　r, g, b: 色補正値（それぞれ-255〜255）
 * 【戻り値】なし
 */
void sys_display_fill_light(int x1, int y1, int x2, int y2,
                            int r, int g, int b)
{
    if (UNLIKELY(x1 < 0)
     || UNLIKELY(y1 < 0)
     || UNLIKELY(x1 > x2)
     || UNLIKELY(y1 > y2)
     || UNLIKELY(x2 >= DISPLAY_WIDTH)
     || UNLIKELY(y2 >= DISPLAY_HEIGHT)
     || UNLIKELY(r < -255) || UNLIKELY(r > 255)
     || UNLIKELY(g < -255) || UNLIKELY(g > 255)
     || UNLIKELY(b < -255) || UNLIKELY(b > 255)
    ) {
        DMSG("Invalid parameters: %d %d %d %d %d %d %d",
             x1, y1, x2, y2, r, g, b);
        return;
    }

    ge_disable(GE_STATE_DEPTH_TEST);
    ge_disable(GE_STATE_DEPTH_WRITE);
    ge_enable(GE_STATE_BLEND);
    ge_set_vertex_format(GE_VERTEXFMT_TRANSFORM_2D
                       | GE_VERTEXFMT_COLOR_8888
                       | GE_VERTEXFMT_VERTEX_16BIT);
    ge_set_vertex_pointer(NULL);
    if (r > 0 || g > 0 || b > 0) {
        ge_set_blend_mode(GE_BLEND_ADD, GE_BLEND_FIX, GE_BLEND_FIX,
                          0xFFFFFF, 0xFFFFFF);
        const uint32_t color = (r > 0 ? r<< 0 : 0)
                             | (g > 0 ? g<< 8 : 0)
                             | (b > 0 ? b<<16 : 0);
        ge_add_color_xy_vertex(color, x1, y1);
        ge_add_color_xy_vertex(color, x2+1, y2+1);
        ge_draw_primitive(GE_PRIMITIVE_SPRITES, 2);
    }
    if (r < 0 || g < 0 || b < 0) {
        ge_set_blend_mode(GE_BLEND_REVERSE_SUBTRACT, GE_BLEND_FIX,
                          GE_BLEND_FIX, 0xFFFFFF, 0xFFFFFF);
        const uint32_t color = (r < 0 ? (-r)<< 0 : 0)
                             | (g < 0 ? (-g)<< 8 : 0)
                             | (b < 0 ? (-b)<<16 : 0);
        ge_add_color_xy_vertex(color, x1, y1);
        ge_add_color_xy_vertex(color, x2+1, y2+1);
        ge_draw_primitive(GE_PRIMITIVE_SPRITES, 2);
    }
    ge_commit();
}

/*************************************************************************/
/************************** ライブラリ共通関数 ***************************/
/*************************************************************************/

/**
 * psp_display_init:  描画機能の初期化を行う。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
int psp_display_init(void)
{
    static int initted = 0;
    if (initted) {
        return 1;
    }

    if (!ge_init()) {
        return 0;
    }
    int32_t res = sceDisplaySetMode(0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (res < 0) {
        DMSG("sceDisplaySetMode() failed: %s", psp_strerror(res));
        return 0;
    }

    mem_clear(sceGeEdramGetAddr(), sceGeEdramGetSize());
    sceKernelDcacheWritebackAll();
    displayed_surface = 0;
    work_surface = 1;

    display_mode = PSP_DISPLAY_PIXEL_FORMAT_8888;
    display_bpp = 32;

    uint8_t *vram_addr = sceGeEdramGetAddr();
    uint32_t vram_size = sceGeEdramGetSize();
    const uint32_t frame_size =
        DISPLAY_STRIDE * DISPLAY_HEIGHT * (display_bpp/8);
    int i;
    for (i = 0; i < lenof(surfaces); i++) {
        surfaces[i] = vram_addr + i*frame_size;
    }
    depth_buffer = (uint16_t *)(vram_addr + lenof(surfaces)*frame_size);
    vram_spare_ptr = (uint8_t *)(depth_buffer + DISPLAY_STRIDE*DISPLAY_HEIGHT);
    vram_top = vram_addr + vram_size;
    work_pixels = (uint32_t *)surfaces[work_surface];
    sceDisplaySetFrameBuf(surfaces[displayed_surface], DISPLAY_STRIDE,
                          display_mode, PSP_DISPLAY_SETBUF_IMMEDIATE);

    initted = 1;
    return 1;
}

/*************************************************************************/

/**
 * psp_draw_buffer, psp_depth_buffer:  現在の描画バッファまたは奥行きバッファ
 * へのポインタを返す。バッファは480x272、ライン長512ピクセルで固定。
 *
 * 【引　数】なし
 * 【戻り値】現在の描画バッファまたは奥行きバッファへのポインタ
 */
uint32_t *psp_draw_buffer(void)
{
    return work_pixels;
}

uint16_t *psp_depth_buffer(void)
{
    return depth_buffer;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_vram_spare_ptr, psp_vram_spare_size:  描画・奥行きバッファに使われて
 * いないVRAMへのポインタ、またはそのサイズを返す。
 *
 * 【引　数】なし
 * 【戻り値】空きVRAMへのポインタ、または空きVRAMのサイズ（バイト）
 */
void *psp_vram_spare_ptr(void)
{
    return vram_spare_ptr;
}

uint32_t psp_vram_spare_size(void)
{
    return vram_top - vram_spare_ptr;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_work_pixel_address:  指定された画面座標にあるピクセルの、描画バッファ
 * 内のメモリアドレスを返す。
 *
 * 【引　数】x, y: 画面座標（ピクセル）
 * 【戻り値】指定されたピクセルのアドレス
 * 【注　意】座標の範囲チェックは行われない。
 */
uint32_t *psp_work_pixel_address(int x, int y)
{
    return WORK_PIXEL_ADDRESS(x, y);
}

/*************************************************************************/

/**
 * psp_restore_clip_area:  sys_display_clip()で設定されたクリップ範囲を
 * 改めて適用する。ge_unset_clip_area()で一時的に解除した場合、再設定する
 * ために呼び出す。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void psp_restore_clip_area(void)
{
    ge_set_clip_area(clip_left, clip_top, clip_right, clip_bottom);
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * do_buffer_flip:  描画と表示バッファを入れ替える。通常はフレーム描画直後、
 * スレッドとして呼び出され、sceDisplayWaitVblankStart()を待って戻ることで
 * sys_display_start()でフレームが進んだかどうか確認できる。万一スレッドを
 * 開始できない場合、フレーム終了時に直接呼び出すことも出来る。
 *
 * 【引　数】args: 引数データサイズ（常にsizeof(void *)）
 * 　　　　　argp: 引数データポインタ（描画バッファポインタへのポインタ）
 * 【戻り値】なし
 */
static void do_buffer_flip(SceSize args, void *argp)
{
#ifdef GE_SYNC_IN_THREAD
    void *my_work_pixels = *(void **)argp;

    ge_end_frame();
    sceDisplaySetFrameBuf(my_work_pixels, DISPLAY_STRIDE, display_mode,
                          PSP_DISPLAY_SETBUF_NEXTFRAME);
#endif
    sceDisplayWaitVblankStart();
}

/*************************************************************************/

#ifdef DEBUG

/**
 * display_ge_debug_info:  GEデバッグ情報を表示する。デバッグ時のみ実装。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
static void display_ge_debug_info(void)
{
    int gelist_used, gelist_used_max, gelist_size;
    int vertlist_used, vertlist_used_max, vertlist_size;
    ge_get_debug_info(&gelist_used, &gelist_used_max, &gelist_size,
                      &vertlist_used, &vertlist_used_max, &vertlist_size);

    char buf[100];
    int x = 0 + iroundf(debugfont_textwidth("VLIST: ", 1, 0, NULL));
    int y = DISPLAY_HEIGHT - 3*iroundf(debugfont_height(1));

    debugfont_draw_text("DLIST: ", x, y, 0xFFFFFF, 1, 1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%d/", gelist_used);
    debugfont_draw_text(buf, x+31, y, 0xFFFFFF, 1, 1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%d/", gelist_used_max);
    debugfont_draw_text(buf, x+62, y, 0xFFFFFF, 1, 1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%d", gelist_size);
    debugfont_draw_text(buf, x+87, y, 0xFFFFFF, 1, 1, FONTSTYLE_ALIGN_RIGHT);

    y += iroundf(debugfont_height(1));

    debugfont_draw_text("VLIST: ", x, y, 0xFFFFFF, 1, 1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%d/", vertlist_used);
    debugfont_draw_text(buf, x+31, y, 0xFFFFFF, 1, 1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%d/", vertlist_used_max);
    debugfont_draw_text(buf, x+62, y, 0xFFFFFF, 1, 1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%d", vertlist_size);
    debugfont_draw_text(buf, x+87, y, 0xFFFFFF, 1, 1, FONTSTYLE_ALIGN_RIGHT);
}

#endif  // DEBUG

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
