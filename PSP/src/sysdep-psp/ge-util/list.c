/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/list.c: Display list management routines for the
 * GE utility library.
 */

#include "../../common.h"
#include "../ge-util.h"
#include "../psplocal.h"

#include "ge-const.h"
#include "ge-local.h"

/*************************************************************************/

/* 現在のサブリストのベースポインタ */
static uint32_t *sublist_base;

/*************************************************************************/
/*************************************************************************/

/**
 * ge_add_command, ge_add_commandf:  任意のGE命令を登録する。
 *
 * 【引　数】  command: 命令（0〜255）
 * 　　　　　parameter: 命令パラメータ（24ビット整数または浮動小数点）
 * 【戻り値】なし
 */
void ge_add_command(uint8_t command, uint32_t parameter)
{
    if (UNLIKELY(parameter & 0xFF000000)) {
        DMSG("Command %d: parameter 0x%08X has high bits set!",
             command, parameter);
        parameter &= 0x00FFFFFF;
    }
    if (UNLIKELY(gelist_ptr >= gelist_limit)) {
        DMSG("Command %d parameter 0x%06X: list full!", command, parameter);
        return;
    }

    internal_add_command(command, parameter);
}

void ge_add_commandf(uint8_t command, float parameter)
{
    if (UNLIKELY(gelist_ptr >= gelist_limit)) {
        DMSG("Command %d parameter %f: list full!", command, parameter);
        return;
    }

    internal_add_commandf(command, parameter);
}

/*************************************************************************/

/**
 * ge_start_sublist:  サブリストの作成を開始する。
 *
 * 【引　数】list: サブリストバッファへのポインタ
 * 　　　　　size: サブリストバッファのサイズ（ワード単位）
 * 【戻り値】0以外＝成功、0＝失敗（すでにサブリストを作成中）
 */
int ge_start_sublist(uint32_t *list, int size)
{
    if (!list || size <= 0) {
        DMSG("Invalid parameters: %p %d", list, size);
        return 0;
    }
    if (saved_gelist_ptr) {
        DMSG("Already creating a sublist!");
        return 0;
    }

    saved_gelist_ptr   = gelist_ptr;
    saved_gelist_limit = gelist_limit;

    /* サブリストの場合、すぐに実行する必要はないので、非キャッシュアクセス
     * を使うのではなく、リスト作成完了時に一括書き込みを行う。 */
    sublist_base       = (uint32_t *)list;
    gelist_ptr         = sublist_base;
    gelist_limit       = gelist_ptr + size;

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * ge_replace_sublist:  作成中のサブリストを別のメモリ領域に置き換える。
 * リスト満杯時、リストバッファを再確保したときに呼び出されることを想定
 * している。
 *
 * サブリスト作成中でない場合、何もしない。
 *
 * 【引　数】list: サブリストバッファへのポインタ
 * 　　　　　size: サブリストバッファのサイズ（ワード単位）
 * 【戻り値】なし
 */
void ge_replace_sublist(uint32_t *list, int size)
{
    if (!list || size <= 0) {
        DMSG("Invalid parameters: %p %d", list, size);
        return;
    }
    if (!saved_gelist_ptr) {
        DMSG("Not currently creating a sublist!");
        return;
    }

    const uint32_t offset = gelist_ptr - sublist_base;
    sublist_base = (uint32_t *)list;
    gelist_ptr   = sublist_base + offset;
    gelist_limit = sublist_base + size;
}

/*-----------------------------------------------------------------------*/

/**
 * ge_finish_sublist:  現在作成中のサブリストを終了させる。サブリスト作成中
 * でない場合、何もせずNULLを返す。
 *
 * 【引　数】なし
 * 【戻り値】リスト終端へのポインタ（最後の命令の直後を指す）
 */
uint32_t *ge_finish_sublist(void)
{
    if (!saved_gelist_ptr) {
        return NULL;
    }

    if (gelist_ptr >= gelist_limit) {
        DMSG("Sublist overflow at %p, dropping last insn", gelist_ptr);
        gelist_ptr = gelist_limit-1;
    }
    internal_add_command(GECMD_RETURN, 0);
    sceKernelDcacheWritebackRange(
        sublist_base, (uintptr_t)gelist_ptr - (uintptr_t)sublist_base
    );
    uint32_t *retval   = gelist_ptr;
    sublist_base       = NULL;
    gelist_ptr         = saved_gelist_ptr;
    gelist_limit       = saved_gelist_limit;
    saved_gelist_ptr   = NULL;
    saved_gelist_limit = NULL;
    return retval;
}

/*-----------------------------------------------------------------------*/

/**
 * ge_call_sublist:  サブリストを呼び出す。
 *
 * 【引　数】list: サブリストへのポインタ
 * 【戻り値】なし
 */
void ge_call_sublist(const uint32_t *list)
{
    CHECK_GELIST(2);
    internal_add_command(GECMD_ADDRESS_BASE, ((uint32_t)list & 0xFF000000)>>8);
    internal_add_command(GECMD_CALL, (uint32_t)list & 0x00FFFFFF);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_sublist_free:  現在作成中のサブリストの空きワード数を返す。
 *
 * 【引　数】なし
 * 【戻り値】空きワード数（サブリスト作成中でない場合は0）
 */
uint32_t ge_sublist_free(void)
{
    if (!saved_gelist_ptr) {
        return 0;
    }

    return gelist_limit - gelist_ptr;
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
