/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/psplocal.h: Common header for declarations local to
 * PSP-specific code.
 */

#ifndef SYSDEP_PSP_PSPLOCAL_H
#define SYSDEP_PSP_PSPLOCAL_H

/*************************************************************************/
/************************* PSP専用構成オプション *************************/
/*************************************************************************/

/*
 * SUPPORT_FIRMWARE_BEFORE_2_71
 *
 * 定義すると、2.71以前のファームウェアをサポートする。変化があるのはMPEG再
 * 生モジュールをロードするsetup.cのload_av_modules()のみで、2.71から使用で
 * きるsceUtilityLoadAvModule()に代わって直接モジュールをロードする機能を有
 * 効にする。（ただ、カスタムFWでなければロード自体が許可されないので意味が
 * ない。）
 */

#define SUPPORT_FIRMWARE_BEFORE_2_71


/*
 * NO_RAW_MEMSTICK_ACCESS
 *
 * 定義すると、sceIo*()関数によるメモリスティックへのアクセスを一切行わない
 * （ゲームデータのインストール先がメモリスティックである場合を除く）。セー
 * ブファイルのスキャンが10倍ほど遅くなるほか、セーブデータのインポートも出
 * 来なくなる。
 */

// #define NO_RAW_MEMSTICK_ACCESS

/*************************************************************************/
/*************************** PSPシステムヘッダ ***************************/
/*************************************************************************/

#include <pspuser.h>
#include <pspaudio.h>
#include <pspaudiocodec.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspmpeg.h>
#include <psppower.h>
#include <psputility.h>

/*************************************************************************/
/************************** 各種共通定数・変数 ***************************/
/*************************************************************************/

/* PSP画面のサイズとライン長（ピクセル単位） */
#define DISPLAY_WIDTH   480
#define DISPLAY_HEIGHT  272
#define DISPLAY_STRIDE  512

/* 音声の再生レート（Hz） */
#define SOUND_RATE  44100

/* スレッド優先度（小さい値ほど高優先度） */
#define THREADPRI_MAIN           32  // メインスレッド
#define THREADPRI_UTILITY_BASE   26  // OSのセーブデータユーティリティ関連
#define THREADPRI_FILEIO         25  // ファイル操作
#define THREADPRI_SOUND          20  // 音声再生とストリーミング
#define THREADPRI_CALLBACK_WATCH 15  // HOMEボタン・電源コールバック処理

/* 直近のシステムコールのエラー値（主にsys_files_*()で使用） */
extern int psp_errno;

/* 休止（サスペンド）フラグ。0以外になっていると、sys_time_delay()で0になる
 * まで永遠に待つ。 */
extern volatile int psp_suspend;

/* 休止準備完了フラグ。休止イベント発生時、休止の準備が整った段階で0以外に
 * 設定される。 */
extern volatile int psp_suspend_ok;

/* 終了フラグ。0以外になった場合、各スレッドは速やかに終了しなければなら
 * ない。 */
extern volatile int psp_exit;

/* 設定ファイル用ICON0.PNGのファイルデータ */
extern const uint32_t icon0_png_size;
extern const uint8_t icon0_png[];

/*************************************************************************/
/******************** PSPライブラリ内部変数・関数宣言 ********************/
/*************************************************************************/

/******** display.c ********/

/**
 * psp_display_init:  描画機能の初期化を行う。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int psp_display_init(void);

/**
 * psp_draw_buffer, psp_depth_buffer:  現在の描画バッファまたは奥行きバッファ
 * へのポインタを返す。バッファは480x272、ライン長512ピクセルで固定。
 *
 * 【引　数】なし
 * 【戻り値】現在の描画バッファまたは奥行きバッファへのポインタ
 */
extern PURE_FUNCTION uint32_t *psp_draw_buffer(void);
extern PURE_FUNCTION uint16_t *psp_depth_buffer(void);

/**
 * psp_vram_spare_ptr, psp_vram_spare_size:  描画・奥行きバッファに使われて
 * いないVRAMへのポインタ、またはそのサイズを返す。
 *
 * 【引　数】なし
 * 【戻り値】空きVRAMへのポインタ、または空きVRAMのサイズ（バイト）
 */
extern PURE_FUNCTION void *psp_vram_spare_ptr(void);
extern PURE_FUNCTION uint32_t psp_vram_spare_size(void);

/**
 * psp_work_pixel_address:  指定された画面座標にあるピクセルの、描画バッファ
 * 内のメモリアドレスを返す。
 *
 * 【引　数】x, y: 画面座標（ピクセル）
 * 【戻り値】指定されたピクセルのアドレス
 * 【注　意】座標の範囲チェックは行われない。
 */
extern PURE_FUNCTION uint32_t *psp_work_pixel_address(int x, int y);

/**
 * psp_restore_clip_area:  sys_display_clip()で設定されたクリップ範囲を
 * 改めて適用する。ge_unset_clip_area()で一時的に解除した場合、再設定する
 * ために呼び出す。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void psp_restore_clip_area(void);

/**
 * convert_ARGB32:  汎用色値をPSP用ABGR値に変換する。
 *
 * 【引　数】color: 変換する汎用ARGB値
 * 【戻り値】PSP用ABGR値
 */
static inline CONST_FUNCTION uint32_t convert_ARGB32(const uint32_t color)
{
    const uint32_t A = color>>24 & 0xFF;
    const uint32_t R = color>>16 & 0xFF;
    const uint32_t G = color>> 8 & 0xFF;
    const uint32_t B = color>> 0 & 0xFF;
    return A<<24 | B<<16 | G<<8 | R<<0;
}


/******** files.c ********/

/**
 * psp_file_init:  ファイルアクセス機能の初期化を行う。
 *
 * 【引　数】basepath: データファイルの基準パス名
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int psp_file_init(const char *basepath_);

/**
 * psp_file_open_async:  指定されたファイルパス名を基に、ファイルを読み込み
 * モードで開く。パス名処理はsys_file_open()同様。操作は非同期で行われ、返
 * された要求識別子はsys_file_peek_async()、sys_file_wait_async()で使うもの
 * で、sys_file_wait_async()の結果は0以外（成功）または0（失敗）となる。
 *
 * なお、本関数が成功した後でオープンが失敗する場合、ファイルポインタは自動
 * 的にクローズされ、その後使用することができない。
 *
 * 【引　数】  file: ファイルパス名
 * 　　　　　fp_ret: 成功時、SysFileポインタを格納する変数へのポインタ
 * 【戻り値】成功の場合は読み込み要求識別子（0以外）、失敗の場合は0
 */
extern int psp_file_open_async(const char *file, SysFile **fp_ret);

/**
 * psp_file_read_async_timed:  ファイルからデータの開始期限付き非同期読み
 * 込みを開始する。
 *
 * 【引　数】        fp: ファイルポインタ
 * 　　　　　       buf: データを読み込むバッファ
 * 　　　　　       len: 読み込むデータ量（バイト）
 * 　　　　　   filepos: 読み込み開始位置（バイト）
 * 　　　　　time_limit: 読み込み開始期限（μ秒、呼び出し時点から）
 * 【戻り値】成功の場合は読み込み要求識別子（0以外）、失敗の場合は0
 */
extern int psp_file_read_async_timed(SysFile *fp, void *buf, int32_t len,
                                     int32_t filepos, int32_t time_limit);

/**
 * psp_file_pause:  実行中の全ての非同期読み込みが終了するのを待って、ファ
 * イルデスクリプタを全てクローズする。（システム停止・再開後、停止前にオー
 * プンしたファイルへのアクセスは全て失敗する模様。でなくても、記憶装置が抜
 * かれた可能性もあり、停止前と同様にファイルにアクセスできるとは限らない。）
 * ファイル自体の状況（読み込み位置、非同期読み込みの結果など）は保持する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void psp_file_pause(void);

/**
 * psp_file_unpause:  使用中の全ファイルのファイルデスクリプタを再度オープン
 * する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void psp_file_unpause(void);


/******** input.c ********/

/**
 * psp_input_init:  入力機能の初期化を行う。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int psp_input_init(void);


/******** map.c ********/

/**
 * psp_install_map_callbacks:  マップ描画用コールバックを設定する。ついでに
 * マップ描画用静的データを初期化する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void psp_install_map_callbacks(void);


/******** memory.c ********/

/**
 * psp_mem_alloc_pools:  メモリ管理に使うプールをシステムから確保する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗。一時プールを確保できなくても成功とする
 */
extern int psp_mem_alloc_pools(void);


/******** misc.c ********/

/**
 * Forbid, Permit:  他スレッドの実行および割り込み処理を禁止・許可する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 * 【注　意】Forbid()を複数回呼び出すと、同じ回数だけPermit()を呼び出さない
 * 　　　　　限り、他スレッドの実行や割り込み処理は再開されない。
 */
extern void Forbid(void);
extern void Permit(void);

/**
 * psp_start_thread:  新しいスレッドハンドルを作成して開始する。
 *
 * 【引　数】     name: スレッド名
 * 　　　　　    entry: 実行開始アドレス（実行する関数へのポインタ）
 * 　　　　　 priority: 優先度
 * 　　　　　stacksize: スタックの最大サイズ（バイト）
 * 　　　　　     args: 引数サイズ
 * 　　　　　     argp: 引数ポインタ
 * 【戻り値】正数＝スレッドハンドル、負数＝エラー値
 */
extern SceUID psp_start_thread(const char *name, void *entry, int priority,
                               int stacksize, int args, void *argp);

/**
 * psp_delete_thread_if_stopped:  スレッドが終了しているかどうかを確認し、
 * 終了している場合は削除する。
 *
 * 【引　数】      thid: スレッドハンドル
 * 　　　　　status_ret: スレッドの終了コード（負数＝異常終了またはエラー）
 * 　　　　　            　を格納する変数へのポインタ（NULL可）
 * 【戻り値】0以外＝スレッド終了、0＝スレッド動作中
 */
extern int psp_delete_thread_if_stopped(SceUID thid, int *status_ret);

/**
 * psp_strerror:  PSPシステムコールからのエラーコードに対応する説明文字列を
 * 返す。
 *
 * 【引　数】psp_errno: PSPシステムコールからのエラーコード
 * 【戻り値】エラー説明文字列（静的バッファに格納される）
 */
extern const char *psp_strerror(int psp_errno);


#ifdef DEBUG

/**
 * psp_display_DMSG:  最新のデバッグメッセージを画面に表示する。デバッグ時
 * のみ実装。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void psp_display_DMSG(void);

#endif  // DEBUG


/******** sound.c ********/

/**
 * psp_sound_init:  音声機能の初期化を行う。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int psp_sound_init(void);


/*************************************************************************/
/************************** その他の定義・宣言 ***************************/
/*************************************************************************/

/* ファイル関連エラーコード（errno.h同様？） */
enum {
    PSP_EPERM   = 0x80010001,
    PSP_ENOENT  = 0x80010002,
    PSP_ESRCH   = 0x80010003,
    PSP_EINTR   = 0x80010004,
    PSP_EIO     = 0x80010005,
    PSP_ENXIO   = 0x80010006,
    PSP_E2BIG   = 0x80010007,
    PSP_ENOEXEC = 0x80010008,
    PSP_EBADF   = 0x80010009,
    PSP_ECHILD  = 0x8001000A,
    PSP_EAGAIN  = 0x8001000B,
    PSP_ENOMEM  = 0x8001000C,
    PSP_EACCES  = 0x8001000D,
    PSP_EFAULT  = 0x8001000E,
    PSP_ENOTBLK = 0x8001000F,
    PSP_EBUSY   = 0x80010010,
    PSP_EEXIST  = 0x80010011,
    PSP_EXDEV   = 0x80010012,
    PSP_ENODEV  = 0x80010013,
    PSP_ENOTDIR = 0x80010014,
    PSP_EISDIR  = 0x80010015,
    PSP_EINVAL  = 0x80010016,
    PSP_ENFILE  = 0x80010017,
    PSP_EMFILE  = 0x80010018,
    PSP_ENOTTY  = 0x80010019,
    PSP_ETXTBSY = 0x8001001A,
    PSP_EFBIG   = 0x8001001B,
    PSP_ENOSPC  = 0x8001001C,
    PSP_ESPIPE  = 0x8001001D,
    PSP_EROFS   = 0x8001001E,
    PSP_EMLINK  = 0x8001001F,
    PSP_EPIPE   = 0x80010020,
    PSP_EDOM    = 0x80010021,
    PSP_ERANGE  = 0x80010022,
    PSP_EDEADLK = 0x80010023,
    PSP_ENAMETOOLONG = 0x80010024,
    PSP_ECANCELED    = 0x8001007D,  // OSが使うか分からないが、必要なので定義
};

/* その他のエラーコード */
enum {
    PSP_SAVEDATA_NOT_FOUND = 0x80110307,
};

/*************************************************************************/
/*************************************************************************/

#endif  // SYSDEP_PSP_PSPLOCAL_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
