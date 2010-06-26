/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/input.c: Input device management.
 */

#include "common.h"
#include "input.h"
#include "sysdep.h"

#ifdef DEBUG
# include "memory.h"  // デバッグ情報表示切り替えのため
# include "timer.h"   // 同上
#endif

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* ボタンの押下状況 */
static uint8_t cur_buttons[INPUT_MAX_BUTTONS];

/* 新しく押されたボタンのインデックス（負数＝なし） */
static int pressed_button = -1;

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * input_update:  各種入力デバイスの現状を取得する。以下の情報取得関数は
 * 全て、この関数が呼び出された時点での状況を参照する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void input_update(void)
{
    /* 各入力デバイスの状況を更新する */
    sys_input_update();

    /* ジョイスティックボタンの押下状況を取得する */
    pressed_button = -1;
    unsigned int i;
    for (i = 0; i < lenof(cur_buttons); i++) {
        const uint8_t new_button = sys_input_buttonstate(i);
        if (pressed_button < 0 && new_button && !cur_buttons[i]) {
            pressed_button = i;
        }
        cur_buttons[i] = new_button;
    }

#ifdef DEBUG
    /* Ctrl+[MC]、PSPでは□＋[LR]ボタンでメモリ・CPUのデバッグ表示を切り
     * 替える */
# ifdef PSP
    if (cur_buttons[15] && pressed_button == 8) {
        debug_memory_display_flag = !debug_memory_display_flag;
    }
    if (cur_buttons[15] && pressed_button == 9) {
        debug_cpu_display_flag = !debug_cpu_display_flag;
    }
# else
    static int last_key_m, last_key_c;
    if (sys_input_keystate('m')) {
        if (!last_key_m && (sys_input_keystate(KEY_LCTRL)
                            || sys_input_keystate(KEY_RCTRL))) {
            debug_memory_display_flag = !debug_memory_display_flag;
        }
        last_key_m = 1;
    } else {
        last_key_m = 0;
    }
    if (sys_input_keystate('c')) {
        if (!last_key_m && (sys_input_keystate(KEY_LCTRL)
                            || sys_input_keystate(KEY_RCTRL))) {
            debug_cpu_display_flag = !debug_cpu_display_flag;
        }
        last_key_c = 1;
    } else {
        last_key_c = 0;
    }
# endif
#endif
}

/*************************************************************************/

/**
 * input_button_state:  指定されたジョイスティックボタンの状態を返す。
 *
 * 【引　数】button: ボタン番号（0〜INPUT_MAX_BUTTONS-1）
 * 【戻り値】ボタンが押されている場合は0以外、押されていない場合（もしくは
 * 　　　　　番号に対応するボタンが存在しない場合）は0
 */
int input_button_state(int button)
{
    if (UNLIKELY(button < 0 || button >= INPUT_MAX_BUTTONS)) {
	DMSG("Invalid parameters: %d", button);
	return 0;
    }
    return cur_buttons[button];
}

/*-----------------------------------------------------------------------*/

/**
 * input_pressed_button:  直近のinput_update()呼び出し時、新しく押された
 * ボタンの番号を返す。
 *
 * 【引　数】なし
 * 【戻り値】新しく押されたボタンの番号（新しく押されたボタンがない場合は-1）
 */
int input_pressed_button(void)
{
    return pressed_button;
}

/*-----------------------------------------------------------------------*/

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
int input_stick_pos(int stick, float *xpos, float *ypos)
{
    if (UNLIKELY(stick < 0 || stick >= INPUT_MAX_STICKS)) {
	DMSG("Invalid parameters: %d", stick);
	return 0;
    }
    return sys_input_joypos(stick, xpos, ypos);
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
