/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/vector.h: Header for vector/matrix manipulation routines.
 */

/*
 * このファイルで定義・宣言される関数は、各システム依存ヘッダでマクロとして
 * 定義することでオーバーライドできる。ただし（#if/#endifの増殖を抑えるため）
 * 以下の関数はグループとして定義しなければならない。
 * 　・vec[234]_{add,sub}
 * 　・vec[234]_scale
 * 　・vec[234]_dot
 * 　・vec[234]_length2
 * 　・vec[234]_length
 * 　・vec[234]_normalize
 * 　・mat4_{add,sub}
 */

#ifndef VECTOR_H
#define VECTOR_H

/*************************************************************************/
/*************************** ベクトル計算関数 ****************************/
/*************************************************************************/

#if !defined(vec2_add) && !defined(vec3_add) && !defined(vec4_add) \
 && !defined(vec2_sub) && !defined(vec3_sub) && !defined(vec4_sub)

/**
 * vec2_add, vec3_add, vec4_add, vec2_sub, vec3_sub, vec4_sub:  2・3・4要素
 * のベクトルに別のベクトルを加算、もしくは減算する。
 *
 * 【引　数】      dest: src1+src2またはsrc1-src2を格納するベクトルへのポインタ
 * 　　　　　src1, src2: 加（減）算するベクトルへのポインタ
 * 【戻り値】なし
 */
static inline void vec2_add(Vector2f *dest,
                            const Vector2f *src1, const Vector2f *src2)
{
    dest->x = src1->x + src2->x;
    dest->y = src1->y + src2->y;
}

static inline void vec3_add(Vector3f *dest,
                            const Vector3f *src1, const Vector3f *src2)
{
    dest->x = src1->x + src2->x;
    dest->y = src1->y + src2->y;
    dest->z = src1->z + src2->z;
}

static inline void vec4_add(Vector4f *dest,
                            const Vector4f *src1, const Vector4f *src2)
{
    dest->x = src1->x + src2->x;
    dest->y = src1->y + src2->y;
    dest->z = src1->z + src2->z;
    dest->w = src1->w + src2->w;
}

static inline void vec2_sub(Vector2f *dest,
                            const Vector2f *src1, const Vector2f *src2)
{
    dest->x = src1->x - src2->x;
    dest->y = src1->y - src2->y;
}

static inline void vec3_sub(Vector3f *dest,
                            const Vector3f *src1, const Vector3f *src2)
{
    dest->x = src1->x - src2->x;
    dest->y = src1->y - src2->y;
    dest->z = src1->z - src2->z;
}

static inline void vec4_sub(Vector4f *dest,
                            const Vector4f *src1, const Vector4f *src2)
{
    dest->x = src1->x - src2->x;
    dest->y = src1->y - src2->y;
    dest->z = src1->z - src2->z;
    dest->w = src1->w - src2->w;
}

#endif

/*************************************************************************/

#if !defined(vec2_scale) && !defined(vec3_scale) && !defined(vec4_scale)

/**
 * vec2_scale, vec3_scale, vec4_scale:  2・3・4要素のベクトルに定数を乗算する。
 *
 * 【引　数】dest: src*kを格納するベクトルへのポインタ
 * 　　　　　 src: 乗算されるベクトルへのポインタ
 * 　　　　　   k: 乗算する定数
 * 【戻り値】なし
 */
static inline void vec2_scale(Vector2f *dest,
                              const Vector2f *src, const float k)
{
    dest->x = src->x * k;
    dest->y = src->y * k;
}
static inline void vec3_scale(Vector3f *dest,
                              const Vector3f *src, const float k)
{
    dest->x = src->x * k;
    dest->y = src->y * k;
    dest->z = src->z * k;
}
static inline void vec4_scale(Vector4f *dest,
                              const Vector4f *src, const float k)
{
    dest->x = src->x * k;
    dest->y = src->y * k;
    dest->z = src->z * k;
    dest->w = src->w * k;
}

#endif

/*************************************************************************/

#if !defined(vec2_dot) && !defined(vec3_dot) && !defined(vec4_dot)

/**
 * vec2_dot, vec3_dot, vec4_dot:  2・3・4要素のベクトルの内積を求める。
 *
 * 【引　数】a, b: 内積を求めるベクトルへのポインタ
 * 【戻り値】a・b
 */
static PURE_FUNCTION inline float vec2_dot(const Vector2f *a, const Vector2f *b)
{
    return a->x*b->x + a->y*b->y;
}

static PURE_FUNCTION inline float vec3_dot(const Vector3f *a, const Vector3f *b)
{
    return a->x*b->x + a->y*b->y + a->z*b->z;
}

static PURE_FUNCTION inline float vec4_dot(const Vector4f *a, const Vector4f *b)
{
    return a->x*b->x + a->y*b->y + a->z*b->z + a->w*b->w;
}

#endif

/*************************************************************************/

#if !defined(vec2_length2) && !defined(vec3_length2) && !defined(vec4_length2)

/**
 * vec2_length2, vec3_length2, vec4_length2:  2・3・4要素のベクトルの長さの
 * 自乗を求める。
 *
 * 【引　数】v: 長さの自乗を求めるベクトルへのポインタ
 * 【戻り値】||v||^2
 */
static PURE_FUNCTION inline float vec2_length2(const Vector2f *v)
{
    return vec2_dot(v, v);
}

static PURE_FUNCTION inline float vec3_length2(const Vector3f *v)
{
    return vec3_dot(v, v);
}

static PURE_FUNCTION inline float vec4_length2(const Vector4f *v)
{
    return vec4_dot(v, v);
}

#endif

/*************************************************************************/

#if !defined(vec2_length) && !defined(vec3_length) && !defined(vec4_length)

/**
 * vec2_length, vec3_length, vec4_length:  2・3・4要素のベクトルの長さを
 * 求める。
 *
 * 【引　数】v: 長さを求めるベクトルへのポインタ
 * 【戻り値】||v||
 */
static PURE_FUNCTION inline float vec2_length(const Vector2f *v)
{
    return sqrtf(vec2_length2(v));
}

static PURE_FUNCTION inline float vec3_length(const Vector3f *v)
{
    return sqrtf(vec3_length2(v));
}

static PURE_FUNCTION inline float vec4_length(const Vector4f *v)
{
    return sqrtf(vec4_length2(v));
}

#endif

/*************************************************************************/

#if !defined(vec2_normalize) \
 && !defined(vec3_normalize) \
 && !defined(vec4_normalize)

/**
 * vec2_normalize, vec3_normalize, vec4_normalize:  2・3・4要素のベクトルを
 * 正規化する。
 *
 * 【引　数】dest: 正規化結果を格納するベクトルへのポインタ
 * 　　　　　 src: 正規化するベクトルへのポインタ
 * 【戻り値】なし
 */

static inline void vec2_normalize(Vector2f *dest, const Vector2f *src)
{
    const float length = vec2_length(src);
    if (length > 0) {
        vec2_scale(dest, src, 1/length);
    } else {
        *dest = *src;
    }
}

static inline void vec3_normalize(Vector3f *dest, const Vector3f *src)
{
    const float length = vec3_length(src);
    if (length > 0) {
        vec3_scale(dest, src, 1/length);
    } else {
        *dest = *src;
    }
}

static inline void vec4_normalize(Vector4f *dest, const Vector4f *src)
{
    const float length = vec4_length(src);
    if (length > 0) {
        vec4_scale(dest, src, 1/length);
    } else {
        *dest = *src;
    }
}

#endif

/*************************************************************************/

#ifndef vec3_cross

/**
 * vec3_cross:  3要素ベクトルの外積を求める。
 *
 * 【引　数】      dest: 外積を格納するベクトルへのポインタ
 * 　　　　　src1, src2: 外積を求めるベクトルへのポインタ
 * 【戻り値】なし
 */
static inline void vec3_cross(Vector3f *dest,
                              const Vector3f *src1, const Vector3f *src2)
{
    Vector3f temp;
    temp.x = src1->y * src2->z - src1->z * src2->y;
    temp.y = src1->z * src2->x - src1->x * src2->z;
    temp.z = src1->x * src2->y - src1->y * src2->x;
    *dest = temp;
}

#endif

/*----------------------------------*/

#ifndef vec4_cross

/**
 * vec4_cross:  4要素ベクトルの外積を求める。
 *
 * 【引　数】            dest: 外積を格納するベクトルへのポインタ
 * 　　　　　src1, src2, src3: 外積を求めるベクトルへのポインタ
 * 【戻り値】なし
 */
static inline void vec4_cross(Vector4f *dest, const Vector4f *src1,
                              const Vector4f *src2, const Vector4f *src3)
{
    Vector4f temp;
    temp.x =   src1->y * (src2->z * src3->w - src2->w * src3->z)
             + src1->z * (src2->w * src3->y - src2->y * src3->w)
             + src1->w * (src2->y * src3->z - src2->z * src3->y);
    temp.y = -(src1->x * (src2->z * src3->w - src2->w * src3->z)
             + src1->z * (src2->w * src3->x - src2->x * src3->w)
             + src1->w * (src2->x * src3->z - src2->z * src3->x));
    temp.z =   src1->x * (src2->y * src3->w - src2->w * src3->y)
             + src1->y * (src2->w * src3->x - src2->x * src3->w)
             + src1->w * (src2->x * src3->y - src2->y * src3->x);
    temp.w = -(src1->x * (src2->y * src3->z - src2->z * src3->y)
             + src1->y * (src2->z * src3->x - src2->x * src3->z)
             + src1->z * (src2->x * src3->y - src2->y * src3->x));
    *dest = temp;
}

#endif

/*************************************************************************/

#ifndef vec3_transform

/**
 * vec3_transform:  指定されたベクトルを座標変換行列で変換する。
 *
 * 【引　数】  res: 変換結果を格納するベクトル変数へのポインタ
 * 　　　　　coord: 座標ベクトルへのポインタ
 * 　　　　　    m: 座標変換行列へのポインタ
 * 【戻り値】なし
 */
static inline void vec3_transform(Vector3f *res,
                                  const Vector3f *coord, const Matrix4f *m)
{
    res->x = DOT4(coord->x,coord->y,coord->z,1, m->_11,m->_21,m->_31,m->_41);
    res->y = DOT4(coord->x,coord->y,coord->z,1, m->_12,m->_22,m->_32,m->_42);
    res->z = DOT4(coord->x,coord->y,coord->z,1, m->_13,m->_23,m->_33,m->_43);
}

#endif

/*************************************************************************/
/***************************** 行列計算関数 ******************************/
/*************************************************************************/

#if !defined(mat4_add) && !defined(mat4_sub)

/**
 * mat4_add, mat4_sub:  4x4行列同士の加算・減算を行う。
 *
 * 【引　数】      dest: src1+src2（src1-src2）を格納する行列へのポインタ
 * 　　　　　src1, src2: 加算（減算）する行列へのポインタ
 * 【戻り値】なし
 */
static inline void mat4_add(Matrix4f *dest,
                            const Matrix4f *src1, const Matrix4f *src2)
{
    vec4_add(&dest->v[0], &src1->v[0], &src2->v[0]);
    vec4_add(&dest->v[1], &src1->v[1], &src2->v[1]);
    vec4_add(&dest->v[2], &src1->v[2], &src2->v[2]);
    vec4_add(&dest->v[3], &src1->v[3], &src2->v[3]);
}

static inline void mat4_sub(Matrix4f *dest,
                            const Matrix4f *src1, const Matrix4f *src2)
{
    vec4_sub(&dest->v[0], &src1->v[0], &src2->v[0]);
    vec4_sub(&dest->v[1], &src1->v[1], &src2->v[1]);
    vec4_sub(&dest->v[2], &src1->v[2], &src2->v[2]);
    vec4_sub(&dest->v[3], &src1->v[3], &src2->v[3]);
}

#endif

/*************************************************************************/

#ifndef mat4_mul

/**
 * mat4_mul:  4x4行列同士の乗算を行う。
 *
 * 【引　数】      dest: src1*src2を格納する行列へのポインタ
 * 　　　　　src1, src2: 乗算する行列へのポインタ
 * 【戻り値】なし
 */
extern void mat4_mul(Matrix4f *dest,
                     const Matrix4f *src1, const Matrix4f *src2);

#endif

/*************************************************************************/

#ifndef mat4_det

/**
 * mat4_det:  4x4行列の行列式を返す。
 *
 * 【引　数】M: 行列式を計算する行列
 * 【戻り値】|M|
 */
extern PURE_FUNCTION float mat4_det(const Matrix4f *M);

#endif

/*************************************************************************/

#ifndef mat4_inv

/**
 * mat4_inv:  4x4行列の逆行列を計算する。
 *
 * 【引　数】dest: src^-1を格納する行列へのポインタ
 * 　　　　　 src: 逆行列を計算する行列へのポインタ
 * 【戻り値】srcの行列式。0の場合、逆行列が存在しない（よってdestは変更されず）
 */
extern float mat4_inv(Matrix4f *dest, const Matrix4f *src);

#endif

/*************************************************************************/

#ifndef mat4_transpose

/**
 * mat4_transpose:  4x4行列を転置する。
 *
 * 【引　数】dest: 転置した行列を格納する行列へのポインタ
 * 　　　　　 src: 転置する行列へのポインタ
 * 【戻り値】なし
 */
static inline void mat4_transpose(Matrix4f *dest, const Matrix4f *src)
{
    const float saved_12 = src->_12;
    const float saved_13 = src->_13;
    const float saved_14 = src->_14;
    const float saved_23 = src->_23;
    const float saved_24 = src->_24;
    const float saved_34 = src->_34;

    dest->_11 = src->_11;
    dest->_12 = src->_21;
    dest->_13 = src->_31;
    dest->_14 = src->_41;
    dest->_21 = saved_12;
    dest->_22 = src->_22;
    dest->_23 = src->_32;
    dest->_24 = src->_42;
    dest->_31 = saved_13;
    dest->_32 = saved_23;
    dest->_33 = src->_33;
    dest->_34 = src->_43;
    dest->_41 = saved_14;
    dest->_42 = saved_24;
    dest->_43 = saved_34;
    dest->_44 = src->_44;
}

#endif

/*************************************************************************/
/*************************************************************************/

#endif  // VECTOR_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
