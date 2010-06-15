/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/vector.c: Vector/matrix manipulation routines.
 */

/*
 * このファイルでは、vector.hで定義するのに複雑すぎる（インライン関数に適さ
 * ない）関数を定義する。
 */

#include "common.h"
#include "vector.h"

/*************************************************************************/
/***************************** 行列計算関数 ******************************/
/*************************************************************************/

#ifndef mat4_mul

/**
 * mat4_mul:  4x4行列同士の乗算を行う。
 *
 * 【引　数】      dest: src1*src2を格納する行列へのポインタ
 * 　　　　　src1, src2: 乗算する行列へのポインタ
 * 【戻り値】なし
 */
void mat4_mul(Matrix4f *dest, const Matrix4f *src1, const Matrix4f *src2)
{
    const float dest_11 = DOT4(src1->_11, src1->_12, src1->_13, src1->_14,
                               src2->_11, src2->_21, src2->_31, src2->_41);
    const float dest_12 = DOT4(src1->_11, src1->_12, src1->_13, src1->_14,
                               src2->_12, src2->_22, src2->_32, src2->_42);
    const float dest_13 = DOT4(src1->_11, src1->_12, src1->_13, src1->_14,
                               src2->_13, src2->_23, src2->_33, src2->_43);
    const float dest_14 = DOT4(src1->_11, src1->_12, src1->_13, src1->_14,
                               src2->_14, src2->_24, src2->_34, src2->_44);

    const float dest_21 = DOT4(src1->_21, src1->_22, src1->_23, src1->_24,
                               src2->_11, src2->_21, src2->_31, src2->_41);
    const float dest_22 = DOT4(src1->_21, src1->_22, src1->_23, src1->_24,
                               src2->_12, src2->_22, src2->_32, src2->_42);
    const float dest_23 = DOT4(src1->_21, src1->_22, src1->_23, src1->_24,
                               src2->_13, src2->_23, src2->_33, src2->_43);
    const float dest_24 = DOT4(src1->_21, src1->_22, src1->_23, src1->_24,
                               src2->_14, src2->_24, src2->_34, src2->_44);

    const float dest_31 = DOT4(src1->_31, src1->_32, src1->_33, src1->_34,
                               src2->_11, src2->_21, src2->_31, src2->_41);
    const float dest_32 = DOT4(src1->_31, src1->_32, src1->_33, src1->_34,
                               src2->_12, src2->_22, src2->_32, src2->_42);
    const float dest_33 = DOT4(src1->_31, src1->_32, src1->_33, src1->_34,
                               src2->_13, src2->_23, src2->_33, src2->_43);
    const float dest_34 = DOT4(src1->_31, src1->_32, src1->_33, src1->_34,
                               src2->_14, src2->_24, src2->_34, src2->_44);

    const float dest_41 = DOT4(src1->_41, src1->_42, src1->_43, src1->_44,
                               src2->_11, src2->_21, src2->_31, src2->_41);
    const float dest_42 = DOT4(src1->_41, src1->_42, src1->_43, src1->_44,
                               src2->_12, src2->_22, src2->_32, src2->_42);
    const float dest_43 = DOT4(src1->_41, src1->_42, src1->_43, src1->_44,
                               src2->_13, src2->_23, src2->_33, src2->_43);
    const float dest_44 = DOT4(src1->_41, src1->_42, src1->_43, src1->_44,
                               src2->_14, src2->_24, src2->_34, src2->_44);

    dest->_11 = dest_11;
    dest->_12 = dest_12;
    dest->_13 = dest_13;
    dest->_14 = dest_14;
    dest->_21 = dest_21;
    dest->_22 = dest_22;
    dest->_23 = dest_23;
    dest->_24 = dest_24;
    dest->_31 = dest_31;
    dest->_32 = dest_32;
    dest->_33 = dest_33;
    dest->_34 = dest_34;
    dest->_41 = dest_41;
    dest->_42 = dest_42;
    dest->_43 = dest_43;
    dest->_44 = dest_44;
}

#endif  // mat4_mul

/*************************************************************************/

#ifndef mat4_det

/**
 * mat4_det:  4x4行列の行列式を返す。
 *
 * 【引　数】M: 行列式を計算する行列へのポインタ
 * 【戻り値】|M|
 */
float mat4_det(const Matrix4f *M)
{
    Vector4f minor;
    vec4_cross(&minor,
               &((Vector4f){{M->_11, M->_21, M->_31, M->_41}}),
               &((Vector4f){{M->_12, M->_22, M->_32, M->_42}}),
               &((Vector4f){{M->_13, M->_23, M->_33, M->_43}}));
    return -DOT4(minor.x, minor.y, minor.z, minor.w,
                 M->_14, M->_24, M->_34, M->_44);
}

#endif  // mat4_det

/*************************************************************************/

#ifndef mat4_inv

/**
 * mat4_inv:  4x4行列の逆行列を計算する。
 *
 * 【引　数】dest: src^-1を格納する行列へのポインタ
 * 　　　　　 src: 逆行列を計算する行列へのポインタ
 * 【戻り値】srcの行列式。0の場合、逆行列が存在しない（よってdestは変更されず）
 */
float mat4_inv(Matrix4f *dest, const Matrix4f *src)
{
    const float det = mat4_det(src);
    if (!det) {
        return 0;
    }

    Vector4f cols[4];
    vec4_cross(&cols[0], &src->v[1], &src->v[2], &src->v[3]);
    vec4_cross(&cols[1], &src->v[0], &src->v[3], &src->v[2]);
    vec4_cross(&cols[2], &src->v[0], &src->v[1], &src->v[3]);
    vec4_cross(&cols[3], &src->v[0], &src->v[2], &src->v[1]);

    dest->_11 = cols[0].x / det;
    dest->_21 = cols[0].y / det;
    dest->_31 = cols[0].z / det;
    dest->_41 = cols[0].w / det;
    dest->_12 = cols[1].x / det;
    dest->_22 = cols[1].y / det;
    dest->_32 = cols[1].z / det;
    dest->_42 = cols[1].w / det;
    dest->_13 = cols[2].x / det;
    dest->_23 = cols[2].y / det;
    dest->_33 = cols[2].z / det;
    dest->_43 = cols[2].w / det;
    dest->_14 = cols[3].x / det;
    dest->_24 = cols[3].y / det;
    dest->_34 = cols[3].z / det;
    dest->_44 = cols[3].w / det;

    return det;
}

#endif  // mat4_inv

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
