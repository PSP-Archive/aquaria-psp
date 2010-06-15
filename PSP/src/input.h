/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/input.h: Input device management header.
 */

#ifndef INPUT_H
#define INPUT_H

/*************************************************************************/

/* サポートされているジョイスティックボタン・スティック数 */

#define INPUT_MAX_BUTTONS  32
#define INPUT_MAX_STICKS   4

/*-----------------------------------------------------------------------*/

/**
 * input_update:  各種入力デバイスの現状を取得する。以下の情報取得関数は
 * 全て、この関数が呼び出された時点での状況を参照する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void input_update(void);

/**
 * input_button_state:  指定されたジョイスティックボタンの状態を返す。
 *
 * 【引　数】button: ボタン番号（0〜INPUT_MAX_BUTTONS-1）
 * 【戻り値】ボタンが押されている場合は0以外、押されていない場合（もしくは
 * 　　　　　番号に対応するボタンが存在しない場合）は0
 */
extern int input_button_state(int button);

/**
 * input_pressed_button:  直近のinput_update()呼び出し時、新しく押された
 * ボタンの番号を返す。
 *
 * 【引　数】なし
 * 【戻り値】新しく押されたボタンの番号（新しく押されたボタンがない場合は-1）
 */
extern int input_pressed_button(void);

/**
 * input_stick_pos:  ジョイスティック系デバイスのスティック位置をxpos
 * （水平軸）とypos（垂直軸）に格納する。値は、左いっぱい・上いっぱいで-1、
 * 右いっぱい・下いっぱいで+1となる。
 *
 * 【引　数】stick: スティックインデックス（0〜INPUT_MAX_STICK-1）
 * 　　　　　 xpos: 水平軸の値を格納するポインタ（NULLも可）
 * 　　　　　 ypos: 垂直軸の値を格納するポインタ（NULLも可）
 * 【戻り値】0以外＝成功、0＝失敗（ジョイスティック未接続、インデックスに
 * 　　　　　対応するスティックが存在しないなど）
 */
extern int input_stick_pos(int stick, float *xpos, float *ypos);

/*************************************************************************/

#endif  // INPUT_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
