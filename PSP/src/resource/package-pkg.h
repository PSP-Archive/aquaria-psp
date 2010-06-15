/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/resource/package-pkg.h: Header defining the custom PKG format for
 * package files.
 */

#ifndef RESOURCE_PACKAGE_PKG_H
#define RESOURCE_PACKAGE_PKG_H

/*
 * PKG（「PacKaGe」）形式パッケージファイルのヘッダ・索引データ形式と、パス
 * 名のハッシュ関数をここで定義する。
 *
 * PKGファイルは、以下のように構成される。
 * 　・ファイルヘッダ（PKGHeader構造体）
 * 　・ファイル毎に、索引情報構造体（PKGIndexEntry構造体）
 * 　　　→ エントリーは、ハッシュ順にソートされていなければならない。同一
 * 　　　　 ハッシュ値を持つパス名の場合、パス名を小文字に変換したときの文
 * 　　　　 字列順にソートする。
 * 　・全てのファイルのパス名データバッファ
 * 　　　→ PKGIndexEntry.nameofsが指すバッファ。文字列はすべてC言語形式と
 * 　　　　 する（終端に'\0'を付加）。
 * 　・ファイルデータ
 * 　　　→ 環境に応じて、ファイルデータのアライメントを変えて構わない。
 *
 * なお、ヘッダ・索引内の数値データはすべてビッグエンディアン方式で記録する。
 * たとえば0x12345678の場合、0x12 0x34 0x56 0x78 の順に4バイトとして記録する。
 * 読み込み・作成の際、PKG_HEADER_SWAP_BYTES、PKG_INDEX_SWAP_BYTESマクロを
 * 使うことでバイト順を変換できる。
 */

/*************************************************************************/

/* ファイルヘッダ */
typedef struct PKGHeader_ {
    char magic[4];        // "PKG\n"
    uint16_t header_size; // ファイルヘッダのサイズ == sizeof(PKGHeader)
    uint16_t entry_size;  // 索引情報構造体のサイズ == sizeof(PKGIndexEntry)
    uint32_t entry_count; // 索引エントリー数
    uint32_t name_size;   // パス名データのサイズ（バイト）
} PKGHeader;

#define PKG_MAGIC  "PKG\012"

/* 索引情報構造体 */
typedef struct PKGIndexEntry_ {
    uint32_t hash;
    uint32_t nameofs_flags; // 下位24ビット：パス名データバッファ内、この
                            // 　ファイルのパス名へのオフセット（バイト）
                            // 上位8ビット：各種フラグ（以下のPKGF_*）
    uint32_t offset;    // パッケージ内のオフセット（バイト、ファイル先頭から）
    uint32_t datalen;   // パッケージ内データサイズ（バイト）
    uint32_t filesize;  // ファイルサイズ（圧縮解凍後）（バイト）
} PKGIndexEntry;

/* nameofs_flagsフィールドからパス名オフセットを取り出すマクロ */
#define PKG_NAMEOFS(nameofs_flags) ((nameofs_flags) & 0x00FFFFFF)

/* 索引エントリーフラグ（PKGIndexEntry.nameofs_flags） */
#define PKGF_DEFLATED  (1<<24)  // deflate方式で圧縮されている

/*************************************************************************/

/**
 * PKG_HEADER_SWAP_BYTES:  PKGHeader構造体の数値フィールドのバイト順を、
 * ファイル順からマシン順に、またはマシン順からファイル順に変更する。
 *
 * 【引　数】header: ファイルヘッダ構造体（注：ポインタではない）
 * 【戻り値】なし
 */
#define PKG_HEADER_SWAP_BYTES(header) do { \
    PKG_SWAP16((header).header_size);      \
    PKG_SWAP16((header).entry_size);       \
    PKG_SWAP32((header).entry_count);      \
    PKG_SWAP32((header).name_size);        \
} while (0)


/**
 * PKG_INDEX_SWAP_BYTES:  PKGIndexEntry構造体配列の数値フィールドのバイト
 * 順を、ファイル順からマシン順に、またはマシン順からファイル順に変更する。
 *
 * 【引　数】index: 索引情報構造体配列へのポインタ
 * 　　　　　count: 索引エントリー数
 * 【戻り値】なし
 */
#define PKG_INDEX_SWAP_BYTES(index,count) do { \
    uint32_t *__ptr32 = (uint32_t *)(index);    \
    uint32_t *__limit = __ptr32 + ((count) * (sizeof(PKGIndexEntry)/4)); \
    while (__ptr32 < __limit) {                 \
        PKG_SWAP32(*__ptr32);                  \
        __ptr32++;                              \
    }                                           \
} while (0)


/**
 * PKG_SWAP16, PKG_SWAP32:  リトルエンディアン環境では、16ビット値または
 * 32ビット値のバイト順を入れ替える。ビッグエンディアン環境では、実質的に
 * 何もしない。PKG_HEADER_SWAP_BYTES、PKG_INDEX_SWAP_BYTESの補助マクロ。
 *
 * 【引　数】var: バイト順を入れ替える変数・フィールド（注：ポインタではない）
 * 【戻り値】なし
 */
#define PKG_SWAP16(var) do { \
    const uint8_t *__ptr8 = (const uint8_t *)&(var); \
    (var) = __ptr8[0]<<8 | __ptr8[1]; \
} while (0)

#define PKG_SWAP32(var) do { \
    const uint8_t *__ptr8 = (const uint8_t *)&(var); \
    (var) = __ptr8[0]<<24 | __ptr8[1]<<16 | __ptr8[2]<<8 | __ptr8[3]; \
} while (0)

/*************************************************************************/

/**
 * pkg_hash:  PKG索引用ハッシュ関数。渡されたパス名のハッシュ値を返す。
 *
 * 【引　数】path: パス名
 * 【戻り値】ハッシュ値
 */
static inline PURE_FUNCTION uint32_t pkg_hash(const char *path)
{
#ifndef IN_TOOL
    PRECOND_SOFT(path != NULL, return 0);
#endif

    /* 1文字毎にハッシュ値を5ビット左へ回転して排他的ビット和で文字を組み
     * 入れる。高速であり、かつ実際のデータセットを用いた実験では、衝突が
     * 殆ど発生しなかった。 */
    uint32_t hash = 0;
    while (*path) {
        uint32_t c = *path++;
        if (c >= 'A' && c <= 'Z') {
            c += 0x20;
        }
        hash = hash<<27 | hash>>5;
        hash ^= c;
    }
    return hash;
}

/*************************************************************************/

#endif  // RESOURCE_PACKAGE_PKG_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
