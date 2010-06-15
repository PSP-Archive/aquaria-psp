/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/sound.c: PSP audio output interface.
 */

#include "../common.h"
#include "../sysdep.h"
#include "../memory.h"
#include "../sound.h"

#include "psplocal.h"
#include "sound-low.h"
#include "sound-mp3.h"

/*************************************************************************/
/**************************** 構成オプション *****************************/
/*************************************************************************/

/* USE_MUTEXを定義すると、排他処理にセマフォア（sound_mutex）を使う */
#define USE_MUTEX

/* 音声データ生成ルーチン選択。
 * 　・SOUNDGEN_MIPS
 * 　　　　汎用MIPSアセンブリで書かれたもの。一番速い。ちなみにVFPUを使っても
 * 　　　　かえって遅くなってしまう。
 * 　・SOUNDGEN_C
 * 　　　　C言語で書かれたもの。実行時間はSOUNDGEN_MIPSより約25%増。
 */
#define SOUNDGEN_MIPS
// #define SOUNDGEN_C

/* 音声再生スレッドのために確保するスタックサイズ（バイト） */
#define SOUNDGEN_STACK_SIZE  16384

/* 処理時間をDMSGで報告 */
// #define SOUNDGEN_TIMING

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* 音声データ生成バッファの長さ（サンプル） */
#define SOUND_BUFLEN  1024

/************************************/

/* 確保したPSP音声チャンネル */
static int psp_sound_channel;

#ifdef USE_MUTEX
/* 排他処理用セマフォア */
static SceUID sound_mutex;
#endif

/* 各音声チャンネルのデータ */
static struct {
    SoundDecodeHandle *decode_handle; 
    SoundTrigCallback trigger;
    uint8_t playing;    // 再生中フラグ
    uint8_t stereo;     // 0以外＝ステレオ、0＝モノラルデータ
    uint8_t fade_cut;   // 0以外＝フェードで無音になった時点でカット
    int32_t volume;     // sys_sound_setvol( 1.0) → VOLUME_MULT
    int32_t fade_rate;  // 出力サンプル当たりのvolume変化量
    int32_t fade_target;// 目標音量値
    int32_t pan;        // sys_sound_setpan(-1.0) → 0
                        // sys_sound_setpan( 1.0) → PAN_MULT
    int16_t pcm_buffer[2*SOUND_BUFLEN];
} channels[SOUND_CHANNELS];
#define VOLUME_MULT  (256<<16)
#define PAN_MULT     256

/* 最大音量 */
#define VOLUME_MAX   ((float)32767 / (float)(VOLUME_MULT>>16))

/* 音声増幅率（+1＝音量2倍） */
#define AMPSHIFT  (-1)

/*************************************************************************/

/* ローカル関数宣言 */

static const void *sound_callback(int blocksize, int *volume_ret,
                                  void *userdata);
static void sound_generate(void *stream, unsigned int len, void *userdata);
static void call_trigger(int channel);

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * sys_sound_pause:  音声出力を停止する。pause()から呼び出される。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sys_sound_pause(void)
{
    psp_sound_low_pause();
}

/*************************************************************************/

/**
 * sys_sound_unpause:  音声出力を再開する。unpause()から呼び出される。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sys_sound_unpause(void)
{
    psp_sound_low_unpause();
}

/*************************************************************************/

/**
 * sys_sound_lock:  全音声処理を一時停止する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sys_sound_lock(void)
{
#ifdef USE_MUTEX
    unsigned int timeout = 10000;  // 最大10ms待つ
    sceKernelWaitSema(sound_mutex, 1, &timeout);
#endif
}

/*************************************************************************/

/**
 * sys_sound_unlock:  全音声処理の一時停止を解除する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void sys_sound_unlock(void)
{
#ifdef USE_MUTEX
    sceKernelSignalSema(sound_mutex, 1);
#endif
}

/*************************************************************************/
/*************************************************************************/

/**
 * sys_sound_checkformat:  指定された音声データ形式がサポートされているか
 * どうかを返す。
 *
 * 【引　数】format: 音声データ形式（SOUND_FORMAT_*）
 * 【戻り値】0以外＝サポートされている、0＝サポートされていない
 */
int sys_sound_checkformat(SoundFormat format)
{
    return format == SYS_SOUND_FORMAT_PCM
        || format == SYS_SOUND_FORMAT_MP3
        || format == SYS_SOUND_FORMAT_OGG;
}

/*************************************************************************/

/**
 * sys_sound_setdata:  指定されたチャンネルに音声データを設定する。
 *
 * チャンネルが再生中の場合、再生が中止される。このとき、sys_sound_stop()
 * 同様に再生終了まで戻らない。
 *
 * 【引　数】  channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 *              format: 音声データ形式（SOUND_FORMAT_*）
 * 　　　　　     data: 設定するデータ
 *             datalen: データ長（バイト）
 * 　　　　　loopstart: ループ開始位置（サンプル数）
 * 　　　　　  looplen: ループ長（サンプル数）。0（ループしない）、
 * 　　　　　           　　負数（データ終端からループする）も可
 * 【戻り値】0以外＝成功、0＝失敗
 */
int sys_sound_setdata(int channel, SoundFormat format,
                      const void *data, uint32_t datalen,
                      uint32_t loopstart, int32_t looplen)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)
     || UNLIKELY(!data)
     || UNLIKELY(!datalen)
    ) {
        DMSG("Invalid parameters: %d 0x%X %p %u %u %u", channel, format,
             data, datalen, loopstart, looplen);
        return 0;
    }

    sys_sound_stop(channel);
    channels[channel].decode_handle =
        sound_decode_open(format, data, datalen, loopstart, looplen,
                          SOUND_RATE);
    if (!channels[channel].decode_handle) {
        DMSG("Failed to get a decode handle");
        return 0;
    }
    channels[channel].stereo =
        sound_decode_is_stereo(channels[channel].decode_handle);
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_sound_setfile:  指定されたチャンネルに音声データファイルを設定する。
 *
 * チャンネルが再生中の場合、再生が中止される。このとき、sys_sound_stop()
 * 同様に再生終了まで戻らない。
 *
 * 【引　数】  channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 *              format: 音声データ形式（SOUND_FORMAT_*）
 * 　　　　　       fp: 設定するデータファイル
 * 　　　　　  dataofs: ファイル内の音声データのオフセット（バイト）
 * 　　　　　  datalen: 音声データのデータ長（バイト）
 * 　　　　　loopstart: ループ開始位置（サンプル数）
 * 　　　　　  looplen: ループ長（サンプル数）。0（ループしない）、
 * 　　　　　           　　負数（データ終端からループする）も可
 * 【戻り値】0以外＝成功、0＝失敗
 */
int sys_sound_setfile(int channel, SoundFormat format, SysFile *fp,
                      uint32_t dataofs, uint32_t datalen,
                      uint32_t loopstart, int32_t looplen)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)
     || UNLIKELY(!fp)
    ) {
        DMSG("Invalid parameters: %d 0x%X %p %u %u %u %u",
             channel, format, fp, dataofs, datalen, loopstart, looplen);
        return 0;
    }

    sys_sound_stop(channel);
    channels[channel].decode_handle =
        sound_decode_open_from_file(format, fp, dataofs, datalen,
                                    loopstart, looplen, SOUND_RATE);
    if (!channels[channel].decode_handle) {
        DMSG("Failed to get a decode handle");
        return 0;
    }
    channels[channel].stereo =
        sound_decode_is_stereo(channels[channel].decode_handle);
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_sound_settrig:  指定されたチャンネルに終了トリガー関数を設定する。
 * 音声の再生が開始された後、再生が終了した時、または他の理由で再生が停止
 * された時に呼び出され、解除される。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 　　　　　   func: 設定する関数（NULLで解除）
 * 【戻り値】0以外＝成功、0＝失敗
 */
int sys_sound_settrig(int channel, SoundTrigCallback func)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)) {
        DMSG("Invalid parameters: %d %p", channel, func);
        return 0;
    }
    channels[channel].trigger = func;
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_sound_setvol:  指定されたチャンネルの音量を設定する。フェード効果が
 * 設定されている場合、その効果が解除される。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 　　　　　 volume: 音量（0〜∞、1＝通常音量）
 * 【戻り値】なし
 */
void sys_sound_setvol(int channel, float volume)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)) {
        DMSG("Invalid parameters: %d %.6f", channel, volume);
        return;
    }
    if (volume < 0) {
        channels[channel].volume = 0;
    } else if (volume >= VOLUME_MAX) {
        channels[channel].volume = iroundf(VOLUME_MAX * VOLUME_MULT);
    } else {
        channels[channel].volume = iroundf(volume * VOLUME_MULT);
    }
    channels[channel].fade_rate = 0;
    channels[channel].fade_cut = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_sound_setpan:  指定されたチャンネルの音声位置を設定する。ステレオ音声
 * データを再生する場合は無効。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 　　　　　    pan: 音声位置（左・-1〜0〜1・右）
 * 【戻り値】なし
 */
void sys_sound_setpan(int channel, float pan)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)) {
        DMSG("Invalid parameters: %d %.6f", channel, pan);
        return;
    }
    if (pan < -1) {
        channels[channel].pan = 0;
    } else if (pan > 1) {
        channels[channel].pan = PAN_MULT;
    } else {
        channels[channel].pan = iroundf(((pan+1)/2) * PAN_MULT);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * sys_sound_setfade:  指定されたチャンネルの音量変化（フェード）設定を行う。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 　　　　　 target: フェード終了時の音量（0〜∞、1＝通常音量）
 * 　　　　　   time: フェード時間（秒、0＝フェード中止）
 * 　　　　　    cut: 0以外＝音量が0（無音）になった時点で再生を停止する
 * 　　　　　         　　0＝音量に関係なく再生を継続する
 * 【戻り値】なし
 */
void sys_sound_setfade(int channel, float target, float time, int cut)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)) {
        DMSG("Invalid parameters: %d %.6f %.6f", channel, target, time);
        return;
    }
    if (time == 0) {
        channels[channel].fade_rate = 0;
        channels[channel].fade_cut = 0;
    } else {
        const float delta_volume =
            bound(target - ((float)channels[channel].volume / VOLUME_MULT),
                  -VOLUME_MAX, VOLUME_MAX);
        const uint32_t samples = lbound(iroundf(time * SOUND_RATE), 1);
        channels[channel].fade_rate =
            iroundf((delta_volume / samples) * VOLUME_MULT);
        channels[channel].fade_target = iroundf(target * VOLUME_MULT);
        channels[channel].fade_cut = (cut != 0);
    }
}

/*************************************************************************/

/**
 * sys_sound_start:  指定されたチャンネルに設定された音声データを初めから
 * 再生する。データが設定されていない場合は何もしない。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】なし
 */
void sys_sound_start(int channel)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)) {
        DMSG("Invalid parameters: %d", channel);
        return;
    }
    if (channels[channel].decode_handle != NULL) {
        sound_decode_reset(channels[channel].decode_handle);
        channels[channel].playing = 1;
    } else {
        call_trigger(channel);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * sys_sound_stop:  指定されたチャンネルの再生を停止する。実際に再生が終了
 * するまで戻らない。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】なし
 */
void sys_sound_stop(int channel)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)) {
        DMSG("Invalid parameters: %d", channel);
        return;
    }
    /* ロックすることによって、(1) 再生スレッドが停止していることが確認でき、
     * (2) トリガー関数の二重呼び出し（channels[channel].playingを確認後、
     * 再生スレッド側再生が自然終了する場合）を防げる */
    sys_sound_lock();
    if (channels[channel].playing) {
        channels[channel].playing = 0;
        call_trigger(channel);
    }
    sys_sound_unlock();
}

/*-----------------------------------------------------------------------*/

/**
 * sys_sound_resume:  指定されたチャンネルの再生を再開する。基本的には
 * sys_sound_start()と同様だが、sys_sound_stop()後は、停止位置から再生を
 * 開始する。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】なし
 */
void sys_sound_resume(int channel)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)) {
        DMSG("Invalid parameters: %d", channel);
        return;
    }
    if (channels[channel].decode_handle != NULL) {
        channels[channel].playing = 1;
    } else {
        call_trigger(channel);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * sys_sound_reset:  指定されたチャンネルの再生を停止し、データをクリアする。
 * 実際に再生が終了するまで戻らない。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】なし
 */
void sys_sound_reset(int channel)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)) {
        DMSG("Invalid parameters: %d", channel);
        return;
    }
    sys_sound_stop(channel);
    if (channels[channel].decode_handle) {
        sound_decode_close(channels[channel].decode_handle);
        channels[channel].decode_handle = NULL;
    }
    channels[channel].trigger   = NULL;
    channels[channel].stereo    = 0;
    channels[channel].fade_cut  = 0;
    channels[channel].volume    = VOLUME_MULT;
    channels[channel].fade_rate = 0;
    channels[channel].pan       = PAN_MULT/2;
}

/*************************************************************************/

/**
 * sys_sound_status:  指定されたチャンネルが再生中かどうかを返す。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】0以外＝再生中、0＝再生中でない
 */
int sys_sound_status(int channel)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)) {
        DMSG("Invalid parameters: %d", channel);
        return 0;
    }
    return channels[channel].playing;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_sound_position:  指定されたチャンネルの再生位置を秒単位で返す。
 * 再生中でない場合は0を返す。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】再生位置（秒）
 */
float sys_sound_position(int channel)
{
    if (UNLIKELY(channel < 0) || UNLIKELY(channel >= SOUND_CHANNELS)) {
        DMSG("Invalid parameters: %d", channel);
        return 0;
    }
    if (channels[channel].decode_handle) {
        return sound_decode_get_position(channels[channel].decode_handle);
    } else {
        return 0;
    }
}

/*************************************************************************/
/************************ PSPライブラリ内部用関数 ************************/
/*************************************************************************/

/**
 * psp_sound_init:  音声機能の初期化を行う。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
int psp_sound_init(void)
{
    /* 二重初期化回避 */
    static int sound_initted = 0;
    if (sound_initted) {  // 二重初期化回避
        return 1;
    }

#ifdef USE_MUTEX
    /* mutexを確保する */
    sound_mutex = sceKernelCreateSema("SoundMutex", 0, 1, 1, 0);
    if (sound_mutex < 0) {
        DMSG("Failed to create sound mutex: %s", psp_strerror(sound_mutex));
        goto error_return;
    }
#endif

    /* MP3デコーダを登録する */
    sound_decode_set_handler(SOUND_FORMAT_MP3, psp_decode_mp3_open);

    /* 音声チャンネルを確保し、再生スレッドを開始する */
    psp_sound_channel = psp_sound_start_channel(SOUND_BUFLEN, sound_callback,
                                                NULL, SOUNDGEN_STACK_SIZE);
    if (psp_sound_channel < 0) {
        DMSG("Failed to allocate primary audio channel");
        goto error_free_sound_mutex;
    }

    /* 初期化成功 */
    sound_initted = 1;
    return 1;

    /* エラー処理 */
  error_free_sound_mutex:
#ifdef USE_MUTEX
    sceKernelDeleteSema(sound_mutex);
    sound_mutex = 0;
  error_return:
#endif
    return 0;
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * sound_callback:  非ストリーミング音声データの生成コールバック。
 *
 * 【引　数】 blocksize: 生成すべきサンプル数
 * 　　　　　volume_ret: 音量（0〜PSP_VOLUME_MAX）を格納する変数へのポインタ
 * 　　　　　  userdata: psp_sound_start_channel()に渡されたuserdataポインタ
 * 　　　　　            　（ここでは未使用）
 * 【戻り値】blocksizeサンプルを格納した音声データバッファ（NULL＝無音）
 */
static const void *sound_callback(int blocksize, int *volume_ret,
                                  void *userdata)
{
    PRECOND(volume_ret != NULL);

    static int16_t audiobuf[2][SOUND_BUFLEN*2];
    static unsigned int buffer = 0;

#ifdef USE_MUTEX
    sys_sound_lock();
#endif
    sound_generate(audiobuf[buffer], SOUND_BUFLEN, NULL);
#ifdef USE_MUTEX
    sys_sound_unlock();
#endif

    *volume_ret = PSP_VOLUME_MAX;
    const void *retval = audiobuf[buffer];
    buffer = (buffer+1) % lenof(audiobuf);
    return retval;
}

/*************************************************************************/

/**
 * sound_generate:  出力するための音声データを生成する。SDLから呼び出される。
 *
 * 【引　数】  stream: 音声データを格納するバッファ
 * 　　　　　   count: 生成すべきサンプル数
 * 　　　　　userdata: 未使用
 * 【戻り値】なし
 */
static void sound_generate(void *stream, unsigned int count, void *userdata)
{
    PRECOND_SOFT(count <= SOUND_BUFLEN, count = SOUND_BUFLEN);

    int16_t *buf = (int16_t *)stream;

    unsigned int ch;
    for (ch = 0; ch < lenof(channels); ch++) {
        if (channels[ch].playing) {
            if (channels[ch].fade_rate) {
                const int32_t samples_left =
                    (channels[ch].fade_target - channels[ch].volume)
                    / channels[ch].fade_rate;
                if (count >= samples_left) {
                    channels[ch].volume = channels[ch].fade_target;
                    channels[ch].fade_rate = 0;
                } else {
                    channels[ch].volume += channels[ch].fade_rate * count;
                }
            }
            if (channels[ch].volume == 0 && channels[ch].fade_cut) {
                channels[ch].playing = 0;
                call_trigger(ch);
                continue;
            }
            const int got_data = sound_decode_get_pcm(
                channels[ch].decode_handle, channels[ch].pcm_buffer, count
            );
            if (!got_data) {
                channels[ch].playing = 0;
                call_trigger(ch);
            }
        }
    }

#ifdef SOUNDGEN_TIMING
    static float timing_sum = 0;
    static int timing_count = 0;
    const double starttime = sys_time_now();
#endif

#ifdef SOUNDGEN_MIPS

    asm(".set push; .set noreorder\n\
        # レジスタの用途は以下の通り。                                  \n\
        #     t0〜t3, t8〜t9: テンポラリなど                            \n\
        #     t4: モノラル：チャンネルの左サンプル音量係数 (volume * (1-pan))\n\
        #         ステレオ：チャンネル音量                              \n\
        #     t5: モノラル：チャンネルの右サンプル音量係数 (volume * pan)\n\
        #         ステレオ：未使用                                      \n\
        #     t6: モノラル：現在のサンプル値                            \n\
        #         ステレオ：現在の左サンプル値                          \n\
        #     t7: モノラル：未使用（旧ampshift）                        \n\
        #         ステレオ：現在の右サンプル値                          \n\
        #     s0: 出力バッファポインタ                                  \n\
        #     s1: 出力バッファ終端ポインタ                              \n\
        #     s2: PCMバッファポインタ                                   \n\
        #     s6: 処理中チャンネルの管理構造体へのポインタ              \n\
        #     s7: 処理中チャンネルのインテックス                        \n\
                                                                        \n\
        # レジスタのセットアップを行う                                  \n\
        sll $t8, %[count], 2                                            \n\
        addu $s1, %[buf], $t8                                           \n\
        addiu $s6, %[channels], -%[sizeof_ch]  # ループ開始時に進める   \n\
                                                                        \n\
        # 出力バッファをゼロクリアする                                  \n\
        move $t8, %[count]                                              \n\
        move $s0, %[buf]                                                \n\
        sltiu $t9, $t8, 8       # 8サンプル未満は高速ループを使わず     \n\
        bnez $t9, 2f                                                    \n\
        nop                                                             \n\
     0: addiu $s0, $s0, 32                                              \n\
        sw $zero, -32($s0)                                              \n\
        sw $zero, -28($s0)                                              \n\
        sw $zero, -24($s0)                                              \n\
        sw $zero, -20($s0)                                              \n\
        sw $zero, -16($s0)                                              \n\
        sw $zero, -12($s0)                                              \n\
        sw $zero, -8($s0)                                               \n\
        sltu $t9, $s0, $s1                                              \n\
        bnez $t9, 0b                                                    \n\
     1: sw $zero, -4($s0)                                               \n\
     2: sltu $t9, $s0, $s1                                              \n\
        bnez $t9, 1b                                                    \n\
        addiu $s0, $s0, 4                                               \n\
                                                                        \n\
        # 再生の準備を行う                                              \n\
        li $s7, -1                                                      \n\
                                                                        \n\
chanloop: # 各チャンネルを処理していく                                  \n\
                                                                        \n\
        # チャンネルポインタとインテックスをインクリメントし、ループ続行\n\
        # 判定を行う                                                    \n\
        lbu $t9, %[sizeof_ch]+%[playing]($s6)                           \n\
        addiu $s7, $s7, 1                                               \n\
        sltiu $t8, $s7, %[num_channels]                                 \n\
        beqz $t8, chanloop_end                                          \n\
        addiu $s6, $s6, %[sizeof_ch]                                    \n\
        # このチャンネルが再生中でない場合、次のチャンネルに進める      \n\
        beqz $t9, chanloop                                              \n\
                                                                        \n\
        ######## モノラルセットアップ ########                          \n\
                                                                        \n\
        # チャンネルのデータなどをロードする                            \n\
        lbu $t8, %[stereo]($s6)                                         \n\
        lw $t9, %[volume]($s6)  # 音量と音声位置を乗算して左右係数を得る\n\
        bnez $t8, stereo_setup                                          \n\
        lw $t5, %[pan]($s6)                                             \n\
        srl $t9, $t9, 16                                                \n\
        li $t4, 256             # PAN_MULT == 256                       \n\
        multu $t9, $t5                                                  \n\
        subu $t4, $t4, $t5                                              \n\
        mflo $t5                                                        \n\
        move $s0, %[buf]                                                \n\
        addiu $s2, $s6, %[pcm_buffer]                                   \n\
        multu $t9, $t4                                                  \n\
        mflo $t4                                                        \n\
        lh $t6, 0($s2)                                                  \n\
                                                                        \n\
        ######## モノラル再生ループ ########                            \n\
                                                                        \n\
        beq $s0, $s1, chanloop                                          \n\
        mult $t6, $t4           # 左チャンネル値を計算（→$t2）         \n\
monoloop:                                                               \n\
        lh $t0, 0($s0)          # $t0: 左チャンネル累積値               \n\
        lh $t1, 2($s0)          # $t1: 右チャンネル累積値               \n\
        addiu $s0, $s0, 4       # 出力ポインタを進めておく              \n\
        mflo $t2                                                        \n\
        sra $t2, $t2, %[ampshift]                                       \n\
        addu $t2, $t2, $t0                                              \n\
        mult $t6, $t5           # 右チャンネル値を計算（→$t3）         \n\
        sh $t2, -4($s0)                                                 \n\
        mflo $t3                                                        \n\
        sra $t3, $t3, %[ampshift]                                       \n\
        addu $t3, $t3, $t1                                              \n\
        sh $t3, -2($s0)                                                 \n\
                                                                        \n\
        # 次のソースサンプルへ                                          \n\
        addiu $s2, $s2, 2                                               \n\
        lh $t6, 0($s2)                                                  \n\
        # 次の出力サンプルへ                                            \n\
        bne $s0, $s1, monoloop                                          \n\
        mult $t6, $t4           # ループ最初の命令を遅延スロットで実行  \n\
        b chanloop                                                      \n\
        nop                                                             \n\
                                                                        \n\
        ######## ステレオセットアップ ########                          \n\
                                                                        \n\
stereo_setup:                                                           \n\
        # チャンネルのデータなどをロードする。この時点ですでに$t9が     \n\
        # ロードされている                                              \n\
        srl $t9, $t9, 16                                                \n\
        move $t4, $t9                                                   \n\
        move $s0, %[buf]                                                \n\
        addiu $s2, $s6, %[pcm_buffer]                                   \n\
        lh $t6, 0($s2)                                                  \n\
        lh $t7, 2($s2)                                                  \n\
                                                                        \n\
        ######## ステレオ再生ループ ########                            \n\
                                                                        \n\
        beq $s0, $s1, chanloop                                          \n\
        mult $t6, $t4           # 左チャンネル値を計算（→$t2）         \n\
stereoloop:                                                             \n\
        lh $t0, 0($s0)          # $t0: 左チャンネル累積値               \n\
        lh $t1, 2($s0)          # $t1: 右チャンネル累積値               \n\
        addiu $s0, $s0, 4       # 出力ポインタを進めておく              \n\
        mflo $t2                                                        \n\
        sra $t2, $t2, %[ampshift]-7                                     \n\
        addu $t2, $t2, $t0                                              \n\
        mult $t7, $t4           # 右チャンネル値を計算（→$t3）         \n\
        sh $t2, -4($s0)                                                 \n\
        mflo $t3                                                        \n\
        sra $t3, $t3, %[ampshift]-7                                     \n\
        addu $t3, $t3, $t1                                              \n\
        sh $t3, -2($s0)                                                 \n\
                                                                        \n\
        # 次のソースサンプルへ                                          \n\
        addiu $s2, $s2, 4                                               \n\
        lh $t6, 0($s2)                                                  \n\
        lh $t7, 2($s2)                                                  \n\
        # 次の出力サンプルへ                                            \n\
        bne $s0, $s1, stereoloop                                        \n\
        mult $t6, $t4           # ループ最初の命令を遅延スロットで実行  \n\
        b chanloop                                                      \n\
        nop                                                             \n\
                                                                        \n\
        ######## 各ループ終了 ########                                  \n\
                                                                        \n\
chanloop_end:                                                           \n\
        .set pop"
        : /* no outputs */
        : [buf] "r" (buf), [count] "r" (count), [channels] "r" (channels),
          [num_channels]  "i" (SOUND_CHANNELS),
          /* ampshift: 8ビット (VOLUME_MULT) + 8ビット (PAN_MULT) - 増幅率 */
          [ampshift]      "i" (16-AMPSHIFT),
          [stereo]        "I" (offsetof(typeof(channels[0]),stereo)),
          [volume]        "I" (offsetof(typeof(channels[0]),volume)),
          [pan]           "I" (offsetof(typeof(channels[0]),pan)),
          [playing]       "I" (offsetof(typeof(channels[0]),playing)),
          [pcm_buffer]    "I" (offsetof(typeof(channels[0]),pcm_buffer)),
          [sizeof_ch]     "I" (sizeof(channels[0])),
          [lenof_ch]      "I" (lenof(channels)),
          "m" (channels[0]), "m" (buf[0])
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9",
          "s0", "s1", "s2", "s6", "s7", "memory"
    );

#endif  // SOUNDGEN_MIPS

#ifdef SOUNDGEN_C

    const int ampshift = 16-AMPSHIFT;

    mem_clear(buf, count*4);

    for (ch = 0; ch < lenof(channels); ch++) {
        if (!channels[ch].playing) {
            continue;
        }
        const int volume = channels[ch].volume >> 16;
        const int pan_l  = PAN_MULT - channels[ch].pan;
        const int pan_r  = channels[ch].pan;
        int i;
        for (i = 0; i < count; i++) {
            if (channels[ch].stereo) {
                buf[i*2+0] += (channels[ch].pcm_buffer[i*2+0] * volume)
                              >> (ampshift-7);
                buf[i*2+1] += (channels[ch].pcm_buffer[i*2+1] * volume)
                              >> (ampshift-7);
            } else {
                const int32_t sample = channels[ch].pcm_buffer[i] * volume;
                buf[i*2+0] += (sample * pan_l) >> ampshift;
                buf[i*2+1] += (sample * pan_r) >> ampshift;
            }
        }
    }  // for (ch = 0 ... lenof(channels)-1)

#endif  // SOUNDGEN_C

#ifdef SOUNDGEN_TIMING
    const double thistime = sys_time_now() - starttime;
    timing_sum += thistime;
    timing_count++;
    if (timing_count == 10) {
        const float sec_smp = timing_sum / timing_count / count;
        DMSG("%.3fus/sample (%.2f%%)",
             sec_smp * 1000000, sec_smp * SOUND_RATE * 100);
        timing_sum = 0;
        timing_count = 0;
    }
#endif
}

/*************************************************************************/

/**
 * call_trigger:  チャンネルのトリガー関数を呼び出し、トリガーを解除する。
 * トリガー関数が設定されていない場合、何もしない。
 *
 * 【引　数】channel: 音声チャンネル
 * 【戻り値】なし
 */
static void call_trigger(int channel)
{
    if (channels[channel].trigger) {
        SoundTrigCallback func = channels[channel].trigger;
        channels[channel].trigger = NULL;
        (*func)(channel);
    }
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
