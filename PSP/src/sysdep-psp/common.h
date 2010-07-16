/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/common.h: PSP-specific common header file.
 */

#ifndef SYSDEP_PSP_COMMON_H
#define SYSDEP_PSP_COMMON_H

/*************************************************************************/
/************************* PSP専用構成オプション *************************/
/*************************************************************************/

/**
 * USE_VFPU_VECTOR_MATRIX_FUNCS:  定義すると、一部のベクトル・行列演算関数
 * を、VFPUを利用した高速のものに置き換える。定義した方がプログラムの実行速
 * 度が上がるので通常は定義した方がよく、主にデバッグなどのために一括無効化
 * するために設けられている。
 * 　※Aquariaでは、OpenGLの座標変換処理でVFPUを使うので無効化。
 */
// #define USE_VFPU_VECTOR_MATRIX_FUNCS

/**
 * USE_TRIG_TABLES:  定義すると、dsinf()、dcosf()、dtanf()、dsincosf()の各
 * 関数を、テーブルを利用したものに切り替える。ただ部分的には速くなるものの、
 * 実際にプログラムで使うと（テーブルデータがキャッシュに留まらないためか）
 * かえって遅くなる模様。
 */
// #define USE_TRIG_TABLES

/*************************************************************************/
/**************** PSP用浮動小数点関数（高速アセンブリ版） ****************/
/*************************************************************************/

/* 通常の浮動小数点関数を定義。引数は%[x]、結果は%[result]に格納。
 * insnはアセンブリ命令の文字列。
 * 注：32ビット整数の有効範囲外の値に対しても使われる可能性があるので、
 * 　　範囲チェックが必要。絶対値が2^24以上であれば、既に整数になっている */
#define DEFINE_FUNC(name,insn) \
static inline CONST_FUNCTION float name(const float x) { \
    int32_t dummy;                        \
    float result;                         \
    asm(".set push; .set noreorder\n"     \
        "mfc1 %[dummy], %[x]\n"           \
        "nop\n"                           \
        "ext %[dummy], %[dummy], 23, 8\n" \
        "sltiu %[dummy], %[dummy], 0x98\n"\
        "beqzl %[dummy], 0f\n"            \
        "mov.s %[result], %[x]\n"         \
        insn                              \
        "\n0: .set pop"                   \
        : [result] "=f" (result), [dummy] "=r" (dummy) : [x] "f" (x)); \
    return result;                        \
}

/* 整数（int32_t型）を返す浮動小数点関数を定義。引数は%[x]、結果は%[result]
 * に格納。テンポラリ用の浮動小数点レジスタとして%[dummy]を使用できる。 */
#define DEFINE_IFUNC(name,insn)                                         \
static inline CONST_FUNCTION int32_t name(const float x) {              \
    float dummy;                                                        \
    int32_t result;                                                     \
    asm(insn                                                            \
         : [result] "=r" (result), [dummy] "=f" (dummy) : [x] "f" (x)); \
    return result;                                                      \
}

/*************************************/

#undef floorf
#define floorf psp_floorf
DEFINE_FUNC(floorf, "floor.w.s %[result],%[x]; cvt.s.w %[result],%[result]")

#undef truncf
#define truncf psp_truncf
DEFINE_FUNC(truncf, "trunc.w.s %[result],%[x]; cvt.s.w %[result],%[result]")

#undef ceilf
#define ceilf psp_ceilf
DEFINE_FUNC(ceilf,  "ceil.w.s  %[result],%[x]; cvt.s.w %[result],%[result]")

#undef roundf
#define roundf psp_roundf
DEFINE_FUNC(roundf, "round.w.s %[result],%[x]; cvt.s.w %[result],%[result]")

/************************************/

#undef ifloorf
#define ifloorf psp_ifloorf
DEFINE_IFUNC(ifloorf, "floor.w.s %[dummy],%[x]; mfc1 %[result],%[dummy]")

#undef itruncf
#define itruncf psp_itruncf
DEFINE_IFUNC(itruncf, "trunc.w.s %[dummy],%[x]; mfc1 %[result],%[dummy]")

#undef iceilf
#define iceilf psp_iceilf
DEFINE_IFUNC(iceilf,  "ceil.w.s  %[dummy],%[x]; mfc1 %[result],%[dummy]")

#undef iroundf
#define iroundf psp_iroundf
DEFINE_IFUNC(iroundf, "round.w.s %[dummy],%[x]; mfc1 %[result],%[dummy]")

/*************************************/

#undef fracf
static inline CONST_FUNCTION float fracf(const float x) {
    float result;
    asm("floor.w.s %[result],%[x];"
        "cvt.s.w %[result],%[result];"
        "sub.s %[result],%[x],%[result]"
        : [result] "=&f" (result) : [x] "f" (x)
    );
    return result;
}

/*************************************/

/* GCC 4.3ではisinff()が省略されてしまう（-ffast-mathのため？）ので、自前で
 * 処理する */
#undef isinff
#define isinff psp_isinff
static inline int psp_isinff(const float x)
{
    unsigned int bits;
    asm("mfc1 %[bits],%[x]; nop" : [bits] "=r" (bits) : [x] "f" (x));
    return bits==0x7F800000 ? 1 : bits==0xFF800000 ? -1 : 0;
}

/*************************************/

#undef DEFINE_FUNC
#undef DEFINE_IFUNC

/*************************************************************************/
/************ PSP用ベクトル・行列計算関数（高速アセンブリ版） ************/
/*************************************************************************/

#ifdef USE_VFPU_VECTOR_MATRIX_FUNCS

/*************************************/

/* 任意のVFPUレジスタから4次元外積を実行するアセンブリを生成するマクロ。
 * 「n」は行列番号（0、4〜7）。M100、M200、M300は破壊される。 */

#define VFPU_VEC4_CROSS(n) \
    /* temp.x =   src1->y * (src2->z * src3->w - src2->w * src3->z)     \
                + src1->z * (src2->w * src3->y - src2->y * src3->w)     \
                + src1->w * (src2->y * src3->z - src2->z * src3->y); */ \
    /* temp.w = -(src1->x * (src2->y * src3->z - src2->z * src3->y)     \
                + src1->y * (src2->z * src3->x - src2->x * src3->z)     \
                + src1->z * (src2->x * src3->y - src2->y * src3->x)); */\
    "vmov.t R100, R"#n"10\n"                                            \
    "vmov.t R103, R"#n"00[-x,-y,-z]\n"                                  \
    "vcrs.t R200, R"#n"11, R"#n"12\n"                                   \
    "vcrs.t R300, R"#n"12, R"#n"11\n"                                   \
    "vcrs.t R203, R"#n"01, R"#n"02\n"                                   \
    "vcrs.t R303, R"#n"02, R"#n"01\n"                                   \
    /* temp.z =   src1->x * (src2->y * src3->w - src2->w * src3->y)     \
                + src1->y * (src2->w * src3->x - src2->x * src3->w)     \
                + src1->w * (src2->x * src3->y - src2->y * src3->x); */ \
    "vmov.t R"#n"03, C"#n"20\n"         /* srcn.zを一時保管 */          \
    "vmov.t C"#n"20, C"#n"30\n"         /* R"#n"0n = srcn.xyw */        \
    "vmov.t R102, R"#n"00\n"                                            \
    "vcrs.t R202, R"#n"01, R"#n"02\n"                                   \
    "vcrs.t R302, R"#n"02, R"#n"01\n"                                   \
    /* temp.y = -(src1->x * (src2->z * src3->w - src2->w * src3->z)     \
                + src1->z * (src2->w * src3->x - src2->x * src3->w)     \
                + src1->w * (src2->x * src3->z - src2->z * src3->x)); */\
    "vmov.t C"#n"10, R"#n"03\n"         /* R"#n"0n = srcn.xzw */        \
    "vmov.t R101, R"#n"00[-x,-y,-z]\n"                                  \
    "vcrs.t R201, R"#n"01, R"#n"02\n"                                   \
    "vcrs.t R301, R"#n"02, R"#n"01\n"                                   \
    /* 延期された減算・内積を実行 */                                    \
    "vsub.t R200, R200, R300\n"                                         \
    "vsub.t R203, R203, R303\n"                                         \
    "vsub.t R202, R202, R302\n"                                         \
    "vsub.t R201, R201, R301\n"                                         \
    "vdot.t S"#n"03, R100, R200\n"                                      \
    "vdot.t S"#n"33, R103, R203\n"                                      \
    "vdot.t S"#n"23, R102, R202\n"                                      \
    "vdot.t S"#n"13, R101, R201\n"

/*************************************/

#define vec2_length psp_vec2_length
static inline float psp_vec2_length(const Vector2f *v) {
    float result;
    uint32_t temp;
    asm("lv.s S000, %[v_x]\n"
        "lv.s S001, %[v_y]\n"
        "vdot.p S020, C000, C000\n"
        "vsqrt.s S021, S020\n"
        "mfv %[temp], S021\n"
        "mtc1 %[temp], %[result]\n"
        : [result] "=f" (result), [temp] "=r" (temp)
        : [v_x] "m" (v->x), [v_y] "m" (v->y)
    );
    return result;
}

#define vec3_length psp_vec3_length
static inline float psp_vec3_length(const Vector3f *v) {
    float result;
    uint32_t temp;
    asm("lv.s S000, %[v_x]\n"
        "lv.s S001, %[v_y]\n"
        "lv.s S002, %[v_z]\n"
        "vdot.t S020, C000, C000\n"
        "vsqrt.s S021, S020\n"
        "mfv %[temp], S021\n"
        "mtc1 %[temp], %[result]\n"
        : [result] "=f" (result), [temp] "=r" (temp)
        : [v_x] "m" (v->x), [v_y] "m" (v->y), [v_z] "m" (v->z)
    );
    return result;
}

#define vec4_length psp_vec4_length
static inline float psp_vec4_length(const Vector4f *v) {
    float result;
    uint32_t temp;
    asm("lvl.q C000, %[v_w]\n"
        "lvr.q C000, %[v_x]\n"
        "vdot.q S020, C000, C000\n"
        "vsqrt.s S021, S020\n"
        "mfv %[temp], S021\n"
        "mtc1 %[temp], %[result]\n"
        : [result] "=f" (result), [temp] "=r" (temp)
        : [v_x] "m" (v->x), [v_y] "m" (v->y), [v_z] "m" (v->z),
          [v_w] "m" (v->w)
    );
    return result;
}

/*************************************/

#define vec2_normalize psp_vec2_normalize
static inline void psp_vec2_normalize(Vector2f *dest, const Vector2f *src)
{
    asm("lv.s S000, %[src_x]\n"
        "lv.s S001, %[src_y]\n"
        "vdot.p S020, C000, C000\n"
        "vzero.p C030\n"
        "vcmp.s EQ, S020, S030\n"
        "vrsq.s S021, S020\n"
        "vscl.p C010, C000, S021\n"
        "vcmovt.p C010, C030, 4\n"
        "sv.s S010, %[dest_x]\n"
        "sv.s S011, %[dest_y]\n"
        : [dest_x] "=m" (dest->x), [dest_y] "=m" (dest->y)
        : [src_x] "m" (src->x), [src_y] "m" (src->y)
    );
}

#define vec3_normalize psp_vec3_normalize
static inline void psp_vec3_normalize(Vector3f *dest, const Vector3f *src)
{
    asm("lv.s S000, %[src_x]\n"
        "lv.s S001, %[src_y]\n"
        "lv.s S002, %[src_z]\n"
        "vdot.t S020, C000, C000\n"
        "vzero.t C030\n"
        "vcmp.s EQ, S020, S030\n"
        "vrsq.s S021, S020\n"
        "vscl.t C010, C000, S021\n"
        "vcmovt.t C010, C030, 4\n"
        "sv.s S010, %[dest_x]\n"
        "sv.s S011, %[dest_y]\n"
        "sv.s S012, %[dest_z]\n"
        : [dest_x] "=m" (dest->x), [dest_y] "=m" (dest->y),
          [dest_z] "=m" (dest->z)
        : [src_x] "m" (src->x), [src_y] "m" (src->y), [src_z] "m" (src->z)
    );
}

#define vec4_normalize psp_vec4_normalize
static inline void psp_vec4_normalize(Vector4f *dest, const Vector4f *src)
{
    asm("lvl.q C000, %[src_w]\n"
        "lvr.q C000, %[src_x]\n"
        "vdot.q S020, C000, C000\n"
        "vzero.q C030\n"
        "vcmp.s EQ, S020, S030\n"
        "vrsq.s S021, S020\n"
        "vscl.q C010, C000, S021\n"
        "vcmovt.q C010, C030, 4\n"
        "svl.q C010, %[dest_w]\n"
        "svr.q C010, %[dest_x]\n"
        : [dest_x] "=m" (dest->x), [dest_y] "=m" (dest->y),
          [dest_z] "=m" (dest->z), [dest_w] "=m" (dest->w)
        : [src_x] "m" (src->x), [src_y] "m" (src->y), [src_z] "m" (src->z),
          [src_w] "m" (src->w)
    );
}

/*************************************/

#define vec4_cross psp_vec4_cross
static inline void psp_vec4_cross(Vector4f *dest, const Vector4f *src1,
                                  const Vector4f *src2, const Vector4f *src3)
{
    /* 使用レジスタ：
     *     R000.q: src1
     *     R001.q: src2
     *     R002.q: src3
     *     R003.q: dest; 一時バッファ
     *     R100.t: src1[y,z,w]
     *     R101.t: src1[-x,-z,-w]
     *     R102.t: src1[x,y,w]
     *     R103.t: src1[x,y,z]
     *     R200.t: {z2*w3, w2*y3, y2*z3}
     *     R201.t: {z2*w3, w2*x3, x2*z3}
     *     R202.t: {y2*w3, w2*x3, x2*y3}
     *     R203.t: {y2*z3, z2*x3, x2*y3}
     *     R300.t: {w2*z3, y2*w3, z2*y3}
     *     R301.t: {w2*z3, x2*w3, z2*x3}
     *     R302.t: {w2*y3, x2*w3, y2*x3}
     *     R303.t: {z2*y3, x2*z3, y2*x3}
     */
    asm("lvl.q R000, %[src1_w]\n"
        "lvr.q R000, %[src1_x]\n"
        "lvl.q R001, %[src2_w]\n"
        "lvr.q R001, %[src2_x]\n"
        "lvl.q R002, %[src3_w]\n"
        "lvr.q R002, %[src3_x]\n"
        VFPU_VEC4_CROSS(0)
        "svl.q R003, %[dest_w]\n"
        "svr.q R003, %[dest_x]\n"
        : [dest_x] "=m" (dest->x), [dest_y] "=m" (dest->y),
          [dest_z] "=m" (dest->z), [dest_w] "=m" (dest->w)
        : [src1_x] "m" (src1->x), [src1_y] "m" (src1->y),
          [src1_z] "m" (src1->z), [src1_w] "m" (src1->w),
          [src2_x] "m" (src2->x), [src2_y] "m" (src2->y),
          [src2_z] "m" (src2->z), [src2_w] "m" (src2->w),
          [src3_x] "m" (src3->x), [src3_y] "m" (src3->y),
          [src3_z] "m" (src3->z), [src3_w] "m" (src3->w)
    );
}

/*************************************/

#define vec3_transform psp_vec3_transform
static inline void psp_vec3_transform(Vector3f *res,
                                      const Vector3f *coord, const Matrix4f *m)
{
    asm("lv.q C000, 0(%[m])\n"
        "lv.q C010, 16(%[m])\n"
        "lv.q C020, 32(%[m])\n"
        "lv.q C030, 48(%[m])\n"
        "lv.s S100, %[x1]\n"
        "lv.s S101, %[y1]\n"
        "lv.s S102, %[z1]\n"
        "vone.s S103\n"
        "vdot.q S110, R000, C100\n"
        "vdot.q S111, R001, C100\n"
        "vdot.q S112, R002, C100\n"
        "sv.s S110, %[x2]\n"
        "sv.s S111, %[y2]\n"
        "sv.s S112, %[z2]\n"
        : [x2] "=m" (res->x), [y2] "=m" (res->y), [z2] "=m" (res->z)
        : [x1] "m" (coord->x), [y1] "m" (coord->y), [z1] "m" (coord->z),
          [m] "r" (m), "m" (*m)
    );
}

/*************************************/

#define mat4_add psp_mat4_add
static inline void psp_mat4_add(Matrix4f *dest,
                                const Matrix4f *src1, const Matrix4f *src2)
{
    asm("lv.q R000, 0(%[src1])\n"
        "lv.q R001, 16(%[src1])\n"
        "lv.q R002, 32(%[src1])\n"
        "lv.q R003, 48(%[src1])\n"
        "lv.q R100, 0(%[src2])\n"
        "lv.q R101, 16(%[src2])\n"
        "lv.q R102, 32(%[src2])\n"
        "lv.q R103, 48(%[src2])\n"
        "vadd.q R200, R000, R100\n"
        "vadd.q R201, R001, R101\n"
        "vadd.q R202, R002, R102\n"
        "vadd.q R203, R003, R103\n"
        "sv.q R200, 0(%[dest])\n"
        "sv.q R201, 16(%[dest])\n"
        "sv.q R202, 32(%[dest])\n"
        "sv.q R203, 48(%[dest])\n"
        : "=m" (*dest)
        : [src1] "r" (src1), "m" (*src1), [src2] "r" (src2), "m" (*src2),
          [dest] "r" (dest)
    );
}

#define mat4_sub psp_mat4_sub
static inline void psp_mat4_sub(Matrix4f *dest,
                                const Matrix4f *src1, const Matrix4f *src2)
{
    asm("lv.q R000, 0(%[src1])\n"
        "lv.q R001, 16(%[src1])\n"
        "lv.q R002, 32(%[src1])\n"
        "lv.q R003, 48(%[src1])\n"
        "lv.q R100, 0(%[src2])\n"
        "lv.q R101, 16(%[src2])\n"
        "lv.q R102, 32(%[src2])\n"
        "lv.q R103, 48(%[src2])\n"
        "vsub.q R200, R000, R100\n"
        "vsub.q R201, R001, R101\n"
        "vsub.q R202, R002, R102\n"
        "vsub.q R203, R003, R103\n"
        "sv.q R200, 0(%[dest])\n"
        "sv.q R201, 16(%[dest])\n"
        "sv.q R202, 32(%[dest])\n"
        "sv.q R203, 48(%[dest])\n"
        : "=m" (*dest)
        : [src1] "r" (src1), "m" (*src1), [src2] "r" (src2), "m" (*src2),
          [dest] "r" (dest)
    );
}

#define mat4_mul psp_mat4_mul
static inline void psp_mat4_mul(Matrix4f *dest,
                                const Matrix4f *src1, const Matrix4f *src2)
{
    asm("lv.q R000, 0(%[src1])\n"
        "lv.q R001, 16(%[src1])\n"
        "lv.q R002, 32(%[src1])\n"
        "lv.q R003, 48(%[src1])\n"
        "lv.q R100, 0(%[src2])\n"
        "lv.q R101, 16(%[src2])\n"
        "lv.q R102, 32(%[src2])\n"
        "lv.q R103, 48(%[src2])\n"
        "vmmul.q M200, M000, M100\n"
        "sv.q R200, 0(%[dest])\n"
        "sv.q R201, 16(%[dest])\n"
        "sv.q R202, 32(%[dest])\n"
        "sv.q R203, 48(%[dest])\n"
        : "=m" (*dest)
        : [src1] "r" (src1), "m" (*src1), [src2] "r" (src2), "m" (*src2),
          [dest] "r" (dest)
    );
}

/*************************************/

#define mat4_det psp_mat4_det
static inline float psp_mat4_det(const Matrix4f *M)
{
    float result;
    uint32_t temp;
    asm("lv.q R400, 0(%[M])\n"
        "lv.q R401, 16(%[M])\n"
        "lv.q R402, 32(%[M])\n"
        "lv.q R403, 48(%[M])\n"
        /* vec4_cross(&minor, &M.col[0], &M.col[1], &M.col[2]); */
        "vmov.q R000, C400\n"
        "vmov.q R001, C410\n"
        "vmov.q R002, C420\n"
        VFPU_VEC4_CROSS(0)
        /* det = vec4_dot(&minor, &M.col[3]); */
        "vdot.q S000, R003, C430[-x,-y,-z,-w]\n"
        "mfv %[temp], S000\n"
        "mtc1 %[temp], %[result]\n"
        : [result] "=f" (result), [temp] "=r" (temp)
        : [M] "r" (M), "m" (*M)
    );
    return result;
}

/*************************************/

#define mat4_inv psp_mat4_inv
static inline float psp_mat4_inv(Matrix4f *dest, const Matrix4f *src)
{
    float result, ftemp;
    uint32_t temp;
    /* 使用レジスタ：
     *     M000.q: src
     *     M100-M300: vec4_cross()同様
     *     S133: det(src), 1/det(src) ※外積計算には使われない
     *     R[4-7]03.q: 逆行列の各列（外積）
     */
    asm(".set push; .set noreorder\n"
        "lv.q R000, 0(%[src])\n"
        "lv.q R001, 16(%[src])\n"
        "lv.q R002, 32(%[src])\n"
        "lv.q R003, 48(%[src])\n"
        /* det = mat4_det(src); if (!det) return 0; */
        "vmov.q R400, C000\n"
        "vmov.q R401, C010\n"
        "vmov.q R402, C020\n"
        VFPU_VEC4_CROSS(4)
        "vdot.q S133, R403, C030[-x,-y,-z,-w]\n"
        "mtc1 $zero, %[ftemp]\n"
        "mfv %[temp], S133\n"
        "mtc1 %[temp], %[result]\n"
        "nop\n"
        "c.eq.s %[result], %[ftemp]\n"
        "nop\n"
        "bc1t 9f\n"
        "nop\n"
        "vrcp.s S133, S133\n"
        /* vec4_cross(&cols[0], &src->v[1], &src->v[2], &src->v[3]); */
        "vmov.q R400, R001\n"
        "vmov.q R401, R002\n"
        "vmov.q R402, R003\n"
        VFPU_VEC4_CROSS(4)
        /* vec4_cross(&cols[1], &src->v[0], &src->v[3], &src->v[2]); */
        "vmov.q R500, R000\n"
        "vmov.q R501, R003\n"
        "vmov.q R502, R002\n"
        VFPU_VEC4_CROSS(5)
        /* vec4_cross(&cols[2], &src->v[0], &src->v[1], &src->v[3]); */
        "vmov.q R600, R000\n"
        "vmov.q R601, R001\n"
        "vmov.q R602, R003\n"
        VFPU_VEC4_CROSS(6)
        /* vec4_cross(&cols[3], &src->v[0], &src->v[2], &src->v[1]); */
        "vmov.q R700, R000\n"
        "vmov.q R701, R002\n"
        "vmov.q R702, R001\n"
        VFPU_VEC4_CROSS(7)
        /* dest.cols = cols / det; */
        "vscl.q C000, R403, S133\n"
        "vscl.q C010, R503, S133\n"
        "vscl.q C020, R603, S133\n"
        "vscl.q C030, R703, S133\n"
        /* 完了 */
        "sv.q R000, 0(%[dest])\n"
        "sv.q R001, 16(%[dest])\n"
        "sv.q R002, 32(%[dest])\n"
        "sv.q R003, 48(%[dest])\n"
        "9: .set pop\n"
        : [result] "=f" (result), [temp] "=r" (temp), [ftemp] "=f" (ftemp),
          "=m" (*dest)
        : [dest] "r" (dest), [src] "r" (src), "m" (*src)
    );
    return result;
}

/*************************************/

#endif  // USE_VFPU_VECTOR_MATRIX_FUNCS

/*************************************************************************/
/************************* テーブル型三角形関数 **************************/
/*************************************************************************/

#ifdef USE_TRIG_TABLES

#define CUSTOM_DTRIG_FUNCTIONS

#undef dsinf
#undef dcosf
#undef dtanf
#undef dsincosf
/* misc.cにて定義 */
extern CONST_FUNCTION float dsinf(const float x);
extern CONST_FUNCTION float dcosf(const float x);
extern CONST_FUNCTION float dtanf(const float x);
extern void dsincosf_kernel(const float x);

#define dsincosf psp_dsincosf  // static定義が../common.hの宣言と異なるため
static inline void dsincosf(const float x, float *sinx, float *cosx)
{
    dsincosf_kernel(x);
    asm volatile (".set push; .set noreorder\n"
        "mov.s %0, $f0\n"
        "mov.s %1, $f1\n"
        ".set pop"
        : "=f" (*sinx), "=f" (*cosx)
        : /* no inputs */
        : "$f0", "$f1"
    );
}

#endif  // USE_TRIG_TABLES

/*************************************************************************/
/************************** その他の共通マクロ ***************************/
/*************************************************************************/

/* PSPピクセルデータ順に応じてPACK_ARGB()を再定義 */
#undef PACK_ARGB
#define PACK_ARGB(a,r,g,b)  ((a)<<24 | (b)<<16 | (g)<<8 | (r))

/*************************************************************************/
/*************************************************************************/

#endif  // SYSDEP_PSP_COMMON_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
