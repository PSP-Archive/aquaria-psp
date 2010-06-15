/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/test/test.c: Interface function for running tests.
 */

#include "../common.h"

#ifdef INCLUDE_TESTS  // ファイル末尾まで

#include "../test.h"

/*************************************************************************/
/*************************************************************************/

/**
 * run_all_tests:  全てのテストを実行し、その結果を返す。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝1つ以上のテストが失敗した
 */
int run_all_tests(void)
{
    static struct {
        const char *name;
        int (*testfunc)(void);
        int result;
    } tests[] = {
#define DEFINE_TEST(name)  {#name, test_##name}
        DEFINE_TEST(endian),
        DEFINE_TEST(snprintf),
        DEFINE_TEST(dtrig),
        DEFINE_TEST(vector),
        DEFINE_TEST(matrix),
        DEFINE_TEST(intersect),
        DEFINE_TEST(memory),
        DEFINE_TEST(decompress),
#undef DEFINE_TEST
    };

    int result = 1;
    int i;
    for (i = 0; i < lenof(tests); i++) {
        tests[i].result = (*tests[i].testfunc)();
        if (!tests[i].result) {
            result = 0;
        }
    }

    DMSG("======== TEST RESULTS ========");
    if (result) {
        DMSG("All tests passed.");
    } else {
        for (i = 0; i < lenof(tests); i++) {
            if (tests[i].result) {
                DMSG("    %s: passed", tests[i].name);
            } else {
                DMSG("[*] %s: FAILED", tests[i].name);
            }
        }
    }
    DMSG("==============================");

    return result;
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
