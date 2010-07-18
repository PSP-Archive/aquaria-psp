/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/resource/package.h: Resource package file handler declarations.
 */

/*
 * ゲームデータが格納されるパッケージファイルへのアクセスは、パッケージファ
 * イルの種類に応じた操作モジュールで実装される。リソースをロードしようとす
 * るとき、パス名に対応したモジュールを検索し、そのモジュールの操作関数によ
 * ってデータをロードする。
 *
 * 圧縮データの場合、データがすべてロードされた後、別関数で圧縮を解除する。
 * CPUの処理能力が十分にある場合、読み込み中から部分的に解除していくことも
 * できるが、特にPSPの場合はフレームレートに影響する可能性を無視できないの
 * でこの方式は採らない。
 */

#ifndef RESOURCE_PACKAGE_H

/*************************************************************************/

/* 各関数の引数に使うので、データ型を先に宣言しておく（構造体定義は後） */
typedef struct PackageModuleInfo_ PackageModuleInfo;

/*************************************************************************/
/***************** パッケージモジュールが実装すべき関数 ******************/
/*************************************************************************/

/**
 * PackageInitFunc:  モジュールの初期化処理を行う。
 *
 * 【引　数】module: パッケージモジュール情報ポインタ
 * 【戻り値】0以外＝成功、0＝失敗
 */
typedef int (*PackageInitFunc)(PackageModuleInfo *module);

/**
 * PackageCleanupFunc:  モジュールの後片付け処理を行う。未初期化状態では
 * 呼び出されない。
 *
 * 【引　数】module: パッケージモジュール情報ポインタ
 * 【戻り値】なし
 */
typedef void (*PackageCleanupFunc)(PackageModuleInfo *module);

/**
 * PackageListStartFunc:  パッケージ内のファイルをリストアップするための
 * 初期化を行う。
 *
 * 【引　数】module: パッケージモジュール情報ポインタ
 * 【戻り値】なし
 */
typedef void (*PackageListStartFunc)(PackageModuleInfo *module);

/**
 * PackageListNextFunc:  次のデータファイルのパス名を返す。
 *
 * 【引　数】module: パッケージモジュール情報ポインタ
 * 【戻り値】パス名（NULL＝リスト完了）
 */
typedef const char *(*PackageListNextFunc)(PackageModuleInfo *module);

/**
 * PackageHasPathFunc:  特定のパス名がパッケージファイルの管理下にあるか
 * どうかを返す。リソースにアクセスする際、パッケージ内でリソースが見つから
 * ない場合において、物理ファイルシステムにアクセスすべきかどうか判断する
 * たみに呼び出される。
 *
 * 【引　数】module: パッケージモジュール情報ポインタ
 * 　　　　　  path: チェックするパス名
 * 【戻り値】0以外＝パス名がパッケージの管理下にあり、物理ファイルシステム
 * 　　　　　　　　　へのアクセスをするべきでない
 * 　　　　　　　0＝パス名がパッケージの管理下にはなく、物理ファイルシステム
 * 　　　　　　　　　へのアクセスをしてもよい
 */
typedef int (*PackageHasPathFunc)(PackageModuleInfo *module, const char *path);

/**
 * PackageFileInfoFunc:  指定されたパス名に関する情報を返す。*_ret変数の値は
 * 成功時のみ設定される。
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
typedef int (*PackageFileInfoFunc)(PackageModuleInfo *module,
                                   const char *path, SysFile **file_ret,
                                   uint32_t *pos_ret, uint32_t *len_ret,
                                   int *comp_ret, uint32_t *size_ret);

/**
 * PackageDecompressFunc:  データの圧縮を解除する。
 *
 * 【引　数】 module: パッケージモジュール情報ポインタ
 * 　　　　　     in: 入力（圧縮データ）バッファ
 * 　　　　　 insize: 入力データ長（バイト）
 * 　　　　　    out: 出力（元データ）バッファ
 * 　　　　　outsize: 出力バッファサイズ（バイト）
 * 【戻り値】0以外＝成功、0＝失敗
 */
typedef int (*PackageDecompressFunc)(PackageModuleInfo *module,
                                     const void *in, uint32_t insize,
                                     void *out, uint32_t outsize);

/*************************************************************************/
/******************** パッケージモジュール情報構造体 *********************/
/*************************************************************************/

/**
 * PackageModuleInfo:  パッケージファイル操作モジュールの情報をまとめた構造体。
 */
struct PackageModuleInfo_ {

    /* このパッケージファイルを指定するパス名。パスの先頭部分がこの文字列と
     * 一致する場合、このモジュールが選択される。必ず255バイト以下 */
    const char *prefix;

    /* 各処理関数へのポインタ */
    PackageInitFunc init;
    PackageCleanupFunc cleanup;
    PackageListStartFunc list_files_start;
    PackageListNextFunc list_files_next;
    PackageHasPathFunc has_path; // NULL可（NULLの場合、戻り値が0以外とみなす）
    PackageFileInfoFunc file_info;
    PackageDecompressFunc decompress;

    /* モジュール用データポインタ（任意。内部データなどに使える） */
    void *module_data;

    /* 以下はモジュール管理用。モジュールによる定義は禁止 */
    uint8_t available;  // 0以外＝初期化済みで利用可能
    uint8_t prefixlen;  // strlen(prefix)

};


/* 各モジュールの情報構造体宣言 */
extern PackageModuleInfo
    package_info_aquaria;

/*************************************************************************/
/*************************************************************************/

#endif  // RESOURCE_PACKAGE_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
