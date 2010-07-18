/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/resource/package-pkg.c: Package access functions for PKG-format
 * package files (a custom format used for Aquaria).
 */

#include "../common.h"
#include "../memory.h"
#include "../resource.h"
#include "../sysdep.h"

#include "package.h"
#include "package-pkg.h"
#include "tinflate.h"

/*************************************************************************/
/*********************** 各ゲーム共通データ・処理 ************************/
/*************************************************************************/

/* パッケージファイル情報構造体 */
typedef struct PackageFile_ {
    const char *pathname;   // パッケージファイルのパス名
    SysFile *fp;            // パッケージファイルポインタ（NULL＝未オープン）
    PKGIndexEntry *index;   // 索引データ（ハッシュ値・ファイル名順）
    int nfiles;             // ファイル数
    char *namebuf;          // ファイル名データが格納されたバッファ
    int list_pos;           // ファイルリスト取得用のリスト位置
} PackageFile;

/* 指定された索引配列インデックスのパス名を返すマクロ */
#define NAME(i)  (info->namebuf + PKG_NAMEOFS(info->index[(i)].nameofs_flags))

/*************************************************************************/

/**
 * package_pkg_init:  PKG形式パッケージモジュールの初期化処理を行う。
 *
 * 【引　数】module: パッケージモジュール情報ポインタ
 * 【戻り値】0以外＝成功、0＝失敗
 */
static int package_pkg_init(PackageModuleInfo *module)
{
    PRECOND_SOFT(module != NULL, return 0);
    PRECOND_SOFT(module->module_data != NULL, return 0);
    PackageFile *info = (PackageFile *)(module->module_data);
    PRECOND_SOFT(info->pathname != NULL, return 0);

    /* パッケージファイルを開く */
    info->fp = sys_file_open(info->pathname);
    if (UNLIKELY(!info->fp)) {
        DMSG("open(%s): %s", info->pathname, sys_last_errstr());
        goto error_return;
    }

    /* ヘッダデータを読み込んで確認する */
    PKGHeader header;
    if (UNLIKELY(sys_file_read(info->fp, &header, 16) != 16)) {
        DMSG("EOF reading %s", info->pathname);
        goto error_close_file;
    }
    if (UNLIKELY(memcmp(header.magic, PKG_MAGIC, 4) != 0)) {
        DMSG("Bad magic number reading %s", info->pathname);
        goto error_close_file;
    }
    PKG_HEADER_SWAP_BYTES(header);
    if (UNLIKELY(header.header_size != sizeof(PKGHeader))) {
        DMSG("Bad header size %d in %s", header.header_size, info->pathname);
        goto error_close_file;
    }
    if (UNLIKELY(header.entry_size != sizeof(PKGIndexEntry))) {
        DMSG("Bad index entry size %d in %s", header.entry_size,
             info->pathname);
        goto error_close_file;
    }

    /* 索引用のメモリバッファを確保する */
    info->nfiles = header.entry_count;
    const int index_size = sizeof(*info->index) * info->nfiles;
    info->index = mem_alloc(index_size, 4, 0);
    if (!info->index) {
        DMSG("No memory for %s directory (%d*%d)", info->pathname,
             info->nfiles, (int)sizeof(*info->index));
        goto error_close_file;
    }
    info->namebuf = mem_alloc(header.name_size, 1, 0);
    if (!info->namebuf) {
        DMSG("No memory for %s pathnames (%u bytes)", info->pathname,
             header.name_size);
        goto error_free_index;
    }

    /* 索引データを読み込む */
    if (UNLIKELY(sys_file_read(info->fp, info->index, index_size)
                 != index_size)
    ) {
        DMSG("EOF reading %s directory", info->pathname);
        goto error_free_namebuf;
    }
    if (UNLIKELY(sys_file_read(info->fp, info->namebuf, header.name_size)
                 != header.name_size)
    ) {
        DMSG("EOF reading %s pathname table", info->pathname);
        goto error_free_namebuf;
    }
    PKG_INDEX_SWAP_BYTES(info->index, info->nfiles);

    /* 成功 */
    return 1;

    /* エラー発生時、必要な後片付けを行いエラーを返す */
  error_free_namebuf:
    mem_free(info->namebuf);
    info->namebuf = NULL;
  error_free_index:
    mem_free(info->index);
    info->index = NULL;
  error_close_file:
    sys_file_close(info->fp);
    info->fp = NULL;
  error_return:
    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * package_pkg_cleanup:  PKG形式パッケージモジュールの後片付け処理を行う。
 * 未初期化状態では呼び出されない。
 *
 * 【引　数】module: パッケージモジュール情報ポインタ
 * 【戻り値】なし
 */
static void package_pkg_cleanup(PackageModuleInfo *module)
{
    PRECOND_SOFT(module != NULL, return);
    PRECOND_SOFT(module->module_data != NULL, return);
    PackageFile *info = (PackageFile *)(module->module_data);

    mem_free(info->namebuf);
    info->namebuf = NULL;
    mem_free(info->index);
    info->index = NULL;
    sys_file_close(info->fp);
    info->fp = NULL;
}

/*************************************************************************/

/**
 * PackageListStartFunc:  パッケージ内のファイルをリストアップするための
 * 初期化を行う。
 *
 * 【引　数】module: パッケージモジュール情報ポインタ
 * 【戻り値】なし
 */
static void package_pkg_list_files_start(PackageModuleInfo *module)
{
    PRECOND_SOFT(module != NULL, return);
    PRECOND_SOFT(module->module_data != NULL, return);
    PackageFile *info = (PackageFile *)(module->module_data);

    info->list_pos = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * PackageListNextFunc:  次のデータファイルのパス名を返す。
 *
 * 【引　数】module: パッケージモジュール情報ポインタ
 * 【戻り値】パス名（NULL＝リスト完了）
 */
static const char *package_pkg_list_files_next(PackageModuleInfo *module)
{
    PRECOND_SOFT(module != NULL, return NULL);
    PRECOND_SOFT(module->module_data != NULL, return NULL);
    PackageFile *info = (PackageFile *)(module->module_data);

    if (info->list_pos < info->nfiles) {
        return NAME(info->list_pos++);
    } else {
        return NULL;
    }
}

/*************************************************************************/

/**
 * package_pkg_file_info:  指定されたパス名に関する情報を返す。*_ret変数の
 * 値は成功時のみ設定される。
 *
 * 【引　数】  module: パッケージモジュール情報ポインタ
 * 　　　　　    path: パス名（モジュール固有プレフィックスを除く）
 * 　　　　　file_ret: ロードするファイルポインタを格納する変数へのポインタ
 * 　　　　　 pos_ret: ファイル内の読み込み開始位置を格納する変数へのポインタ
 * 　　　　　 len_ret: 読み込みデータ量（バイト）を格納する変数へのポインタ
 * 　　　　　comp_ret: 圧縮の有無（0以外＝圧縮）を格納する変数へのポインタ
 * 　　　　　size_ret: 最終データ量（バイト）を格納する変数へのポインタ
 * 【戻り値】0以外＝成功、0＝失敗（パス名に相当するファイルが存在しない等）
 */
static int package_pkg_file_info(PackageModuleInfo *module,
                                  const char *path, SysFile **file_ret,
                                  uint32_t *pos_ret, uint32_t *len_ret,
                                  int *comp_ret, uint32_t *size_ret)
{
    PRECOND_SOFT(module != NULL, return 0);
    PRECOND_SOFT(module->module_data != NULL, return 0);
    PackageFile *info = (PackageFile *)(module->module_data);
    PRECOND_SOFT(path != NULL, return 0);
    PRECOND_SOFT(file_ret != NULL, return 0);
    PRECOND_SOFT(pos_ret != NULL, return 0);
    PRECOND_SOFT(len_ret != NULL, return 0);
    PRECOND_SOFT(comp_ret != NULL, return 0);
    PRECOND_SOFT(size_ret != NULL, return 0);

    if (!info->fp) {
        DMSG("Package not initialized");
        return 0;
    }

    /* 索引の中から指定されたパス名を探す */
    const uint32_t hash = pkg_hash(path);
    int low = 0, high = info->nfiles-1;
    int i = info->nfiles / 2;
    /* 注：stricmp()が大文字を小文字に変換して比較するという仕様に頼っている */
    while (low <= high && stricmp(path, NAME(i)) != 0) {
        if (hash < info->index[i].hash) {
            high = i-1;
        } else if (hash > info->index[i].hash) {
            low = i+1;
        } else if (stricmp(path, NAME(i)) < 0) {
            high = i-1;
        } else {
            low = i+1;
        }
        i = (low + high + 1) / 2;
    }
    if (UNLIKELY(low > high)) {
        return 0;
    }
#undef NAME

    /* 情報を返す */
    *file_ret = info->fp;
    *pos_ret  = info->index[i].offset;
    *len_ret  = info->index[i].datalen;
    *comp_ret = (info->index[i].nameofs_flags & PKGF_DEFLATED) ? 1 : 0;
    *size_ret = info->index[i].filesize;
    return 1;
}

/*************************************************************************/

/**
 * package_pkg_decompress:  データの圧縮を解除する。
 *
 * 【引　数】 module: パッケージモジュール情報ポインタ
 * 　　　　　     in: 入力（圧縮データ）バッファ
 * 　　　　　 insize: 入力データ長（バイト）
 * 　　　　　    out: 出力（元データ）バッファ
 * 　　　　　outsize: 出力バッファサイズ（バイト）
 * 【戻り値】0以外＝成功、0＝失敗
 */
static int package_pkg_decompress(PackageModuleInfo *module,
                                   const void *in, uint32_t insize,
                                   void *out, uint32_t outsize)
{
    PRECOND_SOFT(in != NULL, return 0);
    PRECOND_SOFT(out != NULL, return 0);

    uint32_t result = tinflate(in, insize, out, outsize, NULL);
    return result >= 0 && result <= outsize;
}

/*************************************************************************/
/********************** パッケージモジュールデータ ***********************/
/*************************************************************************/

static PackageFile aquaria_pkg = {.pathname = "aquaria.dat"};

/* MOD対策（_modsがパッケージに入らないため） */
static int package_aquaria_has_path(PackageModuleInfo *module, const char *path)
{
    PRECOND_SOFT(path != NULL, return 0);
    while (strncmp(path, "./", 2) == 0) {
        path += 2;
    }
    return strnicmp(path, "_mods/", 6) != 0;
}

/*-----------------------------------------------------------------------*/

PackageModuleInfo package_info_aquaria = {
    .prefix           = "",  // 全てのファイルに適用
    .init             = package_pkg_init,
    .cleanup          = package_pkg_cleanup,
    .list_files_start = package_pkg_list_files_start,
    .list_files_next  = package_pkg_list_files_next,
    .has_path         = package_aquaria_has_path,
    .file_info        = package_pkg_file_info,
    .decompress       = package_pkg_decompress,
    .module_data      = &aquaria_pkg,
};

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
