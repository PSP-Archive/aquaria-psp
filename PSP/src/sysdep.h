/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep.h: System-dependent functionality header.
 */

#ifndef SYSDEP_H
#define SYSDEP_H

/*************************************************************************/
/************************* 基本初期化・終了処理 **************************/
/*************************************************************************/

/**
 * sys_handle_cmdline_param:  共通オプションとして認識されないコマンドライン
 * 引数を処理する。
 *
 * 【引　数】param: コマンドライン引数
 * 【戻り値】なし
 */
extern void sys_handle_cmdline_param(const char *param);

/**
 * sys_init:  システム依存機能の基本的な初期化処理を行う。
 *
 * 【引　数】argv0: main()に渡されたargv[0]の値
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int sys_init(const char *argv0);

/**
 * sys_exit:  システム依存機能のプログラム終了処理を行う。exit()を呼び出す
 * などして、プログラムを終了させなければならない。
 *
 * 【引　数】error: 通常終了か異常終了か（0＝通常終了、0以外＝異常終了）
 * 【戻り値】なし（戻らない）
 */
extern void sys_exit(int error) NORETURN;

/*************************************************************************/
/******************************* 入力関連 ********************************/
/*************************************************************************/

/* sys_input_keystate()に渡すキー値 */

/* 一応ASCII内だが、分かりやすく */
#define SYS_KEY_BACKSPACE   0x08
#define SYS_KEY_TAB         0x09
#define SYS_KEY_ENTER       0x0D
#define SYS_KEY_ESCAPE      0x1B

/* ASCII外 */
#define SYS_KEY_F1          0x100
#define SYS_KEY_F2          0x101
#define SYS_KEY_F3          0x102
#define SYS_KEY_F4          0x103
#define SYS_KEY_F5          0x104
#define SYS_KEY_F6          0x105
#define SYS_KEY_F7          0x106
#define SYS_KEY_F8          0x107
#define SYS_KEY_F9          0x108
#define SYS_KEY_F10         0x109
#define SYS_KEY_F11         0x10A
#define SYS_KEY_F12         0x10B
#define SYS_KEY_PRTSC       0x10C
#define SYS_KEY_SCRLK       0x10D
#define SYS_KEY_PAUSE       0x10E
#define SYS_KEY_NUMLK       0x10F

#define SYS_KEY_KP7         0x110
#define SYS_KEY_KP8         0x111
#define SYS_KEY_KP9         0x112
#define SYS_KEY_KP4         0x113
#define SYS_KEY_KP5         0x114
#define SYS_KEY_KP6         0x115
#define SYS_KEY_KP1         0x116
#define SYS_KEY_KP2         0x117
#define SYS_KEY_KP3         0x118
#define SYS_KEY_KP0         0x119
#define SYS_KEY_KPDOT       0x11A
#define SYS_KEY_KPSLASH     0x11B
#define SYS_KEY_KPSTAR      0x11C
#define SYS_KEY_KPMINUS     0x11D
#define SYS_KEY_KPPLUS      0x11E
#define SYS_KEY_KPENTER     0x11F

#define SYS_KEY_INSERT      0x120
#define SYS_KEY_DELETE      0x121
#define SYS_KEY_HOME        0x122
#define SYS_KEY_END         0x123
#define SYS_KEY_PGUP        0x124
#define SYS_KEY_PGDN        0x125
#define SYS_KEY_UP          0x126
#define SYS_KEY_DOWN        0x127
#define SYS_KEY_LEFT        0x128
#define SYS_KEY_RIGHT       0x129

#define SYS_KEY_CAPSLK      0x130
#define SYS_KEY_LSHIFT      0x131
#define SYS_KEY_LCTRL       0x132
#define SYS_KEY_LALT        0x133
#define SYS_KEY_RSHIFT      0x134
#define SYS_KEY_RCTRL       0x135
#define SYS_KEY_RALT        0x136

/* 最大キー値 */
#define SYS_KEY__MAX        SYS_KEY_RALT

/*************************************************************************/

/**
 * sys_input_update:  各種入力デバイスの現状を取得する。以下の情報取得関数
 * は全て、この関数が呼び出された時点での状況を参照する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sys_input_update(void);

/**
 * sys_input_keystate:  指定されたキーの状態を返す。
 *
 * 【引　数】key: キー値（ASCII文字またはSYS_KEY_*定数）
 * 【戻り値】キーが押されている場合は0以外、押されていない場合（もしくは
 * 　　　　　キー値が無効な場合）は0
 */
extern int sys_input_keystate(int key);

/**
 * sys_input_buttonstate:  指定されたジョイスティックボタンの状態を返す。
 *
 * 【引　数】button: ボタン番号（0〜）
 * 【戻り値】ボタンが押されている場合は0以外、押されていない場合（もしくは
 * 　　　　　番号に対応するボタンが存在しない場合）は0
 */
extern int sys_input_buttonstate(int button);

/**
 * sys_input_joypos:  ジョイスティック系デバイスのスティック位置をxpos
 * （水平軸）とypos（垂直軸）に格納する。値は、左いっぱい・上いっぱいで-1、
 * 右いっぱい・下いっぱいで+1となる。
 *
 * 【引　数】stick: スティックインデックス（0〜）
 * 　　　　　 xpos: 水平軸の値を格納するポインタ（NULLも可）
 * 　　　　　 ypos: 垂直軸の値を格納するポインタ（NULLも可）
 * 【戻り値】0以外＝成功、0＝失敗（ジョイスティック未接続、インデックスに
 * 　　　　　対応するスティックが存在しないなど）
 */
extern int sys_input_joypos(int stick, float *xpos, float *ypos);

/*************************************************************************/
/******************************* 描画関連 ********************************/
/*************************************************************************/

/*************************** 描画機能関連定数 ****************************/

/**** sys_display_blit()等の描画フラグ ****/

/* BLIT_8BIT: 描画する画像データが8bppのCLUT形式で、色パレットはpalette引数
 * から取る。このフラグが指定されない場合、画像データが32bppである。*/
#define BLIT_8BIT       (1<<0)

/* BLIT_SWIZZLED: 画像データが高速描画用に編集されている（PSPのswizzle等）。
 * 具体的なデータ形式はシステムに依存する。 */
#define BLIT_SWIZZLED   (1<<1)

/* BLIT_BLEND_ADD: 画像データの各成分を画面データに加算する。具体的に、画像
 * （ソース）データを(As,Rs,Gs,Bs)、画面（デスティネーション）データを
 * (Ad,Rd,Gd,Bd)とした場合、以下のように描画する。
 *     Rd = (As*Rs) + Rd
 *     Gd = (As*Rs) + Gd
 *     Bd = (As*Rs) + Bd
 * このフラグを指定しない場合、通常のアルファブレンド処理を行う。 */
#define BLIT_BLEND_ADD  (1<<2)

/**** sys_display_blit_list()に渡す座標データ構造体 ****/

typedef struct SysBlitList_ {
    int16_t srcx, srcy; // データ座標（ピクセル）
    int16_t srcw, srch; // データサイズ（ピクセル）
    struct {            // 4頂点のデータ（左上・右上・左下・右下の順）
        float x, y;     // ├ この頂点の描画先座標（ピクセル）
        uint32_t color; // └ この頂点の濃度・変色効果（0xAARRGGBB）
    } dest[4];
} SysBlitList;

/***************************** 基本描画関数 ******************************/

/**
 * sys_display_start:  新しいフレームを描画する準備を行う。前のフレームで
 * 描画されたデータが画面に残っているかどうかは不定。
 *
 * 【引　数】width_ret, height_ret: 画面のサイズを格納する変数へのポインタ
 * 　　　　　                       　（ピクセル単位、NULL可）
 * 【戻り値】なし
 */
extern void sys_display_start(int *width_ret, int *height_ret);

/**
 * sys_display_finish:  描画されたデータを実際に画面に出力する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sys_display_finish(void);

/**
 * sys_display_sync:  描画処理が全て完了するのを待つ。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sys_display_sync(void);

/*----------------------------------*/

/**
 * sys_display_set_fullscreen:  全画面表示の有無を設定する。ウィンドウ表示の
 * 概念がないシステムでは何もしない。
 *
 * 【引　数】on: 全画面表示フラグ（0以外＝全画面表示）
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int sys_display_set_fullscreen(int on);

/**
 * sys_display_clip:  描画のクリップ範囲を設定する。この範囲以外の描画は
 * 抑制される。フレーム開始時には設定されていない（全画面に描画できる）。
 *
 * 【引　数】    left, top: クリップ範囲の左上隅（ピクセル）
 * 　　　　　width, height: クリップ範囲のサイズ（ピクセル）
 * 【戻り値】なし
 */
extern void sys_display_clip(int left, int top, int width, int height);

/*----------------------------------*/

/**
 * sys_display_clear:  画面全体を黒にクリアする。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sys_display_clear(void);

/**
 * sys_display_blit:  画像データを描画する。色データはBGRA形式（PSP仕様では
 * RGBA形式）とする。自動的にクリップ処理を行う（座標やサイズが画面内に収まる
 * ように調整される）。
 *
 * 【引　数】          src: 描画する画像データ
 * 　　　　　   src_stride: srcのライン幅（ピクセル）
 * 　　　　　   srcx, srcy: 描画する部分の左上隅の座標（ピクセル）
 * 　　　　　 destx, desty: 画像の(srcx,srcy)を描画する画面座標（ピクセル）
 * 　　　　　width, height: 描画する領域の大きさ（ピクセル）
 * 　　　　　      palette: 色パレット（BLIT_8BITを指定する場合は必須）
 * 　　　　　        alpha: 濃度（0〜1、0＝透明）
 * 　　　　　        flags: 動作フラグ（BLIT_*）
 * 【戻り値】なし
 */
extern void sys_display_blit(const void *src, int src_stride, int srcx, int srcy,
                             int destx, int desty, int width, int height,
                             const uint32_t *palette, float alpha,
                             uint32_t flags);

/**
 * sys_display_blit_list:  座標リストを基に画像データを描画する。
 *
 * 【引　数】       src: 描画する画像データ
 * 　　　　　   palette: 色パレット（BLIT_8BITを指定する場合は必須）
 * 　　　　　    stride: srcのライン長（ピクセル）
 * 　　　　　    height: srcの総ライン数
 * 　　　　　 blit_list: 座標リストへのポインタ
 * 　　　　　blit_count: 座標エントリーの数
 * 　　　　　     flags: 動作フラグ（BLIT_*）
 * 【戻り値】なし
 */
extern void sys_display_blit_list(const void *src, const uint32_t *palette,
                                  int stride, int height,
                                  const SysBlitList *blit_list, int blit_count,
                                  uint32_t flags);

/**
 * sys_display_capture:  画面の一部を画像バッファにコピーする。引数無効の
 * 場合、何もせずに失敗を返す。
 *
 * 【引　数】       x, y: キャプチャする領域の左上隅（ピクセル）
 * 　　　　　       w, h: キャプチャする領域のサイズ（ピクセル）
 * 　　　　　       dest: データを格納するバッファへのポインタ
 * 　　　　　     stride: バッファのライン長（ピクセル）
 *               swizzle: 可能であれば、画像データをswizzleすべきかどうか
 * 　　　　　swizzle_ret: 成功時、swizzleの有無を格納する変数へのポインタ
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int sys_display_capture(int x, int y, int w, int h, uint32_t *dest,
                               int stride, int swizzle, int *swizzle_ret);

/**
 * sys_display_line:  指定座標を結んだ直線を指定色で描画する。引数無効の場合、
 * 何もしない。
 *
 * 【引　数】x1, y1: 点1の座標
 * 　　　　　x2, y2: 点2の座標
 * 　　　　　 color: 色と濃度（0xAARRGGBB）
 * 【戻り値】なし
 */
extern void sys_display_line(int x1, int y1, int x2, int y2, uint32_t color);

/**
 * sys_display_fill:  指定領域を指定色でフィルする。x1 > x2 または y1 > y2
 * の場合、何もしない。
 *
 * 【引　数】x1, y1: 左上隅座標
 * 　　　　　x2, y2: 右下隅座標
 * 　　　　　 color: 色と濃度（0xAARRGGBB）
 * 【戻り値】なし
 */
extern void sys_display_fill(int x1, int y1, int x2, int y2, uint32_t color);

/**
 * sys_display_fill_light:  指定領域に指定色の光を加える。各ピクセルの赤成分
 * に r を、緑成分に g を、青成分に b を加算する。x1 > x2 または y1 > y2 の
 * 場合、何もしない。
 *
 * 【引　数】 x1, y1: 左上隅座標
 * 　　　　　 x2, y2: 右下隅座標
 * 　　　　　r, g, b: 色補正値（それぞれ-255〜255）
 * 【戻り値】なし
 */
extern void sys_display_fill_light(int x1, int y1, int x2, int y2,
                                   int r, int g, int b);

/*************************************************************************/
/******************************* 音声関連 ********************************/
/*************************************************************************/

/********************** 音声機能関連定数とデータ型 ***********************/

/**
 * SoundTrigCallback:  sys_sound_settrig()で使われる音声終了トリガー関数の型。
 *
 * 【引　数】channel: 再生が終了したチャンネル
 * 【戻り値】なし
 */
typedef void (*SoundTrigCallback)(int channel);

/***************************** 音声制御全般 ******************************/

/**
 * sys_sound_pause:  音声出力を停止する。pause()から呼び出される。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sys_sound_pause(void);

/**
 * sys_sound_unpause:  音声出力を再開する。unpause()から呼び出される。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sys_sound_unpause(void);

/**
 * sys_sound_lock:  全音声処理を一時停止する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sys_sound_lock(void);

/**
 * sys_sound_unlock:  全音声処理の一時停止を解除する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sys_sound_unlock(void);

/*********************** 音声再生・チャンネル制御 ************************/

/* 音声データ形式。値はWAVファイルなどで使われる一般的なもの */
typedef enum SysSoundFormat_ {
    SYS_SOUND_FORMAT_PCM = 0x0001,  // S16LEのみ
    SYS_SOUND_FORMAT_MP3 = 0x0055,
    SYS_SOUND_FORMAT_OGG = 0x674F,
} SysSoundFormat;

/**
 * sys_sound_checkformat:  指定された音声データ形式がサポートされているか
 * どうかを返す。
 *
 * 【引　数】format: 音声データ形式（SOUND_FORMAT_*）
 * 【戻り値】0以外＝サポートされている、0＝サポートされていない
 */
extern int sys_sound_checkformat(SoundFormat format);

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
extern int sys_sound_setdata(int channel, SoundFormat format,
                             const void *data, const uint32_t datalen,
                             uint32_t loopstart, int32_t looplen);

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
extern int sys_sound_setfile(int channel, SoundFormat format, SysFile *fp,
                             uint32_t dataofs, uint32_t datalen,
                             uint32_t loopstart, int32_t looplen);

/**
 * sys_sound_settrig:  指定されたチャンネルに終了トリガー関数を設定する。
 * 音声の再生が開始された後、再生が終了した時、または他の理由で再生が停止
 * された時に呼び出され、解除される。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 　　　　　   func: 設定する関数（NULLで解除）
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int sys_sound_settrig(int channel, SoundTrigCallback func);

/**
 * sys_sound_setvol:  指定されたチャンネルの音量を設定する。フェード効果が
 * 設定されている場合、その効果が解除される。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 　　　　　 volume: 音量（0〜∞、1＝通常音量）
 * 【戻り値】なし
 */
extern void sys_sound_setvol(int channel, float volume);

/**
 * sys_sound_setpan:  指定されたチャンネルの音声位置を設定する。ステレオ音声
 * データを再生する場合は無効。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 　　　　　    pan: 音声位置（左・-1〜0〜1・右）
 * 【戻り値】なし
 */
extern void sys_sound_setpan(int channel, float pan);

/**
 * sys_sound_setfade:  指定されたチャンネルの音量変化（フェード）設定を行う。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 　　　　　 target: フェード終了時の音量（0〜∞、1＝通常音量）
 * 　　　　　   time: フェード時間（秒）
 * 　　　　　    cut: 0以外＝音量が0（無音）になった時点で再生を停止する
 * 　　　　　         　　0＝音量に関係なく再生を継続する
 * 【戻り値】なし
 */
extern void sys_sound_setfade(int channel, float target, float time, int cut);

/**
 * sys_sound_start:  指定されたチャンネルに設定された音声データを初めから
 * 再生する。データが設定されていない場合は何もしない。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】なし
 */
extern void sys_sound_start(int channel);

/**
 * sys_sound_stop:  指定されたチャンネルの再生を停止する。実際に再生が終了
 * するまで戻らない。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】なし
 */
extern void sys_sound_stop(int channel);

/**
 * sys_sound_resume:  指定されたチャンネルの再生を再開する。基本的には
 * sys_sound_start()と同様だが、sys_sound_stop()後は、停止位置から再生を
 * 開始する。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】なし
 */
extern void sys_sound_resume(int channel);

/**
 * sys_sound_reset:  指定されたチャンネルの再生を停止し、データをクリアする。
 * 実際に再生が終了するまで戻らない。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】なし
 */
extern void sys_sound_reset(int channel);

/**
 * sys_sound_status:  指定されたチャンネルが再生中かどうかを返す。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】0以外＝再生中、0＝再生中でない
 */
extern int sys_sound_status(int channel);

/**
 * sys_sound_position:  指定されたチャンネルの再生位置を秒単位で返す。
 * 再生中でない場合は0を返す。
 *
 * 【引　数】channel: 音声チャンネル（0〜SOUND_CHANNELS-1）
 * 【戻り値】再生位置（秒）
 */
extern float sys_sound_position(int channel);

/*************************************************************************/
/************************* ファイルアクセス関連 **************************/
/*************************************************************************/

/* ファイルアクセスに使うポインタ型。stdioのFILEに当たるもので、内部構造は
 * 隠しておく。 */
//typedef struct SysFile_ SysFile;  // common.hへ

/* 非同期読み込みの最大同時実行数 */
#define MAX_ASYNC_READS  200

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
extern SysFile *sys_file_open(const char *file);

/**
 * sys_file_dup:  ファイルハンドルを複製する。
 *
 * 【引　数】fp: ファイルポインタ
 * 【戻り値】複製したファイルハンドルへのポインタ（失敗した場合はNULL）
 */
extern SysFile *sys_file_dup(SysFile *fp);

/**
 * sys_file_size:  ファイルのサイズを取得する。
 *
 * 【引　数】fp: ファイルポインタ
 * 【戻り値】ファイルのサイズ（バイト）
 * 【注　意】有効なファイルポインタを渡された場合、この関数は失敗しない。
 */
extern PURE_FUNCTION uint32_t sys_file_size(SysFile *fp);

/**
 * sys_file_seek:  ファイルの読み込み位置を変更し、新しい位置を返す。
 *
 * 【引　数】 fp: ファイルポインタ
 * 　　　　　pos: 新しい読み込み位置
 * 　　　　　how: 設定方法（FILE_SEEK_*）
 * 【戻り値】新しい読み込み位置（ファイルの先頭からのバイト数）。エラーが
 * 　　　　　発生した場合は-1を返す
 */
extern int32_t sys_file_seek(SysFile *fp, int32_t pos, int how);
enum {
    FILE_SEEK_SET = 0,  // 絶対値として設定
    FILE_SEEK_CUR,      // 現在位置を基準に相対的に設定
    FILE_SEEK_END,      // ファイル終端を基準に相対的に設定
};

/**
 * sys_file_read:  ファイルからデータを読み込む。
 *
 * 【引　数】 fp: ファイルポインタ
 * 　　　　　buf: データを読み込むバッファ
 * 　　　　　len: 読み込むデータ量（バイト）
 * 【戻り値】読み込んだバイト数。読み込み中にエラーが発生した場合は-1を返す
 * 　　　　　（EOFはエラーとはみなさない）
 */
extern int32_t sys_file_read(SysFile *fp, void *buf, int32_t len);

/**
 * sys_file_read_async:  ファイルからデータの非同期読み込みを開始し、すぐに
 * 戻る。sys_file_wait_async()を呼び出すまでバッファを参照してはならない。
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
extern int sys_file_read_async(SysFile *fp, void *buf, int32_t len, int32_t filepos);

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
extern int sys_file_poll_async(int request);

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
extern int32_t sys_file_wait_async(int request);

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
extern int sys_file_abort_async(int request);

/**
 * sys_file_close:  ファイルを閉じる。fp==NULLの場合、何もしない。
 *
 * 【引　数】fp: ファイルポインタ
 * 【戻り値】なし
 */
extern void sys_file_close(SysFile *fp);

/*************************************************************************/
/****************** セーブデータ・設定データの読み書き *******************/
/*************************************************************************/

/*
 * 注：セーブファイル・設定ファイル読み書き関数は、重複して呼び出されない
 * 　　（一つの操作を実行中、他の関数が呼び出されることはない）。
 */

/*************************************************************************/

/**
 * sys_savefile_load:  セーブファイルのデータを読み込む。データ量がバッファ
 * サイズより多い場合、失敗となる。
 *
 * 操作の結果は、0以外＝成功（総データ量、バイト単位。sizeより大きい場合あ
 * り）、0＝失敗。
 *
 * 【引　数】      num: セーブファイル番号（1〜MAX_SAVE_FILES、SAVE_FILE_*）
 * 　　　　　      buf: データを読み込むバッファ
 * 　　　　　     size: バッファサイズ（バイト）
 * 　　　　　image_ptr: セーブデータに関連づけられた画像データ（Texture構造
 * 　　　　　           　体）へのポインタを格納する変数へのポインタ（NULL
 * 　　　　　           　可）。画像データがない場合、NULLが格納される。
 * 　　　　　           　データの破棄はtexture_destroy()で。
 * 【戻り値】0以外＝成功（操作開始）、0＝失敗
 */
extern int sys_savefile_load(int num, void *buf, int32_t size, Texture **image_ptr);

/**
 * sys_savefile_save:  セーブデータをセーブファイルに書き込む。
 *
 * 操作の結果は、0以外＝成功、0＝失敗。
 *
 * 【引　数】     num: セーブファイル番号（1〜MAX_SAVE_FILES、SAVE_FILE_*）
 * 　　　　　    data: データバッファ
 * 　　　　　data_len: データ長（バイト）
 * 　　　　　    icon: セーブファイルに関連づけるアイコンデータ（形式は
 * 　　　　　          　システム依存。NULLも可）
 * 　　　　　icon_len: アイコンデータのデータ長（バイト）
 * 　　　　　   title: セーブデータタイトル文字列
 * 　　　　　saveinfo: セーブデータ情報文字列（NULL可）
 * 【戻り値】0以外＝成功（操作開始）、0＝失敗
 */
extern int sys_savefile_save(int num, const void *data, int32_t data_len,
                             const void *icon, int32_t icon_len,
                             const char *title, const char *saveinfo);

/**
 * sys_savefile_status:  前回のセーブ・設定ファイル操作の終了状況と結果を返す。
 * 操作の結果については、各関数のドキュメントを参照。
 *
 * 【引　数】result_ret: 操作終了時、結果を格納する変数へのポインタ（NULL可）
 * 【戻り値】0以外＝操作終了、0＝操作実行中
 */
extern int sys_savefile_status(int32_t *result_ret);

/*************************************************************************/
/*************************** メモリ管理・操作 ****************************/
/*************************************************************************/

/**
 * sys_mem_init:  メモリ管理機能（システム依存部分）を初期化する。
 *
 * 【引　数】    main_pool_ret: 基本メモリプールへのポインタを格納する変数
 * 　　　　　main_poolsize_ret: 基本メモリプールのサイズを格納する変数
 * 　　　　　    temp_pool_ret: 基本メモリプールへのポインタを格納する変数
 * 　　　　　temp_poolsize_ret: 基本メモリプールのサイズを格納する変数
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int sys_mem_init(void **main_pool_ret, uint32_t *main_poolsize_ret,
                        void **temp_pool_ret, uint32_t *temp_poolsize_ret);

/**
 * sys_mem_fill8:  メモリ領域を指定された8ビット値でフィルする。アライメント
 * による最適化は考慮しなくて良い（sys_mem_fill32()が使えない場合のみ呼び出さ
 * れる）。
 *
 * 【引　数】ptr: フィルするメモリ領域へのポインタ
 * 　　　　　val: フィル値
 * 　　　　　len: フィルするバイト数
 * 【戻り値】なし
 */
extern void sys_mem_fill8(void *ptr, uint8_t val, uint32_t len);

/**
 * sys_mem_fill32:  メモリ領域を指定された32ビット値でフィルする。
 *
 * 【引　数】ptr: フィルするメモリ領域へのポインタ（必ずアラインされている）
 * 　　　　　val: フィル値
 * 　　　　　len: フィルするバイト数（必ず4以上、4の倍数）
 * 【戻り値】なし
 */
extern void sys_mem_fill32(void *ptr, uint32_t val, uint32_t len);

/*************************************************************************/
/***************************** 時間計測関連 ******************************/
/*************************************************************************/

/**
 * sys_time_base_framerate:  システムの基本フレームレートを返す。
 *
 * 【引　数】なし
 * 【戻り値】基本フレームレート（秒／フレーム）
 */
extern CONST_FUNCTION double sys_time_base_framerate(void);

/**
 * sys_time_now:  現在の時刻をなるべく高精度で返す。戻り値の単位は秒だが、
 * 数値自体の意味はシステム依存で、他の時刻との比較にのみ使える。
 *
 * 【引　数】なし
 * 【戻り値】現在の時刻（秒単位、小数点以下も有意）
 */
extern double sys_time_now(void);

/**
 * sys_time_delay:  指定の秒数だけプログラムの実行を停止する。指定の時間より
 * 短く停止することはないが、システムによっては長く停止する場合がある。0を
 * 指定すると停止しないが、割り込みの確認など、定期的に行う必要のあるシステ
 * ム依存処理は行う。
 *
 * 【引　数】time: 停止する時間（秒単位、60秒未満。小数点以下も有意）
 * 【戻り値】なし
 * 【注　意】60秒以上の停止時間を指定した場合の動作は不定。
 */
extern void sys_time_delay(double time);

/*************************************************************************/
/******************************** その他 *********************************/
/*************************************************************************/

/**
 * sys_set_performance:  システムの処理速度を設定する。
 *
 * 【引　数】level: 処理速度（SYS_PERFORMANCE_*）
 * 【戻り値】なし
 * 【注　意】関数はすぐに戻らず、実行に多少時間がかかる可能性がある。
 */
extern void sys_set_performance(int level);
enum {
    SYS_PERFORMANCE_NORMAL = 0,  // デフォルト設定
    SYS_PERFORMANCE_HIGH   = 1,  // 高速処理
    SYS_PERFORMANCE_LOW    = 2,  // 低速処理（省エネ）
};

/**
 * sys_report_error:  エラーの発生をユーザに知らせる。
 *
 * 【引　数】message:  エラーメッセージ
 * 【戻り値】なし
 */
extern void sys_report_error(const char *message);

/**
 * sys_last_error:  システム依存関数で、直前に発生したエラーの種類を返す。
 * エラー直後以外の時に呼び出した場合の動作は不定。
 *
 * 【引　数】なし
 * 【戻り値】エラー値（SYSERR_*）
 */
extern int sys_last_error(void);
enum {
    SYSERR_NO_ERROR = 0,
    SYSERR_UNKNOWN_ERROR,       // 不明なエラー
    SYSERR_FILE_NOT_FOUND,      // ファイルが存在しない
    SYSERR_FILE_ACCESS_DENIED,  // ファイルにアクセスできない
    SYSERR_FILE_ASYNC_READING,  // 非同期読み込み中
    SYSERR_FILE_ASYNC_ABORTED,  // 非同期読み込みが中止された
    SYSERR_FILE_ASYNC_NONE,     // 指定された非同期読み込み要求が存在しない
};

/**
 * sys_last_errstr:  システム依存関数で、直前に発生したエラーの説明文字列を
 * 返す。エラー直後以外の時に呼び出した場合の動作は不定（ただし戻り値は有効
 * な文字列ポインタとなる）。
 *
 * 【引　数】なし
 * 【戻り値】エラー説明文字列
 */
extern const char *sys_last_errstr(void);

/**
 * sys_ping:  スクリーンセーバー等のタイマーをリセットする。ゲームのイベント
 * シーン等、ユーザが長時間操作せずに画面を見ている時にスクリーンセーバーが
 * 入ってしまうのを回避するため、定期的に（例えば1フレーム毎に）呼び出す。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void sys_ping(void);

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
extern void sys_DMSG(const char *format, va_list args);

#endif  // DEBUG

/*************************************************************************/
/*************************************************************************/

#endif  // SYSDEP_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
