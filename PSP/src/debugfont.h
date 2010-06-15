/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/debugfont.h: Header for debug font routines.
 */

#ifndef DEBUGFONT_H
#define DEBUGFONT_H

#ifdef DEBUG  // ファイル全体にかかる

/*************************************************************************/

/*
 * FONTSTYLE_*
 *
 * フォント描画スタイル定数。
 */
enum {
    FONTSTYLE_NORMAL     = 0,       // 通常（フラグなし）
    FONTSTYLE_ITALIC     = 1<<0,    // 斜体
    FONTSTYLE_SHADOW     = 1<<1,    // 影付加（右下1ピクセル幅）

    FONTSTYLE_ALIGN_LEFT   = 0,     // 左寄せ（デフォルト）
    FONTSTYLE_ALIGN_CENTER = 1<<2,  // 中央寄せ
    FONTSTYLE_ALIGN_RIGHT  = 1<<3,  // 右寄せ
};

/*-----------------------------------------------------------------------*/
/**
 * debugfont_init:  デバッグフォント機能の初期化を行う。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern int debugfont_init(void);

/**
 * debugfont_cleanup:  デバッグフォントデータを破棄する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void debugfont_cleanup(void);

/**
 * debugfont_height:  デバッグフォントの文字の高さを返す。
 *
 * 【引　数】scale: 文字の拡大倍率（1.0＝拡大・縮小しない）
 * 【戻り値】文字の高さ（ピクセル）
 */
extern float debugfont_height(float scale);

/**
 * debugfont_textwidth:  UTF-8文字列の幅を計算して返す。style引数における
 * 描画フラグ（FONTSTYLE_ALIGN_*）は無視される。
 *
 * 【引　数】         str: 文字列（UTF-8）
 * 　　　　　       scale: 文字の拡大倍率（1.0＝拡大・縮小しない）
 * 　　　　　       style: 文字スタイル（FONTSTYLE_*）
 * 　　　　　lastkern_ptr: 最終文字の右側カーニング値を格納する変数への
 * 　　　　　              ポインタ（NULL可）。この値は戻り値には含まれない。
 * 【戻り値】文字列を描画した際の合計幅（0＝エラーまたは空文字列）
 */
extern float debugfont_textwidth(const char *str, float scale, int style,
				 float *lastkern_ptr);

/**
 * debugfont_draw_text:  UTF-8文字列を描画する。
 *
 * 【引　数】  str: 文字列（UTF-8）
 * 　　　　　 x, y: 画面座標（左上隅、小数点以下有意）
 * 　　　　　color: 文字色（0xRRGGBB）
 * 　　　　　alpha: 濃度（0.0〜1.0、0.0＝透明）
 * 　　　　　scale: 文字の拡大倍率（1.0＝拡大・縮小しない）
 * 　　　　　style: 文字スタイル・描画フラグ（FONTSTYLE_*）
 * 【戻り値】文字列の幅（ピクセル、0＝エラーまたは空文字列）
 */
extern float debugfont_draw_text(const char *str, float x, float y, uint32_t color,
				 float alpha, float scale, int style);

/*************************************************************************/

#endif  // DEBUG
#endif  // DEBUGFONT_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */

