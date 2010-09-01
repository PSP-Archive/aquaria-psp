/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/save.c: PSP save file manipulation interface.
 */

/*
 * PSPでのセーブデータの取り扱いは、基本的にファームウェアのセーブデータ・
 * ユーティリティ（sceUtilitySavedata*()関数）を使って行う。設定データも、
 * 特殊な名前を付けたセーブファイルとして保存する。PSPではセーブファイルに
 * アイコンを付けることができるので、セーブ時に画面イメージ（スクリーンショ
 * ット）を縮小してアイコンとして保存し、ロード時にもアイコンデータを画像に
 * 変換してスクリーンショットとして返す。（圧縮に時間がかかったり、外部ライ
 * ブラリが必要になるのを避けるため、アイコンは非圧縮のPNGファイルとして保
 * 存する。）
 */

#include "../common.h"
#include "../memory.h"
#include "../savefile.h"
#include "../sysdep.h"
#include "../texture.h"

#include "psplocal.h"

/*************************************************************************/

/* セーブファイルのパス名生成に使う文字列 */
#define PATH_SAVEROOT       "ms0:/PSP/SAVEDATA"
#define PATH_GAMENAME       "GAME00000"
#define PATH_SAVEDIR_PREFIX "Aquaria_"
#define PATH_SAVEDIR_FMT    PATH_SAVEDIR_PREFIX "%03d"
#define PATH_SAVEFILE       "save.bin"
#define PATH_CONFIGDIR      PATH_SAVEDIR_PREFIX "Settings"
#define PATH_CONFIGFILE     "settings.bin"
#define PATH_STATSDIR       PATH_SAVEDIR_PREFIX "Stats"
#define PATH_STATSFILE      "stats.bin"

/* 共通テキスト情報 */
#define TEXT_GAMETITLE      "Aquaria"

/* ICON0.PNGの必要サイズ（システム的定数） */
#define ICON0_WIDTH  144
#define ICON0_HEIGHT  80

/* ICON0.PNG読み込み時のバッファサイズ */
#define ICON_BUFSIZE  45000

/*-----------------------------------------------------------------------*/

/* 現在の操作 */
static enum {
    NONE = 0,  // 操作中でない
    SAVEFILE_LOAD,
    SAVEFILE_SAVE,
} current_op = NONE;

/* 現在操作中のセーブファイル番号 */
static int current_savenum;

/* 前回の操作の結果（current_op==NONEの時のみ有効） */
static int32_t last_result;

/* セーブデータ読み込み時、呼び出し側の画面イメージ格納用ポインタ */
static Texture **saved_image_ptr;

/*-----------------------------------------------------------------------*/

/* セーブデータユーティリティのシステムコール用パラメータ構造体 */
static SceUtilitySavedataParam saveparams;

/*************************************************************************/

/* ローカル関数宣言 */

static void init_save_params(SceUtilitySavedataParam *params, int mode,
                             int num);
static Texture *unpack_icon0(const uint8_t *icon0, uint32_t icon0_size);

/*************************************************************************/
/************************** インタフェース関数 ***************************/
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
int sys_savefile_load(int num, void *buf, int32_t size, Texture **image_ptr)
{
    PRECOND_SOFT(buf != NULL, return 0);
    PRECOND_SOFT(size > 0, return 0);

    current_savenum = num;

    if (image_ptr) {
        *image_ptr = NULL;  // エラーに備えて
    }
    saved_image_ptr = image_ptr;

    /* システムコールパラメータを設定し、読み込みを開始する */
    init_save_params(&saveparams, PSP_UTILITY_SAVEDATA_AUTOLOAD, num);
    saveparams.dataBuf = buf;
    saveparams.dataBufSize = size;
    if (image_ptr) {
        saveparams.icon0FileData.buf =
            mem_alloc(ICON_BUFSIZE, 0, MEM_ALLOC_TEMP);
        if (UNLIKELY(!saveparams.icon0FileData.buf)) {
            DMSG("No memory for icon0!");
        } else {
            saveparams.icon0FileData.bufSize = ICON_BUFSIZE;
        }
    }
    int res = sceUtilitySavedataInitStart(&saveparams);
    if (UNLIKELY(res < 0)) {
        DMSG("sceUtilitySavedataInitStart(): %s", psp_strerror(res));
        mem_free(saveparams.icon0FileData.buf);
        mem_clear(&saveparams, sizeof(saveparams));
        return 0;
    }

    current_op = SAVEFILE_LOAD;
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_savefile_save:  セーブデータをセーブファイルに書き込む。
 *
 * 操作の結果は、0以外＝成功、0＝失敗。
 *
 * 【引　数】     num: セーブファイル番号（1〜MAX_SAVE_FILES、SAVE_FILE_*）
 * 　　　　　    data: データバッファ
 * 　　　　　data_len: データ長（バイト）
 * 　　　　　    icon: セーブファイルに関連づけるアイコンデータ（PNG形式、
 * 　　　　　          　144x80ピクセル。NULLも可）
 * 　　　　　icon_len: アイコンデータのデータ長（バイト）
 * 　　　　　   title: セーブデータタイトル文字列
 * 　　　　　saveinfo: セーブデータ情報文字列（NULL可）
 * 【戻り値】0以外＝成功（操作開始）、0＝失敗
 */
int sys_savefile_save(int num, const void *data, int32_t data_len,
                      const void *icon, int32_t icon_len,
                      const char *title, const char *saveinfo)
{
    PRECOND_SOFT(data != NULL, return 0);
    PRECOND_SOFT(data_len > 0, return 0);
    PRECOND_SOFT(icon == NULL || icon_len > 0, return 0);
    PRECOND_SOFT(title != NULL, return 0);

    current_savenum = num;

    /* 一時バッファを確保してデータをコピーする */
    void *databuf = mem_alloc(data_len, 0, MEM_ALLOC_TEMP);
    if (!databuf) {
        DMSG("No memory for copy of save data (%d bytes)", data_len);
        return 0;
    }
    memcpy(databuf, data, data_len);
    void *iconbuf = NULL;
    if (icon) {
        iconbuf = mem_alloc(icon_len, 0, MEM_ALLOC_TEMP);
        if (!iconbuf) {
            DMSG("No memory for copy of icon data (%d bytes),"
                 " continuing anyway", icon_len);
        } else {
            memcpy(iconbuf, icon, icon_len);
        }
    }

    /* セーブパラメータを設定する */
    init_save_params(&saveparams, PSP_UTILITY_SAVEDATA_AUTOSAVE, num);
    saveparams.dataBuf = databuf;
    saveparams.dataBufSize = data_len;
    saveparams.dataSize = data_len;
    snprintf(saveparams.sfoParam.title, sizeof(saveparams.sfoParam.title),
             "%s", TEXT_GAMETITLE);
    snprintf(saveparams.sfoParam.savedataTitle,
             sizeof(saveparams.sfoParam.savedataTitle), "%s", title);
    snprintf(saveparams.sfoParam.detail, sizeof(saveparams.sfoParam.detail),
             "%s", saveinfo ? saveinfo : "");
    saveparams.sfoParam.parentalLevel = 1;
    saveparams.icon0FileData.buf = iconbuf;
    saveparams.icon0FileData.bufSize = icon_len;
    saveparams.icon0FileData.size = icon_len;

    /* セーブを開始する */
    int res = sceUtilitySavedataInitStart(&saveparams);
    if (UNLIKELY(res < 0)) {
        DMSG("sceUtilitySavedataInitStart(): %s", psp_strerror(res));
        mem_free(saveparams.dataBuf);
        mem_clear(&saveparams, sizeof(saveparams));
        return 0;
    }

    current_op = SAVEFILE_SAVE;
    return 1;
}

/*************************************************************************/

/**
 * sys_savefile_status:  前回のセーブ・設定ファイル操作の終了状況と結果を返す。
 * 操作の結果については、各関数のドキュメントを参照。
 *
 * 【引　数】result_ret: 操作終了時、結果を格納する変数へのポインタ（NULL可）
 * 【戻り値】0以外＝操作終了、0＝操作実行中
 */
int sys_savefile_status(int32_t *result_ret)
{
    if (current_op == NONE) {
        if (result_ret) {
            *result_ret = last_result;
        }
        return 1;
    }

    int res = sceUtilitySavedataGetStatus();
    if (res >= 1 && res <= 3) {
        if (res == 2) {
            sceUtilitySavedataUpdate(1);
        } else if (res == 3) {
            sceUtilitySavedataShutdownStart();
        }
        return 0;
    }
    if (res >= 0) {
        res = saveparams.base.result;
    }

    switch (current_op) {

      case NONE:
        DMSG("Impossible: current_op==NONE");
        break;

      case SAVEFILE_LOAD:
        if (res < 0) {
            DMSG("Save file read failed: %s", psp_strerror(res));
            last_result = 0;
        } else {
            last_result = saveparams.dataSize;
            if (saved_image_ptr && saveparams.icon0FileData.buf
             && saveparams.icon0FileData.size > 0
            ) {
                *saved_image_ptr =
                    unpack_icon0(saveparams.icon0FileData.buf,
                                 saveparams.icon0FileData.size);
            }
        }
        mem_free(saveparams.icon0FileData.buf);
        break;

      case SAVEFILE_SAVE:
        if (res < 0) {
            DMSG("Save file write failed: %s", psp_strerror(res));
            last_result = 0;
        } else {
            last_result = 1;
        }
        mem_free(saveparams.dataBuf);
        mem_free(saveparams.icon0FileData.buf);
        break;

    }  // switch (current_op)

    mem_clear(&saveparams, sizeof(saveparams));
    current_op = NONE;
    if (result_ret) {
        *result_ret = last_result;
    }
    return 1;
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * init_save_params:  セーブデータユーティリティのパラメータ構造体を初期化
 * する。構造体をクリアし、構造体サイズ・各スレッドの優先度・動作モード・
 * ゲームID文字列を設定する。
 *
 * 【引　数】params: パラメータ構造体へのポインタ
 * 　　　　　  mode: 動作モード（UTILITY_MODE_*）
 * 　　　　　   num: セーブファイル番号（1〜MAX_SAVE_FILES、SAVE_FILE_*）
 * 【戻り値】なし
 */
static void init_save_params(SceUtilitySavedataParam *params, int mode,
                             int num)
{
    PRECOND_SOFT(params != NULL, return);
    mem_clear(params, sizeof(*params));
    params->base.size = sizeof(*params);
    params->base.graphicsThread = THREADPRI_UTILITY_BASE + 1;
    params->base.accessThread   = THREADPRI_UTILITY_BASE + 3;
    params->base.fontThread     = THREADPRI_UTILITY_BASE + 2;
    params->base.soundThread    = THREADPRI_UTILITY_BASE;
    params->mode = mode;
    params->overwrite = 1;
    snprintf(params->gameName, sizeof(params->gameName), "%s", PATH_GAMENAME);
    switch (num) {
      case SAVE_FILE_CONFIG:
        snprintf(saveparams.saveName, sizeof(saveparams.saveName),
                 "%s", PATH_CONFIGDIR);
        snprintf(saveparams.fileName, sizeof(saveparams.fileName),
                 "%s", PATH_CONFIGFILE);
        break;
      case SAVE_FILE_STATS:
        snprintf(saveparams.saveName, sizeof(saveparams.saveName),
                 "%s", PATH_STATSDIR);
        snprintf(saveparams.fileName, sizeof(saveparams.fileName),
                 "%s", PATH_STATSFILE);
        break;
      default:
        snprintf(saveparams.saveName, sizeof(saveparams.saveName),
                 PATH_SAVEDIR_FMT, num-1);
        snprintf(saveparams.fileName, sizeof(saveparams.fileName),
                 "%s", PATH_SAVEFILE);
        break;
    }
}

/*************************************************************************/

/**
 * unpack_icon0:  PSPセーブファイルのICON0.PNGから画面イメージを取り出す。
 * 画面イメージはtexture_new()で確保されたバッファに格納される。
 *
 * 画面イメージ不要の場合はimage_retをNULLとして構わない。
 *
 * 【引　数】     icon0: ICON0.PNGのデータへのポインタ
 * 　　　　　icon0_size: ICON0.PNGのデータサイズ（バイト）
 * 【戻り値】画像データ（エラーの場合はNULL）
 */
static Texture *unpack_icon0(const uint8_t *icon0, uint32_t icon0_size)
{
    PRECOND_SOFT(icon0 != NULL, return 0);
    const uint8_t * const icon0_top = icon0 + icon0_size;

    Texture *image;

    /* PNGヘッダを確認し、画像サイズを取得する */
    if (UNLIKELY(icon0_size < 33+12)
     || UNLIKELY(memcmp(icon0,"\x89PNG\x0D\x0A\x1A\x0A\0\0\0\x0DIHDR",16) != 0)
     ) {
        DMSG("Invalid PNG format");
        return 0;  // PNG形式ではないので、セーブデータも取得できない
    }
    /* これ以降、PNG形式であることは分かっているので、画像データを処理できなく
     * てもセーブデータを探してみる。特にクリアデータの場合、画面イメージが
     * ないのでPNG画像の形式が通常と異なる */
    if (UNLIKELY(memcmp(icon0+24, "\x08\x02\x00\x00\x00", 5) != 0)) {
        DMSG("Unsupported image format");
        return NULL;
    }
    int32_t width  = icon0[16]<<24 | icon0[17]<<16 | icon0[18]<<8 | icon0[19];
    int32_t height = icon0[20]<<24 | icon0[21]<<16 | icon0[22]<<8 | icon0[23];
    if (UNLIKELY(width  <= 0) || UNLIKELY(width  > ICON0_WIDTH)
     || UNLIKELY(height <= 0) || UNLIKELY(height > ICON0_HEIGHT)
    ) {
        DMSG("Invalid width/height %dx%d", width, height);
        return NULL;
    }

    /* PNGファイル内の画像データの位置を検索する */
    icon0 += 33;
    while (memcmp(icon0+4, "IDAT", 4) != 0) {
        const uint32_t chunksize =
            icon0[0]<<24 | icon0[1]<<16 || icon0[2]<<8 | icon0[3];
        icon0 += 12 + chunksize;
        if (UNLIKELY(icon0+12 > icon0_top)) {
            DMSG("IDAT chunk not found");
            return NULL;
        }
    }
    const uint8_t * const idat_end =
        icon0 + 12 + (icon0[0]<<24 | icon0[1]<<16 | icon0[2]<<8 | icon0[3]);
    if (UNLIKELY(idat_end > icon0_top)) {
        DMSG("Image data truncated");
        return NULL;
    }
    icon0 += 8;
    if (UNLIKELY(memcmp(icon0, "\x78\x01", 2) != 0)) {
        DMSG("Invalid compression signature 0x%02X 0x%02X",
             icon0[0], icon0[1]);
        return NULL;
    }
    icon0 += 2;

    /* 画像バッファを確保する */
    image = texture_new(width, height, MEM_ALLOC_TEMP);
    if (UNLIKELY(!image)) {
        DMSG("Failed to allocate %dx%d image", width, height);
        return NULL;
    }

    /* 画像データをバッファに格納する */
    int y;
    for (y = 0; y < height; y++) {
        if (UNLIKELY(icon0[0] != (y==height-1 ? 0x01 : 0x00))) {
            DMSG("Row %d: invalid block header 0x%02X", y, icon0[0]);
            goto error_free_image;
        }
        if (UNLIKELY((icon0[1] | icon0[2]<<8) != 1+width*3)) {
            DMSG("Row %d: invalid block size %d (should be %d)",
                 y, icon0[1] | icon0[2]<<8, 1+width*3);
            goto error_free_image;
        }
        if (UNLIKELY(icon0[3] != (~icon0[1] & 0xFF))
         || UNLIKELY(icon0[4] != (~icon0[2] & 0xFF))
        ) {
            DMSG("Row %d: inverted block size is wrong", y);
            goto error_free_image;
        }
        if (UNLIKELY(icon0[5] != 0)) {
            DMSG("Row %d: invalid filter type %d", y, icon0[5]);
            goto error_free_image;
        }
        icon0 += 6;
        uint32_t *dest = &((uint32_t *)image->pixels)[y * image->stride];
        int x;
        for (x = 0; x < width; x++, icon0 += 3, dest++) {
            *dest = icon0[0] | icon0[1]<<8 | icon0[2]<<16 | 0xFF000000;
        }
    }

    /* 終了 */
    return image;

    /* エラー処理。画像バッファ確保後はバッファを解放して、セーブデータ検索
     * に移る */
  error_free_image:
    texture_destroy(image);
    return NULL;
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
