/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/file-read.c: Low-level file reading logic for the PSP.
 */

/*
 * ファイルからのデータ読み込みにおいては、即時読み込みだけでなく、先読みや
 * ストリーミングもあり、当然それぞれの優先度が違う。また複数の読み込み要求
 * が同時に発生する場合もあり、何らかの制御をしないとデータの到着遅れが発生
 * しやすい。このため、すべての読み込み要求を本ソースファイルで一元管理する。
 *
 * psp_file_read_init()で初期化したあと、ファイルからデータを読み込む場合、
 * ファイルデスクリプタや読み込む部分、格納するメモリ領域、読み込み開始期限
 * を指定してpsp_file_read_submit()で要求を提出し、要求識別子を取得する。こ
 * の時点からデータの読み込みが開始可能となるが、実際の開始は他の要求等によ
 * り左右される。psp_file_read_wait()で読み込み完了を待つことができるほか、
 * psp_file_read_check()で完了の有無も確認できる。なお、完了を確認しても、
 * 一度はpsp_file_read_wait()で読み込み結果を取得しなければならない（Unixに
 * おける子プロセス終了時のwait()と同様）。
 *
 * 読み込み要求には、データがすぐに必要となる「即時要求」と、特定の時間まで
 * に読み込みを開始すればよい「期限付き要求」がある。基本的に即時要求が優先
 * されるが、期限付き要求の開始期限が到来して読み込みが完了していなければ、
 * その要求を最優先して読み込む。（期限指定で「完了期限」ではなく「開始期限」
 * を指定するのは、物理媒体からの読み込み速度が不安定だったり、途中で新たな
 * 要求が入ったりするので、読み込み完了時期を特定するのは難しいため。）
 *
 * 内部的には、システムコールのオーバヘッドが問題にならない程度の大きさに読
 * み込み要求を分割して実行する（BLOCKSIZE定数で分割サイズを設定）。ループ
 * 毎に、以下のように処理する。
 * 　・新しい要求があれば、即時要求リストまたは期限付き要求リストに挿入する。
 * 　・未完了の期限付き要求のうち、すでに期限が過ぎているものがあれば、その
 * 　　要求を即時に完了させる。この場合、分割せずに一括読み込みを行う。また
 * 　　読み込み終了後は一定時間、即時要求を受け付けない（以下参照）。
 * 　・即時要求の有無を確認する。
 * 　　→ 即時要求がある場合、最も古い即時要求を読み込み対象とする。
 * 　　→ 即時要求がなければ、期限の最も早い限付き要求を読み込み対象とする。
 * 　　→ 即時要求も期限付き要求もない場合、スレッドを停止し、再開後ループ
 * 　　　 をやり直す。
 * 　・読み込み対象要求の次の1ブロックを読み込む。
 *
 * 期限付き要求の読み込み開始期限が過ぎたために一括読み込みを行った場合、そ
 * の後も期限付き要求が続く可能性が高い（実際、ストリーミング音声再生機能で
 * 一つの読み込みが終了すると、バッファが空いていれば次の読み込み要求をすぐ
 * に提出する）。しかしスレッドの実行順や関数の呼び出しタイミングによって、
 * 読み込みスレッドが次のループを開始した後に要求が提出される可能性がある。
 * この場合、期限付き要求のファイルと、即時要求のファイルから交互に読み込み
 * を行うことになり、特にディスクなどランダムアクセスに弱いメディアの場合、
 * ヘッドのスラッシング（thrashing：ヘッドが頻繁に動いて、読み込み速度が極
 * 端に減少する現象）が発生してしまう危険性がある。
 *
 * この問題を回避するため、読み込み開始期限の到来によって読み込みを実行した
 * 場合、一定時間（PRIORITY_TIME定数で設定）、同様に期限が過ぎている期限付き
 * 読み込み以外の要求を一切無視する（コメントでは「期限付き専用モード」と呼
 * ぶ）。充分なデータ量が読み込まれて、新たな要求が提出されなくなると、通常
 * の処理に戻る。
 *
 * 注：要求待ち（psp_file_read_wait()）は、同一の要求に対して複数のスレッド
 * 　　から行うことはできない。
 */

#include "../common.h"

#include "psplocal.h"
#include "file-read.h"

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* 読み込みブロックサイズ */
#define BLOCKSIZE       65536

/* 同時に受け付けられる読み込み要求の最大数 */
#define MAX_REQUESTS    210  // sysdep.hのMAX_ASYNC_READSに若干の余裕を持たせる

/* 期限付き専用モードの持続時間（μ秒） */
#define PRIORITY_TIME   50000

/* 期限付き専用モードのループ間隔（μ秒） */
#define PRIORITY_DELAY  10000

/* イベントフラグ用ビット定義 */
#define EVENT_BIT_FINISHED  1   // 読み込み完了

/* psp_file_read_submit()のmutex確保タイムアウト（μ秒）。高優先度のセーブ
 * スレッドに割り込まれてもエラーにならないよう、高めに設定されているが、
 * 通常は数μ秒以内に解放される。 */
#define SUBMIT_MUTEX_TIMEOUT  3000000

/*************************************************************************/

static struct request {
    int16_t next;       // 優先から見て、次の要求（-1＝リスト終端）
    uint8_t inuse,      // 使用中フラグ
            new,        // 新規提出フラグ（子スレッドでリストに追加される）
            timed,      // deadlineフィールド有効フラグ
            finished,   // 終了フラグ兼mutex
                        // 　0：メインスレッドからのアクセス禁止（inuse参照、
                        // 　　　abort設定を除く）
                        // 　1：読み込みスレッドからのアクセス禁止
            abort;      // 中止フラグ
    SceUID event_flag;  // 同期用イベントフラグ
    int fd;             // ファイルデスクリプタ
    uint32_t start;     // 次ブロックの読み込み位置
    uint32_t len;       // 残り読み込みバイト数
    uint8_t *buf;       // 次ブロックの格納アドレス
    int32_t deadline;   // 読み込み開始期限
    SceUID waiter;      // この要求の完了を待っているスレッドのID（0＝なし）
    int32_t res;        // 読み込み結果（バイト数またはエラーコード）
} requests[MAX_REQUESTS];

/* 即時要求・期限付き要求リストの最初の要求インデックス（-1＝要求は一つもな
 * い）。即時要求リストは提出順、期限付き要求リストは期限順に保たれる */
static int16_t first_immediate, first_timed;

/* 読み込みスレッドハンドル */
static SceUID file_read_thread_handle;

/* psp_file_read_submit()用mutex */
static SceUID file_read_submit_mutex;

/*************************************************************************/

/* ローカル関数宣言 */
static int file_read_thread(SceSize args, void *argp);
static int handle_request(struct request *req, int all);

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * psp_file_read_init:  ファイル読み込み管理機能を初期化する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
int psp_file_read_init(void)
{
    first_immediate = -1;
    first_timed = -1;

    file_read_submit_mutex = sceKernelCreateSema("FileReadSubmitMutex",
                                                 0, 1, 1, NULL);
    if (file_read_submit_mutex < 0) {
        DMSG("Error creating submit mutex: %s",
             psp_strerror(file_read_submit_mutex));
        file_read_submit_mutex = 0;
        goto error_return;
    }

    mem_clear(requests, sizeof(requests));
    unsigned int i;
    for (i = 0; i < lenof(requests); i++) {
        char namebuf[28];
        snprintf(namebuf, sizeof(namebuf), "FileReadFlag%u", i);
        requests[i].event_flag = sceKernelCreateEventFlag(namebuf, 0, 0, 0);
        if (requests[i].event_flag < 0) {
            DMSG("Error creating event flag %u: %s", i,
                 psp_strerror(requests[i].event_flag));
            requests[i].event_flag = 0;
            goto error_free_event_flags;
        }
    }

    file_read_thread_handle = psp_start_thread(
        "FileReadThread", file_read_thread, THREADPRI_FILEIO, 0x1000, 0, NULL
    );
    if (file_read_thread_handle < 0) {
        goto error_free_event_flags;
    }

    return 1;

  error_free_event_flags:
    for (i = 0; i < lenof(requests); i++) {
        if (requests[i].event_flag) {
            sceKernelDeleteEventFlag(requests[i].event_flag);
            requests[i].event_flag = 0;
        }
    }
    sceKernelDeleteEventFlag(file_read_submit_mutex);
    file_read_submit_mutex = 0;
  error_return:
    return 0;
}

/*************************************************************************/

/**
 * psp_file_read_submit:  ファイル読み込み要求を提出する。
 *
 * 【引　数】        fd: ファイルデスクリプタ
 * 　　　　　     start: 読み込み開始位置（ファイル先頭からのバイトオフセット）
 * 　　　　　       len: 読み込むバイト数
 * 　　　　　       buf: 読み込んだデータを格納するバッファ
 * 　　　　　       pri: 読み込み優先度（FILE_READ_PRI_*定数のいずれか）
 * 　　　　　     timed: 0以外＝期限付き要求、0＝即時要求
 * 　　　　　time_limit: 読み込み開始期限（μ秒単位、呼び出し時点から。
 * 　　　　　            　timed==0の場合は無視）
 * 【戻り値】要求識別子（0＝エラー）
 */
int psp_file_read_submit(int fd, uint32_t start, uint32_t len, void *buf,
                         int timed, int32_t time_limit)
{
    if (UNLIKELY(fd < 0)
     || UNLIKELY(buf == NULL)
     || UNLIKELY(timed && time_limit < 0)
    ) {
        DMSG("Invalid parameters: %d 0x%08X %u %p %d %d",
             fd, start, len, buf, timed, time_limit);
        return 0;
    }

    /* まず期限を計算する */
    int32_t deadline = sceKernelGetSystemTimeLow() + time_limit;

    /* mutexを確保する */
    unsigned int timeout = SUBMIT_MUTEX_TIMEOUT;
    int res = sceKernelWaitSema(file_read_submit_mutex, 1, &timeout);
    if (res != 0) {
        DMSG("Failed to lock submit mutex: %s", psp_strerror(res));
        return 0;
    }

    /* 空きエントリーを探す */
    int index;
    for (index = 0; index < lenof(requests); index++) {
        if (!requests[index].inuse) {
            break;
        }
    }
    if (index >= lenof(requests)) {
        DMSG("No open request slots for: %d 0x%08X %u %p %d %d",
             fd, start, len, buf, timed, time_limit);
        sceKernelSignalSema(file_read_submit_mutex, 1);
        return 0;
    }

    /* エントリーを確保して、mutexを解放する（inuseフラグさえ立っていれば
     * 別スレッドに上書きされてしまう心配はない） */
    requests[index].inuse = 1;
    sceKernelSignalSema(file_read_submit_mutex, 1);

    /* 要求データを格納する */
    requests[index].new      = 1;
    requests[index].timed    = (timed != 0);
    requests[index].finished = 0;
    requests[index].abort    = 0;
    requests[index].fd       = fd;
    requests[index].start    = start;
    requests[index].len      = len;
    requests[index].buf      = buf;
    requests[index].deadline = deadline;
    requests[index].waiter   = 0;
    sceKernelClearEventFlag(requests[index].event_flag, ~0);

    /* 読み込みスレッドが停止中であれば、再開する */
    sceKernelWakeupThread(file_read_thread_handle);

    /* 成功。添え字+1を識別子として返す */
    return index+1;
}

/*************************************************************************/

/**
 * psp_file_read_check:  読み込みが完了しているかどうかを返す。
 *
 * 【引　数】id: 要求識別子
 * 【戻り値】正数＝読み込み完了、0＝読み込み中、負数＝要求識別子不正
 */
int psp_file_read_check(int id)
{
    const int index = id-1;
    if (index < 0 || index >= lenof(requests) || !requests[index].inuse) {
        return -1;
    }
    return requests[index].finished;
}

/*************************************************************************/

/**
 * psp_file_read_wait:  読み込みが完了するのを待って、結果を返す。
 *
 * 【引　数】id: 要求識別子
 * 【戻り値】0以上＝成功（読み込んだバイト数）、負数＝エラー発生
 */
int psp_file_read_wait(int id)
{
    const int index = id-1;
    if (index < 0 || index >= lenof(requests) || !requests[index].inuse) {
        return PSP_EINVAL;
    }
    if (requests[index].waiter) {
        DMSG("Two threads tried to sleep on request %d! old=%08X new=%08X",
             id, requests[index].waiter, sceKernelGetThreadId());
        return SCE_KERNEL_ERROR_ASYNC_BUSY;
    }
    requests[index].waiter = sceKernelGetThreadId();
    sceKernelWaitEventFlag(requests[index].event_flag, EVENT_BIT_FINISHED,
                           PSP_EVENT_WAITCLEAR, NULL, NULL);
    int32_t retval = requests[index].res;
    requests[index].inuse = 0;
    return retval;
}

/*************************************************************************/

/**
 * psp_file_read_abort:  読み込みを中止する。指定された読み込み要求がすでに
 * 完了している場合、何もしない。
 *
 * 【引　数】id: 要求識別子
 * 【戻り値】0以外＝成功、0＝失敗（識別子不正）
 */
int psp_file_read_abort(int id)
{
    const int index = id-1;
    if (index < 0 || index >= lenof(requests) || !requests[index].inuse) {
        return 0;
    }
    requests[index].abort = 1;
    return 1;
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * file_read_thread:  ファイル読み込みスレッド。
 *
 * 【引　数】args: 引数サイズ
 * 　　　　　argp: 引数ポインタ
 * 【戻り値】スレッド終了コード
 */
static int file_read_thread(SceSize args, void *argp)
{
    /* 期限付き専用モードのフラグとタイムアウト時刻 */
    int priority_mode = 0;
    int32_t priority_timeout = 0;

    while (LIKELY(!psp_exit)) {

        /* 新規提出の要求を各リストに追加し、中止フラグが立っている要求を
         * 中止する */
        int i;
        for (i = 0; i < lenof(requests); i++) {
            if (UNLIKELY(requests[i].new)) {
                int16_t *nextptr;
                if (requests[i].timed) {
                    const int32_t this_deadline = requests[i].deadline;
                    nextptr = &first_timed;
                    while (*nextptr >= 0) {
                        const int32_t diff =
                            requests[*nextptr].deadline - this_deadline;
                        if (diff > 0) {  // つまりnext.deadline > this.deadline
                            break;
                        }
                        nextptr = &requests[*nextptr].next;
                    }
                } else {
                    nextptr = &first_immediate;
                    while (*nextptr >= 0) {
                        nextptr = &requests[*nextptr].next;
                    }
                }
                requests[i].next = *nextptr;
                *nextptr = i;
                requests[i].new = 0;
                requests[i].res = 0;
            }  // if (UNLIKELY(requests[i].new))
            if (UNLIKELY(requests[i].abort)) {
                handle_request(&requests[i], 1);
            }
        }

        /* 現在時刻（符号付き変数） */
        const int32_t now = sceKernelGetSystemTimeLow();

        /* 期限付き要求で期限が到来したものがあれば、一括で読み込む */
        while (first_timed >= 0 && (requests[first_timed].deadline - now) < 0){
            handle_request(&requests[first_timed], 1);
            first_timed = requests[first_timed].next;
            priority_timeout = sceKernelGetSystemTimeLow() + PRIORITY_TIME;
            priority_mode = 1;
        }

        /* 期限切れによる期限付き要求専用モードのタイムアウトチェック
         * （アルゴリズムドキュメントを参照） */
        if (priority_mode) {
            if (priority_timeout - now > 0) {
                sceKernelDelayThread(PRIORITY_DELAY);
            } else {
                priority_mode = 0;
            }
            continue;
        }

        /* ブロック読み込み対象を選んで、1ブロックを読み込む */
        if (first_immediate >= 0) {
            /* 即時要求があるので優先 */
            if (handle_request(&requests[first_immediate], 0)) {
                first_immediate = requests[first_immediate].next;
            }
        } else if (first_timed >= 0) {
            /* 即時要求がない */
            if (handle_request(&requests[first_timed], 0)) {
                first_timed = requests[first_timed].next;
            }
        } else {
            /* 何の要求もないのでスレッドを停止する */
            sceKernelSleepThread();
        }

    }  // while (!psp_exit)
    return 0;
}

/*************************************************************************/

/**
 * handle_request:  一つの要求の読み込みまたは中止処理を行う。
 *
 * 【引　数】req: 処理する要求情報へのポインタ
 * 　　　　　all: 0以外＝一括読み込み、0＝1ブロックだけ
 * 【戻り値】0以外＝要求処理完了（req->res、req->finished設定済み）、0＝未完了
 */
static int handle_request(struct request *req, int all)
{
    if (req->abort) {
        req->res = PSP_ECANCELED;
        goto finish_request;
    }

    uint32_t toread = req->len;
    if (!all) {
        toread = ubound(toread, BLOCKSIZE);
    }
    if (UNLIKELY(toread == 0)) {
        goto finish_request;
    }
    int32_t res;
    res = sceIoLseek(req->fd, req->start, PSP_SEEK_SET);
    if (UNLIKELY(res != req->start)) {
        DMSG("Failed seeking to position %d in file %d: %s",
             req->start, req->fd, psp_strerror(res));
        req->res = res;
        goto finish_request;
    }
    res = sceIoRead(req->fd, req->buf, toread);
    if (UNLIKELY(res != toread)) {
        if (res < 0) {
            DMSG("Failed reading %d from position %d in file %d: %s",
                 toread, req->start, req->fd, psp_strerror(res));
            req->res = res;
        } else {  // ファイル終端
            req->res += res;
        }
        goto finish_request;
    }
    req->start += toread;
    req->len   -= toread;
    req->buf   += toread;
    req->res   += toread;
    if (req->len == 0) {
        goto finish_request;
    }
    return 0;

  finish_request:;
    /* 要求の処理が完了したので完了フラグを立て、待っているスレッドがあれば
     * 起こす */
    req->finished = 1;
    sceKernelSetEventFlag(req->event_flag, EVENT_BIT_FINISHED);
    return 1;
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
