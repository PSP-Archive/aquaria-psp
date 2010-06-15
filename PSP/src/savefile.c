/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/savefile.c: Save file management routines.
 */

#include "common.h"
#include "savefile.h"
#include "sysdep.h"

/*
 * 注：現時点では単なるsysdepへの中継モジュールに過ぎない。
 */

/*************************************************************************/

/**
 * savefile_load:  セーブファイルのデータを読み込む。データ量がバッファ
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
int savefile_load(int num, void *buf, int32_t size, Texture **image_ptr)
{
    if (UNLIKELY((num < 1 || num > MAX_SAVE_FILES)
                 && num != SAVE_FILE_CONFIG
                 && num != SAVE_FILE_STATS)
     || UNLIKELY(!buf)
     || UNLIKELY(size <= 0)
    ) {
        DMSG("Invalid parameters: %d %p %d %p", num, buf, size, image_ptr);
        return 0;
    }

    return sys_savefile_load(num, buf, size, image_ptr);
}

/*-----------------------------------------------------------------------*/

/**
 * savefile_save:  セーブデータをセーブファイルに書き込む。
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
int savefile_save(int num, const void *data, int32_t data_len,
		  const void *icon, int32_t icon_len,
                  const char *title, const char *saveinfo)
{
    if (UNLIKELY((num < 1 || num > MAX_SAVE_FILES)
                 && num != SAVE_FILE_CONFIG
                 && num != SAVE_FILE_STATS)
     || UNLIKELY(!data)
     || UNLIKELY(data_len <= 0)
     || UNLIKELY(icon && icon_len <= 0)
     || UNLIKELY(!title)
    ) {
        DMSG("Invalid parameters: %d %p %d %p %d %p[%s] %p[%s]", num,
             data, data_len, icon, icon_len, title, title ? title : "",
             saveinfo, saveinfo ? saveinfo : "");
        return 0;
    }

    return sys_savefile_save(num, data, data_len, icon, icon_len,
                             title, saveinfo);
}

/*-----------------------------------------------------------------------*/

/**
 * savefile_status:  前回のセーブ・設定ファイル操作の終了状況と結果を返す。
 * 操作の結果については、各関数のドキュメントを参照。
 *
 * 【引　数】result_ret: 操作終了時、結果を格納する変数へのポインタ（NULL可）
 * 【戻り値】0以外＝操作終了、0＝操作実行中
 */
int savefile_status(int32_t *result_ret)
{
    return sys_savefile_status(result_ret);
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
