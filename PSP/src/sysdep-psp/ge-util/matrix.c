/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/matrix.c: 3D coordinate transformation matrix
 * manipulation routines for the GE utility library.
 */

#include "../../common.h"
#include "../ge-util.h"
#include "../psplocal.h"

#include "ge-const.h"
#include "ge-local.h"

/*************************************************************************/
/*************************************************************************/

/**
 * ge_set_projection_matrix:  投影座標変換行列を設定する。
 *
 * 【引　数】matrix: 設定する投影座標変換行列
 * 【戻り値】なし
 */
void ge_set_projection_matrix(const Matrix4f *matrix)
{
    if (UNLIKELY(!matrix)) {
        DMSG("matrix == NULL");
        return;
    }
    CHECK_GELIST(17);

    internal_add_command(GECMD_PROJ_START, 0);
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            internal_add_commandf(GECMD_PROJ_UPLOAD, matrix->m[i][j]);
        }
    }
}

/*************************************************************************/

/**
 * ge_set_view_matrix:  視点座標変換行列を設定する。
 *
 * 【引　数】matrix: 設定する視点座標変換行列
 * 【戻り値】なし
 */
void ge_set_view_matrix(const Matrix4f *matrix)
{
    if (UNLIKELY(!matrix)) {
        DMSG("matrix == NULL");
        return;
    }
    CHECK_GELIST(13);

    internal_add_command(GECMD_VIEW_START, 0);
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            internal_add_commandf(GECMD_VIEW_UPLOAD, matrix->m[i][j]);
        }
    }
}

/*************************************************************************/

/**
 * ge_set_model_matrix:  モデル座標変換行列を設定する。
 *
 * 【引　数】matrix: 設定するモデル座標変換行列
 * 【戻り値】なし
 */
void ge_set_model_matrix(const Matrix4f *matrix)
{
    if (UNLIKELY(!matrix)) {
        DMSG("matrix == NULL");
        return;
    }
    CHECK_GELIST(13);

    internal_add_command(GECMD_MODEL_START, 0);
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            internal_add_commandf(GECMD_MODEL_UPLOAD, matrix->m[i][j]);
        }
    }
}

/*************************************************************************/

/**
 * ge_set_texture_matrix:  テキスチャ座標変換行列を設定する。
 *
 * 【引　数】matrix: 設定するテキスチャ座標変換行列
 * 【戻り値】なし
 */
void ge_set_texture_matrix(const Matrix4f *matrix)
{
    if (UNLIKELY(!matrix)) {
        DMSG("matrix == NULL");
        return;
    }
    CHECK_GELIST(13);

    internal_add_command(GECMD_TEXTURE_START, 0);
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            internal_add_commandf(GECMD_TEXTURE_UPLOAD, matrix->m[i][j]);
        }
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
