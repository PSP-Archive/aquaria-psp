/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/test/test-basic.c: Basic test routines.
 */

#include "../common.h"

#ifdef INCLUDE_TESTS  // ファイル末尾まで

#include "../test.h"

/*************************************************************************/
/*************************************************************************/

/**
 * test_endian:  エンディアン操作関数の動作をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
int test_endian(void)
{
    int failed = 0;

    /* エンディアン性の検知をテストする */
    DMSG("is_little_endian() = %d", is_little_endian());
    union {
        char c[4];
        uint32_t i;
    } buf = {.c = {0x12, 0x34, 0x56, 0x78}};
    if (buf.i != (is_little_endian() ? 0x78563412 : 0x12345678)) {
        DMSG("FAIL: endian test");
        failed = 1;
    }

    /* ビッグエンディアン→ネイティブ変換をテストする */
    const int16_t s16_native  = -0x1234;
    const int16_t s16_be      = (is_little_endian() ? -0x3313 : -0x1234);
    const uint16_t u16_native = 0xCDEF;
    const uint16_t u16_be     = (is_little_endian() ? 0xEFCD : 0xCDEF);
    const int32_t s32_native  = -0x12345678;
    const int32_t s32_be      = (is_little_endian() ? -0x77563413:-0x12345678);
    const uint32_t u32_native = 0x89ABCDEF;
    const uint32_t u32_be     = (is_little_endian() ? 0xEFCDAB89 : 0x89ABCDEF);
    const float float_native  = 1.0f;
    const union {uint32_t i; float f;} float_be =
        {.i = (is_little_endian() ? 0x0000803F : 0x3F800000)};
    /* 戻り値の符号を確認するため、16ビット値を32ビット比較、32ビット値を64
     * ビット比較する */
    if ((int32_t)s16_native != (int32_t)be_to_s16(s16_be)) {
        DMSG("FAIL: be_to_s16");
        failed = 1;
    }
    if ((int32_t)u16_native != (int32_t)be_to_u16(u16_be)) {
        DMSG("FAIL: be_to_u16");
        failed = 1;
    }
    if ((int64_t)s32_native != (int64_t)be_to_s32(s32_be)) {
        DMSG("FAIL: be_to_s32");
        failed = 1;
    }
    if ((int64_t)u32_native != (int64_t)be_to_u32(u32_be)) {
        DMSG("FAIL: be_to_u32");
        failed = 1;
    }
    if (float_native != be_to_float(float_be.f)) {
        DMSG("FAIL: be_to_float");
        failed = 1;
    }
    if ((int32_t)s16_be != (int32_t)s16_to_be(s16_native)) {
        DMSG("FAIL: s16_to_be");
        failed = 1;
    }
    if ((int32_t)u16_be != (int32_t)u16_to_be(u16_native)) {
        DMSG("FAIL: u16_to_be");
        failed = 1;
    }
    if ((int64_t)s32_be != (int64_t)s32_to_be(s32_native)) {
        DMSG("FAIL: s32_to_be");
        failed = 1;
    }
    if ((int64_t)u32_be != (int64_t)u32_to_be(u32_native)) {
        DMSG("FAIL: u32_to_be");
        failed = 1;
    }
    const union {float f; uint32_t i;} float_test =
        {.f = float_to_be(float_native)};
    if (float_be.i != float_test.i) {
        DMSG("FAIL: float_to_be");
        failed = 1;
    }

    return !failed;
}

/*************************************************************************/

/**
 * test_snprintf:  snprintf()の動作をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
int test_snprintf(void)
{
    int failed = 0;
    char buf[1000];

    /* TRY_SNPRINTF: snprintf()をテストして、戻り値と結果文字列を期待値と
     * 比較する。結果文字列がbufに収まると想定され、戻り値の期待値は
     * strlen(expect)となる。expectは文字列定数でなければならない。 */
#define TRY_SNPRINTF(expect,format,...) \
    TRY_SNPRINTF_EX(strlen((expect)), (expect), sizeof(buf), (format) , ## __VA_ARGS__)

    /* TRY_SNPRINTF_EX: snprintf()をテストして、結果文字列を期待値と比較す
     * る。戻り値の期待値をexpect_retvalで、snprintf()のバッファサイズ引数
     * をbufsizeで指定する。補助関数ではなくマクロとするのは、DMSG()で出力
     * される行番号から失敗したテストを容易に特定できるようにするため。 */
#define TRY_SNPRINTF_EX(expect_retval,expect,bufsize,format,...)  do {  \
    const char *__expect = (expect);                                    \
    const int __expect_retval = (expect_retval);                        \
    const int __bufsize = (bufsize);                                    \
    /* メモリ破壊を検知するため、バッファを予め特定の値でフィルする */  \
    mem_fill8(buf, 0xBE, sizeof(buf));                                  \
    const int retval = snprintf(buf, __bufsize, format , ## __VA_ARGS__); \
    if (retval != __expect_retval) {                                    \
        DMSG("FAIL: bad return value (%d, expected %d)",                \
             retval, __expect_retval);                                  \
        failed = 1;                                                     \
    } else if (strcmp(buf, __expect) != 0) {                            \
        DMSG("FAIL: bad result string ([%s], expected [%s])", buf, __expect);\
        failed = 1;                                                     \
    } else {                                                            \
        const char *s;                                                  \
        for (s = buf + strlen(buf) + 1; s < buf + sizeof(buf); s++) {   \
            if (*s != (char)0xBE) {                                     \
                DMSG("FAIL: memory corruption at relative offset %d",   \
                     (int)(s - (buf + strlen(buf) + 1)));               \
                failed = 1;                                             \
                break;                                                  \
            }                                                           \
        }                                                               \
    }                                                                   \
} while (0)

    /* 指定子無しの文字列をまずチェック。メモリ破壊を検知するため、バッファ
     * をまず特定の値でフィルする */
    const char *emptystr = "";  // コンパイラ警告回避
    TRY_SNPRINTF("", emptystr, "");  // 最後の空文字列はコンパイラ警告回避用
    TRY_SNPRINTF("abcde", "abcde");

    /* バッファオーバフローが起こらないかチェック */
    TRY_SNPRINTF_EX(5, "ab", 3, "abcde");

    /* %% */
    TRY_SNPRINTF("%", "%%");

    /* %c */
    TRY_SNPRINTF("0", "%c", 0x30);
    TRY_SNPRINTF("¡", "%c", 0xA1);
    TRY_SNPRINTF("グ", "%c", 0x30B0);

    /* %dと、%*...による幅指定 */
    TRY_SNPRINTF("-123", "%d", -123);
    TRY_SNPRINTF("  123", "%5d", 123);
    TRY_SNPRINTF("123  ", "%-5d", 123);
    TRY_SNPRINTF("  123", "%*d", 5, 123);
    TRY_SNPRINTF("123  ", "%*d", -5, 123);
    TRY_SNPRINTF("123456", "%ld", 123456L);
    TRY_SNPRINTF("12345678901", "%lld", 12345678901LL);

    /* %+d (Aquariaでは未実装。実装したらFIXME) */
#if 0
    current_lang = LANG_EN;
    TRY_SNPRINTF("123", "%+d", 123);
    TRY_SNPRINTF("  123", "%+5d", 123);
    TRY_SNPRINTF("00123", "%+05d", 123);
    current_lang = LANG_JA;
    TRY_SNPRINTF("１２３", "%+d", 123);
    TRY_SNPRINTF("　　１２３", "%+5d", 123);
    TRY_SNPRINTF("００１２３", "%+05d", 123);
#endif

    /* %f */
    TRY_SNPRINTF("1.234560", "%f",    1.23456);
    TRY_SNPRINTF(" 1.23456", "%8.5f", 1.23456);
    TRY_SNPRINTF("  1.2346", "%8.4f", 1.23456);
    TRY_SNPRINTF("   1.235", "%8.3f", 1.23456);
    TRY_SNPRINTF("    1.23", "%8.2f", 1.23456);
    TRY_SNPRINTF("       1", "%8.0f", 1.23456);
    TRY_SNPRINTF("  inf", "%5f", 1.0/0.0);
    TRY_SNPRINTF(" -inf", "%5f", -1.0/0.0);
    TRY_SNPRINTF("  nan", "%5f", 0.0/0.0);

    /* %i */
    TRY_SNPRINTF("123", "%i", 123);
    TRY_SNPRINTF("00123", "%05i", 123);

    /* %o */
    TRY_SNPRINTF("173", "%o", 123);

    /* %p */
    TRY_SNPRINTF("0x12345678", "%p", (void *)0x12345678);
    TRY_SNPRINTF("(null)", "%p", NULL);

    /* %s */
    const char *nullstr = NULL;  // コンパイラ警告を回避
    TRY_SNPRINTF("test", "%s", "test");
    TRY_SNPRINTF("(null)", "%s", nullstr);

    /* %u */
    TRY_SNPRINTF("123", "%u", 123);

    /* %x */
    TRY_SNPRINTF("7b", "%x", 123);

    /* %X */
    TRY_SNPRINTF("7B", "%X", 123);
    TRY_SNPRINTF("FEDCBA9876543210", "%llX", 0xFEDCBA9876543210ULL);

    return !failed;

#undef TRY_SNPRINTF
#undef TRY_SNPRINTF_EX
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
