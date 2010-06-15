/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/sound-mp3.h: PSP MP3 decoding module header.
 */

#ifndef SYSDEP_PSP_SOUND_MP3_H
#define SYSDEP_PSP_SOUND_MP3_H

/*************************************************************************/

/**
 * psp_decode_mp3_open:  MP3形式の音声データのデコードを開始する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int psp_decode_mp3_open(SoundDecodeHandle *this);

/*************************************************************************/

#endif  // SYSDEP_PSP_SOUND_MP3_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
