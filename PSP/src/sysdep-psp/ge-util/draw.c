/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/draw.c: Drawing functions for the GE utility
 * library.
 */

#include "../../common.h"
#include "../ge-util.h"
#include "../psplocal.h"

#include "ge-const.h"
#include "ge-local.h"

/*************************************************************************/
/*************************************************************************/

/**
 * ge_set_draw_buffer:  描画バッファを設定する。ptr==NULLの場合、フレーム
 * バッファを使う。
 *
 * 【引　数】buffer: 描画バッファへのポインタ（アドレスの下位6ビット切り捨て）
 * 　　　　　stride: 描画バッファのライン長（ピクセル）
 * 【戻り値】なし
 */
void ge_set_draw_buffer(void *buffer, int stride)
{
    if (!buffer) {
        buffer = psp_draw_buffer();
        stride = DISPLAY_STRIDE;
    }
    CHECK_GELIST(2);
    internal_add_command(GECMD_DRAW_ADDRESS, ((uint32_t)buffer & 0x00FFFFC0));
    internal_add_command(GECMD_DRAW_STRIDE,  ((uint32_t)buffer & 0xFF000000)>>8
                                             | stride);
}

/*************************************************************************/

/**
 * ge_set_vertex_format:  頂点データ形式を設定する。
 *
 * 【引　数】format: 頂点データ形式（GE_VERTEXFMT_*）
 * 【戻り値】なし
 */
void ge_set_vertex_format(uint32_t format)
{
    CHECK_GELIST(1);
    internal_add_command(GECMD_VERTEX_FORMAT, format & 0xFFFFFF);
}

/*************************************************************************/

/**
 * ge_set_vertex_pointer:  プリミティブ描画用頂点ポインタを設定する。
 * ptr==NULLの場合、内部ポインタを使う。
 *
 * 【引　数】ptr: 頂点データへのポインタ
 * 【戻り値】なし
 */
void ge_set_vertex_pointer(const void *ptr)
{
    if (!ptr) {
        ptr = vertlist_ptr;
    }
    CHECK_GELIST(2);
    internal_add_command(GECMD_ADDRESS_BASE, ((uint32_t)ptr & 0xFF000000)>>8);
    internal_add_command(GECMD_VERTEX_POINTER, (uint32_t)ptr & 0x00FFFFFF);
}

/*************************************************************************/

/**
 * ge_draw_primitive:  プリミティブを描画する。事前に頂点データ形式と頂点
 * ポインタを設定しなければならない。ただし連続で呼び出した場合、頂点データ
 * も連続していれば、再設定する必要はない。
 *
 * 【引　数】   primitive: プリミティブ（GE_PRIMITIVE_*）
 * 　　　　　num_vertices: 頂点数
 * 【戻り値】なし
 */
void ge_draw_primitive(GEPrimitive primitive, uint16_t num_vertices)
{
    CHECK_GELIST(1);
    internal_add_command(GECMD_DRAW_PRIMITIVE, primitive<<16 | num_vertices);
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
