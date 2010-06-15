/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/sound-low.c: Low-level PSP sound driver.
 */

#include "../common.h"

#include "psplocal.h"
#include "sound-low.h"

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* 最大スレッド数 */
#define MAX_THREADS  8  // PSPのハードウェアチャンネル数と同じ

/* スレッド管理構造体 */
typedef struct SoundThreadInfo_ {
    SceUID handle;      // スレッドハンドル（0＝エントリー未使用）
    int channel;        // ハードウェアチャンネル番号
    int blocksize;      // 1ブロックあたりのサンプル数
    PSPSoundCallback callback;  // データ生成関数
    void *userdata;     // コールバックに渡すデータ
    volatile int stop;  // 再生停止・スレッド終了要求フラグ
} SoundThreadInfo;
static SoundThreadInfo threads[MAX_THREADS];

/* スレッド関数宣言 */
static int sound_thread(SceSize args, void *argp);

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * psp_sound_start_channel:  音声チャンネルを確保し、再生を開始する。
 *
 * 【引　数】blocksize: 再生ブロックサイズ（サンプル単位）
 * 　　　　　 callback: 再生コールバック関数
 * 　　　　　 userdata: コールバックに渡すデータ
 * 　　　　　stacksize: 再生スレッドの最大スタックサイズ（バイト）
 * 【戻り値】0以上＝成功（ハードウェアチャンネル番号）、負数＝失敗
 */
int psp_sound_start_channel(int blocksize, PSPSoundCallback callback,
                            void *userdata, int stacksize)
{
    /* 空きスロットを探す */
    int index;
    for (index = 0; index < lenof(threads); index++) {
        if (!threads[index].handle) {
            break;
        }
    }
    if (index >= lenof(threads)) {
        DMSG("No thread slots available for blocksize %d callback %p",
             blocksize, callback);
        return -1;
    }

    /* 音声チャンネルを確保する */
    int channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, blocksize,
                                    PSP_AUDIO_FORMAT_STEREO);
    if (channel < 0) {
        DMSG("Failed to allocate channel: %s", psp_strerror(channel));
        return -1;
    }

    /* スレッド情報を設定する */
    threads[index].channel   = channel;
    threads[index].blocksize = blocksize;
    threads[index].callback  = callback;
    threads[index].userdata  = userdata;
    threads[index].stop      = 0;

    /* スレッドを開始する */
    char namebuf[100];
    snprintf(namebuf, sizeof(namebuf), "SoundCh%d", channel);
    SoundThreadInfo *infoptr = &threads[index];
    int handle = psp_start_thread(namebuf, sound_thread,
                                  THREADPRI_SOUND, stacksize,
                                  sizeof(infoptr), &infoptr);
    if (handle < 0) {
        DMSG("Failed to create thread: %s", psp_strerror(handle));
        sceAudioChRelease(channel);
        return -1;
    }

    /* 成功 */
    threads[index].handle = handle;
    return channel;
}

/*************************************************************************/

/**
 * psp_sound_stop_channel:  指定された音声チャンネルの再生を停止し、チャン
 * ネルを解放する。チャンネルはpsp_sound_start_channel()で確保されたもので
 * なければならない。
 *
 * 【引　数】channel: 停止するチャンネル
 * 【戻り値】なし
 */
void psp_sound_stop_channel(int channel)
{
    /* 指定されたチャンネルの担当スレッドを探す */
    int index;
    for (index = 0; index < lenof(threads); index++) {
        if (threads[index].channel == channel && threads[index].handle != 0) {
            break;
        }
    }
    if (index >= lenof(threads)) {
        DMSG("No thread found for channel %d", channel);
        return;
    }
    threads[index].stop = 1;
}

/*************************************************************************/

/**
 * psp_sound_low_pause:  音声出力を停止する。sys_sound_pause()から呼び出され
 * る。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void psp_sound_low_pause(void)
{
    int i;
    for (i = 0; i < lenof(threads); i++) {
        if (threads[i].handle) {
            sceKernelSuspendThread(threads[i].handle);
        }
    }
}

/*************************************************************************/

/**
 * psp_sound_low_unpause:  音声出力を再開する。sys_sound_pause()から呼び出さ
 * れる。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void psp_sound_low_unpause(void)
{
    int i;
    for (i = 0; i < lenof(threads); i++) {
        if (threads[i].handle) {
            sceKernelResumeThread(threads[i].handle);
        }
    }
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * sound_thread:  音声再生スレッド。一つのチャンネルで音声をひたすら再生し
 * 続ける。チャンネル番号やコールバック関数はスレッドパラメータから取る。
 *
 * 【引　数】args: 引数サイズ
 * 　　　　　argp: 引数ポインタ
 * 【戻り値】スレッド終了コード
 */
static int sound_thread(SceSize args, void *argp)
{
    SoundThreadInfo * const info = *(SoundThreadInfo **)argp;

    while (LIKELY(!psp_exit) && LIKELY(!info->stop)) {
        int volume = -1;
        const void *data = (*info->callback)(info->blocksize, &volume,
                                             info->userdata);
        if (data) {
            sceAudioOutputBlocking(info->channel, volume, data);
        } else {
            sceKernelDelayThread(10000);
        }
    }

    sceAudioChRelease(info->channel);
    info->handle = 0;
    sceKernelExitDeleteThread(0);
    return 0;  // コンパイラ警告回避
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
