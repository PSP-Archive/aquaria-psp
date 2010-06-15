/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/ge-local.h: Header declaring internal variables
 * and functions for the GE utility library.
 */

#ifndef GE_LOCAL_H
#define GE_LOCAL_H

/*************************************************************************/
/*************************************************************************/

/* PSPシステムヘッダをインクルード */

#include <pspuser.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>

/*************************************************************************/

/* ライブラリ共通データ宣言 */

/************************************/

/* 次に命令を格納するポインタ */
extern uint32_t *gelist_ptr;

/* 現在のリストの限界アドレス（list + lenof(list)） */
extern uint32_t *gelist_limit;

/* サブリスト作成時、直前のリストポインタ（メインリスト作成中はともにNULL） */
extern uint32_t *saved_gelist_ptr, *saved_gelist_limit;

/************************************/

/* 次に頂点を格納するポインタと限界アドレス */
extern uint32_t *vertlist_ptr;
extern uint32_t *vertlist_limit;

/* 2つの16ビット値を一つの32ビット値にまとめるマクロ */
#define INT16_PAIR(first,second)  (((first) & 0xFFFF) | (second)<<16)

/* float型変数を32ビット整数としてアクセスするマクロ */
#define FLOAT(val)  __extension__({              \
    uint32_t i;                                  \
    asm("mfc1 %0, %1" : "=r" (i) : "f" ((val))); \
    i;                                           \
})

/************************************/

/* 画面のビット／ピクセル */
extern int display_bpp;

/*************************************************************************/

/* ライブラリ共通ユーティリティマクロ・関数 */

/************************************/

/**
 * CHECK_GELIST, CHECK_VERTLIST:  GE命令リストまたは頂点リストの空き容量を
 * 確認し、空き容量が足りない場合、関数から戻る。
 *
 * 【引　数】required: 必要容量（32ビット単位）
 */
#define CHECK_GELIST(required) do {                             \
    if (UNLIKELY(gelist_ptr + (required) > gelist_limit)) {     \
        DMSG("Command list full!");                             \
        return;                                                 \
    }                                                           \
} while (0)

#define CHECK_VERTLIST(required) do {                           \
    if (UNLIKELY(vertlist_ptr + (required) > vertlist_limit)) { \
        DMSG("Vertex list full!");                              \
        return;                                                 \
    }                                                           \
} while (0)

/************************************/

/**
 * internal_add_command, internal_add_commandf:  GE命令をリストに登録する。
 * リスト満杯やパラメータの高位ビットなどはチェックしない。
 *
 * 【引　数】  command: 命令（8ビット）
 * 　　　　　parameter: パラメータ（24ビットまたは浮動小数点）
 * 【戻り値】なし
 */
static inline void internal_add_command(GECommand command, uint32_t parameter)
{
    *gelist_ptr++ = (uint32_t)command<<24 | parameter;
}

static inline void internal_add_commandf(GECommand command, float parameter)
{
    uint32_t bits;
    asm(".set push; .set noreorder\n"
        "mfc1 %[out], %[in]\n"
        "nop\n"
        "srl %[out], %[out], 8\n"
        ".set pop"
        : [out] "=r" (bits)
        : [in] "f" (parameter)
    );
    *gelist_ptr++ = (uint32_t)command<<24 | bits;
}

/*************************************************************************/
/*************************************************************************/

#endif  // GE_LOCAL_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
