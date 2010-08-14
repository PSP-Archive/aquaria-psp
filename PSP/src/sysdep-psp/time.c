/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/time.c: PSP timing functions.
 */

#include "../common.h"
#include "../sysdep.h"

#include "psplocal.h"

/*************************************************************************/

/* ローカル関数宣言 */

static void check_suspend_resume(void);

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * sys_time_base_framerate:  システムの基本フレームレートを返す。
 *
 * 【引　数】なし
 * 【戻り値】基本フレームレート（秒／フレーム）
 */
double sys_time_base_framerate(void)
{
    return 1.001/60;  // PSPはNTSC準拠
}

/*************************************************************************/

/**
 * sys_time_now:  現在の時刻をなるべく高精度で返す。戻り値の単位は秒だが、
 * 数値自体の意味はシステム依存で、他の時刻との比較にのみ使える。
 *
 * 【引　数】なし
 * 【戻り値】現在の時刻（秒単位、小数点以下も有意）
 */
double sys_time_now(void)
{
#ifdef __GNUC__
    /* 現在のGCCでは、本来の「sceKernelGetSystemTimeWide() * 0.000001」が
     * 相当重いので、手前で整数→浮動小数点変換を行う */
    double now;
    asm(".set push; .set noreorder\n"
        "jal sceKernelGetSystemTimeWide\n"
        "nop\n"

        "clz $a0, $v0\n"
        "clz $a1, $v1\n"
        "addiu $a2, $a0, 32\n"
        "movn $a2, $a1, $v1\n"  // 先頭の0ビットの数
        "addiu $a3, $a2, -11\n"
        "beqzl $a3, 1f\n"
        "nop\n"                 // シフトなしの場合
        "bltz $a3, 0f\n"

        "li $t0, 32\n"          // 左シフトの場合
        "subu $t0, $t0, $a3\n"
        "srlv $a1, $v0, $t0\n"
        "sllv $v0, $v0, $a3\n"
        "sllv $v1, $v1, $a3\n"
        "or $v1, $v1, $a1\n"
        "slti $t1, $a3, 32\n"
        "movz $v1, $v0, $t1\n"
        "b 1f\n"
        "movz $v0, $zero, $t1\n"

        "0:"                    // 右シフトの場合
        "negu $a3, $a3\n"
        "li $t0, 32\n"
        "subu $t0, $t0, $a3\n"
        "sllv $a0, $v1, $t0\n"
        "srlv $v0, $v0, $a3\n"
        "srlv $v1, $v1, $a3\n"
        "or $v0, $v0, $a0\n"

        "1:"                    // 共通処理
        "ext $v1, $v1, 0, 20\n"
        "li $a0, 0x43E\n"
        "subu $a0, $a0, $a2\n"
        "ins $v1, $a0, 20, 11\n"
        "sw $v0, %0\n"
        "sw $v1, 4+%0\n"

        ".set pop"
        : "=m" (now)
        : /* no inputs */
        : "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1",
          "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9", "ra"
    );
    return now * 0.000001;
#else  // !__GNUC__
    return sceKernelGetSystemTimeWide() * 0.000001;
#endif
}

/*************************************************************************/

/**
 * sys_time_delay:  指定の秒数だけプログラムの実行を停止する。指定の時間より
 * 短く停止することはないが、システムによっては長く停止する場合がある。0を
 * 指定すると停止しないが、割り込みの確認など、定期的に行う必要のあるシステ
 * ム依存処理は行う。
 *
 * 【引　数】time: 停止する時間（秒単位、60秒未満。小数点以下も有意）
 * 【戻り値】なし
 * 【注　意】60秒以上の停止時間を指定した場合の動作は不定。
 */
void sys_time_delay(double time)
{
    check_suspend_resume();

    if (time >= 0x7FFFFFFF/1000000.0) {
        DMSG("WARNING: delays >2147s not supported (time=%.3f)", time);
    }

    const int32_t target = sceKernelGetSystemTimeLow() + iround(time*1000000);

#if 0  //FIXME: no longer applicable
    /* 必ず1フレーム以上待つ（表示バグ回避の為） */
    sceDisplayWaitVblankStart();
#endif

    while ((int32_t)(target-sceKernelGetSystemTimeLow()) > 0 || psp_suspend) {
        sceDisplayWaitVblankStart();
    }
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * check_suspend_resume:  電源イベントに応じて、システムの休止・再開処理を
 * 行う。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
static void check_suspend_resume(void)
{
    if (psp_suspend) {
        sys_sound_pause();
        psp_file_pause();
        psp_suspend_ok = 1;  // 休止準備完了

        while (psp_suspend) {
            sceKernelDelayThread(10000);
        }

        psp_file_unpause();
        sys_sound_unpause();
    }
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
