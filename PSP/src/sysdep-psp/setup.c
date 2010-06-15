/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/setup.c: PSP initialization code.
 */

#include "../common.h"
#include "../sound.h"
#include "../sysdep.h"

#include "psplocal.h"
#include "file-read.h"

/*************************************************************************/
/****************************** 各種データ *******************************/
/*************************************************************************/

/* PSPモジュール情報 */
PSP_MODULE_INFO("Aquaria", 0, 0, 1);
const PSP_MAIN_THREAD_PRIORITY(THREADPRI_MAIN);
const PSP_MAIN_THREAD_STACK_SIZE_KB(128);
const PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
const PSP_HEAP_SIZE_KB(0);

/*************************************************************************/

/* 休止（サスペンド）フラグ。0以外になっていると、sys_time_delay()で0になる
 * まで永遠に待つ。 */
volatile int psp_suspend;

/* 休止準備完了フラグ。休止イベント発生時、休止の準備が整った段階で0以外に
 * 設定される。 */
volatile int psp_suspend_ok;

/* 終了フラグ。0以外になった場合、各スレッドは速やかに終了しなければなら
 * ない。 */
volatile int psp_exit;

/************************************/

/* メインスレッドのスレッドID */
static SceUID main_thread;

/*************************************************************************/

/* ローカル関数宣言 */

static void get_base_directory(const char *argv0, char *buf, int bufsize);
static int install_callbacks(void);
static int load_av_modules(void);
#ifdef SUPPORT_FIRMWARE_BEFORE_2_71
static SceUID load_start_module(const char *module, int partition);
#endif

static int callback_thread(SceSize args, void *argp);
static int exit_callback(int arg1, int arg2, void *common);
static int power_callback(int unknown, int powerInfo, void *common);

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * sys_handle_cmdline_param:  共通オプションとして認識されないコマンドライン
 * 引数を処理する。
 *
 * 【引　数】param: コマンドライン引数
 * 【戻り値】なし
 */
void sys_handle_cmdline_param(const char *param)
{
    /* PSP専用オプション等はない */
}

/*************************************************************************/

/**
 * sys_init:  システム依存機能の基本的な初期化処理を行う。
 *
 * 【引　数】argv0: main()に渡されたargv[0]の値
 * 【戻り値】0以外＝成功、0＝失敗
 */
int sys_init(const char *argv0)
{
    /* FPU制御レジスタを初期化。
     * ・ビット24：FS=1 ⇒ 非正規化数を0にflush
     * ・ビット11〜7：EnableVZOUI=0 ⇒ 例外を発生させない
     * ・ビット1〜0：RM=0 ⇒ 最も近い値に丸める
     * デバッグ時は不正確（I）、アンダフロー（U）以外の例外を有効にする。 */
#ifdef DEBUG
    asm("ctc1 %[val], $31" : : [val] "r" (0x01000E00));
#else
    asm("ctc1 %[val], $31" : : [val] "r" (0x01000000));
#endif

    /* このスレッドのIDを取得する */
    main_thread = sceKernelGetThreadId();

    /* プログラムファイルが格納されているをデータディレクトリとする */
    char basepath[256];
    get_base_directory(argv0, basepath, sizeof(basepath));

#ifndef CXX_CONSTRUCTOR_HACK
    /* メモリプールを確保する */
    if (!psp_mem_alloc_pools()) {
        DMSG("Failed to allocate memory pools");
        goto fail;
    }
#endif

    /* HOMEボタン（ユーザによる終了要求）・サスペンド（システム停止）の
     * コールバックを登録する */
    if (!install_callbacks()) {
        DMSG("Failed to install system callbacks");
        goto fail;
    }

    /* 音声・映像ライブラリをロードする */
    if (!load_av_modules()) {
        DMSG("Failed to load AV modules");
#ifdef SUPPORT_FIRMWARE_BEFORE_2_71
        /* 正規FWではそもそもロードできないので、エラーを無視して続行 */
        if (sceKernelDevkitVersion() >= 0x02070100)
#endif
        goto fail;
    }

    /* 読み込みスレッドを開始する */
    if (!psp_file_read_init()) {
        DMSG("Failed to initialize file read thread");
        goto fail;
    }

    /* 各サブ機能を初期化する */
    if (!psp_display_init()) {
        DMSG("Failed to initialize display");
        goto fail;
    }
    if (!psp_file_init(basepath)) {
        DMSG("Failed to initialize file management");
        goto fail;
    }
    if (!psp_input_init()) {
        DMSG("Failed to initialize input control");
        goto fail;
    }
    if (!psp_sound_init()) {
        DMSG("Failed to initialize sound output");
        goto fail;
    }

    /* 成功 */
    return 1;

    /* 失敗した場合はプログラムを強制終了する */
  fail:
    sceKernelExitGame();
    return 0;  // 通常には不要だが、万一システムコールが失敗した場合に備える
}

/*************************************************************************/

/**
 * sys_exit:  システム依存機能のプログラム終了処理を行う。exit()を呼び出す
 * などして、プログラムを終了させなければならない。
 *
 * 【引　数】error: 通常終了か異常終了か（0＝通常終了、0以外＝異常終了）
 * 【戻り値】なし（戻らない）
 */
void sys_exit(int error)
{
    /* 二重呼び出しを回避 */
    static volatile int exiting = 0;
    Forbid();
    const int old_exiting = exiting;
    exiting = 1;
    Permit();
    if (old_exiting) {
        sceKernelExitThread(error);
        DMSG("sceKernelExitThread() returned!!");
        for (;;) {}
    }

    /* 音声関連をすべてリセットする（音声バグを回避） */
    sys_sound_lock();
    int i;
    for (i = 0; i < SOUND_CHANNELS; i++) {
        sys_sound_reset(i);
    }
    sys_sound_unlock();

    /* 終了フラグを立てて、各スレッドが終了するのを待つ */
    psp_exit = 1;
    sceKernelDelayThread(500000);

    /* セーブデータ処理が終了していることを確認する。こうしないと、
     * sceUtilitySavedata周りでフリーズしてしまう模様 */
    while (!sys_savefile_status(NULL)) {
        sceKernelDelayThread(10000);
    }

    /* 終了する。万一システムコールが失敗した場合にも備える */
    sceKernelExitGame();
    DMSG("sceKernelExitGame() failed!!");
    for (;;) {}
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * get_base_directory:  argv[0]からプログラムのベースディレクトリを抽出する。
 *
 * 【引　数】  argv0: argv[0]の値
 * 　　　　　    buf: パス名を格納するバッファ
 * 　　　　　bufsize: bufのサイズ（バイト）
 * 【戻り値】0以外＝成功、0＝失敗
 */
static void get_base_directory(const char *argv0, char *buf, int bufsize)
{
    PRECOND_SOFT(buf != NULL, return);
    PRECOND_SOFT(bufsize > 0, return);

    *buf = 0;
    if (argv0) {
        const char *s = strrchr(argv0, '/');
        if (strncmp(argv0, "disc0:", 6) == 0) {
            snprintf(buf, bufsize, "disc0:/PSP_GAME/USRDIR");
        } else if (s != NULL) {
            int n = snprintf(buf, bufsize,
                             "%.*s", s - argv0, argv0);
            if (UNLIKELY(n >= bufsize)) {
                DMSG("argv[0] too long! %s", argv0);
                *buf = 0;
            }
        } else {
            DMSG("argv[0] has no directory: %s", argv0);
        }
    } else {
        DMSG("argv[0] == NULL!");
    }
}

/*************************************************************************/

/**
 * install_callbacks:  システムコールバック関数を登録する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
static int install_callbacks(void)
{
    SceUID thid = psp_start_thread(
        "SysCallbackThread", callback_thread, THREADPRI_CALLBACK_WATCH,
        0x1000, 0, NULL
    );
    if (UNLIKELY(thid < 0)) {
        DMSG("psp_start_thread(callback_thread) failed: %s",
             psp_strerror(thid));
        return 0;
    }

    return 1;
}

/*************************************************************************/

/**
 * load_av_modules:  音声・映像処理用システムモジュールをロードする。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
static int load_av_modules(void)
{
    int res;

#ifdef SUPPORT_FIRMWARE_BEFORE_2_71
    const int32_t firmware_version = sceKernelDevkitVersion();
    if (firmware_version < 0x02070100) {
        goto load271_fail;
    }
#endif

    res = sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC);
    if (res < 0) {
        DMSG("sceUtilityLoadAvModule(AVCODEC): %s", psp_strerror(res));
        goto load271_fail;
    }
    return 1;

  load271_fail:

#ifdef SUPPORT_FIRMWARE_BEFORE_2_71
    /* 直接モジュールをロードしてみる（注：カスタムFW必須。正規FWでは
     * UMDからしかロードできない） */
    res = load_start_module("flash0:/kd/audiocodec.prx",
                            PSP_MEMORY_PARTITION_KERNEL);
    /* すでにロードされている場合、ERROR_EXCLUSIVE_LOADが返ってくる */
    if (res < 0 && res != SCE_KERNEL_ERROR_EXCLUSIVE_LOAD) {
        DMSG("audiocodec.prx: %s", psp_strerror(res));
        goto load100_fail;
    }
    return 1;

  load100_fail:
#endif  // SUPPORT_FIRMWARE_BEFORE_2_71

    return 0;
}

/*************************************************************************/

#ifdef SUPPORT_FIRMWARE_BEFORE_2_71

/**
 * load_start_module:  PSPモジュールをロードして開始する。
 *
 * 【引　数】   module: モジュールのパス名
 * 　　　　　partition: メモリパーティション (PSP_MEMORY_PARTITON_*)
 * 【戻り値】モジュールIDまたはエラーコード
 */
static SceUID load_start_module(const char *module, int partition)
{
    SceKernelLMOption lmopts;
    mem_clear(&lmopts, sizeof(lmopts));
    lmopts.size     = sizeof(lmopts);
    lmopts.mpidtext = partition;
    lmopts.mpiddata = partition;
    lmopts.position = 0;
    lmopts.access   = 1;

    SceUID modid = sceKernelLoadModule(module, 0, &lmopts);
    if (modid < 0) {
        return modid;
    }

    int dummy;
    int res = sceKernelStartModule(modid, strlen(module)+1, (char *)module,
                                   &dummy, NULL);
    if (res < 0) {
        sceKernelUnloadModule(modid);
        return res;
    }

    return modid;
}

#endif  // SUPPORT_FIRMWARE_BEFORE_2_71

/*************************************************************************/
/*********************** システムコールバック関数 ************************/
/*************************************************************************/

/**
 * callback_thread:  終了要求・電源イベント監視スレッド。
 *
 * 【引　数】args: 引数サイズ（未使用）
 * 　　　　　argp: 引数ポインタ（未使用）
 * 【戻り値】スレッド終了コード（終了しない）
 */
static int callback_thread(SceSize args, void *argp)
{
    SceUID cbid;
    cbid = sceKernelCreateCallback("ExitCallback", exit_callback, NULL);
    if (cbid < 0) {
        DMSG("sceKernelCreateCallback(exit_callback) failed: %s",
             psp_strerror(cbid));
        return 0;
    }
    sceKernelRegisterExitCallback(cbid);

    cbid = sceKernelCreateCallback("PowerCallback", power_callback, NULL);
    if (cbid < 0) {
        DMSG("sceKernelCreateCallback(power_callback) failed: %s",
             psp_strerror(cbid));
        return 0;
    }
    scePowerRegisterCallback(-1, cbid);

    for (;;) {
        sceKernelSleepThreadCB();
    }
    return 0;  // コンパイラ警告回避
}

/*************************************************************************/

/**
 * exit_callback:  終了要求コールバック。ユーザが終了要求を行うと、カーネル
 * から呼び出される。
 *
 * 【引　数】arg1, arg2, common: 未使用
 * 【戻り値】不定（戻らない）
 */
static int exit_callback(int arg1, int arg2, void *common)
{
    sys_exit(0);
}

/*************************************************************************/

/**
 * power_callback:  電源イベントコールバック。
 *
 * 【引　数】   unknown: 未使用
 * 　　　　　power_info: イベントの種類
 * 　　　　　    common: 未使用
 * 【戻り値】不明（常に0）
 */
static int power_callback(int unknown, int power_info, void *common)
{
    if (power_info & (PSP_POWER_CB_SUSPENDING | PSP_POWER_CB_STANDBY)) {
        psp_suspend_ok = 0;
        psp_suspend = 1;
        int i;
        for (i = 0; i < 100; i++) {
            sceKernelDelayThread(10000);  // 0.01秒
            if (psp_suspend_ok) {
                break;
            }
        }
        if (UNLIKELY(i >= 100)) {
            DMSG("WARNING: main thread failed to suspend");
        }
    } else if (power_info & PSP_POWER_CB_RESUME_COMPLETE) {
        psp_suspend = 0;
    }
    return 0;
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
