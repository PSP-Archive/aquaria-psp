/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/test.h: Header file for testing functions.
 */

#ifndef TEST_H
#define TEST_H

/*************************************************************************/

/******** test.c ********/

/**
 * run_all_tests:  全てのテストを実行し、その結果を返す。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝1つ以上のテストが失敗した
 */
extern int run_all_tests(void);


/******** test-basic.c ********/

/**
 * test_endian:  エンディアン操作関数の動作をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
extern int test_endian(void);

/**
 * test_snprintf:  snprintf()の動作をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
extern int test_snprintf(void);


/******** test-decompress.c ********/

/**
 * test_decompress:  圧縮データ解凍関数の動作をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
extern int test_decompress(void);


/******** test-math.c ********/

/**
 * test_dtrig:  度単位の三角関数の動作をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
extern int test_dtrig(void);

/**
 * test_vector:  ベクトル関数をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
extern int test_vector(void);

/**
 * test_matrix:  行列関数をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
extern int test_matrix(void);

/**
 * test_intersect:  2直線交差関数をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
extern int test_intersect(void);


/******** test-memory.c ********/

/**
 * test_memory:  メモリ操作関数の動作をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
extern int test_memory(void);


/*************************************************************************/

#endif  // TEST_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
