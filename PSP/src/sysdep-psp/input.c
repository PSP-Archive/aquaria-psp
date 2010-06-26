/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/input.c: PSP user input interface.
 */

#include "../common.h"
#include "../sysdep.h"

#include "psplocal.h"

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* アナログスティックの入力閾値（中央＝128からの距離） */
#define ANALOG_THRESHOLD  32

/*-----------------------------------------------------------------------*/

/* ボタン状態 （1＝押されている） */
static uint8_t buttons[16];

/* ジョイスティック状態 */
static float joy_x, joy_y;

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * sys_input_update:  各種入力デバイスの現状を取得する。以下の情報取得関数
 * は全て、この関数が呼び出された時点での状況を参照する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sys_input_update(void)
{
    SceCtrlData pad_data;

    /* 現在のパッド状態を取得 */
    sceCtrlPeekBufferPositive(&pad_data, 1);

    /* アナログスティック処理 */
    if (pad_data.Lx >= 128 - ANALOG_THRESHOLD
     && pad_data.Lx < 128 + ANALOG_THRESHOLD
    ) {
        joy_x = 0;
    } else {
        joy_x = (pad_data.Lx - 127.5f) * (1/127.5f);
    }
    if (pad_data.Ly >= 128 - ANALOG_THRESHOLD
     && pad_data.Ly < 128 + ANALOG_THRESHOLD
    ) {
        joy_y = 0;
    } else {
        joy_y = (pad_data.Ly - 127.5f) * (1/127.5f);
    }

    /* ファームウェアバグ対応。
     * (1) HOLDスイッチを入れても、アナログスティックが反応してしまうので、
     *     無効化する。
     * (2) アナログスティックを動かしても、PSPの省エネタイマーがリセット
     *     されないので、手動的にリセットする。
     */
    if (pad_data.Buttons & PSP_CTRL_HOLD) {
        joy_x = joy_y = 0;
    }
    if (joy_x || joy_y) {
        scePowerTick(0);
    }

    /* ボタン処理 */
    mem_clear(buttons, sizeof(buttons));
    unsigned int i;
    for (i = 0; i < lenof(buttons); i++) {
        buttons[i] = (pad_data.Buttons >> i) & 1;
    }
}

/*************************************************************************/

/**
 * sys_input_keystate:  指定されたキーの状態を返す。
 *
 * 【引　数】key: キー値（ASCII文字またはSYS_KEY_*定数）
 * 【戻り値】キーが押されている場合は0以外、押されていない場合（もしくは
 * 　　　　　キー値が無効な場合）は0
 */
int sys_input_keystate(int key)
{
    /* PSPにはキーボードがない */
    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_input_buttonstate:  指定されたジョイスティックボタンの状態を返す。
 *
 * 【引　数】button: ボタン番号（0〜）
 * 【戻り値】ボタンが押されている場合は0以外、押されていない場合（もしくは
 * 　　　　　番号に対応するボタンが存在しない場合）は0
 */
int sys_input_buttonstate(int button)
{
    if (UNLIKELY(button < 0) || UNLIKELY(button >= lenof(buttons))) {
        return 0;
    }
    return buttons[button];
}

/*-----------------------------------------------------------------------*/

/**
 * sys_input_joypos:  ジョイスティック系デバイスのスティック位置をxpos
 * （水平軸）とypos（垂直軸）に格納する。値は、左いっぱい・上いっぱいで-1、
 * 右いっぱい・下いっぱいで+1となる。
 *
 * 【引　数】stick: スティックインデックス（0〜）
 * 　　　　　 xpos: 水平軸の値を格納するポインタ（NULLも可）
 * 　　　　　 ypos: 垂直軸の値を格納するポインタ（NULLも可）
 * 【戻り値】0以外＝成功、0＝失敗（ジョイスティック未接続、インデックスに
 * 　　　　　対応するスティックが存在しないなど）
 */
int sys_input_joypos(int stick, float *xpos, float *ypos)
{
    if (stick != 0) {
        return 0;
    }

    if (xpos) {
        *xpos = joy_x;
    }
    if (ypos) {
        *ypos = joy_y;
    }
    return 1;
}

/*************************************************************************/
/************************ PSPライブラリ内部用関数 ************************/
/*************************************************************************/

/**
 * psp_input_init:  入力機能の初期化を行う。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
int psp_input_init(void)
{
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    return 1;
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
