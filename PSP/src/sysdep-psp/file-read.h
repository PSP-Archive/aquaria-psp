/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/file-read.h: Header for PSP low-level file reading logic.
 */

#ifndef SYSDEP_PSP_FILE_READ_H
#define SYSDEP_PSP_FILE_READ_H

/*************************************************************************/

/**
 * psp_file_read_init:  ファイル読み込み管理機能を初期化する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int psp_file_read_init(void);

/**
 * psp_file_read_submit:  ファイル読み込み要求を提出する。
 *
 * 【引　数】        fd: ファイルデスクリプタ
 * 　　　　　     start: 読み込み開始位置（ファイル先頭からのバイトオフセット）
 * 　　　　　       len: 読み込むバイト数
 * 　　　　　       buf: 読み込んだデータを格納するバッファ
 * 　　　　　       pri: 読み込み優先度（FILE_READ_PRI_*定数のいずれか）
 * 　　　　　     timed: 0以外＝期限付き要求、0＝即時要求
 * 　　　　　time_limit: 読み込み開始期限（timed==0の場合は無視）
 * 【戻り値】要求識別子（0＝エラー）
 */
extern int psp_file_read_submit(int fd, uint32_t start, uint32_t len, void *buf,
                                int timed, int32_t time_limit);

/**
 * psp_file_read_check:  読み込みが完了しているかどうかを返す。
 *
 * 【引　数】id: 要求識別子
 * 【戻り値】正数＝読み込み完了、0＝読み込み中、負数＝要求識別子不正
 */
extern int psp_file_read_check(int id);

/**
 * psp_file_read_wait:  読み込みが完了するのを待って、結果を返す。
 *
 * 【引　数】id: 要求識別子
 * 【戻り値】0以上＝成功（読み込んだバイト数）、負数＝エラー発生
 */
extern int psp_file_read_wait(int id);

/**
 * psp_file_read_abort:  読み込みを中止する。指定された読み込み要求がすでに
 * 完了している場合、何もしない。
 *
 * 【引　数】id: 要求識別子
 * 【戻り値】0以外＝成功、0＝失敗（識別子不正）
 */
extern int psp_file_read_abort(int id);

/*************************************************************************/

#endif  // SYSDEP_PSP_FILE_READ_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
