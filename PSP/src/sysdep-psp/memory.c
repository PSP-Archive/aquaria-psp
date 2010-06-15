/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/memory.c: PSP memory management functions.
 */

#include "../common.h"
#include "../sysdep.h"

#include "psplocal.h"

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* 各プールのサイズ */
#define MAIN_POOLSIZE      (46*1024*1024)
#define TEMP_POOLSIZE      (512*1024)

/*************************************************************************/

/* 各プールのアドレス・サイズ */
static void *main_pool, *temp_pool;
static uint32_t main_poolsize, temp_poolsize;

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * sys_mem_init:  メモリ管理機能（システム依存部分）を初期化する。
 *
 * 【引　数】    main_pool_ret: 基本メモリプールへのポインタを格納する変数
 * 　　　　　main_poolsize_ret: 基本メモリプールのサイズを格納する変数
 * 　　　　　    temp_pool_ret: 基本メモリプールへのポインタを格納する変数
 * 　　　　　temp_poolsize_ret: 基本メモリプールのサイズを格納する変数
 * 【戻り値】0以外＝成功、0＝失敗
 */
int sys_mem_init(void **main_pool_ret, uint32_t *main_poolsize_ret,
                 void **temp_pool_ret, uint32_t *temp_poolsize_ret)
{
    if (!main_pool_ret || !main_poolsize_ret
     || !temp_pool_ret || !temp_poolsize_ret
    ) {
        DMSG("Invalid parameters: %p %p %p %p",
             main_pool_ret, main_poolsize_ret,
             temp_pool_ret, temp_poolsize_ret);
        return 0;
    }
    *main_pool_ret     = main_pool;
    *main_poolsize_ret = main_poolsize;
    *temp_pool_ret     = temp_pool;
    *temp_poolsize_ret = temp_poolsize;
    return 1;
}

/*************************************************************************/

/**
 * sys_mem_fill8:  メモリ領域を指定された8ビット値でフィルする。アライメント
 * による最適化は考慮しなくて良い（sys_mem_fill32()が使えない場合のみ呼び出さ
 * れる）。
 *
 * 【引　数】ptr: フィルするメモリ領域へのポインタ
 * 　　　　　val: フィル値
 * 　　　　　len: フィルするバイト数
 * 【戻り値】なし
 */
void sys_mem_fill8(void *ptr, uint8_t val, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++) {
        ((uint8_t *)ptr)[i] = val;
    }
}

/*************************************************************************/

/**
 * sys_mem_fill32:  メモリ領域を指定された32ビット値でフィルする。
 *
 * 【引　数】ptr: フィルするメモリ領域へのポインタ（必ずアラインされている）
 * 　　　　　val: フィル値
 * 　　　　　len: フィルするバイト数（必ず4以上、4の倍数）
 * 【戻り値】なし
 */
void sys_mem_fill32(void *ptr, uint32_t val, uint32_t len)
{
    asm(".set push; .set noreorder\n\
        andi $t1, %[len], 0x1F  \n\
        beqz $t1, 1f            \n\
        addu $t0, %[ptr], %[len]\n\
     0: addiu $t0, $t0, -4      \n\
        addiu $t1, $t1, -4      \n\
        bnez $t1, 0b            \n\
        sw %[val], 0($t0)       \n\
        beq $t0, %[ptr], 9f     \n\
        nop                     \n\
     1: sw %[val], -4($t0)      \n\
        sw %[val], -8($t0)      \n\
        sw %[val], -12($t0)     \n\
        sw %[val], -16($t0)     \n\
        sw %[val], -20($t0)     \n\
        sw %[val], -24($t0)     \n\
        sw %[val], -28($t0)     \n\
        addiu $t0, $t0, -32     \n\
        bne $t0, %[ptr], 1b     \n\
        sw %[val], 0($t0)       \n\
     9: .set pop"
        : /* no outputs */
        : [ptr] "r" (ptr), [val] "r" (val), [len] "r" (len)
        : "t0", "t1", "memory"
    );
}

/*************************************************************************/
/************************ PSPライブラリ内部用関数 ************************/
/*************************************************************************/

/**
 * psp_mem_alloc_pools:  メモリ管理に使うプールをシステムから確保する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗。一時プールを確保できなくても成功とする
 */
int psp_mem_alloc_pools(void)
{
    main_poolsize = MAIN_POOLSIZE;
    SceUID block = sceKernelAllocPartitionMemory(
        PSP_MEMORY_PARTITION_USER, "MainPool", PSP_SMEM_Low,
        main_poolsize, NULL
    );
    if (block <= 0) {
        DMSG("Not enough memory! (want=%08X total_free=%08X max_free=%08X)",
             main_poolsize, sceKernelTotalFreeMemSize(),
             sceKernelMaxFreeMemSize());
        return 0;
    }
    main_pool = sceKernelGetBlockHeadAddr(block);
    mem_clear(main_pool, main_poolsize);

    block = sceKernelAllocPartitionMemory(
        PSP_MEMORY_PARTITION_USER, "TempPool", PSP_SMEM_Low,
        TEMP_POOLSIZE, NULL
    );
    if (block > 0) {
        temp_pool     = sceKernelGetBlockHeadAddr(block);
        temp_poolsize = TEMP_POOLSIZE;
    } else {
        DMSG("WARNING: not enough memory for temp (want=%08X total_free=%08X"
             " max_free=%08X)", TEMP_POOLSIZE, sceKernelTotalFreeMemSize(),
             sceKernelMaxFreeMemSize());
        temp_poolsize = sceKernelMaxFreeMemSize();
        if (temp_poolsize > 0) {
            block = sceKernelAllocPartitionMemory(
                PSP_MEMORY_PARTITION_USER, "TempPool", PSP_SMEM_Low,
                temp_poolsize, NULL
            );
            if (block > 0) {
                temp_pool = sceKernelGetBlockHeadAddr(block);
            } else {
                DMSG("sceKernelMaxFreeMemSize() lied!!");
                temp_pool = NULL;
                temp_poolsize = 0;
            }
        }
    }
    if (temp_pool) {
        mem_clear(temp_pool, temp_poolsize);
    }

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
