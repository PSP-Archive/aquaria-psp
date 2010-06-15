/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/graphics.c: Graphics functions.
 */

#include "common.h"
#include "graphics.h"
#include "sysdep.h"

#ifdef DEBUG
# include "memory.h"  // デバッグ情報表示のため
# include "malloc.h"  // 同上
# include "timer.h"   // 同上
#endif

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* 現在の画面サイズ（ピクセル）、0＝未初期化 */
static int display_width = 0, display_height = 0;

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * graphics_display_width, graphics_display_height:  画面の幅または高さを返す。
 * 同一フレーム内、画面のサイズは変わらない。
 *
 * 【引　数】なし
 * 【戻り値】画面の幅または高さ（ピクセル）
 */
int graphics_display_width(void)
{
    if (UNLIKELY(display_width <= 0)) {
        DMSG("graphics_display_width() outside a frame!");
        display_width = 640;
        display_height = 480;
    }
    return display_width;
}

int graphics_display_height(void)
{
    if (UNLIKELY(display_height <= 0)) {
        DMSG("graphics_display_height() outside a frame!");
        display_width = 640;
        display_height = 480;
    }
    return display_height;
}

/*************************************************************************/

/**
 * graphics_start_frame:  1フレームの描画を開始する。毎フレーム、他の描画
 * 関数より先に呼び出さなければならない。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void graphics_start_frame(void)
{
    sys_display_start(&display_width, &display_height);
}

/*-----------------------------------------------------------------------*/

/**
 * graphics_finish_frame:  1フレームの描画を終了する。毎フレーム、すべての
 * 描画処理が終わった後に呼び出さなければならない。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void graphics_finish_frame(void)
{
#ifdef DEBUG
    mem_display_debuginfo();
    malloc_display_debuginfo();
    timer_display_debuginfo();
#endif

    sys_display_finish();

#ifdef DEBUG
    display_width = display_height = 0;  // フレーム外描画時に警告する
#endif
}

/*-----------------------------------------------------------------------*/

/**
 * graphics_sync:  ハードウェアが行っている描画処理が全て完了するのを待つ。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void graphics_sync(void)
{
    sys_display_sync();
}

/*************************************************************************/

/**
 * graphics_set_fullscreen:  全画面表示の有無を設定する。ウィンドウ表示の
 * 概念がないシステムでは何もしない。
 *
 * 【引　数】on: 全画面表示フラグ（0以外＝全画面表示）
 * 【戻り値】0以外＝成功、0＝失敗
 */
int graphics_set_fullscreen(int on)
{
    return sys_display_set_fullscreen(on);
}

/*************************************************************************/

/**
 * graphics_clear:  画面全体を黒にクリアする。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void graphics_clear(void)
{
    sys_display_clear();
}

/*-----------------------------------------------------------------------*/

/**
 * graphics_draw_line:  画面に直線を描画する。座標は画面外でも構わない（画
 * 面内の部分だけ描画される）。
 *
 * 【引　数】x1, y1, x2, y2: 直線の両端の座標（ピクセル単位、画面左上＝0,0）
 * 　　　　　         color: 色と濃度（0xAARRGGBB）
 * 【戻り値】なし
 */
void graphics_draw_line(int x1, int y1, int x2, int y2, uint32_t color)
{
    sys_display_line(x1, y1, x2, y2, color);
}

/*-----------------------------------------------------------------------*/

/**
 * graphics_draw_box:  画面に四角形を描画する。座標は画面外でも構わない（画
 * 面内の部分だけ描画される）。動作は、
 * 　　graphics_draw_line(x,       y,       x+(w-1), y,       color);
 * 　　graphics_draw_line(x+(w-1), y,       x+(w-1), y+(h-1), color);
 * 　　graphics_draw_line(x+(w-1), y+(h-1), x,       y+(h-1), color);
 * 　　graphics_draw_line(x,       y+(h-1), x,       y,       color);
 * と同様で、w==0またはh==0の場合は何もしない。
 *
 * 【引　数】 x, y: 四角形の基準点の座標（ピクセル単位、画面左上＝0,0）
 * 　　　　　 w, h: 四角形の幅と高さ（ピクセル単位）
 * 　　　　　color: 色と濃度（0xAARRGGBB）
 * 【戻り値】なし
 */
void graphics_draw_box(int x, int y, int w, int h, uint32_t color)
{
    if (UNLIKELY(w == 0) || UNLIKELY(h == 0)) {
        return;
    }
    graphics_draw_line(x,       y,       x+(w-1), y,       color);
    graphics_draw_line(x+(w-1), y,       x+(w-1), y+(h-1), color);
    graphics_draw_line(x+(w-1), y+(h-1), x,       y+(h-1), color);
    graphics_draw_line(x,       y+(h-1), x,       y,       color);
}

/*-----------------------------------------------------------------------*/

/**
 * graphics_fill_box:  画面に塗り潰された四角形を描画する。座標は画面外でも
 * 構わない（画面内の部分だけ描画される）。塗り潰される領域は、
 * graphics_draw_box(x,y,w,h,color)を呼び出したときに描画される四角形とその
 * 内側で、w==0またはh==0の場合は何もしない。
 *
 * 【引　数】 x, y: 四角形の基準点の座標（ピクセル単位、画面左上＝0,0）
 * 　　　　　 w, h: 四角形の幅と高さ（ピクセル単位）
 * 　　　　　color: 色と濃度（0xAARRGGBB）
 * 【戻り値】なし
 */
void graphics_fill_box(int x, int y, int w, int h, uint32_t color)
{
    int x1 = x, y1 = y;
    int x2 = x1 + (w-1), y2 = y1 + (h-1);
    if (UNLIKELY(w == 0) || UNLIKELY(h == 0)) {
        return;
    }

    if (x1 > x2) {
        int tmp = x1;  x1 = x2;  x2 = tmp;
    }
    if (y1 > y2) {
        int tmp = y1;  y1 = y2;  y2 = tmp;
    }
    sys_display_fill(x1, y1, x2, y2, color);
}

/*-----------------------------------------------------------------------*/

/**
 * graphics_fill_light:  画面に光の四角形を描画する。graphics_fill_box()と
 * 違って、色を赤・緑・青別に指定し、指定された値をピクセルデータに加算する
 * （アルファブレンドではない）。w==0またはh==0の場合は何もしない。
 *
 * 【引　数】   x, y: 四角形の基準点の座標（ピクセル単位、画面左上＝0,0）
 * 　　　　　   w, h: 四角形の幅と高さ（ピクセル単位）
 * 　　　　　r, g, b: 色補正値（それぞれ-255〜255）
 * 【戻り値】なし
 */
void graphics_fill_light(int x, int y, int w, int h,
                         int r, int g, int b)
{
    int x1 = x, y1 = y;
    int x2 = x1 + (w-1), y2 = y1 + (h-1);
    if (UNLIKELY(w == 0) || UNLIKELY(h == 0)) {
        return;
    }

    if (x1 > x2) {
        int tmp = x1;  x1 = x2;  x2 = tmp;
    }
    if (y1 > y2) {
        int tmp = y1;  y1 = y2;  y2 = tmp;
    }
    sys_display_fill_light(x1, y1, x2, y2, bound(r,-255,255),
                           bound(g,-255,255), bound(b,-255,255));
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
