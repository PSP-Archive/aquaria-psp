/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/test/test-math.c: Test routines for mathematical functions.
 */

#include "../common.h"

#ifdef INCLUDE_TESTS  // ファイル末尾まで

#include "../test.h"
#include "../vector.h"

/*************************************************************************/

/* 1.0fに対して、仮数部最下位ビットの値の半分より少し小さい値。つまり、
 * 丸め動作が「近い方へ」となっているとき、1.0f + TINY == 1.0fが成り立つ。
 * 単精度浮動小数点数の動作確認に使う。 */
#define TINY  (0.999f / (1<<24))

/* 2つの浮動小数点数の差が、誤差と認められる範囲内かどうかを確認する。 */
#define CLOSE_ENOUGH(a,b) (fabsf((a) - (b)) / (b) < 1/(float)(1<<20))

/*************************************************************************/
/*************************************************************************/

/**
 * test_dtrig:  度単位の三角関数の動作をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
int test_dtrig(void)
{
    static const struct {
        float deg;
        double expect_sin;
        double expect_cos;
        double expect_tan;
    } testlist[] = {
#define SQRT_2 1.41421356237310
#define SQRT_3 1.73205080756888
        {  0, 0.0, 1.0, 0.0},
        { 30, 0.5, SQRT_3/2, 1/SQRT_3},
        { 45, SQRT_2/2, SQRT_2/2, 1.0},
        { 60, SQRT_3/2, 0.5, SQRT_3},
        { 90, 1.0, 0.0, 1.0/0.0},
        {120, SQRT_3/2, -0.5, -SQRT_3},
        {135, SQRT_2/2, -SQRT_2/2, -1.0},
        {150, 0.5, -SQRT_3/2, -1/SQRT_3},
        {180, 0.0, -1.0, 0.0},
        {210, -0.5, -SQRT_3/2, 1/SQRT_3},
        {225, -SQRT_2/2, -SQRT_2/2, 1.0},
        {240, -SQRT_3/2, -0.5, SQRT_3},
        {270, -1.0, 0.0, 1.0/0.0},
        {300, -SQRT_3/2, 0.5, -SQRT_3},
        {315, -SQRT_2/2, SQRT_2/2, -1.0},
        {330, -0.5, SQRT_3/2, -1/SQRT_3},
        {360, 0.0, 1.0, 0.0},
        {390, 0.5, SQRT_3/2, 1/SQRT_3},
        {720, 0.0, 1.0, 0.0},
        {750, 0.5, SQRT_3/2, 1/SQRT_3},
        {-0.0, 0.0, 1.0, 0.0},
        {-30, -0.5, SQRT_3/2, -1/SQRT_3},
        {-390, -0.5, SQRT_3/2, -1/SQRT_3},
        {0.25, 0.00436330928474657, 0.999990480720734, 0.00436335082070157},
        {(float)0x3FFFFFC0L, 0.0, 1.0, 0.0},
        {(float)-0x7FFFFF80L, 0.0, 1.0, 0.0},
    };

    int failed = 0;

    int i;
    for (i = 0; i < lenof(testlist); i++) {
        const float sinf_res  = dsinf(testlist[i].deg);
        const float cosf_res  = dcosf(testlist[i].deg);
        const float tanf_res  = dtanf(testlist[i].deg);
        const float atanf_res = datan2f(testlist[i].expect_sin,
                                        testlist[i].expect_cos);
        float sincosf_sin, sincosf_cos;
        dsincosf(testlist[i].deg, &sincosf_sin, &sincosf_cos);
#ifdef USE_DOUBLE_DTRIG
        const double sin_res  = dsin(testlist[i].deg);
        const double cos_res  = dcos(testlist[i].deg);
        const double tan_res  = dtan(testlist[i].deg);
        const double atan_res = datan2(testlist[i].expect_sin,
                                       testlist[i].expect_cos);
        double sincos_sin, sincos_cos;
        dsincos(testlist[i].deg, &sincos_sin, &sincos_cos);
#endif

        const double expect_sin = testlist[i].expect_sin;
        const double delta_sin = (expect_sin==floor(expect_sin)) ? 0 : 1.0e-6;
#ifdef USE_DOUBLE_DTRIG
        const double min_sin = expect_sin - delta_sin;
        const double max_sin = expect_sin + delta_sin;
#endif
        const float expect_sinf = (float)expect_sin;
        const float min_sinf = expect_sinf - (float)delta_sin;
        const float max_sinf = expect_sinf + (float)delta_sin;
        const double expect_cos = testlist[i].expect_cos;
        const double delta_cos = (expect_cos==floor(expect_cos)) ? 0 : 1.0e-6;
#ifdef USE_DOUBLE_DTRIG
        const double min_cos = expect_cos - delta_cos;
        const double max_cos = expect_cos + delta_cos;
#endif
        const float expect_cosf = (float)expect_cos;
        const float min_cosf = expect_cosf - (float)delta_cos;
        const float max_cosf = expect_cosf + (float)delta_cos;
        const double expect_tan = testlist[i].expect_tan;
        const double delta_tan = (expect_tan==floor(expect_tan)) ? 0 : 1.0e-6;
#ifdef USE_DOUBLE_DTRIG
        const double min_tan = expect_tan - delta_tan;
        const double max_tan = expect_tan + delta_tan;
#endif
        const float expect_tanf = (float)expect_tan;
        const float min_tanf = expect_tanf - (float)delta_tan;
        const float max_tanf = expect_tanf + (float)delta_tan;
        const double expect_atan = fmod(fmod(testlist[i].deg, 360) + 360, 360);
        const double delta_atan = (expect_atan/45==floor(expect_atan/45))
                                  ? 0 : 1.0e-4;  // 範囲が広いので誤差も大きく
#ifdef USE_DOUBLE_DTRIG
        const double min_atan = expect_atan - delta_atan;
        const double max_atan = expect_atan + delta_atan;
#endif
        const float expect_atanf = (float)expect_atan;
        const float min_atanf = expect_atanf - delta_atan;
        const float max_atanf = expect_atanf + delta_atan;

#if defined(PSP) && defined(DEBUG)  // チェック中の例外発生を抑える
        asm("ctc1 %[val], $31" : : [val] "r" (0x01000000));
#endif
        if (sinf_res < min_sinf || sinf_res > max_sinf) {
            DMSG("FAIL: dsinf(%f) = %f (d=%f)", testlist[i].deg, sinf_res,
                sinf_res - expect_sinf);
            failed = 1;
        }
        if (cosf_res < min_cosf || cosf_res > max_cosf) {
            DMSG("FAIL: dcosf(%f) = %f (d=%f)", testlist[i].deg, cosf_res,
                cosf_res - expect_cosf);
            failed = 1;
        }
        if (isinff(expect_tanf)
            ? !isinff(tanf_res)
            : (tanf_res < min_tanf || tanf_res > max_tanf)
        ) {
            DMSG("FAIL: dtanf(%f) = %f (d=%f)", testlist[i].deg, tanf_res,
                 tanf_res - expect_tanf);
            failed = 1;
        }
        if (atanf_res < min_atanf || atanf_res > max_atanf) {
            DMSG("FAIL: datan2f(%f,%f) = %f (d=%f)", testlist[i].expect_sin,
                 testlist[i].expect_cos, atanf_res, atanf_res - expect_atanf);
            failed = 1;
        }
        if (sincosf_sin < min_sinf || sincosf_sin > max_sinf) {
            DMSG("FAIL: dsincosf(%f).sin = %f (d=%f)", testlist[i].deg,
                 sincosf_sin, sincosf_sin - expect_sinf);
            failed = 1;
        }
        if (sincosf_cos < min_cosf || sincosf_cos > max_cosf) {
            DMSG("FAIL: dsincosf(%f).cos = %f (d=%f)", testlist[i].deg,
                 sincosf_cos, sincosf_cos - expect_cosf);
            failed = 1;
        }

#ifdef USE_DOUBLE_DTRIG
        if (sin_res < min_sin || sin_res > max_sin) {
            DMSG("FAIL: dsin(%f) = %f (d=%f)", testlist[i].deg, sin_res,
                sin_res - expect_sin);
            failed = 1;
        }
        if (cos_res < min_cos || cos_res > max_cos) {
            DMSG("FAIL: dcos(%f) = %f (d=%f)", testlist[i].deg, cos_res,
                cos_res - expect_cos);
            failed = 1;
        }
        if (isinf(expect_tan)
            ? !isinf(tan_res)
            : (tan_res < min_tan || tan_res > max_tan)
        ) {
            DMSG("FAIL: dtan(%f) = %f (d=%f)", testlist[i].deg, tan_res,
                tan_res - expect_tan);
            failed = 1;
        }
        if (atan_res < min_atan || atan_res > max_atan) {
            DMSG("FAIL: datan2(%f,%f) = %f (d=%f)", testlist[i].expect_sin,
                 testlist[i].expect_cos, atan_res, atan_res - expect_atan);
            failed = 1;
        }
        if (sincos_sin < min_sin || sincos_sin > max_sin) {
            DMSG("FAIL: dsincos(%f).sin = %f (d=%f)", testlist[i].deg,
                 sincos_sin, sincos_sin - expect_sin);
            failed = 1;
        }
        if (sincos_cos < min_cos || sincos_cos > max_cos) {
            DMSG("FAIL: dsincos(%f).cos = %f (d=%f)", testlist[i].deg,
                 sincos_cos, sincos_cos - expect_cos);
            failed = 1;
        }
#endif  // USE_DOUBLE_DTRIG

#if defined(PSP) && defined(DEBUG)  // 例外処理を元に戻す
        asm("cfc1 $zero, $31");  // FPU処理完了を待つ
        asm("ctc1 %[val], $31" : : [val] "r" (0x01000000));  // 例外をクリア
        asm("ctc1 %[val], $31" : : [val] "r" (0x01000E00));
#endif
    }

    return !failed;
}

/*************************************************************************/

/**
 * test_vector:  ベクトル関数をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
int test_vector(void)
{
    static const struct {
        enum {ADD, SUB, SCALE, DOT, LEN, LEN2, NORM, CROSS} type;
        int size;
        float a[4], b[4], res[4];
        int allow_approximate;  // 若干の誤差を容認する（PSP VFPU用）
    } testlist[] = {
        {ADD,   2, {1,2}, {3.5,5.5}, {4.5,7.5}},
        {ADD,   2, {1,1}, {TINY}, {1,1}},
        {ADD,   3, {1,2,3}, {4.5,6.5,8.5}, {5.5,8.5,11.5}},
        {ADD,   3, {1,1,1}, {TINY,TINY,TINY}, {1,1,1}},
        {ADD,   4, {1,2,3,4}, {5.5,7.5,9.5,11.5}, {6.5,9.5,12.5,15.5}},
        {ADD,   4, {1,1,1,1}, {TINY,TINY,TINY,TINY}, {1,1,1,1}},

        {SUB,   2, {1,2}, {3.5,5.5}, {-2.5,-3.5}},
        {SUB,   2, {1,1}, {TINY/2,TINY/2}, {1,1}},
        {SUB,   3, {1,2,3}, {4.5,6.5,8.5}, {-3.5,-4.5,-5.5}},
        {SUB,   3, {1,1,1}, {TINY/2,TINY/2,TINY/2}, {1,1,1}},
        {SUB,   4, {1,2,3,4}, {5.5,7.5,9.5,11.5}, {-4.5,-5.5,-6.5,-7.5}},
        {SUB,   4, {1,1,1,1}, {TINY/2,TINY/2,TINY/2,TINY/2}, {1,1,1,1}},

        {SCALE, 2, {0,0}, {0}, {0,0}},
        {SCALE, 2, {0,0}, {1}, {0,0}},
        {SCALE, 2, {0,0}, {2.5}, {0,0}},
        {SCALE, 2, {1,1}, {0}, {0,0}},
        {SCALE, 2, {1,1}, {1}, {1,1}},
        {SCALE, 2, {1,1}, {2.5}, {2.5,2.5}},
        {SCALE, 2, {3,4}, {0}, {0,0}},
        {SCALE, 2, {3,4}, {1}, {3,4}},
        {SCALE, 2, {3,4}, {2.5}, {7.5,10}},
        {SCALE, 3, {0,0,0}, {0}, {0,0,0}},
        {SCALE, 3, {0,0,0}, {1}, {0,0,0}},
        {SCALE, 3, {0,0,0}, {2.5}, {0,0,0}},
        {SCALE, 3, {1,1,1}, {0}, {0,0,0}},
        {SCALE, 3, {1,1,1}, {1}, {1,1,1}},
        {SCALE, 3, {1,1,1}, {2.5}, {2.5,2.5,2.5}},
        {SCALE, 3, {3,4,5}, {0}, {0,0,0}},
        {SCALE, 3, {3,4,5}, {1}, {3,4,5}},
        {SCALE, 3, {3,4,5}, {2.5}, {7.5,10,12.5}},
        {SCALE, 4, {0,0,0,0}, {0}, {0,0,0,0}},
        {SCALE, 4, {0,0,0,0}, {1}, {0,0,0,0}},
        {SCALE, 4, {0,0,0,0}, {2.5}, {0,0,0,0}},
        {SCALE, 4, {1,1,1,1}, {0}, {0,0,0,0}},
        {SCALE, 4, {1,1,1,1}, {1}, {1,1,1,1}},
        {SCALE, 4, {1,1,1,1}, {2.5}, {2.5,2.5,2.5,2.5}},
        {SCALE, 4, {3,4,5,6}, {0}, {0,0,0,0}},
        {SCALE, 4, {3,4,5,6}, {1}, {3,4,5,6}},
        {SCALE, 4, {3,4,5,6}, {2.5}, {7.5,10,12.5,15}},

        {DOT,   2, {0,0}, {0,0}, {0}},
        {DOT,   2, {0,0}, {1,1}, {0}},
        {DOT,   2, {0,1}, {0,1}, {1}},
        {DOT,   2, {0,1}, {1,0}, {0}},
        {DOT,   2, {1,0}, {0,1}, {0}},
        {DOT,   2, {1,0}, {1,0}, {1}},
        {DOT,   2, {1,1}, {1,1}, {2}},
        {DOT,   2, {3,4}, {5,6}, {39}},
        {DOT,   2, {3,4}, {5.5,6.5}, {42.5}},
        {DOT,   3, {0,0,0}, {0,0,0}, {0}},
        {DOT,   3, {0,0,0}, {1,1,1}, {0}},
        {DOT,   3, {0,0,1}, {0,0,1}, {1}},
        {DOT,   3, {0,0,1}, {0,1,0}, {0}},
        {DOT,   3, {0,0,1}, {1,0,0}, {0}},
        {DOT,   3, {0,1,0}, {0,0,1}, {0}},
        {DOT,   3, {0,1,0}, {0,1,0}, {1}},
        {DOT,   3, {0,1,0}, {1,0,0}, {0}},
        {DOT,   3, {1,0,0}, {0,0,1}, {0}},
        {DOT,   3, {1,0,0}, {0,1,0}, {0}},
        {DOT,   3, {1,0,0}, {1,0,0}, {1}},
        {DOT,   3, {1,1,1}, {1,1,1}, {3}},
        {DOT,   3, {3,4,5}, {5,6,7}, {74}},
        {DOT,   3, {3,4,5}, {5.5,6.5,7.5}, {80}},
        {DOT,   4, {0,0,0,0}, {0,0,0}, {0}},
        {DOT,   4, {0,0,0,0}, {1,1,1}, {0}},
        {DOT,   4, {0,0,0,1}, {0,0,0,1}, {1}},
        {DOT,   4, {0,0,0,1}, {0,0,1,0}, {0}},
        {DOT,   4, {0,0,0,1}, {0,1,0,0}, {0}},
        {DOT,   4, {0,0,0,1}, {1,0,0,0}, {0}},
        {DOT,   4, {0,0,1,0}, {0,0,0,1}, {0}},
        {DOT,   4, {0,0,1,0}, {0,0,1,0}, {1}},
        {DOT,   4, {0,0,1,0}, {0,1,0,0}, {0}},
        {DOT,   4, {0,0,1,0}, {1,0,0,0}, {0}},
        {DOT,   4, {0,1,0,0}, {0,0,0,1}, {0}},
        {DOT,   4, {0,1,0,0}, {0,0,1,0}, {0}},
        {DOT,   4, {0,1,0,0}, {0,1,0,0}, {1}},
        {DOT,   4, {0,1,0,0}, {1,0,0,0}, {0}},
        {DOT,   4, {1,0,0,0}, {0,0,0,1}, {0}},
        {DOT,   4, {1,0,0,0}, {0,0,1,0}, {0}},
        {DOT,   4, {1,0,0,0}, {0,1,0,0}, {0}},
        {DOT,   4, {1,0,0,0}, {1,0,0,0}, {1}},
        {DOT,   4, {1,1,1,1}, {1,1,1,1}, {4}},
        {DOT,   4, {3,4,5,6}, {5,6,7,8}, {122}},
        {DOT,   4, {3,4,5,6}, {5.5,6.5,7.5,8.5}, {131}},

        {LEN,   2, {0,0}, {}, {0}},
        {LEN,   2, {0,1}, {}, {1}},
        {LEN,   2, {1,0}, {}, {1}},
        {LEN,   2, {1.5,2}, {}, {2.5}},
        {LEN,   3, {0,0,0}, {}, {0}},
        {LEN,   3, {0,0,1}, {}, {1}},
        {LEN,   3, {0,1,0}, {}, {1}},
        {LEN,   3, {1,0,0}, {}, {1}},
        {LEN,   3, {1.5,3,3}, {}, {4.5}},
        {LEN,   4, {0,0,0,0}, {}, {0}},
        {LEN,   4, {0,0,0,1}, {}, {1}},
        {LEN,   4, {0,0,1,0}, {}, {1}},
        {LEN,   4, {0,1,0,0}, {}, {1}},
        {LEN,   4, {1,0,0,0}, {}, {1}},
        {LEN,   4, {1.5,1.5,1.5,1.5}, {}, {3}},

        {LEN2,  2, {0,0}, {}, {0}},
        {LEN2,  2, {0,1}, {}, {1}},
        {LEN2,  2, {1,0}, {}, {1}},
        {LEN2,  2, {1.5,2}, {}, {6.25}},
        {LEN2,  2, {5,6}, {}, {61}},    // √61は単精度では正確に表現できない
        {LEN2,  3, {0,0,0}, {}, {0}},   // ので、sqrtf()が入った時は失敗する
        {LEN2,  3, {0,0,1}, {}, {1}},
        {LEN2,  3, {0,1,0}, {}, {1}},
        {LEN2,  3, {1,0,0}, {}, {1}},
        {LEN2,  3, {1.5,3,3}, {}, {20.25}},
        {LEN2,  3, {3,4,6}, {}, {61}},
        {LEN2,  4, {0,0,0,0}, {}, {0}},
        {LEN2,  4, {0,0,0,1}, {}, {1}},
        {LEN2,  4, {0,0,1,0}, {}, {1}},
        {LEN2,  4, {0,1,0,0}, {}, {1}},
        {LEN2,  4, {1,0,0,0}, {}, {1}},
        {LEN2,  4, {1.5,1.5,1.5,1.5}, {}, {9}},
        {LEN2,  4, {5,5,2,1}, {}, {55}},  // √55も単精度では正確に表現できない

        {NORM,  2, {0,1}, {}, {0,1}},
        {NORM,  2, {1,0}, {}, {1,0}},
        {NORM,  2, {3,4}, {}, {0.6,0.8}, 1},
        {NORM,  3, {0,0,1}, {}, {0,0,1}},
        {NORM,  3, {0,1,0}, {}, {0,1,0}},
        {NORM,  3, {1,0,0}, {}, {1,0,0}},
        {NORM,  3, {2,4,4}, {}, {0.333333333,0.666666667,0.666666667}, 1},
        {NORM,  4, {0,0,0,1}, {}, {0,0,0,1}},
        {NORM,  4, {0,0,1,0}, {}, {0,0,1,0}},
        {NORM,  4, {0,1,0,0}, {}, {0,1,0,0}},
        {NORM,  4, {1,0,0,0}, {}, {1,0,0,0}},
        {NORM,  4, {1,1,1,1}, {}, {0.5,0.5,0.5,0.5}},

        {CROSS, 3, {0,0,0}, {0,0,0}, {0,0,0}},
        {CROSS, 3, {0,0,1}, {0,0,1}, {0,0,0}},
        {CROSS, 3, {0,0,1}, {0,1,0}, {-1,0,0}},
        {CROSS, 3, {0,0,1}, {1,0,0}, {0,1,0}},
        {CROSS, 3, {0,1,0}, {0,0,1}, {1,0,0}},
        {CROSS, 3, {0,1,0}, {0,1,0}, {0,0,0}},
        {CROSS, 3, {0,1,0}, {1,0,0}, {0,0,-1}},
        {CROSS, 3, {1,0,0}, {0,0,1}, {0,-1,0}},
        {CROSS, 3, {1,0,0}, {0,1,0}, {0,0,1}},
        {CROSS, 3, {1,0,0}, {1,0,0}, {0,0,0}},
        {CROSS, 3, {1.5,2.5,3.5}, {4.25,5.5,7.75}, {0.125,3.25,-2.375}},
    };

    static const struct {
        /* 4成分ベクトルの外積には3つのベクトルが必要 */
        float a[4], b[4], c[4], res[4];
    } testlist_cross4[] = {
        {{0,0,0,0}, {0,0,0,0}, {0,0,0,0},   {0,0,0,0}},

        {{0,0,0,1}, {0,0,0,1}, {0,0,0,1},   {0,0,0,0}},
        {{0,0,0,1}, {0,0,0,1}, {0,0,1,0},   {0,0,0,0}},
        {{0,0,0,1}, {0,0,0,1}, {0,1,0,0},   {0,0,0,0}},
        {{0,0,0,1}, {0,0,0,1}, {1,0,0,0},   {0,0,0,0}},
        {{0,0,0,1}, {0,0,1,0}, {0,0,0,1},   {0,0,0,0}},
        {{0,0,0,1}, {0,0,1,0}, {0,0,1,0},   {0,0,0,0}},
        {{0,0,0,1}, {0,0,1,0}, {0,1,0,0},   {-1,0,0,0}},
        {{0,0,0,1}, {0,0,1,0}, {1,0,0,0},   {0,1,0,0}},
        {{0,0,0,1}, {0,1,0,0}, {0,0,0,1},   {0,0,0,0}},
        {{0,0,0,1}, {0,1,0,0}, {0,0,1,0},   {1,0,0,0}},
        {{0,0,0,1}, {0,1,0,0}, {0,1,0,0},   {0,0,0,0}},
        {{0,0,0,1}, {0,1,0,0}, {1,0,0,0},   {0,0,-1,0}},
        {{0,0,0,1}, {1,0,0,0}, {0,0,0,1},   {0,0,0,0}},
        {{0,0,0,1}, {1,0,0,0}, {0,0,1,0},   {0,-1,0,0}},
        {{0,0,0,1}, {1,0,0,0}, {0,1,0,0},   {0,0,1,0}},
        {{0,0,0,1}, {1,0,0,0}, {1,0,0,0},   {0,0,0,0}},

        {{0,0,1,0}, {0,0,0,1}, {0,0,0,1},   {0,0,0,0}},
        {{0,0,1,0}, {0,0,0,1}, {0,0,1,0},   {0,0,0,0}},
        {{0,0,1,0}, {0,0,0,1}, {0,1,0,0},   {1,0,0,0}},
        {{0,0,1,0}, {0,0,0,1}, {1,0,0,0},   {0,-1,0,0}},
        {{0,0,1,0}, {0,0,1,0}, {0,0,0,1},   {0,0,0,0}},
        {{0,0,1,0}, {0,0,1,0}, {0,0,1,0},   {0,0,0,0}},
        {{0,0,1,0}, {0,0,1,0}, {0,1,0,0},   {0,0,0,0}},
        {{0,0,1,0}, {0,0,1,0}, {1,0,0,0},   {0,0,0,0}},
        {{0,0,1,0}, {0,1,0,0}, {0,0,0,1},   {-1,0,0,0}},
        {{0,0,1,0}, {0,1,0,0}, {0,0,1,0},   {0,0,0,0}},
        {{0,0,1,0}, {0,1,0,0}, {0,1,0,0},   {0,0,0,0}},
        {{0,0,1,0}, {0,1,0,0}, {1,0,0,0},   {0,0,0,1}},
        {{0,0,1,0}, {1,0,0,0}, {0,0,0,1},   {0,1,0,0}},
        {{0,0,1,0}, {1,0,0,0}, {0,0,1,0},   {0,0,0,0}},
        {{0,0,1,0}, {1,0,0,0}, {0,1,0,0},   {0,0,0,-1}},
        {{0,0,1,0}, {1,0,0,0}, {1,0,0,0},   {0,0,0,0}},

        {{0,1,0,0}, {0,0,0,1}, {0,0,0,1},   {0,0,0,0}},
        {{0,1,0,0}, {0,0,0,1}, {0,0,1,0},   {-1,0,0,0}},
        {{0,1,0,0}, {0,0,0,1}, {0,1,0,0},   {0,0,0,0}},
        {{0,1,0,0}, {0,0,0,1}, {1,0,0,0},   {0,0,1,0}},
        {{0,1,0,0}, {0,0,1,0}, {0,0,0,1},   {1,0,0,0}},
        {{0,1,0,0}, {0,0,1,0}, {0,0,1,0},   {0,0,0,0}},
        {{0,1,0,0}, {0,0,1,0}, {0,1,0,0},   {0,0,0,0}},
        {{0,1,0,0}, {0,0,1,0}, {1,0,0,0},   {0,0,0,-1}},
        {{0,1,0,0}, {0,1,0,0}, {0,0,0,1},   {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {0,0,1,0},   {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {0,1,0,0},   {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {1,0,0,0},   {0,0,0,0}},
        {{0,1,0,0}, {1,0,0,0}, {0,0,0,1},   {0,0,-1,0}},
        {{0,1,0,0}, {1,0,0,0}, {0,0,1,0},   {0,0,0,1}},
        {{0,1,0,0}, {1,0,0,0}, {0,1,0,0},   {0,0,0,0}},
        {{0,1,0,0}, {1,0,0,0}, {1,0,0,0},   {0,0,0,0}},

        {{1,0,0,0}, {0,0,0,1}, {0,0,0,1},   {0,0,0,0}},
        {{1,0,0,0}, {0,0,0,1}, {0,0,1,0},   {0,1,0,0}},
        {{1,0,0,0}, {0,0,0,1}, {0,1,0,0},   {0,0,-1,0}},
        {{1,0,0,0}, {0,0,0,1}, {1,0,0,0},   {0,0,0,0}},
        {{1,0,0,0}, {0,0,1,0}, {0,0,0,1},   {0,-1,0,0}},
        {{1,0,0,0}, {0,0,1,0}, {0,0,1,0},   {0,0,0,0}},
        {{1,0,0,0}, {0,0,1,0}, {0,1,0,0},   {0,0,0,1}},
        {{1,0,0,0}, {0,0,1,0}, {1,0,0,0},   {0,0,0,0}},
        {{1,0,0,0}, {0,1,0,0}, {0,0,0,1},   {0,0,1,0}},
        {{1,0,0,0}, {0,1,0,0}, {0,0,1,0},   {0,0,0,-1}},
        {{1,0,0,0}, {0,1,0,0}, {0,1,0,0},   {0,0,0,0}},
        {{1,0,0,0}, {0,1,0,0}, {1,0,0,0},   {0,0,0,0}},
        {{1,0,0,0}, {1,0,0,0}, {0,0,0,1},   {0,0,0,0}},
        {{1,0,0,0}, {1,0,0,0}, {0,0,1,0},   {0,0,0,0}},
        {{1,0,0,0}, {1,0,0,0}, {0,1,0,0},   {0,0,0,0}},
        {{1,0,0,0}, {1,0,0,0}, {1,0,0,0},   {0,0,0,0}},

        {{1.5,2.5,3.5,4.5}, {5.25,-6.75,7.25,-8.75}, {-9,-10,-11.5,-12.5},
         {208.375,-240.375,-153.375,183.375}},
    };

    static const struct {
        /* 並べて記述できるように、行単位で定義する */
        float coord[3], m1[4], res[3], m2[4], m3[4], m4[4];
    } testlist_xform[] = {
        {{2,3,4}, {1,0,0,0}, {2,3,4},
                  {0,1,0,0},
                  {0,0,1,0},
                  {0,0,0,1}},
        {{2,3,4}, {2,0,0,0}, {4,6,8},
                  {0,2,0,0},
                  {0,0,2,0},
                  {0,0,0,1}},
        {{2,3,4}, {1,0,0,0}, {5,7,9},
                  {0,1,0,0},
                  {0,0,1,0},
                  {3,4,5,1}},
        {{2,3,4}, {1.5,2.5,3.5,0}, {36.5,44.5,52.5},
                  {4.5,5.5,6.5,0},
                  {7.5,8.5,9.5,0},
                  {-10,-11,-12,1}},
    };

    int failed = 0;

    /* まず、入力と出力ベクトルに同じポインタを使った場合、外積計算が失敗
     * しないことを確認する */
    {
        Vector3f a3, b3;
        a3.x = a3.y = a3.z = 1;
        b3.x = 2; b3.y = 4; b3.z = 7;
        vec3_cross(&a3, &a3, &b3);
        if (a3.x != 3 || a3.y != -5 || a3.z != 2) {
            DMSG("FAIL: vec3_cross(dest == src1): result=<%.2f,%.2f,%.2f>"
                 " expect=<3.00,-5.00,2.00>", a3.x, a3.y, a3.z);
            failed = 1;
        }
        a3.x = a3.y = a3.z = 1;
        vec3_cross(&b3, &a3, &b3);
        if (b3.x != 3 || b3.y != -5 || b3.z != 2) {
            DMSG("FAIL: vec3_cross(dest == src2): result=<%.2f,%.2f,%.2f>"
                 " expect=<3.00,-5.00,2.00>", b3.x, b3.y, b3.z);
            failed = 1;
        }

        Vector4f a4, b4, c4;
        a4.x = a4.y = a4.z = a4.w = 1;
        b4.x = 2; b4.y = 4; b4.z = 7; b4.w = 11;
        c4.x = -5; c4.y = -11; c4.z = -18; c4.w = -26;
        vec4_cross(&a4, &a4, &b4, &c4);
        if (a4.x != 4 || a4.y != -12 || a4.z != 12 || a4.w != -4) {
            DMSG("FAIL: vec4_cross(dest == src1): result=<%.2f,%.2f,%.2f,%.2f>"
                 " expect=<4.00,-12.00,12.00,-4.00>", a4.x, a4.y, a4.z, a4.w);
            failed = 1;
        }
        a4.x = a4.y = a4.z = a4.w = 1;
        vec4_cross(&b4, &a4, &b4, &c4);
        if (b4.x != 4 || b4.y != -12 || b4.z != 12 || b4.w != -4) {
            DMSG("FAIL: vec4_cross(dest == src2): result=<%.2f,%.2f,%.2f,%.2f>"
                 " expect=<4.00,-12.00,12.00,-4.00>", b4.x, b4.y, b4.z, b4.w);
            failed = 1;
        }
        b4.x = 2; b4.y = 4; b4.z = 7; b4.w = 11;
        vec4_cross(&c4, &a4, &b4, &c4);
        if (c4.x != 4 || c4.y != -12 || c4.z != 12 || c4.w != -4) {
            DMSG("FAIL: vec4_cross(dest == src3): result=<%.2f,%.2f,%.2f,%.2f>"
                 " expect=<4.00,-12.00,12.00,-4.00>", c4.x, c4.y, c4.z, c4.w);
            failed = 1;
        }
    }

    int i;
    for (i = 0; i < lenof(testlist); i++) {
        if (testlist[i].size == 2) {
            Vector2f a, b, res;
            a.v[0] = testlist[i].a[0];
            a.v[1] = testlist[i].a[1];
            b.v[0] = testlist[i].b[0];
            b.v[1] = testlist[i].b[1];
            res.v[0] = res.v[1] = 0;
            switch (testlist[i].type) {
                case ADD:   vec2_add(&res, &a, &b); break;
                case SUB:   vec2_sub(&res, &a, &b); break;
                case SCALE: vec2_scale(&res, &a, b.v[0]); break;
                case DOT:   res.v[0] = vec2_dot(&a, &b); break;
                case LEN:   res.v[0] = vec2_length(&a); break;
                case LEN2:  res.v[0] = vec2_length2(&a); break;
                case NORM:  vec2_normalize(&res, &a); break;
                case CROSS: DMSG("FAIL: test %d: CROSS(2) invalid", i);
                            failed = 1; break;
            }
            if (testlist[i].allow_approximate
             && CLOSE_ENOUGH(res.v[0], testlist[i].res[0])
             && CLOSE_ENOUGH(res.v[1], testlist[i].res[1])
            ) {
                continue;
            }
            if (res.v[0] != testlist[i].res[0]
             || res.v[1] != testlist[i].res[1]
            ) {
                DMSG("FAIL: test %d: result <%f,%f> != expect <%f,%f>",
                     i, res.v[0], res.v[1],
                     testlist[i].res[0], testlist[i].res[1]);
                failed = 1;
            }
        } else if (testlist[i].size == 3) {
            Vector3f a, b, res;
            a.v[0] = testlist[i].a[0];
            a.v[1] = testlist[i].a[1];
            a.v[2] = testlist[i].a[2];
            b.v[0] = testlist[i].b[0];
            b.v[1] = testlist[i].b[1];
            b.v[2] = testlist[i].b[2];
            res.v[0] = res.v[1] = res.v[2] = 0;
            switch (testlist[i].type) {
                case ADD: vec3_add(&res, &a, &b); break;
                case SUB: vec3_sub(&res, &a, &b); break;
                case SCALE: vec3_scale(&res, &a, b.v[0]); break;
                case DOT:   res.v[0] = vec3_dot(&a, &b); break;
                case LEN:   res.v[0] = vec3_length(&a); break;
                case LEN2:  res.v[0] = vec3_length2(&a); break;
                case NORM:  vec3_normalize(&res, &a); break;
                case CROSS: vec3_cross(&res, &a, &b);
            }
            if (testlist[i].allow_approximate
             && CLOSE_ENOUGH(res.v[0], testlist[i].res[0])
             && CLOSE_ENOUGH(res.v[1], testlist[i].res[1])
             && CLOSE_ENOUGH(res.v[2], testlist[i].res[2])
            ) {
                continue;
            }
            if (res.v[0] != testlist[i].res[0]
             || res.v[1] != testlist[i].res[1]
             || res.v[2] != testlist[i].res[2]
            ) {
                DMSG("FAIL: test %d: result <%f,%f,%f> != expect <%f,%f,%f>",
                     i, res.v[0], res.v[1], res.v[2],
                     testlist[i].res[0], testlist[i].res[1],
                     testlist[i].res[2]);
                failed = 1;
            }
        } else if (testlist[i].size == 4) {
            Vector4f a, b, res;
            a.v[0] = testlist[i].a[0];
            a.v[1] = testlist[i].a[1];
            a.v[2] = testlist[i].a[2];
            a.v[3] = testlist[i].a[3];
            b.v[0] = testlist[i].b[0];
            b.v[1] = testlist[i].b[1];
            b.v[2] = testlist[i].b[2];
            b.v[3] = testlist[i].b[3];
            res.v[0] = res.v[1] = res.v[2] = res.v[3] = 0;
            switch (testlist[i].type) {
                case ADD: vec4_add(&res, &a, &b); break;
                case SUB: vec4_sub(&res, &a, &b); break;
                case SCALE: vec4_scale(&res, &a, b.v[0]); break;
                case DOT:   res.v[0] = vec4_dot(&a, &b); break;
                case LEN:   res.v[0] = vec4_length(&a); break;
                case LEN2:  res.v[0] = vec4_length2(&a); break;
                case NORM:  vec4_normalize(&res, &a); break;
                case CROSS: DMSG("FAIL: test %d: CROSS(4) invalid", i);
                            failed = 1; break;
            }
            if (testlist[i].allow_approximate
             && CLOSE_ENOUGH(res.v[0], testlist[i].res[0])
             && CLOSE_ENOUGH(res.v[1], testlist[i].res[1])
             && CLOSE_ENOUGH(res.v[2], testlist[i].res[2])
             && CLOSE_ENOUGH(res.v[3], testlist[i].res[3])
            ) {
                continue;
            }
            if (res.v[0] != testlist[i].res[0]
             || res.v[1] != testlist[i].res[1]
             || res.v[2] != testlist[i].res[2]
             || res.v[3] != testlist[i].res[3]
            ) {
                DMSG("FAIL: test %d: result <%f,%f,%f,%f> != expect <%f,%f,"
                     "%f,%f>", i, res.v[0], res.v[1], res.v[2], res.v[3],
                     testlist[i].res[0], testlist[i].res[1],
                     testlist[i].res[2], testlist[i].res[3]);
                failed = 1;
            }
        } else {
            DMSG("FAIL: test %d: bad vector size %d", i, testlist[i].size);
            failed = 1;
        }
    }

    for (i = 0; i < lenof(testlist_cross4); i++) {
        Vector4f a, b, c, res;
        a.v[0] = testlist_cross4[i].a[0];
        a.v[1] = testlist_cross4[i].a[1];
        a.v[2] = testlist_cross4[i].a[2];
        a.v[3] = testlist_cross4[i].a[3];
        b.v[0] = testlist_cross4[i].b[0];
        b.v[1] = testlist_cross4[i].b[1];
        b.v[2] = testlist_cross4[i].b[2];
        b.v[3] = testlist_cross4[i].b[3];
        c.v[0] = testlist_cross4[i].c[0];
        c.v[1] = testlist_cross4[i].c[1];
        c.v[2] = testlist_cross4[i].c[2];
        c.v[3] = testlist_cross4[i].c[3];
        res.v[0] = res.v[1] = res.v[2] = res.v[3] = 0;
        vec4_cross(&res, &a, &b, &c);
        if (res.v[0] != testlist_cross4[i].res[0]
         || res.v[1] != testlist_cross4[i].res[1]
         || res.v[2] != testlist_cross4[i].res[2]
         || res.v[3] != testlist_cross4[i].res[3]
        ) {
            DMSG("FAIL: test %d: result <%f,%f,%f,%f> != expect <%f,%f,"
                 "%f,%f>", i, res.v[0], res.v[1], res.v[2], res.v[3],
                 testlist_cross4[i].res[0], testlist_cross4[i].res[1],
                 testlist_cross4[i].res[2], testlist_cross4[i].res[3]);
            failed = 1;
        }
    }

    for (i = 0; i < lenof(testlist_xform); i++) {
        Vector3f coord, res;
        // Static because of http://gcc.gnu.org/bugzilla/show_bug.cgi?id=16660
        static Matrix4f m;
        coord.x = testlist_xform[i].coord[0];
        coord.y = testlist_xform[i].coord[1];
        coord.z = testlist_xform[i].coord[2];
        m._11 = testlist_xform[i].m1[0];
        m._12 = testlist_xform[i].m1[1];
        m._13 = testlist_xform[i].m1[2];
        m._14 = testlist_xform[i].m1[3];
        m._21 = testlist_xform[i].m2[0];
        m._22 = testlist_xform[i].m2[1];
        m._23 = testlist_xform[i].m2[2];
        m._24 = testlist_xform[i].m2[3];
        m._31 = testlist_xform[i].m3[0];
        m._32 = testlist_xform[i].m3[1];
        m._33 = testlist_xform[i].m3[2];
        m._34 = testlist_xform[i].m3[3];
        m._41 = testlist_xform[i].m4[0];
        m._42 = testlist_xform[i].m4[1];
        m._43 = testlist_xform[i].m4[2];
        m._44 = testlist_xform[i].m4[3];
        vec3_transform(&res, &coord, &m);
        if (res.x != testlist_xform[i].res[0]
         || res.y != testlist_xform[i].res[1]
         || res.z != testlist_xform[i].res[2]
        ) {
            DMSG("FAIL: test %d: result <%f,%f,%f> != expect <%f,%f,%f>",
                 i, res.v[0], res.v[1], res.v[2],
                 testlist_xform[i].res[0], testlist_xform[i].res[1],
                 testlist_xform[i].res[2]);
            failed = 1;
        }
    }

    return !failed;
}

/*************************************************************************/

static int check_matrix_error(const Matrix4f *result, const Matrix4f *expect,
                              const char *errmsg, ...);

/**
 * test_matrix:  行列関数をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
int test_matrix(void)
{
    static const struct {
        enum {ADD, SUB, MUL, INV, TRANS} type;
        int size;
        Matrix4f a, b, expect;
    } testlist[] = {

        /* 加算 */
        {ADD,   4, {{{1,2,3,4}, {5,6,7,8}, {9,10,11,12}, {13,14,15,16}}},
                   {{{5.5,7.5,9.5,11.5}, {13.5,15.5,17.5,19.5},
                     {21.5,23.5,25.5,27.5}, {29.5,31.5,33.5,35.5}}},
                   {{{6.5,9.5,12.5,15.5}, {18.5,21.5,24.5,27.5},
                     {30.5,33.5,36.5,39.5}, {42.5,45.5,48.5,51.5}}}},
        {ADD,   4, {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                    {{{TINY,TINY,TINY,TINY}, {TINY,TINY,TINY,TINY},
                      {TINY,TINY,TINY,TINY}, {TINY,TINY,TINY,TINY}}},
                    {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}}},

        /* 減算 */
        {SUB,   4, {{{1,2,3,4}, {5,6,7,8}, {9,10,11,12}, {13,14,15,16}}},
                   {{{5.5,7.5,9.5,11.5}, {13.5,15.5,17.5,19.5},
                     {21.5,23.5,25.5,27.5}, {29.5,31.5,33.5,35.5}}},
                   {{{-4.5,-5.5,-6.5,-7.5}, {-8.5,-9.5,-10.5,-11.5},
                     {-12.5,-13.5,-14.5,-15.5}, {-16.5,-17.5,-18.5,-19.5}}}},
        {SUB,   4, {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{TINY/2,TINY/2,TINY/2,TINY/2},
                     {TINY/2,TINY/2,TINY/2,TINY/2},
                     {TINY/2,TINY/2,TINY/2,TINY/2},
                     {TINY/2,TINY/2,TINY/2,TINY/2}}},
                   {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}}},

        /* 乗算・基本テスト */
        {MUL,   4, {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}}},
        {MUL,   4, {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}}},

        /* 乗算・成分毎テスト */
        {MUL,   4, {{{1,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{2,3,4,5}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{2,3,4,5}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}}},
        {MUL,   4, {{{0,1,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{1,1,1,1}, {2,3,4,5}, {1,1,1,1}, {1,1,1,1}}},
                   {{{2,3,4,5}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,1,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{1,1,1,1}, {1,1,1,1}, {2,3,4,5}, {1,1,1,1}}},
                   {{{2,3,4,5}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,1}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {2,3,4,5}}},
                   {{{2,3,4,5}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,0}, {1,0,0,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{2,3,4,5}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{0,0,0,0}, {2,3,4,5}, {0,0,0,0}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,0}, {0,1,0,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{1,1,1,1}, {2,3,4,5}, {1,1,1,1}, {1,1,1,1}}},
                   {{{0,0,0,0}, {2,3,4,5}, {0,0,0,0}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,1,0}, {0,0,0,0}, {0,0,0,0}}},
                   {{{1,1,1,1}, {1,1,1,1}, {2,3,4,5}, {1,1,1,1}}},
                   {{{0,0,0,0}, {2,3,4,5}, {0,0,0,0}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,0,1}, {0,0,0,0}, {0,0,0,0}}},
                   {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {2,3,4,5}}},
                   {{{0,0,0,0}, {2,3,4,5}, {0,0,0,0}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,0,0}, {1,0,0,0}, {0,0,0,0}}},
                   {{{2,3,4,5}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{0,0,0,0}, {0,0,0,0}, {2,3,4,5}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,0,0}, {0,1,0,0}, {0,0,0,0}}},
                   {{{1,1,1,1}, {2,3,4,5}, {1,1,1,1}, {1,1,1,1}}},
                   {{{0,0,0,0}, {0,0,0,0}, {2,3,4,5}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,0,0}, {0,0,1,0}, {0,0,0,0}}},
                   {{{1,1,1,1}, {1,1,1,1}, {2,3,4,5}, {1,1,1,1}}},
                   {{{0,0,0,0}, {0,0,0,0}, {2,3,4,5}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,0,0}, {0,0,0,1}, {0,0,0,0}}},
                   {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {2,3,4,5}}},
                   {{{0,0,0,0}, {0,0,0,0}, {2,3,4,5}, {0,0,0,0}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {1,0,0,0}}},
                   {{{2,3,4,5}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {2,3,4,5}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,1,0,0}}},
                   {{{1,1,1,1}, {2,3,4,5}, {1,1,1,1}, {1,1,1,1}}},
                   {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {2,3,4,5}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,1,0}}},
                   {{{1,1,1,1}, {1,1,1,1}, {2,3,4,5}, {1,1,1,1}}},
                   {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {2,3,4,5}}}},
        {MUL,   4, {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,1}}},
                   {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {2,3,4,5}}},
                   {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {2,3,4,5}}}},

        /* 乗算・単位行列テスト */
        {MUL,   4, {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}},
                   {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}},
                   {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}}},
        {MUL,   4, {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}},
                   {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}}},
        {MUL,   4, {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}},
                   {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}}},

        /* 乗算・一般テスト */
        {MUL,   4, {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1}}},
                   {{{4,4,4,4}, {4,4,4,4}, {4,4,4,4}, {4,4,4,4}}}},
        {MUL,   4, {{{1,3,5,7}, {9,11,13,15}, {17,19,21,23}, {25,27,29,31}}},
                   {{{2,4,6,8}, {10,12,14,16}, {18,20,22,24}, {26,28,30,32}}},
                   {{{304,336,368,400}, {752,848,944,1040},
                     {1200,1360,1520,1680}, {1648,1872,2096,2320}}}},
        {MUL,   4, {{{1,3,5,7}, {9,11,13,15}, {17,19,21,23}, {25,27,29,31}}},
                   {{{2.5,4.5,6.5,8.5}, {10.5,12.5,14.5,16.5},
                     {18.5,20.5,22.5,24.5}, {26.5,28.5,30.5,32.5}}},
                   {{{312,344,376,408}, {776,872,968,1064},
                     {1240,1400,1560,1720}, {1704,1928,2152,2376}}}},

        /* 逆行列・単位行列 */
        {INV,   4, {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}}, {},
                   {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}}},

        /* 逆行列・特異（非可逆）行列 */
        {INV,   4, {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}}, {{{1}}},{}},
        {INV,   4, {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {2,2,2,0}}}, {{{1}}},{}},
        {INV,   4, {{{1,0,0,0}, {0,1,0,0}, {2,2,0,2}, {0,0,0,1}}}, {{{1}}},{}},
        {INV,   4, {{{1,0,0,0}, {2,0,2,2}, {0,0,1,0}, {0,0,0,1}}}, {{{1}}},{}},
        {INV,   4, {{{0,2,2,2}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}}, {{{1}}},{}},

        /* 逆行列・一般 */
        {INV,   4, {{{2,0,0,0}, {0,4,0,0}, {0,0,8,0}, {-6,-20,-56,1}}}, {},
                   {{{0.5,0,0,0}, {0,0.25,0,0}, {0,0,0.125,0}, {3,5,7,1}}}},

        /* 転置 */
        {TRANS, 4, {{{1,2,3,4}, {5,6,7,8}, {9,10,11,12}, {13,14,15,16}}}, {},
                   {{{1,5,9,13}, {2,6,10,14}, {3,7,11,15}, {4,8,12,16}}}},
    };


    int failed = 0;

    int i;
    for (i = 0; i < lenof(testlist); i++) {
        if (testlist[i].size == 4) {
            static Matrix4f res;

            /* 逆行列の失敗テストは特殊処理 */
            if (testlist[i].type == INV && testlist[i].b.a[0] != 0) {
                if (mat4_inv(&res, &testlist[i].a)) {
                    DMSG("FAIL: test %d: inverted a non-invertible matrix", i);
                    failed = 1;
                }
                continue;  // 通常処理を飛ばす
            }

            /* 通常演算をテスト */
            mem_clear(&res, sizeof(res));
            switch (testlist[i].type) {
                case ADD:   mat4_add(&res, &testlist[i].a, &testlist[i].b);
                            break;
                case SUB:   mat4_sub(&res, &testlist[i].a, &testlist[i].b);
                            break;
                case MUL:   mat4_mul(&res, &testlist[i].a, &testlist[i].b);
                            break;
                case INV:   mat4_inv(&res, &testlist[i].a); break;
                case TRANS: mat4_transpose(&res, &testlist[i].a); break;
            }
            if (!check_matrix_error(&res, &testlist[i].expect,
                                    "test %d: result != expect", i)) {
                failed = 1;
                continue;  // そもそも失敗なので、dest==srcテストを飛ばす
            }

            /* dest==src1でテスト */
            res = testlist[i].a;
            switch (testlist[i].type) {
                case ADD:   mat4_add(&res, &res, &testlist[i].b); break;
                case SUB:   mat4_sub(&res, &res, &testlist[i].b); break;
                case MUL:   mat4_mul(&res, &res, &testlist[i].b); break;
                case INV:   mat4_inv(&res, &res); break;
                case TRANS: mat4_transpose(&res, &res); break;
            }
            if (!check_matrix_error(&res, &testlist[i].expect,
                                    "test %d: fail on dest == src1", i)) {
                failed = 1;
            }

            /* dest==src2でテスト */
            res = testlist[i].b;
            switch (testlist[i].type) {
                case ADD:   mat4_add(&res, &testlist[i].a, &res); break;
                case SUB:   mat4_sub(&res, &testlist[i].a, &res); break;
                case MUL:   mat4_mul(&res, &testlist[i].a, &res); break;
                case INV:   goto skip_src2;  // 引数が一つしかない
                case TRANS: goto skip_src2;  // 引数が一つしかない
            }
            if (!check_matrix_error(&res, &testlist[i].expect,
                                    "test %d: fail on dest == src2", i)) {
                failed = 1;
            }
          skip_src2:;

        } else {
            DMSG("FAIL: test %d: bad matrix size %d", i, testlist[i].size);
            failed = 1;
        }
    }

    return !failed;
}

/*-----------------------------------------------------------------------*/

/**
 * check_matrix_error:  行列テストが成功した稼動かを確認し、失敗した場合は
 * エラーメッセージを出力する。
 *
 * 【引　数】result: 演算で実際に得られた行列
 * 　　　　　expect: 期待される行列
 * 　　　　　errmsg: エラーメッセージ（printf()指定子有効）
 * 　　　　　   ...: errmsg生成に使うパラメータ
 * 【戻り値】0以外＝テスト成功、0＝テスト失敗
 */
static int check_matrix_error(const Matrix4f *result, const Matrix4f *expect,
                              const char *errmsg, ...)
{
    PRECOND(result != NULL);
    PRECOND(expect != NULL);
    PRECOND(errmsg != NULL);

    int i;
    for (i = 0; i < 16; i++) {
        if (result->a[i] != expect->a[i]) {
            goto failed;
        }
    }
    return 1;  // 成功

  failed:;
    char msgbuf[1000];
    va_list args;
    va_start(args, errmsg);
    vsnprintf(msgbuf, sizeof(msgbuf), errmsg, args);
    va_end(args);

    DMSG("FAIL: %s"
         "\n   result: [%6.3f,%6.3f,%6.3f,%6.3f]  expect: [%6.3f,%6.3f,%6.3f,%6.3f]"
         "\n           [%6.3f,%6.3f,%6.3f,%6.3f]          [%6.3f,%6.3f,%6.3f,%6.3f]"
         "\n           [%6.3f,%6.3f,%6.3f,%6.3f]          [%6.3f,%6.3f,%6.3f,%6.3f]"
         "\n           [%6.3f,%6.3f,%6.3f,%6.3f]          [%6.3f,%6.3f,%6.3f,%6.3f]",
         msgbuf,
         result->_11, result->_12, result->_13, result->_14,
         expect->_11, expect->_12, expect->_13, expect->_14,
         result->_21, result->_22, result->_23, result->_24,
         expect->_21, expect->_22, expect->_23, expect->_24,
         result->_31, result->_32, result->_33, result->_34,
         expect->_31, expect->_32, expect->_33, expect->_34,
         result->_41, result->_42, result->_43, result->_44,
         expect->_41, expect->_42, expect->_43, expect->_44);

    return 0;  // 失敗
}

/*************************************************************************/

/**
 * test_intersect:  2直線交差関数をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
int test_intersect(void)
{
    static const struct {
        Vector2f P1, v1, P2, v2;
        int succeed;
        float t1, t2;
        int allow_approximate;
    } testlist[] = {
        {{{0,0}},{{1,0}}, {{1,1}},{{0,1}}, 1, 1, -1},  // 基本テスト
        {{{0,0}},{{1,0}}, {{1,-1}},{{0,1}}, 1, 1, 1},
        {{{0,0}},{{1,0}}, {{-1,1}},{{0,1}}, 1, -1, -1},
        {{{0,0}},{{1,0}}, {{-1,-1}},{{0,1}}, 1, -1, 1},
        {{{0,0}},{{1,0}}, {{1,1}},{{0,-1}}, 1, 1, 1},  // 逆方向ベクトル
        {{{0,0}},{{1,0}}, {{1,-1}},{{0,-1}}, 1, 1, -1},
        {{{0,0}},{{1,0}}, {{-1,1}},{{0,-1}}, 1, -1, 1},
        {{{0,0}},{{1,0}}, {{-1,-1}},{{0,-1}}, 1, -1, -1},
        {{{0,0}},{{-1,0}}, {{1,1}},{{0,1}}, 1, -1, -1},
        {{{0,0}},{{-1,0}}, {{1,-1}},{{0,1}}, 1, -1, 1},
        {{{0,0}},{{-1,0}}, {{-1,1}},{{0,1}}, 1, 1, -1},
        {{{0,0}},{{-1,0}}, {{-1,-1}},{{0,1}}, 1, 1, 1},
        {{{0,0}},{{-1,0}}, {{1,1}},{{0,-1}}, 1, -1, 1},
        {{{0,0}},{{-1,0}}, {{1,-1}},{{0,-1}}, 1, -1, -1},
        {{{0,0}},{{-1,0}}, {{-1,1}},{{0,-1}}, 1, 1, 1},
        {{{0,0}},{{-1,0}}, {{-1,-1}},{{0,-1}}, 1, 1, -1},
        {{{0,0}},{{2,0}}, {{1,1}},{{0,2}}, 1, 1, -1},  // 非正規化ベクトル
        {{{0,0}},{{2,0}}, {{1,-1}},{{0,2}}, 1, 1, 1},
        {{{0,0}},{{2,0}}, {{-1,1}},{{0,2}}, 1, -1, -1},
        {{{0,0}},{{2,0}}, {{-1,-1}},{{0,2}}, 1, -1, 1},
        {{{0,0}},{{1,0}}, {{1,1}},{{1,0}}, 0},  // 平行線
        {{{0,0}},{{0,1}}, {{1,1}},{{0,1}}, 0},
        {{{0,0}},{{1,1}}, {{1,0}},{{1,1}}, 0},
        {{{0,0}},{{1,1}}, {{1,1}},{{1,1}}, 0},  // 同一線
        {{{0,0}},{{1,0}}, {{1,1}},{{0,0}}, 0},  // ゼロベクトル
        {{{0,0}},{{0,0}}, {{1,1}},{{0,1}}, 0},
        {{{0,0}},{{1,0}}, {{0,-4}},{{3,4}}, 1, 3, 5, 1},  // 斜め
        {{{2,3}},{{1,0}}, {{2,-1}},{{3,4}}, 1, 3, 5, 1},  // 原点を含めない
    };

    int failed = 0;

    int i;
    for (i = 0; i < lenof(testlist); i++) {
        Vector2f v1_norm, v2_norm;
        vec2_normalize(&v1_norm, &testlist[i].v1);
        vec2_normalize(&v2_norm, &testlist[i].v2);
        float t1 = 0, t2 = 0;
        int res = intersect_lines(&testlist[i].P1, &v1_norm,
                                  &testlist[i].P2, &v2_norm, &t1, &t2);
        if ((res && !testlist[i].succeed)
         || (!res && testlist[i].succeed)
         || (res && (testlist[i].allow_approximate
                     ? !CLOSE_ENOUGH(t1, testlist[i].t1)
                       || !CLOSE_ENOUGH(t2, testlist[i].t2)
                     : t1 != testlist[i].t1 || t2 != testlist[i].t2))
        ) {
            DMSG("FAIL: test %d: got %d %f %f expect %d %f %f", i, res, t1, t2,
                 testlist[i].succeed, testlist[i].t1, testlist[i].t2);
            failed = 1;
        }
    }

    return !failed;
}

/*************************************************************************/
/*************************************************************************/

#endif  // INCLUDE_TESTS

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
