/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/sound-low.h: Low-level PSP sound driver header.
 */

#ifndef SYSDEP_PSP_SOUND_LOW_H
#define SYSDEP_PSP_SOUND_LOW_H

/*************************************************************************/

/* PSPのハードウェア音量最大値 */
#define PSP_VOLUME_MAX  0xFFFF

/*-----------------------------------------------------------------------*/

/**
 * PSPSoundCallback:  音声再生コールバック関数の型。生成すべきサンプル数を
 * 受け、データバッファへのポインタと音量を返す。音量を設定しない場合、
 * チャンネルの音量を変更しない。
 *
 * 【引　数】 blocksize: 生成すべきサンプル数
 * 　　　　　volume_ret: 音量（0〜PSP_VOLUME_MAX）を格納する変数へのポインタ
 * 　　　　　  userdata: psp_sound_start_channel()に渡されたuserdataポインタ
 * 【戻り値】blocksizeサンプルを格納した音声データバッファ（NULL＝無音）
 */
typedef const void *(*PSPSoundCallback)(int blocksize, int *volume_ret,
                                        void *userdata);

/*-----------------------------------------------------------------------*/

/**
 * psp_sound_start_channel:  音声チャンネルを確保し、再生を開始する。
 *
 * 【引　数】blocksize: 再生ブロックサイズ（サンプル単位）
 * 　　　　　 callback: 再生コールバック関数
 * 　　　　　 userdata: コールバックに渡すデータ
 * 　　　　　stacksize: 再生スレッドの最大スタックサイズ（バイト）
 * 【戻り値】0以上＝成功（ハードウェアチャンネル番号）、負数＝失敗
 */
extern int psp_sound_start_channel(int blocksize, PSPSoundCallback callback,
                                   void *userdata, int stacksize);

/**
 * psp_sound_stop_channel:  指定された音声チャンネルの再生を停止し、チャン
 * ネルを解放する。チャンネルはpsp_sound_start_channel()で確保されたもので
 * なければならない。
 *
 * 【引　数】channel: 停止するチャンネル
 * 【戻り値】なし
 */
extern void psp_sound_stop_channel(int channel);

/**
 * psp_sound_low_pause:  音声出力を停止する。sys_sound_pause()から呼び出され
 * る。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void psp_sound_low_pause(void);

/**
 * psp_sound_low_unpause:  音声出力を再開する。sys_sound_pause()から呼び出さ
 * れる。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void psp_sound_low_unpause(void);

/*************************************************************************/

#endif  // SYSDEP_PSP_SOUND_LOW_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
