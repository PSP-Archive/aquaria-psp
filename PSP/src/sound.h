/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sound.h: Sound-related functionality header.
 */

#ifndef SOUND_H
#define SOUND_H

/*************************************************************************/

/*
 * SOUND_CHANNELS:  サポートされているチャンネルの数。
 */
#define SOUND_CHANNELS  128

/*
 * SoundFormat:  音声データ形式を指定する定数。値はWAVファイルヘッダなどで
 * 使われる一般的なもの。
 */

//typedef enum SoundFormat_ SoundFormat; -- common.hへ
enum SoundFormat_ {
    SOUND_FORMAT_WAV = 0x0001,  // S16LE型PCMのみ
    SOUND_FORMAT_MP3 = 0x0055,
    SOUND_FORMAT_OGG = 0x674F,  // Vorbis音声の入ったOggファイル
};
#ifdef __cplusplus  // common.hでは定義できない模様
typedef enum SoundFormat_ SoundFormat;
#endif

/*
 * SoundDecodeHandle:  任意形式の音声データをPCMにデコードするためのオブジェ
 * クトハンドル。sound_decode_open()またはsound_decode_open_from_file()を
 * 呼び出すことでハンドルを作成し、sound_decode_close()によって破棄する。
 */
typedef struct SoundDecodeHandle_ SoundDecodeHandle;

/*-----------------------------------------------------------------------*/

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
extern int sound_play_buffer(int channel, SoundFormat format, const void *data,
                             uint32_t datalen, float volume, float pan, int loop);

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
extern int sound_play_file(int channel, SoundFormat format, SysFile *fp,
                           uint32_t dataofs, uint32_t datalen,
                           float volume, float pan, int loop);

/**
 * sound_pause:  指定された音声チャンネルの再生を停止する。すでに停止中の
 * 場合は何もしない。
 *
 * 【引　数】channel: 再生チャンネル
 * 【戻り値】なし
 */
extern void sound_pause(int channel);

/**
 * sound_resume:  指定された音声チャンネルの再生を再開する。すでに再生中の
 * 場合は何もしない。
 *
 * 【引　数】channel: 再生チャンネル
 * 【戻り値】なし
 */
extern void sound_resume(int channel);

/**
 * sound_cut:  指定された音声チャンネルの再生を即時に中止する。中止後は再開
 * できない。再生中でない場合、何もしない。
 *
 * 【引　数】channel: 音声チャンネル
 * 【戻り値】なし
 */
extern void sound_cut(int channel);

/**
 * sound_fade:  指定された音声チャンネルの再生をフェードアウトして中止する。
 * 中止後は再開できない。
 *
 * 【引　数】channel: 音声チャンネル
 * 　　　　　   time: フェード時間（秒）
 * 【戻り値】なし
 */
extern void sound_fade(int channel, float time);

/**
 * sound_adjust_volume:  指定された音声チャンネルの再生音量を変更する。再生
 * 中でない場合、何もしない。
 *
 * 【引　数】   channel: 音声チャンネル
 * 　　　　　new_volume: 新しい音量（0〜∞、1＝通常音量）
 * 　　　　　      time: フェード時間（秒、0＝即時変更）
 * 【戻り値】なし
 */
extern void sound_adjust_volume(int channel, float new_volume, float time);

/**
 * sound_is_playing:  指定された音声チャンネルが再生中かどうかを返す。一時
 * 停止中は再生中として扱われる。
 *
 * 【引　数】channel: 音声チャンネル
 * 【戻り値】0以外＝再生中、0＝再生中でない
 */
extern int sound_is_playing(int channel);

/**
 * sound_playback_pos:  指定された音声チャンネルの再生位置を秒単位で返す。
 * 再生中でない場合は0を返す。
 *
 * 【引　数】channel: 音声チャンネル
 * 【戻り値】再生位置（秒）
 */
extern float sound_playback_pos(int channel);

/*-----------------------------------------------------------------------*/

/**
 * SoundDecodeOpenFunc:  音声デコーダモジュールのデコード開始関数
 * （sound_decode_set_handler()に渡される関数）の型。
 *
 * 【引　数】this: 音声デコードオブジェクト
 * 【戻り値】0以外＝成功、0＝失敗
 */
typedef int SoundDecodeOpenFunc(SoundDecodeHandle *this);

/**
 * sound_decode_set_handler:  指定された音声データ形式のデコーダモジュール
 * を登録する。
 *
 * 【引　数】   format: 音声データ形式（SOUND_FORMAT_*）
 * 　　　　　open_func: デコード開始関数
 * 【戻り値】なし
 */
extern void sound_decode_set_handler(SoundFormat format,
                                     SoundDecodeOpenFunc *open_func);

/**
 * sound_decode_open:  メモリバッファに格納されている音声データのデコードを
 * 開始する。
 *
 * 【引　数】   format: 音声データ形式（SOUND_FORMAT_*定数）
 * 　　　　　     data: 音声データバッファ
 * 　　　　　  datalen: 音声データ長（バイト）
 * 　　　　　loopstart: ループ開始位置（サンプル）
 * 　　　　　  looplen: ループ長（サンプル）、0＝ループなし、
 * 　　　　　           　　負数＝データ終端でループ
 * 　　　　　     freq: PCMデータの出力周波数
 * 【戻り値】音声デコードハンドル（エラーの場合はNULL）
 */
extern SoundDecodeHandle *sound_decode_open(SoundFormat format,
					    const void *data, uint32_t datalen,
					    uint32_t loopstart, int32_t looplen,
                                            uint32_t freq);

/**
 * sound_decode_open_from_file:  データファイルに格納されている音声データの
 * デコードを開始する。渡されたファイルハンドルはデコーダが専用するので、
 * この関数を呼び出した後に再利用してはならない。ファイルハンドルはデコード
 * 終了時、またはこの関数が失敗したときに自動的にクローズされる。
 *
 * 【引　数】   format: 音声データ形式（SOUND_FORMAT_*定数）
 * 　　　　　       fp: 音声データファイルハンドル
 * 　　　　　  dataofs: ファイル内の音声データのオフセット（バイト）
 * 　　　　　  datalen: 音声データのデータ長（バイト）
 * 　　　　　loopstart: ループ開始位置（サンプル）
 * 　　　　　  looplen: ループ長（サンプル）、0＝ループなし、
 * 　　　　　           　　負数＝データ終端でループ
 * 　　　　　     freq: PCMデータの出力周波数
 * 【戻り値】音声デコードハンドル（エラーの場合はNULL）
 */
extern SoundDecodeHandle *sound_decode_open_from_file(
    SoundFormat format, SysFile *fp,
    uint32_t dataofs, uint32_t datalen,
    uint32_t loopstart, int32_t looplen, uint32_t freq);

/**
 * sound_decode_is_stereo:  音声データのステレオ・モノラルの別を返す。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝ステレオ、0＝モノラル
 */
extern int sound_decode_is_stereo(SoundDecodeHandle *this);

/**
 * sound_decode_set_freq:  get_pcm()が返すPCMデータの周波数を設定する。
 *
 * 【引　数】freq: PCM出力周波数
 * 【戻り値】なし
 */
extern void sound_decode_set_freq(SoundDecodeHandle *this, int freq);

/**
 * sound_decode_reset:  音声データを初めから再生するための準備を行う。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sound_decode_reset(SoundDecodeHandle *this);

/**
 * sound_decode_get_pcm:  現在の再生位置からPCMデータを取得し、再生位置を
 * 進める。PCMバッファが一杯になる前に音声データの終端に到達した場合、無音
 * データでバッファの残りを埋める。
 *
 * 【引　数】pcm_buffer: PCMデータ（S16LE型）を格納するバッファ
 * 　　　　　   pcm_len: 取得するサンプル数
 * 【戻り値】0以外＝成功、0＝失敗（データ終端またはデコードエラー）
 */
extern int sound_decode_get_pcm(SoundDecodeHandle *this, int16_t *pcm_buffer,
                                uint32_t pcm_len);

/**
 * sound_decode_get_position:  get_pcm()が次に返すPCMデータの時間的位置を
 * 秒単位で返す。
 *
 * 【引　数】なし
 * 【戻り値】デコード位置（秒）
 */
extern float sound_decode_get_position(SoundDecodeHandle *this);

/**
 * sound_decode_close:  デコードを終了し、関連リソースを破棄する。ソース
 * データに関しては、メモリバッファは破棄されないが、ファイルからストリー
 * ミングされている場合はファイルもクローズされる。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sound_decode_close(SoundDecodeHandle *this);

/*************************************************************************/

#endif  // SOUND_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
