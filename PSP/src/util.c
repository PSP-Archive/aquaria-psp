/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/util.c: Miscellaneous utility routines.
 */

/*
 * プログラム全体で頻繁に使われるユーティリティ関数をここで定義する。他の
 * ソースファイルと違って、専用のヘッダはなく、関数宣言はcommon.hに記述され
 * ている。
 */

#include "common.h"
#include "sysdep.h"
#include "vector.h"

/*************************************************************************/

/**
 * VSNPRINTF_USE_FLOATS:  定義時、vsnprintf()の「%f」「%g」指定子処理に単精
 * 度処理を使う（通常は標準ライブラリ同様に倍精度）。PSPのCPUでは倍精度演算
 * ができないのと、実際には（デバッグを除いて）倍精度で値を表示する必要がな
 * いと思われるので、重い倍精度処理を省く。
 */
#if defined(PSP)
# define VSNPRINTF_USE_FLOATS
#endif

/*************************************************************************/
/**************************** メモリ操作関数 *****************************/
/*************************************************************************/

#ifndef mem_clear  // マクロとして定義されている場合は関数を定義しない

/**
 * mem_clear:  メモリ領域をゼロにクリアする。
 *
 * 【引　数】ptr: クリアするメモリ領域へのポインタ
 * 　　　　　len: クリアするバイト数
 * 【戻り値】なし
 */
void mem_clear(void *ptr, uint32_t len)
{
    if (UNLIKELY(!ptr)) {
        DMSG("ptr == NULL");
        return;
    }
    if (UNLIKELY(!len)) {
        DMSG("len == 0");
        return;
    }
    if ((((uintptr_t)ptr | (uintptr_t)len) & 3) == 0) {
        sys_mem_fill32(ptr, 0, len);
    } else if (len >= 16 && ((uintptr_t)ptr & 3) == 0) {
        sys_mem_fill32(ptr, 0, len & ~3);
        sys_mem_fill8((uint8_t *)ptr + (len & ~3), 0, len & 3);
    } else {
        sys_mem_fill8(ptr, 0, len);
    }
}

#endif

/*************************************************************************/

/**
 * mem_fill8:  メモリ領域を指定された8ビット値でフィルする。
 *
 * 【引　数】ptr: フィルするメモリ領域へのポインタ
 * 　　　　　val: フィル値
 * 　　　　　len: フィルするバイト数
 * 【戻り値】なし
 */
void mem_fill8(void *ptr, uint8_t val, uint32_t len)
{
    if (UNLIKELY(!ptr)) {
        DMSG("ptr == NULL");
        return;
    }
    if (UNLIKELY(!len)) {
        DMSG("len == 0");
        return;
    }
    if ((((uintptr_t)ptr | (uintptr_t)len) & 3) == 0) {
        sys_mem_fill32(ptr, val<<24 | val<<16 | val<<8 | val, len);
    } else if (len >= 16 && ((uintptr_t)ptr & 3) == 0) {
        sys_mem_fill32(ptr, val<<24 | val<<16 | val<<8 | val, len & ~3);
        sys_mem_fill8((uint8_t *)ptr + (len & ~3), val, len & 3);
    } else {
        sys_mem_fill8(ptr, val, len);
    }
}

/*************************************************************************/

/**
 * mem_fill32:  メモリ領域を指定された32ビット値でフィルする。
 *
 * 【引　数】ptr: フィルするメモリ領域へのポインタ
 * 　　　　　val: フィル値
 * 　　　　　len: フィルするバイト数
 * 【戻り値】なし
 */
void mem_fill32(void *ptr, uint32_t val, uint32_t len)
{
    if (UNLIKELY(!ptr)) {
        DMSG("ptr == NULL");
        return;
    }
    if (UNLIKELY(!len)) {
        DMSG("len == 0");
        return;
    }
    if ((((uintptr_t)ptr | (uintptr_t)len) & 3) == 0) {
        sys_mem_fill32(ptr, val, len);
    } else {
        DMSG("WARNING: unaligned fill32(%p,%08X,%d)", ptr, val, len);
        uint32_t i;
        for (i = 0; i < len; i++) {
            ((uint8_t *)ptr)[i] = ((uint8_t *)&val)[i%4];
        }
    }
}

/*************************************************************************/
/************************* エンディアン操作関数 **************************/
/*************************************************************************/

#ifndef is_little_endian  // マクロとして定義されている場合は関数を定義しない

/**
 * is_little_endian:  実行環境のエンディアン性がリトルエンディアンかどうかを
 * 返す。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝リトルエンディアン環境、0＝ビッグエンディアン環境
 * 【注　意】プログラム全体を通して、ビッグエンディアン・リトルエンディアン
 * 　　　　　以外の環境には対応していない。
 */
int is_little_endian(void)
{
    union {char c[4]; uint32_t i;} intbuf = {.c = {0x12,0x34,0x56,0x78}};
    const uint32_t val = intbuf.i;
    PRECOND(val == 0x12345678 || val == 0x78563412);
    return val == 0x78563412;
}

#endif  // defined(is_little_endian)

/*************************************************************************/

/*
 * 注：実際のデータ操作は最適化のため、common.hにてインライン関数として定義
 * 　　されている。
 */

/*************************************************************************/
/***************************** 角度関連関数 ******************************/
/*************************************************************************/

#ifndef CUSTOM_DTRIG_FUNCTIONS

/* 度単位三角関数のルックアップテーブル。正弦・正接の値を15度ずつ記録 */

static const float dsinf_table[24] = {
    0.0, 0.258819045102521, 0.5,
    0.707106781186548, 0.866025403784439, 0.965925826289068,
    1.0, 0.965925826289068, 0.866025403784439,
    0.707106781186548, 0.5, 0.258819045102521,
    0.0, -0.258819045102521, -0.5,
    -0.707106781186548, -0.866025403784439, -0.965925826289068,
    -1.0, -0.965925826289068, -0.866025403784439,
    -0.707106781186548, -0.5, -0.258819045102521,
};
static const float dtanf_table[12] = {
    0.0, 0.267949192431123, 0.577350269189626,
    1.0, 1.73205080756888, 3.73205080756888,
    1.0/0.0, -3.73205080756888, -1.73205080756888,
    -1.0, -0.577350269189626, -0.267949192431123,
};

static const double dsin_table[24] = {
    0.0, 0.258819045102521, 0.5,
    0.707106781186548, 0.866025403784439, 0.965925826289068,
    1.0, 0.965925826289068, 0.866025403784439,
    0.707106781186548, 0.5, 0.258819045102521,
    0.0, -0.258819045102521, -0.5,
    -0.707106781186548, -0.866025403784439, -0.965925826289068,
    -1.0, -0.965925826289068, -0.866025403784439,
    -0.707106781186548, -0.5, -0.258819045102521,
};
static const double dtan_table[12] = {
    0.0, 0.267949192431123, 0.577350269189626,
    1.0, 1.73205080756888, 3.73205080756888,
    1.0/0.0, -3.73205080756888, -1.73205080756888,
    -1.0, -0.577350269189626, -0.267949192431123,
};

/*************************************************************************/

/**
 * dsinf, dcosf, dtanf, dsincosf, datan2f:  度単位の単精度三角関数。
 * datan2f(0,0)はエラーにはならず、0を返す。
 *
 * 【引　数】  angle: 角度（度単位）
 * 　　　　　sin_ret: 角度の正弦を格納する変数へのポインタ（dsincosf()のみ）
 * 　　　　　cos_ret: 角度の余弦を格納する変数へのポインタ（dsincosf()のみ）
 * 　　　　　   y, x: datan2f()における正接のY・X成分
 * 【戻り値】各三角関数の結果。datan2f()の結果は[0,360)の範囲内
 */

CONST_FUNCTION float dsinf(const float angle)
{
    const float angle_15 = floorf(angle / 15);
    if (angle == 15 * angle_15) {
        /* int型をオーバーフローするような値では、そもそも正確性が失われて
         * いるので、fmodf()を回避して直接intに変換する */
        int index = (int)angle_15 % 24;
        if (index < 0) {
            index += 24;
        }
        return dsinf_table[index];
    }
    return sinf(angle * (M_PIf/180));
}

CONST_FUNCTION float dcosf(const float angle)
{
    const float angle_15 = floorf(angle / 15);
    if (angle == 15 * angle_15) {
        int index = (int)angle_15 % 24;
        if (index < 0) {
            index += 24;
        }
        return dsinf_table[(index+6) % 24];
    }
    return cosf(angle * (M_PIf/180));
}

CONST_FUNCTION float dtanf(const float angle)
{
    const float angle_15 = floorf(angle / 15);
    if (angle == 15 * angle_15) {
        int index = (int)angle_15 % 12;
        if (index < 0) {
            index += 12;
        }
        return dtanf_table[index];
    }
    return tanf(angle * (M_PIf/180));
}

void dsincosf(const float angle, float *sin_ret, float *cos_ret)
{
    const float angle_15 = floorf(angle / 15);
    if (angle == 15 * angle_15) {
        int index = (int)angle_15 % 24;
        if (index < 0) {
            index += 24;
        }
        *sin_ret = dsinf_table[index];
        *cos_ret = dsinf_table[(index+6) % 24];
        return;
    }
    const float sin_val = sinf(angle * (M_PIf/180));
    const float cos_val = sqrtf(1 - (sin_val*sin_val));
    *sin_ret = sin_val;
    const int test = ifloorf(fmodf(fabsf(angle), 360));
    *cos_ret = (test >= 90 && test < 270) ? -cos_val : cos_val;
}

CONST_FUNCTION float datan2f(const float y, const float x)
{
    if (y == 0) {
        return (x < 0) ? 180 : 0;
    } else if (x == 0) {
        return (y < 0) ? 270 : 90;
    } else if (x == y) {
        return (x < 0) ? 225 : 45;
    } else if (x == -y) {
        return (x < 0) ? 135 : 315;
    } else {
        const float angle = atan2f(y, x) * (180/M_PIf);
        return angle < 0 ? angle+360 : angle;
    }
}

/*************************************************************************/

# ifdef USE_DOUBLE_DTRIG

/**
 * dsin, dcos, dtan, dsincos, datan2:  度単位の倍精度三角関数。
 * datan2(0,0)はエラーにはならず、0を返す。
 *
 * 【引　数】  angle: 角度（度単位）
 * 　　　　　sin_ret: 角度の正弦を格納する変数へのポインタ（dsincos()のみ）
 * 　　　　　cos_ret: 角度の余弦を格納する変数へのポインタ（dsincos()のみ）
 * 　　　　　   y, x: datan2()における正接のY・X成分
 * 【戻り値】各三角関数の結果。datan2()の結果は[0,360)の範囲内
 */

CONST_FUNCTION double dsin(const double angle)
{
    const double angle_15 = floor(angle / 15);
    if (angle == 15 * angle_15) {
        int index = (int)fmod(angle_15, 24);
        if (index < 0) {
            index += 24;
        }
        return dsin_table[index];
    }
    return sin(angle * (M_PI/180));
}

CONST_FUNCTION double dcos(const double angle)
{
    const double angle_15 = floor(angle / 15);
    if (angle == 15 * angle_15) {
        int index = (int)fmod(angle_15, 24);
        if (index < 0) {
            index += 24;
        }
        return dsin_table[(index+6) % 24];
    }
    return cos(angle * (M_PI/180));
}

CONST_FUNCTION double dtan(const double angle)
{
    const double angle_15 = floor(angle / 15);
    if (angle == 15 * angle_15) {
        int index = (int)fmod(angle_15, 12);
        if (index < 0) {
            index += 12;
        }
        return dtan_table[index];
    }
    return tan(angle * (M_PI/180));
}

void dsincos(const double angle, double *sin_ret, double *cos_ret)
{
    const double angle_15 = floor(angle / 15);
    if (angle == 15 * angle_15) {
        int index = (int)fmod(angle_15, 24);
        if (index < 0) {
            index += 24;
        }
        *sin_ret = dsin_table[index];
        *cos_ret = dsin_table[(index+6) % 24];
        return;
    }
    const double sin_val = sin(angle * (M_PIf/180));
    const double cos_val = sqrt(1 - (sin_val*sin_val));
    *sin_ret = sin_val;
    const int test = ifloor(fmod(fabs(angle), 360));
    *cos_ret = (test >= 90 && test < 270) ? -cos_val : cos_val;
}

CONST_FUNCTION double datan2(const double y, const double x)
{
    if (y == 0) {
        return (x < 0) ? 180 : 0;
    } else if (x == 0) {
        return (y < 0) ? 270 : 90;
    } else if (x == y) {
        return (x < 0) ? 225 : 45;
    } else if (x == -y) {
        return (x < 0) ? 135 : 315;
    } else {
        const double angle = atan2(y, x) * (180/M_PI);
        return angle < 0 ? angle+360 : angle;
    }
}

# endif  // USE_DOUBLE_DTRIG

#endif  // !CUSTOM_DTRIG_FUNCTIONS

/*************************************************************************/

/**
 * anglediff:  2つの角度の差を返す。
 *
 * 【引　数】angle1: 1つ目の角度（度単位）
 * 　　　　　angle2: 2つ目の角度（度単位）
 * 【戻り値】angle1 - angle2（度単位、[-180,+180)の範囲内）
 */
CONST_FUNCTION float anglediff(const float angle1, const float angle2)
{
    const float diff = fmodf(angle1 - angle2, 360);
    if (diff < -180) {
        return diff + 360;
    } else if (diff >= 180) {
        return diff - 360;
    } else {
        return diff;
    }
}

/*************************************************************************/
/****************************** 幾何学関数 *******************************/
/*************************************************************************/

/**
 * intersect_lines:  2つの直線の交差点を求め、交差点の各直線における位置
 * （方向ベクトルを1としたパラメータ化座標）を返す。
 *
 * 【引　数】P1, v1: 直線1の基点と方向ベクトルへのポインタ
 * 　　　　　P2, v2: 直線2の基点と方向ベクトルへのポインタ
 * 　　　　　t1_ret: 直線1における交差点の位置を格納する変数へのポインタ
 * 　　　　　t2_ret: 直線2における交差点の位置を格納する変数へのポインタ
 * 【戻り値】0以外＝成功、0＝失敗（直線が平行か、v1またはv2がゼロベクトル）
 * 【注　意】・v1, v2は予め正規化しなければならない。
 * 　　　　　・両方の直線が重なる（同一の直線である）場合、幾何学における定義
 * 　　　　　　　とは違って「交差しない」（0）を返す。
 * 　　　　　・t1_ret, t2_retの各値が不要な場合、該当引数にNULLを渡してもよい。
 */
int intersect_lines(const Vector2f *P1, const Vector2f *v1,
                    const Vector2f *P2, const Vector2f *v2,
                    float *t1_ret, float *t2_ret)
{
    PRECOND_SOFT(P1 != NULL, return 0);
    PRECOND_SOFT(v1 != NULL, return 0);
    PRECOND_SOFT(P2 != NULL, return 0);
    PRECOND_SOFT(v2 != NULL, return 0);

    const float det = (v1->x * v2->y) - (v1->y * v2->x);
    if (fabsf(det) < 0.000001f) {
        return 0;
    }
    if (t1_ret) {
        *t1_ret = (v2->y * (P2->x - P1->x) + v2->x * (P1->y - P2->y)) / det;
    }
    if (t2_ret) {
        *t2_ret = (v1->y * (P2->x - P1->x) + v1->x * (P1->y - P2->y)) / det;
    }
    return 1;

    /* 上記の計算は以下のように導かれた。 */

    // L1 = (x1,y1) + t(p1,q1)
    // L2 = (x2,y2) + u(p2,q2)
    // x1 + tp1 = x2 + up2    y1 + tq1 = y2 + uq2

    // x1 + tp1 = x2 + up2
    // tp1 = (x2 - x1) + up2
    // t = (1/p1)(x2 - x1) + (p2/p1)u
    // y1 + ((1/p1)(x2 - x1) + (p2/p1)u)q1 = y2 + uq2
    // y1 + (q1/p1)(x2 - x1) + (p2q1/p1)u = y2 + uq2
    // (y1 - y2) + (q1/p1)(x2 - x1) + (p2q1/p1)u = uq2
    // (1/q2)(y1 - y2) + (q1/p1q2)(x2 - x1) + (p2q1/p1q2)u = u
    // (1/q2)(y1 - y2) + (q1/p1q2)(x2 - x1) = (1 - p2q1/p1q2)u
    // (1/q2)(y1 - y2) + (q1/p1q2)(x2 - x1) = (p1q2/p1q2 - p2q1/p1q2)u
    // (1/q2)(y1 - y2) + (q1/p1q2)(x2 - x1) = ((p1q2 - p2q1)/p1q2)u
    // u = (p1q2/(p1q2 - p2q1)) . ((1/q2)(y1 - y2) + (q1/p1q2)(x2 - x1))
    //   = 1/(p1q2 - p2q1) . p1q2((p1/p1q2)(y1 - y2) + (q1/p1q2)(x2 - x1))
    //   = (p1(y1 - y2) + q1(x2 - x1)) / (p1q2 - p2q1)
    //   = (q1(x2 - x1) + p1(y1 - y2)) / (p1q2 - p2q1)

    // y1 + tq1 = y2 + uq2
    // uq2 = (y1 - y2) + tq1
    // u = (1/q2)(y1 - y2) + (q1/q2)t
    // x1 + tp1 = x2 + ((1/q2)(y1 - y2) + (q1/q2)t)p2
    // x1 + tp1 = x2 + (p2/q2)(y1 - y2) + (p2q1/q2)t
    // tp1 = (x2 - x1) + (p2/q2)(y1 - y2) + (p2q1/q2)t
    // t = (1/p1)(x2 - x1) + (p2/p1q2)(y1 - y2) + (p2q1/p1q2)t
    // (1 - p2q1/p1q2)t = (1/p1)(x2 - x1) + (p2/p1q2)(y1 - y2)
    // (p1q2/p1q2 - p2q1/p1q2)t = (1/p1)(x2 - x1) + (p2/p1q2)(y1 - y2)
    // ((p1q2 - p2q1)/p1q2)t = (1/p1)(x2 - x1) + (p2/p1q2)(y1 - y2)
    // t = (p1q2/(p1q2 - p2q1)) . ((1/p1)(x2 - x1) + (p2/p1q2)(y1 - y2))
    //   = 1/(p1q2 - p2q1) . p1q2((1/p1)(x2 - x1) + (p2/p1q2)(y1 - y2))
    //   = (q2(x2 - x1) + p2(y1 - y2)) / (p1q2 - p2q1)
}

/*************************************************************************/
/**************************** 文字列操作関数 *****************************/
/*************************************************************************/

/* vsnprintf()用マクロ。1バイトまたは文字列を出力バッファに格納する。buf、
 * size、total変数はvsnprintf()のもの。 */

#define OUTCHAR(c)  do {  \
    const char __c = (c); \
    if (total < size) {   \
        buf[total] = __c; \
    }                     \
    total++;              \
} while (0)

#define OUTSTR(s,len)  do { \
    const char *__s = (s);  \
    const char *__top = __s + (len); \
    while (__s < __top) {   \
        OUTCHAR(*__s);      \
        __s++;              \
    }                       \
} while (0)

/**
 * snprintf, vsnprintf:  文字列をフォーマットして、結果をバッファに書き込む。
 * libcの同名関数と同様、フォーマット文字列中の、「%」に続くフォーマット指定
 * 子に従ってフォーマットデータを文字列に挿入し、結果を指定されたバッファに
 * 書き込む。以下のフォーマット指定子がサポートされている。
 * 　（フィールド幅・精度等を指定する修飾文字）
 * 　　　「0」：パディング文字として、空白の代わりに「0」を使用する。なお、
 * 　　　　　　　符号付き数値の場合でも特殊処理はしない（"%-6d",123は
 * 　　　　　　　「-00123」ではなく「00-123」となる）。「-」と併用不可。
 * 　　　「-」：データを左寄せで出力（デフォルトは右寄せ）。「0」と併用不可。
 * 　　　「+」：10進数整数において、言語設定が日本語の場合は全角数字、日本語
 * 　　　　　　　以外の場合は半角数字で出力する。パディングに空白文字を使う
 * 　　　　　　　場合、空白文字も言語に合わせて全角・半角で出力される。
 * 　　　　　　　※Aquariaでは、言語選択機能がないため現在未サポート。(FIXME)
 * 　　　「数字[.数字]」：フィールド幅と精度を指定する。「.」の前の数値は文
 * 　　　　　　　字数、後の数値は精度を指定する。数値の代わりに「*」を記述
 * 　　　　　　　すると、次のフォーマットデータ項目をint型として取得し、
 * 　　　　　　　フィールド幅または精度として使う。精度の意味は指定子に依存
 * 　　　　　　　するので、各指定子の説明を参照のこと（説明がない場合、精度
 * 　　　　　　　を無視する）。
 * 　（データサイズを指定する修飾文字）
 * 　　　「l」：整数型がintではなくlong。他のデータ型では無視（含む浮動小数点）
 * 　　　「ll」：整数型がintではなくlong long。
 * 　（基本指定子）
 * 　　　「%」：文字「%」を出力。全ての修飾子は無効。
 * 　　　「c」：文字（int型の文字コード）。0x80以上の場合、UTF-8として出力。
 * 　　　「d」「i」：符号付き整数（int、longまたはlong long型）。修飾子「+」
 * 　　　　　　　　　を使用すると、ゲームの言語が日本語に設定されている場合
 * 　　　　　　　　　のみ、全角数字を用いる。
 * 　　　「f」「g」：浮動小数点（double型）。精度は小数点以下の桁数を指定する。
 * 　　　「o」：符号無し整数（int、longまたはlong long型）を8進数で出力。
 * 　　　「p」：ポインタ値。アドレスを「0x%X」形式で出力。
 * 　　　「s」：文字列。精度は最大出力バイト数（注：文字ではなくバイト）。
 * 　　　「u」：符号無し整数（int、longまたはlong long型）を10進数で出力。
 * 　　　「x」「X」：符号無し整数（int、longまたはlong long型）を16進数で
 * 　　　　　　　　　出力。9を超える桁は、「x」の場合は小文字（abcdef）、
 * 　　　　　　　　　「X」の場合は大文字（ABCDEF）となる。
 * C99で規定されていて、サポートされていない文字は以下の通り。
 * 　（フィールド幅・精度等を指定する修飾子。無視されるがエラーにはならない）
 * 　　　「+」「 （空白文字）」「#」
 * 　　　※「+」は「d」「i」の場合、本来とは別の動作をする。
 * 　（データサイズを指定する修飾子。無視されるがエラーにはならない）
 * 　　　「hh」「h」「j」「z」「t」「L」
 * 　　　※「l」は整数型にのみ影響し、「c」「s」での使用は未サポート。
 * 　（基本指定子）
 * 　　　「a」「A」「e」「E」「F」「G」「n」
 * 　　　※「g」は「f」とほぼ同様に処理されるので（末尾の「0」が削除される
 * 　　　　だけ）、C99の規定とは動作が異なる。
 *
 * 【引　数】     buf: 出力バッファ
 * 　　　　　    size: 出力バッファのサイズ（バイト）
 * 　　　　　  format: フォーマット文字列
 * 　　　　　.../args: フォーマットデータ
 * 【戻り値】結果文字列の長さ（バイト）。size-1より大きい場合、結果の一部が
 * 　　　　　切り捨てられている。なお、切り捨ての有無にかかわらず、文字列の
 * 　　　　　末尾に終端文字'\0'が付加される。format==NULLの場合、0を返す。
 */
int32_t snprintf(char *buf, uint32_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    const int res = vsnprintf(buf, size, format, args);
    va_end(args);
    return res;
}

int32_t vsnprintf(char *buf, uint32_t size, const char *format, va_list args)
{
    int32_t total = 0;          // 結果文字列の合計バイト数


    /* 引数チェック */
    if (format == NULL) {
        DMSG("format == NULL");
        return 0;
    }
    if ((int32_t)size < 0) {
        DMSG("size (%d) < 0", (int32_t)size);
        return 0;
    }
    if (buf == NULL) {
        size = 0;  /* 書き込まないようにする */
    }

    while (*format) {

        if (*format++ != '%') {
            /* フォーマット指定子ではないので、そのまま出力 */
            OUTCHAR(format[-1]);
            continue;
        }

        const char * const start = format;  // 指定子へのポインタ（回復用）
        int left_justify = 0;    // 左寄せフラグ
        const char *pad = " ";   // パディング文字（全角可なので文字列型）
        int plus_flag = 0;       // 修飾子「+」の有無
        int width = -1;          // フィールド幅（-1：未設定）
        int prec = -1;           // 精度（-1：未設定）
        enum {M,L,LL} dsize = M; // データ型のサイズ（M：通常）
        char type = 0;           // 基本指定子（0：まだ見つかっていない）
        char tmpbuf[100];        // 数値等のフォーマット結果を格納するバッファ
        const char *data = NULL; // この指定子で出力するデータ（パディング前）
        int datalen = -1;        // dataの長さ（バイト単位）

        /* (1) フォーマット指定子を解釈する */

        while (!type) {
            const char c = *format++;
            switch (c) {

              /* (1.1) フォーマット指定子中の文字列終端に気をつける */

              case 0:
                /* フォーマット文字列の終端を検知したので、不正指定子として
                 * そのまま出力する。指定子解釈が失敗した場合でもここに飛ぶ */
              invalid_format:
                OUTCHAR('%');
                format = start;
                goto restart;  // 一番外側のループをcontinue

              /* (1.2) 「-」「0」を処理。フィールド幅指定前（width<0）のみ
               *       有効。「+」の有無も記録する。「#」と空白は飛ばすが
               *       エラーにはしない */

              case '+':
                plus_flag = 1;
                break;

              case '#':
              case ' ':
                break;

              case '-':
                if (left_justify || *pad != ' ' || width >= 0 || prec >= 0
                 || dsize != M
                ) {
                    goto invalid_format;
                }
                left_justify = 1;
                break;

              case '0':
                if (!left_justify && *pad == ' ' && width < 0 && prec < 0
                 && dsize == M
                ) {
                    pad = "0";
                    break;
                }
                /* 先頭以外の場合でも、フィールド幅・精度指定として有効なので
                 * 次ケースに続く */

              /* (1.3) フィールド幅・精度指定を処理 */

              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                if (dsize != M) {
                    /* データ型サイズ指定後の数字は無効 */
                    goto invalid_format;
                }
                if (prec >= 0) {
                    prec = (prec*10) + (c - '0');
                    if (prec < 0) {  // オーバフロー
                        prec = 10000;
                    }
                } else {
                    if (width < 0) {  // まだ設定を開始していない
                        width = 0;
                    }
                    width = (width*10) + (c - '0');
                    if (width < 0) {
                        width = 10000;
                    }
                }
                break;

              case '.':
                if (dsize != M) {
                    /* データ型サイズ指定後の数字は無効 */
                    goto invalid_format;
                }
                if (prec >= 0) {
                    /* 「.」は一回のみ有効 */
                    goto invalid_format;
                }
                prec = 0;  // 精度指定開始を記録
                break;

              case '*': {
                int val;
                if (dsize != M) {
                    /* データ型サイズ指定後の数字は無効 */
                    goto invalid_format;
                }
                if (prec > 0 || (prec < 0 && width >= 0)) {
                    /* フィールド幅・精度指定の途中でも無効 */
                    goto invalid_format;
                }
                val = va_arg(args, int);
                if (prec == 0) {
                    prec = lbound(val, 0);
                } else {
                    if (val < 0) {
                        left_justify = 1;
                        val = -val;
                    }
                    width = val;
                }
                break;
              }  // case '*'

              /* (1.4) データ型サイズ指定を処理 */

              case 'l':
                if (dsize == LL) {
                    /* 3個以上は無効 */
                    goto invalid_format;
                }
                dsize++;
                break;

              case 'h':
              case 'j':
              case 'z':
              case 't':
              case 'L':
                break;

              /* (1.5) 基本指定子を認識 */

              case '%':
              case 'c':
              case 'd':
              case 'f':
              case 'g':
              case 'i':
              case 'o':
              case 'p':
              case 's':
              case 'u':
              case 'x':
              case 'X':
                type = c;
                break;

              /* (1.6) その他は無効 */

              default:
                DMSG("Invalid format character %c", c);
                goto invalid_format;

            }  // switch (*format++)
        }  // while (!type)

        /* (2) 指定子に応じてデータを取得してフォーマットする */

        switch (type) {

          case '%':
            data = "%";
            datalen = 1;
            break;

          case 'c': {
            unsigned int val = va_arg(args, unsigned int);
            if (val >= 0x800) {
                tmpbuf[0] = 0xE0 | ((val>>12) & 0x0F);
                tmpbuf[1] = 0x80 | ((val>>6) & 0x3F);
                tmpbuf[2] = 0x80 | (val & 0x3F);
                datalen = 3;
            } else if (val >= 0x80) {
                tmpbuf[0] = 0xC0 | ((val>>6) & 0x1F);
                tmpbuf[1] = 0x80 | (val & 0x3F);
                datalen = 2;
            } else {
                tmpbuf[0] = val;
                datalen = 1;
            }
            data = tmpbuf;
            break;
          }  // case 'c'

          case 's':
            data = va_arg(args, const char *);
            if (!data) {
                data = "(null)";
            }
            datalen = strlen(data);
            if (prec >= 0) {
                datalen = ubound(datalen, prec);
            }
            break;

          case 'd':
          case 'i':
          case 'u': {
#if 0 // FIXME: no language support at the moment
            const int use_fullwidth = plus_flag && current_lang == LANG_JA;
#else
            const int use_fullwidth = 0;
#endif
            if (use_fullwidth) {
                /* widthはバイト単位なので、3倍にする */
                width *= 3;
                /* パディング文字も全角にする */
                if (*pad == '0') {
                    pad = "０";
                } else {
                    pad = "　";
                }
            }
            /* long longを扱う場合は処理が重くなるので、それ以外のデータ型の
             * 場合はlongで処理する。コードの重複は仕方ない */
            int pos;
            if (dsize == LL) {
                unsigned long long val;
                int isneg;
                if (type == 'u') {
                    val = va_arg(args, unsigned long long);
                    isneg = 0;
                } else {
                    long long sval = va_arg(args, long long);
                    if (sval >= 0) {
                        isneg = 0;
                        val = sval;
                    } else {
                        isneg = 1;
                        val = -sval;
                    }
                }
                pos = sizeof(tmpbuf);
                do {
                    const unsigned long long newval = val / 10;
                    const int digit = val - newval*10;
                    if (use_fullwidth) {
                        /* '０': U+FF10 ⇒ 0xEF 0xBC 0x90 */
                        tmpbuf[--pos] = 0x90 + digit;
                        tmpbuf[--pos] = 0xBC;
                        tmpbuf[--pos] = 0xEF;
                    } else {
                        tmpbuf[--pos] = '0' + digit;
                    }
                    val = newval;
                } while (val != 0 && pos >= (use_fullwidth ? 6 : 2));
                if (isneg) {
                    if (use_fullwidth) {
                        /* '−': U+FF0D */
                        tmpbuf[--pos] = 0x8D;
                        tmpbuf[--pos] = 0xBC;
                        tmpbuf[--pos] = 0xEF;
                    } else {
                        tmpbuf[--pos] = '-';
                    }
                }
            } else {
                unsigned long val;
                int isneg;
                if (type == 'u') {
                    if (dsize == L) {
                        val = va_arg(args, unsigned long);
                    } else {
                        val = va_arg(args, unsigned int);
                    }
                    isneg = 0;
                } else {
                    long sval;
                    if (dsize == L) {
                        sval = va_arg(args, long);
                    } else {
                        sval = va_arg(args, int);
                    }
                    if (sval >= 0) {
                        isneg = 0;
                        val = sval;
                    } else {
                        isneg = 1;
                        val = -sval;
                    }
                }
                pos = sizeof(tmpbuf);
                do {
                    const long newval = val / 10;
                    const int digit = val % 10;
                    if (use_fullwidth) {
                        /* '０': U+FF10 ⇒ 0xEF 0xBC 0x90 */
                        tmpbuf[--pos] = 0x90 + digit;
                        tmpbuf[--pos] = 0xBC;
                        tmpbuf[--pos] = 0xEF;
                    } else {
                        tmpbuf[--pos] = '0' + digit;
                    }
                    val = newval;
                } while (val != 0 && pos >= (use_fullwidth ? 6 : 2));
                if (isneg) {
                    if (use_fullwidth) {
                        /* '−': U+FF0D */
                        tmpbuf[--pos] = 0x8D;
                        tmpbuf[--pos] = 0xBC;
                        tmpbuf[--pos] = 0xEF;
                    } else {
                        tmpbuf[--pos] = '-';
                    }
                }
            }
            data = &tmpbuf[pos];
            datalen = sizeof(tmpbuf) - pos;
            break;
          }  // case 'd', 'i'

          case 'o':
          case 'p':
          case 'x':
          case 'X': {
            const unsigned int shift = (type=='o' ? 3 : 4);
            const char * const digits = (type=='p' || type=='X'
                                         ? "0123456789ABCDEF"
                                         : "0123456789abcdef");
            int pos = sizeof(tmpbuf);
            if (dsize == LL && type != 'p') {
                unsigned long long val = va_arg(args, unsigned long long);
                do {
                    const unsigned long long newval = val >> shift;
                    const unsigned int digit = val & ((1<<shift) - 1);
                    tmpbuf[--pos] = digits[digit];
                    val = newval;
                } while (val != 0 && pos > 2);
            } else {
                uintptr_t val;
                if (type == 'p') {
                    val = (uintptr_t) va_arg(args, const void *);
                    if (val == 0) {
                        /* %pでNULLポインタの場合、数字ではなく「(null)」を
                         * 出力する */
                        data = "(null)";
                        datalen = strlen(data);
                        break;
                    }
                } else if (dsize == L) {
                    val = va_arg(args, unsigned long);
                } else {
                    val = va_arg(args, unsigned int);
                }
                do {
                    const unsigned long newval = val >> shift;
                    const unsigned int digit = val & ((1<<shift) - 1);
                    tmpbuf[--pos] = digits[digit];
                    val = newval;
                } while (val != 0 && pos > 2);
            }
            if (type == 'p') {
                tmpbuf[--pos] = 'x';
                tmpbuf[--pos] = '0';
            }
            data = &tmpbuf[pos];
            datalen = sizeof(tmpbuf) - pos;
            break;
          }  // case 'o', 'p', 'x', 'X'

          case 'f':
          case 'g': {
            if (prec < 0) {
                prec = 6; // デフォルト精度
            }
#ifdef VSNPRINTF_USE_FLOATS
# define DOUBLE_FLOAT float
# define ISNAN_DF isnanf
# define ISINF_DF isinff
# define FABS_DF fabsf
# define TRUNC_DF truncf
# define FMOD_DF fmodf
#else
# define DOUBLE_FLOAT double
# define ISNAN_DF isnan
# define ISINF_DF isinf
# define FABS_DF fabs
# define TRUNC_DF trunc
# define FMOD_DF fmod
#endif
            DOUBLE_FLOAT val = va_arg(args, double);
            if (ISNAN_DF(val)) {
                data = "nan";
                datalen = strlen(data);
                break;
            } else if (ISINF_DF(val)) {
                if (val < 0) {
                    data = "-inf";
                    datalen = strlen(data);
                } else {
                    data = "inf";
                    datalen = strlen(data);
                }
                break;
            }
            int isneg = (val < 0);
            val = FABS_DF(val);
            /* 最下位の桁を丸める */
            DOUBLE_FLOAT round_temp = 1;
            int round_count;
            for (round_count = 0; round_count < prec; round_count++) {
                round_temp *= 10;
            }
            val += (DOUBLE_FLOAT)0.5 / round_temp;
            DOUBLE_FLOAT val_trunc = TRUNC_DF(val);
            DOUBLE_FLOAT val_frac  = val - val_trunc;
            int pos = sizeof(tmpbuf);
            tmpbuf[--pos] = 0;
            do {
                const int digit = (int)TRUNC_DF(FMOD_DF(val_trunc, 10));
                tmpbuf[--pos] = '0' + digit;
                val_trunc = TRUNC_DF(val_trunc / 10);
            } while (val_trunc != 0 && pos > 1);
            if (isneg) {
                tmpbuf[--pos] = '-';
            }
            if (pos > 0) {
                memmove(tmpbuf, &tmpbuf[pos], strlen(&tmpbuf[pos])+1);
            }
            pos = strlen(tmpbuf);
            if (prec > (sizeof(tmpbuf)-pos) - 2) {
                prec = (sizeof(tmpbuf)-pos) - 2;
            }
            int have_decimal = 0;
            if (prec > 0) {
                have_decimal = 1;
                tmpbuf[pos++] = '.';
                for (; prec > 0; prec--) {
                    val_frac *= 10;
                    const int digit = (int)TRUNC_DF(val_frac);
                    tmpbuf[pos++] = '0' + digit;
                    val_frac -= digit;
                }
                tmpbuf[pos] = 0;  // オーバフロー不可
            }
            data = tmpbuf;
            datalen = strlen(data);
            if (type == 'g' && have_decimal) {
                // 'g'の場合は「.」以降、末尾の「0」を削除
                while (datalen > 0 && data[datalen-1] == '0') {
                    datalen--;
                }
                if (datalen > 0 && data[datalen-1] == '.') {
                    datalen--;
                }
            }
            break;
          }  // case 'f', 'g'

        }  // switch (type)

        /* (3) フォーマットの結果を、必要に王子でパディングし、バッファに
         *     格納して合計バイト数を更新する */

        if (UNLIKELY(!data)) {
            DMSG("ERROR: data==NULL after format!");
            data = "(ERROR)";
            datalen = strlen(data);
        }
        if (UNLIKELY(datalen < 0)) {
            DMSG("ERROR: invalid datalen %d!", datalen);
            data = "(ERROR)";
            datalen = strlen(data);
        }
        width -= datalen;
        if (left_justify) {
            OUTSTR(data, datalen);
        }
        while (width > 0) {
            const char *padptr = pad;
            while (*padptr) {
                OUTCHAR(*padptr);
                padptr++;
                width--;
            }
        }
        if (!left_justify) {
            OUTSTR(data, datalen);
        }

      restart:;
    }  // while (*format)

    /* バッファに\0バイトを追加（ただsize<=0の場合を除く） */
    if (size > 0) {
        buf[ubound(total,size-1)] = 0;
    }

    /* 結果文字列の長さを返す */
    return total;
}

/*************************************************************************/
/***************************** その他の関数 ******************************/
/*************************************************************************/

/**
 * report_error:  エラーの発生をユーザに知らせる。
 *
 * 【引　数】message:  エラーメッセージ
 * 【戻り値】なし
 */
void report_error(const char *message)
{
    sys_report_error(message);
}

/*************************************************************************/

/**
 * set_performance:  システムの処理速度を設定する。
 *
 * 【引　数】level: 処理速度（PERFORMANCE_*）
 * 【戻り値】なし
 */
void set_performance(int level)
{
    switch (level) {
      case PERFORMANCE_LOW:
        sys_set_performance(SYS_PERFORMANCE_LOW);
        break;
      case PERFORMANCE_HIGH:
        sys_set_performance(SYS_PERFORMANCE_HIGH);
        break;
      default:
        DMSG("Unknown level %d, assuming NORMAL", level);
        /* 次ケースへ続く */
      case PERFORMANCE_NORMAL:
        sys_set_performance(SYS_PERFORMANCE_NORMAL);
        break;
    }
};

/*************************************************************************/
/***************************** デバッグ関数 ******************************/
/*************************************************************************/

#ifdef DEBUG

/**
 * do_DMSG:  DMSGマクロの補助関数。システム依存出力関数を呼び出す。
 * デバッグ時のみ実装。
 *
 * 【引　数】format: フォーマット文字列
 * 　　　　　   ...: フォーマットデータ
 * 【戻り値】なし
 */
void do_DMSG(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    sys_DMSG(format, args);
    va_end(args);
}

/*************************************************************************/

/**
 * abort:  プログラムを強制終了させる。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void abort(void)
{
    sys_exit(1);
}

#endif  // DEBUG

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
