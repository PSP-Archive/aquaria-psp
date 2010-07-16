/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/misc.c: Miscellaneous PSP functions.
 */

#include "../common.h"
#include "../sysdep.h"

#include "psplocal.h"

#ifdef DEBUG
# include "../debugfont.h"  // デバッグ表示用
#endif

/*************************************************************************/
/*************************** グローバルデータ ****************************/
/*************************************************************************/

/* 直近のシステムコールのエラー値（主にsys_file_*()で使用） */
int psp_errno;

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* Forbid()呼び出し回数 */
static unsigned int forbid_count;
/* Forbid()・Permit()用割り込みレジスタ値 */
static int forbid_intstatus;

#ifdef DEBUG

/* デバッグメッセージバッファ関連 */
static char DMSG_buffer[16384];         // テキストバッファ
static unsigned int DMSG_buffer_index;  // 次のメッセージを書き込むオフセット
static struct {                         // 各行のデータ
    uint16_t offset;                    // ├ テキストの開始オフセット
    uint8_t length;                     // ├ テキストの長さ（バイト）
    uint8_t indented;                   // └ 表示時、インデントするかどうか
} DMSG_lines[100];
static int DMSG_lines_index;            // 次行のデータを格納するインデックス

/* デバッグメッセージ表示パラメータ */
#define DMSG_DISPLAY_X0      18
#define DMSG_DISPLAY_Y0      16
#define DMSG_DISPLAY_X1      (480-18)
#define DMSG_DISPLAY_Y1      (272-16)
#define DMSG_DISPLAY_BORDER  4
#define DMSG_DISPLAY_INDENT  10

#endif

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * sys_set_performance:  システムの処理速度を設定する。
 *
 * 【引　数】level: 処理速度（SYS_PERFORMANCE_*）
 * 【戻り値】なし
 * 【注　意】関数はすぐに戻らず、実行に多少時間がかかる可能性がある。
 */
void sys_set_performance(int level)
{
    switch (level) {
      case SYS_PERFORMANCE_LOW:
        scePowerSetClockFrequency(111, 111,  55);
        break;
      case SYS_PERFORMANCE_NORMAL:
        scePowerSetClockFrequency(222, 222, 111);
        break;
      case SYS_PERFORMANCE_HIGH:
        scePowerSetClockFrequency(333, 333, 166);
        break;
      default:
        DMSG("Invalid level %d", level);
        break;
    }
}

/*************************************************************************/

/**
 * sys_report_error:  エラーの発生をユーザに知らせる。
 *
 * 【引　数】message:  エラーメッセージ
 * 【戻り値】なし
 */
void sys_report_error(const char *message)
{
#if 0 // 一応動くが、sys_display_start()前は表示されず、sys_display_start()後
      // はクラッシュしてしまうのでコメントアウト
    /* SceUtilityMsgDialogParams構造体。pspsdkのものはbuttons・unknown2が
     * 抜けている */
    struct {
        int size;
        int language;
        int swap_buttons;       // 0以外＝○×を入れ替える
        int threadpri_graphics;
        int threadpri_unknown;
        int threadpri_font;
        int threadpri_sound;
        int result;
        int unknown[7];
        char message[512];
        int buttons;            // 有効ボタン
        int unknown2;
    } dialog;

    mem_clear(&dialog, sizeof(dialog));
    dialog.size = sizeof(dialog);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,
                                &dialog.language);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,
                                &dialog.swap_buttons);
    dialog.threadpri_graphics = THREADPRI_MAIN - 3;
    dialog.threadpri_unknown  = THREADPRI_MAIN - 1;
    dialog.threadpri_font     = THREADPRI_MAIN - 2;
    dialog.threadpri_sound    = THREADPRI_MAIN - 4;
    dialog.unknown[5] = 1;
    snprintf(dialog.message, sizeof(dialog.message), "%s", message);
    dialog.buttons = 0x001;  // または011, 111, 000, 010, 110？
    dialog.unknown2 = -1;

    sys_display_init();  // 念のため
    sceUtilityMsgDialogInitStart((void *)&dialog);
    int res;
    while ((res = sceUtilityMsgDialogGetStatus()) != 0) {
        if (res == 2) {
            sceUtilityMsgDialogUpdate(2);
        }
        if (res == 3) {
            sceUtilityMsgDialogShutdownStart();
        }
        sceDisplayWaitVblankStart();
    }
#else  // 0
    DMSG("%s", message);
#endif
}

/*************************************************************************/

/**
 * sys_last_error:  システム依存関数で、直前に発生したエラーの種類を返す。
 * エラー直後以外の時に呼び出した場合の動作は不定。
 *
 * 【引　数】なし
 * 【戻り値】エラー値（SYSERR_*）
 */
int sys_last_error(void)
{
    switch (psp_errno) {
        case 0:             return SYSERR_NO_ERROR;
        case PSP_ENOENT:    return SYSERR_FILE_NOT_FOUND;
        case PSP_EACCES:    return SYSERR_FILE_ACCESS_DENIED;
        case PSP_ECANCELED: return SYSERR_FILE_ASYNC_ABORTED;
        case SCE_KERNEL_ERROR_ASYNC_BUSY: return SYSERR_FILE_ASYNC_READING;
        case SCE_KERNEL_ERROR_NOASYNC:    return SYSERR_FILE_ASYNC_NONE;
    }
    return SYSERR_UNKNOWN_ERROR;
}

/*************************************************************************/

/**
 * sys_last_errstr:  システム依存関数で、直前に発生したエラーの説明文字列を
 * 返す。エラー直後以外の時に呼び出した場合の動作は不定（ただし戻り値は有効
 * な文字列ポインタとなる）。
 *
 * 【引　数】なし
 * 【戻り値】エラー説明文字列
 */
const char *sys_last_errstr(void)
{
    return psp_strerror(psp_errno);
}

/*************************************************************************/

/**
 * sys_ping:  スクリーンセーバー等のタイマーをリセットする。ゲームのイベント
 * シーン等、ユーザが長時間操作せずに画面を見ている時にスクリーンセーバーが
 * 入ってしまうのを回避するため、定期的に（例えば1フレーム毎に）呼び出す。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sys_ping(void)
{
    scePowerTick(0);
}

/*************************************************************************/
/*************************************************************************/

#ifdef DEBUG

/**
 * sys_DMSG:  DMSGマクロの補助関数。実際にメッセージを出力する。
 * デバッグ時のみ実装。
 *
 * 【引　数】format: フォーマット文字列
 * 　　　　　  args: フォーマットデータ
 * 【戻り値】なし
 */
void sys_DMSG(const char *format, va_list args)
{
    static char buf[10000];
    uint32_t time = sceKernelGetSystemTimeLow();
    snprintf(buf, sizeof(buf), "%d.%06d: ", time/1000000, time%1000000);
    vsnprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), format, args);
    sceIoWrite(2, buf, strlen(buf));

    if (!strstr(buf, "timer_wait")) {  // 遅延メッセージは表示しない

        const unsigned int start = DMSG_buffer_index;
        const unsigned int size = ubound(strlen(buf)+1, sizeof(DMSG_buffer)-1);

        /* バッファの末尾をオーバーフローする場合、ポインタをバッファ先頭に
         * 戻す。startは、最後の上書き検索のためにそのままにしておく */
        if (DMSG_buffer_index + size > sizeof(DMSG_buffer)) {
            DMSG_buffer_index = 0;
        }

        /* この行の開始位置を記録する（配列が満杯の場合は最も古い行を消す） */
        if (DMSG_lines_index >= lenof(DMSG_lines)) {
            memmove(&DMSG_lines[0], &DMSG_lines[1],
                    sizeof(DMSG_lines) - sizeof(DMSG_lines[0]));
            DMSG_lines_index = lenof(DMSG_lines) - 1;
        }
        unsigned int line = DMSG_lines_index++;
        DMSG_lines[line].offset = DMSG_buffer_index;
        DMSG_lines[line].length = 0;
        DMSG_lines[line].indented = 0;

        /* 文字列をテキストバッファに格納する */
        memcpy(&DMSG_buffer[DMSG_buffer_index], buf, size);
        DMSG_buffer_index += size;

        /* 上書きされた行があれば消す */
        const unsigned int overwritten =
            ((DMSG_buffer_index + sizeof(DMSG_buffer)) - start)
            % sizeof(DMSG_buffer);
        int i;
        for (i = DMSG_lines_index-2; i >= 0; i--) {
            const unsigned int offset =
                ((DMSG_lines[i].offset + sizeof(DMSG_buffer)) - start)
                % sizeof(DMSG_buffer);
            if (offset < overwritten) {
                /* 後ろから検索しているので、これが上書きされた行のうち最も
                 * 新しいものである。0番からこの行を含めて消す */
                const int num_to_delete = i + 1;
                memmove(&DMSG_lines[0], &DMSG_lines[num_to_delete],
                        sizeof(DMSG_lines[0])
                            * (lenof(DMSG_lines) - num_to_delete));
                DMSG_lines_index -= num_to_delete;
                break;
            }
        }

        /* 表示領域に合わせて改行する。フォント関数をここで呼び出すわけには
         * いかない（初期化されていないかもしれないし、DMSG()出力があると
         * 無限再帰ループに入ってしまう）ので、1文字6ピクセルとして計算 */
        uint32_t linestart = DMSG_lines[line].offset;
        int indented = 0;
        do {
            const int x = DMSG_DISPLAY_X0 + DMSG_DISPLAY_BORDER
                          + (indented ? DMSG_DISPLAY_INDENT : 0);
            const int w = (DMSG_DISPLAY_X1 - DMSG_DISPLAY_BORDER) - x;
            const int left = strlen(&DMSG_buffer[linestart]);
            int len = ubound(left, w/6);
            DMSG_lines[line].offset = linestart;
            DMSG_lines[line].length = len;
            DMSG_lines[line].indented = indented;
            indented = 1;
            linestart += len;
            if (DMSG_buffer[linestart]) {  // まだ文字列の末尾ではない
                if (DMSG_lines_index < lenof(DMSG_lines)) {
                    line = DMSG_lines_index++;
                } else {
                    /* DMSG_lines[1..line]を一つ前へずらす */
                    memmove(&DMSG_lines[0], &DMSG_lines[1],
                            sizeof(DMSG_lines[0]) * (line-1));
                }
            }
        } while (DMSG_buffer[linestart]);

    }  // if (!strstr(buf, "timer_wait"))
}

#endif  // DEBUG

/*************************************************************************/
/************************ PSPライブラリ内部用関数 ************************/
/*************************************************************************/

/**
 * Forbid, Permit:  他スレッドの実行および割り込み処理を禁止・許可する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 * 【注　意】Forbid()を複数回呼び出すと、同じ回数だけPermit()を呼び出さない
 * 　　　　　限り、他スレッドの実行や割り込み処理は再開されない。
 */

void Forbid(void)
{
    const int intstatus = sceKernelCpuSuspendIntr();
    if (forbid_count == 0) {
        forbid_intstatus = intstatus;
    }
    forbid_count++;
}

void Permit(void)
{
    if (forbid_count > 0) {
        forbid_count--;
        if (forbid_count == 0) {
            sceKernelCpuResumeIntrWithSync(forbid_intstatus);
        }
    }
}

/*************************************************************************/

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
SceUID psp_start_thread(const char *name, void *entry, int priority,
                        int stacksize, int args, void *argp)
{
    if (UNLIKELY(!name)
     || UNLIKELY(!entry)
     || UNLIKELY(priority < 0)
     || UNLIKELY(stacksize < 0)
    ) {
        DMSG("Invalid parameters: %p[%s] %p %d %d %d %p",
             name, name ? name : "", entry, priority, stacksize, args, argp);
        return PSP_EINVAL;
    }
    SceUID handle = sceKernelCreateThread(name, entry, priority, stacksize,
                                          0, NULL);
    if (UNLIKELY(handle < 0)) {
        DMSG("Failed to create thread \"%s\": %s", name, psp_strerror(handle));
        return handle;
    }
    int32_t res = sceKernelStartThread(handle, args, argp);
    if (UNLIKELY(res < 0)) {
        DMSG("Failed to start thread \"%s\": %s", name, psp_strerror(res));
        sceKernelDeleteThread(handle);
        return res;
    }
    return handle;
}

/*************************************************************************/

/**
 * psp_delete_thread_if_stopped:  スレッドが終了しているかどうかを確認し、
 * 終了している場合は削除する。
 *
 * 【引　数】      thid: スレッドハンドル
 * 　　　　　status_ret: スレッドの終了コード（負数＝異常終了またはエラー）
 * 　　　　　            　を格納する変数へのポインタ（NULL可）
 * 【戻り値】0以外＝スレッド終了、0＝スレッド動作中
 */
int psp_delete_thread_if_stopped(SceUID thid, int *status_ret)
{
    SceKernelThreadInfo thinfo;
    mem_clear(&thinfo, sizeof(thinfo));
    thinfo.size = sizeof(thinfo);
    int res = sceKernelReferThreadStatus(thid, &thinfo);
    if (UNLIKELY(res < 0)) {
        DMSG("sceKernelReferThreadStatus(0x%08X) failed: %s",
             thid, psp_strerror(res));
        sceKernelTerminateThread(thid);
    } else if (thinfo.status & (PSP_THREAD_RUNNING | PSP_THREAD_READY | PSP_THREAD_WAITING)) {
        return 0;
    } else if (thinfo.status & PSP_THREAD_STOPPED) {
        res = thinfo.exitStatus;
    } else {
        res = 0x80000000 | thinfo.status;
        sceKernelTerminateThread(thid);
    }
    sceKernelDeleteThread(thid);
    if (status_ret) {
        *status_ret = res;
    }
    return 1;
}

/*************************************************************************/

/**
 * psp_strerror:  PSPシステムコールからのエラーコードに対応する説明文字列を
 * 返す。
 *
 * 【引　数】psp_errno: PSPシステムコールからのエラーコード
 * 【戻り値】エラー説明文字列（静的バッファに格納される）
 */
const char *psp_strerror(int psp_errno)
{
    static char errbuf[100];
    snprintf(errbuf, sizeof(errbuf), "%08X%s", psp_errno,
             psp_errno==PSP_EPERM   ? ": Operation not permitted" :
             psp_errno==PSP_ENOENT  ? ": No such file or directory" :
             psp_errno==PSP_ESRCH   ? ": No such process" :
             psp_errno==PSP_EINTR   ? ": Interrupted system call" :
             psp_errno==PSP_EIO     ? ": I/O error" :
             psp_errno==PSP_ENXIO   ? ": No such device or address" :
             psp_errno==PSP_E2BIG   ? ": Argument list too long" :
             psp_errno==PSP_ENOEXEC ? ": Exec format error" :
             psp_errno==PSP_EBADF   ? ": Bad file number" :
             psp_errno==PSP_ECHILD  ? ": No child processes" :
             psp_errno==PSP_EAGAIN  ? ": Try again" :
             psp_errno==PSP_ENOMEM  ? ": Out of memory" :
             psp_errno==PSP_EACCES  ? ": Permission denied" :
             psp_errno==PSP_EFAULT  ? ": Bad address" :
             psp_errno==PSP_ENOTBLK ? ": Block device required" :
             psp_errno==PSP_EBUSY   ? ": Device or resource busy" :
             psp_errno==PSP_EEXIST  ? ": File exists" :
             psp_errno==PSP_EXDEV   ? ": Cross-device link" :
             psp_errno==PSP_ENODEV  ? ": No such device" :
             psp_errno==PSP_ENOTDIR ? ": Not a directory" :
             psp_errno==PSP_EISDIR  ? ": Is a directory" :
             psp_errno==PSP_EINVAL  ? ": Invalid argument" :
             psp_errno==PSP_ENFILE  ? ": File table overflow" :
             psp_errno==PSP_EMFILE  ? ": Too many open files" :
             psp_errno==PSP_ENOTTY  ? ": Not a typewriter" :
             psp_errno==PSP_ETXTBSY ? ": Text file busy" :
             psp_errno==PSP_EFBIG   ? ": File too large" :
             psp_errno==PSP_ENOSPC  ? ": No space left on device" :
             psp_errno==PSP_ESPIPE  ? ": Illegal seek" :
             psp_errno==PSP_EROFS   ? ": Read-only file system" :
             psp_errno==PSP_EMLINK  ? ": Too many links" :
             psp_errno==PSP_EPIPE   ? ": Broken pipe" :
             psp_errno==PSP_EDOM    ? ": Math argument out of domain of func" :
             psp_errno==PSP_ERANGE  ? ": Math result not representable" :
             psp_errno==PSP_EDEADLK ? ": Resource deadlock would occur" :
             psp_errno==PSP_ENAMETOOLONG             ? ": File name too long" :
             psp_errno==PSP_ECANCELED                ? ": Operation canceled" :
             psp_errno==SCE_KERNEL_ERROR_NOFILE      ? ": File not found" :
             psp_errno==SCE_KERNEL_ERROR_MFILE       ? ": Too many files open" :
             psp_errno==SCE_KERNEL_ERROR_NODEV       ? ": Device not found" :
             psp_errno==SCE_KERNEL_ERROR_XDEV        ? ": Cross-device link" :
             psp_errno==SCE_KERNEL_ERROR_INVAL       ? ": Invalid argument" :
             psp_errno==SCE_KERNEL_ERROR_BADF        ? ": Bad file descriptor" :
             psp_errno==SCE_KERNEL_ERROR_NAMETOOLONG ? ": File name too long" :
             psp_errno==SCE_KERNEL_ERROR_IO          ? ": I/O error" :
             psp_errno==SCE_KERNEL_ERROR_NOMEM       ? ": Out of memory" :
             psp_errno==SCE_KERNEL_ERROR_ASYNC_BUSY  ? ": Asynchronous I/O in progress" :
             psp_errno==SCE_KERNEL_ERROR_NOASYNC     ? ": No asynchronous I/O in progress" :
             psp_errno==0x80000023 ? ": Invalid address" :
             psp_errno==0x80110002 ? ": sceUtility: Bad address" :
             psp_errno==0x80110004 ? ": sceUtility: Invalid parameter size" :
             psp_errno==0x80110005 ? ": sceUtility: Other utility busy" :
             psp_errno==0x80110301 ? ": sceUtilitySavedata: No memory card inserted (load)" :
             psp_errno==0x80110305 ? ": sceUtilitySavedata: I/O error (load)" :
             psp_errno==0x80110306 ? ": sceUtilitySavedata: Save file corrupt" :
             psp_errno==0x80110307 ? ": sceUtilitySavedata: Save file not found" :
             psp_errno==0x80110308 ? ": sceUtilitySavedata: Invalid parameters for load" :
             psp_errno==0x80110381 ? ": sceUtilitySavedata: No memory card inserted (save)" :
             psp_errno==0x80110383 ? ": sceUtilitySavedata: Memory card full" :
             psp_errno==0x80110384 ? ": sceUtilitySavedata: Memory card write-protected" :
             psp_errno==0x80110385 ? ": sceUtilitySavedata: I/O error (save)" :
             psp_errno==0x80110388 ? ": sceUtilitySavedata: Invalid parameters for save" :
             psp_errno==0x80260003 ? ": sceAudio: Bad channel number" :
             psp_errno==0x80260009 ? ": sceAudio: Channel is playing" :
             psp_errno==0x8026000B ? ": sceAudio: Bad volume" :
             psp_errno==0x806101FE ? ": sceMpeg: Invalid parameter" :
             psp_errno==0x80618005 ? ": sceMpeg: Stream already registered _or_ double init" :
             psp_errno==0x80618006 ? ": sceMpeg: Initialization failed" :
             psp_errno==0x806201FE ? ": sceVideocodec: Invalid parameter / internal error" :
             psp_errno==0x807F0002 ? ": sceAudiocodec: Invalid codec" :
             psp_errno==0x807F0003 ? ": sceAudiocodec: EDRAM allocation failed" :
             psp_errno==0x807F00FD ? ": sceAudiocodec: Decoding failed" :
             "");
    return errbuf;
}

/*************************************************************************/

#ifdef DEBUG

/**
 * psp_display_DMSG:  最新のデバッグメッセージを画面に表示する。デバッグ時
 * のみ実装。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void psp_display_DMSG(void)
{
    const int x0 = DMSG_DISPLAY_X0;
    const int y0 = DMSG_DISPLAY_Y0;
    const int x1 = DMSG_DISPLAY_X1;
    const int y1 = DMSG_DISPLAY_Y1;
    const int border = DMSG_DISPLAY_BORDER;
    const int indent = DMSG_DISPLAY_INDENT;
    const uint32_t background = 0x55000000;

    const int fonth = iroundf(debugfont_height(1));
    int y = y1 - border - fonth;
    int line = DMSG_lines_index - 1;

    sys_display_fill(x0, y+fonth, x1-1, y1-1, background);
    while (y >= y0+border && line >= 0) {
        char buf[200];
        sys_display_fill(x0, y, x1-1, y+fonth-1, background);
        const int x = x0 + border + (DMSG_lines[line].indented ? indent : 0);
        snprintf(buf, sizeof(buf), "%.*s", DMSG_lines[line].length,
                 &DMSG_buffer[DMSG_lines[line].offset]);
        debugfont_draw_text(buf, x, y, 0xFFFFFF, 1, 1, 0);
        y -= fonth;
        line--;
    }
    y += fonth;
    sys_display_fill(x0, y-border, x1-1, y-1, background);
}

#endif  // DEBUG

/*************************************************************************/
/*************************** 三角関数テーブル ****************************/
/*************************************************************************/

#ifdef USE_TRIG_TABLES

/*************************************************************************/

/*
 * 三角関数のsin()、cos()、tan()を1/4度単位で計算したテーブルと、テーブルを
 * 利用する関数。ハードウェアsinやcos命令を持たないPSPにおける処理の高速化
 * のほか、ラジアン処理で生じる誤差（例えばsin(M_PI)は0にはならない）を回避
 * するために使う。
 */

#undef dsinf
#undef dcosf
#undef dtanf
#undef dsincosf

/*************************************************************************/

/**
 * dsincosf_table:  dsinf()及びdcosf()のテーブル。dsinf(0)からdsinf(90)まで
 * 0.5度単位で定義されている。
 */
static const float dsincosf_table[90*2+1] = {
    +0.00000000000, +0.00872653550,
    +0.01745240644, +0.02617694831,
    +0.03489949670, +0.04361938737,
    +0.05233595624, +0.06104853953,
    +0.06975647374, +0.07845909573,
    +0.08715574275, +0.09584575252,
    +0.10452846327, +0.11320321377,
    +0.12186934341, +0.13052619222,
    +0.13917310096, +0.14780941113,
    +0.15643446504, +0.16504760586,
    +0.17364817767, +0.18223552549,
    +0.19080899538, +0.19936793442,
    +0.20791169082, +0.21643961394,
    +0.22495105434, +0.23344536386,
    +0.24192189560, +0.25038000405,
    +0.25881904510, +0.26723837608,
    +0.27563735582, +0.28401534470,
    +0.29237170472, +0.30070579950,
    +0.30901699437, +0.31730465641,
    +0.32556815446, +0.33380685923,
    +0.34202014333, +0.35020738126,
    +0.35836794955, +0.36650122672,
    +0.37460659342, +0.38268343237,
    +0.39073112849, +0.39874906893,
    +0.40673664308, +0.41469324266,
    +0.42261826174, +0.43051109681,
    +0.43837114679, +0.44619781311,
    +0.45399049974, +0.46174861324,
    +0.46947156279, +0.47715876026,
    +0.48480962025, +0.49242356010,
    +0.50000000000, +0.50753836296,
    +0.51503807491, +0.52249856472,
    +0.52991926423, +0.53729960835,
    +0.54463903502, +0.55193698531,
    +0.55919290347, +0.56640623692,
    +0.57357643635, +0.58070295571,
    +0.58778525229, +0.59482278675,
    +0.60181502315, +0.60876142901,
    +0.61566147533, +0.62251463664,
    +0.62932039105, +0.63607822028,
    +0.64278760969, +0.64944804833,
    +0.65605902899, +0.66262004822,
    +0.66913060636, +0.67559020762,
    +0.68199836006, +0.68835457569,
    +0.69465837046, +0.70090926430,
    +0.70710678119, +0.71325044915,
    +0.71933980034, +0.72537437101,
    +0.73135370162, +0.73727733681,
    +0.74314482548, +0.74895572079,
    +0.75470958022, +0.76040596560,
    +0.76604444312, +0.77162458339,
    +0.77714596146, +0.78260815685,
    +0.78801075361, +0.79335334029,
    +0.79863551005, +0.80385686062,
    +0.80901699437, +0.81411551836,
    +0.81915204429, +0.82412618862,
    +0.82903757256, +0.83388582207,
    +0.83867056795, +0.84339144581,
    +0.84804809616, +0.85264016435,
    +0.85716730070, +0.86162916044,
    +0.86602540378, +0.87035569594,
    +0.87461970714, +0.87881711266,
    +0.88294759286, +0.88701083318,
    +0.89100652419, +0.89493436160,
    +0.89879404630, +0.90258528435,
    +0.90630778704, +0.90996127088,
    +0.91354545764, +0.91706007439,
    +0.92050485345, +0.92387953251,
    +0.92718385457, +0.93041756798,
    +0.93358042650, +0.93667218925,
    +0.93969262079, +0.94264149109,
    +0.94551857560, +0.94832365521,
    +0.95105651630, +0.95371695075,
    +0.95630475596, +0.95881973487,
    +0.96126169594, +0.96363045321,
    +0.96592582629, +0.96814764038,
    +0.97029572628, +0.97236992040,
    +0.97437006479, +0.97629600712,
    +0.97814760073, +0.97992470462,
    +0.98162718345, +0.98325490756,
    +0.98480775301, +0.98628560154,
    +0.98768834060, +0.98901586336,
    +0.99026806874, +0.99144486137,
    +0.99254615164, +0.99357185568,
    +0.99452189537, +0.99539619837,
    +0.99619469809, +0.99691733373,
    +0.99756405026, +0.99813479842,
    +0.99862953475, +0.99904822158,
    +0.99939082702, +0.99965732498,
    +0.99984769516, +0.99996192306,
    +1.00000000000
};

/*************************************************************************/

/**
 * dtanf_table:  dtanf()のテーブル。dtanf(0)からdtanf(45)まで0.25度単位で
 * 定義されている（関数の値が大きくなるに連れて精度が落ちるので、45度以上
 * は逆数で計算する）。
 */
static const float dtanf_table[45*4+1] = {
    +0.00000000000, +0.00436335082, +0.00872686779, +0.01309071708,
    +0.01745506493, +0.02182007762, +0.02618592157, +0.03055276330,
    +0.03492076949, +0.03929010701, +0.04366094291, +0.04803344449,
    +0.05240777928, +0.05678411513, +0.06116262015, +0.06554346282,
    +0.06992681194, +0.07431283674, +0.07870170682, +0.08309359225,
    +0.08748866353, +0.09188709167, +0.09628904820, +0.10069470518,
    +0.10510423527, +0.10951781168, +0.11393560830, +0.11835779964,
    +0.12278456090, +0.12721606800, +0.13165249759, +0.13609402708,
    +0.14054083470, +0.14499309949, +0.14945100135, +0.15391472106,
    +0.15838444032, +0.16286034179, +0.16734260908, +0.17183142683,
    +0.17632698071, +0.18082945746, +0.18533904493, +0.18985593211,
    +0.19438030914, +0.19891236738, +0.20345229942, +0.20800029913,
    +0.21255656167, +0.21712128355, +0.22169466264, +0.22627689826,
    +0.23086819113, +0.23546874348, +0.24007875908, +0.24469844323,
    +0.24932800284, +0.25396764647, +0.25861758436, +0.26327802844,
    +0.26794919243, +0.27263129185, +0.27732454406, +0.28202916830,
    +0.28674538576, +0.29147341959, +0.29621349496, +0.30096583913,
    +0.30573068146, +0.31050825346, +0.31529878888, +0.32010252370,
    +0.32491969623, +0.32975054714, +0.33459531950, +0.33945425886,
    +0.34432761329, +0.34921563342, +0.35411857253, +0.35903668658,
    +0.36397023427, +0.36891947711, +0.37388467948, +0.37886610870,
    +0.38386403504, +0.38887873185, +0.39391047561, +0.39895954597,
    +0.40402622584, +0.40911080143, +0.41421356237, +0.41933480176,
    +0.42447481621, +0.42963390597, +0.43481237496, +0.44001053089,
    +0.44522868531, +0.45046715369, +0.45572625553, +0.46100631443,
    +0.46630765815, +0.47163061877, +0.47697553270, +0.48234274082,
    +0.48773258857, +0.49314542603, +0.49858160805, +0.50404149432,
    +0.50952544949, +0.51503384328, +0.52056705055, +0.52612545149,
    +0.53170943166, +0.53731938214, +0.54295569964, +0.54861878663,
    +0.55430905145, +0.56002690847, +0.56577277819, +0.57154708737,
    +0.57735026919, +0.58318276340, +0.58904501642, +0.59493748154,
    +0.60086061903, +0.60681489631, +0.61280078814, +0.61881877672,
    +0.62486935191, +0.63095301138, +0.63707026081, +0.64322161401,
    +0.64940759320, +0.65562872910, +0.66188556120, +0.66817863792,
    +0.67450851684, +0.68087576490, +0.68728095860, +0.69372468426,
    +0.70020753821, +0.70673012705, +0.71329306790, +0.71989698859,
    +0.72654252801, +0.73323033626, +0.73996107503, +0.74673541778,
    +0.75355405010, +0.76041766995, +0.76732698798, +0.77428272784,
    +0.78128562651, +0.78833643459, +0.79543591667, +0.80258485167,
    +0.80978403320, +0.81703426989, +0.82433638582, +0.83169122088,
    +0.83909963118, +0.84656248944, +0.85408068546, +0.86165512651,
    +0.86928673782, +0.87697646299, +0.88472526456, +0.89253412440,
    +0.90040404430, +0.90833604645, +0.91633117402, +0.92439049166,
    +0.93251508614, +0.94070606691, +0.94896456671, +0.95729174225,
    +0.96568877481, +0.97415687092, +0.98269726312, +0.99131121059,
    +1.00000000000
};

/*************************************************************************/

/**
 * dsinf, dcosf:  度単位で表した角度の正弦または余弦を単精度で計算する。同一
 * 関数で実装され、sin(x)はcos(x-90)として評価される（cos(x) = cos(-x)が成り
 * 立つため、内部的に処理が若干速くなる）。
 *
 * 具体的には、
 * (1) 角度の絶対値を取る（符号ビットをクリアする）。
 * (2) オーバフローチェックを行う。汎用レジスタを使用するため、角度が2^30
 *     以上になるとFPU例外が発生し、正しい結果を計算できなくなるので、この
 *     場合はNaNを返す。
 * (3) 角度を2倍にする。指数部を直接操作するため、角度が0の場合は誤作動を
 *     起こしてしまう（0が2^-122になる）ので、このときは1.0を関数から返す。
 * (4) 4倍にした角度を整数と余りに分割する。
 * (5) 整数をさらに90*2で割り、商の下位ビット（0〜3）と整数余り
 *     （0〜90*2-1＝0.0〜89.5度）に従って、テーブルから計算する。
 *     　・商＝0（角度＝  0〜 89）：sin(90-余り)を計算する
 *     　・商＝1（角度＝ 90〜179）：sin(余り)を計算し、符号を反転する
 *     　・商＝2（角度＝180〜269）：sin(90-余り)を計算し、符号を反転する
 *     　・商＝3（角度＝270〜359）：sin(余り)を計算する
 *     ※内部的には商に1を足し、ビット0でsin(余り)とsin(90-余り)を選択し、
 *     　ビット1で符号の反転を行う。
 * (6) (4)で分割した角度の余りが0でない（つまり角度が0.5の倍数でない）場合、
 *     補間処理を行う（次のテーブルエントリーと余りの積を関数結果に加える）。
 *
 * 【引　数】x: 角度（度単位）
 * 【戻り値】xの正弦または余弦
 * 【前条件】|x| < 2^30
 */

CONST_FUNCTION float dsinf(const float x)
{
    return dcosf(x-90);
}

CONST_FUNCTION float dcosf(const float x)
{
    float result, temp, x_frac;
    uint32_t x_bits;
    asm(".set push; .set noreorder\n\
        mfc1 %[x_bits], %[x]                                            \n\
        li $t0, 0x4E800000      # 2^30（これ以上はオーバフロー発生）    \n\
        ins %[x_bits], $zero, 31, 1  # 符号をクリア                     \n\
        sltu $t0, %[x_bits], $t0                                        \n\
        beqz $t0, 9f                                                    \n\
        srl $t0, %[x_bits], 23  # 0を検出する（2倍できないため特殊処理）\n\
        beqz $t0, 8f                                                    \n\
        li $t0, 0x00800000      # 指数に1を足し、2倍にする              \n\
        addu $t0, %[x_bits], $t0                                        \n\
        mtc1 $t0, %[result]                                             \n\
        nop                                                             \n\
        trunc.w.s %[temp], %[result]                                    \n\
        mfc1 $t0, %[temp]                                               \n\
        cvt.s.w %[temp], %[temp]                                        \n\
        sub.s %[x_frac], %[result], %[temp]                             \n\
        # ((x+90)*2)/(90*2)の商を$t1に、余りを$t0に格納                 \n\
        sltiu $t1, $t0, 90*2                                            \n\
        li $t2, 90*2                                                    \n\
        bnez $t1, 1f                                                    \n\
        li $t1, 1                                                       \n\
        divu $0, $t0, $t2                                               \n\
        mfhi $t0                                                        \n\
        mflo $t1                                                        \n\
        addiu $t1, $t1, 1                                               \n\
     1: # xがちょうど0.5の倍数の場合、補間処理を省く                    \n\
        mfc1 $t2, %[x_frac]                                             \n\
        andi $t3, $t1, 1                                                \n\
        bnez $t2, 2f                                                    \n\
        srl $t1, $t1, 1                                                 \n\
        # $t1ビット0（角度0〜90、180〜270）で角度を逆にする（30度を     \n\
        # 60度に、など）。cos(30) = sin(90+30) = sin(90-30)             \n\
        li $t2, 90*2                                                    \n\
        subu $t2, $t2, $t0                                              \n\
        movn $t0, $t2, $t3                                              \n\
        # テーブルから関数の値をロード                                  \n\
        sll $t0, $t0, 2                                                 \n\
        addu $t3, %[table], $t0                                         \n\
        lw $t0, 0($t3)                                                  \n\
        # $t1ビット1（角度90〜270）で符号を反転する                     \n\
        sll $t1, $t1, 31                                                \n\
        xor $t0, $t0, $t1                                               \n\
        # 終了                                                          \n\
        jr $ra                                                          \n\
        mtc1 $t0, $f0                                                   \n\
     2: # 補間処理                                                      \n\
        li $t2, 90*2-1                                                  \n\
        subu $t2, $t2, $t0                                              \n\
        movn $t0, $t2, $t3                                              \n\
        bnezl $t3, 3f                                                   \n\
        sub.s %[x_frac], %[one], %[x_frac]                              \n\
     3: sll $t0, $t0, 2                                                 \n\
        addu $t3, %[table], $t0                                         \n\
        lwc1 %[result], 0($t3)                                          \n\
        lwc1 %[temp], 4($t3)                                            \n\
        sub.s %[temp], %[temp], %[result]                               \n\
        mul.s %[temp], %[temp], %[x_frac]                               \n\
        add.s $f0, %[result], %[temp]                                   \n\
        andi $t1, $t1, 1                                                \n\
        bnezl $t1, 4f                                                   \n\
        neg.s $f0, $f0                                                  \n\
     4: jr $ra                                                          \n\
        nop                                                             \n\
     8: # 0は特殊処理（2倍しても0）                                     \n\
        jr $ra                                                          \n\
        mov.s $f0, %[one]                                               \n\
     9: # オーバフローの場合はNaNを返す                                 \n\
        li $t0, 0x7FFFFFFF                                              \n\
        mtc1 $t0, %[result]                                             \n\
        .set pop"
        : [result] "=&f" (result), [temp] "=&f" (temp),
          [x_frac] "=&f" (x_frac), [x_bits] "=&r" (x_bits)
        : [x] "f" (x), [table] "r" (dsincosf_table), [one] "f" (1.0f)
        : "t0", "t1", "t2", "t3"
    );
    return result;
}

/*************************************************************************/

/**
 * dsincosf_kernel:  度単位で表した角度の正弦と余弦を同時に単精度で計算する。
 *
 * 【引　数】   x: 角度（度単位）
 * 　　　　　sinx: xの正弦を格納する変数へのポインタ
 * 　　　　　cosx: xの正弦を格納する変数へのポインタ
 * 【戻り値】$f0に正弦、$f1に余弦を返す
 * 【前条件】|x| < 2^30
 */
void dsincosf_kernel(const float x)
{
    float result, temp, x_frac, cos_frac;
    uint32_t x_bits, sign;
    asm volatile(".set push; .set noreorder\n\
        mfc1 %[x_bits], %[x]                                            \n\
        li $t0, 0x4E800000      # 2^30（これ以上はオーバフロー発生）    \n\
        srl %[sign], %[x_bits], 31                                      \n\
        ins %[x_bits], $zero, 31, 1  # 符号をクリア                     \n\
        sltu $t0, %[x_bits], $t0                                        \n\
        beqz $t0, 9f                                                    \n\
        srl $t0, %[x_bits], 23  # 0を検出する（2倍できないため特殊処理）\n\
        beqz $t0, 8f                                                    \n\
        li $t0, 0x00800000      # 指数に1を足し、2倍にする              \n\
        addu $t0, %[x_bits], $t0                                        \n\
        mtc1 $t0, %[result]                                             \n\
        nop                                                             \n\
        trunc.w.s %[temp], %[result]                                    \n\
        mfc1 $t0, %[temp]                                               \n\
        cvt.s.w %[temp], %[temp]                                        \n\
        sub.s %[x_frac], %[result], %[temp]                             \n\
        # 正弦用に、(x*2)/(90*2)の商を$t1に、余りを$t0に格納。余弦用には\n\
        # $t5, $t4をそれぞれ使う                                        \n\
        sltiu $t1, $t0, 90*2                                            \n\
        li $t2, 90*2                                                    \n\
        bnez $t1, 1f                                                    \n\
        li $t1, 0                                                       \n\
        divu $0, $t0, $t2                                               \n\
        mfhi $t0                                                        \n\
        mflo $t1                                                        \n\
     1: move $t4, $t0                                                   \n\
        addiu $t5, $t1, 1                                               \n\
        # xがちょうど0.5の倍数の場合、補間処理を省く                    \n\
        mfc1 $t2, %[x_frac]                                             \n\
        andi $t3, $t1, 1                                                \n\
        srl $t1, $t1, 1                                                 \n\
        andi $t7, $t5, 1                                                \n\
        srl $t5, $t5, 1                                                 \n\
        bnez $t2, 2f                                                    \n\
        xor $t1, $t1, %[sign]                                           \n\
        li $t6, 90*2                                                    \n\
        subu $t2, $t6, $t0                                              \n\
        movn $t0, $t2, $t3                                              \n\
        subu $t6, $t6, $t4                                              \n\
        movn $t4, $t6, $t7                                              \n\
        sll $t0, $t0, 2                                                 \n\
        addu $t3, %[table], $t0                                         \n\
        sll $t4, $t4, 2                                                 \n\
        addu $t7, %[table], $t4                                         \n\
        lw $t0, 0($t3)                                                  \n\
        lw $t4, 0($t7)                                                  \n\
        sll $t1, $t1, 31                                                \n\
        xor $t0, $t0, $t1                                               \n\
        sll $t5, $t5, 31                                                \n\
        xor $t4, $t4, $t5                                               \n\
        mtc1 $t0, $f0                                                   \n\
        jr $ra                                                          \n\
        mtc1 $t4, $f1                                                   \n\
     2: # 補間処理                                                      \n\
        li $t6, 90*2-1                                                  \n\
        subu $t2, $t6, $t0                                              \n\
        movn $t0, $t2, $t3                                              \n\
        subu $t6, $t6, $t4                                              \n\
        movn $t4, $t6, $t7                                              \n\
        mov.s %[cos_frac], %[x_frac]                                    \n\
        bnezl $t3, 3f                                                   \n\
        sub.s %[x_frac], %[one], %[x_frac]                              \n\
     3: bnezl $t7, 4f                                                   \n\
        sub.s %[cos_frac], %[one], %[cos_frac]                          \n\
     4: sll $t0, $t0, 2                                                 \n\
        addu $t3, %[table], $t0                                         \n\
        lwc1 %[result], 0($t3)                                          \n\
        lwc1 %[temp], 4($t3)                                            \n\
        sub.s %[temp], %[temp], %[result]                               \n\
        mul.s %[temp], %[temp], %[x_frac]                               \n\
        add.s %[result], %[result], %[temp]                             \n\
        mfc1 $t0, %[result]                                             \n\
        sll $t4, $t4, 2                                                 \n\
        addu $t7, %[table], $t4                                         \n\
        lwc1 %[result], 0($t7)                                          \n\
        lwc1 %[temp], 4($t7)                                            \n\
        sub.s %[temp], %[temp], %[result]                               \n\
        mul.s %[temp], %[temp], %[cos_frac]                             \n\
        add.s %[result], %[result], %[temp]                             \n\
        mfc1 $t4, %[result]                                             \n\
        sll $t1, $t1, 31                                                \n\
        xor $t0, $t0, $t1                                               \n\
        sll $t5, $t5, 31                                                \n\
        xor $t4, $t4, $t5                                               \n\
        mtc1 $t0, $f0                                                   \n\
        jr $ra                                                          \n\
        mtc1 $t4, $f1                                                   \n\
     8: # 0は特殊処理（2倍しても0）                                     \n\
        mtc1 $zero, $f0                                                 \n\
        jr $ra                                                          \n\
        mov.s $f1, %[one]                                               \n\
     9: # オーバフローの場合はNaNを返す                                 \n\
        li $t0, 0x7FFFFFFF                                              \n\
        mtc1 $t0, $f0                                                   \n\
        jr $ra                                                          \n\
        mtc1 $t0, $f1                                                   \n\
        .set pop"
        : [result] "=&f" (result), [temp] "=&f" (temp),
          [x_frac] "=&f" (x_frac), [cos_frac] "=&f" (cos_frac),
          [x_bits] "=&r" (x_bits), [sign] "=&r" (sign)
        : [x] "f" (x), [table] "r" (dsincosf_table), [one] "f" (1.0f)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "$f0", "$f1"
    );
}

/*************************************************************************/

/**
 * dtanf:  度単位で表した角度の正接を単精度で計算する。処理は概ねdcosf()同
 * 様だが、関数の周期がsin、cosの半分の180度なので、0〜45度の範囲でテーブル
 * から計算して、必要に応じて逆数・符号反転を行う。90度の場合、0除算例外を
 * 発生させないように無限値を手動で生成して返す。
 *
 * 【引　数】x: 角度（度単位）
 * 【戻り値】当該三角関数の値
 * 【前条件】|x| < 2^29
 */
CONST_FUNCTION float dtanf(const float x)
{
    float result, temp, x_frac;
    uint32_t x_bits;
    asm(".set push; .set noreorder\n\
        mfc1 %[x_bits], %[x]                                            \n\
        li $t0, 0x4E000000      # 2^29（これ以上はオーバフロー発生）    \n\
        ins %[x_bits], $zero, 31, 1  # 符号をクリア                     \n\
        sltu $t0, %[x_bits], $t0                                        \n\
        beqz $t0, 9f                                                    \n\
        srl $t0, %[x_bits], 23  # 0を検出する（4倍できないため特殊処理）\n\
        beqz $t0, 8f                                                    \n\
        li $t0, 0x01000000      # 指数に2を足し、4倍にする              \n\
        addu $t0, %[x_bits], $t0                                        \n\
        mtc1 $t0, %[result]                                             \n\
        nop                                                             \n\
        trunc.w.s %[temp], %[result]                                    \n\
        mfc1 $t0, %[temp]                                               \n\
        cvt.s.w %[temp], %[temp]                                        \n\
        sltiu $t1, $t0, 180*4                                           \n\
        bnez $t1, 1f                                                    \n\
        sub.s %[x_frac], %[result], %[temp]                             \n\
        divu $0, $t0, %[modval]                                         \n\
        mfhi $t0                                                        \n\
     1: # ちょうど90度の場合は無限を返す                                \n\
        mfc1 $t2, %[x_frac]                                             \n\
        li $t1, 90*4                                                    \n\
        subu $t1, $t1, $t0                                              \n\
        or $t2, $t1, $t2                                                \n\
        bnez $t2, 2f                                                    \n\
        li $t2, 0x7F800000      # ＋∞                                  \n\
        jr $ra                                                          \n\
        mtc1 $t2, $f0                                                   \n\
     2: # 90〜180度は90〜0度として計算し、符号を反転                    \n\
        bgtz $t1, 3f                                                    \n\
        li $t3, 45*4                                                    \n\
        addiu $t0, $t1, 90*4-1                                          \n\
        not %[x_bits], %[x_bits]                                        \n\
        sub.s %[x_frac], %[one], %[x_frac]                              \n\
     3: # 45〜90度は45〜0度の逆数として計算                             \n\
        subu $t2, $t3, $t0                                              \n\
        bgtz $t2, 4f                                                    \n\
        sll $t0, $t0, 2                                                 \n\
        addiu $t0, $t2, 45*4-1                                          \n\
        sub.s %[x_frac], %[one], %[x_frac]                              \n\
        sll $t0, $t0, 2                                                 \n\
     4: # 実際に計算する                                                \n\
        addu $t1, %[table], $t0                                         \n\
        lwc1 %[result], 0($t1)                                          \n\
        lwc1 %[temp], 4($t1)                                            \n\
        sub.s %[temp], %[temp], %[result]                               \n\
        mul.s %[temp], %[temp], %[x_frac]                               \n\
        add.s %[result], %[result], %[temp]                             \n\
        # 45〜90度は逆数を取る                                          \n\
        blezl $t2, 5f                                                   \n\
        div.s %[result], %[one], %[result]                              \n\
     5: # 90〜180度または負数の場合は符号を反転する                     \n\
        bltzl %[x_bits], 6f                                             \n\
        neg.s %[result], %[result]                                      \n\
     6: jr $ra                                                          \n\
        nop                                                             \n\
     8: # 0は特殊処理（4倍しても0）                                     \n\
        jr $ra                                                          \n\
        mtc1 $zero, $f0                                                 \n\
     9: # オーバフローの場合はNaNを返す                                 \n\
        li $t0, 0x7FFFFFFF                                              \n\
        mtc1 $t0, %[result]                                             \n\
        .set pop"
        : [result] "=&f" (result), [temp] "=&f" (temp),
          [x_frac] "=&f" (x_frac), [x_bits] "=&r" (x_bits)
        : [x] "f" (x), [table] "r" (dtanf_table), [one] "f" (1.0f),
          [modval] "r" (180*4)
        : "t0", "t1", "t2", "t3"
    );
    return result;
}

/*************************************************************************/

#endif  // USE_TRIG_TABLES

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
