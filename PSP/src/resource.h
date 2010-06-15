/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/resource.h: Resource management header.
 */

#ifndef RESOURCE_H
#define RESOURCE_H

/*
 * ゲームで使われる画像や音声、マップデータなどのリソースを、リソース管理構
 * 造体ResourceManagerで一元管理する。リソースの非同期読み込みを可能にする
 * ほか、一括解放もできるため各々解放したり、ポインタをリセットする手間が省
 * ける。
 *
 * ResourceManager構造体は、基本的に変数として定義する（mem_alloc()で確保す
 * ることも可能だが、後にmem_free()で解放しなければならず、リソース管理簡素
 * 化という考え方にはそぐわない）。例えば
 * 　　ResourceManager resmgr;
 * で定義した場合、resource_create(&resmgr)でまず初期化する（注：static定義
 * でない場合、resource_create()前に構造体をゼロクリアしなければならない）。
 *
 * リソースを読み込むには、リソースの種類（一般データ、画像など）に応じた
 * resource_load_*()関数を呼び出して読み込みを開始する。読み込みが完了した
 * ら、リソースデータへのポインタ（種類によってはTextureやSound構造体へのポ
 * インタ）は、load()関数に渡された「リソースポインタ」（データポインタを保
 * 持する変数や配列要素へのポインタ）に格納される。なお、リソース用のメモリ
 * はresource_load_*()呼び出し時に確保される。
 *
 * 読み込みは背景で行われるため、resource_load_*()が成功したからといって、
 * すぐにデータが利用できるわけではない。ロードされたリソースを利用する前に
 * 読み込み処理と同期を取って、読み込みが完了していることを確認しなければな
 * らない。この同期取りはresource_mark()及びresource_sync()関数で行われる。
 * resource_mark()を呼び出すと、それまでにロードを開始されたリソースに関連
 * 付けられた同期マーク値が返される。このマーク値をresource_sync()に渡すこ
 * とで、当該リソースの読み込みがすべて完了しているかどうか調べられる。
 *
 * リソースはロードのほか、新規作成によって登録することもできる。この場合、
 * 作成するリソースの種類に応じたresource_new_*()関数を呼び出し、ロードと同
 * 様にリソースポインタを渡す。新規作成はロードと違って、関数が成功すればす
 * ぐにリソースを使える（resource_mark()やresource_sync()を呼び出す必要はな
 * い）。
 *
 * リソースが不要になった場合、登録時と同じリソースポインタをresource_free()
 * に渡すことで解放できるほか、resource_free_all()で全てのリソースを一括解
 * 放することもできる。resource_free_all()を呼び出す場合、リソースは確保順
 * とは逆に解放されるので、例えばリソースポインタの配列バッファを確保して、
 * その配列の別のリソースを格納した場合はリソース→配列バッファの順に解放さ
 * れる（解放されたメモリにアクセスする心配はない）。
 *
 * 管理構造体自体が不要になった場合、resource_delete()を呼び出すことで
 * リソースと内部で使われる管理データをすべて破棄できる。
 *
 * すでに読み込みまたは新規作成されたリソースを別の管理構造体で使いたい場合
 * （例えば、一つのモジュールでロードされたリソースを別の独立したモジュール
 * で使いたい場合）、resource_link()を呼び出すことによってリソースへのリン
 * クを作成することができる。このリンクとは、Unix系ファイルシステムにおける
 * ハードリンクなどと同じ概念で、同一リソースデータを複数のリソースポインタ
 * が指すというもので、メモリ使用量が増えない（かつメモリコピーも発生しない）
 * のと、そして一方が解放しても他方で引き続き利用できるという利点がある。そ
 * の一方で、本来はresource_free_all()で解放されるはずのリソースがリンクに
 * よって解放されずに残るとメモリ断片化の原因になるので、リンクを利用する場
 * 合はリソースのロード手順等を考慮する必要がある。
 *
 * リソース管理機能自体の初期化・後片付けはresource_init()とresource_free()
 * で行う。初期化ではパッケージファイルから読み込みを行うため、data_init()
 * 成功後に呼び出さなければならない。
 */

/*************************************************************************/
/************************ データ型・関連定数定義 *************************/
/*************************************************************************/

/* リソース管理構造体のデータ型 */
// typedef struct ResourceManager_ ResourceManager;  // common.hへ
struct ResourceManager_ {
    void *static_buffer;    // リソース情報を保管する静的バッファ。手動設定禁止
    uint32_t static_size;   // static_bufferのサイズ（バイト）。手動設定禁止
    void *private;          // 内部は閲覧禁止
};

/*
 * 動的なメモリ確保を回避するため、一定数のリソースを管理するための静的デー
 * タバッファを予め設定することができる。resource_create()を呼び出さなければ
 * ならないのは変わらないが、指定されたリソース数がバッファに収まれば、別途
 * メモリを確保せずに静的バッファを使う。
 *
 * このような静的バッファ付きリソース管理構造体を定義する場合、以下のように
 * 記述する。
 *     DEFINE_STATIC_RESOURCEMANAGER(resmgr, num);
 * 「resmgr」に定義する構造体名（変数名）、「num」には静的バッファに情報を
 * 格納できる最大リソース数を指定する。なお、定義されたリソース管理構造体に
 * static属性が付加される。
 */
#define DEFINE_STATIC_RESOURCEMANAGER(resmgr,num) \
static uintptr_t resmgr##_static_buffer[(RESOURCE_SIZE1 + RESOURCE_SIZE2*(num) + sizeof(uintptr_t)-1) / sizeof(uintptr_t)]; \
static ResourceManager resmgr = \
    {.static_buffer = resmgr##_static_buffer, \
     .static_size = sizeof(resmgr##_static_buffer), .private = NULL}

#ifdef DEBUG
# define RESOURCE_SIZE1  (sizeof(void *) + align_up(112,sizeof(void *)))
# define RESOURCE_SIZE2  ((sizeof(void *) * 4) + align_up(12,sizeof(void *)) \
                          + sizeof(void *) + align_up(100,sizeof(void *)))
#else
# define RESOURCE_SIZE1  (sizeof(void *) + align_up(12,sizeof(void *)))
# define RESOURCE_SIZE2  ((sizeof(void *) * 4) + align_up(12,sizeof(void *)) \
                          + sizeof(void *))
#endif

/*************************************************************************/

/* 確保フラグ（注：MEM_*フラグと同様ではあるが、resource_*()関数では必ず
 * こちらのフラグを使うこと） */

/* MEM_*フラグ検知のため、1<<0〜2は使わない */
#define RES_ALLOC_TOP    (1<<3)  // メモリプールの後方から確保する
#define RES_ALLOC_TEMP   (1<<4)  // 一時プールから確保する
#define RES_ALLOC_CLEAR  (1<<5)  // 確保時クリアする（resource_new_data()のみ）

/*************************************************************************/
/**************************** インタフェース *****************************/
/*************************************************************************/

/**************************** 初期化・片付け *****************************/

/**
 * resource_init:  リソース管理機能を初期化する。失敗することはない。
 * パッケージファイル操作モジュールの初期化を行うため、ファイルアクセスに
 * よって時間がかかることがある。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void resource_init(void);

/**
 * resource_cleanup:  リソース管理機能の後片付けを行い、パッケージファイルを
 * クローズする。ただし、使用中のリソース管理構造体の破棄はしない。リソース
 * 読み込む中に呼び出した場合、読み込み終了まで停止したり、メモリリークする
 * 可能性がある。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void resource_cleanup(void);

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
extern int resource_create(ResourceManager *resmgr, int num_resources
#ifdef DEBUG
                           , const char *file, int line
#endif
);

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
extern void resource_delete(ResourceManager *resmgr
#ifdef DEBUG
                            , const char *file, int line
#endif
);

/***************************** 情報取得関数 ******************************/

/**
 * resource_exists:  リソースの存在の有無を確認する。resource_load_*()で
 * 実際にデータを読み込んで確認するよりは早い。
 *
 * 【引　数】path: リソースファイルのパス名
 * 【戻り値】0以外＝リソースが存在する、0＝リソースが存在しない
 */
extern int resource_exists(const char *path);

/**
 * resource_list_files_start():  パッケージ内の全ファイルをリストアップする
 * ための初期化を行う。同時に複数のパッケージからファイルリストを取得する
 * ことはできない。
 *
 * 【引　数】path: ファイルリストを取得するパッケージのプレフィックスパス名
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int resource_list_files_start(const char *path);

/**
 * resource_list_files_next():  次のデータファイルのパス名を返す。
 * resource_list_files_start()を先に呼び出さなければならない。
 *
 * 【引　数】なし
 * 【戻り値】パス名（NULL＝リスト完了）
 */
extern const char *resource_list_files_next(void);

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
extern SysFile *resource_open_as_file(const char *path, uint32_t *offset_ret,
                                      uint32_t *filesize_ret);

/*************************** データロード関連 ****************************/

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
extern int resource_load_data(ResourceManager *resmgr, void **data_ptr,
                              uint32_t *size_ptr, const char *path,
                              uint16_t align, uint32_t flags
#ifdef DEBUG
                              , const char *file, int line
#endif
);

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
extern int resource_load_texture(ResourceManager *resmgr, Texture **texture_ptr,
                                 const char *path, uint32_t flags
#ifdef DEBUG
                                 , const char *file, int line
#endif
);

/**
 * resource_mark:  同期を取るためのマークを登録する。失敗することはないが、
 * 同期を取らずに連続1000回以上呼び出した場合の動作は不定。
 *
 * 【引　数】resmgr: リソース管理構造体へのポインタ
 * 【戻り値】マーク値（必ず0以外）
 */
extern int resource_mark(ResourceManager *resmgr);

/**
 * resource_sync:  resource_mark()で登録したマークとの同期状況を返す。指定
 * したマーク以前にresource_load()を呼び出されたすべてのリソースがロードさ
 * れていれば、0以外の値（同期済み）を返す。
 *
 * 【引　数】resmgr: リソース管理構造体へのポインタ
 * 　　　　　  mark: マーク値
 * 【戻り値】0以外＝同期済み、0＝ロード未完了リソース有り
 */
extern int resource_sync(ResourceManager *resmgr, int mark
#ifdef DEBUG
                         , const char *file, int line
#endif
);

/**
 * resource_wait:  resource_mark()で登録したマークとの同期が取れるまで待つ。
 * 関数から戻った時、指定したマーク以前にresource_load()を呼び出されたすべて
 * のリソースがロードされている。
 *
 * 【引　数】resmgr: リソース管理構造体へのポインタ
 * 　　　　　  mark: マーク値
 * 【戻り値】なし
 */
extern void resource_wait(ResourceManager *resmgr, int mark
#ifdef DEBUG
                          , const char *file, int line
#endif
);

/******************************* 新規作成 ********************************/

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
extern int resource_new_data(ResourceManager *resmgr, void **data_ptr,
                             uint32_t size, uint32_t align, uint32_t flags
#ifdef DEBUG
                             , const char *file, int line
#endif
);

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
                           const char *str, uint32_t flags
#ifdef DEBUG
                             , const char *file, int line
#endif
);

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
extern int resource_new_texture(ResourceManager *resmgr, Texture **texture_ptr,
                                int width, int height, uint32_t flags
#ifdef DEBUG
                                , const char *file, int line
#endif
);

/********************** 既存リソースを管理下に置く ***********************/

/**
 * resource_take_data:  管理されていない一般データリソースを管理対象に加える。
 *
 * 【引　数】  resmgr: リソース管理構造体へのポインタ
 * 　　　　　data_ptr: リソースポインタ（データポインタが格納されている変数
 * 　　　　　          　へのポインタ）
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int resource_take_data(ResourceManager *resmgr, void **data_ptr
#ifdef DEBUG
                              , const char *file, int line
#endif
                              );

/***************************** その他の操作 ******************************/

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
extern int resource_link(ResourceManager *resmgr, ResourceManager *old_resmgr,
                         void **old_ptr, void **new_ptr
#ifdef DEBUG
                         , const char *file, int line
#endif
);

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
extern void resource_free(ResourceManager *resmgr, void **data_ptr
#ifdef DEBUG
                          , const char *file, int line
#endif
);

/**
 * resource_free_all:  リソース管理構造体で管理されている全てのリソースを
 * 一括破棄する。
 *
 * 読み込み中のリソースがある場合、その読み込みが完了するまで停止する。
 *
 * 【引　数】resmgr: リソース管理構造体へのポインタ
 * 【戻り値】なし
 */
extern void resource_free_all(ResourceManager *resmgr
#ifdef DEBUG
                              , const char *file, int line
#endif
);

/*************************************************************************/

/*
 * デバッグ有効時、メモリ確保時に呼び出し元のソースファイル・行目が記録され
 * るが、リソースメモリの確保元をすべてresource.cにしていては意味がないので、
 * これらのマクロで一つ上の呼び出し元をメモリ管理関数に渡す。
 */

#ifdef DEBUG
# define resource_create(resmgr,num_resources) \
    resource_create((resmgr), (num_resources), __FILE__, __LINE__)
# define resource_delete(resmgr) \
    resource_delete((resmgr), __FILE__, __LINE__)
# define resource_load_data(resmgr,data_ptr,size_ptr,path,align,flags) \
    resource_load_data((resmgr), (data_ptr), (size_ptr), (path), \
                       (align), (flags), __FILE__, __LINE__)
# define resource_load_texture(resmgr,texture_ptr,path,flags) \
    resource_load_texture((resmgr), (texture_ptr), (path), (flags), \
                          __FILE__, __LINE__)
# define resource_load_sound(resmgr,sound_ptr,path,flags) \
    resource_load_sound((resmgr), (sound_ptr), (path), (flags), \
                        __FILE__, __LINE__)
# define resource_sync(resmgr,mark) \
    resource_sync((resmgr), (mark), __FILE__, __LINE__)
# define resource_wait(resmgr,mark) \
    resource_wait((resmgr), (mark), __FILE__, __LINE__)
# define resource_new_data(resmgr,data_ptr,size,align,flags) \
    resource_new_data((resmgr), (data_ptr), (size), (align), (flags), \
                      __FILE__, __LINE__)
# define resource_strdup(resmgr,data_ptr,str,flags) \
    resource_strdup((resmgr), (data_ptr), (str), (flags), __FILE__, __LINE__)
# define resource_new_texture(resmgr,texture_ptr,width,height,flags) \
    resource_new_texture((resmgr), (texture_ptr), (width), (height), (flags), \
                         __FILE__, __LINE__)
# define resource_take_data(resmgr,data_ptr) \
    resource_take_data((resmgr), (data_ptr), __FILE__, __LINE__)
# define resource_link(resmgr,old_resmgr,old_ptr,new_ptr) \
    resource_link((resmgr), (old_resmgr), (old_ptr), (new_ptr), \
                  __FILE__, __LINE__)
# define resource_free(resmgr,data_ptr) \
    resource_free((resmgr), (data_ptr), __FILE__, __LINE__)
# define resource_free_all(resmgr) \
    resource_free_all((resmgr), __FILE__, __LINE__)
#endif

/*************************************************************************/

#endif  // RESOURCE_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
