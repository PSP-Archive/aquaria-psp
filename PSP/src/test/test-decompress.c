/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/test/test-decompress.c: Test routines for decompression code.
 */

#include "../common.h"

#ifdef INCLUDE_TESTS  // ファイル末尾まで

#include "../test.h"
#include "../resource/tinflate.h"

/*************************************************************************/

/* ローカル関数宣言 */
static int test_decompress_one(const uint8_t * const in,
                               const int32_t in_size,
                               uint8_t * const out,
                               const int32_t out_size,
                               const long expected_result,
                               const unsigned long expected_crc32,
                               const uint8_t * const test,
                               const int32_t test_size);

/*************************************************************************/
/*************************************************************************/

/**
 * test_decompress:  圧縮データ解凍関数の動作をテストする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝全テストが成功した、0＝一つ以上のテストが失敗した
 */
int test_decompress(void)
{
    static const struct {
        uint16_t line;          // ソースコードの行番号
        int8_t out_size;        // 使用する出力バッファサイズ（バイト）
        int8_t result;          // 関数戻り値の期待値
        unsigned long crc32;    // CRC32の期待値（失敗するテストでは0）
        const char *in;         // 圧縮データ
        int32_t in_size;        // 圧縮データ長（バイト）
        const char *test;       // 期待出力データ
        int32_t test_size;      // 期待出力データ長（バイト）
    } tests[] = {

#define T(params...) {__LINE__, params}

        /**** 正常系 ****/

        /* ゼロサイズデータ */
        T(0, 0, 0x00000000, "\x03\x00", 2, NULL, 0),
        /* 同、RFC1950ヘッダ有り */
        T(0, 0, 0x00000000, "\x78\x01\x03\x00", 4, NULL, 0),

        /* 非圧縮データ */
        T(5, 5, 0x8587D865, "\x01\x05\x00\xFA\xFF" "abcde", 10, "abcde", 5),

        /* 1バイトデータ・静的テーブル（全バイト値） */
        T(1, 1, 0xD202EF8D, "\x63\x00\x00", 3, "\x00", 1),
        T(1, 1, 0xA505DF1B, "\x63\x04\x00", 3, "\x01", 1),
        T(1, 1, 0x3C0C8EA1, "\x63\x02\x00", 3, "\x02", 1),
        T(1, 1, 0x4B0BBE37, "\x63\x06\x00", 3, "\x03", 1),
        T(1, 1, 0xD56F2B94, "\x63\x01\x00", 3, "\x04", 1),
        T(1, 1, 0xA2681B02, "\x63\x05\x00", 3, "\x05", 1),
        T(1, 1, 0x3B614AB8, "\x63\x03\x00", 3, "\x06", 1),
        T(1, 1, 0x4C667A2E, "\x63\x07\x00", 3, "\x07", 1),
        T(1, 1, 0xDCD967BF, "\xE3\x00\x00", 3, "\x08", 1),
        T(1, 1, 0xABDE5729, "\xE3\x04\x00", 3, "\x09", 1),
        T(1, 1, 0x32D70693, "\xE3\x02\x00", 3, "\x0A", 1),
        T(1, 1, 0x45D03605, "\xE3\x06\x00", 3, "\x0B", 1),
        T(1, 1, 0xDBB4A3A6, "\xE3\x01\x00", 3, "\x0C", 1),
        T(1, 1, 0xACB39330, "\xE3\x05\x00", 3, "\x0D", 1),
        T(1, 1, 0x35BAC28A, "\xE3\x03\x00", 3, "\x0E", 1),
        T(1, 1, 0x42BDF21C, "\xE3\x07\x00", 3, "\x0F", 1),
        T(1, 1, 0xCFB5FFE9, "\x13\x00\x00", 3, "\x10", 1),
        T(1, 1, 0xB8B2CF7F, "\x13\x04\x00", 3, "\x11", 1),
        T(1, 1, 0x21BB9EC5, "\x13\x02\x00", 3, "\x12", 1),
        T(1, 1, 0x56BCAE53, "\x13\x06\x00", 3, "\x13", 1),
        T(1, 1, 0xC8D83BF0, "\x13\x01\x00", 3, "\x14", 1),
        T(1, 1, 0xBFDF0B66, "\x13\x05\x00", 3, "\x15", 1),
        T(1, 1, 0x26D65ADC, "\x13\x03\x00", 3, "\x16", 1),
        T(1, 1, 0x51D16A4A, "\x13\x07\x00", 3, "\x17", 1),
        T(1, 1, 0xC16E77DB, "\x93\x00\x00", 3, "\x18", 1),
        T(1, 1, 0xB669474D, "\x93\x04\x00", 3, "\x19", 1),
        T(1, 1, 0x2F6016F7, "\x93\x02\x00", 3, "\x1A", 1),
        T(1, 1, 0x58672661, "\x93\x06\x00", 3, "\x1B", 1),
        T(1, 1, 0xC603B3C2, "\x93\x01\x00", 3, "\x1C", 1),
        T(1, 1, 0xB1048354, "\x93\x05\x00", 3, "\x1D", 1),
        T(1, 1, 0x280DD2EE, "\x93\x03\x00", 3, "\x1E", 1),
        T(1, 1, 0x5F0AE278, "\x93\x07\x00", 3, "\x1F", 1),
        T(1, 1, 0xE96CCF45, "\x53\x00\x00", 3, "\x20", 1),
        T(1, 1, 0x9E6BFFD3, "\x53\x04\x00", 3, "\x21", 1),
        T(1, 1, 0x0762AE69, "\x53\x02\x00", 3, "\x22", 1),
        T(1, 1, 0x70659EFF, "\x53\x06\x00", 3, "\x23", 1),
        T(1, 1, 0xEE010B5C, "\x53\x01\x00", 3, "\x24", 1),
        T(1, 1, 0x99063BCA, "\x53\x05\x00", 3, "\x25", 1),
        T(1, 1, 0x000F6A70, "\x53\x03\x00", 3, "\x26", 1),
        T(1, 1, 0x77085AE6, "\x53\x07\x00", 3, "\x27", 1),
        T(1, 1, 0xE7B74777, "\xD3\x00\x00", 3, "\x28", 1),
        T(1, 1, 0x90B077E1, "\xD3\x04\x00", 3, "\x29", 1),
        T(1, 1, 0x09B9265B, "\xD3\x02\x00", 3, "\x2A", 1),
        T(1, 1, 0x7EBE16CD, "\xD3\x06\x00", 3, "\x2B", 1),
        T(1, 1, 0xE0DA836E, "\xD3\x01\x00", 3, "\x2C", 1),
        T(1, 1, 0x97DDB3F8, "\xD3\x05\x00", 3, "\x2D", 1),
        T(1, 1, 0x0ED4E242, "\xD3\x03\x00", 3, "\x2E", 1),
        T(1, 1, 0x79D3D2D4, "\xD3\x07\x00", 3, "\x2F", 1),
        T(1, 1, 0xF4DBDF21, "\x33\x00\x00", 3, "\x30", 1),
        T(1, 1, 0x83DCEFB7, "\x33\x04\x00", 3, "\x31", 1),
        T(1, 1, 0x1AD5BE0D, "\x33\x02\x00", 3, "\x32", 1),
        T(1, 1, 0x6DD28E9B, "\x33\x06\x00", 3, "\x33", 1),
        T(1, 1, 0xF3B61B38, "\x33\x01\x00", 3, "\x34", 1),
        T(1, 1, 0x84B12BAE, "\x33\x05\x00", 3, "\x35", 1),
        T(1, 1, 0x1DB87A14, "\x33\x03\x00", 3, "\x36", 1),
        T(1, 1, 0x6ABF4A82, "\x33\x07\x00", 3, "\x37", 1),
        T(1, 1, 0xFA005713, "\xB3\x00\x00", 3, "\x38", 1),
        T(1, 1, 0x8D076785, "\xB3\x04\x00", 3, "\x39", 1),
        T(1, 1, 0x140E363F, "\xB3\x02\x00", 3, "\x3A", 1),
        T(1, 1, 0x630906A9, "\xB3\x06\x00", 3, "\x3B", 1),
        T(1, 1, 0xFD6D930A, "\xB3\x01\x00", 3, "\x3C", 1),
        T(1, 1, 0x8A6AA39C, "\xB3\x05\x00", 3, "\x3D", 1),
        T(1, 1, 0x1363F226, "\xB3\x03\x00", 3, "\x3E", 1),
        T(1, 1, 0x6464C2B0, "\xB3\x07\x00", 3, "\x3F", 1),
        T(1, 1, 0xA4DEAE1D, "\x73\x00\x00", 3, "\x40", 1),
        T(1, 1, 0xD3D99E8B, "\x73\x04\x00", 3, "\x41", 1),
        T(1, 1, 0x4AD0CF31, "\x73\x02\x00", 3, "\x42", 1),
        T(1, 1, 0x3DD7FFA7, "\x73\x06\x00", 3, "\x43", 1),
        T(1, 1, 0xA3B36A04, "\x73\x01\x00", 3, "\x44", 1),
        T(1, 1, 0xD4B45A92, "\x73\x05\x00", 3, "\x45", 1),
        T(1, 1, 0x4DBD0B28, "\x73\x03\x00", 3, "\x46", 1),
        T(1, 1, 0x3ABA3BBE, "\x73\x07\x00", 3, "\x47", 1),
        T(1, 1, 0xAA05262F, "\xF3\x00\x00", 3, "\x48", 1),
        T(1, 1, 0xDD0216B9, "\xF3\x04\x00", 3, "\x49", 1),
        T(1, 1, 0x440B4703, "\xF3\x02\x00", 3, "\x4A", 1),
        T(1, 1, 0x330C7795, "\xF3\x06\x00", 3, "\x4B", 1),
        T(1, 1, 0xAD68E236, "\xF3\x01\x00", 3, "\x4C", 1),
        T(1, 1, 0xDA6FD2A0, "\xF3\x05\x00", 3, "\x4D", 1),
        T(1, 1, 0x4366831A, "\xF3\x03\x00", 3, "\x4E", 1),
        T(1, 1, 0x3461B38C, "\xF3\x07\x00", 3, "\x4F", 1),
        T(1, 1, 0xB969BE79, "\x0B\x00\x00", 3, "\x50", 1),
        T(1, 1, 0xCE6E8EEF, "\x0B\x04\x00", 3, "\x51", 1),
        T(1, 1, 0x5767DF55, "\x0B\x02\x00", 3, "\x52", 1),
        T(1, 1, 0x2060EFC3, "\x0B\x06\x00", 3, "\x53", 1),
        T(1, 1, 0xBE047A60, "\x0B\x01\x00", 3, "\x54", 1),
        T(1, 1, 0xC9034AF6, "\x0B\x05\x00", 3, "\x55", 1),
        T(1, 1, 0x500A1B4C, "\x0B\x03\x00", 3, "\x56", 1),
        T(1, 1, 0x270D2BDA, "\x0B\x07\x00", 3, "\x57", 1),
        T(1, 1, 0xB7B2364B, "\x8B\x00\x00", 3, "\x58", 1),
        T(1, 1, 0xC0B506DD, "\x8B\x04\x00", 3, "\x59", 1),
        T(1, 1, 0x59BC5767, "\x8B\x02\x00", 3, "\x5A", 1),
        T(1, 1, 0x2EBB67F1, "\x8B\x06\x00", 3, "\x5B", 1),
        T(1, 1, 0xB0DFF252, "\x8B\x01\x00", 3, "\x5C", 1),
        T(1, 1, 0xC7D8C2C4, "\x8B\x05\x00", 3, "\x5D", 1),
        T(1, 1, 0x5ED1937E, "\x8B\x03\x00", 3, "\x5E", 1),
        T(1, 1, 0x29D6A3E8, "\x8B\x07\x00", 3, "\x5F", 1),
        T(1, 1, 0x9FB08ED5, "\x4B\x00\x00", 3, "\x60", 1),
        T(1, 1, 0xE8B7BE43, "\x4B\x04\x00", 3, "\x61", 1),
        T(1, 1, 0x71BEEFF9, "\x4B\x02\x00", 3, "\x62", 1),
        T(1, 1, 0x06B9DF6F, "\x4B\x06\x00", 3, "\x63", 1),
        T(1, 1, 0x98DD4ACC, "\x4B\x01\x00", 3, "\x64", 1),
        T(1, 1, 0xEFDA7A5A, "\x4B\x05\x00", 3, "\x65", 1),
        T(1, 1, 0x76D32BE0, "\x4B\x03\x00", 3, "\x66", 1),
        T(1, 1, 0x01D41B76, "\x4B\x07\x00", 3, "\x67", 1),
        T(1, 1, 0x916B06E7, "\xCB\x00\x00", 3, "\x68", 1),
        T(1, 1, 0xE66C3671, "\xCB\x04\x00", 3, "\x69", 1),
        T(1, 1, 0x7F6567CB, "\xCB\x02\x00", 3, "\x6A", 1),
        T(1, 1, 0x0862575D, "\xCB\x06\x00", 3, "\x6B", 1),
        T(1, 1, 0x9606C2FE, "\xCB\x01\x00", 3, "\x6C", 1),
        T(1, 1, 0xE101F268, "\xCB\x05\x00", 3, "\x6D", 1),
        T(1, 1, 0x7808A3D2, "\xCB\x03\x00", 3, "\x6E", 1),
        T(1, 1, 0x0F0F9344, "\xCB\x07\x00", 3, "\x6F", 1),
        T(1, 1, 0x82079EB1, "\x2B\x00\x00", 3, "\x70", 1),
        T(1, 1, 0xF500AE27, "\x2B\x04\x00", 3, "\x71", 1),
        T(1, 1, 0x6C09FF9D, "\x2B\x02\x00", 3, "\x72", 1),
        T(1, 1, 0x1B0ECF0B, "\x2B\x06\x00", 3, "\x73", 1),
        T(1, 1, 0x856A5AA8, "\x2B\x01\x00", 3, "\x74", 1),
        T(1, 1, 0xF26D6A3E, "\x2B\x05\x00", 3, "\x75", 1),
        T(1, 1, 0x6B643B84, "\x2B\x03\x00", 3, "\x76", 1),
        T(1, 1, 0x1C630B12, "\x2B\x07\x00", 3, "\x77", 1),
        T(1, 1, 0x8CDC1683, "\xAB\x00\x00", 3, "\x78", 1),
        T(1, 1, 0xFBDB2615, "\xAB\x04\x00", 3, "\x79", 1),
        T(1, 1, 0x62D277AF, "\xAB\x02\x00", 3, "\x7A", 1),
        T(1, 1, 0x15D54739, "\xAB\x06\x00", 3, "\x7B", 1),
        T(1, 1, 0x8BB1D29A, "\xAB\x01\x00", 3, "\x7C", 1),
        T(1, 1, 0xFCB6E20C, "\xAB\x05\x00", 3, "\x7D", 1),
        T(1, 1, 0x65BFB3B6, "\xAB\x03\x00", 3, "\x7E", 1),
        T(1, 1, 0x12B88320, "\xAB\x07\x00", 3, "\x7F", 1),
        T(1, 1, 0x3FBA6CAD, "\x6B\x00\x00", 3, "\x80", 1),
        T(1, 1, 0x48BD5C3B, "\x6B\x04\x00", 3, "\x81", 1),
        T(1, 1, 0xD1B40D81, "\x6B\x02\x00", 3, "\x82", 1),
        T(1, 1, 0xA6B33D17, "\x6B\x06\x00", 3, "\x83", 1),
        T(1, 1, 0x38D7A8B4, "\x6B\x01\x00", 3, "\x84", 1),
        T(1, 1, 0x4FD09822, "\x6B\x05\x00", 3, "\x85", 1),
        T(1, 1, 0xD6D9C998, "\x6B\x03\x00", 3, "\x86", 1),
        T(1, 1, 0xA1DEF90E, "\x6B\x07\x00", 3, "\x87", 1),
        T(1, 1, 0x3161E49F, "\xEB\x00\x00", 3, "\x88", 1),
        T(1, 1, 0x4666D409, "\xEB\x04\x00", 3, "\x89", 1),
        T(1, 1, 0xDF6F85B3, "\xEB\x02\x00", 3, "\x8A", 1),
        T(1, 1, 0xA868B525, "\xEB\x06\x00", 3, "\x8B", 1),
        T(1, 1, 0x360C2086, "\xEB\x01\x00", 3, "\x8C", 1),
        T(1, 1, 0x410B1010, "\xEB\x05\x00", 3, "\x8D", 1),
        T(1, 1, 0xD80241AA, "\xEB\x03\x00", 3, "\x8E", 1),
        T(1, 1, 0xAF05713C, "\xEB\x07\x00", 3, "\x8F", 1),
        T(1, 1, 0x220D7CC9, "\x9B\x00\x00", 3, "\x90", 1),
        T(1, 1, 0x550A4C5F, "\x9B\x08\x00", 3, "\x91", 1),
        T(1, 1, 0xCC031DE5, "\x9B\x04\x00", 3, "\x92", 1),
        T(1, 1, 0xBB042D73, "\x9B\x0C\x00", 3, "\x93", 1),
        T(1, 1, 0x2560B8D0, "\x9B\x02\x00", 3, "\x94", 1),
        T(1, 1, 0x52678846, "\x9B\x0A\x00", 3, "\x95", 1),
        T(1, 1, 0xCB6ED9FC, "\x9B\x06\x00", 3, "\x96", 1),
        T(1, 1, 0xBC69E96A, "\x9B\x0E\x00", 3, "\x97", 1),
        T(1, 1, 0x2CD6F4FB, "\x9B\x01\x00", 3, "\x98", 1),
        T(1, 1, 0x5BD1C46D, "\x9B\x09\x00", 3, "\x99", 1),
        T(1, 1, 0xC2D895D7, "\x9B\x05\x00", 3, "\x9A", 1),
        T(1, 1, 0xB5DFA541, "\x9B\x0D\x00", 3, "\x9B", 1),
        T(1, 1, 0x2BBB30E2, "\x9B\x03\x00", 3, "\x9C", 1),
        T(1, 1, 0x5CBC0074, "\x9B\x0B\x00", 3, "\x9D", 1),
        T(1, 1, 0xC5B551CE, "\x9B\x07\x00", 3, "\x9E", 1),
        T(1, 1, 0xB2B26158, "\x9B\x0F\x00", 3, "\x9F", 1),
        T(1, 1, 0x04D44C65, "\x5B\x00\x00", 3, "\xA0", 1),
        T(1, 1, 0x73D37CF3, "\x5B\x08\x00", 3, "\xA1", 1),
        T(1, 1, 0xEADA2D49, "\x5B\x04\x00", 3, "\xA2", 1),
        T(1, 1, 0x9DDD1DDF, "\x5B\x0C\x00", 3, "\xA3", 1),
        T(1, 1, 0x03B9887C, "\x5B\x02\x00", 3, "\xA4", 1),
        T(1, 1, 0x74BEB8EA, "\x5B\x0A\x00", 3, "\xA5", 1),
        T(1, 1, 0xEDB7E950, "\x5B\x06\x00", 3, "\xA6", 1),
        T(1, 1, 0x9AB0D9C6, "\x5B\x0E\x00", 3, "\xA7", 1),
        T(1, 1, 0x0A0FC457, "\x5B\x01\x00", 3, "\xA8", 1),
        T(1, 1, 0x7D08F4C1, "\x5B\x09\x00", 3, "\xA9", 1),
        T(1, 1, 0xE401A57B, "\x5B\x05\x00", 3, "\xAA", 1),
        T(1, 1, 0x930695ED, "\x5B\x0D\x00", 3, "\xAB", 1),
        T(1, 1, 0x0D62004E, "\x5B\x03\x00", 3, "\xAC", 1),
        T(1, 1, 0x7A6530D8, "\x5B\x0B\x00", 3, "\xAD", 1),
        T(1, 1, 0xE36C6162, "\x5B\x07\x00", 3, "\xAE", 1),
        T(1, 1, 0x946B51F4, "\x5B\x0F\x00", 3, "\xAF", 1),
        T(1, 1, 0x19635C01, "\xDB\x00\x00", 3, "\xB0", 1),
        T(1, 1, 0x6E646C97, "\xDB\x08\x00", 3, "\xB1", 1),
        T(1, 1, 0xF76D3D2D, "\xDB\x04\x00", 3, "\xB2", 1),
        T(1, 1, 0x806A0DBB, "\xDB\x0C\x00", 3, "\xB3", 1),
        T(1, 1, 0x1E0E9818, "\xDB\x02\x00", 3, "\xB4", 1),
        T(1, 1, 0x6909A88E, "\xDB\x0A\x00", 3, "\xB5", 1),
        T(1, 1, 0xF000F934, "\xDB\x06\x00", 3, "\xB6", 1),
        T(1, 1, 0x8707C9A2, "\xDB\x0E\x00", 3, "\xB7", 1),
        T(1, 1, 0x17B8D433, "\xDB\x01\x00", 3, "\xB8", 1),
        T(1, 1, 0x60BFE4A5, "\xDB\x09\x00", 3, "\xB9", 1),
        T(1, 1, 0xF9B6B51F, "\xDB\x05\x00", 3, "\xBA", 1),
        T(1, 1, 0x8EB18589, "\xDB\x0D\x00", 3, "\xBB", 1),
        T(1, 1, 0x10D5102A, "\xDB\x03\x00", 3, "\xBC", 1),
        T(1, 1, 0x67D220BC, "\xDB\x0B\x00", 3, "\xBD", 1),
        T(1, 1, 0xFEDB7106, "\xDB\x07\x00", 3, "\xBE", 1),
        T(1, 1, 0x89DC4190, "\xDB\x0F\x00", 3, "\xBF", 1),
        T(1, 1, 0x49662D3D, "\x3B\x00\x00", 3, "\xC0", 1),
        T(1, 1, 0x3E611DAB, "\x3B\x08\x00", 3, "\xC1", 1),
        T(1, 1, 0xA7684C11, "\x3B\x04\x00", 3, "\xC2", 1),
        T(1, 1, 0xD06F7C87, "\x3B\x0C\x00", 3, "\xC3", 1),
        T(1, 1, 0x4E0BE924, "\x3B\x02\x00", 3, "\xC4", 1),
        T(1, 1, 0x390CD9B2, "\x3B\x0A\x00", 3, "\xC5", 1),
        T(1, 1, 0xA0058808, "\x3B\x06\x00", 3, "\xC6", 1),
        T(1, 1, 0xD702B89E, "\x3B\x0E\x00", 3, "\xC7", 1),
        T(1, 1, 0x47BDA50F, "\x3B\x01\x00", 3, "\xC8", 1),
        T(1, 1, 0x30BA9599, "\x3B\x09\x00", 3, "\xC9", 1),
        T(1, 1, 0xA9B3C423, "\x3B\x05\x00", 3, "\xCA", 1),
        T(1, 1, 0xDEB4F4B5, "\x3B\x0D\x00", 3, "\xCB", 1),
        T(1, 1, 0x40D06116, "\x3B\x03\x00", 3, "\xCC", 1),
        T(1, 1, 0x37D75180, "\x3B\x0B\x00", 3, "\xCD", 1),
        T(1, 1, 0xAEDE003A, "\x3B\x07\x00", 3, "\xCE", 1),
        T(1, 1, 0xD9D930AC, "\x3B\x0F\x00", 3, "\xCF", 1),
        T(1, 1, 0x54D13D59, "\xBB\x00\x00", 3, "\xD0", 1),
        T(1, 1, 0x23D60DCF, "\xBB\x08\x00", 3, "\xD1", 1),
        T(1, 1, 0xBADF5C75, "\xBB\x04\x00", 3, "\xD2", 1),
        T(1, 1, 0xCDD86CE3, "\xBB\x0C\x00", 3, "\xD3", 1),
        T(1, 1, 0x53BCF940, "\xBB\x02\x00", 3, "\xD4", 1),
        T(1, 1, 0x24BBC9D6, "\xBB\x0A\x00", 3, "\xD5", 1),
        T(1, 1, 0xBDB2986C, "\xBB\x06\x00", 3, "\xD6", 1),
        T(1, 1, 0xCAB5A8FA, "\xBB\x0E\x00", 3, "\xD7", 1),
        T(1, 1, 0x5A0AB56B, "\xBB\x01\x00", 3, "\xD8", 1),
        T(1, 1, 0x2D0D85FD, "\xBB\x09\x00", 3, "\xD9", 1),
        T(1, 1, 0xB404D447, "\xBB\x05\x00", 3, "\xDA", 1),
        T(1, 1, 0xC303E4D1, "\xBB\x0D\x00", 3, "\xDB", 1),
        T(1, 1, 0x5D677172, "\xBB\x03\x00", 3, "\xDC", 1),
        T(1, 1, 0x2A6041E4, "\xBB\x0B\x00", 3, "\xDD", 1),
        T(1, 1, 0xB369105E, "\xBB\x07\x00", 3, "\xDE", 1),
        T(1, 1, 0xC46E20C8, "\xBB\x0F\x00", 3, "\xDF", 1),
        T(1, 1, 0x72080DF5, "\x7B\x00\x00", 3, "\xE0", 1),
        T(1, 1, 0x050F3D63, "\x7B\x08\x00", 3, "\xE1", 1),
        T(1, 1, 0x9C066CD9, "\x7B\x04\x00", 3, "\xE2", 1),
        T(1, 1, 0xEB015C4F, "\x7B\x0C\x00", 3, "\xE3", 1),
        T(1, 1, 0x7565C9EC, "\x7B\x02\x00", 3, "\xE4", 1),
        T(1, 1, 0x0262F97A, "\x7B\x0A\x00", 3, "\xE5", 1),
        T(1, 1, 0x9B6BA8C0, "\x7B\x06\x00", 3, "\xE6", 1),
        T(1, 1, 0xEC6C9856, "\x7B\x0E\x00", 3, "\xE7", 1),
        T(1, 1, 0x7CD385C7, "\x7B\x01\x00", 3, "\xE8", 1),
        T(1, 1, 0x0BD4B551, "\x7B\x09\x00", 3, "\xE9", 1),
        T(1, 1, 0x92DDE4EB, "\x7B\x05\x00", 3, "\xEA", 1),
        T(1, 1, 0xE5DAD47D, "\x7B\x0D\x00", 3, "\xEB", 1),
        T(1, 1, 0x7BBE41DE, "\x7B\x03\x00", 3, "\xEC", 1),
        T(1, 1, 0x0CB97148, "\x7B\x0B\x00", 3, "\xED", 1),
        T(1, 1, 0x95B020F2, "\x7B\x07\x00", 3, "\xEE", 1),
        T(1, 1, 0xE2B71064, "\x7B\x0F\x00", 3, "\xEF", 1),
        T(1, 1, 0x6FBF1D91, "\xFB\x00\x00", 3, "\xF0", 1),
        T(1, 1, 0x18B82D07, "\xFB\x08\x00", 3, "\xF1", 1),
        T(1, 1, 0x81B17CBD, "\xFB\x04\x00", 3, "\xF2", 1),
        T(1, 1, 0xF6B64C2B, "\xFB\x0C\x00", 3, "\xF3", 1),
        T(1, 1, 0x68D2D988, "\xFB\x02\x00", 3, "\xF4", 1),
        T(1, 1, 0x1FD5E91E, "\xFB\x0A\x00", 3, "\xF5", 1),
        T(1, 1, 0x86DCB8A4, "\xFB\x06\x00", 3, "\xF6", 1),
        T(1, 1, 0xF1DB8832, "\xFB\x0E\x00", 3, "\xF7", 1),
        T(1, 1, 0x616495A3, "\xFB\x01\x00", 3, "\xF8", 1),
        T(1, 1, 0x1663A535, "\xFB\x09\x00", 3, "\xF9", 1),
        T(1, 1, 0x8F6AF48F, "\xFB\x05\x00", 3, "\xFA", 1),
        T(1, 1, 0xF86DC419, "\xFB\x0D\x00", 3, "\xFB", 1),
        T(1, 1, 0x660951BA, "\xFB\x03\x00", 3, "\xFC", 1),
        T(1, 1, 0x110E612C, "\xFB\x0B\x00", 3, "\xFD", 1),
        T(1, 1, 0x88073096, "\xFB\x07\x00", 3, "\xFE", 1),
        T(1, 1, 0xFF000000, "\xFB\x0F\x00", 3, "\xFF", 1),

        /* 1バイトデータ・動的テーブル（全バイト値） */
#define DYNHDR "\x05\xE0\x21\x09\x00\x00\x00\x00\x20"
        T(1, 1, 0xD202EF8D, DYNHDR "\x38\xFD\xBA\x08", 13, "\x00", 1),
        T(1, 1, 0xA505DF1B, DYNHDR "\xE0\xF3\xEB\x22", 13, "\x01", 1),
        T(1, 1, 0x3C0C8EA1, DYNHDR "\x80\xCB\xAF\x8B", 13, "\x02", 1),
        T(1, 1, 0x4B0BBE37, DYNHDR "\x04\x8F\x5F\x17\x01", 14, "\x03", 1),
        T(1, 1, 0xD56F2B94, DYNHDR "\x14\x87\x5F\x17\x01", 14, "\x04", 1),
        T(1, 1, 0xA2681B02, DYNHDR "\x24\x7F\x5F\x17\x01", 14, "\x05", 1),
        T(1, 1, 0x3B614AB8, DYNHDR "\x34\x77\x5F\x17\x01", 14, "\x06", 1),
        T(1, 1, 0x4C667A2E, DYNHDR "\x44\x6F\x5F\x17\x01", 14, "\x07", 1),
        T(1, 1, 0xDCD967BF, DYNHDR "\x54\x67\x5F\x17\x01", 14, "\x08", 1),
        T(1, 1, 0xABDE5729, DYNHDR "\x64\x5F\x5F\x17\x01", 14, "\x09", 1),
        T(1, 1, 0x32D70693, DYNHDR "\x74\x57\x5F\x17\x01", 14, "\x0A", 1),
        T(1, 1, 0x45D03605, DYNHDR "\x0C\xF0\xF4\x75\x11", 14, "\x0B", 1),
        T(1, 1, 0xDBB4A3A6, DYNHDR "\x1C\x70\xF4\x75\x11", 14, "\x0C", 1),
        T(1, 1, 0xACB39330, DYNHDR "\x2C\xF0\xF3\x75\x11", 14, "\x0D", 1),
        T(1, 1, 0x35BAC28A, DYNHDR "\x3C\x70\xF3\x75\x11", 14, "\x0E", 1),
        T(1, 1, 0x42BDF21C, DYNHDR "\x4C\xF0\xF2\x75\x11", 14, "\x0F", 1),
        T(1, 1, 0xCFB5FFE9, DYNHDR "\x5C\x70\xF2\x75\x11", 14, "\x10", 1),
        T(1, 1, 0xB8B2CF7F, DYNHDR "\x6C\xF0\xF1\x75\x11", 14, "\x11", 1),
        T(1, 1, 0x21BB9EC5, DYNHDR "\x7C\x70\xF1\x75\x11", 14, "\x12", 1),
        T(1, 1, 0x56BCAE53, DYNHDR "\x8C\xF0\xF0\x75\x11", 14, "\x13", 1),
        T(1, 1, 0xC8D83BF0, DYNHDR "\x9C\x70\xF0\x75\x11", 14, "\x14", 1),
        T(1, 1, 0xBFDF0B66, DYNHDR "\xAC\xF0\xEF\x75\x11", 14, "\x15", 1),
        T(1, 1, 0x26D65ADC, DYNHDR "\xBC\x70\xEF\x75\x11", 14, "\x16", 1),
        T(1, 1, 0x51D16A4A, DYNHDR "\xCC\xF0\xEE\x75\x11", 14, "\x17", 1),
        T(1, 1, 0xC16E77DB, DYNHDR "\xDC\x70\xEE\x75\x11", 14, "\x18", 1),
        T(1, 1, 0xB669474D, DYNHDR "\xEC\xF0\xED\x75\x11", 14, "\x19", 1),
        T(1, 1, 0x2F6016F7, DYNHDR "\xFC\x70\xED\x75\x11", 14, "\x1A", 1),
        T(1, 1, 0x58672661, DYNHDR "\x0C\xF1\xEC\x75\x11", 14, "\x1B", 1),
        T(1, 1, 0xC603B3C2, DYNHDR "\x1C\x71\xEC\x75\x11", 14, "\x1C", 1),
        T(1, 1, 0xB1048354, DYNHDR "\x2C\xF1\xEB\x75\x11", 14, "\x1D", 1),
        T(1, 1, 0x280DD2EE, DYNHDR "\x3C\x71\xEB\x75\x11", 14, "\x1E", 1),
        T(1, 1, 0x5F0AE278, DYNHDR "\x4C\xF1\xEA\x75\x11", 14, "\x1F", 1),
        T(1, 1, 0xE96CCF45, DYNHDR "\x5C\x71\xEA\x75\x11", 14, "\x20", 1),
        T(1, 1, 0x9E6BFFD3, DYNHDR "\x6C\xF1\xE9\x75\x11", 14, "\x21", 1),
        T(1, 1, 0x0762AE69, DYNHDR "\x7C\x71\xE9\x75\x11", 14, "\x22", 1),
        T(1, 1, 0x70659EFF, DYNHDR "\x8C\xF1\xE8\x75\x11", 14, "\x23", 1),
        T(1, 1, 0xEE010B5C, DYNHDR "\x9C\x71\xE8\x75\x11", 14, "\x24", 1),
        T(1, 1, 0x99063BCA, DYNHDR "\xAC\xF1\xE7\x75\x11", 14, "\x25", 1),
        T(1, 1, 0x000F6A70, DYNHDR "\xBC\x71\xE7\x75\x11", 14, "\x26", 1),
        T(1, 1, 0x77085AE6, DYNHDR "\xCC\xF1\xE6\x75\x11", 14, "\x27", 1),
        T(1, 1, 0xE7B74777, DYNHDR "\xDC\x71\xE6\x75\x11", 14, "\x28", 1),
        T(1, 1, 0x90B077E1, DYNHDR "\xEC\xF1\xE5\x75\x11", 14, "\x29", 1),
        T(1, 1, 0x09B9265B, DYNHDR "\xFC\x71\xE5\x75\x11", 14, "\x2A", 1),
        T(1, 1, 0x7EBE16CD, DYNHDR "\x0C\xF2\xE4\x75\x11", 14, "\x2B", 1),
        T(1, 1, 0xE0DA836E, DYNHDR "\x1C\x72\xE4\x75\x11", 14, "\x2C", 1),
        T(1, 1, 0x97DDB3F8, DYNHDR "\x2C\xF2\xE3\x75\x11", 14, "\x2D", 1),
        T(1, 1, 0x0ED4E242, DYNHDR "\x3C\x72\xE3\x75\x11", 14, "\x2E", 1),
        T(1, 1, 0x79D3D2D4, DYNHDR "\x4C\xF2\xE2\x75\x11", 14, "\x2F", 1),
        T(1, 1, 0xF4DBDF21, DYNHDR "\x5C\x72\xE2\x75\x11", 14, "\x30", 1),
        T(1, 1, 0x83DCEFB7, DYNHDR "\x6C\xF2\xE1\x75\x11", 14, "\x31", 1),
        T(1, 1, 0x1AD5BE0D, DYNHDR "\x7C\x72\xE1\x75\x11", 14, "\x32", 1),
        T(1, 1, 0x6DD28E9B, DYNHDR "\x8C\xF2\xE0\x75\x11", 14, "\x33", 1),
        T(1, 1, 0xF3B61B38, DYNHDR "\x9C\x72\xE0\x75\x11", 14, "\x34", 1),
        T(1, 1, 0x84B12BAE, DYNHDR "\xAC\xF2\xDF\x75\x11", 14, "\x35", 1),
        T(1, 1, 0x1DB87A14, DYNHDR "\xBC\x72\xDF\x75\x11", 14, "\x36", 1),
        T(1, 1, 0x6ABF4A82, DYNHDR "\xCC\xF2\xDE\x75\x11", 14, "\x37", 1),
        T(1, 1, 0xFA005713, DYNHDR "\xDC\x72\xDE\x75\x11", 14, "\x38", 1),
        T(1, 1, 0x8D076785, DYNHDR "\xEC\xF2\xDD\x75\x11", 14, "\x39", 1),
        T(1, 1, 0x140E363F, DYNHDR "\xFC\x72\xDD\x75\x11", 14, "\x3A", 1),
        T(1, 1, 0x630906A9, DYNHDR "\x0C\xF3\xDC\x75\x11", 14, "\x3B", 1),
        T(1, 1, 0xFD6D930A, DYNHDR "\x1C\x73\xDC\x75\x11", 14, "\x3C", 1),
        T(1, 1, 0x8A6AA39C, DYNHDR "\x2C\xF3\xDB\x75\x11", 14, "\x3D", 1),
        T(1, 1, 0x1363F226, DYNHDR "\x3C\x73\xDB\x75\x11", 14, "\x3E", 1),
        T(1, 1, 0x6464C2B0, DYNHDR "\x4C\xF3\xDA\x75\x11", 14, "\x3F", 1),
        T(1, 1, 0xA4DEAE1D, DYNHDR "\x5C\x73\xDA\x75\x11", 14, "\x40", 1),
        T(1, 1, 0xD3D99E8B, DYNHDR "\x6C\xF3\xD9\x75\x11", 14, "\x41", 1),
        T(1, 1, 0x4AD0CF31, DYNHDR "\x7C\x73\xD9\x75\x11", 14, "\x42", 1),
        T(1, 1, 0x3DD7FFA7, DYNHDR "\x8C\xF3\xD8\x75\x11", 14, "\x43", 1),
        T(1, 1, 0xA3B36A04, DYNHDR "\x9C\x73\xD8\x75\x11", 14, "\x44", 1),
        T(1, 1, 0xD4B45A92, DYNHDR "\xAC\xF3\xD7\x75\x11", 14, "\x45", 1),
        T(1, 1, 0x4DBD0B28, DYNHDR "\xBC\x73\xD7\x75\x11", 14, "\x46", 1),
        T(1, 1, 0x3ABA3BBE, DYNHDR "\xCC\xF3\xD6\x75\x11", 14, "\x47", 1),
        T(1, 1, 0xAA05262F, DYNHDR "\xDC\x73\xD6\x75\x11", 14, "\x48", 1),
        T(1, 1, 0xDD0216B9, DYNHDR "\xEC\xF3\xD5\x75\x11", 14, "\x49", 1),
        T(1, 1, 0x440B4703, DYNHDR "\xFC\x73\xD5\x75\x11", 14, "\x4A", 1),
        T(1, 1, 0x330C7795, DYNHDR "\x0C\xF4\xD4\x75\x11", 14, "\x4B", 1),
        T(1, 1, 0xAD68E236, DYNHDR "\x1C\x74\xD4\x75\x11", 14, "\x4C", 1),
        T(1, 1, 0xDA6FD2A0, DYNHDR "\x2C\xF4\xD3\x75\x11", 14, "\x4D", 1),
        T(1, 1, 0x4366831A, DYNHDR "\x3C\x74\xD3\x75\x11", 14, "\x4E", 1),
        T(1, 1, 0x3461B38C, DYNHDR "\x4C\xF4\xD2\x75\x11", 14, "\x4F", 1),
        T(1, 1, 0xB969BE79, DYNHDR "\x5C\x74\xD2\x75\x11", 14, "\x50", 1),
        T(1, 1, 0xCE6E8EEF, DYNHDR "\x6C\xF4\xD1\x75\x11", 14, "\x51", 1),
        T(1, 1, 0x5767DF55, DYNHDR "\x7C\x74\xD1\x75\x11", 14, "\x52", 1),
        T(1, 1, 0x2060EFC3, DYNHDR "\x8C\xF4\xD0\x75\x11", 14, "\x53", 1),
        T(1, 1, 0xBE047A60, DYNHDR "\x9C\x74\xD0\x75\x11", 14, "\x54", 1),
        T(1, 1, 0xC9034AF6, DYNHDR "\xAC\xF4\xCF\x75\x11", 14, "\x55", 1),
        T(1, 1, 0x500A1B4C, DYNHDR "\xBC\x74\xCF\x75\x11", 14, "\x56", 1),
        T(1, 1, 0x270D2BDA, DYNHDR "\xCC\xF4\xCE\x75\x11", 14, "\x57", 1),
        T(1, 1, 0xB7B2364B, DYNHDR "\xDC\x74\xCE\x75\x11", 14, "\x58", 1),
        T(1, 1, 0xC0B506DD, DYNHDR "\xEC\xF4\xCD\x75\x11", 14, "\x59", 1),
        T(1, 1, 0x59BC5767, DYNHDR "\xFC\x74\xCD\x75\x11", 14, "\x5A", 1),
        T(1, 1, 0x2EBB67F1, DYNHDR "\x0C\xF5\xCC\x75\x11", 14, "\x5B", 1),
        T(1, 1, 0xB0DFF252, DYNHDR "\x1C\x75\xCC\x75\x11", 14, "\x5C", 1),
        T(1, 1, 0xC7D8C2C4, DYNHDR "\x2C\xF5\xCB\x75\x11", 14, "\x5D", 1),
        T(1, 1, 0x5ED1937E, DYNHDR "\x3C\x75\xCB\x75\x11", 14, "\x5E", 1),
        T(1, 1, 0x29D6A3E8, DYNHDR "\x4C\xF5\xCA\x75\x11", 14, "\x5F", 1),
        T(1, 1, 0x9FB08ED5, DYNHDR "\x5C\x75\xCA\x75\x11", 14, "\x60", 1),
        T(1, 1, 0xE8B7BE43, DYNHDR "\x6C\xF5\xC9\x75\x11", 14, "\x61", 1),
        T(1, 1, 0x71BEEFF9, DYNHDR "\x7C\x75\xC9\x75\x11", 14, "\x62", 1),
        T(1, 1, 0x06B9DF6F, DYNHDR "\x8C\xF5\xC8\x75\x11", 14, "\x63", 1),
        T(1, 1, 0x98DD4ACC, DYNHDR "\x9C\x75\xC8\x75\x11", 14, "\x64", 1),
        T(1, 1, 0xEFDA7A5A, DYNHDR "\xAC\xF5\xC7\x75\x11", 14, "\x65", 1),
        T(1, 1, 0x76D32BE0, DYNHDR "\xBC\x75\xC7\x75\x11", 14, "\x66", 1),
        T(1, 1, 0x01D41B76, DYNHDR "\xCC\xF5\xC6\x75\x11", 14, "\x67", 1),
        T(1, 1, 0x916B06E7, DYNHDR "\xDC\x75\xC6\x75\x11", 14, "\x68", 1),
        T(1, 1, 0xE66C3671, DYNHDR "\xEC\xF5\xC5\x75\x11", 14, "\x69", 1),
        T(1, 1, 0x7F6567CB, DYNHDR "\xFC\x75\xC5\x75\x11", 14, "\x6A", 1),
        T(1, 1, 0x0862575D, DYNHDR "\x0C\xF6\xC4\x75\x11", 14, "\x6B", 1),
        T(1, 1, 0x9606C2FE, DYNHDR "\x1C\x76\xC4\x75\x11", 14, "\x6C", 1),
        T(1, 1, 0xE101F268, DYNHDR "\x2C\xF6\xC3\x75\x11", 14, "\x6D", 1),
        T(1, 1, 0x7808A3D2, DYNHDR "\x3C\x76\xC3\x75\x11", 14, "\x6E", 1),
        T(1, 1, 0x0F0F9344, DYNHDR "\x4C\xF6\xC2\x75\x11", 14, "\x6F", 1),
        T(1, 1, 0x82079EB1, DYNHDR "\x5C\x76\xC2\x75\x11", 14, "\x70", 1),
        T(1, 1, 0xF500AE27, DYNHDR "\x6C\xF6\xC1\x75\x11", 14, "\x71", 1),
        T(1, 1, 0x6C09FF9D, DYNHDR "\x7C\x76\xC1\x75\x11", 14, "\x72", 1),
        T(1, 1, 0x1B0ECF0B, DYNHDR "\x8C\xF6\xC0\x75\x11", 14, "\x73", 1),
        T(1, 1, 0x856A5AA8, DYNHDR "\x9C\x76\xC0\x75\x11", 14, "\x74", 1),
        T(1, 1, 0xF26D6A3E, DYNHDR "\xAC\xF6\xBF\x08", 13, "\x75", 1),
        T(1, 1, 0x6B643B84, DYNHDR "\xBC\x76\xBF\x08", 13, "\x76", 1),
        T(1, 1, 0x1C630B12, DYNHDR "\xCC\xF6\xBE\x08", 13, "\x77", 1),
        T(1, 1, 0x8CDC1683, DYNHDR "\xDC\x76\xBE\x08", 13, "\x78", 1),
        T(1, 1, 0xFBDB2615, DYNHDR "\xEC\xF6\xBD\x08", 13, "\x79", 1),
        T(1, 1, 0x62D277AF, DYNHDR "\xFC\x76\xBD\x08", 13, "\x7A", 1),
        T(1, 1, 0x15D54739, DYNHDR "\x0C\xF7\xBC\x08", 13, "\x7B", 1),
        T(1, 1, 0x8BB1D29A, DYNHDR "\x1C\x77\xBC\x08", 13, "\x7C", 1),
        T(1, 1, 0xFCB6E20C, DYNHDR "\x2C\xF7\xBB\x08", 13, "\x7D", 1),
        T(1, 1, 0x65BFB3B6, DYNHDR "\x3C\x77\xBB\x08", 13, "\x7E", 1),
        T(1, 1, 0x12B88320, DYNHDR "\x4C\xF7\xBA\x08", 13, "\x7F", 1),
        T(1, 1, 0x3FBA6CAD, DYNHDR "\x5C\x77\xBA\x08", 13, "\x80", 1),
        T(1, 1, 0x48BD5C3B, DYNHDR "\x6C\xF7\xB9\x08", 13, "\x81", 1),
        T(1, 1, 0xD1B40D81, DYNHDR "\x7C\x77\xB9\x08", 13, "\x82", 1),
        T(1, 1, 0xA6B33D17, DYNHDR "\x8C\xF7\xB8\x08", 13, "\x83", 1),
        T(1, 1, 0x38D7A8B4, DYNHDR "\x9C\x77\xB8\x08", 13, "\x84", 1),
        T(1, 1, 0x4FD09822, DYNHDR "\xAC\xF7\xB7\x08", 13, "\x85", 1),
        T(1, 1, 0xD6D9C998, DYNHDR "\xBC\x77\xB7\x08", 13, "\x86", 1),
        T(1, 1, 0xA1DEF90E, DYNHDR "\xCC\xF7\xB6\x08", 13, "\x87", 1),
        T(1, 1, 0x3161E49F, DYNHDR "\xDC\x77\xB6\x08", 13, "\x88", 1),
        T(1, 1, 0x4666D409, DYNHDR "\xEC\xF7\xB5\x08", 13, "\x89", 1),
        T(1, 1, 0xDF6F85B3, DYNHDR "\x4C\x1F\xE0\x6A\x11", 14, "\x8A", 1),
        T(1, 1, 0xA868B525, DYNHDR "\x4C\x3F\xE0\x69\x11", 14, "\x8B", 1),
        T(1, 1, 0x360C2086, DYNHDR "\x4C\x5F\xE0\x68\x11", 14, "\x8C", 1),
        T(1, 1, 0x410B1010, DYNHDR "\x4C\x7F\xE0\x67\x11", 14, "\x8D", 1),
        T(1, 1, 0xD80241AA, DYNHDR "\x4C\x9F\xE0\x66\x11", 14, "\x8E", 1),
        T(1, 1, 0xAF05713C, DYNHDR "\x4C\xBF\xE0\x65\x11", 14, "\x8F", 1),
        T(1, 1, 0x220D7CC9, DYNHDR "\x4C\xDF\xE0\x64\x11", 14, "\x90", 1),
        T(1, 1, 0x550A4C5F, DYNHDR "\x4C\xFF\xE0\x63\x11", 14, "\x91", 1),
        T(1, 1, 0xCC031DE5, DYNHDR "\x4C\x1F\xE1\x62\x11", 14, "\x92", 1),
        T(1, 1, 0xBB042D73, DYNHDR "\x4C\x3F\xE1\x61\x11", 14, "\x93", 1),
        T(1, 1, 0x2560B8D0, DYNHDR "\x4C\x5F\xE1\x60\x11", 14, "\x94", 1),
        T(1, 1, 0x52678846, DYNHDR "\x4C\x7F\xE1\x5F\x11", 14, "\x95", 1),
        T(1, 1, 0xCB6ED9FC, DYNHDR "\x4C\x9F\xE1\x5E\x11", 14, "\x96", 1),
        T(1, 1, 0xBC69E96A, DYNHDR "\x4C\xBF\xE1\x5D\x11", 14, "\x97", 1),
        T(1, 1, 0x2CD6F4FB, DYNHDR "\x4C\xDF\xE1\x5C\x11", 14, "\x98", 1),
        T(1, 1, 0x5BD1C46D, DYNHDR "\x4C\xFF\xE1\x5B\x11", 14, "\x99", 1),
        T(1, 1, 0xC2D895D7, DYNHDR "\x4C\x1F\xE2\x5A\x11", 14, "\x9A", 1),
        T(1, 1, 0xB5DFA541, DYNHDR "\x4C\x3F\xE2\x59\x11", 14, "\x9B", 1),
        T(1, 1, 0x2BBB30E2, DYNHDR "\x4C\x5F\xE2\x58\x11", 14, "\x9C", 1),
        T(1, 1, 0x5CBC0074, DYNHDR "\x4C\x7F\xE2\x57\x11", 14, "\x9D", 1),
        T(1, 1, 0xC5B551CE, DYNHDR "\x4C\x9F\xE2\x56\x11", 14, "\x9E", 1),
        T(1, 1, 0xB2B26158, DYNHDR "\x4C\xBF\xE2\x55\x11", 14, "\x9F", 1),
        T(1, 1, 0x04D44C65, DYNHDR "\x4C\xDF\xE2\x54\x11", 14, "\xA0", 1),
        T(1, 1, 0x73D37CF3, DYNHDR "\x4C\xFF\xE2\x53\x11", 14, "\xA1", 1),
        T(1, 1, 0xEADA2D49, DYNHDR "\x4C\x1F\xE3\x52\x11", 14, "\xA2", 1),
        T(1, 1, 0x9DDD1DDF, DYNHDR "\x4C\x3F\xE3\x51\x11", 14, "\xA3", 1),
        T(1, 1, 0x03B9887C, DYNHDR "\x4C\x5F\xE3\x50\x11", 14, "\xA4", 1),
        T(1, 1, 0x74BEB8EA, DYNHDR "\x4C\x7F\xE3\x4F\x11", 14, "\xA5", 1),
        T(1, 1, 0xEDB7E950, DYNHDR "\x4C\x9F\xE3\x4E\x11", 14, "\xA6", 1),
        T(1, 1, 0x9AB0D9C6, DYNHDR "\x4C\xBF\xE3\x4D\x11", 14, "\xA7", 1),
        T(1, 1, 0x0A0FC457, DYNHDR "\x4C\xDF\xE3\x4C\x11", 14, "\xA8", 1),
        T(1, 1, 0x7D08F4C1, DYNHDR "\x4C\xFF\xE3\x4B\x11", 14, "\xA9", 1),
        T(1, 1, 0xE401A57B, DYNHDR "\x4C\x1F\xE4\x4A\x11", 14, "\xAA", 1),
        T(1, 1, 0x930695ED, DYNHDR "\x4C\x3F\xE4\x49\x11", 14, "\xAB", 1),
        T(1, 1, 0x0D62004E, DYNHDR "\x4C\x5F\xE4\x48\x11", 14, "\xAC", 1),
        T(1, 1, 0x7A6530D8, DYNHDR "\x4C\x7F\xE4\x47\x11", 14, "\xAD", 1),
        T(1, 1, 0xE36C6162, DYNHDR "\x4C\x9F\xE4\x46\x11", 14, "\xAE", 1),
        T(1, 1, 0x946B51F4, DYNHDR "\x4C\xBF\xE4\x45\x11", 14, "\xAF", 1),
        T(1, 1, 0x19635C01, DYNHDR "\x4C\xDF\xE4\x44\x11", 14, "\xB0", 1),
        T(1, 1, 0x6E646C97, DYNHDR "\x4C\xFF\xE4\x43\x11", 14, "\xB1", 1),
        T(1, 1, 0xF76D3D2D, DYNHDR "\x4C\x1F\xE5\x42\x11", 14, "\xB2", 1),
        T(1, 1, 0x806A0DBB, DYNHDR "\x4C\x3F\xE5\x41\x11", 14, "\xB3", 1),
        T(1, 1, 0x1E0E9818, DYNHDR "\x4C\x5F\xE5\x40\x11", 14, "\xB4", 1),
        T(1, 1, 0x6909A88E, DYNHDR "\x4C\x7F\xE5\x3F\x11", 14, "\xB5", 1),
        T(1, 1, 0xF000F934, DYNHDR "\x4C\x9F\xE5\x3E\x11", 14, "\xB6", 1),
        T(1, 1, 0x8707C9A2, DYNHDR "\x4C\xBF\xE5\x3D\x11", 14, "\xB7", 1),
        T(1, 1, 0x17B8D433, DYNHDR "\x4C\xDF\xE5\x3C\x11", 14, "\xB8", 1),
        T(1, 1, 0x60BFE4A5, DYNHDR "\x4C\xFF\xE5\x3B\x11", 14, "\xB9", 1),
        T(1, 1, 0xF9B6B51F, DYNHDR "\x4C\x1F\xE6\x3A\x11", 14, "\xBA", 1),
        T(1, 1, 0x8EB18589, DYNHDR "\x4C\x3F\xE6\x39\x11", 14, "\xBB", 1),
        T(1, 1, 0x10D5102A, DYNHDR "\x4C\x5F\xE6\x38\x11", 14, "\xBC", 1),
        T(1, 1, 0x67D220BC, DYNHDR "\x4C\x7F\xE6\x37\x11", 14, "\xBD", 1),
        T(1, 1, 0xFEDB7106, DYNHDR "\x4C\x9F\xE6\x36\x11", 14, "\xBE", 1),
        T(1, 1, 0x89DC4190, DYNHDR "\x4C\xBF\xE6\x35\x11", 14, "\xBF", 1),
        T(1, 1, 0x49662D3D, DYNHDR "\x4C\xDF\xE6\x34\x11", 14, "\xC0", 1),
        T(1, 1, 0x3E611DAB, DYNHDR "\x4C\xFF\xE6\x33\x11", 14, "\xC1", 1),
        T(1, 1, 0xA7684C11, DYNHDR "\x4C\x1F\xE7\x32\x11", 14, "\xC2", 1),
        T(1, 1, 0xD06F7C87, DYNHDR "\x4C\x3F\xE7\x31\x11", 14, "\xC3", 1),
        T(1, 1, 0x4E0BE924, DYNHDR "\x4C\x5F\xE7\x30\x11", 14, "\xC4", 1),
        T(1, 1, 0x390CD9B2, DYNHDR "\x4C\x7F\xE7\x2F\x11", 14, "\xC5", 1),
        T(1, 1, 0xA0058808, DYNHDR "\x4C\x9F\xE7\x2E\x11", 14, "\xC6", 1),
        T(1, 1, 0xD702B89E, DYNHDR "\x4C\xBF\xE7\x2D\x11", 14, "\xC7", 1),
        T(1, 1, 0x47BDA50F, DYNHDR "\x4C\xDF\xE7\x2C\x11", 14, "\xC8", 1),
        T(1, 1, 0x30BA9599, DYNHDR "\x4C\xFF\xE7\x2B\x11", 14, "\xC9", 1),
        T(1, 1, 0xA9B3C423, DYNHDR "\x4C\x1F\xE8\x2A\x11", 14, "\xCA", 1),
        T(1, 1, 0xDEB4F4B5, DYNHDR "\x4C\x3F\xE8\x29\x11", 14, "\xCB", 1),
        T(1, 1, 0x40D06116, DYNHDR "\x4C\x5F\xE8\x28\x11", 14, "\xCC", 1),
        T(1, 1, 0x37D75180, DYNHDR "\x4C\x7F\xE8\x27\x11", 14, "\xCD", 1),
        T(1, 1, 0xAEDE003A, DYNHDR "\x4C\x9F\xE8\x26\x11", 14, "\xCE", 1),
        T(1, 1, 0xD9D930AC, DYNHDR "\x4C\xBF\xE8\x25\x11", 14, "\xCF", 1),
        T(1, 1, 0x54D13D59, DYNHDR "\x4C\xDF\xE8\x24\x11", 14, "\xD0", 1),
        T(1, 1, 0x23D60DCF, DYNHDR "\x4C\xFF\xE8\x23\x11", 14, "\xD1", 1),
        T(1, 1, 0xBADF5C75, DYNHDR "\x4C\x1F\xE9\x22\x11", 14, "\xD2", 1),
        T(1, 1, 0xCDD86CE3, DYNHDR "\x4C\x3F\xE9\x21\x11", 14, "\xD3", 1),
        T(1, 1, 0x53BCF940, DYNHDR "\x4C\x5F\xE9\x20\x11", 14, "\xD4", 1),
        T(1, 1, 0x24BBC9D6, DYNHDR "\x4C\x7F\xE9\x1F\x11", 14, "\xD5", 1),
        T(1, 1, 0xBDB2986C, DYNHDR "\x4C\x9F\xE9\x1E\x11", 14, "\xD6", 1),
        T(1, 1, 0xCAB5A8FA, DYNHDR "\x4C\xBF\xE9\x1D\x11", 14, "\xD7", 1),
        T(1, 1, 0x5A0AB56B, DYNHDR "\x4C\xDF\xE9\x1C\x11", 14, "\xD8", 1),
        T(1, 1, 0x2D0D85FD, DYNHDR "\x4C\xFF\xE9\x1B\x11", 14, "\xD9", 1),
        T(1, 1, 0xB404D447, DYNHDR "\x4C\x1F\xEA\x1A\x11", 14, "\xDA", 1),
        T(1, 1, 0xC303E4D1, DYNHDR "\x4C\x3F\xEA\x19\x11", 14, "\xDB", 1),
        T(1, 1, 0x5D677172, DYNHDR "\x4C\x5F\xEA\x18\x11", 14, "\xDC", 1),
        T(1, 1, 0x2A6041E4, DYNHDR "\x4C\x7F\xEA\x17\x11", 14, "\xDD", 1),
        T(1, 1, 0xB369105E, DYNHDR "\x4C\x9F\xEA\x16\x11", 14, "\xDE", 1),
        T(1, 1, 0xC46E20C8, DYNHDR "\x4C\xBF\xEA\x15\x11", 14, "\xDF", 1),
        T(1, 1, 0x72080DF5, DYNHDR "\x4C\xDF\xEA\x14\x11", 14, "\xE0", 1),
        T(1, 1, 0x050F3D63, DYNHDR "\x4C\xFF\xEA\x13\x11", 14, "\xE1", 1),
        T(1, 1, 0x9C066CD9, DYNHDR "\x4C\x1F\xEB\x12\x11", 14, "\xE2", 1),
        T(1, 1, 0xEB015C4F, DYNHDR "\x4C\x3F\xEB\x11\x11", 14, "\xE3", 1),
        T(1, 1, 0x7565C9EC, DYNHDR "\x4C\x5F\xEB\x10\x11", 14, "\xE4", 1),
        T(1, 1, 0x0262F97A, DYNHDR "\x4C\x7F\xEB\x0F\x11", 14, "\xE5", 1),
        T(1, 1, 0x9B6BA8C0, DYNHDR "\x4C\x9F\xEB\x0E\x11", 14, "\xE6", 1),
        T(1, 1, 0xEC6C9856, DYNHDR "\x4C\xBF\xEB\x0D\x11", 14, "\xE7", 1),
        T(1, 1, 0x7CD385C7, DYNHDR "\x4C\xDF\xEB\x0C\x11", 14, "\xE8", 1),
        T(1, 1, 0x0BD4B551, DYNHDR "\x4C\xFF\xEB\x0B\x11", 14, "\xE9", 1),
        T(1, 1, 0x92DDE4EB, DYNHDR "\x4C\x1F\xEC\x0A\x11", 14, "\xEA", 1),
        T(1, 1, 0xE5DAD47D, DYNHDR "\x4C\x3F\xEC\x09\x11", 14, "\xEB", 1),
        T(1, 1, 0x7BBE41DE, DYNHDR "\x4C\x5F\xEC\x08\x11", 14, "\xEC", 1),
        T(1, 1, 0x0CB97148, DYNHDR "\x4C\x7F\xEC\x07\x11", 14, "\xED", 1),
        T(1, 1, 0x95B020F2, DYNHDR "\x4C\x9F\xEC\x06\x11", 14, "\xEE", 1),
        T(1, 1, 0xE2B71064, DYNHDR "\x4C\xBF\xEC\x05\x11", 14, "\xEF", 1),
        T(1, 1, 0x6FBF1D91, DYNHDR "\x4C\xDF\xEC\x04\x11", 14, "\xF0", 1),
        T(1, 1, 0x18B82D07, DYNHDR "\x4C\xFF\xEC\x03\x11", 14, "\xF1", 1),
        T(1, 1, 0x81B17CBD, DYNHDR "\x4C\x1F\xED\x02\x11", 14, "\xF2", 1),
        T(1, 1, 0xF6B64C2B, DYNHDR "\x4C\x3F\xED\x01\x11", 14, "\xF3", 1),
        T(1, 1, 0x68D2D988, DYNHDR "\x4C\x5F\xED\x00\x11", 14, "\xF4", 1),
        T(1, 1, 0x1FD5E91E, DYNHDR "\x4C\x7F\x6D\x17\x01", 14, "\xF5", 1),
        T(1, 1, 0x86DCB8A4, DYNHDR "\x4C\x9F\x6D\x16\x01", 14, "\xF6", 1),
        T(1, 1, 0xF1DB8832, DYNHDR "\x4C\xBF\x6D\x15\x01", 14, "\xF7", 1),
        T(1, 1, 0x616495A3, DYNHDR "\x4C\xDF\x6D\x14\x01", 14, "\xF8", 1),
        T(1, 1, 0x1663A535, DYNHDR "\x4C\xFF\x6D\x13\x01", 14, "\xF9", 1),
        T(1, 1, 0x8F6AF48F, DYNHDR "\x4C\x1F\x6E\x12\x01", 14, "\xFA", 1),
        T(1, 1, 0xF86DC419, DYNHDR "\x4C\x3F\x6E\x11\x01", 14, "\xFB", 1),
        T(1, 1, 0x660951BA, DYNHDR "\x4C\x5F\x6E\x10\x01", 14, "\xFC", 1),
        T(1, 1, 0x110E612C, DYNHDR "\x4C\x7F\x2E\x88", 13, "\xFD", 1),
        T(1, 1, 0x88073096, DYNHDR "\x4C\x9F\x2E\x22", 13, "\xFE", 1),
        T(1, 1, 0xFF000000, DYNHDR "\x4C\xBF\xAE\x08", 13, "\xFF", 1),
#undef DYNHDR

        /**** 異常系 ****/

        /* 不正パラメータ系 */
        T(0, -1, 0, NULL, 0, NULL, 0),
        T(0, -1, 0, "", -1, NULL, 0),
        T(-1, -1, 0, "", 1, NULL, 0),
        T(-2, -1, 0, "", 1, NULL, 0),  // out_size==-2は特殊処理になる

        /* データ不足 */
        T(0, -1, 0, "\x03", 1, NULL, 0),
        T(0, -1, 0, "\x02\x00", 2, NULL, 0),

        /* 無効なブロック種類コード */
        T(0, -1, 0, "\x07\x00", 2, NULL, 0),

        /* 出力バッファ不足時、オーバーランしないことを確認 */
        T(4, 5, 0x8587D865, "\x01\x05\x00\xFA\xFF" "abcde", 10, "abcd", 4),
        /* 出力バッファサイズ0でも出力データサイズを取得できることを確認 */
        T(0, 5, 0x8587D865, "\x01\x05\x00\xFA\xFF" "abcde", 10, NULL, 0),

#undef T

    };  // tests[]

    uint8_t outbuf[1000], testbuf[sizeof(outbuf)];
    int i;
    for (i = 0; i < lenof(tests); i++) {
        mem_fill32(outbuf, 0xDEADBEEF, sizeof(outbuf));
        memcpy(testbuf, outbuf, sizeof(outbuf));
        if (tests[i].test_size > 0) {
            memcpy(testbuf, tests[i].test, tests[i].test_size);
        }
        if (!test_decompress_one((const uint8_t *)tests[i].in,
                                 tests[i].in_size,
                                 tests[i].out_size==-2 ? NULL : outbuf,
                                 tests[i].out_size==-2 ? 1 : tests[i].out_size,
                                 tests[i].result, tests[i].crc32,
                                 testbuf, sizeof(testbuf))) {
            DMSG("test %d (line %u) failed, aborting", i, tests[i].line);
            return 0;
        }
    }

    return 1;
}

/*************************************************************************/

/**
 * test_decompress_one:  特定のデータで圧縮データ解凍関数をテストする。
 *
 * 【引　数】    in, in_size: 入力（圧縮データ）バッファとそのサイズ（バイト）
 * 　　　　　  out, out_size: 出力（解凍データ）バッファとそのサイズ（バイト）
 * 　　　　　expected_result: 関数戻り値の期待値
 * 　　　　　 expected_crc32: 解凍データのCRC32の期待値
 * 　　　　　test, test_size: 比較バッファとそのサイズ（バイト）
 * 【戻り値】0以外＝成功、0＝失敗
 * 【注　意】test_size > out_sizeも可（バッファオーバフローの有無を確認する
 * 　　　　　ため）。
 */
static int test_decompress_one(const uint8_t * const in,
                               const int32_t in_size,
                               uint8_t * const out,
                               const int32_t out_size,
                               const long expected_result,
                               const unsigned long expected_crc32,
                               const uint8_t * const test,
                               const int32_t test_size)
{
    unsigned long crc32;
    const long result = tinflate(in, in_size, out, out_size, &crc32);
    if (result != expected_result) {
        DMSG("FAIL: expected result %ld, got %ld", expected_result, result);
        return 0;
    }
    if (result >= 0 && crc32 != expected_crc32) {
        DMSG("FAIL: expected CRC32 %08lX, got %08lX", expected_crc32, crc32);
        return 0;
    }
    if (out != NULL) {
        uint32_t i;
        for (i = 0; i < test_size; i++) {
            if (out[i] != test[i]) {
                DMSG("FAIL: data mismatch at 0x%X: expected %02X, got %02X",
                     i, test[i], out[i]);
                return 0;
            }
        }
    }
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
