/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/vertex.c: Vertex manipulation routines for the
 * GE utility library.
 */

#include "../../common.h"
#include "../ge-util.h"
#include "../psplocal.h"

#include "ge-const.h"
#include "ge-local.h"

/*************************************************************************/
/*************************************************************************/

/**
 * ge_add_color_xy_vertex:  色とXY座標を指定した頂点を登録する。
 *
 * 【引　数】color: 色 (0xAABBGGRR)
 * 　　　　　 x, y: 座標
 * 【戻り値】なし
 */
void ge_add_color_xy_vertex(uint32_t color, int16_t x, int16_t y)
{
    CHECK_VERTLIST(3);
    *vertlist_ptr++ = color;
    *vertlist_ptr++ = INT16_PAIR(x, y);
    *vertlist_ptr++ = INT16_PAIR(0 /*z*/, 0 /*pad*/);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_add_color_xy_vertexf:  色と浮動小数点のXY座標を指定した頂点を登録する。
 *
 * 【引　数】color: 色 (0xAABBGGRR)
 * 　　　　　 x, y: 座標
 * 【戻り値】なし
 */
void ge_add_color_xy_vertexf(uint32_t color, float x, float y)
{
    CHECK_VERTLIST(4);
    *vertlist_ptr++ = color;
    *vertlist_ptr++ = FLOAT(x);
    *vertlist_ptr++ = FLOAT(y);
    *vertlist_ptr++ = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * ge_add_color_xyz_vertexf:  色と浮動小数点のXYZ座標を指定した頂点を登録する。
 *
 * 【引　数】  color: 色 (0xAABBGGRR)
 * 　　　　　x, y, z: 座標
 * 【戻り値】なし
 */
void ge_add_color_xyz_vertexf(uint32_t color, float x, float y, float z)
{
    CHECK_VERTLIST(4);
    *vertlist_ptr++ = color;
    *vertlist_ptr++ = FLOAT(x);
    *vertlist_ptr++ = FLOAT(y);
    *vertlist_ptr++ = FLOAT(z);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_add_uv_xy_vertex:  UV座標とXY座標を指定した頂点を登録する。アライメント
 * の関係で、1つの処理について必ず偶数回呼び出さなければならない。（奇数回に
 * なるような場合はge_add_uv_xy_vertexf()を利用すること）
 *
 * 【引　数】u, v: テキスチャ座標
 * 　　　　　x, y: 座標
 * 【戻り値】なし
 */
void ge_add_uv_xy_vertex(int16_t u, int16_t v, int16_t x, int16_t y)
{
    CHECK_VERTLIST(3);
    static int which = 0;
    if (which == 0) {  // 1個目
        *vertlist_ptr++ = INT16_PAIR(u, v);
        *vertlist_ptr++ = INT16_PAIR(x, y);
    } else {  // 2個目
        *vertlist_ptr++ = INT16_PAIR(0 /*z*/, u);
        *vertlist_ptr++ = INT16_PAIR(v, x);
        *vertlist_ptr++ = INT16_PAIR(y, 0 /*z*/);
    }
    which = ~which;
}

/*-----------------------------------------------------------------------*/

/**
 * ge_add_uv_xyz_vertexf:  浮動小数点のUV座標とXYZ座標を指定した頂点を登録
 * する。
 *
 * 【引　数】    u, v: テキスチャ座標
 * 　　　　　 x, y, z: 座標
 * 【戻り値】なし
 */
void ge_add_uv_xyz_vertexf(float u, float v, float x, float y, float z)
{
    CHECK_VERTLIST(5);
    *vertlist_ptr++ = FLOAT(u);
    *vertlist_ptr++ = FLOAT(v);
    *vertlist_ptr++ = FLOAT(x);
    *vertlist_ptr++ = FLOAT(y);
    *vertlist_ptr++ = FLOAT(z);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_add_uv_color_xy_vertex:  UV座標、色、XY座標を指定した頂点を登録する。
 *
 * 【引　数】 u, v: テキスチャ座標
 * 　　　　　color: 色 (0xAABBGGRR)
 * 　　　　　 x, y: 座標
 * 【戻り値】なし
 */
void ge_add_uv_color_xy_vertex(int16_t u, int16_t v, uint32_t color,
                               int16_t x, int16_t y)
{
    CHECK_VERTLIST(4);
    *vertlist_ptr++ = INT16_PAIR(u, v);
    *vertlist_ptr++ = color;
    *vertlist_ptr++ = INT16_PAIR(x, y);
    *vertlist_ptr++ = INT16_PAIR(0 /*z*/, 0 /*pad*/);
}

/*************************************************************************/

/**
 * ge_reserve_vertexbytes:  指定されたデータ量を頂点バッファから確保し、
 * メモリ領域へのポインタを返す。
 *
 * 【引　数】size: 確保する領域のサイズ（バイト）
 * 【戻り値】頂点メモリ領域へのポインタ（NULL＝確保できなかった）
 */
void *ge_reserve_vertexbytes(int size)
{
    if (UNLIKELY(size <= 0)) {
        DMSG("Invalid size %d", size);
        return NULL;
    }

    const int nwords = (size+3) / 4;
    if (UNLIKELY(vertlist_ptr + nwords > vertlist_limit)) {
        DMSG("No memory for %d vertex bytes", size);
        return NULL;
    }
    void *retval = vertlist_ptr;
    vertlist_ptr += nwords;
    return retval;
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
