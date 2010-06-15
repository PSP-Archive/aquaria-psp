/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/graphics.h: Graphics function header.
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

/*************************************************************************/

/**
 * graphics_display_width, graphics_display_height:  画面の幅または高さを返す。
 * 同一フレーム内、画面のサイズは変わらない。
 *
 * 【引　数】なし
 * 【戻り値】画面の幅または高さ（ピクセル）
 */
extern PURE_FUNCTION int graphics_display_width(void);
extern PURE_FUNCTION int graphics_display_height(void);

/*----------------------------------*/

/**
 * graphics_start_frame:  1フレームの描画を開始する。毎フレーム、他の描画
 * 関数より先に呼び出さなければならない。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void graphics_start_frame(void);

/**
 * graphics_finish_frame:  1フレームの描画を終了する。毎フレーム、すべての
 * 描画処理が終わった後に呼び出さなければならない。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void graphics_finish_frame(void);

/**
 * graphics_sync:  ハードウェアが行っている描画処理が全て完了するのを待つ。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void graphics_sync(void);

/*----------------------------------*/

/**
 * graphics_set_fullscreen:  全画面表示の有無を設定する。ウィンドウ表示の
 * 概念がないシステムでは何もしない。
 *
 * 【引　数】on: 全画面表示フラグ（0以外＝全画面表示）
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int graphics_set_fullscreen(int on);

/*----------------------------------*/

/**
 * graphics_clear:  画面全体を黒にクリアする。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void graphics_clear(void);

/**
 * graphics_draw_line:  画面に直線を描画する。座標は画面外でも構わない（画
 * 面内の部分だけ描画される）。
 *
 * 【引　数】x1, y1, x2, y2: 直線の両端の座標（ピクセル単位、画面左上＝0,0）
 * 　　　　　         color: 色と濃度（0xAARRGGBB）
 * 【戻り値】なし
 */
extern void graphics_draw_line(int x1, int y1, int x2, int y2, uint32_t color);

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
extern void graphics_draw_box(int x, int y, int w, int h, uint32_t color);

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
extern void graphics_fill_box(int x, int y, int w, int h, uint32_t color);

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
extern void graphics_fill_light(int x, int y, int w, int h,
                                int r, int g, int b);

/*************************************************************************/

#endif  // GRAPHICS_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
