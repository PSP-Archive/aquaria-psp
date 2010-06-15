/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/timer.h: Header for timing routines.
 */

#ifndef TIMER_H
#define TIMER_H

/*************************************************************************/

/**
 * timer_init:  タイマーを初期化する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void timer_init(void);

/**
 * timer_left:  現在のフレームの残り処理時間（フレームスキップを起こさない
 * ために処理を終えなければならない時間）を返す。
 *
 * 【引　数】なし
 * 【戻り値】フレームの残り処理時間（秒）
 */
extern float timer_left(void);

/**
 * timer_wait:  次のフレーム処理時刻まで待つ。
 *
 * 【引　数】なし
 * 【戻り値】前のフレーム処理開始からの名目経過時間（実値をフレームレートの
 * 　　　　　倍数に切り下げたもの。秒単位）
 */
extern float timer_wait(void);

/**
 * timer_reset:  現時点までに蓄積した遅延をクリアする。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void timer_reset(void);

/**
 * timer_setrate:  フレームレートを設定する。0を指定すると、基本フレーム
 * レート（sys_base_framerate()の戻り値）に戻す。
 *
 * 【引　数】rate: フレームレート（秒／フレーム）
 * 【戻り値】なし
 */
extern void timer_setrate(double rate);

/**
 * timer_getrate, timer_getratef:  現在のフレームレートを返す。timer_getrate()
 * では倍精度で、timer_getratef()では単精度で返す。
 *
 * 【引　数】なし
 * 【戻り値】現在のフレームレート（秒／フレーム）
 */
extern double timer_getrate(void);
extern float timer_getratef(void);

/*************************************************************************/

#ifdef DEBUG  // デバッグ用関数

/* CPU使用率表示フラグ */
extern int debug_cpu_display_flag;

/**
 * timer_mark:  現時点までの処理時間使用率を記録する。
 *
 * 【引　数】id: マークの種類（TIMER_MARK_*）
 * 【戻り値】なし
 */
extern void timer_mark(int id);
enum {
    TIMER_MARK_START,           // フレーム処理開始（外部からは使用禁止）
    TIMER_MARK_PROCESS_START,   // ゲーム処理開始
    TIMER_MARK_PROCESS_MID,     // ゲーム処理途中（描画関連処理開始）
    TIMER_MARK_PROCESS_END,     // ゲーム処理終了・描画開始
    TIMER_MARK_DISPLAY_END,     // 描画終了
    TIMER_MARK__NUM             // 種類の数
};

/**
 * timer_display_debuginfo:  Ctrl+Cを監視し、押された時にCPU使用状況を表示
 * する。デバッグ時のみ実装。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void timer_display_debuginfo(void);

#endif


/* 非デバッグ時も、timer_mark()を一応定義しておく。あちこちに#ifdef DEBUGが
 * 溢れないように */
#ifndef DEBUG
# define timer_mark(index) /*nothing*/
#endif

/*************************************************************************/

#endif  // TIMER_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
