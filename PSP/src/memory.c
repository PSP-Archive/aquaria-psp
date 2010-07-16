/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/memory.c: Memory management routines.
 */

/*
 * メモリ管理機能の実装として、各メモリプールの確保済み領域・空き領域の先頭
 * に管理構造体（AreaInfo）を付加して領域の管理を行う。AreaInfoには確保・空
 * きの区別や所属プールのほか、領域のサイズをブロック単位（以下参照）を格納
 * する。また確保領域の場合、要求サイズや要求時のフラグ・アライメント指定を
 * 記録し、再確保時の領域サイズ調整に使う。すべての領域が連続しており、ブロ
 * ック数の値によって一種のリンクリストを成しているが、独立したリストポイン
 * タを設定すると整合性を保つ必要が生じてバグの温床となるので、こうした疑似
 * リストで管理している。
 *
 * 過度な断片化を避けるため、メモリ確保は一定のサイズ（MEM_BLOCKSIZE）を
 * 単位として行われる。（元々、メモリをビットマップで管理する構想から出た
 * ものを流用した。）
 *
 * メモリ確保の際、適切な空き領域を見つけた後、その領域の先頭にある構造体を
 * 更新して、アライメントに応じてデータポインタのアドレスを計算する。データ
 * は構造体の後方に位置する。
 *
 * 注：マルチスレッド未対応。
 */

#include "common.h"
#include "memory.h"
#include "sysdep.h"

#ifdef DEBUG  // デバッグ用マクロを解除
# undef mem_alloc
# undef mem_realloc
# undef mem_free
# undef mem_strdup
#endif

#ifdef DEBUG
# include "debugfont.h"  // デバッグ情報表示のため
# include "graphics.h"   // 同上
#endif

/*************************************************************************/
/*************************** グローバルデータ ****************************/
/*************************************************************************/

#ifdef DEBUG

/* メモリ状況表示フラグ */
int debug_memory_display_flag;

#endif

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/**
 * POINTER_CHECK:  定義すると、mem_realloc()およびmem_free()に渡されたポイ
 * ンタの有効性を確認する。mem_realloc()とmem_free()の動作が若干遅くなる。
 *
 * DEEP_POINTER_CHECKとの併用は不可（DEEP_POINTER_CHECKが優先される）。
 */
#define POINTER_CHECK

/**
 * DEEP_POINTER_CHECK:  定義すると、mem_realloc()およびmem_free()に渡された
 * ポインタが実際に確保されたポインタなのかどうか、メモリブロックのリストを
 * 実際に歩いて確認する。当然ながら、mem_realloc()とmem_free()の動作が遅く
 * なる。
 */
// #define DEEP_POINTER_CHECK

/**
 * FREE_LIST_CHECK:  定義すると、空き領域リスト操作時、空き領域リストが正し
 * く作成されていることを確認する。当然ながら、メモリ確保・解放の動作が遅く
 * なる。
 */
// #define FREE_LIST_CHECK

/**
 * TRACE_ALLOCS:  定義すると、全てのメモリ確保・解放をDMSG()でログする。デ
 * バッグ時のみ有効。
 */
// #define TRACE_ALLOCS

/**
 * PROFILE_ALLOCS:  定義すると、全てのメモリ確保・解放呼び出しの実行時間を
 * 計測する。PSP専用。
 */
// #define PROFILE_ALLOCS

/*************************************************************************/

/* valgrindでデバッグするためのマクロ */

#ifdef USE_VALGRIND

# include <valgrind/memcheck.h>

/* メモリ領域をアクセス可能にする */
# define VG_ALLOC_REGION(addr,len) \
    VALGRIND_MALLOCLIKE_BLOCK((addr), (len), 0, 0)

/* メモリ領域をアクセス不可にする */
# define VG_FREE_REGION(addr) \
    VALGRIND_FREELIKE_BLOCK((addr), 0)

/* メモリ領域のvalidビットを更新する */
# define VG_SET_VBITS(ptr,vbits,nbytes) \
    VALGRIND_SET_VBITS((ptr), (vbits), (nbytes))

/* AreaInfo構造体へのアクセスを許可・不許可にする */

// 一般的なアクセスを許可する。管理フィールド以外は未定義と見なす */
# define VG_ENABLE_AREA(area) do { \
    const AreaInfo * const __area = (area); \
    /* 最初4バイトは管理用なので定義済み */ \
    VALGRIND_MAKE_MEM_DEFINED(__area, 4); \
    VALGRIND_MAKE_MEM_UNDEFINED((uint8_t *)__area+4, sizeof(AreaInfo)-4); \
} while (0)
// まっさらな管理構造体へのアクセスを許可
# define VG_ENABLE_EMPTY_AREA(area) \
    VALGRIND_MAKE_MEM_UNDEFINED((area), sizeof(AreaInfo))
// 全てのフィールドが設定されている領域（確保済み領域）へのアクセスを許可
# define VG_ENABLE_DEFINED_AREA(area) \
    VALGRIND_MAKE_MEM_DEFINED((area), sizeof(AreaInfo))
// 不許可に戻す
# define VG_DISABLE_AREA(area) \
    VALGRIND_MAKE_MEM_NOACCESS((area), sizeof(AreaInfo))

#else

# define VG_ALLOC_REGION(addr,len)      /*nothing*/
# define VG_FREE_REGION(addr)           /*nothing*/
# define VG_ENABLE_AREA(area)           /*nothing*/
# define VG_ENABLE_EMPTY_AREA(area)     /*nothing*/
# define VG_ENABLE_DEFINED_AREA(area)   /*nothing*/
# define VG_DISABLE_AREA(area)          /*nothing*/
# define VG_SET_VBITS(ptr,vbits,bytes)  /*nothing*/

#endif

/*************************************************************************/

typedef struct AreaInfo_ AreaInfo;

/* メモリ確保単位 */
#define MEM_BLOCKSIZE   64

/* メモリプール情報 */
typedef struct MemoryPool_ {
    void *base;
    uint32_t size;
    AreaInfo *first_free;  // アドレスがもっとも小さい空き領域へのポインタ
    AreaInfo *last_free;   // アドレスがもっとも大きい空き領域へのポインタ
} MemoryPool;
static MemoryPool
    main_pool,  // 基本メモリプール
    temp_pool;  // 一時確保用メモリプール（メモリ断片化回避のため）

/* ブロック情報 */
struct AreaInfo_ {
    uint32_t magic;     // 常にAREAINFO_MAGIC
    uint32_t free:1,    // 確保領域（0）か、空き領域（1）か
             temp:1,    // 通常プール（0）か、一時プール（1）か
             nblocks:30;// この領域のブロック数（0＝プール末尾）
    AreaInfo *prev;     // 直前の領域へのポインタ（最初の領域ではNULL）
    AreaInfo *prev_free;// 直前の空き領域へのポインタ（空き領域のみ）
    AreaInfo *next_free;// 直後の空き領域へのポインタ（空き領域のみ）
    /* 以降は確保領域の場合のみ有効 */
    uint32_t alloc_size:30, // 領域の要求サイズ（バイト）
             alloc_temp:1,  // MEM_ALLOC_TEMP指定の有無
             alloc_top:1;   // MEM_ALLOC_TOP指定の有無
    uint16_t align;     // 確保アライメント（バイト）
    uint16_t alignofs;  // 構造体の末尾からデータ領域の先頭までのオフセット
    void *base;         // mem_alloc()から返されたポインタ
#ifdef DEBUG
    const char *file;   // 所有者（確保を行った関数）のソースファイル名
    uint16_t line;      // 同、行番号
    uint16_t type;      // mem_info()用の種類番号
#endif
};
#define AREAINFO_MAGIC  0xA4EA19F0

/* 次の領域に進む */
#define NEXT_AREA(area) \
    ((AreaInfo *)((uintptr_t)(area) + ((area)->nblocks * MEM_BLOCKSIZE)))

/* 指定されたエリアがプールの末尾エリアかどうかを返す */
#define AREA_IS_FENCEPOST(area)  ((area)->nblocks == 0)

/*************************************************************************/

/* 確保・解放ログマクロ */

#if defined(DEBUG) && defined(TRACE_ALLOCS)

# define LOG_ALLOC(size,flags,ptr) \
    DMSG("[%s:%d] alloc(%d,%d) -> %p", file, line, (size), (flags), (ptr))
# define LOG_REALLOC(old,size,flags,ptr) \
    DMSG("[%s:%d] realloc(%p,%d,%d) -> %p", file, line, (old), (size), (flags), (ptr))
# define LOG_FREE(ptr) \
    DMSG("[%s:%d] free(%p)", file, line, (ptr))

#else

# define LOG_ALLOC(size,flags,ptr)        /*nothing*/
# define LOG_REALLOC(old,size,flags,ptr)  /*nothing*/
# define LOG_FREE(ptr)                    /*nothing*/

#endif

/*************************************************************************/

/* プロファイリング関連 */

#ifdef PROFILE_ALLOCS

# include <pspuser.h>

static uint32_t malloc_usec, realloc_usec, free_usec;
static uint32_t malloc_calls, realloc_calls, free_calls;

# define CHECK_PRINT_PROFILE()  do {                                    \
    if (malloc_calls + realloc_calls + free_calls >= 10000) {           \
        DMSG("[profile]  malloc: %u usec / %u calls = %u usec/call",    \
             malloc_usec, malloc_calls,                                 \
             malloc_calls ? malloc_usec / malloc_calls : 0);            \
        DMSG("[profile] realloc: %u usec / %u calls = %u usec/call",    \
             realloc_usec, realloc_calls,                               \
             realloc_calls ? realloc_usec / realloc_calls : 0);         \
        DMSG("[profile]    free: %u usec / %u calls = %u usec/call",    \
             free_usec, free_calls,                                     \
             free_calls ? free_usec / free_calls : 0);                  \
        malloc_usec  = realloc_usec  = free_usec  = 0;                  \
        malloc_calls = realloc_calls = free_calls = 0;                  \
    }                                                                   \
} while (0)

#endif

/*************************************************************************/

/* ローカル関数宣言 */

static AreaInfo *do_alloc(MemoryPool *pool,
                          uint32_t size, uint16_t align, int top);
static inline void do_free(AreaInfo *area);

static inline AreaInfo *ptr_to_area(const void *ptr);

typedef enum {SPLIT_USE_FRONT, SPLIT_USE_BACK} SplitUseSelect;
static inline AreaInfo *split_area(AreaInfo *area, uint32_t newblocks,
                                   SplitUseSelect which);
static inline void merge_free(AreaInfo *area);

static inline void mark_used(AreaInfo *area);
static inline void mark_free(AreaInfo *area);
#ifdef FREE_LIST_CHECK
static NOINLINE void free_list_check(void);
#endif

#ifdef DEBUG
static uint16_t memtype(const char *file, unsigned int line);
static void get_info(uint32_t flags, uint8_t *map, uint32_t size);
#endif

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * mem_init:  メモリ管理機能を初期化する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
int mem_init(void)
{
    if (sizeof(AreaInfo) > MEM_BLOCKSIZE) {
        DMSG("Block size %d too small for AreaInfo size %d", MEM_BLOCKSIZE,
             (int)sizeof(AreaInfo));
        return 0;
    }

    if (!main_pool.base || !main_pool.size) {
        if (UNLIKELY(!sys_mem_init(&main_pool.base, &main_pool.size,
                                   &temp_pool.base, &temp_pool.size))) {
            return 0;
        } else if (UNLIKELY(!main_pool.base) || UNLIKELY(!main_pool.size)) {
            DMSG("sys_mem_init() failed to set a main pool!");
            return 0;
        }
    }

#ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS(main_pool.base, main_pool.size);
#endif
    AreaInfo *area = (AreaInfo *)main_pool.base;
    main_pool.first_free = main_pool.last_free = area;
    AreaInfo *fencepost;
    VG_ENABLE_EMPTY_AREA(area);
    area->magic   = AREAINFO_MAGIC;
    area->free    = 1;
    area->temp    = 0;
    area->nblocks = (main_pool.size / MEM_BLOCKSIZE) - 1;
    area->prev    = NULL;
    fencepost =
        (AreaInfo *)((uintptr_t)area + (area->nblocks * MEM_BLOCKSIZE));
    VG_DISABLE_AREA(area);
    VG_ENABLE_EMPTY_AREA(fencepost);
    fencepost->magic      = AREAINFO_MAGIC;
    fencepost->free       = 0;  // merge_area()では、この末尾ブロックのfree
                                // フィールドがクリアされていることが前提
    fencepost->temp       = 0;
    fencepost->nblocks    = 0;
    fencepost->prev       = area;
    /* 以下のフィールドは実際には使われないが、一応設定しておく */
    fencepost->alloc_size = 0;
    fencepost->alloc_temp = 0;
    fencepost->alloc_top  = 0;
    fencepost->align      = 1;
    fencepost->alignofs   = 0;
    fencepost->base       = NULL;
#ifdef DEBUG
    fencepost->file       = __FILE__;
    fencepost->line       = __LINE__;
    fencepost->type       = MEM_INFO_MANAGE;
#endif

    if (temp_pool.base) {
#ifdef USE_VALGRIND
        VALGRIND_MAKE_MEM_NOACCESS(temp_pool.base, temp_pool.size);
#endif
        area = (AreaInfo *)temp_pool.base;
        temp_pool.first_free = temp_pool.last_free = area;
        VG_ENABLE_EMPTY_AREA(area);
        area->magic   = AREAINFO_MAGIC;
        area->free    = 1;
        area->temp    = 1;
        area->nblocks = (temp_pool.size / MEM_BLOCKSIZE) - 1;
        area->prev    = NULL;
        fencepost =
            (AreaInfo *)((uintptr_t)area + (area->nblocks * MEM_BLOCKSIZE));
        VG_DISABLE_AREA(area);
        VG_ENABLE_EMPTY_AREA(fencepost);
        fencepost->magic      = AREAINFO_MAGIC;
        fencepost->free       = 0;
        fencepost->temp       = 1;
        fencepost->nblocks    = 0;
        fencepost->prev       = area;
        fencepost->alloc_size = 0;
        fencepost->alloc_temp = 1;
        fencepost->alloc_top  = 0;
        fencepost->align      = 1;
        fencepost->alignofs   = 0;
        fencepost->base       = NULL;
#ifdef DEBUG
        fencepost->file       = __FILE__;
        fencepost->line       = __LINE__;
        fencepost->type       = MEM_INFO_MANAGE;
#endif
    }

    return 1;
}

/*************************************************************************/

/**
 * mem_total:  合計メモリ量を返す。
 *
 * 【引　数】flags: 0 (通常メモリ) またはMEM_ALLOC_TEMP (一時メモリ)
 * 【戻り値】合計メモリ量（バイト）
 */
uint32_t mem_total(uint32_t flags)
{
    uint32_t size = (flags & MEM_ALLOC_TEMP) ? temp_pool.size : main_pool.size;
    return (size / MEM_BLOCKSIZE - 1) * MEM_BLOCKSIZE;
}

/*************************************************************************/

/**
 * mem_avail:  現在の空きメモリ量を返す。
 *
 * 【引　数】flags: 0 (通常メモリ) またはMEM_ALLOC_TEMP (一時メモリ)
 * 【戻り値】空きメモリ量（バイト）
 */
uint32_t mem_avail(uint32_t flags)
{
    uint32_t free = 0;
    AreaInfo *area =
        (flags & MEM_ALLOC_TEMP) ? (AreaInfo *)temp_pool.first_free
                                 : (AreaInfo *)main_pool.first_free;
    AreaInfo *next_free;
    for (; area; area = next_free) {
        VG_ENABLE_DEFINED_AREA(area);
        next_free = area->next_free;
        free += area->nblocks;
        VG_DISABLE_AREA(area);
    }
    return free * MEM_BLOCKSIZE;
}

/*************************************************************************/

/**
 * mem_contig:  現在、確保し得る最大メモリ量を返す。
 *
 * 【引　数】flags: 0 (通常メモリ) またはMEM_ALLOC_TEMP (一時メモリ)
 * 【戻り値】確保し得る最大メモリ量（バイト）
 */
uint32_t mem_contig(uint32_t flags)
{
    uint32_t maxcontig = 0;
    AreaInfo *area =
        (flags & MEM_ALLOC_TEMP) ? (AreaInfo *)temp_pool.first_free
                                 : (AreaInfo *)main_pool.first_free;
    AreaInfo *next_free;
    for (; area; area = next_free) {
        VG_ENABLE_DEFINED_AREA(area);
        next_free = area->next_free;
        if (area->nblocks > maxcontig) {
            maxcontig = area->nblocks;
        }
        VG_DISABLE_AREA(area);
    }
    return maxcontig * MEM_BLOCKSIZE;
}

/*************************************************************************/
/*************************************************************************/

/**
 * mem_alloc:  メモリ領域を確保する。
 *
 * 【引　数】 size: 確保するメモリ量（バイト）
 * 　　　　　align: 確保するメモリのアライメント（バイト）。0は16と同義
 * 　　　　　flags: 動作フラグ（MEM_ALLOC_*）
 * 【戻り値】確保されたメモリへのポインタ（エラーの場合はNULL）
 */
void *DEBUG_MEM(mem_alloc)(uint32_t size, uint16_t align, uint32_t flags
                           __MEM_ALLOC_PARAMS)
{
#ifdef PROFILE_ALLOCS
    CHECK_PRINT_PROFILE();
    malloc_calls++;
    const uint32_t start = sceKernelGetSystemTimeLow();
#endif

    if (UNLIKELY(!size)) {
        DMSG("Attempt to allocate 0 bytes! (called from %s:%d)", file, line);
        return NULL;
    }
    if (!align) {
        align = 16;
    }

    /* メモリを確保する */
    AreaInfo *newarea;
    if (!(flags & MEM_ALLOC_TEMP)
     || !(newarea = do_alloc(&temp_pool, size, align, flags & MEM_ALLOC_TOP))
    ) {
        /* 一時確保で、一時プールから確保できない場合は、通常プールの末尾から
         * 確保し、他の確保との間で断片化が生じないように気を付ける */
        newarea = do_alloc(&main_pool, size, align,
                           flags & (MEM_ALLOC_TOP | MEM_ALLOC_TEMP));
    }
    if (UNLIKELY(!newarea)) {
        DMSG("Unable to allocate %u bytes", size);
#ifdef PROFILE_ALLOCS
        malloc_usec += sceKernelGetSystemTimeLow() - start;
#endif
        return NULL;
    }
    VG_ENABLE_DEFINED_AREA(newarea);
    VG_ALLOC_REGION(newarea->base, newarea->alloc_size);
    void *base = newarea->base;
    newarea->alloc_temp = (flags & MEM_ALLOC_TEMP ? 1 : 0);
    newarea->alloc_top  = (flags & MEM_ALLOC_TOP  ? 1 : 0);
#ifdef DEBUG
    newarea->file = file;
    newarea->line = line;
    newarea->type = type<0 ? memtype(file, line) : type;
#endif
    VG_DISABLE_AREA(newarea);

    /* フラグに応じてゼロクリア */
    if (flags & MEM_ALLOC_CLEAR) {
        mem_clear(base, size);
    }

    /* メモリポインタを返す */
    LOG_ALLOC(size, flags, base);
#ifdef PROFILE_ALLOCS
    malloc_usec += sceKernelGetSystemTimeLow() - start;
#endif
    return base;
}

/*************************************************************************/

/**
 * mem_realloc:  メモリ領域を拡大・縮小する。拡大・縮小が失敗した場合、元の
 * メモリ領域はそのまま残される（解放されない）。データのアライメントは維持
 * される。
 *
 * flagsにおいて、MEM_ALLOC_TEMP・MEM_ALLOC_TOPの有無が確保時と違う場合、
 * mem_alloc()→memcpy()→mem_free()と同様な動作となり、データコピーが必ず
 * 発生する。
 *
 * ptr==NULLの場合、Cライブラリのrealloc()と同様に新たにメモリを確保する。
 * mem_realloc(NULL,size,flags)はmem_alloc(size,0,flags)と同義で、アライメ
 * ントはデフォルトの16バイトとなる。
 *
 * size==0で呼び出すと、メモリを解放する（mem_free()同様）。
 *
 * 【引　数】  ptr: 現在確保されているメモリポインタ（NULLで新規確保）
 * 　　　　　 size: 確保するメモリ量（バイト、0の場合はメモリを解放）
 * 　　　　　flags: 動作フラグ（MEM_ALLOC_CLEARのみ有効）
 * 【戻り値】確保されたメモリへのポインタ（エラーまたは解放の場合はNULL）
 */
void *DEBUG_MEM(mem_realloc)(void *ptr, uint32_t size, uint32_t flags
                             __MEM_ALLOC_PARAMS)
{
    if (UNLIKELY(!ptr)) {
        return debug_mem_alloc(size, 0, flags, file, line, type);
    } else if (UNLIKELY(!size)) {
        debug_mem_free(ptr, file, line, type);
        return NULL;
    }

#ifdef PROFILE_ALLOCS
    CHECK_PRINT_PROFILE();
    realloc_calls++;
    const uint32_t start = sceKernelGetSystemTimeLow();
#endif

    /* ポインタの有効性を確認し、AreaInfoと前の領域を取得する */
    AreaInfo *area = ptr_to_area(ptr);
    if (UNLIKELY(!area)) {
        DMSG("realloc(%p,%lu,%lu): Invalid pointer! (called from %s:%d)",
             ptr, (unsigned long)size, (unsigned long)flags, file, line);
        return NULL;
    }
    VG_ENABLE_DEFINED_AREA(area);
    AreaInfo *prev = area->prev;
    const uint32_t oldsize = area->alloc_size;
#ifdef USE_VALGRIND
    static uint8_t vbits[16<<20];
    if (UNLIKELY(size > sizeof(vbits)*4)) {
        DMSG("Block size too large!!");
#ifdef PROFILE_ALLOCS
        realloc_usec += sceKernelGetSystemTimeLow() - start;
#endif
        return NULL;
    }
    VALGRIND_GET_VBITS(ptr, vbits, oldsize);
#endif

    /* MEM_ALLOC_TEMP・MEM_ALLOC_TOPが変更されている場合、再確保しなければ
     * ならない */
    if (area->alloc_temp != (flags & MEM_ALLOC_TEMP ? 1 : 0)
     || area->alloc_top  != (flags & MEM_ALLOC_TOP  ? 1 : 0)
    ) {
        const int align = area->align;
        VG_DISABLE_AREA(area);
        void *newbuf = debug_mem_alloc(size, align, flags & ~MEM_ALLOC_CLEAR,
                                       file, line, type);
        if (!newbuf) {
#ifdef PROFILE_ALLOCS
            realloc_usec += sceKernelGetSystemTimeLow() - start;
#endif
            return NULL;
        }
        if (size > oldsize) {
            memcpy(newbuf, ptr, oldsize);
            if (flags & MEM_ALLOC_CLEAR) {
                mem_clear((uint8_t *)newbuf + oldsize, size - oldsize);
            }
        } else {
            memcpy(newbuf, ptr, size);
        }
        debug_mem_free(ptr, file, line, type);
        LOG_REALLOC(ptr, size, flags, newbuf);
#ifdef PROFILE_ALLOCS
        realloc_usec += sceKernelGetSystemTimeLow() - start;
#endif
        return newbuf;
    }

    /* 新しいサイズに必要なブロック数を計算する */
    uint32_t nblocks =
        (sizeof(AreaInfo) + area->alignofs + size + MEM_BLOCKSIZE-1)
        / MEM_BLOCKSIZE;

    /* valgrind: とりあえず解放されたことにする */
    VG_FREE_REGION(area->base);

    /* 小さくする場合、サイズが変わらない場合は単純 */
    if (nblocks <= area->nblocks) {

        if (nblocks == area->nblocks) {
            /* ブロック数変更なし */
        } else {
            /* 小さくして、余ったブロックを解放する */
            (void) split_area(area, nblocks, SPLIT_USE_FRONT);
        }

    } else {  // nblocks > area->nblocks

        uint32_t addblocks = nblocks - area->nblocks;
        AreaInfo *next = NEXT_AREA(area);
        /* valgrind: next, prevへのアクセスはこのブロック中のみ */
        VG_ENABLE_DEFINED_AREA(next);
        if (AREA_IS_FENCEPOST(next)) {
            VG_DISABLE_AREA(next);
            next = NULL;
        }
        if (prev) {
            VG_ENABLE_DEFINED_AREA(prev);
        }

        /* まず、後方に空き領域がある（足りている）かをチェック */
        if (next && next->free && next->nblocks >= addblocks) {
            /* 空きが足りているので、そのまま後方へ拡大 */
            if (next->nblocks > addblocks) {
                (void) split_area(next, addblocks, SPLIT_USE_FRONT);
            } else {
                mark_used(next);
            }
            area->nblocks += addblocks;
            mem_clear(next, sizeof(*next));
            VG_DISABLE_AREA(next);
            next = NEXT_AREA(area);
            VG_ENABLE_AREA(next);
            next->prev = area;
            VG_DISABLE_AREA(next);
            if (prev) {
                VG_DISABLE_AREA(prev);
            }

        /* 後方だけでは充分な空きがないので、前方を合わせて確認 */
        } else if (prev && prev->free) {
            uint32_t totalavail = prev->nblocks + area->nblocks
                                + (next && next->free ? next->nblocks : 0);
            if (totalavail >= nblocks) {
                /* どちらか近い方へ寄せる。ALLOC_TOPの場合、前方に大きな空き
                 * 領域があるかも知れないので、それをなるべく放っておきたい
                 * のと、データ破壊対策にもなる（以下参照） */
                const int top =
                    !(next && next->free && next->nblocks > prev->nblocks);
                /* まずは前方領域、現在の領域、後方領域を1つの領域に結合 */
                uint32_t align    = area->align;
                uint32_t alignofs = area->alignofs;
                mark_free(area);
                VG_DISABLE_AREA(area);
                if (next) {
                    VG_DISABLE_AREA(next);
                }
                merge_free(prev);  // 現在の領域を結合
                merge_free(prev);  // 後方領域を結合
                /*
                 * 結合された領域を再び分割する。以前のデータ領域は必ず
                 * 新しいデータ領域に含まれているため、分割でデータが破壊
                 * される心配はない。
                 * 　・next->nblocks <= prev->nblocksの場合、nextを完全に
                 * 　　　消費した上で前方にも拡張する形となる。
                 * 　・next->nblocks > prev->nblocksの場合、前方に移動する
                 * 　　　が、拡張ブロック数がnext->nblocksを超えているため、
                 * 　　　当然prev->nblocksも超えており、prevの先頭〜nextの
                 * 　　　途中まで確保されることとなる。
                 * つまり、拡張ブロック数がnext・prevの小さい方より大きいの
                 * で、小さい方を優先的に消費することでデータ破壊が防げる。
                 */
                if (prev->nblocks == nblocks) {
                    mark_used(prev);
                    area = prev;
                } else if (top) {
                    area = split_area(prev, prev->nblocks - nblocks,
                                      SPLIT_USE_BACK);
                } else {
                    area = split_area(prev, nblocks, SPLIT_USE_FRONT);
                }
                /* データを移動する */
                area->align    = align;
                area->alignofs = alignofs;
                area->base     = (uint8_t *)area + sizeof(AreaInfo) + alignofs;
                area->alloc_size = size;
#ifdef USE_VALGRIND
                /* valgrind: 下で領域の登録を行うので、ここで重複しないように
                 * 直接memcheckのマクロを呼び出す。また、旧メモリ領域をすでに
                 * 解放しているので、一旦アクセス可能にしておく。
                 * ※UNDEFINEDとは「アクセス可能だがデータ未定義」の意。未定
                 * 　義データをコピーするだけではエラー扱いではないので無問題*/
                VALGRIND_MAKE_MEM_UNDEFINED(ptr, oldsize);
                VALGRIND_MAKE_MEM_UNDEFINED(area->base, area->alloc_size);
#endif
                memmove(area->base, ptr, oldsize);
#ifdef USE_VALGRIND
                /* valgrind: 旧領域をもう一度アクセス禁止にする */
                VALGRIND_MAKE_MEM_NOACCESS(ptr, oldsize);
#endif
            } else {  // totalavail < nblocks
                goto realloc_last_try;
            }

        /* 前方、後方を合わせても空きが足りないので、新たな領域を確保 */
        } else {
          realloc_last_try:
            if (prev) {
                VG_DISABLE_AREA(prev);
            }
            if (next) {
                VG_DISABLE_AREA(next);
            }
            /* valgrind: do_alloc()を呼び出すとAreaInfoのアクセス許可が
             * 解除される可能性があるので、アライメントを保存 */
            unsigned int oldalign = area->align;
            VG_DISABLE_AREA(area);
            AreaInfo *newarea;
            if (!(flags & MEM_ALLOC_TEMP)
             || !(newarea = do_alloc(&temp_pool, size, oldalign,
                                     flags & MEM_ALLOC_TOP))
            ) {
                newarea = do_alloc(&main_pool, size, oldalign,
                                   flags & (MEM_ALLOC_TOP | MEM_ALLOC_TEMP));
            }
            if (UNLIKELY(!newarea)) {
                DMSG("Unable to realloc %p (%u bytes) to %u bytes",
                     ptr, oldsize, size);
                /* valgrind: これで元の確保情報等が失われてしまうが、そもそも
                 * エラーなので特に気にしない */
                VG_ALLOC_REGION(ptr, oldsize);
                VG_SET_VBITS(ptr, vbits, oldsize);
#ifdef PROFILE_ALLOCS
                realloc_usec += sceKernelGetSystemTimeLow() - start;
#endif
                return NULL;
            }
            VG_ENABLE_DEFINED_AREA(newarea);
#ifdef USE_VALGRIND
            VALGRIND_MAKE_MEM_UNDEFINED(ptr, oldsize);
            VALGRIND_MAKE_MEM_UNDEFINED(newarea->base, newarea->alloc_size);
#endif
            memcpy(newarea->base, ptr, oldsize);
#ifdef USE_VALGRIND
            VALGRIND_MAKE_MEM_NOACCESS(ptr, oldsize);
#endif
            do_free(area);
            area = newarea;
        }

    }  // if (nblocks <= area->blocks)

    void *base = area->base;
    area->alloc_size = size;
    area->alloc_temp = (flags & MEM_ALLOC_TEMP ? 1 : 0);
    area->alloc_top  = (flags & MEM_ALLOC_TOP  ? 1 : 0);
#ifdef DEBUG
    area->file = file;
    area->line = line;
    area->type = type<0 ? memtype(file, line) : type;
#endif
    VG_ALLOC_REGION(area->base, area->alloc_size);
    VG_SET_VBITS(area->base, vbits, min(area->alloc_size,oldsize));
    VG_DISABLE_AREA(area);

    if (size > oldsize && (flags & MEM_ALLOC_CLEAR)) {
        mem_clear((uint8_t *)base + oldsize, size - oldsize);
    }

    LOG_REALLOC(ptr, size, flags, base);
#ifdef PROFILE_ALLOCS
    realloc_usec += sceKernelGetSystemTimeLow() - start;
#endif
    return base;
}

/*************************************************************************/

/**
 * mem_free:  メモリ領域を開放する。ptr==NULLの場合、何もしない。
 *
 * 【引　数】ptr: 解放するメモリ領域
 * 【戻り値】なし
 */
void DEBUG_MEM(mem_free)(void *ptr __MEM_ALLOC_PARAMS)
{
#ifdef PROFILE_ALLOCS
    CHECK_PRINT_PROFILE();
    free_calls++;
    const uint32_t start = sceKernelGetSystemTimeLow();
#endif

    if (ptr) {
        AreaInfo *area;
        area = ptr_to_area(ptr);
        if (UNLIKELY(!area)) {
            DMSG("free(%p): Invalid pointer! (called from %s:%d)",
                 ptr, file, line);
#ifdef PROFILE_ALLOCS
            free_usec += sceKernelGetSystemTimeLow() - start;
#endif
            return;
        }
        VG_ENABLE_DEFINED_AREA(area);
        VG_FREE_REGION(area->base);
        VG_DISABLE_AREA(area);
        do_free(area);
        LOG_FREE(ptr);
    }

#ifdef PROFILE_ALLOCS
    free_usec += sceKernelGetSystemTimeLow() - start;
#endif
}

/*************************************************************************/

/**
 * mem_strdup:  文字列を複製する。
 *
 * 【引　数】  str: 複製する文字列
 * 　　　　　flags: 動作フラグ（MEM_ALLOC_*）
 * 【戻り値】文字列の複製（エラーの場合はNULL）
 */
char *DEBUG_MEM(mem_strdup)(const char *str, uint32_t flags
                            __MEM_ALLOC_PARAMS)
{
    if (!str) {
        return NULL;
    }
    const uint32_t size = strlen(str) + 1;
    AreaInfo *area;
    if (!(flags & MEM_ALLOC_TEMP)
     || !(area = do_alloc(&temp_pool, size, 1, flags & MEM_ALLOC_TOP))
    ) {
        area = do_alloc(&main_pool, size, 1,
                        flags & (MEM_ALLOC_TOP | MEM_ALLOC_TEMP));
    }
    if (!area) {
        return NULL;
    }
    VG_ENABLE_DEFINED_AREA(area);
    VG_ALLOC_REGION(area->base, area->alloc_size);
    void *base = area->base;
    area->alloc_temp = (flags & MEM_ALLOC_TEMP ? 1 : 0);
    area->alloc_top  = (flags & MEM_ALLOC_TOP  ? 1 : 0);
#ifdef DEBUG
    area->file = file;
    area->line = line;
    area->type = type<0 ? memtype(file, line) : type;
#endif
    VG_DISABLE_AREA(area);

    memcpy(base, str, size);
    LOG_ALLOC(size, flags, base);
    return base;
}

/*************************************************************************/
/*************************************************************************/

#ifdef DEBUG

/**
 * mem_report_allocs:  確保されているメモリについて報告する。デバッグ時のみ
 * 実装。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void mem_report_allocs(void)
{
    AreaInfo *area, *next;

    DMSG("[main] %p - %p",
         main_pool.base, (uint8_t *)main_pool.base + main_pool.size);
    for (area = (AreaInfo *)main_pool.base; ; area = next) {
        VG_ENABLE_DEFINED_AREA(area);
        if (AREA_IS_FENCEPOST(area)) {
            VG_DISABLE_AREA(area);
            break;
        }
        next = NEXT_AREA(area);
        if (area->free) {
            DMSG("%p: %d bytes, free",
                 area, area->nblocks * MEM_BLOCKSIZE);
        } else {
            DMSG("%p: %d bytes, allocated at %s:%d",
                 area->base, area->alloc_size, area->file, area->line);
        }
        VG_DISABLE_AREA(area);
    }

    DMSG("[temp] %p - %p",
         temp_pool.base, (uint8_t *)temp_pool.base + temp_pool.size);
    for (area = (AreaInfo *)temp_pool.base; area; area = next) {
        VG_ENABLE_DEFINED_AREA(area);
        if (AREA_IS_FENCEPOST(area)) {
            VG_DISABLE_AREA(area);
            break;
        }
        next = NEXT_AREA(area);
        if (area->free) {
            DMSG("%p: %d bytes, free",
                 area, area->nblocks * MEM_BLOCKSIZE);
        } else {
            DMSG("%p: %d bytes, allocated at %s:%d",
                 area->base, area->alloc_size, area->file, area->line);
        }
        VG_DISABLE_AREA(area);
    }
}

/*************************************************************************/

/**
 * mem_display_debuginfo:  Ctrl+Mを監視し、押された時にメモリ使用状況を表示
 * する。デバッグ時のみ実装。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void mem_display_debuginfo(void)
{
    if (!debug_memory_display_flag) {
        return;
    }


    const int barwidth = graphics_display_width() - 128;
    static const char * const labels[] = {
        "FONT", "TEX", "SND", "FILE", "MANAGE", "MISC"
    };
    const uint32_t colors[] = {
        0x5555FF, 0xFF5555, 0x55FF55, 0xFF55FF, 0x55C6FF, 0xFFFFFF,
        0xFFFF55, 0xFFFFFF,
    };
    char buf[16];
    unsigned int x, i;

    /* 表示領域を黒にクリア */
    graphics_fill_box(0, 0, graphics_display_width(), 12, 0x80000000);

    /* 合計使用量を左側に表示（上に通常メモリ、下に一時メモリ） */
    snprintf(buf, sizeof(buf), "%u", mem_total(0) - mem_avail(0));
    debugfont_draw_text(buf, 50, 0, 0xFFFFFF, 1, 1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%u", mem_total(MEM_ALLOC_TEMP)
                                     - mem_avail(MEM_ALLOC_TEMP));
    debugfont_draw_text(buf, 50, 6, 0xFFFFFF, 1, 1, FONTSTYLE_ALIGN_RIGHT);

    /* 詳細状況を表示 */
    uint32_t sizes[8][2];
    uint8_t map_main[barwidth*16], map_temp[barwidth];
    uint32_t pixels_main[barwidth], pixels_temp[barwidth];
    const uint32_t onepixel = sizeof(map_main) / barwidth;
    mem_clear(sizes, sizeof(sizes));
    get_info(0, map_main, sizeof(map_main));
    get_info(MEM_ALLOC_TEMP, map_temp, sizeof(map_temp));

    for (x = 0; x < barwidth; x++) {
        const uint8_t *ptr = map_main + x*onepixel;
        uint32_t thissize[16][2];
        mem_clear(thissize, sizeof(thissize));
        for (i = 0; i < onepixel; i++) {
            thissize[ptr[i]>>4][0] += (ptr[i] & 0x0F) + 1;
        }
        thissize[map_temp[x]>>4][1] += (map_temp[x] & 0x0F) + 1;
        uint32_t file  = thissize[MEM_INFO_FILE]   [0] * 255 / (onepixel*16);
        uint32_t font  = thissize[MEM_INFO_FONT]   [0] * 255 / (onepixel*16);
        uint32_t tex   = thissize[MEM_INFO_TEXTURE][0] * 255 / (onepixel*16);
        uint32_t sound =(thissize[MEM_INFO_SOUND]  [0]
                       + thissize[MEM_INFO_MUSIC]  [0])* 255 / (onepixel*16);
        uint32_t manage= thissize[MEM_INFO_MANAGE] [0] * 255 / (onepixel*16);
        uint32_t other =(thissize[MEM_INFO_TEXT]   [0]
                       + thissize[MEM_INFO_UNKNOWN][0])* 255 / (onepixel*16);
        pixels_main[x] = 0xFF000000
                       | (85 + ubound(tex  +file +manage   +other,255)*2/3)<<16
                       | (85 + ubound(      file +sound*2/3+other,255)*2/3)<< 8
                       | (85 + ubound(font+manage+sound    +other,255)*2/3);
        file   = thissize[MEM_INFO_FILE]   [1] * 255 / 16;
        font   = thissize[MEM_INFO_FONT]   [1] * 255 / 16;
        tex    = thissize[MEM_INFO_TEXTURE][1] * 255 / 16;
        sound  =(thissize[MEM_INFO_SOUND]  [1]
               + thissize[MEM_INFO_MUSIC]  [1])* 255 / 16;
        manage = thissize[MEM_INFO_MANAGE] [1] * 255 / 16;
        other  =(thissize[MEM_INFO_TEXT]   [1]
               + thissize[MEM_INFO_UNKNOWN][1])* 255 / 16;
        pixels_temp[x] = 0xFF000000
                       | (85 + ubound(tex  +file +manage   +other,255)*2/3)<<16
                       | (85 + ubound(      file +sound*2/3+other,255)*2/3)<< 8
                       | (85 + ubound(font+manage+sound    +other,255)*2/3);
        sizes[0][0] += thissize[MEM_INFO_FONT]   [0];
        sizes[1][0] += thissize[MEM_INFO_TEXTURE][0];
        sizes[2][0] += thissize[MEM_INFO_SOUND]  [0];
        sizes[2][0] += thissize[MEM_INFO_MUSIC]  [0];
        sizes[3][0] += thissize[MEM_INFO_FILE]   [0];
        sizes[4][0] += thissize[MEM_INFO_MANAGE] [0];
        sizes[5][0] += thissize[MEM_INFO_TEXT]   [0];
        sizes[5][0] += thissize[MEM_INFO_UNKNOWN][0];
        sizes[0][1] += thissize[MEM_INFO_FONT]   [1];
        sizes[1][1] += thissize[MEM_INFO_TEXTURE][1];
        sizes[2][1] += thissize[MEM_INFO_SOUND]  [1];
        sizes[2][1] += thissize[MEM_INFO_MUSIC]  [1];
        sizes[3][1] += thissize[MEM_INFO_FILE]   [1];
        sizes[4][1] += thissize[MEM_INFO_MANAGE] [1];
        sizes[5][1] += thissize[MEM_INFO_TEXT]   [1];
        sizes[5][1] += thissize[MEM_INFO_UNKNOWN][1];
    }

    unsigned int x0_main = 0, x0_temp = 0;
    uint32_t last_main = pixels_main[0], last_temp = pixels_temp[0];
    for (x = 1; x < barwidth; x++) {
        if (pixels_main[x] != last_main) {
            graphics_fill_box(54 + x0_main, 0, x - x0_main, 2, last_main);
            x0_main = x;
            last_main = pixels_main[x];
        }
        if (pixels_temp[x] != last_temp) {
            graphics_fill_box(54 + x0_temp, 9, x - x0_temp, 2, last_temp);
            x0_temp = x;
            last_temp = pixels_temp[x];
        }
    }
    graphics_fill_box(54 + x0_main, 0, x - x0_main, 2, last_main);
    graphics_fill_box(54 + x0_temp, 9, x - x0_temp, 2, last_temp);

    const uint32_t sizescale = mem_total(0) / (sizeof(map_main) * 16);
    const uint32_t sizescale_temp =
        mem_total(MEM_ALLOC_TEMP) / (sizeof(map_temp) * 16);
    x = 57;
    for (i = 0; i < lenof(labels); i++) {
        snprintf(buf, sizeof(buf), "%s:%u+%u", labels[i],
                 sizes[i][0] * sizescale / 1024,
                 sizes[i][1] * sizescale_temp / 1024);
        x += debugfont_draw_text(buf, x, 3, colors[i], 1, 1, 0) + barwidth/50;
    }

    /* 空きメモリ量を表示 */
    debugfont_draw_text("FREE:", graphics_display_width()-44, 0, 0xFFFFFF, 1,
                        1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%u+", mem_avail(0) / 1024);
    debugfont_draw_text(buf, graphics_display_width()-15, 0, 0xFFFFFF, 1,
                        1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%u", mem_avail(MEM_ALLOC_TEMP) / 1024);
    debugfont_draw_text(buf, graphics_display_width(), 0, 0xFFFFFF, 1,
                        1, FONTSTYLE_ALIGN_RIGHT);

    debugfont_draw_text("MAX:", graphics_display_width()-44, 6, 0xFFFFFF, 1,
                        1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%u+", mem_contig(0) / 1024);
    debugfont_draw_text(buf, graphics_display_width()-15, 6, 0xFFFFFF, 1,
                        1, FONTSTYLE_ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), "%u", mem_contig(MEM_ALLOC_TEMP) / 1024);
    debugfont_draw_text(buf, graphics_display_width(), 6, 0xFFFFFF, 1,
                        1, FONTSTYLE_ALIGN_RIGHT);
}

#endif  // DEBUG

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * do_alloc:  メモリを確保し、新しい領域のAreaInfoポインタを返す。
 *
 * valgrind下では、AreaInfoを検索するので、VG_ENABLE_AREAでアクセス許可した
 * 領域でも不許可に変わる可能性がある。
 *
 * 【引　数】 pool: 確保を行うメモリプール
 * 　　　　　 size: 確保するメモリ量（バイト）。0は無効
 * 　　　　　align: 確保するメモリのアライメント（バイト）。0は無効
 * 　　　　　  top: メモリをプールの末尾から確保するか（0＝先頭から）
 * 【戻り値】確保されたメモリ領域のAreaInfoへのポインタ（エラーの場合はNULL）
 */
static AreaInfo *do_alloc(MemoryPool *pool,
                          uint32_t size, uint16_t align, int top)
{
    PRECOND_SOFT(pool != NULL, return NULL);
    PRECOND_SOFT(pool->base != NULL, return NULL);
    PRECOND_SOFT(size > 0, return NULL);
    PRECOND_SOFT(align > 0, return NULL);
    if (UNLIKELY(align > MEM_BLOCKSIZE)) {
        DMSG("align(%d) > blocksize(%d) not supported", align, MEM_BLOCKSIZE);
        return NULL;
    }

    /* アライメントオフセットと確保すべきブロック数を計算する */
    uint32_t alignofs;
    if (!align) {
        alignofs = 0;
    } else {
        alignofs = sizeof(AreaInfo) % align;
        if (alignofs > 0) {
            alignofs = align - alignofs;
        }
    }
    uint32_t nblocks = (sizeof(AreaInfo) + alignofs + size + MEM_BLOCKSIZE-1)
                       / MEM_BLOCKSIZE;

    /* 充分に大きい空きブロックを探す */
    AreaInfo *area, *found = NULL, *next;
    for (area = top ? pool->last_free : pool->first_free;
         area && !found; area = next
    ) {
        VG_ENABLE_DEFINED_AREA(area);
        next = top ? area->prev_free : area->next_free;
        if (area->free && area->nblocks >= nblocks) {
            found = area;
        }
        VG_DISABLE_AREA(area);
    }
    if (!found) {
        return NULL;
    }
    VG_ENABLE_DEFINED_AREA(found);

    /* 必要に応じて空き領域を分割し、新確保領域ポインタを設定する */
    AreaInfo *newarea;
    if (found->nblocks == nblocks) {
        /* 空き領域をそのまま確保領域に変換する */
        mark_used(found);
        newarea = found;
    } else if (top) {
        /* 領域の末尾から確保する */
        newarea = split_area(found, found->nblocks - nblocks, SPLIT_USE_BACK);
    } else {
        /* 領域の先頭から確保する */
        newarea = split_area(found, nblocks, SPLIT_USE_FRONT);
    }

    /* 新確保領域の情報を設定して返す */
    newarea->temp       = found->temp;
    newarea->nblocks    = nblocks;
    newarea->alloc_size = size;
    newarea->align      = align;
    newarea->alignofs   = (uint16_t)alignofs;
    newarea->base       = (uint8_t *)newarea + sizeof(AreaInfo) + alignofs;
    VG_DISABLE_AREA(newarea);
    if (found != newarea) {
        VG_DISABLE_AREA(found);
    }
    return newarea;
}

/*-----------------------------------------------------------------------*/

/**
 * do_free:  メモリ領域を解放する。前後に空き領域があれば、それらの領域を
 * 結合して一つの領域にする。
 *
 * 【引　数】area: 解放するメモリ領域
 * 【戻り値】なし
 */
static inline void do_free(AreaInfo *area)
{
    PRECOND_SOFT(area != NULL, return);
    VG_ENABLE_DEFINED_AREA(area);
    AreaInfo *prev = area->prev;
    if (prev) {
        VG_ENABLE_DEFINED_AREA(prev);
    }

    mark_free(area);
    if (prev && prev->free) {
        VG_DISABLE_AREA(area);
        merge_free(prev);
        area = prev;
    }
    merge_free(area);
    VG_DISABLE_AREA(area);
}

/*************************************************************************/

/**
 * ptr_to_area:  realloc()・free()のポインタ引数をAreaInfoに変換する。
 *
 * 【引　数】ptr: メモリ領域のベースポインタ
 * 【戻り値】当該領域のAreaInfoへのポインタ（ptrが不正の場合はNULL）
 */
static inline AreaInfo *ptr_to_area(const void *ptr)
{
    PRECOND_SOFT(ptr != NULL, return NULL);

    AreaInfo *area;

#ifdef DEEP_POINTER_CHECK

    if ((uintptr_t)ptr >= (uintptr_t)temp_pool.base
     && (uintptr_t)ptr < (uintptr_t)temp_pool.base + temp_pool.size
    ) {
        area = (AreaInfo *)temp_pool.base;
    } else {
        area = (AreaInfo *)main_pool.base;
    }
    AreaInfo *prev = NULL;
    for (;;) {
        /* valgrind: area->baseは必ずしも設定されてはいないが、baseへの
         * アクセスはfree==0の場合だけなのでDEFINEDに設定しても安全 */
        VG_ENABLE_DEFINED_AREA(area);
        if (UNLIKELY(AREA_IS_FENCEPOST(area))) {
            VG_DISABLE_AREA(area);
            return NULL;
        }
        if (!(area->free || area->base != ptr)) {
            VG_DISABLE_AREA(area);
            break;
        }
        prev = area;
        area = NEXT_AREA(area);
        VG_DISABLE_AREA(prev);
    }
    if (UNLIKELY(prev != area->prev)) {
        DMSG("prev mismatch for %p (ptr %p): area=%p found=%p", area, ptr,
             area->prev, prev);
        return NULL;
    }

#else  // !DEEP_POINTER_CHECK

    /* sizeof(AreaInfo) <= MEM_BLOCKSIZEが担保されているのと、MEM_BLOCKSIZE
     * を超えるアライメントを認めていないことから、ポインタ直前のブロックに
     * ブロック情報が格納されている */
    area = (AreaInfo *)(((uintptr_t)ptr - 1) & -MEM_BLOCKSIZE);

# ifdef POINTER_CHECK
    VG_ENABLE_DEFINED_AREA(area);
    if (UNLIKELY(area->magic != AREAINFO_MAGIC)) {
        DMSG("Bad magic at %p for ptr %p: %08X", area, ptr, area->magic);
        return NULL;
    }
    if (UNLIKELY(area->free)) {
        return NULL;
    }
    if (UNLIKELY(area->base != ptr)) {
        DMSG("ptr mismatch for %p: area=%p, ptr=%p", area, area->base, ptr);
    }
    VG_DISABLE_AREA(area);
# endif  // POINTER_CHECK

#endif  // DEEP_POINTER_CHECK

    return area;
}

/*************************************************************************/

/**
 * split_area:  空きメモリ領域を分割し、指定部分を使用中領域とする。
 *
 * valgrind下で実行されている場合、areaへのアクセスが許可されている必要が
 * あり、戻り時は返された領域へのアクセスが許可され、もう一つの領域への
 * アクセスが拒否されている。
 *
 * 【引　数】     area: 分割するメモリ領域
 * 　　　　　newblocks: areaの分割後のサイズ（ブロック単位）
 * 　　　　　    which: 使用中領域とする部分（SPLIT_USE_FRONT＝先頭部分、
 * 　　　　　           　SPLIT_USE_BACK＝末尾部分）
 * 【戻り値】whichで指定された部分の領域へのポインタ
 * 【注　意】SPLIT_USE_BACKは空き領域の分割にのみ使える。
 */
static inline AreaInfo *split_area(AreaInfo *area, uint32_t newblocks,
                                   SplitUseSelect which)
{
    PRECOND(area != NULL);
    PRECOND(newblocks > 0);
    PRECOND(newblocks < area->nblocks);
    PRECOND(which == SPLIT_USE_FRONT || area->free);

    uint32_t oldblocks = area->nblocks;
    area->nblocks = newblocks;
    AreaInfo *newarea = NEXT_AREA(area);
    VG_ENABLE_EMPTY_AREA(newarea);
    newarea->magic   = AREAINFO_MAGIC;
    newarea->temp    = area->temp;
    newarea->nblocks = oldblocks - newblocks;
    newarea->prev    = area;
    AreaInfo *next = NEXT_AREA(newarea);
    VG_ENABLE_AREA(next);
    next->prev = newarea;
    VG_DISABLE_AREA(next);

    if (which == SPLIT_USE_FRONT) {
        if (area->free) {
#ifdef FREE_LIST_CHECK
            /* まだ空きリストに追加されていないので、「リストに入っていない」
             * エラーが表示されないようにfreeフラグをクリアしておく */
            newarea->free = 0;
#endif
            mark_used(area);
        }
        mark_free(newarea);
        merge_free(newarea);
        VG_DISABLE_AREA(newarea);
        return area;
    } else {
        VG_DISABLE_AREA(area);
        /* newareaは空きリストには入っていないのでmark_used()を呼び出さない */
        newarea->free = 0;
        return newarea;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * merge_free:  空きメモリ領域を、連続している次の空きメモリ領域と結合する。
 * 指定された領域がプール末尾にあるか、次の領域が空き領域でない場合、何も
 * しない。
 *
 * valgrind下で実行されている場合、areaへのアクセスが許可されている必要がある。
 *
 * 【引　数】area: 結合する1つ目のメモリ領域
 * 【戻り値】なし
 */
static inline void merge_free(AreaInfo *area)
{
    PRECOND(area != NULL);
    PRECOND(area->free);

    AreaInfo *next = NEXT_AREA(area);
    VG_ENABLE_DEFINED_AREA(next);
    if (next->free) {  // 末尾ブロックは!freeなので、FENCEPOSTチェックが不要
        area->nblocks += next->nblocks;
        area->next_free = next->next_free;
        mem_clear(next, sizeof(*next));
        AreaInfo *next2 = NEXT_AREA(area);
        VG_ENABLE_AREA(next2);
        next2->prev = area;
        VG_DISABLE_AREA(next2);
        if (area->next_free) {
            VG_ENABLE_AREA(area->next_free);
            area->next_free->prev_free = area;
            VG_DISABLE_AREA(area->next_free);
        } else {
            MemoryPool *pool;
            if ((uintptr_t)area >= (uintptr_t)temp_pool.base
             && (uintptr_t)area < (uintptr_t)temp_pool.base + temp_pool.size
            ) {
                pool = &temp_pool;
            } else {
                pool = &main_pool;
            }
            pool->last_free = area;
        }
    }
    VG_DISABLE_AREA(next);
}

/*************************************************************************/

/**
 * mark_used:  指定されたメモリ領域の空きフラグをクリアし、空き領域リストを
 * 更新する。
 *
 * valgrind下で実行されている場合、areaへのアクセスが許可されている必要がある。
 *
 * 【引　数】area: 空きフラグをセットする領域
 * 【戻り値】なし
 * 【前条件】領域が空きリストに入っていること。
 */
static inline void mark_used(AreaInfo *area)
{
    PRECOND(area != NULL);
    PRECOND(area->free);

    MemoryPool *pool = area->temp ? &temp_pool : &main_pool;

    area->free = 0;

    if (area->prev_free) {
        VG_ENABLE_DEFINED_AREA(area->prev_free);
        area->prev_free->next_free = area->next_free;
        VG_DISABLE_AREA(area->prev_free);
    } else {
        pool->first_free = area->next_free;
    }
    if (area->next_free) {
        VG_ENABLE_DEFINED_AREA(area->next_free);
        area->next_free->prev_free = area->prev_free;
        VG_DISABLE_AREA(area->next_free);
    } else {
        pool->last_free = area->prev_free;
    }

#ifdef DEBUG
    /* 使用中領域の空きリストポインタを使うとクラッシュするように設定 */
    area->prev_free = area->next_free = (AreaInfo *) -1;
#endif

#ifdef FREE_LIST_CHECK
    free_list_check();
#endif
}

/*-----------------------------------------------------------------------*/

/**
 * mark_free:  指定されたメモリ領域の空きフラグをセットし、空き領域リストを
 * 更新する。
 *
 * valgrind下で実行されている場合、areaへのアクセスが許可されている必要がある。
 *
 * 【引　数】area: 空きフラグをセットする領域
 * 【戻り値】なし
 * 【前条件】・領域が空きリストに入っていないこと。
 * 　　　　　・area->prevが正しく設定されていること。
 */
static inline void mark_free(AreaInfo *area)
{
    PRECOND(area != NULL);

    MemoryPool *pool = area->temp ? &temp_pool : &main_pool;

    area->free = 1;

    if (!pool->first_free) {
        PRECOND(!pool->last_free);
        area->prev_free = NULL;
        area->next_free = NULL;
        pool->first_free = pool->last_free = area;

    } else if (area < pool->first_free) {
        area->prev_free = NULL;
        area->next_free = pool->first_free;
        pool->first_free->prev_free = area;
        pool->first_free = area;

    } else if (area > pool->last_free) {
        area->prev_free = pool->last_free;
        area->next_free = NULL;
        pool->last_free->next_free = area;
        pool->last_free = area;

    } else {
        AreaInfo *prev_free;
        for (prev_free = area->prev; prev_free; prev_free = prev_free->prev) {
            VG_ENABLE_DEFINED_AREA(prev_free);
            const int is_free = prev_free->free;
            VG_DISABLE_AREA(prev_free);
            if (is_free) {
                break;
            }
        }
        /* すでにリスト先頭・末尾でないことを確認している */
        PRECOND(prev_free != NULL);
        PRECOND(prev_free->next_free != NULL);
        area->prev_free = prev_free;
        area->next_free = prev_free->next_free;
        VG_ENABLE_DEFINED_AREA(area->prev_free);
        area->prev_free->next_free = area;
        VG_DISABLE_AREA(area->prev_free);
        VG_ENABLE_DEFINED_AREA(area->next_free);
        area->next_free->prev_free = area;
        VG_DISABLE_AREA(area->next_free);
    }

#ifdef FREE_LIST_CHECK
    free_list_check();
#endif
}

/*-----------------------------------------------------------------------*/

#ifdef FREE_LIST_CHECK

/**
 * free_list_check:  空き領域リストが正しく作成されていることを確認する。
 *
 * FREE_LIST_CHECK定義時のみ定義。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
static NOINLINE void free_list_check(void)
{
    MemoryPool *pool;

    for (pool = &main_pool; pool;
         pool = (pool==&main_pool ? &temp_pool : NULL)
    ) {

        if ((pool->first_free && !pool->last_free)
         || (!pool->first_free && pool->last_free)
        ) {
            DMSG("%s pool (%p): NULLness of first_free(%p) and last_free(%p)"
                 " doesn't match", pool==&main_pool ? "Main" : "Temporary",
                 pool->base, pool->first_free, pool->last_free);
            for (;;) sys_time_delay(1);
        }
        if (pool->first_free) {
            VG_ENABLE_DEFINED_AREA(pool->first_free);
            if (pool->first_free->prev_free != NULL) {
                DMSG("%s pool (%p): first_free(%p)->prev_free != NULL",
                     pool==&main_pool ? "Main" : "Temporary", pool->base,
                     pool->first_free);
                for (;;) sys_time_delay(1);
            }
            VG_DISABLE_AREA(pool->first_free);
        }
        if (pool->last_free) {
            VG_ENABLE_DEFINED_AREA(pool->last_free);
            if (pool->last_free->next_free != NULL) {
                DMSG("%s pool (%p): last_free(%p)->next_free != NULL",
                     pool==&main_pool ? "Main" : "Temporary", pool->base,
                     pool->first_free);
                for (;;) sys_time_delay(1);
            }
            VG_DISABLE_AREA(pool->last_free);
        }

        AreaInfo *area, *next, *free_area;
        for (area = (AreaInfo *)pool->base, free_area = pool->first_free;
             area && free_area; area = next
        ) {
            VG_ENABLE_DEFINED_AREA(area);
            if (area < free_area) {
                if (area->free) {
                    DMSG("%s pool (%p): Free area %p is not on free list",
                         pool==&main_pool ? "Main" : "Temporary", pool->base,
                         area);
                    for (;;) sys_time_delay(1);
                }
            } else if (area == free_area) {
                if (!area->free) {
                    DMSG("%s pool (%p): In-use area %p is on free list",
                         pool==&main_pool ? "Main" : "Temporary", pool->base,
                         area);
                    for (;;) sys_time_delay(1);
                }
                if (area->next_free) {
                    if (area->next_free < area) {
                        DMSG("%s pool (%p): %p->next_free(%p) is out of order",
                             pool==&main_pool ? "Main" : "Temporary",
                             pool->base, area, area->next_free);
                        for (;;) sys_time_delay(1);
                    }
                    VG_ENABLE_DEFINED_AREA(area->next_free);
                    if (area->next_free->prev_free != area) {
                        DMSG("%s pool (%p): %p->next_free(%p)->prev_free(%p)"
                             " doesn't match",
                             pool==&main_pool ? "Main" : "Temporary",
                             pool->base, area, area->next_free,
                             area->next_free->prev_free);
                        for (;;) sys_time_delay(1);
                    }
                    VG_DISABLE_AREA(area->next_free);
                }
                free_area = area->next_free;
            } else {  // area > free_area
                DMSG("%s pool (%p): Free list entry %p is not a valid area",
                     pool==&main_pool ? "Main" : "Temporary", pool->base,
                     free_area);
                for (;;) sys_time_delay(1);
            }
            next = NEXT_AREA(area);
            VG_DISABLE_AREA(area);
        }

        if (free_area) {
            DMSG("%s pool (%p): Free list contains area %p not in pool",
                 pool==&main_pool ? "Main" : "Temporary", pool->base,
                 free_area);
            for (;;) sys_time_delay(1);
        }

    }  // for (pool)
}

#endif  // FREE_LIST_CHECK

/*************************************************************************/

#ifdef DEBUG

/**
 * memtype:  ソースファイル名から種類番号（MEM_INFO_*）を判定して返す。
 *
 * 【引　数】file: ソースファイル名
 * 　　　　　line: ソース行番号
 * 【戻り値】メモリ種類番号（MEM_INFO_*）
 */
static uint16_t memtype(const char *file, unsigned int line)
{
    if (!file) {
        return MEM_INFO_UNKNOWN;
    } else if (strcmp(file, "src/sysdep-psp/files.c") == 0
            || strcmp(file, "src/dirent.c") == 0
            || strcmp(file, "src/stdio.c") == 0) {
        return MEM_INFO_FILE;
    } else if (strcmp(file, "src/debugfont.c") == 0) {
        return MEM_INFO_FONT;
    } else if (strcmp(file, "src/texture.c") == 0) {
        return MEM_INFO_TEXTURE;
    } else if (strncmp(file, "src/sound/", 10) == 0
               || strncmp(file, "src/sysdep-psp/sound", 20) == 0
               || strstr(file, "FmodPSPBridge") != NULL) {
        return MEM_INFO_SOUND;
    } else if (strncmp(file, "src/resource/", 13) == 0) {
        return MEM_INFO_MANAGE;
    } else {
        //DMSG("Unknown %s:%d", file, line);
        return MEM_INFO_UNKNOWN;
    }
}

/*************************************************************************/

/**
 * get_info:  メモリプールの使用状況を返す。プールをsize個に均等分割し、
 * それぞれの部分について、対応するmapのバイトに使用状況を格納する。使用状
 * 況は、下位4ビットに確保割合（0＝1/16以下、15＝15/16超）、上位4ビットに
 * メモリの用途（MEM_INFO_*）を設定する。
 *
 * デバッグ時のみ実装。size次第で、実行に相当な時間がかかるので要注意。
 *
 * 【引　数】flags: 0 (通常メモリ) またはMEM_ALLOC_TEMP (一時メモリ)
 * 　　　　　  map: 使用状況を格納するバッファ
 * 　　　　　 size: mapのサイズ（バイト）
 * 【戻り値】なし
 */
static void get_info(uint32_t flags, uint8_t *map, uint32_t size)
{
    MemoryPool *pool = flags & MEM_ALLOC_TEMP ? &temp_pool : &main_pool;
    const uintptr_t poolbase = (uintptr_t)pool->base;
    const uint32_t poolstep = pool->size / size;

    const AreaInfo *area = (AreaInfo *)poolbase;
    int i;
    for (i = 0; i < size; i++) {
        const uintptr_t bottom = poolbase + ((i  ) * poolstep);
        const uintptr_t top    = poolbase + ((i+1) * poolstep);
        uint32_t used[16], free;
        mem_clear(used, sizeof(used));
        free = 0;
        for (;;) {
            VG_ENABLE_DEFINED_AREA(area);
            if (AREA_IS_FENCEPOST(area)) {
                VG_DISABLE_AREA(area);
                break;
            }
            const uintptr_t aptr = (const uintptr_t)area;
            const uintptr_t aend = aptr + (area->nblocks * MEM_BLOCKSIZE);
            const uint32_t areasize =
                (aend>top ? top : aend) - (aptr<bottom ? bottom : aptr);
            if (area->free) {
                free += areasize;
            } else {
                used[area->type & 15] += areasize;
            }
            if (aend > top) {
                break;
            }
            AreaInfo *next = NEXT_AREA(area);
            VG_DISABLE_AREA(area);
            area = next;
        }
        if (free == top - bottom) {
            map[i] = 0;
        } else {
            int j, type = MEM_INFO_UNKNOWN;
            for (j = 0; j < lenof(used); j++) {
                /* 使用中メモリ量に半分超の種類があれば返す */
                if (used[j] > (top - bottom - free) / 2) {
                    type = j;
                }
            }
            map[i] = (type<<4) | (15 - (16*free / (top - bottom)));
        }
    }  // for (i = 0..size)
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
