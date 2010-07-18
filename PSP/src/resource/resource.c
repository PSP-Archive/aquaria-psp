/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/resource/resource.c: Resource management functions.
 */

/*
 * リソースは、配列とハッシュテーブルで管理される。resource_create()では内部
 * データ構造体（ResourceManagerPrivate）と、リソース情報構造体（ResourceInfo）
 * の配列を確保し、resource_load()またはresource_new_*()で登録されたリソース
 * を配列に格納する。リソース情報の検索・登録・削除は、3つの補助関数
 * （find_resource()、add_resource()、del_resource()）で行う。
 *
 * リソースの登録と削除（ResourceInfoエントリーの確保と解放）は、あくまで
 * typeフィールドの操作によって行われる。これは、確保順を保つために
 * ResourceInfo配列内の要素順を変更した場合、リソースリンクのリスト
 * (ResourceInfo.link_nextによって成り立つ循環リスト）が破壊されてしまう
 * ためだが、リソースの登録と削除が交互に行われると、必然的にリソースデータ
 * の順序が確保順ではなくなってくる。しかしresource_free_all()による全解放
 * は確保順の逆に実行しなければならないので、別の32ビットカウンターフィール
 * ドで確保順を記録し、解放時は値の大きいものから小さいものの順に解放する。
 * 1つのリソース管理構造体において、特定のリソースを確保してから別リソース
 * の確保・解放を2^32回以上繰り返すと、全解放時にの解放順がおかしくなる。
 *
 * 登録において空きエントリーがない場合、mem_realloc()によって配列を長くし、
 * 新たにできたエントリーを利用する。この際、管理データが長時間メモリに残る
 * と想定されるため、MEM_ALLOC_TEMPとMEM_ALLOC_TOPフラグを指定し、通常メモリ
 * プールの断片化をなるべく避ける。
 */

#include "../common.h"
#include "../memory.h"
#include "../resource.h"
#include "../sound.h"
#include "../sysdep.h"
#include "../texture.h"

#include "package.h"
#include "tinflate.h"

/* resource.hや、確保用に呼び出すtexture.hのデバッグ用マクロを解除。また、
 * 関数宣言がごちゃごちゃにならないように、デバッグ有効時のみfile/line引数
 * を追加するマクロも定義する */
#ifdef DEBUG
# undef resource_create
# undef resource_delete
# undef resource_load_data
# undef resource_load_sound
# undef resource_load_texture
# undef resource_sync
# undef resource_wait
# undef resource_new_data
# undef resource_strdup
# undef resource_new_texture
# undef resource_take_data
# undef resource_link
# undef resource_free
# undef resource_free_all
# undef texture_new
# undef texture_parse
# undef texture_destroy
# define __DEBUG_PARAMS  , const char *file, int line
# define __DEBUG_ARGS    , file, line
#else
# define __DEBUG_PARAMS  /*nothing*/
# define __DEBUG_ARGS    /*nothing*/
#endif

/*************************************************************************/
/****************** リソース管理構造体の内部データ定義 *******************/
/*************************************************************************/

/* 全てのパッケージモジュールの情報ポインタ */
static PackageModuleInfo * const packages[] = {
    &package_info_aquaria,
};

/* 現在ファイルリスト取得中のパッケージ（NULL＝なし） */
static PackageModuleInfo *filelist_package;

/*-----------------------------------------------------------------------*/

/* リソースの種類 */
typedef enum ResourceType_ {
    RES_UNUSED = 0,     // 未使用
    RES_UNKNOWN,        // 不明
    RES_DATA,           // 一般データ
    RES_TEXTURE,        // テキスチャ
} ResourceType;

/*----------------------------------*/

/* リソースのロード中にのみ使われる情報（ロード完了後は解放する） */
typedef struct LoadInfo_ {
    /* ファイルバッファへのポインタ */
    void *file_data;
    /* ロード関連フラグ */
    uint8_t need_close;   // 1＝ロード後、ファイルをクローズする
    uint8_t need_finish;  // 1＝読み込み完了、最終処理待ち
    /* メモリアライメント */
    uint16_t mem_align;
    /* メモリ確保フラグ */
    uint32_t mem_flags;
#ifdef DEBUG
    /* メモリの種類（debug_mem_*()に渡すもの） */
    int32_t mem_type;
#endif
    /* パッケージからの圧縮フラグ */
    int compressed;
    /* 圧縮データのサイズと圧縮解除後のデータサイズ（ともにバイト）*/
    uint32_t compressed_size, data_size;
    /* 読み込み要求識別子 */
    int32_t read_request;
    /* 関連パッケージの情報構造体 */
    PackageModuleInfo *pkginfo;
    /* 読み込みファイルポインタ（パッケージまたは通常ファイル） */
    SysFile *fp;
} LoadInfo;

/*----------------------------------*/

/* 互いに参照するので、データ型を先に宣言する */
typedef struct ResourceInfo_ ResourceInfo;
typedef struct ResourceManagerPrivate_ ResourceManagerPrivate;

/* 一つのリソースに関するデータ */
struct ResourceInfo_ {
    /* このリソースを管理しているリソース管理構造体（の内部データ） */
    ResourceManagerPrivate *owner;
    /* リソースリンクの循環リスト */
    ResourceInfo *link_next;
    /* データバッファ等へのポインタ */
    void **data_ptr;     // ユーザーから渡されたデータポインタ格納先
    uint32_t *size_ptr;  // 一般データにおけるデータ量（任意）
    /* リソースの種類 */
    ResourceType type;
    /* 同期マーク値（load()・new()時のResourceManagerPrivate.mark） */
    int32_t mark;
    /* 確保順（低い数値ほど早く確保されている） */
    int32_t alloc_order;
    /* ロード用の情報（NULL以外＝ロード中） */
    LoadInfo *load_info;
#ifdef DEBUG
    /* パス名（デバッグメッセージ用） */
    char debug_path[100];
#endif
};

/* リソース管理構造体の内部データ（ResourceManager.privateが指すもの） */
struct ResourceManagerPrivate_ {
    /* リソース情報の配列とサイズ（エントリー数）。リソース破棄時は、
     * ハッシュリストへの影響を考えてエントリーを移動せず、登録時に空き
     * エントリーを探す。 */
    ResourceInfo *resources;
    int16_t resources_size;
    /* 静的バッファ利用フラグ */
    uint8_t private_is_static;    // この構造体が静的バッファに格納されている
    uint8_t resources_is_static;  // resources[]配列が静的バッファ内にある
    /* 現在の同期用マーク値。resource_mark()が呼び出される度に1を加算して
     * 返し、sync()では指定されたマーク値未満の読み込み未完了リソースの有無
     * を調べる。なお、wraparoundによる誤動作を回避するため、「未満」判定は
     * 通常の比較演算ではなく、両値の差で判定する。 */
    int32_t mark;
    /* 現在の確保順。新しいResourceInfoを確保するたびに現在の値を
     * ResourceInfo.alloc_orderに格納し、1を加算する */
    int32_t alloc_order;
#ifdef DEBUG
    /* 確保元（デバッグメッセージ用） */
    char owner[100];
#endif
};

/*************************************************************************/
/******************* 補助マクロ定義とローカル関数宣言 ********************/
/*************************************************************************/

/* リソース管理構造体が初期化されていることを確認するマクロ。resmgrがNULLで
 * なく、初期化されていれば、内部データへのポインタをprivate（変数）に格納
 * する。NULLまたは未初期化の場合は、指定のエラー処理を行う（必ずreturnする
 * こと）。 */
#define VALIDATE_RESMGR(resmgr,private_var,error_return) do {   \
    if (UNLIKELY(!resmgr)) {                                    \
        DMSG(#resmgr " == NULL");                               \
        error_return;                                           \
    }                                                           \
    private_var = (ResourceManagerPrivate *)resmgr->private;    \
    if (UNLIKELY(!private_var)) {                               \
        DMSG(#resmgr " is not initialized");                    \
        error_return;                                           \
    }                                                           \
} while (0)


static uint32_t convert_mem_flags(const uint32_t res_flags);
static PackageModuleInfo *find_package(const char *path);

static ResourceInfo *find_resource(ResourceManagerPrivate *private,
                                   void **data_ptr);
static ResourceInfo *add_resource(ResourceManagerPrivate *private,
                                  ResourceType type, void **data_ptr
                                  __DEBUG_PARAMS);
static void del_resource(ResourceInfo *resinfo);
static int load_resource(ResourceInfo *resinfo, const char *path,
                         uint16_t align, uint32_t flags __DEBUG_PARAMS);
static void free_resource(ResourceInfo *resinfo __DEBUG_PARAMS);

static int load_from_package(ResourceInfo *resinfo, const char *path
                             __DEBUG_PARAMS);
static int load_from_file(ResourceInfo *resinfo, const char *path
                          __DEBUG_PARAMS);
static void finish_load(ResourceInfo *resinfo __DEBUG_PARAMS);

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * resource_init:  リソース管理機能を初期化する。失敗することはない。
 * パッケージファイル操作モジュールの初期化を行うため、ファイルアクセスに
 * よって時間がかかることがある。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void resource_init(void)
{
    /* サイズマクロと実際の構造体サイズの整合性を確認する。非デバッグ時でも
     * すぐ分かるように、ミスマッチ時は初期化を中止する */
    PRECOND_SOFT(RESOURCE_SIZE1 == sizeof(ResourceManagerPrivate), return);
    PRECOND_SOFT(RESOURCE_SIZE2 == sizeof(ResourceInfo), return);

    int i;
    for (i = 0; i < lenof(packages); i++) {
        PRECOND_SOFT(packages[i] != NULL, continue);
        PRECOND_SOFT(packages[i]->available == 0, continue);
        PRECOND_SOFT(packages[i]->prefix != NULL, continue);
        PRECOND_SOFT(packages[i]->init != NULL, continue);
        PRECOND_SOFT(packages[i]->cleanup != NULL, continue);
        PRECOND_SOFT(packages[i]->file_info != NULL, continue);
        PRECOND_SOFT(packages[i]->decompress != NULL, continue);
        uint32_t prefixlen = strlen(packages[i]->prefix);
        PRECOND_SOFT(prefixlen <= 255, continue);
        packages[i]->prefixlen = prefixlen;
        packages[i]->available = (*packages[i]->init)(packages[i]);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * resource_cleanup:  リソース管理機能の後片付けを行い、パッケージファイルを
 * クローズする。ただし、使用中のリソース管理構造体の破棄はしない。リソース
 * 読み込む中に呼び出した場合、読み込み終了まで停止したり、メモリリークする
 * 可能性がある。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void resource_cleanup(void)
{
    int i;
    for (i = 0; i < lenof(packages); i++) {
        PRECOND_SOFT(packages[i] != NULL, continue);
        if (packages[i]->available) {
            (*packages[i]->cleanup)(packages[i]);
            packages[i]->available = 0;
        }
    }

    filelist_package = NULL;
}

/*************************************************************************/

/**
 * resource_create:  リソース管理構造体を初期化する。すでに初期化された
 * リソース管理構造体を渡した場合、何もせずに成功を返す。
 *
 * num_resourcesには、同時に使われると想定されるリソース数を指定することで
 * メモリの断片化を回避できる。不明の場合は0でよい。
 *
 * 特定の管理構造体を他のリソース管理関数で利用する前に、必ずこの関数を呼び
 * 出さなければならない。なお、初めて初期化するときは構造体のメモリがゼロク
 * リアされている必要がある（resource_delete()後に再クリアしなくてよい）。
 *
 * 【引　数】       resmgr: 初期化するリソース管理構造体へのポインタ
 * 　　　　　num_resources: 想定される最大リソース数（0＝デフォルト）
 * 【戻り値】0以外＝成功、0＝失敗
 */
int resource_create(ResourceManager *resmgr, int num_resources __DEBUG_PARAMS)
{
    if (UNLIKELY(resmgr == NULL)) {
        DMSG("resmgr == NULL");
        return 0;
    }
    if (UNLIKELY(resmgr->private)) {
        return 1;
    }
    if (UNLIKELY(num_resources < 0)) {
        DMSG("Invalid num_resources: %d", num_resources);
        return 0;
    }

    /* 静的バッファが設定されている場合、実際に利用できることを確認する */
    if (resmgr->static_size > 0) {
        if (UNLIKELY(resmgr->static_buffer == NULL)) {
            DMSG("%p: static_size is %d but static_buffer is NULL!",
                 resmgr, resmgr->static_size);
            resmgr->static_size = 0;
        } else if (UNLIKELY((uintptr_t)resmgr->static_buffer
                            % sizeof(uintptr_t) != 0)) {
            DMSG("%p: static_buffer %p is not %d-byte aligned!",
                 resmgr, resmgr->static_buffer, (int)sizeof(uintptr_t));
            resmgr->static_size = 0;
        }
    }

    ResourceManagerPrivate *private;
    if (resmgr->static_size >= sizeof(*private)) {
        private = (ResourceManagerPrivate *)resmgr->static_buffer;
        mem_clear(private, sizeof(*private));
        private->private_is_static = 1;
    } else {
        private = debug_mem_alloc(sizeof(*private), 0, MEM_ALLOC_CLEAR,
                                  file, line, MEM_INFO_MANAGE);
        if (!private) {
            DMSG("Out of memory for resmgr->private");
            return 0;
        }
    }
    if (num_resources == 0) {
        num_resources = 100;  // 適当に
    }
    const uint32_t resources_size = sizeof(ResourceInfo) * num_resources;
    if (resmgr->static_size >= sizeof(*private) + resources_size) {
        private->resources = (ResourceInfo *)
            ((uint8_t *)resmgr->static_buffer + sizeof(*private));
        private->resources_is_static = 1;
        mem_clear(private->resources, resources_size);
    } else {
        private->resources = debug_mem_alloc(resources_size, 0, MEM_ALLOC_CLEAR,
                                             file, line, MEM_INFO_MANAGE);
        if (!private->resources) {
            DMSG("Out of memory for %d ResourceInfos", num_resources);
            if (!private->private_is_static) {
                mem_free(private);
            }
            return 0;
        }
        private->resources_is_static = 0;
    }

    private->resources_size = num_resources;
    private->mark = 0;
    private->alloc_order = -0x80000000;
#ifdef DEBUG
    snprintf(private->owner, sizeof(private->owner), "%s:%d", file, line);
#endif
    resmgr->private = private;
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * resource_delete:  ロードされているリソース及び管理データを破棄し、メモリ
 * を解放する。初期化されていないリソース管理構造体を渡した場合、何もしない。
 * 呼び出し後、同じ管理構造体を再び利用する前に必ずresource_create()を呼び
 * 出さなければならない。
 *
 * 読み込み中のリソースがある場合、その読み込みが完了するまで停止する。
 *
 * 【引　数】resmgr: 初期化するリソース管理構造体へのポインタ
 * 【戻り値】なし
 */
void resource_delete(ResourceManager *resmgr __DEBUG_PARAMS)
{
    if (UNLIKELY(resmgr == NULL)) {
        DMSG("resmgr == NULL");
        return;
    }
    if (UNLIKELY(!resmgr->private)) {
        /* エラー扱いではないのでマクロを使わない */
        return;
    }
    resource_free_all(resmgr __DEBUG_ARGS);
    ResourceManagerPrivate *private = (ResourceManagerPrivate *)resmgr->private;
    if (!private->resources_is_static) {
        debug_mem_free(private->resources, file, line, -1);
    }
    if (!private->private_is_static) {
        debug_mem_free(resmgr->private, file, line, -1);
    }
    resmgr->private = NULL;
}

/*************************************************************************/

/**
 * resource_exists:  リソースの存在の有無を確認する。resource_load_*()で
 * 実際にデータを読み込んで確認するよりは早い。
 *
 * 【引　数】path: リソースファイルのパス名
 * 【戻り値】0以外＝リソースが存在する、0＝リソースが存在しない
 */
int resource_exists(const char *path)
{
    PackageModuleInfo *pkginfo = find_package(path);
    if (pkginfo) {
        PRECOND_SOFT(pkginfo->prefix != NULL, return 0);
        path += strlen(pkginfo->prefix);
        SysFile *fp;
        uint32_t pos, len, size;
        int compressed;
        if ((*pkginfo->file_info)(pkginfo, path, &fp, &pos, &len,
                                  &compressed, &size)) {
            return 1;
        }
        if (!pkginfo->has_path || (*pkginfo->has_path)(pkginfo, path)) {
            return 0;
        }
    }

    SysFile *fp = sys_file_open(path);
    if (fp) {
        sys_file_close(fp);
        return 1;
    } else {
        return 0;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * resource_list_files_start():  パッケージ内の全ファイルをリストアップする
 * ための初期化を行う。同時に複数のパッケージからファイルリストを取得する
 * ことはできない。
 *
 * 【引　数】path: ファイルリストを取得するパッケージのプレフィックスパス名
 * 【戻り値】0以外＝成功、0＝失敗
 */
int resource_list_files_start(const char *path)
{
    filelist_package = find_package(path);
    if (filelist_package) {
        (*filelist_package->list_files_start)(filelist_package);
        return 1;
    } else {
        DMSG("No package found for prefix path %s", path);
        return 0;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * resource_list_files_next():  次のデータファイルのパス名を返す。
 * resource_list_files_start()を先に呼び出さなければならない。
 *
 * 【引　数】なし
 * 【戻り値】パス名（NULL＝リスト完了）
 */
const char *resource_list_files_next(void)
{
    if (filelist_package) {
        const char *path =
            (*filelist_package->list_files_next)(filelist_package);
        if (!path) {
            filelist_package = NULL;
        }
        return path;
    } else {
        DMSG("No active file list package");
        return NULL;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * resource_open_as_file:  指定されたリソースをファイルとしてオープンする。
 * 成功した場合に返されたファイルハンドルは、パッケージファイルのコピーかも
 * しれないので、絶対位置へシークする際は*offset_retに格納されたオフセット
 * 値を加算しなければならない。また、同じ理由でsys_file_size()からファイル
 * サイズを取得できない可能性があるので、代わりに*filesize_retに格納された
 * 値を使わなければならない。
 *
 * 圧縮されているリソースはオープンできない。
 *
 * 【引　数】        path: リソースファイルのパス名
 * 　　　　　  offset_ret: ファイルハンドルにおけるファイルデータのオフセット
 * 　　　　　filesize_ret: ファイルのサイズ（バイト）
 * 【戻り値】ファイルハンドル（オープンに失敗した場合はNULL）
 */
SysFile *resource_open_as_file(const char *path, uint32_t *offset_ret,
                               uint32_t *filesize_ret)
{
    PRECOND_SOFT(path != NULL, return NULL);
    PRECOND_SOFT(offset_ret != NULL, return NULL);
    PRECOND_SOFT(filesize_ret != NULL, return NULL);

    /* まずはパッケージで探す */

    PackageModuleInfo *pkginfo = find_package(path);
    if (pkginfo) {
        path += pkginfo->prefixlen;
        SysFile *fp;
        uint32_t pos, len, size;
        int compressed;
        if ((*pkginfo->file_info)(pkginfo, path, &fp, &pos, &len,
                                  &compressed, &size)) {
            if (compressed) {
                DMSG("%s is compressed, can't open as file", path);
                return NULL;
            }
            fp = sys_file_dup(fp);
            if (!fp) {
                DMSG("Failed to dup package file handle for %s: %s", path,
                     sys_last_errstr());
                return NULL;
            }
            if (sys_file_seek(fp, pos, FILE_SEEK_SET) == -1) {
                DMSG("Failed to seek to start of file for %s", path);
                sys_file_close(fp);
                return NULL;
            }
            *offset_ret = pos;
            *filesize_ret = len;
            return fp;
        }
    }

    /* パッケージにはなかったので、ファイルシステムからオープンする */

    SysFile *fp = sys_file_open(path);
    if (fp) {
        *offset_ret = 0;
        *filesize_ret = sys_file_size(fp);
    }
    return fp;
}

/*************************************************************************/

/**
 * resource_load_data:  一般データリソースのためのメモリを確保し、ロードを
 * 開始する。関数が成功した場合でも、すぐにリソースのデータを取得できるとは
 * 限らないので、取得する前に必ずresource_sync()でロードが完了していること
 * を確認しなければならない。
 *
 * 【引　数】  resmgr: リソース管理構造体へのポインタ
 * 　　　　　data_ptr: リソースポインタ（データポインタを格納する変数への
 * 　　　　　          　ポインタ）
 * 　　　　　size_ptr: データサイズ（バイト）を格納する変数へのポインタ
 * 　　　　　    path: リソースファイルのパス名
 * 　　　　　   align: メモリアライメント（バイト単位、0＝デフォルト）
 * 　　　　　   flags: メモリ確保フラグ（RES_ALLOC_*）
 * 【戻り値】0以外＝成功、0＝失敗
 */
int resource_load_data(ResourceManager *resmgr, void **data_ptr,
                       uint32_t *size_ptr, const char *path,
                       uint16_t align, uint32_t flags
                       __DEBUG_PARAMS)
{
    PRECOND((flags & RES_ALLOC_CLEAR) == 0);

    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return 0);
    if (UNLIKELY(!data_ptr)) {
        DMSG("data_ptr == NULL");
        return 0;
    }

    if (find_resource(private, data_ptr)) {
        DMSG("Attempt to register pointer %p more than once (called from"
             " %s:%d for %s)", data_ptr, file, line, path);
        return 0;
    }

    ResourceInfo *resinfo = add_resource(private, RES_DATA, data_ptr
                                         __DEBUG_ARGS);
    if (UNLIKELY(!resinfo)) {
        return 0;
    }
    resinfo->mark = private->mark;
    resinfo->size_ptr = size_ptr;

    /* ユーザポインタにはとりあえずNULLを格納し、ロード用のファイルバッファ
     * をfile_dataフィールドで管理する。ロードが完了しないうちにファイルバッ
     * ファを格納すると、ユーザがsync()せずに利用してクラッシュ等の誤動作を
     * 引き起こす恐れがある */
    *data_ptr = NULL;

    const int res = load_resource(resinfo, path, align,
                                  convert_mem_flags(flags) __DEBUG_ARGS);
    if (UNLIKELY(!res)) {
        del_resource(resinfo);
    }
    return res;
}

/*-----------------------------------------------------------------------*/

/**
 * resource_load_texture:  テキスチャリソースのためのメモリを確保し、ロード
 * を開始する。関数が成功した場合でも、すぐにリソースのデータを取得できると
 * は限らないので、取得する前に必ずresource_sync()でロードが完了しているこ
 * とを確認しなければならない。
 *
 * 【引　数】     resmgr: リソース管理構造体へのポインタ
 * 　　　　　texture_ptr: リソースポインタ（テキスチャポインタを格納する変数
 * 　　　　　             　へのポインタ）
 * 　　　　　       path: リソースファイルのパス名
 * 　　　　　      flags: メモリ確保フラグ（RES_ALLOC_*）
 * 【戻り値】0以外＝成功、0＝失敗
 */
int resource_load_texture(ResourceManager *resmgr, Texture **texture_ptr,
                          const char *path, uint32_t flags __DEBUG_PARAMS)
{
    PRECOND((flags & RES_ALLOC_CLEAR) == 0);

    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return 0);
    if (UNLIKELY(!texture_ptr)) {
        DMSG("texture_ptr == NULL");
        return 0;
    }

    if (find_resource(private, (void **)texture_ptr)) {
        DMSG("Attempt to register pointer %p more than once (called from"
             " %s:%d for %s)", texture_ptr, file, line, path);
        return 0;
    }

    ResourceInfo *resinfo = add_resource(private, RES_TEXTURE,
                                         (void **)texture_ptr __DEBUG_ARGS);
    if (UNLIKELY(!resinfo)) {
        return 0;
    }
    resinfo->mark = private->mark;

    *texture_ptr = NULL;

    const int res = load_resource(resinfo, path, 64,
                                  convert_mem_flags(flags) __DEBUG_ARGS);
    if (UNLIKELY(!res)) {
        del_resource(resinfo);
    }
    return res;
}

/*************************************************************************/

/**
 * resource_mark:  同期を取るためのマークを登録する。失敗することはないが、
 * 同期を取らずに連続1000回以上呼び出した場合の動作は不定。
 *
 * 【引　数】resmgr: リソース管理構造体へのポインタ
 * 【戻り値】マーク値（必ず0以外）
 */
int resource_mark(ResourceManager *resmgr)
{
    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return 0);

    private->mark++;
    if (private->mark == 0) {
        private->mark++;
    }
    return private->mark;
}

/*-----------------------------------------------------------------------*/

/**
 * resource_sync:  resource_mark()で登録したマークとの同期状況を返す。指定
 * したマーク以前にresource_load()を呼び出されたすべてのリソースがロードさ
 * れていれば、0以外の値（同期済み）を返す。
 *
 * 【引　数】resmgr: リソース管理構造体へのポインタ
 * 　　　　　  mark: マーク値
 * 【戻り値】0以外＝同期済み、0＝ロード未完了リソース有り
 */
int resource_sync(ResourceManager *resmgr, int mark __DEBUG_PARAMS)
{
    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return 0);

    /* まずは読み込み中のリソースがあるかどうかをチェック */
    int index;
    for (index = 0; index < private->resources_size; index++) {
        if (private->resources[index].type != RES_UNUSED) {
            LoadInfo *load_info = private->resources[index].load_info;
            if (load_info && !load_info->need_finish
             && private->resources[index].mark - mark < 0 // 注：単純比較はダメ
            ) {
                if (!sys_file_poll_async(load_info->read_request)) {
                    sys_file_wait_async(load_info->read_request);
                    load_info->read_request = 0;
                    load_info->need_finish = 1;
                } else {
                    return 0;
                }
            }
        }
    }

    /* 全て読み込み完了なので、最終処理を行う。メモリ断片化回避の観点から、
     * 逆順に処理する（圧縮バッファがメモリプールの反対側で確保されている
     * ため）。なお、このマーク値以降で読み込み完了リソースがあっても、
     * 時間がかかる可能性もあるため今は処理しない */
    for (index = private->resources_size-1; index >= 0; index--) {
        if (private->resources[index].type != RES_UNUSED
         && private->resources[index].load_info
         && private->resources[index].load_info->need_finish
         && private->resources[index].mark - mark < 0  // 注：単純比較はダメ
        ) {
            finish_load(&private->resources[index] __DEBUG_ARGS);
        }
    }

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * resource_wait:  resource_mark()で登録したマークとの同期が取れるまで待つ。
 * 関数から戻った時、指定したマーク以前にresource_load()を呼び出されたすべて
 * のリソースがロードされている。
 *
 * 【引　数】resmgr: リソース管理構造体へのポインタ
 * 　　　　　  mark: マーク値
 * 【戻り値】なし
 */
extern void resource_wait(ResourceManager *resmgr, int mark __DEBUG_PARAMS)
{
    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return);

    /* 読み込み中のリソースがあれば、読み込み完了を待つ */
    int index;
    for (index = 0; index < private->resources_size; index++) {
        if (private->resources[index].type != RES_UNUSED) {
            LoadInfo *load_info = private->resources[index].load_info;
            if (load_info && !load_info->need_finish
             && private->resources[index].mark - mark < 0 // 注：単純比較はダメ
            ) {
                sys_file_wait_async(load_info->read_request);
                load_info->read_request = 0;
                load_info->need_finish = 1;
            }
        }
    }

    /* 全て読み込み完了なので、最終処理を行う */
    for (index = private->resources_size-1; index >= 0; index--) {
        if (private->resources[index].type != RES_UNUSED
         && private->resources[index].load_info
         && private->resources[index].load_info->need_finish
         && private->resources[index].mark - mark < 0  // 注：単純比較はダメ
        ) {
            finish_load(&private->resources[index] __DEBUG_ARGS);
        }
    }
}

/*************************************************************************/

/**
 * resource_new_data:  新規一般データリソースを作成する。
 *
 * 【引　数】  resmgr: リソース管理構造体へのポインタ
 * 　　　　　data_ptr: リソースポインタ（データポインタを格納する変数への
 * 　　　　　          　ポインタ）
 * 　　　　　    size: データサイズ（バイト）
 * 　　　　　   align: メモリアライメント（バイト単位、0＝デフォルト）
 * 　　　　　   flags: メモリ確保フラグ（RES_ALLOC_*）
 * 【戻り値】0以外＝成功、0＝失敗
 */
int resource_new_data(ResourceManager *resmgr, void **data_ptr,
                      uint32_t size, uint32_t align, uint32_t flags
                      __DEBUG_PARAMS)
{
    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return 0);
    if (UNLIKELY(!data_ptr)) {
        DMSG("data_ptr == NULL");
        return 0;
    }

    if (find_resource(private, data_ptr)) {
        DMSG("Attempt to register pointer %p more than once (called from"
             " %s:%d)", data_ptr, file, line);
        return 0;
    }

    ResourceInfo *resinfo = add_resource(private, RES_DATA, data_ptr
                                         __DEBUG_ARGS);
    if (UNLIKELY(!resinfo)) {
        return 0;
    }
    *data_ptr = debug_mem_alloc(size, align, convert_mem_flags(flags),
                                file, line, -1);
    if (UNLIKELY(!*data_ptr)) {
        del_resource(resinfo);
        return 0;
    }
#ifdef DEBUG
    snprintf(resinfo->debug_path, sizeof(resinfo->debug_path),
             "%s:%d", file, line);
#endif
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * resource_strdup:  strdup()のように文字列を複製し、複製をデータリソースと
 * して管理する。
 *
 * 【引　数】  resmgr: リソース管理構造体へのポインタ
 * 　　　　　data_ptr: リソースポインタ（新しい文字列のデータポインタを格納
 * 　　　　　          　する変数へのポインタ）
 * 　　　　　     str: 複製する文字列
 * 　　　　　   flags: メモリ確保フラグ（RES_ALLOC_*）
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int resource_strdup(ResourceManager *resmgr, char **data_ptr,
                           const char *str, uint32_t flags __DEBUG_PARAMS)
{
    PRECOND((flags & RES_ALLOC_CLEAR) == 0);

    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return 0);
    if (UNLIKELY(!data_ptr)) {
        DMSG("data_ptr == NULL");
        return 0;
    }
    if (UNLIKELY(!str)) {
        DMSG("str == NULL");
        return 0;
    }

    if (find_resource(private, (void **)data_ptr)) {
        DMSG("Attempt to register pointer %p more than once (called from"
             " %s:%d)", data_ptr, file, line);
        return 0;
    }

    ResourceInfo *resinfo = add_resource(private, RES_DATA, (void **)data_ptr
                                         __DEBUG_ARGS);
    if (UNLIKELY(!resinfo)) {
        return 0;
    }
    *data_ptr = debug_mem_strdup(str, convert_mem_flags(flags),
                                 file, line, -1);
    if (UNLIKELY(!*data_ptr)) {
        del_resource(resinfo);
        return 0;
    }
#ifdef DEBUG
    snprintf(resinfo->debug_path, sizeof(resinfo->debug_path),
             "%s:%d", file, line);
#endif
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * resource_new_texture:  新規テキスチャリソースを作成する。
 *
 * 【引　数】     resmgr: リソース管理構造体へのポインタ
 * 　　　　　texture_ptr: リソースポインタ（テキスチャポインタを格納する変数
 * 　　　　　             　へのポインタ）
 * 　　　　　      width: 作成するテキスチャの幅（ピクセル）
 * 　　　　　     height: 作成するテキスチャの高さ（ピクセル）
 * 　　　　　      flags: メモリ確保フラグ（RES_ALLOC_*）
 * 【戻り値】0以外＝成功、0＝失敗
 */
int resource_new_texture(ResourceManager *resmgr, Texture **texture_ptr,
                         int width, int height, uint32_t flags __DEBUG_PARAMS)
{
    PRECOND((flags & RES_ALLOC_CLEAR) == 0);

    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return 0);
    if (UNLIKELY(!texture_ptr)) {
        DMSG("texture_ptr == NULL");
        return 0;
    }

    if (find_resource(private, (void **)texture_ptr)) {
        DMSG("Attempt to register pointer %p more than once (called from"
             " %s:%d)", texture_ptr, file, line);
        return 0;
    }

    ResourceInfo *resinfo = add_resource(private, RES_TEXTURE,
                                         (void **)texture_ptr __DEBUG_ARGS);
    if (UNLIKELY(!resinfo)) {
        return 0;
    }
    *texture_ptr = texture_new(width, height,
                               convert_mem_flags(flags) __DEBUG_ARGS);
    if (UNLIKELY(!*texture_ptr)) {
        del_resource(resinfo);
        return 0;
    }
#ifdef DEBUG
    snprintf(resinfo->debug_path, sizeof(resinfo->debug_path),
             "%s:%d", file, line);
#endif
    return 1;
}

/*************************************************************************/

/**
 * resource_take_data:  管理されていない一般データリソースを管理対象に加える。
 *
 * 【引　数】  resmgr: リソース管理構造体へのポインタ
 * 　　　　　data_ptr: リソースポインタ（データポインタが格納されている変数
 * 　　　　　          　へのポインタ）
 * 【戻り値】0以外＝成功、0＝失敗
 */
int resource_take_data(ResourceManager *resmgr, void **data_ptr
                       __DEBUG_PARAMS)
{
    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return 0);
    if (UNLIKELY(!data_ptr)) {
        DMSG("data_ptr == NULL");
        return 0;
    }

    if (find_resource(private, data_ptr)) {
        DMSG("Attempt to register pointer %p more than once (called from"
             " %s:%d)", data_ptr, file, line);
        return 0;
    }

#ifdef DEBUG
    /* 「data pointer is not NULL」警告を回避 */
    void *saved_ptr = *data_ptr;
    *data_ptr = NULL;
#endif
    ResourceInfo *resinfo = add_resource(private, RES_DATA, data_ptr
                                         __DEBUG_ARGS);
#ifdef DEBUG
    *data_ptr = saved_ptr;
#endif
    if (UNLIKELY(!resinfo)) {
        return 0;
    }
#ifdef DEBUG
    snprintf(resinfo->debug_path, sizeof(resinfo->debug_path),
             "%s:%d", file, line);
#endif
    return 1;
}

/*************************************************************************/

/**
 * resource_link:  指定されたリソースへのリンクを作成する。Unix系のファイル
 * システム同様、リンクされているリソースに対して破棄操作を実行しても、リン
 * クが1つ以上残っていればリソースは破棄されない。リンク作成後は通常のリソ
 * ースと同様に扱うことができ、resource_free()もしくはresource_free_all()で
 * リンクを解除できる。
 *
 * 【引　数】    resmgr: リンクを登録するリソース管理構造体へのポインタ
 * 　　　　　old_resmgr: 既存リソースのリソース管理構造体へのポインタ
 * 　　　　　   old_ptr: 既存リソースのリソースポインタ
 * 　　　　　   new_ptr: 新たに作成するリソースのリソースポインタ
 * 【戻り値】0以外＝成功、0＝失敗
 */
int resource_link(ResourceManager *resmgr, ResourceManager *old_resmgr,
                  void **old_ptr, void **new_ptr __DEBUG_PARAMS)
{
    ResourceManagerPrivate *private, *old_private;
    VALIDATE_RESMGR(resmgr, private, return 0);
    VALIDATE_RESMGR(old_resmgr, old_private, return 0);
    if (UNLIKELY(!old_ptr)) {
        DMSG("old_ptr == NULL");
        return 0;
    }
    if (UNLIKELY(!new_ptr)) {
        DMSG("new_ptr == NULL");
        return 0;
    }

    /* 新しいリソース情報を作成する（種類は既存リソースと同じ） */
    ResourceInfo *old_resinfo = find_resource(old_private, old_ptr);
    if (UNLIKELY(!old_resinfo)) {
        return 0;
    }
    ResourceInfo *new_resinfo = add_resource(private, old_resinfo->type,
                                             new_ptr __DEBUG_ARGS);
    if (UNLIKELY(!new_resinfo)) {
        return 0;
    }
#ifdef DEBUG
    snprintf(new_resinfo->debug_path, sizeof(new_resinfo->debug_path),
             "%s:%d", file, line);
#endif

    /* リンクリストに追加する */
    ResourceInfo *prev = old_resinfo->link_next;
    int tries = 10000;
    while (prev->link_next != old_resinfo) {
        prev = prev->link_next;
        tries--;
        if (UNLIKELY(tries <= 0)) {
            DMSG("BUG!! endless linked list on resource %p (%s) in resmgr"
                 " %p (%s)", old_resinfo, old_resinfo->debug_path,
                old_private, old_private->owner);
            del_resource(new_resinfo);
            return 0;
        }
    }
    prev->link_next = new_resinfo;
    new_resinfo->link_next = old_resinfo;

    /* データポインタを設定して完了 */
    *new_ptr = *old_ptr;
    return 1;
}

/*************************************************************************/

/**
 * resource_free:  指定されたリソースを破棄する。リソースが存在しない場合、
 * またはdata_ptr==NULLの場合は何もしない。
 *
 * 指定されたリソースが読み込み中の場合、読み込みが完了するまで停止する。
 *
 * 【引　数】  resmgr: リソース管理構造体へのポインタ
 * 　　　　　data_ptr: リソースポインタ
 * 【戻り値】なし
 */
void resource_free(ResourceManager *resmgr, void **data_ptr __DEBUG_PARAMS)
{
    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return);
    if (UNLIKELY(!data_ptr)) {
        return;
    }

    ResourceInfo *resinfo = find_resource(private, data_ptr);
    if (UNLIKELY(!resinfo)) {
        return;
    }
    if (resinfo->load_info) {
        sys_file_abort_async(resinfo->load_info->read_request);
    }
    free_resource(resinfo __DEBUG_ARGS);
    del_resource(resinfo);
}

/*-----------------------------------------------------------------------*/

/**
 * resource_free_all:  リソース管理構造体で管理されている全てのリソースを
 * 一括破棄する。
 *
 * 読み込み中のリソースがある場合、その読み込みが完了するまで停止する。
 *
 * 【引　数】resmgr: リソース管理構造体へのポインタ
 * 【戻り値】なし
 */
void resource_free_all(ResourceManager *resmgr __DEBUG_PARAMS)
{
    ResourceManagerPrivate *private;
    VALIDATE_RESMGR(resmgr, private, return);

    /* まずすべての読み込みを中止して、それから解放を始める。一個ずつ読み込み
     * を中止して完了を待つと、その間に次の読み込みが始まってしまい、余計に
     * 時間がかかってしまう */
    int index;
    for (index = 0; index < private->resources_size; index++) {
        if (private->resources[index].type != RES_UNUSED
         && private->resources[index].load_info
        ) {
            LoadInfo *load_info = private->resources[index].load_info;
            sys_file_abort_async(load_info->read_request);
            if (load_info->fp && load_info->need_close) {
                sys_file_close(load_info->fp);
                load_info->fp = NULL;
                load_info->need_close = 0;
            }
        }
    }
    /* 必ず確保順の逆に解放する。例えばリソースポインタを格納した配列の
     * メモリバッファをその要素のリソースより先に解放してしまうと、要素の
     * リソースを解放する時に不正メモリアクセスが発生する。（現行実装上、
     * クラッシュしたりはしないが、その内部動作を当てにしてはならない） */
    int count;
    for (count = 0; count < private->resources_size; count++) {  // 最大値
        int best = -1;
        for (index = 0; index < private->resources_size; index++) {
            if (private->resources[index].type != RES_UNUSED) {
                if (best < 0 || private->resources[index].alloc_order
                                > private->resources[best].alloc_order) {
                    best = index;
                }
            }
        }
        if (best >= 0) {
            free_resource(&private->resources[best] __DEBUG_ARGS);
            del_resource(&private->resources[best]);
        } else {
            break;  // すべて解放済み
        }
    }
    private->mark = 0;
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * convert_mem_flags:  RES_ALLOC_*フラグをMEM_ALLOC_*に変換する。
 *
 * 【引　数】res_flags: RES_ALLOC_*フラグ
 * 【戻り値】対応するMEM_ALLOC_*フラグ
 */
static uint32_t convert_mem_flags(const uint32_t res_flags)
{
    PRECOND((res_flags & 7) == 0);  // MEM_ALLOC_*が使用されていないことを確認
    return (res_flags & RES_ALLOC_TOP   ? MEM_ALLOC_TOP   : 0)
         | (res_flags & RES_ALLOC_TEMP  ? MEM_ALLOC_TEMP  : 0)
         | (res_flags & RES_ALLOC_CLEAR ? MEM_ALLOC_CLEAR : 0);
}

/*-----------------------------------------------------------------------*/

/**
 * find_package:  指定されたパス名に対応するパッケージファイル操作モジュール
 * を返す。
 *
 * 【引　数】path: パス名
 * 【戻り値】対応するパッケージファイル操作モジュールの情報構造体へのポインタ
 * 　　　　　（対応するモジュールがない場合はNULL）
 */
static PackageModuleInfo *find_package(const char *path)
{
    PRECOND_SOFT(path != NULL, return NULL);
    int i;
    for (i = 0; i < lenof(packages); i++) {
        PRECOND_SOFT(packages[i] != NULL, continue);
        if (packages[i]->available
         && strncmp(path, packages[i]->prefix, packages[i]->prefixlen) == 0
        ) {
            return packages[i];
        }
    }
    return NULL;
}

/*************************************************************************/

/**
 * find_resource:  リソースを検索する。
 *
 * 【引　数】 private: リソース管理構造体の内部データへのポインタ
 * 　　　　　data_ptr: リソースのユーザーデータポインタ
 * 【戻り値】リソースのResourceInfo構造体（見つからない場合はNULL）
 */
static ResourceInfo *find_resource(ResourceManagerPrivate *private,
                                   void **data_ptr)
{
    PRECOND_SOFT(private != NULL, return NULL);
    PRECOND_SOFT(private->resources != NULL, return NULL);
    PRECOND_SOFT(data_ptr != NULL, return NULL);

    int index;
    for (index = 0; index < private->resources_size; index++) {
        if (private->resources[index].type != RES_UNUSED
         && private->resources[index].data_ptr == data_ptr
        ) {
            return &private->resources[index];
        }
    }
    return NULL;
}

/*-----------------------------------------------------------------------*/

/**
 * add_resource:  リソース管理構造体にリソースを追加する。必要に応じて
 * resources[]配列を延長して再確保する。
 *
 * 【引　数】 private: リソース管理構造体の内部データへのポインタ
 * 　　　　　    type: リソースの種類
 * 　　　　　data_ptr: リソースのユーザーデータポインタ
 * 【戻り値】新しく作ったリソースのResourceInfo構造体（エラーの場合はNULL）
 */
static ResourceInfo *add_resource(ResourceManagerPrivate *private,
                                  ResourceType type, void **data_ptr
                                  __DEBUG_PARAMS)
{
    PRECOND_SOFT(private != NULL, return NULL);
    PRECOND_SOFT(private->resources != NULL, return NULL);
    PRECOND_SOFT(data_ptr != NULL, return NULL);

    int index;
    for (index = 0; index < private->resources_size; index++) {
        if (private->resources[index].type == RES_UNUSED) {
            break;
        }
    }
    if (UNLIKELY(index >= private->resources_size)) {
        /* 空きがないので、配列を伸ばす */
        const int new_num = private->resources_size + 100;  // 適当に
        DMSG("%p (%s): add %p: out of resource entries, reallocing to %d",
             private, private->owner, data_ptr, new_num);
        ResourceInfo *new_resources;
        if (private->resources_is_static) {
            new_resources = debug_mem_alloc(
                sizeof(ResourceInfo) * new_num, 0,
                MEM_ALLOC_TEMP | MEM_ALLOC_TOP | MEM_ALLOC_CLEAR, // 断片化回避
                file, line, MEM_INFO_MANAGE
            );
        } else {
            new_resources = debug_mem_realloc(
                private->resources, sizeof(ResourceInfo) * new_num,
                MEM_ALLOC_TEMP | MEM_ALLOC_TOP | MEM_ALLOC_CLEAR,
                file, line, MEM_INFO_MANAGE
            );
        }
        if (UNLIKELY(!new_resources)) {
            DMSG("... failed to realloc resource list!");
            return NULL;
        }
        /* link_nextフィールドのアドレスを更新する */
        int i;
        for (i = 0; i < private->resources_size; i++) {
            ResourceInfo *ptr = &new_resources[i];
            int tries = 10000;
            while (ptr->link_next != &private->resources[i]) {
                ptr = ptr->link_next;
                tries--;
                if (UNLIKELY(tries <= 0)) {
                    DMSG("BUG!! endless linked list on resource %p (%s)"
                         " in resmgr %p (%s)", &private->resources[i],
                         new_resources[i].debug_path, private, private->owner);
                    goto error_no_fixup;
                }
            }
            ptr->link_next = &new_resources[i];
          error_no_fixup:;
        }
        /* private構造体を更新する */
        private->resources = new_resources;
        index = private->resources_size;  // 念のため
        private->resources_size = new_num;
        private->resources_is_static = 0;
    }

    mem_clear(&private->resources[index], sizeof(private->resources[index]));
    private->resources[index].owner = private;
    private->resources[index].link_next = &private->resources[index];
    private->resources[index].data_ptr = data_ptr;
    private->resources[index].type = type;
    private->resources[index].alloc_order = private->alloc_order++;

    if (*data_ptr) {
        DMSG("WARNING: Data pointer %p is not NULL (%p)", data_ptr, *data_ptr);
    }

    return &private->resources[index];
}

/*-----------------------------------------------------------------------*/

/**
 * del_resource:  リソース管理構造体からリソースを削除する。
 *
 * 【引　数】resinfo: リソース情報構造体へのポインタ
 * 【戻り値】なし
 */
static void del_resource(ResourceInfo *resinfo)
{
    PRECOND_SOFT(resinfo != NULL, return);
    resinfo->type = RES_UNUSED;
}

/*-----------------------------------------------------------------------*/

/**
 * load_resource:  リソースのロードを開始する。
 *
 * 【引　数】resinfo: ロードするリソースの情報構造体へのポインタ
 * 　　　　　   path: パス名
 * 　　　　　  align: mem_alloc()に渡すメモリアライメント
 * 　　　　　  flags: mem_alloc()に渡すメモリ確保フラグ
 * 【戻り値】0以外＝ロード開始、0＝ロード失敗
 */
static int load_resource(ResourceInfo *resinfo, const char *path,
                         uint16_t align, uint32_t flags __DEBUG_PARAMS)
{
    PRECOND_SOFT(resinfo != NULL, return 0);
    PRECOND_SOFT(path != NULL, return 0);

    resinfo->load_info = mem_alloc(sizeof(*resinfo->load_info), sizeof(void *),
                                   MEM_ALLOC_TEMP | MEM_ALLOC_CLEAR);
    if (UNLIKELY(!resinfo->load_info)) {
        DMSG("%p (%s): Out of memory for load info", resinfo, path);
        return 0;
    }
    resinfo->load_info->mem_align = align;
    resinfo->load_info->mem_flags = flags;
#ifdef DEBUG
    resinfo->load_info->mem_type =
        (resinfo->type==RES_TEXTURE ? MEM_INFO_TEXTURE : -1);
#endif

    const int res = load_from_package(resinfo, path __DEBUG_ARGS);
    if (res > 0) {
        return 1;
    } else if (res == 0) {  // 不存在確定の場合は飛ばす
        if (load_from_file(resinfo, path __DEBUG_ARGS)) {
            return 1;
        }
    }
    DMSG("%s: Resource not found", path);
    mem_free(resinfo->load_info);
    resinfo->load_info = NULL;
    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * free_resource:  リソースのデータを解放する（リソースエントリーは解放しな
 * い）。リンクがある場合はリンクを解除するが、リソース自体は解放しない。
 *
 * リソースが読み込み中の場合、読み込みが完了するまで停止する。
 *
 * 【引　数】resinfo: 解放するリソースの情報構造体へのポインタ
 * 【戻り値】なし
 */
static void free_resource(ResourceInfo *resinfo __DEBUG_PARAMS)
{
    PRECOND_SOFT(resinfo != NULL, return);

    if (UNLIKELY(resinfo->link_next == NULL)) {

        DMSG("BUG!! resinfo->link_next == NULL for resource %p (%s)",
             resinfo, resinfo->debug_path);
        goto do_free;

    } else if (resinfo->link_next != resinfo) {

        ResourceInfo *prev = resinfo->link_next;
        int tries = 10000;
        while (prev->link_next != resinfo) {
            prev = prev->link_next;
            tries--;
            if (UNLIKELY(tries <= 0)) {
                DMSG("BUG!! endless linked list on resource %p (%s)",
                     resinfo, resinfo->debug_path);
                goto unlink_failed;
            }
        }
        prev->link_next = resinfo->link_next;
      unlink_failed:
        resinfo->link_next = resinfo;

    } else {  // リンクがないので解放する

      do_free:
        if (resinfo->load_info) {
            if (resinfo->load_info->read_request) {
                sys_file_wait_async(resinfo->load_info->read_request);
            }
            /* すぐに解放するのでfinish_load()を呼び出す必要はない */
            mem_free(resinfo->load_info->file_data);
            mem_free(resinfo->load_info);
            resinfo->load_info = NULL;
        } else {
            switch (resinfo->type) {
              case RES_UNUSED:
                break;
              case RES_UNKNOWN:
                DMSG("BUG: resource %p has type UNKNOWN!", resinfo->data_ptr);
                break;
              case RES_DATA:
                debug_mem_free(*(resinfo->data_ptr), file, line, -1);
                break;
              case RES_TEXTURE:
                texture_destroy(*(Texture **)(resinfo->data_ptr) __DEBUG_ARGS);
                break;
            }
        }

    }  // if (リンクの有無)

    *(resinfo->data_ptr) = NULL;
}

/*************************************************************************/

/**
 * load_from_package:  指定されたパス名がパッケージファイルのものである場合、
 * そのパッケージファイルからのロードを開始する。パッケージファイルのもので
 * ない場合、何もせずに失敗を返す。リソース情報構造体の各フィールドは初期化
 * されていなければならない（pkginfo, loadingフィールドを除く）。
 *
 * 【引　数】resinfo: ロードに使うリソース構造体
 * 　　　　　   path: パス名
 * 【戻り値】正数＝成功、負数＝リソースが確実に存在しない、0＝その他のエラー
 */
static int load_from_package(ResourceInfo *resinfo, const char *path
                             __DEBUG_PARAMS)
{
    PRECOND_SOFT(resinfo != NULL, return 0);
    PRECOND_SOFT(resinfo->data_ptr != NULL, return 0);
    PRECOND_SOFT(resinfo->load_info != NULL, return 0);
    PRECOND_SOFT(path != NULL, return 0);
    LoadInfo *load_info = resinfo->load_info;

    PackageModuleInfo *pkginfo = find_package(path);
    if (!pkginfo) {
        return 0;
    }
    path += pkginfo->prefixlen;

    SysFile *fp;
    uint32_t pos, len, size;
    int compressed;
    if (!(*pkginfo->file_info)(pkginfo, path, &fp, &pos, &len,
                               &compressed, &size)) {
        if (pkginfo->has_path && !(*pkginfo->has_path)(pkginfo, path)) {
            return 0;  // 通常のファイルとして存在するかもしれない
        }
        return -1;  // パッケージプレフィックスが一致するので、確実に存在しない
    }
    if (!compressed) {
        len = size;
    }

    void *data;
    if (compressed) {
        /* MEM_ALLOC_TOPを反転することでメモリ断片化を回避。また、一時メモリ
         * のため、デバッグ有効時は呼び出し元ではなくこのファイルのものとして
         * 登録する */
        data = mem_alloc(len, 0, load_info->mem_flags ^ MEM_ALLOC_TOP);
    } else {
        data = debug_mem_alloc(len, load_info->mem_align, load_info->mem_flags,
                               file, line, resinfo->load_info->mem_type);
    }
    if (UNLIKELY(!data)) {
        DMSG("%s: Out of memory", path);
        return -1;
    }

    load_info->read_request = sys_file_read_async(fp, data, len, pos);
    if (UNLIKELY(!load_info->read_request)) {
        DMSG("%s: Failed to read %u from %u in package file", path, len, pos);
        mem_free(data);
        return -1;
    }

    load_info->compressed      = compressed;
    load_info->compressed_size = len;
    load_info->data_size       = size;
    load_info->file_data       = data;
    load_info->fp              = fp;
    load_info->pkginfo         = pkginfo;
#ifdef DEBUG
    snprintf(resinfo->debug_path, sizeof(resinfo->debug_path), "%s", path);
#endif
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * load_from_file:  通常ファイルからリソースをロードする。
 *
 * 【引　数】resinfo: ロードに使うリソース構造体
 * 　　　　　   path: パス名
 * 【戻り値】0以外＝成功、0＝失敗
 */
static int load_from_file(ResourceInfo *resinfo, const char *path
                          __DEBUG_PARAMS)
{
    PRECOND_SOFT(resinfo != NULL, return 0);
    PRECOND_SOFT(resinfo->data_ptr != NULL, return 0);
    PRECOND_SOFT(resinfo->load_info != NULL, return 0);
    PRECOND_SOFT(path != NULL, return 0);
    LoadInfo *load_info = resinfo->load_info;

    SysFile *fp = sys_file_open(path);
    if (!fp) {
        if (sys_last_error() != SYSERR_FILE_NOT_FOUND) {
            DMSG("[%s] open(): %s", path,  sys_last_errstr());
        }
        return 0;
    }

    const uint32_t filesize = sys_file_size(fp);
    void *data = debug_mem_alloc(filesize ? filesize : 1, load_info->mem_align,
                                 load_info->mem_flags, file, line,
                                 resinfo->load_info->mem_type);
    if (UNLIKELY(!data)) {
        DMSG("[%s] Out of memory for filebuf (%d bytes)", path, filesize);
        sys_file_close(fp);
        return 0;
    }
    load_info->read_request = sys_file_read_async(fp, data, filesize, 0);
    if (UNLIKELY(!load_info->read_request)) {
        DMSG("%s: Failed to read %u bytes (async)", path, filesize);
        debug_mem_free(data, file, line, resinfo->load_info->mem_type);
        mem_free(data);
        sys_file_close(fp);
        return 0;
    }

    load_info->file_data  = data;
    load_info->fp         = fp;
    load_info->data_size  = filesize;
    load_info->need_close = 1;
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * finish_load:  読み込みが完了したリソースの最終処理（圧縮解除等）を行う。
 * 読み込みが完了・成功したことが前提。
 *
 * 【引　数】resinfo: ロードに使うリソース構造体
 * 【戻り値】なし
 */
static void finish_load(ResourceInfo *resinfo __DEBUG_PARAMS)
{
    PRECOND_SOFT(resinfo != NULL, return);
    PRECOND_SOFT(resinfo->data_ptr != NULL, return);
    PRECOND_SOFT(resinfo->load_info != NULL, return);
    LoadInfo *load_info = resinfo->load_info;

    /* 必要に応じてファイルをクローズする */
    if (load_info->fp && load_info->need_close) {
        sys_file_close(load_info->fp);
        load_info->fp = NULL;
        load_info->need_close = 0;
    }

    /* 圧縮を解凍する */
    if (load_info->compressed && load_info->pkginfo) {
        void *newdata = debug_mem_alloc(
            load_info->data_size, load_info->mem_align,
            load_info->mem_flags, file, line, resinfo->load_info->mem_type
        );
        if (UNLIKELY(!newdata)) {
            DMSG("%s: Out of memory for final buffer", resinfo->debug_path);
            mem_free(load_info->file_data);
            goto free_and_return;
        }
        if (UNLIKELY(!(*load_info->pkginfo->decompress)(
            load_info->pkginfo,
            load_info->file_data, load_info->compressed_size,
            newdata, load_info->data_size
        ))) {
            DMSG("%s: Decompression failed", resinfo->debug_path);
            mem_free(newdata);
            mem_free(load_info->file_data);
            goto free_and_return;
        }
        mem_free(load_info->file_data);
        load_info->file_data = newdata;
    }

    /* サイズポインタがあれば、データサイズを格納する */
    if (resinfo->size_ptr) {
        *resinfo->size_ptr = load_info->data_size;
    }

    /* 必要なデータ変換を行い、ユーザポインタを設定する */
    if (resinfo->type == RES_TEXTURE) {
        *resinfo->data_ptr = texture_parse(
            load_info->file_data, load_info->data_size, load_info->mem_flags, 1
            __DEBUG_ARGS
        );
        if (UNLIKELY(!*resinfo->data_ptr)) {
            DMSG("%s: Texture parse failed", resinfo->debug_path);
            goto free_and_return;
        }
    } else {
        *resinfo->data_ptr = load_info->file_data;
    }

  free_and_return:
    mem_free(resinfo->load_info);
    resinfo->load_info = NULL;
}

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
