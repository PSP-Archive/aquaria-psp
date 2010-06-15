/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/memory.h: Memory management header.
 */

#ifndef MEMORY_H
#define MEMORY_H

/*
 * システム依存メモリ管理関数をプログラム本体から分離するためと、malloc()の
 * 性能の限界（複数のメモリプールを利用できない、アライメントを指定できない
 * など）を回避するため、そしてデバッグ時のメモリ利用監視やリーク発見を容易
 * にするという3つの目的で、独自にメモリ管理を行う。
 *
 * プログラムの実行開始時に、mem_init()を呼び出してメモリ管理機能を初期化す
 * る必要がある。その後、mem_alloc()やmem_realloc()、mem_free()を、POSIXの
 * malloc()・realloc()・free()同様に利用する。利用可能メモリ量と空き量、最
 * 大確保可能量を返す関数もあり、更にデバッグ有効時はメモリの確保状況を出力
 * したり画面に表示する機能もある。
 *
 * メモリ確保は大きく分けて、「通常確保」と「一時確保」の二種類に分類される。
 * 長時間メモリに滞在する前者に対して、データの変換や操作のために確保され、
 * すぐに解放される一時バッファが混在することが多々ある。両方とも同じメモリ
 * プールから確保するとメモリの断片化に繋がり、最悪の場合、空きメモリ量が充
 * 分あるのにデータをロードできない事態も想定される。この問題を回避するため、
 * この二種類の確保に応じて二つの独立したメモリプールが用意されている。プー
 * ルの選択はメモリ確保時に、確保フラグMEM_ALLOC_TEMPを指定することで行う。
 * なお、一時確保するときに一時プールが満杯だったり、実行環境の制約等で一時
 * プールを用意できなかった場合は通常プールの後方（以下参照）から確保するが、
 * 通常確保を一時プールから確保することはなく、通常プールに空きメモリ量が足
 * りない場合は確保失敗となる。
 *
 * もう一つのメモリ断片化回避策として、メモリ確保時に確保位置を選択できる。
 * 通常はプールの前方からメモリを確保していくが、MEM_ALLOC_TOPを指定すると、
 * プールの後方からメモリを確保する。同じ通常確保でも確保や解放の時期が異な
 * る場合、前方・後方確保を使い分けることでメモリの断片化を回避できる。
 *
 * 確保されたメモリのデフォルトアライメントはmalloc()と同様、どのデータ型に
 * も利用可能なものになっている。通常、変更する必要はないが、ハードウェアの
 * 制約などにより特定のアライメントが必要な場合、バイト単位で指定できる（但
 * し2のべき乗のみ）。なお一般的な使い方において、仮に64バイトアライメント
 * に統一したとしても、無駄になるメモリ量は使用量の0.1%未満なので、データ型
 * によって細かく指定する必要がない。
 * 　※上記はこのエンジンに合わせて開発したプログラムの場合。C++等、頻繁に
 * 　　少量のメモリを確保・解放する言語では効率は悪くなる。
 *
 * mem_realloc()による再確保（サイズ調整）の場合、確保時のアライメントを引
 * き継ぎながら、可能であればブロックを縮小・拡大する（MEM_ALLOC_TOPで確保
 * されて縮小する場合、データを後方に移動する）。ただしMEM_ALLOC_TOP、
 * MEM_ALLOC_TEMPの指定が確保時と異なる場合、新しいフラグに応じて新しい領域
 * を確保して古いものを解放する。
 */

/*************************************************************************/

/***** メモリ初期化・情報関数 *****/

/**
 * mem_init:  メモリ管理機能を初期化する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int mem_init(void);

/**
 * mem_total:  合計メモリ量を返す。
 *
 * 【引　数】flags: 0 (通常メモリ) またはMEM_ALLOC_TEMP (一時メモリ)
 * 【戻り値】合計メモリ量（バイト）
 */
extern uint32_t mem_total(uint32_t flags);

/**
 * mem_avail:  現在の空きメモリ量を返す。
 *
 * 【引　数】flags: 0 (通常メモリ) またはMEM_ALLOC_TEMP (一時メモリ)
 * 【戻り値】空きメモリ量（バイト）
 */
extern uint32_t mem_avail(uint32_t flags);

/**
 * mem_contig:  現在、確保し得る最大メモリ量を返す。
 *
 * 【引　数】flags: 0 (通常メモリ) またはMEM_ALLOC_TEMP (一時メモリ)
 * 【戻り値】確保し得る最大メモリ量（バイト）
 */
extern uint32_t mem_contig(uint32_t flags);

/*************************************************************************/

/***** メモリ確保・解放関数 *****/

/*
 * デバッグ時は、メモリ確保・解放の呼び出し位置を関数に渡す。また、画像など
 * 他のメモリ確保関数が呼び出し位置とメモリ種類を渡せるように関数名も変更。
 * 使用例：
 *
 * struct hogestruct *hoge_alloc(int size, const char *srcfile, int srcline)
 * {
 *     struct hogestruct *ptr;
 *     size += sizeof(*ptr);
 *     ptr = debug_mem_alloc(size, 0, 0, srcfile, srcline, MEM_INFO_HOGE);
 *     // ...
 *     return ptr;
 * }
 */

#ifdef DEBUG

# define __MEM_ALLOC_PARAMS , const char *file, int line, int type
# define DEBUG_MEM(symbol) debug_##symbol
# define mem_alloc(size,align,flags) \
    debug_mem_alloc((size), (align), (flags), __FILE__, __LINE__, -1)
# define mem_realloc(ptr,size,flags) \
    debug_mem_realloc((ptr), (size), (flags), __FILE__, __LINE__, -1)
# define mem_free(ptr) \
    debug_mem_free((ptr), __FILE__, __LINE__, -1)
# define mem_strdup(str,flags) \
    debug_mem_strdup((str), (flags), __FILE__, __LINE__, -1)

enum {
    MEM_INFO_FREE    =  0,  // 全空き
    MEM_INFO_FILE    =  1,  // データファイル関連
    MEM_INFO_FONT    =  2,  // フォント関連
    MEM_INFO_TEXTURE =  3,  // 画像関連
    MEM_INFO_MUSIC   =  4,  // 音楽関連
    MEM_INFO_SOUND   =  5,  // 音楽以外の音声関連
    MEM_INFO_TEXT    =  6,  // テキストデータ
    MEM_INFO_MANAGE  = 14,  // リソース等の管理データ
    MEM_INFO_UNKNOWN = 15   // 不明または混在
};

#else  // !DEBUG

# define __MEM_ALLOC_PARAMS
# define DEBUG_MEM(symbol) symbol
# define debug_mem_alloc(size,align,flags,file,line,type) \
    mem_alloc((size), (align), (flags))
# define debug_mem_realloc(ptr,size,flags,file,line,type) \
    mem_realloc((ptr), (size), (flags))
# define debug_mem_free(ptr,file,line,type)          mem_free((ptr))
# define debug_mem_strdup(str,flags,file,line,type)  mem_strdup((str), (flags))

#endif


/* メモリ確保関連フラグ等 */

#define MEM_ALLOC_CLEAR (1<<0)  // メモリ確保と同時にゼロクリア
#define MEM_ALLOC_TOP   (1<<1)  // メモリプールの末尾から確保する
#define MEM_ALLOC_TEMP  (1<<2)  // 一時バッファ等、すぐに解放されるもの

/**
 * mem_alloc:  メモリ領域を確保する。
 *
 * 【引　数】 size: 確保するメモリ量（バイト）
 * 　　　　　align: 確保するメモリのアライメント（バイト）。0は16と同義
 * 　　　　　flags: 動作フラグ（MEM_ALLOC_*）
 * 【戻り値】確保されたメモリへのポインタ（エラーの場合はNULL）
 */
extern void *DEBUG_MEM(mem_alloc)(uint32_t size, uint16_t align, uint32_t flags
                                  __MEM_ALLOC_PARAMS);

/**
 * mem_realloc:  メモリ領域を拡大・縮小する。拡大・縮小が失敗した場合、元の
 * メモリ領域はそのまま残される（解放されない）。データのアライメントは維持
 * される。
 *
 * flagsにおいて、MEM_ALLOC_TEMP・MEM_ALLOC_TOPの有無が確保時と違う場合、
 * mem_alloc()→memcpy()→mem_free()と同様な動作となり、データコピーが必ず
 * 発生する。
 *
 * ptr==NULLの場合、Cライブラリのrealloc()と同様に新たにメモリを確保する。
 * mem_realloc(NULL,size,flags)はmem_alloc(size,0,flags)と同義で、アライメ
 * ントはデフォルトの16バイトとなる。
 *
 * size==0で呼び出すと、メモリを解放する（mem_free()同様）。
 *
 * 【引　数】  ptr: 現在確保されているメモリポインタ（NULLで新規確保）
 * 　　　　　 size: 確保するメモリ量（バイト、0の場合はメモリを解放）
 * 　　　　　flags: 動作フラグ（MEM_ALLOC_*）
 * 【戻り値】確保されたメモリへのポインタ（エラーまたは解放の場合はNULL）
 */
extern void *DEBUG_MEM(mem_realloc)(void *ptr, uint32_t size, uint32_t flags
                                    __MEM_ALLOC_PARAMS);

/**
 * mem_free:  メモリ領域を開放する。ptr==NULLの場合、何もしない。
 *
 * 【引　数】ptr: 解放するメモリ領域
 * 【戻り値】なし
 */
extern void DEBUG_MEM(mem_free)(void *ptr __MEM_ALLOC_PARAMS);

/**
 * mem_strdup:  文字列を複製する。
 *
 * 【引　数】  str: 複製する文字列
 * 　　　　　flags: 動作フラグ（MEM_ALLOC_*）
 * 【戻り値】文字列の複製（エラーの場合はNULL）
 */
extern char *DEBUG_MEM(mem_strdup)(const char *str, uint32_t flags
                                   __MEM_ALLOC_PARAMS);

/*************************************************************************/

/***** デバッグ宣言・関数 *****/

#ifdef DEBUG

/* メモリ状況表示フラグ */
extern int debug_memory_display_flag;

/**
 * mem_report_allocs:  確保されているメモリについて報告する。デバッグ時のみ
 * 実装。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void mem_report_allocs(void);

/**
 * mem_display_debuginfo:  Ctrl+Mを監視し、押された時にメモリ使用状況を表示
 * する。デバッグ時のみ実装。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void mem_display_debuginfo(void);

#endif  // DEBUG

/*************************************************************************/

#endif  // MEMORY_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
