/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sound/sound.c: Sound playback routines.
 */

#include "../common.h"
#include "../sound.h"
#include "../sysdep.h"

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* 各チャンネルの使用中フラグ */
static uint8_t channel_in_use[SOUND_CHANNELS];

/* 各チャンネルの一時停止フラグ（これが立っていると、再生停止コールバックの
 * チャンネル解放処理を抑制する） */
static uint8_t channel_paused[SOUND_CHANNELS];

/*-----------------------------------------------------------------------*/

/* ローカル関数宣言 */
static void sound_trigger_callback(int channel);

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * sound_play_buffer:  メモリバッファにロードされている音声データを再生
 * する。
 *
 * 【引　数】channel: 再生チャンネル（1〜SOUND_CHANNELS、0＝自動確保）
 * 　　　　　 format: 音声データ形式（SOUND_FORMAT_*）
 * 　　　　　   data: 音声データバッファ
 * 　　　　　datalen: データ長（バイト）
 * 　　　　　 volume: 音量（0〜∞、0＝無音、1＝通常音量）
 * 　　　　　    pan: 音声位置（-1＝左、0＝中央、1＝右）
 * 　　　　　   loop: 0以外＝ループ再生を行う、0＝ループしない
 * 【戻り値】再生チャンネル（0以外）、エラーの場合は0
 */
int sound_play_buffer(int channel, SoundFormat format, const void *data,
                      uint32_t datalen, float volume, float pan, int loop)
{
    if (UNLIKELY(channel < 0 || channel > SOUND_CHANNELS)
     || UNLIKELY(data == NULL)
     || UNLIKELY(datalen == 0)
     || UNLIKELY(volume < 0)
     || UNLIKELY(pan < -1 || pan > 1)
    ) {
        DMSG("Invalid parameters: %d 0x%X %p %u %.3f %.3f %d",
             channel, format, data, datalen, volume, pan, loop);
        return 0;
    }

    if (channel) {
        channel--;
        if (channel_in_use[channel]) {
            sys_sound_reset(channel);
        }
    } else {
        for (channel = 0; channel < lenof(channel_in_use); channel++) {
            if (!channel_in_use[channel]) {
                break;
            }
        }
        if (channel >= lenof(channel_in_use)) {
            DMSG("No free channels for sound %p (vol=%.3f pan=%.3f loop=%d)",
                 data, volume, pan, loop);
            return 0;
        }
    }

    sys_sound_lock();
    if (!sys_sound_setdata(channel, format, data, datalen, 0, loop ? -1 : 0)
     || !sys_sound_settrig(channel, sound_trigger_callback)
    ) {
        sys_sound_unlock();
        DMSG("Failed to start sound %p (vol=%.3f pan=%.3f loop=%d) on"
             " channel %u", data, volume, pan, loop, channel+1);
        return 0;
    }
    sys_sound_setvol(channel, volume);
    sys_sound_setpan(channel, pan);
    sys_sound_start(channel);
    channel_in_use[channel] = 1;
    channel_paused[channel] = 0;
    sys_sound_unlock();

    return channel + 1;
}

/*-----------------------------------------------------------------------*/

/**
 * sound_play_file:  データファイルに格納されている音声データを再生する。
 *
 * 【引　数】channel: 再生チャンネル（1〜SOUND_CHANNELS、0＝自動確保）
 * 　　　　　 format: 音声データ形式（SOUND_FORMAT_*）
 * 　　　　　     fp: 音声データファイル
 * 　　　　　dataofs: ファイル内の音声データのオフセット（バイト）
 * 　　　　　datalen: 音声データのデータ長（バイト）
 * 　　　　　 volume: 音量（0〜∞、0＝無音、1＝通常音量）
 * 　　　　　    pan: 音声位置（-1＝左、0＝中央、1＝右）
 * 　　　　　   loop: 0以外＝ループ再生を行う、0＝ループしない
 * 【戻り値】再生チャンネル（0以外）、エラーの場合は0
 */
int sound_play_file(int channel, SoundFormat format, SysFile *fp,
                    uint32_t dataofs, uint32_t datalen,
                    float volume, float pan, int loop)
{
    if (UNLIKELY(fp == NULL)
     || UNLIKELY(volume < 0)
     || UNLIKELY(pan < -1 || pan > 1)
    ) {
        DMSG("Invalid parameters: %d 0x%X %p %u %u %.3f %.3f %d",
             channel, format, fp, dataofs, datalen, volume, pan, loop);
        return 0;
    }

    if (channel) {
        channel--;
        if (channel_in_use[channel]) {
            sys_sound_reset(channel);
        }
    } else {
        for (channel = 0; channel < lenof(channel_in_use); channel++) {
            if (!channel_in_use[channel]) {
                break;
            }
        }
        if (channel >= lenof(channel_in_use)) {
            DMSG("No free channels for sound %p (vol=%.3f pan=%.3f loop=%d)",
                 fp, volume, pan, loop);
            return 0;
        }
    }

    sys_sound_lock();
    if (!sys_sound_setfile(channel, format, fp, dataofs, datalen,
                           0, loop ? -1 : 0)
     || !sys_sound_settrig(channel, sound_trigger_callback)
    ) {
        sys_sound_unlock();
        DMSG("Failed to start sound %p (vol=%.3f pan=%.3f loop=%d) on"
             " channel %u", fp, volume, pan, loop, channel+1);
        return 0;
    }
    sys_sound_setvol(channel, volume);
    sys_sound_setpan(channel, pan);
    sys_sound_start(channel);
    channel_in_use[channel] = 1;
    channel_paused[channel] = 0;
    sys_sound_unlock();

    return channel + 1;
}

/*************************************************************************/

/**
 * sound_pause:  指定された音声チャンネルの再生を停止する。すでに停止中の
 * 場合は何もしない。
 *
 * 【引　数】channel: 再生チャンネル
 * 【戻り値】なし
 */
void sound_pause(int channel)
{
    if (channel < 1 || channel > lenof(channel_in_use)) {
        DMSG("Invalid parameters: %d", channel);
        return;
    }
    channel--;
    if (!channel_in_use[channel]) {
        return;
    }

    sys_sound_lock();
    channel_paused[channel] = 1;
    sys_sound_stop(channel);
    sys_sound_unlock();
}

/*-----------------------------------------------------------------------*/

/**
 * sound_resume:  指定された音声チャンネルの再生を再開する。すでに再生中の
 * 場合は何もしない。
 *
 * 【引　数】channel: 音声チャンネル
 * 【戻り値】なし
 */
void sound_resume(int channel)
{
    if (channel < 1 || channel > lenof(channel_in_use)) {
        DMSG("Invalid parameters: %d", channel);
        return;
    }
    channel--;
    if (!channel_in_use[channel]) {
        return;
    }

    sys_sound_lock();
    channel_paused[channel] = 0;
    sys_sound_resume(channel);
    sys_sound_unlock();
}

/*-----------------------------------------------------------------------*/

/**
 * sound_cut:  指定された音声チャンネルの再生を即時に中止する。中止後は再開
 * できない。再生中でない場合、何もしない。
 *
 * 【引　数】channel: 音声チャンネル
 * 【戻り値】なし
 */
void sound_cut(int channel)
{
    if (channel < 1 || channel > lenof(channel_in_use)) {
        DMSG("Invalid parameters: %d", channel);
        return;
    }
    channel--;

    if (channel_in_use[channel]) {
        sys_sound_reset(channel);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * sound_fade:  指定された音声チャンネルの再生をフェードアウトして中止する。
 * 中止後は再開できない。
 *
 * 【引　数】channel: 音声チャンネル
 * 　　　　　   time: フェード時間（秒）
 * 【戻り値】なし
 */
void sound_fade(int channel, float time)
{
    if (channel < 1 || channel > lenof(channel_in_use)) {
        DMSG("Invalid parameters: %d %.3f", channel, time);
        return;
    }
    channel--;
    if (!channel_in_use[channel]) {
        DMSG("Channel %d not in use", channel+1);
        return;
    }

    sys_sound_setfade(channel, 0.0f, time, 1);
}

/*-----------------------------------------------------------------------*/

/**
 * sound_adjust_volume:  指定された音声チャンネルの再生音量を変更する。再生
 * 中でない場合、何もしない。
 *
 * 【引　数】   channel: 音声チャンネル
 * 　　　　　new_volume: 新しい音量（0〜∞、1＝通常音量）
 * 　　　　　      time: フェード時間（秒、0＝即時変更）
 * 【戻り値】なし
 */
void sound_adjust_volume(int channel, float new_volume, float time)
{
    if (channel < 1 || channel > lenof(channel_in_use)
     || new_volume < 0 || time < 0
    ) {
        DMSG("Invalid parameters: %d %.3f %.3f", channel, new_volume, time);
        return;
    }
    channel--;

    if (channel_in_use[channel]) {
        if (time == 0) {
            sys_sound_setvol(channel, new_volume);
        } else {
            sys_sound_setfade(channel, new_volume, time, 0);
        }
    }
}

/*************************************************************************/

/**
 * sound_is_playing:  指定された音声チャンネルが再生中かどうかを返す。一時
 * 停止中は再生中として扱われる。
 *
 * 【引　数】channel: 音声チャンネル
 * 【戻り値】0以外＝再生中、0＝再生中でない
 */
int sound_is_playing(int channel)
{
    if (channel < 1 || channel > lenof(channel_in_use)) {
        DMSG("Invalid parameters: %d", channel);
        return 0;
    }
    channel--;

    return channel_in_use[channel];
}

/*-----------------------------------------------------------------------*/

/**
 * sound_playback_pos:  指定された音声チャンネルの再生位置を秒単位で返す。
 * 再生中でない場合は0を返す。
 *
 * 【引　数】channel: 音声チャンネル
 * 【戻り値】再生位置（秒）
 */
float sound_playback_pos(int channel)
{
    if (channel < 1 || channel > lenof(channel_in_use)) {
        DMSG("Invalid parameters: %d", channel);
        return 0;
    }
    channel--;

    if (!channel_in_use[channel]) {
        return 0;
    }
    return sys_sound_position(channel);
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * sound_trigger_callback:  再生トリガー用コールバック関数。再生が終了した
 * チャンネルのリソースを解放し、使用中フラグをクリアする。
 *
 * 【引　数】channel: 再生が終了したチャンネル
 * 【戻り値】なし
 */
static void sound_trigger_callback(int channel)
{
    if (channel_paused[channel]) {
        if (!sys_sound_settrig(channel, sound_trigger_callback)) {
            DMSG("WARNING: failed to restore trigger on channel %d,"
                 " channel will leak!", channel+1);
        }
    } else {
        sys_sound_reset(channel);
        channel_in_use[channel] = 0;
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
