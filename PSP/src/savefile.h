/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/savefile.h: Save file management header.
 */

/*
 * セーブファイルの操作はすべて非同期となる。読み込み・書き込みは各関数を
 * 呼び出すことで開始した後、定期的にsavefile_status()を呼び出し、操作の
 * 完了と結果を確認しなければならない。
 */

#ifndef SAVEFILE_H
#define SAVEFILE_H

/*************************************************************************/

/*
 * MAX_SAVE_FILES:  利用できるセーブファイルの最大数。
 */
#define MAX_SAVE_FILES  100

/*
 * SaveFileSystemID:  システム用セーブファイル（ユーザ設定等）を指定する
 * 識別子。セーブファイル番号の代わりにsavefile_load(), savefile_save()に
 * 渡す。
 */
typedef enum SaveFileSystemID_ {
    SAVE_FILE_CONFIG = -1,  // ユーザ設定
    SAVE_FILE_STATS  = -2,  // 実績データ
} SaveFileSystemID;

/*-----------------------------------------------------------------------*/

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
extern int savefile_load(int num, void *buf, int32_t size, Texture **image_ptr);

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
extern int savefile_save(int num, const void *data, int32_t data_len,
                         const void *icon, int32_t icon_len,
                         const char *title, const char *saveinfo);

/**
 * savefile_status:  前回のセーブ・設定ファイル操作の終了状況と結果を返す。
 * 操作の結果については、各関数のドキュメントを参照。
 *
 * 【引　数】result_ret: 操作終了時、結果を格納する変数へのポインタ（NULL可）
 * 【戻り値】0以外＝操作終了、0＝操作実行中
 */
extern int savefile_status(int32_t *result_ret);

/*************************************************************************/

#endif  // SAVEFILE_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
