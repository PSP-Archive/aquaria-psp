/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/common.h: Main header file.
 */

/* 二重インクルード防止 */

#ifndef COMMON_H
#define COMMON_H

/*************************************************************************/
/**************************** 構成オプション *****************************/
/*************************************************************************/

/*
 * DEBUG
 *
 * 定義するとデバッグモードでコンパイルされ、以下の変更が適用される。
 *
 * 　・前条件、後条件チェック（下のPRECOND()、POSTCOND()マクロ）が有効に
 * 　　なり、条件が満たされないとプログラムが強制終了される。
 *
 * 　・各種デバッグメッセージが出力される。
 *
 * 　・ゲーム中の各種デバッグ操作が可能になる。以下「JoyDebug」とは、
 * 　　systemopts.joy_debugに指定されているジョイスティックボタンを指す。
 * 　　　○Ctrl+「M」またはJoyDebug+左ボタンで使用メモリ量を、
 * 　　　　Ctrl+「C」またはJoyDebug+右ボタンでCPU使用率を表示
 */

#define DEBUG

#if defined(_WIN32) && !defined(__MINGW__)  // VCでは構成で管理
# ifdef _DEBUG
#  define DEBUG
# else
#  undef DEBUG
# endif
#endif


/*
 * INCLUDE_TESTS
 *
 * 定義すると、コマンドライン引数「-test」により実行される各種テスト関数を
 * プログラムに組み込む。定義しない場合、テスト関数そのものは省かれ、
 * 「-test」も認識されない。
 *
 * 非デバッグ時は無効。
 */

// #define INCLUDE_TESTS


/*
 * USE_VALGRIND
 *
 * 動的メモリエラー監視ツールであるValgrind下でプログラムを実行する場合、
 * このシンボルを定義することで、mem_alloc()等の関数でValgrindと連動させ、
 * malloc()同様に監視することができる。
 *
 * 通常はsysdep-.../Makeconf-sysにより設定される。非デバッグ時は無効。
 */

// #define USE_VALGRIND


/* 非デバッグ時、無効な設定を強制解除する */
#ifndef DEBUG
# undef INCLUDE_TESTS
# undef USE_VALGRIND
#endif

/*-----------------------------------------------------------------------*/

/*
 * USE_DOUBLE_DTRIG
 *
 * 定義すると、度単位の倍精度三角形関数（dsin()等）を定義する。倍精度の三角
 * 形関数を使わない場合、これを定義しないことで一定のコード量削減が図れる。
 *
 * Aquariaでは使わないので定義しない。
 */

// #define USE_DOUBLE_DTRIG

/*
 * CUSTOM_DTRIG_FUNCTIONS
 *
 * 定義すると、util.cで度単位の三角形関数（dsinf()等）の定義を抑制する。
 * システム依存ソースで定義する際、sysdep-.../common.hでこのシンボルを定義
 * することで識別子衝突を回避できる。
 */

// #define CUSTOM_DTRIG_FUNCTIONS

/*-----------------------------------------------------------------------*/

/*
 * CXX_CONSTRUCTOR_HACK
 *
 * C++コードをリンクする場合、main()が呼び出される前にコンストラクタ関数が
 * 動き出す。コンストラクタの中でメモリ確保関数が呼び出されると、（本来は
 * main()から呼び出される初期化処理で確保されるはずの）プールが準備されて
 * いないため、メモリ確保が失敗してしまう。
 *
 * この問題をとりあえず回避するため、CXX_CONSTRUCTOR_HACKを定義することが
 * できる。これが定義されていると、malloc()・calloc()・realloc()時にメモリ
 * 管理機能が初期化されているかどうか確認し、初めての呼び出しで初期化を行う。
 * また、二重初期化を回避するため、通常の初期化処理ではメモリ管理機能の初期
 * 化は行わない。
 *
 * main()呼び出し前にmalloc()等が呼び出される場合のみ定義すること。
 */

// #define CXX_CONSTRUCTOR_HACK

/*************************************************************************/
/************************** 共通システムヘッダ ***************************/
/*************************************************************************/

/* -Wshadow対応（使わないライブラリ関数との識別子競合を回避）。ついでに
 * メモリ管理関数など、使いたくない関数も無効化。ただしツールでは使用を許可 */
#ifndef IN_TOOL
# define abort   __UNUSED_abort
# define bzero   __UNUSED_bzero
# define calloc  __UNUSED_calloc
# define exit    __UNUSED_exit
# define free    __UNUSED_free
# define malloc  __UNUSED_malloc
# define memset  __UNUSED_memset
# define realloc __UNUSED_realloc
# define strdup  __UNUSED_strdup
# define tmpfile __UNUSED_tmpfile
#endif
#define div     __UNUSED_div
#define exp     __UNUSED_exp
#define index   __UNUSED_index //なんでこんな一般変数名みたいなものを作るかなー
#define gamma   __UNUSED_gamma
#define y0      __UNUSED_y0
#define y0f     __UNUSED_y0f
#define y1      __UNUSED_y1
#define y1f     __UNUSED_y1f

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef IN_TOOL
# undef abort
# undef bzero
# undef calloc
# undef exit
# undef free
# undef malloc
# undef memset
# undef realloc
# undef strdup
# undef tmpfile
#endif
#undef div
#undef exp
#undef index
#undef gamma
#undef y0
#undef y0f
#undef y1
#undef y1f

#ifdef IN_TOOL  // ツールではstdio()の使用を許可する
# include <stdio.h>
# include <errno.h>
# define stricmp strcasecmp  // このネーミングは未だに大嫌い
# define strnicmp strncasecmp
#endif

/*************************************************************************/
/********************* システム・コンパイラ依存機能 **********************/
/*************************************************************************/

/*
 * IS_BIG_ENDIAN
 * IS_LITTLE_ENDIAN
 *
 * コンパイル時に実行環境のエンディアン性が判る場合、このどちらかを定義する
 * ことでバイト順変換処理の最適化を図ることができる。
 *
 * 通常はsysdep-.../Makeconf-sysにより設定される。
 */
// #define IS_BIG_ENDIAN
// #define IS_LITTLE_ENDIAN

/*
 * ALIGNED(n)
 *
 * データのアライメントを指定するキーワード。例えば
 *     int32_t ALIGNED(64) buf[1000];
 * と記述することで、アドレスが64の倍数になるバッファを定義できる。
 */
#ifdef __GNUC__
# define ALIGNED(n) __attribute__((aligned(n)))
#else
/* 未実装だと実行中にアドレスエラーが発生することもあるので、コンパイル中止 */
# error ALIGNED not defined for this compiler
#endif

/*
 * CONST_FUNCTION
 *
 * const型関数（実行ステートやメモリ内容）に関係なく、特定の引数に対して
 * 常に同じ値を返す関数）を定義するときのキーワード。C言語ではconstが
 * 使えないので、GCCの__attribute__()を利用。
 */
#ifdef __GNUC__
# define CONST_FUNCTION __attribute__((const))
#else
# define CONST_FUNCTION /*nothing*/
#endif

/*
 * PURE_FUNCTION
 *
 * 純関数（メモリ変更などの副作用がない関数）を定義するときのキーワード。
 * CONST_FUNCTIONとは違って、揮発メモリにアクセスしても構わないので、構造体
 * フィールドの値を返す関数などに利用できる。
 */
#ifdef __GNUC__
# define PURE_FUNCTION __attribute__((pure))
#else
# define PURE_FUNCTION /*nothing*/
#endif

/*
 * FORMAT
 *
 * printf()のように、フォーマット文字列を受ける関数を宣言するキーワード。
 * GCCでは、フォーマット文字列にデータ型が合っているかチェックできる。
 *
 *      fmt: フォーマット文字列の引数番号（最初の引数＝1）
 * firstarg: 最初のフォーマットデータ引数の引数番号（「...」の位置）
 */
#ifdef __GNUC__
# define FORMAT(fmt,firstarg) __attribute__((format(printf,fmt,firstarg)))
#else
# define FORMAT(fmt,firstarg) /*nothing*/
#endif

/*
 * NORETURN
 *
 * 関数が戻らないことを宣言するキーワード。abort()、sys_exit()の宣言で使う。
 */
#ifdef __GNUC__
# define NORETURN __attribute__((noreturn))
#else
# define NORETURN /*nothing*/
#endif

/*
 * NOINLINE
 *
 * コンパイラに特定の関数をインラインさせないキーワード。
 */
#ifdef __GNUC__
# define NOINLINE __attribute__((noinline))
#else
# define NOINLINE /*nothing*/
#endif

/*
 * LIKELY, UNLIKELY
 *
 * ifの分岐条件の期待値を明示するマクロ。LIKELY(x)は「x!=0が期待される」、
 * UNLIKELY(x)は「x==0が期待される」を意味する。ともにxを評価し、結果が0以
 * 外の場合は真、0の場合は偽を返す（返された値は真理値としてのみ利用可能）。
 */
#ifdef __GNUC__
# define LIKELY(x)   __builtin_expect(!!(x), 1)
# define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
# define LIKELY(x)   (x)
# define UNLIKELY(x) (x)
#endif

/*
 * BARRIER
 *
 * メモリバリアを設定する。メモリロード・ストアはバリアを超えて移動されない。
 * 主に複数のスレッドからアクセスされるデータへのアクセス順序を制限するのに
 * 用いる。
 */
#ifdef __GNUC__
# if defined(__x86_64__)
#  define BARRIER()  asm volatile("mfence")
# elif defined(__i386__)
/* MFENCEはSSE2専用だが、CPUIDで同様の結果が得られる */
#  define BARRIER()  asm volatile("cpuid" : : : "eax", "ebx", "ecx", "edx")
# elif defined(__mips__)
#  define BARRIER()  asm volatile("sync")
# else
#  warning Memory barrier CPU instruction unknown!  Multithreading behavior may be unstable.
#  define BARRIER()  asm volatile("")
# endif
#else
# error Please define BARRIER() for this compiler.
#endif

/*
 * DEBUG_MATH_BARRIER
 *
 * 浮動小数点演算において、引数として渡された変数に対する最適化を断ち切る。
 *
 * 最適化オプションによって、浮動小数点演算の例外を無視してゼロ除算などが
 * 行われることがある。本来はこの最適化オプションと実際の例外処理の有無が
 * 一致しなければならないが、デバッグ時において、バグの原因となり得る不正な
 * 演算を検出するため、あえて例外を有効にしている。このため、最適化でゼロ
 * チェックなどのテストを越えて移動された演算が行われたとき、実際の処理に
 * 問題がないのに例外が発生してしまう可能性がある。これを回避するため、この
 * DEBUG_MATH_BARRIER()を変数のテストと演算の間に挟み、テストより前に演算が
 * 行われないようにすることができる。
 *
 * 非デバッグ時においては、最適化オプションに合わせて例外が無効化されている
 * ので、何もしない。
 *
 * （例）
 * dist = sqrtf(x*x + y*y);
 * if (dist > 0) {
 *     DEBUG_MATH_BARRIER(dist);  // distを使った演算はここを越えて移動されない
 *     x /= dist;
 *     y /= dist;
 * }
 */
#ifdef DEBUG
# ifdef __GNUC__
#  ifdef __mips__
#   define DEBUG_MATH_BARRIER(var)  asm volatile("" : "=f" (var) : "0" (var))
#  else
#   define DEBUG_MATH_BARRIER(var)  asm volatile("" : "=x" (var) : "0" (var))
#  endif
# else
#  define DEBUG_MATH_BARRIER(var)
# endif
#else
# define DEBUG_MATH_BARRIER(var)
#endif

/*************************************************************************/
/****************************** 便利マクロ *******************************/
/*************************************************************************/

/**** 整数値関連 ****/

/* 最小値・最大値・絶対値 */
#undef min
#undef max
#undef abs
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
#define abs(n) ((n) > 0 ? (n) : -(n))

/* 値の制限。min・maxと同様だが、名称を使い分けることでコードの可読性を高める。
 * 使う際は引数の順序に注意すること */
#define lbound(n,lower) max((n), (lower))
#define ubound(n,upper) min((n), (upper))
#define bound(n,lower,upper) ubound(lbound((n), (lower)), (upper))

/* 値xをalignの倍数にアラインする。align_up()は切り上げ、align_down()は
 * 切り捨てる */
/* 注：GCC 4.2からは「a/b*b」（bが2のべき乗の場合）を「a&-b」に最適化できる
 * 　　ので、これで遅くなることはない */
#define align_up(x,align)   (((x) + ((align)-1)) / (align) * (align))
#define align_down(x,align) ((x) / (align) * (align))

/**** 浮動小数点値関連 ****/

/* 浮動小数点値の小数点以下の部分を返す。fmod[f](n,1)とほぼ同義で、より高速
 * だがNaN等には対応していない。なお負数の場合、fmod()と違って正数を返す */
#define frac(n)   ((n) - floor ((n)))
#define fracf(n)  ((n) - floorf((n)))

/* 浮動小数点値を32ビット整数に変換 */
#define ifloor(n)  ((int32_t)floor ((n)))  // -∞へ
#define ifloorf(n) ((int32_t)floorf((n)))
#define itrunc(n)  ((int32_t)trunc ((n)))  // 0へ
#define itruncf(n) ((int32_t)truncf((n)))
#define iceil(n)   ((int32_t)ceil  ((n)))  // +∞へ
#define iceilf(n)  ((int32_t)ceilf ((n)))
#define iround(n)  ((int32_t)round ((n)))  // 近い方へ
#define iroundf(n) ((int32_t)roundf((n)))

/* 最適化効率向上のため、rand()を符号無しにする。（モジュロ計算で負数にな
 * る可能性がなくなれば、アセンブリレベルで論理積計算ができるようになる） */
#define random() ((unsigned int)rand())

/* 浮動小数点乱数（0.0 ≦ x ＜ 1.0） */
#define frandom()  (random() / ((double)RAND_MAX + 1))
#define frandomf() (random() / ((float)((double)RAND_MAX + 1)))

/* 特定範囲の乱数（lo ≦ x ≦ hi） ※lo、hiに副作用がないこと */
#define random2(lo,hi)   ((lo) +  random () % ((hi) - (lo) + 1))
#define frandom2(lo,hi)  ((lo) + frandom () * ((hi) - (lo)    ))
#define frandom2f(lo,hi) ((lo) + frandomf() * ((hi) - (lo)    ))

/* M_PI（円周率）の単精度版 */
#undef M_PIf
#define M_PIf 3.14159265f

/* 2次元座標に適した度単位の三角関数（通常の関数はutil.cにて定義）。
 * ※2次元処理では画面座標を利用するので、デカルト座標と比べてY軸が反転され
 * 　ている。このため、各三角関数のY値を反転しないと正しい結果が得られない。
 * 　（厳密にはY座標が反転されても余弦は変わらないが、読みやすさのため、他の
 * 　関数と同様に2次元版を定義しておく。） */
#define dsinf_2D(deg)   (-dsinf((deg)))
#define dcosf_2D(deg)   ( dcosf((deg)))
#define dtanf_2D(deg)   (-dtanf((deg)))
#define datan2f_2D(y,x) (datan2f(-(y), (x)))
#define dsincosf_2D(deg,sin_ret,cos_ret)  do {  \
    float sin_val;                              \
    dsincosf((deg), &sin_val, (cos_ret));       \
    *(sin_ret) = -sin_val;                      \
} while (0)

/* 3次元内積・外積計算マクロ */
#define DOT3(x1,y1,z1,x2,y2,z2)  ((x1)*(x2) + (y1)*(y2) + (z1)*(z2))
#define DOT4(x1,y1,z1,w1,x2,y2,z2,w2) \
    ((x1)*(x2) + (y1)*(y2) + (z1)*(z2) + (w1)*(w2))
#define CROSS_X(x1,y1,z1,x2,y2,z2) ((y1)*(z2) - (z1)*(y2))
#define CROSS_Y(x1,y1,z1,x2,y2,z2) ((z1)*(x2) - (x1)*(z2))
#define CROSS_Z(x1,y1,z1,x2,y2,z2) ((x1)*(y2) - (y1)*(x2))
#define CROSS(x1,y1,z1,x2,y2,z2) \
    (sqrtf((CROSS_X(x1,y1,z1,x2,y2,z2) * CROSS_X(x1,y1,z1,x2,y2,z2)) \
         + (CROSS_Y(x1,y1,z1,x2,y2,z2) * CROSS_Y(x1,y1,z1,x2,y2,z2)) \
         + (CROSS_Z(x1,y1,z1,x2,y2,z2) * CROSS_Z(x1,y1,z1,x2,y2,z2))))

/**** その他 ****/

/* 配列の要素数を返す */
#define lenof(array) ((int)(sizeof(array) / sizeof(*(array))))

/* 1ビットフラグをのセット・テスト・クリア
 * ※flagは副作用がないこと */
#define SETFLAG(array,flag)   ((array)[(flag)>>3] |=  (1<<(7^((flag) & 7))))
#define TESTFLAG(array,flag)  ((array)[(flag)>>3] &   (1<<(7^((flag) & 7))))
#define CLEARFLAG(array,flag) ((array)[(flag)>>3] &= ~(1<<(7^((flag) & 7))))

/* ピクセルのARGB値（それぞれ0〜255）を32ビット符号無し整数に詰め込む。
 * リトルエンディアンでBGRA順のシステムを想定しており、バイト順が異なる
 * システムではsysdep-.../common.hにて再定義すること */
#define PACK_ARGB(a,r,g,b)  ((a)<<24 | (r)<<16 | (g)<<8 | (b))

/*************************************************************************/
/***************************** 共通データ型 ******************************/
/*************************************************************************/

/**
 * Vector2f, Vector3f, Vector4f, Matrix3f, Matrix4f: ベクトル・行列型
 * （vector.hで使用）。
 */

typedef union Vector2f_ {
    float v[2];
    struct {
        float x, y;
    };
} Vector2f;

typedef union Vector3f_ {
    float v[3];
    struct {
        float x, y, z;
    };
} Vector3f;

typedef union Vector4f_ {
    float v[4];
    struct {
        float x, y, z, w;
    };
} Vector4f;

typedef union Matrix3f_ {
    float m[3][3];
    float a[9];
    Vector3f v[3];
    struct {
        float _11, _12, _13,
              _21, _22, _23,
              _31, _32, _33;
    };
} Matrix3f;

typedef union Matrix4f_ {
    float m[4][4];
    float a[16];
    Vector4f v[4];
    struct {
        float _11, _12, _13, _14,
              _21, _22, _23, _24,
              _31, _32, _33, _34,
              _41, _42, _43, _44;
    };
} Matrix4f ALIGNED(16);

/*************************************************************************/

/* Texture: テキスチャ構造体（texture.hで定義） */
typedef struct Texture_ Texture;

/* ResourceManager: リソース管理構造体（resource.hで定義） */
typedef struct ResourceManager_ ResourceManager;

/* Sound: 音声データ構造体（sound.hで定義） */
typedef struct Sound_ Sound;

/* SoundFormat: 音声データ形式定数（sound.hで定義） */
#ifndef __cplusplus  // C++では、ここで定義できない模様
typedef enum SoundFormat_ SoundFormat;
#endif

/* SysFile: 低層ファイルポインタ（sysdep.hで使用） */
typedef struct SysFile_ SysFile;

/*************************************************************************/
/****************** グローバル定数の定義とデータの宣言 *******************/
/*************************************************************************/

/*
 * MAX_SKIPPED_FRAMES
 *
 * 過負荷時、通常は落ちたフレームの数に応じてゲーム時間を進行させるが、その数
 * があまりにも多いと、キャラクターが急に動いたり、MIDI音楽で多数のイベントが
 * 同時に発生してしまうなど、不具合の原因となり得るので、スキップするフレーム
 * の数をこの値以下に制限する。
 *
 * なお、Valgrindにより実行時メモリチェックを行っている場合は動作が極めて遅く
 * なるので、制限も多めに設定しておく。
 */

#ifdef USE_VALGRIND
# define MAX_SKIPPED_FRAMES  20
#else
# define MAX_SKIPPED_FRAMES  2
#endif

/*************************************************************************/

/*
 * SystemOptions systemopts
 *
 * システム関連の環境設定。ゲームデータには保存されず、あくまで実行システム
 * の設定として扱う。
 */

typedef struct {
    /* 全画面表示の有無 */
    uint8_t fullscreen;
    /* ワイド（アスペクト比16:9）画面フラグ（0＝アスペクト比4:3） */
    uint8_t wide;
    /* デフォルト言語（LANG_*） */
    uint8_t lang;
    /* コントローラ（ジョイスティック）設定 */
    int8_t joy_ok;         // メニュー決定
    int8_t joy_cancel;     // メニューキャンセル
    int8_t joy_menu;       // メニュー呼び出し
    int8_t joy_status;     // ステータス切り替え
    int8_t joy_run;        // 走る
    int8_t joy_item;       // 所持品を使う
    int8_t joy_ring_l;     // 前の指輪を装備
    int8_t joy_ring_r;     // 次の指輪を装備
#ifdef DEBUG
    int8_t joy_debug;      // デバッグ用ボタン（設定ファイルには保存されない）
#endif
    int8_t joy_x_axis;     // X軸入力に使うコントローラ軸
    int8_t joy_y_axis;     // Y軸入力に使うコントローラ軸
    float joy_thresh;      // 入力閾値（0〜1）
} SystemOptions;

extern SystemOptions systemopts;

/*************************************************************************/
/************************ 共通ユーティリティ関数 *************************/
/*************************************************************************/

/**
 * mem_clear:  メモリ領域をゼロにクリアする。
 *
 * 【引　数】ptr: クリアするメモリ領域へのポインタ
 * 　　　　　len: クリアするバイト数
 * 【戻り値】なし
 */
extern void mem_clear(void *ptr, uint32_t len);

/**
 * mem_fill8:  メモリ領域を指定された8ビット値でフィルする。
 *
 * 【引　数】ptr: フィルするメモリ領域へのポインタ
 * 　　　　　val: フィル値
 * 　　　　　len: フィルするバイト数
 * 【戻り値】なし
 */
extern void mem_fill8(void *ptr, uint8_t val, uint32_t len);

/**
 * mem_fill32:  メモリ領域を指定された32ビット値でフィルする。
 *
 * 【引　数】ptr: フィルするメモリ領域へのポインタ
 * 　　　　　val: フィル値
 * 　　　　　len: フィルするバイト数
 * 【戻り値】なし
 */
extern void mem_fill32(void *ptr, uint32_t val, uint32_t len);

#ifdef IN_TOOL  // ツールではmemset()を使用
# define mem_clear(ptr,len)      memset((ptr), 0,     (len))
# define mem_fill8(ptr,val,len)  memset((ptr), (val), (len))
#endif

/*************************************************************************/

/**
 * is_little_endian:  実行環境のエンディアン性がリトルエンディアンかどうかを
 * 返す。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝リトルエンディアン環境、0＝ビッグエンディアン環境
 * 【注　意】プログラム全体を通して、ビッグエンディアン・リトルエンディアン
 * 　　　　　以外の環境には対応していない。
 */
#if defined(IS_BIG_ENDIAN)
# define is_little_endian()  0
#elif defined(IS_LITTLE_ENDIAN)
# define is_little_endian()  1
#else
extern CONST_FUNCTION int is_little_endian(void);
#endif

/**
 * be_to_s16, be_to_u16, be_to_s32, be_to_u32, be_to_float:  ビッグエンディ
 * アン整数値または浮動小数点値をネイティブ形式に変換する。
 * 　・be_to_s16: 符号付き16ビット整数値を変換する。
 * 　・be_to_u16: 符号無し16ビット整数値を変換する。
 * 　・be_to_s32: 符号付き32ビット整数値を変換する。
 * 　・be_to_u32: 符号無し32ビット整数値を変換する。
 * 　・be_to_float: 浮動小数点値を変換する。
 *
 * 【引　数】val: 変換するビッグエンディアン値
 * 【戻り値】ネイティブ形式に変換した値
 */
static CONST_FUNCTION inline int16_t be_to_s16(const int16_t val) {
    return is_little_endian() ? ((val>>8 & 0xFF) | val<<8) : val;
}
static CONST_FUNCTION inline uint16_t be_to_u16(const uint16_t val) {
    return is_little_endian() ? (val>>8 | val<<8) : val;
}
static CONST_FUNCTION inline int32_t be_to_s32(const int32_t val) {
    return is_little_endian() ? ((val>>24 & 0xFF) | (val>>8 & 0xFF00)
                               | (val & 0xFF00)<<8 | val<<24) : val;
}
static CONST_FUNCTION inline uint32_t be_to_u32(const uint32_t val) {
    return is_little_endian() ? (val>>24 | (val>>8 & 0xFF00)
                               | (val & 0xFF00)<<8 | val<<24) : val;
}
static CONST_FUNCTION inline float be_to_float(const float val) {
    if (is_little_endian()) {
        union {float f; uint32_t i;} u;
        u.f = val;
        u.i = be_to_u32(u.i);
        return u.f;
    } else {
        return val;
    }
}

/**
 * s16_to_be, u16_to_be, s32_to_be, u32_to_be, float_to_be:  ネイティブ形式
 * の整数値または浮動小数点値をビッグエンディアンに変換する。
 * 　・s16_to_be: 符号付き16ビット整数値を変換する。
 * 　・u16_to_be: 符号無し16ビット整数値を変換する。
 * 　・s32_to_be: 符号付き32ビット整数値を変換する。
 * 　・u32_to_be: 符号無し32ビット整数値を変換する。
 * 　・float_to_be: 浮動小数点値を変換する。
 *
 * 【引　数】val: 変換するネイティブ形式の値
 * 【戻り値】ビッグエンディアンに変換した値
 */
#define s16_to_be(val)    (be_to_s16(val))
#define u16_to_be(val)    (be_to_u16(val))
#define s32_to_be(val)    (be_to_s32(val))
#define u32_to_be(val)    (be_to_u32(val))
#define float_to_be(val)  (be_to_float(val))

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
extern CONST_FUNCTION float dsinf(const float angle);
extern CONST_FUNCTION float dcosf(const float angle);
extern CONST_FUNCTION float dtanf(const float angle);
extern void dsincosf(const float angle, float *sin_ret, float *cos_ret);
extern CONST_FUNCTION float datan2f(const float y, const float x);

#ifdef USE_DOUBLE_DTRIG

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
extern CONST_FUNCTION double dsin(const double angle);
extern CONST_FUNCTION double dcos(const double angle);
extern CONST_FUNCTION double dtan(const double angle);
extern void dsincos(const double angle, double *sin_ret, double *cos_ret);
extern CONST_FUNCTION double datan2(const double y, const double x);

#endif  // USE_DOUBLE_DTRIG

/**
 * anglediff:  2つの角度の差を返す。
 *
 * 【引　数】angle1: 1つ目の角度（度単位）
 * 　　　　　angle2: 2つ目の角度（度単位）
 * 【戻り値】angle1 - angle2（度単位、[-180,+180)の範囲内）
 */
extern CONST_FUNCTION float anglediff(const float angle1, const float angle2);

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
extern int intersect_lines(const Vector2f *P1, const Vector2f *v1,
                           const Vector2f *P2, const Vector2f *v2,
                           float *t1_ret, float *t2_ret);

/*************************************************************************/

#ifndef IN_TOOL  // ツールではstdio版を使用

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
extern int32_t snprintf(char *buf, uint32_t size, const char *format, ...)
    FORMAT(3,4);
extern int32_t vsnprintf(char *buf, uint32_t size, const char *format,
                         va_list args);

#endif  // !IN_TOOL

/*************************************************************************/

/**
 * report_error:  エラーの発生をユーザに知らせる。
 *
 * 【引　数】message:  エラーメッセージ
 * 【戻り値】なし
 */
extern void report_error(const char *message);

/**
 * set_performance:  システムの処理速度を設定する。
 *
 * 【引　数】level: 処理速度（PERFORMANCE_*）
 * 【戻り値】なし
 */
extern void set_performance(int level);
enum {
    PERFORMANCE_LOW,
    PERFORMANCE_NORMAL,
    PERFORMANCE_HIGH,
};

/*************************************************************************/

#if defined(IN_TOOL)

/* ツールでは標準エラーに出力する */
# define do_DMSG(fmt,...) fprintf(stderr, fmt , ## __VA_ARGS__)

#elif defined(DEBUG)

/**
 * do_DMSG:  DMSGマクロの補助関数。システム依存出力関数を呼び出す。
 * デバッグ時のみ実装。
 *
 * 【引　数】format: フォーマット文字列
 * 　　　　　   ...: フォーマットデータ
 * 【戻り値】なし
 */
extern void do_DMSG(const char *format, ...) FORMAT(1,2);

#endif  // DEBUG || IN_TOOL


#if defined(DEBUG)

/**
 * abort:  プログラムを強制終了させる。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern NORETURN void abort(void);

#endif  // DEBUG

/*************************************************************************/
/****************************** テスト関連 *******************************/
/*************************************************************************/

/*
 * PRECOND(condition)
 * PRECOND_SOFT(condition,fail_action)
 *
 * conditionが満たされない（評価の結果が0である）場合、プログラムを強制終了
 * させる。デバッグモードでのみ有効。なお非デバッグ時において、PRECOND_SOFTは
 * conditionが満たされない場合にfail_actionを実行する（PRECONDは何もしない）。
 */

#if defined(DEBUG) || defined(IN_TOOL)

# define PRECOND(condition) do { \
    if (UNLIKELY(!(condition))) { \
        do_DMSG("\n*** ALERT *** PRECONDITION FAILED\n%s:%d(%s): %s\n\n", \
                __FILE__, __LINE__, __FUNCTION__, #condition); \
        abort(); \
    } \
} while (0)
# define PRECOND_SOFT(condition,fail_action) do { \
    if (UNLIKELY(!(condition))) { \
        do_DMSG("\n*** ALERT *** PRECONDITION FAILED\n%s:%d(%s): %s\n\n", \
                __FILE__, __LINE__, __FUNCTION__, #condition); \
        /* fail_actionは実行されないが、非デバッグ時にコンパイルエラーが \
         * 発生しないよう、構文チェックはしておく */ \
        if (0) {fail_action;} \
        abort(); \
    } \
} while (0)

#else  // !DEBUG && !IN_TOOL

# define PRECOND(condition) /*nothing*/
# define PRECOND_SOFT(condition,fail_action) do { \
    if (UNLIKELY(!(condition))) { \
        fail_action; \
    } \
} while (0)

#endif

/*************************************************************************/

/*
 * POSTCOND(condition)
 *
 * conditionが満たされない（評価の結果が0である）場合、abort()でプログラム
 * を強制終了させる。デバッグモードとツールでのみ有効。
 *
 * 動作はPRECOND(condition)と同じだが、出力メッセージがPOSTCONDITIONとなって
 * いる。なおPOSTCOND_SOFTは存在しない。
 */

#if defined(DEBUG) || defined(IN_TOOL)

# define POSTCOND(condition) do { \
    if (UNLIKELY(!(condition))) { \
        do_DMSG("\n*** ALERT *** POSTCONDITION FAILED\n%s:%d(%s): %s\n\n", \
                __FILE__, __LINE__, __FUNCTION__, #condition); \
        abort(); \
    } \
} while (0)

#else  // !DEBUG && !IN_TOOL

# define POSTCOND(condition) /*nothing*/

#endif

/*************************************************************************/

/*
 * DMSG
 *
 * デバッグメッセージを、ソースファイル・行番号・関数名に続いて出力する。
 * デバッグモードとツールでのみ有効。
 */

#if defined(DEBUG) || defined(IN_TOOL)
# define DMSG(msg,...) do_DMSG("%s:%d(%s): " msg "\n", \
                               __FILE__, __LINE__, __FUNCTION__ \
                               , ## __VA_ARGS__)
#else
# define DMSG(msg,...) /*nothing*/
#endif

/*************************************************************************/
/***** システム依存共通ヘッダをインクルード (パス名はMakeconfで定義) *****/
/*************************************************************************/

#ifndef IN_TOOL
#include SYSDEP_COMMON_H
#endif

/*************************************************************************/
/*************************************************************************/

/* 二重インクルード防止終了 */

#endif  // COMMON_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
