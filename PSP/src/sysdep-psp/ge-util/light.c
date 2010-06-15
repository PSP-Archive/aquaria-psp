/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/light.c: Light source manipulation routines for
 * the GE utility library.
 */

#include "../../common.h"
#include "../ge-util.h"
#include "../psplocal.h"

#include "ge-const.h"
#include "ge-local.h"

/*************************************************************************/
/*************************************************************************/

/**
 * ge_set_light_mode:  光源処理モードを設定する。
 *
 * 【引　数】mode: 光源処理モード（GU_LIGHT_MODE_*）
 * 【戻り値】なし
 */
void ge_set_light_mode(unsigned int mode)
{
    CHECK_GELIST(1);
    internal_add_command(GECMD_LIGHT_MODEL, mode);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_enable_light:  指定された光源を有効にする。
 *
 * 【引　数】light: 有効にする光源インデックス（0〜3）
 * 【戻り値】なし
 */
void ge_enable_light(unsigned int light)
{
    if (light > 3) {
        DMSG("Invalid light source %u", light);
        return;
    }
    CHECK_GELIST(1);
    internal_add_command(GECMD_ENA_LIGHT0 + light, 1);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_disable_light:  指定された光源を無効にする。
 *
 * 【引　数】light: 無効にする光源インデックス（0〜3）
 * 【戻り値】なし
 */
void ge_disable_light(unsigned int light)
{
    if (light > 3) {
        DMSG("Invalid light source %u", light);
        return;
    }
    CHECK_GELIST(1);
    internal_add_command(GECMD_ENA_LIGHT0 + light, 0);
}

/*************************************************************************/

/**
 * ge_set_light_type:  光源の種類を設定する。
 *
 * 【引　数】       light: 光源インデックス（0〜3）
 * 　　　　　        type: 光源の種類（GE_LIGHT_TYPE_*）
 * 　　　　　has_specular: 0以外＝スペキュラーの有無
 * 【戻り値】なし
 */
void ge_set_light_type(unsigned int light, GELightType type,
                       int has_specular)
{
    if (light > 3) {
        DMSG("Invalid light source %u", light);
        return;
    }
    CHECK_GELIST(1);
    internal_add_command(GECMD_LIGHT0_TYPE + light,
                         (type & 3) << 8 | (has_specular ? 1 : 0));
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_light_position:  光源の位置を設定する。
 *
 * 【引　数】  light: 光源インデックス（0〜3）
 * 　　　　　x, y, z: 光源の位置
 * 【戻り値】なし
 */
void ge_set_light_position(unsigned int light, float x, float y, float z)
{
    if (light > 3) {
        DMSG("Invalid light source %u", light);
        return;
    }
    CHECK_GELIST(3);
    internal_add_commandf(GECMD_LIGHT0_XPOS + light*3, x);
    internal_add_commandf(GECMD_LIGHT0_YPOS + light*3, y);
    internal_add_commandf(GECMD_LIGHT0_ZPOS + light*3, z);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_light_direction:  光源の方向を設定する。
 *
 * 【引　数】  light: 光源インデックス（0〜3）
 * 　　　　　x, y, z: 光源の方向
 * 【戻り値】なし
 */
void ge_set_light_direction(unsigned int light, float x, float y, float z)
{
    if (light > 3) {
        DMSG("Invalid light source %u", light);
        return;
    }
    CHECK_GELIST(3);
    internal_add_commandf(GECMD_LIGHT0_XDIR + light*3, x);
    internal_add_commandf(GECMD_LIGHT0_YDIR + light*3, y);
    internal_add_commandf(GECMD_LIGHT0_ZDIR + light*3, z);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_light_attenuation:  光源の減衰率を設定する。
 *
 * 【引　数】    light: 光源インデックス（0〜3）
 * 　　　　　 constant: 定率減衰率
 * 　　　　　   linear: 直線減衰率
 * 　　　　　quadratic: 二乗減衰率
 * 【戻り値】なし
 */
void ge_set_light_attenuation(unsigned int light, float constant,
                              float linear, float quadratic)
{
    if (light > 3) {
        DMSG("Invalid light source %u", light);
        return;
    }
    CHECK_GELIST(3);
    internal_add_commandf(GECMD_LIGHT0_CATT + light*3, constant);
    internal_add_commandf(GECMD_LIGHT0_LATT + light*3, linear);
    internal_add_commandf(GECMD_LIGHT0_QATT + light*3, quadratic);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_light_color:  光源の位置を設定する。
 *
 * 【引　数】    light: 光源インデックス（0〜3）
 * 　　　　　component: 色を設定する成分（GE_LIGHT_COMPONENT_*）
 * 　　　　　    color: 色（0xBBGGRR）
 * 【戻り値】なし
 */
void ge_set_light_color(unsigned int light, unsigned int component,
                        uint32_t color)
{
    if (light > 3) {
        DMSG("Invalid light source %u", light);
        return;
    }
    if (component > 2) {
        DMSG("Invalid component %u", component);
        return;
    }
    CHECK_GELIST(1);
    internal_add_command(GECMD_LIGHT0_ACOL + light*3 + component,
                         color & 0xFFFFFF);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_spotlight_exponent:  スポット光源の指数を設定する。
 *
 * 【引　数】   light: 光源インデックス（0〜3）
 * 　　　　　exponent: スポット指数
 * 【戻り値】なし
 */
void ge_set_spotlight_exponent(unsigned int light, float exponent)
{
    if (light > 3) {
        DMSG("Invalid light source %u", light);
        return;
    }
    CHECK_GELIST(1);
    internal_add_commandf(GECMD_LIGHT0_SPOTEXP + light, exponent);
}

/*-----------------------------------------------------------------------*/

/**
 * ge_set_spotlight_cutoff:  スポット光源の閾値を設定する。
 *
 * 【引　数】 light: 光源インデックス（0〜3）
 * 　　　　　cutoff: スポット閾値
 * 【戻り値】なし
 */
void ge_set_spotlight_cutoff(unsigned int light, float cutoff)
{
    if (light > 3) {
        DMSG("Invalid light source %u", light);
        return;
    }
    CHECK_GELIST(1);
    internal_add_commandf(GECMD_LIGHT0_SPOTLIM + light, cutoff);
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
