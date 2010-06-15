/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/test/test-math.c: Test routines for mathematical functions.
 */

#include "../common.h"

#ifdef INCLUDE_TESTS  // ファイル末尾まで

#include "../memory.h"
#include "../test.h"

/*************************************************************************/

/* メモリ管理用のブロックサイズ（memory.cのMEM_BLOCKSIZE） */
#define BLOCKSIZE  64

/* メイン・一時プールのベースアドレスとサイズ */
static uint8_t *main_base, *temp_base;
static uint32_t main_size, temp_size;

/* メモリ管理テストが失敗した場合、その失敗が以降のテストにも悪影響を与える
 * 可能性があるので、失敗した時点でエラーを返す */
#define FAIL(msg,...) do {              \
    DMSG("FAIL: " msg , ## __VA_ARGS__);\
    return 0;                           \
} while (0)

/* 空きメモリ量を確認するマクロ */
#define CHECK_FREE_MEMORY(main_avail,main_contig,temp_avail,temp_contig) do { \
    const uint32_t __main_avail  = (main_avail);                        \
    const uint32_t __main_contig = (main_contig);                       \
    const uint32_t __temp_avail  = (temp_avail);                        \
    const uint32_t __temp_contig = (temp_contig);                       \
    if (mem_avail(0) != __main_avail) {                                 \
        FAIL("mem_avail(MAIN) %u != %u", mem_avail(0), __main_avail);   \
    }                                                                   \
    if (mem_contig(0) != __main_contig) {                               \
        FAIL("mem_contig(MAIN) %u != %u", mem_contig(0), __main_contig);\
    }                                                                   \
    if (mem_avail(MEM_ALLOC_TEMP) != __temp_avail) {                    \
        FAIL("mem_avail(TEMP) %u != %u", mem_avail(MEM_ALLOC_TEMP),     \
             __temp_avail);                                             \
    }                                                                   \
    if (mem_contig(MEM_ALLOC_TEMP) != __temp_contig) {                  \
        FAIL("mem_contig(TEMP) %u != %u", mem_contig(MEM_ALLOC_TEMP),   \
             __temp_contig);                                            \
    }                                                                   \
} while(0)

/* 各種テストを実行する補助関数 */
static int test_memory_alloc_align(void);
static int test_memory_alloc_top(void);
static int test_memory_alloc_clear(void);
static int test_memory_alloc_full_pool(int poolflag, int failover);
static int test_memory_realloc(void);
static int test_memory_realloc_clear(void);
static int test_memory_realloc_move(void);

/*************************************************************************/
/*************************************************************************/

/**
 * test_memory:  メモリ操作関数の動作をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
int test_memory(void)
{
    /* テストに使うポインタ変数 */
    uint8_t *ptr, *ptr2;


    /* まずはメモリ量を取得し、全てのメモリが空いていることを確認する */
#ifdef CXX_CONSTRUCTOR_HACK  // すでに一部確保されているかも
    main_size = mem_avail(0);
#else
    main_size = mem_total(0);
#endif
    temp_size = mem_total(MEM_ALLOC_TEMP);
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    /* 確保時のアライメントを確認する */
    if (!test_memory_alloc_align()) {
        return 0;
    }
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    /* メモリブロックを確保することで、各メモリプールの先頭アドレスを取得
     * する。なお、この処理をはじめ、以下多くのテストはメモリ管理システム
     * の内部処理方法にある程度依存している。 */
    ptr = mem_alloc(BLOCKSIZE, BLOCKSIZE, 0);
    if (!ptr) {
        FAIL("mem_alloc(%d,%d,MAIN) failed!", BLOCKSIZE, BLOCKSIZE);
    }
    main_base = ptr - BLOCKSIZE;
    ptr2 = mem_alloc(BLOCKSIZE, BLOCKSIZE, MEM_ALLOC_TEMP);
    if (!ptr2) {
        FAIL("mem_alloc(%d,%d,MAIN) failed!", BLOCKSIZE, BLOCKSIZE);
    }
    temp_base = ptr2 - BLOCKSIZE;
    /* ついでに、メモリの空き容量が減少していることも確認する */
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size - BLOCKSIZE*2, temp_size - BLOCKSIZE*2);
    /* 一時ブロックを解放し、空き容量が元に戻っていることを確認する */
    mem_free(ptr);
    mem_free(ptr2);
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    /********/

    /* MEM_ALLOC_TOPの動作を確認する */
    if (!test_memory_alloc_top()) {
        return 0;
    }
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    /* 確保時クリア（MEM_ALLOC_CLEAR）の動作を確認する */
    if (!test_memory_alloc_clear()) {
        return 0;
    }
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    /* メモリ不足時の動作を確認する（メインプール） */
    if (!test_memory_alloc_full_pool(0, 0)) {
        FAIL("alloc_full_pool failed for MAIN");
    }
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    /* メモリ不足時の動作を確認する（一時プール）。メインプールの空き状態に
     * よって動作が変わるので、メインプール満杯・空きありの両方でテストする */
    ptr = mem_alloc(main_size - BLOCKSIZE, BLOCKSIZE, 0);
    if (!ptr) {
        FAIL("failed to alloc all memory from main pool");
    }
    if (!test_memory_alloc_full_pool(MEM_ALLOC_TEMP, 0)) {
        FAIL("alloc_full_pool failed for TEMP (0)");
    }
    mem_free(ptr);
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);
    /* そしてメインプールを空けておいたまま、もう一回 */
    if (!test_memory_alloc_full_pool(MEM_ALLOC_TEMP, 1)) {
        FAIL("alloc_full_pool failed for TEMP (0)");
    }
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    /********/

    /* mem_realloc()の基本動作を確認する */
    if (!test_memory_realloc()) {
        return 0;
    }
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    /* mem_realloc()におけるMEM_ALLOC_CLEARの動作を確認する */
    if (!test_memory_realloc_clear()) {
        return 0;
    }
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    /* mem_realloc()時のフラグ変更によるメモリ領域移動の動作を確認する */
    if (!test_memory_realloc_move()) {
        return 0;
    }
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    /********/

    /* 全テスト成功 */
    return 1;
}

/*************************************************************************/

/**
 * test_memory_alloc_align:  確保アライメント指定の動作を確認する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
static int test_memory_alloc_align(void)
{
    int i;
    for (i = 1; i <= BLOCKSIZE; i <<= 1) {
        uint8_t *ptr = mem_alloc(i, i, 0);
        if (!ptr) {
            FAIL("failed to alloc block for testing alignment %d", i);
        }
        if ((uintptr_t)ptr & (i-1)) {
            FAIL("pointer %p not aligned to %d bytes", ptr, i);
        }
        mem_free(ptr);
    }

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * test_memory_alloc_top:  MEM_ALLOC_TOPフラグの動作を確認する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
static int test_memory_alloc_top(void)
{
    uint8_t *ptr;

    ptr = mem_alloc(BLOCKSIZE, BLOCKSIZE, MEM_ALLOC_TOP);
    if (ptr != main_base + (main_size - BLOCKSIZE)) {
        FAIL("mem_alloc(MAIN|TOP) failed: returned %p, should be %p+0x%x"
             " = %p", ptr, main_base, main_size - BLOCKSIZE,
             main_base + (main_size - BLOCKSIZE));
    }
    mem_free(ptr);

    ptr = mem_alloc(BLOCKSIZE, BLOCKSIZE, MEM_ALLOC_TEMP | MEM_ALLOC_TOP);
    if (ptr != temp_base + (temp_size - BLOCKSIZE)) {
        FAIL("mem_alloc(TEMP|TOP) failed: returned %p, should be %p+0x%x"
             " = %p", ptr, temp_base, temp_size - BLOCKSIZE,
             temp_base + (temp_size - BLOCKSIZE));
    }
    mem_free(ptr);

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * test_memory_alloc_clear:  MEM_ALLOC_CLEARフラグの動作を確認する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
static int test_memory_alloc_clear(void)
{
    uint8_t *ptr, *ptr2;
    int i;

    ptr = mem_alloc(BLOCKSIZE, BLOCKSIZE, 0);
    if (!ptr) {
        FAIL("mem_alloc() failed");
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        ptr[i] = i+1;
    }
    mem_free(ptr);

    ptr2 = mem_alloc(BLOCKSIZE, BLOCKSIZE, MEM_ALLOC_CLEAR);
    if (!ptr2) {
        FAIL("mem_alloc() failed 2");
    }
    if (ptr2 != ptr) {
        FAIL("didn't get the same pointer!  %p, was %p", ptr2, ptr);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        if (ptr[i] != 0) {
            FAIL("byte %d is not zero", i);
        }
    }
    mem_free(ptr2);

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * test_memory_alloc_full_pool:  メモリプール満杯時のmem_alloc()動作を確認
 * する。
 *
 * 【引　数】poolflag: メモリプールを指定する確保フラグ（0かMEM_ALLOC_TEMP）
 * 　　　　　failover: 0以外＝一時確保をメインプールから確保する動作を確認
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
static int test_memory_alloc_full_pool(int poolflag, int failover)
{
    const char *poolid = poolflag ? "TEMP" : "MAIN";
    uint8_t *ptr, *ptr2;

    /* 空きが全くない場合 */
    ptr = mem_alloc((poolflag ? temp_size : main_size) - BLOCKSIZE,
                    BLOCKSIZE, poolflag);
    if (!ptr) {
        FAIL("failed to alloc all memory from %s pool", poolid);
    }
    if (poolflag && ptr != temp_base+BLOCKSIZE) {
        FAIL("temp alloc didn't come from temp pool (%p, should be %p)",
            ptr, temp_base+BLOCKSIZE);
    }
    CHECK_FREE_MEMORY(!poolflag || !failover ? 0 : main_size,
                      !poolflag || !failover ? 0 : main_size,
                      poolflag ? 0 : temp_size, poolflag ? 0 : temp_size);
    ptr2 = mem_alloc(1, 1, poolflag);
    if (failover) {
        if (!ptr2) {
            FAIL("attempt to alloc(1,1,TEMP) from full pool didn't fail over");
        }
        if (ptr2 < main_base + main_size - BLOCKSIZE
         || ptr2 >= main_base + main_size
        ) {
            FAIL("alloc(1,1,TEMP) from full pool gave bad pointer: %p"
                 " (should be between %p and %p inclusive)", ptr2,
                 main_base + main_size - BLOCKSIZE, main_base + main_size - 1);
        }
        mem_free(ptr2);
    } else {
        if (ptr2) {
            FAIL("attempt to alloc(1,1,%s) from full pool succeeded! (%p)",
                 poolid, ptr2);
        }
    }
    mem_free(ptr);
    CHECK_FREE_MEMORY(poolflag && !failover ? 0 : main_size,
                      poolflag && !failover ? 0 : main_size,
                      temp_size, temp_size);

    /* 1ブロックのみ残っている場合 */
    ptr = mem_alloc((poolflag ? temp_size : main_size) - BLOCKSIZE*2,
                    BLOCKSIZE, poolflag);
    if (!ptr) {
        FAIL("failed to alloc almost all memory from %s pool", poolid);
    }
    if (poolflag && ptr != temp_base+BLOCKSIZE) {
        FAIL("temp alloc didn't come from temp pool (%p, should be %p)",
            ptr, temp_base+BLOCKSIZE);
    }
    ptr2 = mem_alloc(1, 1, poolflag);
    if (!ptr2) {
        FAIL("failed to alloc 1 byte in last block of %s pool", poolid);
    }
    mem_free(ptr2);
    ptr2 = mem_alloc(BLOCKSIZE, 1, poolflag);
    if (failover) {
        if (!ptr2) {
            FAIL("attempt to alloc(%d,1,TEMP) from almost-full pool didn't"
                 " fail over", BLOCKSIZE);
        }
        if (ptr2 < main_base + main_size - BLOCKSIZE*2
         || ptr2 >= main_base + main_size
        ) {
            FAIL("alloc(%d,1,TEMP) from full pool gave bad pointer: %p"
                 " (should be between %p and %p inclusive)", BLOCKSIZE, ptr2,
                 main_base + main_size - BLOCKSIZE*2,
                 main_base + main_size - 1);
        }
        mem_free(ptr2);
    } else {
        if (ptr2) {
            FAIL("attempt to alloc(%d,1,%s) from almost-full pool succeeded!"
                 " (%p)", BLOCKSIZE, poolid, ptr2);
        }
    }
    ptr2 = mem_alloc(1, BLOCKSIZE, poolflag);
    if (failover) {
        if (!ptr2) {
            FAIL("attempt to alloc(1,%d,TEMP) from almost-full pool didn't"
                 " fail over", BLOCKSIZE);
        }
        if (ptr2 < main_base + main_size - BLOCKSIZE*2
         || ptr2 >= main_base + main_size
        ) {
            FAIL("alloc(1,%d,TEMP) from full pool gave bad pointer: %p"
                 " (should be between %p and %p inclusive)", BLOCKSIZE, ptr2,
                 main_base + main_size - BLOCKSIZE*2,
                 main_base + main_size - 1);
        }
        mem_free(ptr2);
    } else {
        if (ptr2) {
            FAIL("attempt to alloc(1,%d,%s) from almost-full pool succeeded!"
                 " (%p)", BLOCKSIZE, poolid, ptr2);
        }
    }
    mem_free(ptr);

    return 1;
}

/*************************************************************************/

/**
 * test_memory_realloc:  mem_realloc()の基本動作を確認する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
static int test_memory_realloc(void)
{
    uint8_t *ptr, *ptr2;
    int i;

    ptr = mem_alloc(BLOCKSIZE, BLOCKSIZE, 0);
    if (ptr != main_base + BLOCKSIZE) {
        FAIL("mem_alloc() failed: returned %p, should be %p",
             ptr, main_base + BLOCKSIZE);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        ptr[i] = i+1;
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size, temp_size);

    /* ブロック数を増やす */
    ptr2 = mem_realloc(ptr, BLOCKSIZE*3/2, 0);
    if (ptr2 != ptr) {
        FAIL("realloc(%p,%d) failed: returned %p, should be %p",
             ptr, BLOCKSIZE*2, ptr2, ptr);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    for (i = BLOCKSIZE; i < BLOCKSIZE*3/2; i++) {
        ptr[i] = i+1;
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*3, main_size - BLOCKSIZE*3,
                      temp_size, temp_size);

    /* ブロック数を変更せずバイト数を増やす */
    ptr2 = mem_realloc(ptr, BLOCKSIZE*2, 0);
    if (ptr2 != ptr) {
        FAIL("realloc(%p,%d) failed: returned %p, should be %p",
             ptr, BLOCKSIZE*2, ptr2, ptr);
    }
    for (i = 0; i < BLOCKSIZE*3/2; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*3, main_size - BLOCKSIZE*3,
                      temp_size, temp_size);

    /* ブロック数を変更せずバイト数を減らす */
    ptr2 = mem_realloc(ptr, BLOCKSIZE*3/2, 0);
    if (ptr2 != ptr) {
        FAIL("realloc(%p,%d) failed: returned %p, should be %p",
             ptr, BLOCKSIZE*2, ptr2, ptr);
    }
    for (i = 0; i < BLOCKSIZE*3/2; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*3, main_size - BLOCKSIZE*3,
                      temp_size, temp_size);

    /* ブロック数を減らす */
    ptr2 = mem_realloc(ptr, BLOCKSIZE, 0);
    if (ptr2 != ptr) {
        FAIL("realloc(%p,%d) failed: returned %p, should be %p",
             ptr, BLOCKSIZE, ptr2, ptr);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size, temp_size);

    /* サイズ0により領域を解放する */
    mem_realloc(ptr, 0, 0);
    CHECK_FREE_MEMORY(main_size, main_size, temp_size, temp_size);

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * test_memory_realloc_clear:  mem_realloc(...,MEM_ALLOC_CLEAR)の動作を確認
 * する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
static int test_memory_realloc_clear(void)
{
    uint8_t *ptr, *ptr2;
    int i;

    ptr = mem_alloc(BLOCKSIZE, BLOCKSIZE, 0);
    if (ptr != main_base + BLOCKSIZE) {
        FAIL("mem_alloc() failed: returned %p, should be %p",
             ptr, main_base + BLOCKSIZE);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        ptr[i] = i+1;
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size, temp_size);

    /* ここでCLEARは無意味だが、データ破壊が起きないことを確認するために指定 */
    ptr2 = mem_realloc(ptr, BLOCKSIZE/2, MEM_ALLOC_CLEAR);
    if (ptr2 != ptr) {
        FAIL("realloc(%p,%d) failed: returned %p, should be %p",
             ptr, BLOCKSIZE*2, ptr2, ptr);
    }
    for (i = 0; i < BLOCKSIZE/2; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size, temp_size);

    ptr2 = mem_realloc(ptr, BLOCKSIZE, MEM_ALLOC_CLEAR);
    if (ptr2 != ptr) {
        FAIL("realloc(%p,%d) failed: returned %p, should be %p",
             ptr, BLOCKSIZE, ptr2, ptr);
    }
    for (i = 0; i < BLOCKSIZE/2; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    for (i = BLOCKSIZE/2; i < BLOCKSIZE; i++) {
        if (ptr[i] != 0) {
            FAIL("realloc failed to clear byte at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size, temp_size);

    mem_free(ptr);
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * test_memory_realloc_move:  mem_realloc()のフラグ変更による領域移動動作を
 * 確認する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
static int test_memory_realloc_move(void)
{
    uint8_t *ptr;
    int i;

    ptr = mem_alloc(BLOCKSIZE, BLOCKSIZE, 0);
    if (ptr != main_base + BLOCKSIZE) {
        FAIL("mem_alloc() failed: returned %p, should be %p",
             ptr, main_base + BLOCKSIZE);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        ptr[i] = i+1;
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size, temp_size);

    /* TOPフラグ変更による移動を確認する。管理フラグの不更新なども考えられる
     * ので、TOPに変更した後、もう一度非TOPに戻す */
    ptr = mem_realloc(ptr, BLOCKSIZE, MEM_ALLOC_TOP);
    if (ptr != main_base + main_size - BLOCKSIZE) {
        FAIL("realloc MAIN/bottom -> MAIN/top failed: returned %p, should"
             " be %p+0x%x = %p", ptr, main_base, main_size - BLOCKSIZE,
             main_base + main_size - BLOCKSIZE);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size, temp_size);
    ptr = mem_realloc(ptr, BLOCKSIZE, 0);
    if (ptr != main_base + BLOCKSIZE) {
        FAIL("realloc MAIN/top -> MAIN/bottom failed: returned %p, should"
             " be %p", ptr, main_base + BLOCKSIZE);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size, temp_size);

    /* メモリプール変更による移動を確認する */
    ptr = mem_realloc(ptr, BLOCKSIZE, MEM_ALLOC_TEMP);
    if (ptr != temp_base + BLOCKSIZE) {
        FAIL("realloc MAIN/bottom -> TEMP/bottom failed: returned %p, should"
             " be %p", ptr, temp_base + BLOCKSIZE);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size, main_size,
                      temp_size - BLOCKSIZE*2, temp_size - BLOCKSIZE*2);
    ptr = mem_realloc(ptr, BLOCKSIZE, 0);
    if (ptr != main_base + BLOCKSIZE) {
        FAIL("realloc TEMP/bottom -> MAIN/bottom failed: returned %p, should"
             " be %p", ptr, main_base + BLOCKSIZE);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size, temp_size);

    /* TOPフラグとメモリプールの同時変更による移動を確認する */
    ptr = mem_realloc(ptr, BLOCKSIZE, MEM_ALLOC_TEMP | MEM_ALLOC_TOP);
    if (ptr != temp_base + temp_size - BLOCKSIZE) {
        FAIL("realloc MAIN/bottom -> TEMP/top failed: returned %p, should"
             " be %p+0x%x = %p", ptr, temp_base, temp_size - BLOCKSIZE,
             temp_base + temp_size - BLOCKSIZE);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size, main_size,
                      temp_size - BLOCKSIZE*2, temp_size - BLOCKSIZE*2);
    ptr = mem_realloc(ptr, BLOCKSIZE, 0);
    if (ptr != main_base + BLOCKSIZE) {
        FAIL("realloc TEMP/top -> MAIN/bottom failed: returned %p, should"
             " be %p", ptr, main_base + BLOCKSIZE);
    }
    for (i = 0; i < BLOCKSIZE; i++) {
        if (ptr[i] != i+1) {
            FAIL("realloc corrupted data at offset %d", i);
        }
    }
    CHECK_FREE_MEMORY(main_size - BLOCKSIZE*2, main_size - BLOCKSIZE*2,
                      temp_size, temp_size);

    mem_free(ptr);
    return 1;
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
