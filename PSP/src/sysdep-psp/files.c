/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/files.c: PSP data file access interface.
 */

#include "../common.h"
#include "../sysdep.h"

#include "psplocal.h"
#include "file-read.h"

/*
 * PSPにおけるファイルアクセスの注意事項
 *
 * ・全般的にはマルチスレッド対応であり、複数のスレッドからファイルのオープ
 * 　ン、読み込み、クローズを同時に行うことができる。
 *
 * ・同一ファイルハンドルを複数のスレッドから同時に操作してはならない。ただ、
 * 　一つのスレッドでオープンし、別スレッドで読み込みを行うことは、呼び出し
 * 　側が排他処理をするなどで操作が重ならないようにすれば可能。
 *
 * ・読み込みキャッシュがなく、かつ全ての読み込みは一定のブロックサイズ
 * 　（file-read.cのBLOCKSIZE定数）で行うため、小さい読み込み要求のオーバー
 * 　ヘッドはかなり大きい。基本的には、ファイルのデータを全て読み込んだ上、
 * 　メモリ内で必要な解析を行うことが望ましい。
 */

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* データファイルの基準パス名 */
static char basepath[256];

/* ファイル管理構造体 */
struct SysFile_ {
    int inuse;          // 使用中フラグ（0＝未使用）
    char path[256];     // ファイルのパス名（システム停止後の回復用）
    int fd;             // sceIo*()システムコールで使うファイルデスクリプタ
    int32_t filesize;   // ファイルのサイズ（書き込み非実装のため一定）
    int32_t filepos;    // 読み込み位置（先読みの関係でシステムの位置とは違う）
};

/* ファイル管理テーブルと最大同時オープン数。メモリの動的確保によるオープン
 * 失敗を回避するため、管理構造体を静的に定義しておく */
#define MAX_FILES  32
static SysFile filetable[MAX_FILES];

/* 各ファイルハンドルのmutex。mem_clear(fp)などでうっかり消さないように、
 * filetable[]とは別の配列で管理する */
static SceUID file_mutex[MAX_FILES];

/* 非同期オープン・読み込み管理データ */
struct {
    SysFile *fp;    // 関連ファイルポインタ（NULL＝エントリー未使用）
    enum {ASYNC_OPEN, ASYNC_READ} type;  // 要求の種類
    int request;    // file-read用非同期読み込み要求識別子またはOPEN_REQUEST
                    // ※fp!=NULL、request==0の場合はwait待ち
    int32_t res;    // 非同期読み込み結果
} async_info[MAX_ASYNC_READS];
#define OPEN_REQUEST  (-1)  // 非同期オープンの場合に使うrequest値


/* ローカル関数宣言 */

static void lock_file(const SysFile *fp);
static void unlock_file(const SysFile *fp);
static SysFile *alloc_file(void);

static int alloc_async(SysFile *fp);
static void free_async(int index);
static int check_async_request(int index, int wait);

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * sys_file_open:  指定されたファイルパス名を基に、ファイルを読み込みモード
 * で開く。ファイルパス名は「/」をディレクトリ区別文字とし、大文字・小文字を
 * 区別しない。該当するファイルが複数ある場合、どれか1つを開く（実際に開かれ
 * るファイルは不定）。
 *
 * 【引　数】file: ファイルパス名
 * 【戻り値】SysFileポインタ（オープンできない場合はNULL）
 */
SysFile *sys_file_open(const char *file)
{
    if (UNLIKELY(!file)) {
        DMSG("file == NULL");
        psp_errno = PSP_EINVAL;
        return NULL;
    }

    /* 空き管理構造体（ファイルハンドル）を確保する */
    SysFile *fp = alloc_file();
    if (UNLIKELY(!fp)) {
        psp_errno = PSP_EMFILE;
        return NULL;
    }
    mem_clear(fp, sizeof(*fp));
    fp->inuse = 1;

    /* パス名の長さをチェックし、ファイル構造体にコピーする */
    int len;
    if (strchr(file, ':')) {
        len = snprintf(fp->path, sizeof(fp->path), "%s", file);
    } else {
        len = snprintf(fp->path, sizeof(fp->path), "%s/%s", basepath, file);
    }
    if (UNLIKELY(len >= sizeof(fp->path))) {
        psp_errno = PSP_ENAMETOOLONG;
        goto error_unlock;
    }

    /* ファイルをオープンしてみる */
    int fd = sceIoOpen(fp->path, PSP_O_RDONLY, 0);
    if (UNLIKELY(fd < 0)) {
        psp_errno = fd;
        goto error_unlock;
    }

    /* ファイル構造体を初期化して返す */
    fp->fd       = fd;
    fp->filepos  = 0;
    fp->filesize = sceIoLseek(fd, 0, PSP_SEEK_END);
    if (UNLIKELY(fp->filesize < 0)) {
        DMSG("Error getting file size for %s: %s",
             file, psp_strerror(fp->filesize));
        fp->filesize = 0;
    }

    unlock_file(fp);
    return fp;

  error_unlock:
    fp->inuse = 0;
    unlock_file(fp);
    return NULL;
}

/*************************************************************************/

/**
 * sys_file_dup:  ファイルハンドルを複製する。
 *
 * 【引　数】fp: ファイルポインタ
 * 【戻り値】複製したファイルハンドルへのポインタ（失敗した場合はNULL）
 */
SysFile *sys_file_dup(SysFile *fp)
{
    if (UNLIKELY(!fp)) {
        DMSG("fp == NULL");
        return NULL;
    }

    SysFile *newfp = alloc_file();
    if (UNLIKELY(!newfp)) {
        psp_errno = PSP_EMFILE;
        return NULL;
    }

    lock_file(fp);
    memcpy(newfp, fp, sizeof(*fp));
    unlock_file(fp);

    int newfd = sceIoOpen(newfp->path, PSP_O_RDONLY, 0);
    if (newfd < 0) {
        DMSG("Failed to reopen %s: %s", fp->path, psp_strerror(newfd));
        psp_errno = newfd;
        newfp->inuse = 0;
        unlock_file(newfp);
        return NULL;
    }
    newfp->fd = newfd;

    unlock_file(newfp);
    return newfp;
}

/*************************************************************************/

/**
 * sys_file_size:  ファイルのサイズを取得する。
 *
 * 【引　数】fp: ファイルポインタ
 * 【戻り値】ファイルのサイズ（バイト）
 * 【注　意】有効なファイルポインタを渡された場合、この関数は失敗しない。
 */
uint32_t sys_file_size(SysFile *fp)
{
    if (UNLIKELY(!fp)) {
        DMSG("fp == NULL");
        return 0;
    }
    return fp->filesize;
}

/*************************************************************************/

/**
 * sys_file_seek:  ファイルの読み込み位置を変更し、新しい位置を返す。
 *
 * 【引　数】 fp: ファイルポインタ
 * 　　　　　pos: 新しい読み込み位置
 * 　　　　　how: 設定方法（FILE_SEEK_*）
 * 【戻り値】新しい読み込み位置（ファイルの先頭からのバイト数）。エラーが
 * 　　　　　発生した場合は-1を返す
 */
int32_t sys_file_seek(SysFile *fp, int32_t pos, int how)
{
    if (UNLIKELY(!fp)) {
        DMSG("fp == NULL");
        psp_errno = PSP_EINVAL;
        goto error_return;
    }
    lock_file(fp);

    if (how == FILE_SEEK_SET) {
        fp->filepos = pos;
    } else if (how == FILE_SEEK_CUR) {
        fp->filepos += pos;
    } else if (how == FILE_SEEK_END) {
        fp->filepos = fp->filesize + pos;
    } else {
        DMSG("Invalid how: %d", how);
        psp_errno = PSP_EINVAL;
        goto error_unlock_file;
    }

    if (fp->filepos < 0) {
        fp->filepos = 0;
    } else if (fp->filepos > fp->filesize) {
        fp->filepos = fp->filesize;
    }

    unlock_file(fp);
    return fp->filepos;

  error_unlock_file:
    unlock_file(fp);
  error_return:
    return -1;
}

/*************************************************************************/

/**
 * sys_file_read:  ファイルからデータを読み込む。
 *
 * 【引　数】 fp: ファイルポインタ
 * 　　　　　buf: データを読み込むバッファ
 * 　　　　　len: 読み込むデータ量（バイト）
 * 【戻り値】読み込んだバイト数。読み込み中にエラーが発生した場合は-1を返す
 * 　　　　　（EOFはエラーとはみなさない）
 */
int32_t sys_file_read(SysFile *fp, void *buf, int32_t len)
{
    if (UNLIKELY(!fp)) {
        DMSG("fp == NULL");
        psp_errno = PSP_EINVAL;
        goto error_return;
    }
    if (UNLIKELY(!buf)) {
        DMSG("buf == NULL");
        psp_errno = PSP_EINVAL;
        goto error_return;
    }
    if (UNLIKELY(len < 0)) {
        DMSG("len (%d) < 0", len);
        psp_errno = PSP_EINVAL;
        goto error_return;
    }
    lock_file(fp);

    if (len == 0) {
        unlock_file(fp);
        return 0;
    }

    int request = psp_file_read_submit(fp->fd, fp->filepos, len, buf, 0, 0);
    if (UNLIKELY(!request)) {
        DMSG("(%d,%p,%d): Read request submission failed", fp->fd, buf, len);
        psp_errno = PSP_EIO;
        goto error_unlock_file;
    }
    int32_t res = psp_file_read_wait(request);
    if (UNLIKELY(res < 0)) {
        DMSG("Read request failed");
        psp_errno = res;
        goto error_unlock_file;
    }

    fp->filepos += res;
    unlock_file(fp);
    return res;

  error_unlock_file:
    unlock_file(fp);
  error_return:
    return -1;
}

/*************************************************************************/

/**
 * sys_file_read_async:  ファイルからデータの非同期読み込みを開始し、すぐに
 * 戻る。sys_file_wait_async()を呼び出すまで、バッファを参照してはならない。
 * 成功後、ファイルポインタの読み込み位置は不定となる。
 *
 * 非同期読み込みをサポートしない環境では、この関数でバッファとデータ量を
 * 記録し、sys_file_wait_async()で読み込みを実行する。
 *
 * 【引　数】     fp: ファイルポインタ
 * 　　　　　    buf: データを読み込むバッファ
 * 　　　　　    len: 読み込むデータ量（バイト）
 * 　　　　　filepos: 読み込み開始位置（バイト）
 * 【戻り値】成功の場合は読み込み要求識別子（0以外）、失敗の場合は0
 */
int sys_file_read_async(SysFile *fp, void *buf, int32_t len, int32_t filepos)
{
    if (UNLIKELY(!fp)
     || UNLIKELY(!buf)
     || UNLIKELY(len < 0)
     || UNLIKELY(filepos < 0)
    ) {
        DMSG("Invalid parameters: %p %p %d %d", fp, buf, len, filepos);
        psp_errno = PSP_EINVAL;
        goto error_return;
    }
    lock_file(fp);

    const int index = alloc_async(fp);
    if (UNLIKELY(index < 0)) {
        DMSG("No free async blocks");
        psp_errno = SCE_KERNEL_ERROR_ASYNC_BUSY;
        goto error_unlock_file;
    }

    async_info[index].type = ASYNC_READ;
    async_info[index].request =
        psp_file_read_submit(fp->fd, filepos, len, buf, 0, 0);
    if (UNLIKELY(!async_info[index].request)) {
        DMSG("(%d,%p,%d): Read request submission failed", fp->fd, buf, len);
        psp_errno = PSP_EIO;
        goto error_free_async;
    }
    async_info[index].res = -1;

    unlock_file(fp);
    return index+1;

  error_free_async:
    free_async(index);
  error_unlock_file:
    unlock_file(fp);
  error_return:
    return 0;
}

/*************************************************************************/

/**
 * sys_file_poll_async:  ファイルからの非同期読み込みが終了しているかどうか
 * を返す。request==0の場合、1つでも読み込み中の要求があれば「読み込む中」
 * を返す。
 *
 * 非同期読み込みをサポートしない環境では常に0を返す。
 *
 * 【引　数】request: 読み込み要求識別子（0＝全要求をチェック）
 * 【戻り値】0以外＝非同期読み込み中、0＝非同期読み込み終了またはエラー
 */
int sys_file_poll_async(int request)
{
    if (UNLIKELY(request < 0 || request > lenof(async_info))) {
        DMSG("Request %d out of range", request);
        psp_errno = PSP_EINVAL;
        return 0;
    }

    if (request == 0) {
        int index;
        for (index = 0; index < lenof(async_info); index++) {
            if (async_info[index].fp && sys_file_poll_async(index+1)) {
                return 1;
            }
        }
        return 0;
    }  // if (request == 0)

    const int index = request-1;
    if (UNLIKELY(!async_info[index].fp)) {
        psp_errno = SCE_KERNEL_ERROR_NOASYNC;
        return 0;
    }
    if (!async_info[index].request) {  // すでに完了している
        return 0;
    }
    return !check_async_request(index, 0);
}

/*************************************************************************/

/**
 * sys_file_wait_async:  ファイルからの非同期読み込みの終了を待ち、読み込み
 * の結果を返す。戻り値は、sys_file_read()を呼び出した場合と同じものとなる
 * （但し、非同期読み込みを開始していないというエラーも発生し得る）。
 *
 * request==0の場合、実行中の全ての読み込み要求が終了するまで待つ。この場合
 * の戻り値は不定。
 *
 * 非同期読み込みをサポートしない環境では、sys_file_poll_async()が0を返した
 * 後でもブロックする場合がある。
 *
 * 【引　数】request: 読み込み要求識別子（0＝全要求を待つ）
 * 【戻り値】読み込んだバイト数。読み込み中にエラーが発生した場合は-1を返す
 * 　　　　　（EOFはエラーとはみなさない）
 */
int32_t sys_file_wait_async(int request)
{
    if (UNLIKELY(request < 0 || request > lenof(async_info))) {
        DMSG("Request %d out of range", request);
        psp_errno = PSP_EINVAL;
        return -1;
    }

    if (request == 0) {
        int index;
        for (index = 0; index < lenof(async_info); index++) {
            if (async_info[index].fp) {
                sys_file_wait_async(index+1);
            }
        }
        return 0;
    }  // if (request == 0)

    const int index = request-1;
    if (UNLIKELY(!async_info[index].fp)) {
        psp_errno = SCE_KERNEL_ERROR_NOASYNC;
        return -1;
    }
    SysFile *fp = async_info[index].fp;
    lock_file(fp);

    if (async_info[index].request) {
        /* また終了していないので、終了を待つ */
        check_async_request(index, 1);
    }

    int32_t retval;
    if (UNLIKELY(async_info[index].res < 0)) {
        psp_errno = async_info[index].res;
        if (async_info[index].type == ASYNC_OPEN) {
            sceIoClose(fp->fd);
            fp->inuse = 0;
            retval = 0;
        } else {
            retval = -1;
        }
    } else {
        if (async_info[index].type == ASYNC_OPEN) {
            fp->filepos  = 0;
            fp->filesize = sceIoLseek(fp->fd, 0, PSP_SEEK_END);
            if (UNLIKELY(fp->filesize < 0)) {
                DMSG("Error getting file size for %s: %s",
                     fp->path, psp_strerror(fp->filesize));
                fp->filesize = 0;
            }
            retval = 1;
        } else {
            retval = async_info[index].res;
        }
    }
    free_async(index);
    unlock_file(fp);
    return retval;
}

/*************************************************************************/

/**
 * sys_file_abort_async:  ファイルからの非同期読み込みを中止する。中止して
 * も、sys_file_poll_async()やsys_file_wait_async()で中止処理の完了を確認
 * するまでは読み込みバッファを解放したり、他の用途に使ってはならない。
 *
 * なお、システムによっては読み込みをすぐに中止できない場合があるので、中止
 * 処理が成功したからといって、sys_file_wait_async()がすぐに戻るとは限らない。
 *
 * 【引　数】request: 読み込み要求識別子
 * 【戻り値】0以外＝成功、0＝失敗
 */
int sys_file_abort_async(int request)
{
    if (UNLIKELY(request < 0 || request > lenof(async_info))) {
        DMSG("Request %d out of range", request);
        psp_errno = PSP_EINVAL;
        return 0;
    }
    const int index = request-1;
    if (!async_info[index].fp) {
        psp_errno = SCE_KERNEL_ERROR_NOASYNC;
        return 0;
    }
    psp_file_read_abort(async_info[index].request);
    return 1;
}

/*************************************************************************/

/**
 * sys_file_close:  ファイルを閉じる。fp==NULLの場合、何もしない。
 *
 * 【引　数】fp: ファイルポインタ
 * 【戻り値】なし
 */
void sys_file_close(SysFile *fp)
{
    if (fp) {
        lock_file(fp);
        int i;
        for (i = 0; i < lenof(async_info); i++) {
            if (async_info[i].fp == fp) {
                check_async_request(i, 1);
                free_async(i);
            }
        }
        sceIoClose(fp->fd);
        fp->inuse = 0;
        unlock_file(fp);
    }
}

/*************************************************************************/
/************************ PSPライブラリ内部用関数 ************************/
/*************************************************************************/

/**
 * psp_file_init:  ファイルアクセス機能の初期化を行う。
 *
 * 【引　数】basepath: データファイルの基準パス名
 * 【戻り値】0以外＝成功、0＝失敗
 */
int psp_file_init(const char *basepath_)
{
    PRECOND_SOFT(basepath_ != NULL, return 0);

    /* 基準パス名を記録する */
    const int basepath_size = strlen(basepath_) + 1;
    if (basepath_size > sizeof(basepath)) {
        DMSG("Base path length too long!  max=%d path=%s",
             sizeof(basepath)-1, basepath_);
        return 0;
    }
    memcpy(basepath, basepath_, basepath_size);

    /* 各ファイルハンドルのmutexを作成する */
    int i;
    for (i = 0; i < lenof(filetable); i++) {
        char namebuf[32];
        snprintf(namebuf, sizeof(namebuf), "File%dMutex", i);
        file_mutex[i] = sceKernelCreateSema(namebuf, 0, 1, 1, NULL);
        if (file_mutex[i] < 0) {
            DMSG("Failed to create file %d mutex: %s",
                 i, psp_strerror(file_mutex[i]));
            while (--i >= 0) {
                sceKernelDeleteSema(file_mutex[i]);
                file_mutex[i] = 0;
            }
            return 0;
        }
    }

    /* その他 */
    mem_clear(async_info, sizeof(async_info));
    return 1;
}

/*************************************************************************/

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
int psp_file_open_async(const char *file, SysFile **fp_ret)
{
    if (UNLIKELY(!file) || UNLIKELY(!fp_ret)) {
        DMSG("Invalid parameters: %p[%s] %p", file, file ? file : "", fp_ret);
        psp_errno = PSP_EINVAL;
        goto error_return;
    }

    /* 空き管理構造体（ファイルハンドル）を確保する */
    SysFile *fp = alloc_file();
    if (UNLIKELY(!fp)) {
        psp_errno = PSP_EMFILE;
        return 0;
    }
    mem_clear(fp, sizeof(*fp));
    fp->inuse = 1;

    /* パス名の長さをチェックし、ファイル構造体にコピーする */
    int len;
    if (strchr(file, ':')) {
        len = snprintf(fp->path, sizeof(fp->path), "%s", file);
    } else {
        len = snprintf(fp->path, sizeof(fp->path), "%s/%s", basepath, file);
    }
    if (UNLIKELY(len >= sizeof(fp->path))) {
        psp_errno = PSP_ENAMETOOLONG;
        goto error_unlock_file;
    }

    /* 非同期要求スロットを確保する */
    const int req_index = alloc_async(fp);
    if (UNLIKELY(req_index < 0)) {
        DMSG("No free async blocks");
        psp_errno = SCE_KERNEL_ERROR_ASYNC_BUSY;
        goto error_unlock_file;
    }
    async_info[req_index].type = ASYNC_OPEN;
    async_info[req_index].request = OPEN_REQUEST;
    const int request = req_index + 1;

    /* ファイルをオープンしてみる */
    int fd = sceIoOpenAsync(fp->path, PSP_O_RDONLY, 0);
    if (UNLIKELY(fd < 0)) {
        psp_errno = fd;
        async_info[req_index].fp = NULL;
        goto error_unlock_file;
    }
    fp->fd = fd;

    /* ファイルポインタと要求識別子を返す */
    *fp_ret = fp;
    unlock_file(fp);
    return request;

  error_unlock_file:
    unlock_file(fp);
    fp->inuse = 0;
  error_return:
    return 0;
}

/*************************************************************************/

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
int psp_file_read_async_timed(SysFile *fp, void *buf, int32_t len,
                              int32_t filepos, int32_t time_limit)
{
    if (UNLIKELY(!fp)
     || UNLIKELY(!buf)
     || UNLIKELY(len < 0)
     || UNLIKELY(filepos < 0)
     || UNLIKELY(time_limit < 0)
    ) {
        DMSG("Invalid parameters: %p %p %d %d %d",
             fp, buf, len, filepos, time_limit);
        psp_errno = PSP_EINVAL;
        goto error_return;
    }
    lock_file(fp);

    const int index = alloc_async(fp);
    if (UNLIKELY(index < 0)) {
        DMSG("No free async blocks");
        psp_errno = SCE_KERNEL_ERROR_ASYNC_BUSY;
        goto error_unlock_file;
    }

    async_info[index].type = ASYNC_READ;
    async_info[index].request = psp_file_read_submit(
        fp->fd, filepos, len, buf, 1, time_limit
    );
    if (UNLIKELY(!async_info[index].request)) {
        DMSG("(%d,%p,%d): Read request submission failed", fp->fd, buf, len);
        psp_errno = PSP_EIO;
        goto error_free_async;
    }
    async_info[index].res = -1;

    unlock_file(fp);
    return index+1;

  error_free_async:
    free_async(index);
  error_unlock_file:
    unlock_file(fp);
  error_return:
    return 0;
}

/*************************************************************************/

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
void psp_file_pause(void)
{
    int i;

    for (i = 0; i < lenof(async_info); i++) {
        if (async_info[i].fp && async_info[i].request) {
            check_async_request(i, 1);
        }
    }
    for (i = 0; i < lenof(filetable); i++) {
        lock_file(&filetable[i]);
        if (filetable[i].inuse) {
            sceIoClose(filetable[i].fd);
            filetable[i].fd = -1;
        }
    }
}

/*************************************************************************/

/**
 * psp_file_unpause:  使用中の全ファイルのファイルデスクリプタを再度オープン
 * する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void psp_file_unpause(void)
{
    int i;

#ifdef DEBUG
    if (strncmp(basepath, "host", 4) == 0) {
        sceKernelDelayThread(250000);  // PSPlinkのUSB通信が復活するのを待つ
    }
#endif

    for (i = 0; i < lenof(filetable); i++) {
        if (filetable[i].inuse) {
            filetable[i].fd = sceIoOpen(filetable[i].path, PSP_O_RDONLY, 0);
            if (filetable[i].fd < 0) {
                DMSG("Unable to reopen %s: %s",
                     filetable[i].path, psp_strerror(filetable[i].fd));
            }
        }
        unlock_file(&filetable[i]);
    }
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * lock_file:  ファイルハンドルの排他ロックを取得する。
 *
 * 【引　数】fp: ファイルポインタ
 * 【戻り値】なし
 */
static void lock_file(const SysFile *fp)
{
    PRECOND_SOFT(fp != NULL, return);
    const int index = fp - filetable;
    if (UNLIKELY(index < 0)
     || UNLIKELY(index >= lenof(filetable))
     || UNLIKELY(fp != &filetable[index])
    ) {
        DMSG("Invalid file pointer %p (filetable=%p)", fp, filetable);
        return;
    }
    sceKernelWaitSema(file_mutex[index], 1, NULL);
}

/*************************************************************************/

/**
 * unlock_file:  ファイルハンドルの排他ロックを解放する。
 *
 * 【引　数】fp: ファイルポインタ
 * 【戻り値】なし
 */
static void unlock_file(const SysFile *fp)
{
    PRECOND_SOFT(fp != NULL, return);
    const int index = fp - filetable;
    if (UNLIKELY(index < 0)
     || UNLIKELY(index >= lenof(filetable))
     || UNLIKELY(fp != &filetable[index])
    ) {
        DMSG("Invalid file pointer %p (filetable=%p)", fp, filetable);
        return;
    }
    sceKernelSignalSema(file_mutex[index], 1);
}

/*************************************************************************/

/**
 * alloc_file:  新しいファイルハンドルを確保する。成功した場合、ファイル
 * ハンドルはロックされた状態で返される。
 *
 * 【引　数】なし
 * 【戻り値】確保したファイルハンドルへのポインタ（エラーの場合はNULL）
 */
static SysFile *alloc_file(void)
{
    int i;
    for (i = 0; i < lenof(filetable); i++) {
        /* ファイルハンドルがすでに使用中の場合、ロックを取らない。
         * （読み込み中などでロックがすぐに取れない場合があるため） */
        if (!filetable[i].inuse) {
            lock_file(&filetable[i]);
            /* ロックした状態で、もう一度未使用であることを確認する。
             * 別スレッドが同時にこのチェックをしているかもしれない */
            if (!filetable[i].inuse) {
                break;
            } else {
                /* レースに負けたので、ロックを解除して次へ */
                unlock_file(&filetable[i]);
            }
        }
    }
    if (UNLIKELY(i >= lenof(filetable))) {
        return NULL;
    }
    return &filetable[i];
}

/*************************************************************************/
/*************************************************************************/

/**
 * alloc_async:  async_info[]エントリーを確保し、指定されたファイルハンドル
 * と関連づける。
 *
 * 【引　数】fp: 関連づけるファイルポインタ
 * 【戻り値】確保したasync_info[]インデックス（負数＝失敗）
 */
static int alloc_async(SysFile *fp)
{
    int index;

    Forbid();
    for (index = 0; index < lenof(async_info); index++) {
        if (!async_info[index].fp) {
            async_info[index].fp = fp;
            break;
        }
    }
    Permit();
    return (index < lenof(async_info)) ? index : -1;
}

/*************************************************************************/

/**
 * free_async:  async_info[]エントリーを解放する。
 *
 * 【引　数】index: 解放するエントリーのasync_info[]インデックス
 * 【戻り値】なし
 */
static void free_async(int index)
{
    PRECOND_SOFT(index >= 0 && index < lenof(async_info), return);
    Forbid();
    mem_clear(&async_info[index], sizeof(async_info[index]));
    Permit();
}

/*************************************************************************/

/**
 * check_async_request:  非同期オープン・読み込み要求の完了状況をチェック
 * して（wait!=0の場合は完了を待って）、完了した場合は処理の結果を
 * async_info[index].resに格納する。
 *
 * 【引　数】index: async_info[]添え字
 * 　　　　　 wait: 0以外＝完了を待つ、0＝待たない
 * 【戻り値】0以外＝完了、0＝完了でない
 */
static int check_async_request(int index, int wait)
{
    PRECOND_SOFT(index >= 0 && index < lenof(async_info), return 1);
    PRECOND_SOFT(async_info[index].request != 0, return 1);

    if (async_info[index].type == ASYNC_OPEN) {
        int64_t res = 0;
        int err;
        if (wait) {
            err = sceIoWaitAsync(async_info[index].fp->fd, &res);
        } else {
            err = sceIoPollAsync(async_info[index].fp->fd, &res);
            if (err > 0) {  // 処理中
                return 0;
            }
        }
        if (err == 0) {
            async_info[index].res = (int32_t)res;
        } else {
            async_info[index].res = err;
        }
    } else {
        if (!wait && !psp_file_read_check(async_info[index].request)) {
            return 0;
        }
        async_info[index].res = psp_file_read_wait(async_info[index].request);
    }
    async_info[index].request = 0;
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
