/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/base.c: Base functionality for the GE utility
 * library.
 */

#include "../../common.h"
#include "../ge-util.h"
#include "../psplocal.h"

#include "ge-const.h"
#include "ge-local.h"

/*************************************************************************/
/*************************** ライブラリデータ ****************************/
/*************************************************************************/

/* 次に命令を格納するポインタ */
uint32_t *gelist_ptr;

/* 現在のリストの限界アドレス（list + lenof(list)） */
uint32_t *gelist_limit;

/* サブリスト作成時、直前のリストポインタ（メインリスト作成中はともにNULL） */
uint32_t *saved_gelist_ptr, *saved_gelist_limit;

/*-----------------------------------------------------------------------*/

/* 次に頂点を格納するポインタと限界アドレス */
uint32_t *vertlist_ptr;
uint32_t *vertlist_limit;

/*-----------------------------------------------------------------------*/

/* 画面のビット／ピクセル */
int display_bpp;

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* GE命令リスト。キャッシュに合わせるため、64バイトアライメントが必要で、
 * 要素数も64バイト（16個）の倍数にする。 */
static uint32_t ALIGNED(64) gelist[50000];

/* GE命令で使う頂点を格納するバッファ（やはりキャッシュの関係で64バイト
 * アライメント）。 */
static uint32_t ALIGNED(64) vertlist[100000];

/* 各バッファのベースアドレスに非キャッシュビットをセットしたもの（初期化時
 * に設定される） */
static uint32_t *gelist_base, *vertlist_base;

/*-----------------------------------------------------------------------*/

/* GEシステムコールで使うリストID */
static int gelist_id;

/*-----------------------------------------------------------------------*/

/* 描画リスト登録フラグ */
static int frame_started;

/*-----------------------------------------------------------------------*/

#ifdef DEBUG

/* 最近のフレームにおける描画リスト・頂点リストの使用ワード数 */
static int gelist_used, vertlist_used;

/* gelist_used、vertlist_usedの過去最高値 */
static int gelist_used_max, vertlist_used_max;

/* デバッグ時、デバッガーなどにより設定することで、その回数だけge_end_frame()
 * 呼び出し時にGE命令リストと頂点リストをDMSG()で出力する。
 * 注：../display.cにてGE_SYNC_IN_THREADを定義していると、クラッシュする
 * 　　可能性が高い。 */
volatile int dumpflag;

#endif

/*************************************************************************/

/* ローカル関数宣言 */

static void internal_commit(void);

/*************************************************************************/
/***************************** 基本操作関数 ******************************/
/*************************************************************************/

/**
 * ge_init:  GEを初期化する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
int ge_init(void)
{
    /* 初期化プログラム（pspsdkのsceGuInit.cから。一部変更あり） */
    static const uint32_t ge_init_list[] = {
        GECMD_VERTEX_POINTER<<24 | 0x000000,
        GECMD_INDEX_POINTER <<24 | 0x000000,
        GECMD_ADDRESS_BASE  <<24 | 0x000000,
        GECMD_VERTEX_FORMAT <<24 | 0x000000,
        GECMD_UNKNOWN_13    <<24 | 0x000000,
        GECMD_DRAWAREA_LOW  <<24 | 0x000000,
        GECMD_DRAWAREA_HIGH <<24 | 0x000000,
        GECMD_ENA_LIGHTING  <<24 | 0x000000,
        GECMD_ENA_LIGHT0    <<24 | 0x000000,
        GECMD_ENA_LIGHT1    <<24 | 0x000000,
        GECMD_ENA_LIGHT2    <<24 | 0x000000,
        GECMD_ENA_LIGHT3    <<24 | 0x000000,
        GECMD_ENA_ZCLIP     <<24 | 0x000000,
        GECMD_ENA_FACE_CULL <<24 | 0x000000,
        GECMD_ENA_TEXTURE   <<24 | 0x000000,
        GECMD_ENA_FOG       <<24 | 0x000000,
        GECMD_ENA_DITHER    <<24 | 0x000000,
        GECMD_ENA_BLEND     <<24 | 0x000000,
        GECMD_ENA_ALPHA_TEST<<24 | 0x000000,
        GECMD_ENA_DEPTH_TEST<<24 | 0x000000,
        GECMD_ENA_STENCIL   <<24 | 0x000000,
        GECMD_ENA_ANTIALIAS <<24 | 0x000000,
        GECMD_ENA_PATCH_CULL<<24 | 0x000000,
        GECMD_ENA_COLOR_TEST<<24 | 0x000000,
        GECMD_ENA_LOGIC_OP  <<24 | 0x000000,
        GECMD_BONE_OFFSET   <<24 | 0x000000,
        GECMD_BONE_UPLOAD   <<24 | 0x000000,
        GECMD_MORPH_0       <<24 | 0x000000,
        GECMD_MORPH_1       <<24 | 0x000000,
        GECMD_MORPH_2       <<24 | 0x000000,
        GECMD_MORPH_3       <<24 | 0x000000,
        GECMD_MORPH_4       <<24 | 0x000000,
        GECMD_MORPH_5       <<24 | 0x000000,
        GECMD_MORPH_6       <<24 | 0x000000,
        GECMD_MORPH_7       <<24 | 0x000000,
        GECMD_PATCH_SUBDIV  <<24 | 0x000000,
        GECMD_PATCH_PRIM    <<24 | 0x000000,
        GECMD_PATCH_FRONT   <<24 | 0x000000,
        GECMD_MODEL_START   <<24 | 0x000000,
        GECMD_MODEL_UPLOAD  <<24 | 0x3F8000,
        GECMD_MODEL_UPLOAD  <<24 |           0x000000,
        GECMD_MODEL_UPLOAD  <<24 |                     0x000000,
        GECMD_MODEL_UPLOAD  <<24 | 0x000000,
        GECMD_MODEL_UPLOAD  <<24 |           0x3F8000,
        GECMD_MODEL_UPLOAD  <<24 |                     0x000000,
        GECMD_MODEL_UPLOAD  <<24 | 0x000000,
        GECMD_MODEL_UPLOAD  <<24 |           0x000000,
        GECMD_MODEL_UPLOAD  <<24 |                     0x3F8000,
        GECMD_MODEL_UPLOAD  <<24 | 0x000000,
        GECMD_MODEL_UPLOAD  <<24 |           0x000000,
        GECMD_MODEL_UPLOAD  <<24 |                     0x000000,
        GECMD_VIEW_START    <<24 | 0x000000,
        GECMD_VIEW_UPLOAD   <<24 | 0x3F8000,
        GECMD_VIEW_UPLOAD   <<24 |           0x000000,
        GECMD_VIEW_UPLOAD   <<24 |                     0x000000,
        GECMD_VIEW_UPLOAD   <<24 | 0x000000,
        GECMD_VIEW_UPLOAD   <<24 |           0x3F8000,
        GECMD_VIEW_UPLOAD   <<24 |                     0x000000,
        GECMD_VIEW_UPLOAD   <<24 | 0x000000,
        GECMD_VIEW_UPLOAD   <<24 |           0x000000,
        GECMD_VIEW_UPLOAD   <<24 |                     0x3F8000,
        GECMD_VIEW_UPLOAD   <<24 | 0x000000,
        GECMD_VIEW_UPLOAD   <<24 |           0x000000,
        GECMD_VIEW_UPLOAD   <<24 |                     0x000000,
        GECMD_PROJ_START    <<24 | 0x000000,
        GECMD_PROJ_UPLOAD   <<24 | 0x3F8000,
        GECMD_PROJ_UPLOAD   <<24 |           0x000000,
        GECMD_PROJ_UPLOAD   <<24 |                     0x000000,
        GECMD_PROJ_UPLOAD   <<24 |                               0x000000,
        GECMD_PROJ_UPLOAD   <<24 | 0x000000,
        GECMD_PROJ_UPLOAD   <<24 |           0x3F8000,
        GECMD_PROJ_UPLOAD   <<24 |                     0x000000,
        GECMD_PROJ_UPLOAD   <<24 |                               0x000000,
        GECMD_PROJ_UPLOAD   <<24 | 0x000000,
        GECMD_PROJ_UPLOAD   <<24 |           0x000000,
        GECMD_PROJ_UPLOAD   <<24 |                     0x3F8000,
        GECMD_PROJ_UPLOAD   <<24 |                               0x000000,
        GECMD_PROJ_UPLOAD   <<24 | 0x000000,
        GECMD_PROJ_UPLOAD   <<24 |           0x000000,
        GECMD_PROJ_UPLOAD   <<24 |                     0x000000,
        GECMD_PROJ_UPLOAD   <<24 |                               0x3F8000,
        GECMD_TEXTURE_START <<24 | 0x000000,
        GECMD_TEXTURE_UPLOAD<<24 | 0x3F8000,
        GECMD_TEXTURE_UPLOAD<<24 |           0x000000,
        GECMD_TEXTURE_UPLOAD<<24 |                     0x000000,
        GECMD_TEXTURE_UPLOAD<<24 | 0x000000,
        GECMD_TEXTURE_UPLOAD<<24 |           0x3F8000,
        GECMD_TEXTURE_UPLOAD<<24 |                     0x000000,
        GECMD_TEXTURE_UPLOAD<<24 | 0x000000,
        GECMD_TEXTURE_UPLOAD<<24 |           0x000000,
        GECMD_TEXTURE_UPLOAD<<24 |                     0x3F8000,
        GECMD_TEXTURE_UPLOAD<<24 | 0x000000,
        GECMD_TEXTURE_UPLOAD<<24 |           0x000000,
        GECMD_TEXTURE_UPLOAD<<24 |                     0x000000,
        GECMD_XSCALE        <<24 | 0x000000,
        GECMD_YSCALE        <<24 | 0x000000,
        GECMD_ZSCALE        <<24 | 0x000000,
        GECMD_XPOS          <<24 | 0x000000,
        GECMD_YPOS          <<24 | 0x000000,
        GECMD_ZPOS          <<24 | 0x000000,
        GECMD_USCALE        <<24 | 0x3F8000,
        GECMD_VSCALE        <<24 | 0x3F8000,
        GECMD_UOFFSET       <<24 | 0x000000,
        GECMD_VOFFSET       <<24 | 0x000000,
        GECMD_XOFFSET       <<24 | 0x000000,
        GECMD_YOFFSET       <<24 | 0x000000,
        GECMD_SHADE_MODE    <<24 | 0x000000,
        GECMD_REV_NORMALS   <<24 | 0x000000,
        GECMD_COLOR_MATERIAL<<24 | 0x000000,
        GECMD_EMISSIVE_COLOR<<24 | 0x000000,
        GECMD_AMBIENT_COLOR <<24 | 0x000000,
        GECMD_DIFFUSE_COLOR <<24 | 0x000000,
        GECMD_SPECULAR_COLOR<<24 | 0x000000,
        GECMD_AMBIENT_ALPHA <<24 | 0x000000,
        GECMD_SPECULAR_POWER<<24 | 0x000000,
        GECMD_LIGHT_AMBCOLOR<<24 | 0x000000,
        GECMD_LIGHT_AMBALPHA<<24 | 0x000000,
        GECMD_LIGHT_MODEL   <<24 | 0x000000,
        GECMD_LIGHT0_TYPE   <<24 | 0x000000,
        GECMD_LIGHT1_TYPE   <<24 | 0x000000,
        GECMD_LIGHT2_TYPE   <<24 | 0x000000,
        GECMD_LIGHT3_TYPE   <<24 | 0x000000,
        GECMD_LIGHT0_XPOS   <<24 | 0x000000,
        GECMD_LIGHT0_YPOS   <<24 | 0x000000,
        GECMD_LIGHT0_ZPOS   <<24 | 0x000000,
        GECMD_LIGHT1_XPOS   <<24 | 0x000000,
        GECMD_LIGHT1_YPOS   <<24 | 0x000000,
        GECMD_LIGHT1_ZPOS   <<24 | 0x000000,
        GECMD_LIGHT2_XPOS   <<24 | 0x000000,
        GECMD_LIGHT2_YPOS   <<24 | 0x000000,
        GECMD_LIGHT2_ZPOS   <<24 | 0x000000,
        GECMD_LIGHT3_XPOS   <<24 | 0x000000,
        GECMD_LIGHT3_YPOS   <<24 | 0x000000,
        GECMD_LIGHT3_ZPOS   <<24 | 0x000000,
        GECMD_LIGHT0_XDIR   <<24 | 0x000000,
        GECMD_LIGHT0_YDIR   <<24 | 0x000000,
        GECMD_LIGHT0_ZDIR   <<24 | 0x000000,
        GECMD_LIGHT1_XDIR   <<24 | 0x000000,
        GECMD_LIGHT1_YDIR   <<24 | 0x000000,
        GECMD_LIGHT1_ZDIR   <<24 | 0x000000,
        GECMD_LIGHT2_XDIR   <<24 | 0x000000,
        GECMD_LIGHT2_YDIR   <<24 | 0x000000,
        GECMD_LIGHT2_ZDIR   <<24 | 0x000000,
        GECMD_LIGHT3_XDIR   <<24 | 0x000000,
        GECMD_LIGHT3_YDIR   <<24 | 0x000000,
        GECMD_LIGHT3_ZDIR   <<24 | 0x000000,
        GECMD_LIGHT0_CATT   <<24 | 0x000000,
        GECMD_LIGHT0_LATT   <<24 | 0x000000,
        GECMD_LIGHT0_QATT   <<24 | 0x000000,
        GECMD_LIGHT1_CATT   <<24 | 0x000000,
        GECMD_LIGHT1_LATT   <<24 | 0x000000,
        GECMD_LIGHT1_QATT   <<24 | 0x000000,
        GECMD_LIGHT2_CATT   <<24 | 0x000000,
        GECMD_LIGHT2_LATT   <<24 | 0x000000,
        GECMD_LIGHT2_QATT   <<24 | 0x000000,
        GECMD_LIGHT3_CATT   <<24 | 0x000000,
        GECMD_LIGHT3_LATT   <<24 | 0x000000,
        GECMD_LIGHT3_QATT   <<24 | 0x000000,
        GECMD_LIGHT0_SPOTEXP<<24 | 0x000000,
        GECMD_LIGHT1_SPOTEXP<<24 | 0x000000,
        GECMD_LIGHT2_SPOTEXP<<24 | 0x000000,
        GECMD_LIGHT3_SPOTEXP<<24 | 0x000000,
        GECMD_LIGHT0_SPOTLIM<<24 | 0x000000,
        GECMD_LIGHT1_SPOTLIM<<24 | 0x000000,
        GECMD_LIGHT2_SPOTLIM<<24 | 0x000000,
        GECMD_LIGHT3_SPOTLIM<<24 | 0x000000,
        GECMD_LIGHT0_ACOL   <<24 | 0x000000,
        GECMD_LIGHT0_DCOL   <<24 | 0x000000,
        GECMD_LIGHT0_SCOL   <<24 | 0x000000,
        GECMD_LIGHT1_ACOL   <<24 | 0x000000,
        GECMD_LIGHT1_DCOL   <<24 | 0x000000,
        GECMD_LIGHT1_SCOL   <<24 | 0x000000,
        GECMD_LIGHT2_ACOL   <<24 | 0x000000,
        GECMD_LIGHT2_DCOL   <<24 | 0x000000,
        GECMD_LIGHT2_SCOL   <<24 | 0x000000,
        GECMD_LIGHT3_ACOL   <<24 | 0x000000,
        GECMD_LIGHT3_DCOL   <<24 | 0x000000,
        GECMD_LIGHT3_SCOL   <<24 | 0x000000,
        GECMD_FACE_ORDER    <<24 | 0x000000,
        GECMD_DRAW_ADDRESS  <<24 | 0x000000,
        GECMD_DRAW_STRIDE   <<24 | 0x000000,
        GECMD_DEPTH_ADDRESS <<24 | 0x000000,
        GECMD_DEPTH_STRIDE  <<24 | 0x000000,
        GECMD_TEX0_ADDRESS  <<24 | 0x000000,
        GECMD_TEX1_ADDRESS  <<24 | 0x000000,
        GECMD_TEX2_ADDRESS  <<24 | 0x000000,
        GECMD_TEX3_ADDRESS  <<24 | 0x000000,
        GECMD_TEX4_ADDRESS  <<24 | 0x000000,
        GECMD_TEX5_ADDRESS  <<24 | 0x000000,
        GECMD_TEX6_ADDRESS  <<24 | 0x000000,
        GECMD_TEX7_ADDRESS  <<24 | 0x000000,
        GECMD_TEX0_STRIDE   <<24 | 0x040004,
        GECMD_TEX1_STRIDE   <<24 | 0x000000,
        GECMD_TEX2_STRIDE   <<24 | 0x000000,
        GECMD_TEX3_STRIDE   <<24 | 0x000000,
        GECMD_TEX4_STRIDE   <<24 | 0x000000,
        GECMD_TEX5_STRIDE   <<24 | 0x000000,
        GECMD_TEX6_STRIDE   <<24 | 0x000000,
        GECMD_TEX7_STRIDE   <<24 | 0x000000,
        GECMD_CLUT_ADDRESS_L<<24 | 0x000000,
        GECMD_CLUT_ADDRESS_H<<24 | 0x000000,
        GECMD_COPY_S_ADDRESS<<24 | 0x000000,
        GECMD_COPY_S_STRIDE <<24 | 0x000000,
        GECMD_COPY_D_ADDRESS<<24 | 0x000000,
        GECMD_COPY_D_STRIDE <<24 | 0x000000,
        GECMD_TEX0_SIZE     <<24 | 0x000101,
        GECMD_TEX1_SIZE     <<24 | 0x000000,
        GECMD_TEX2_SIZE     <<24 | 0x000000,
        GECMD_TEX3_SIZE     <<24 | 0x000000,
        GECMD_TEX4_SIZE     <<24 | 0x000000,
        GECMD_TEX5_SIZE     <<24 | 0x000000,
        GECMD_TEX6_SIZE     <<24 | 0x000000,
        GECMD_TEX7_SIZE     <<24 | 0x000000,
        GECMD_TEXTURE_MAP   <<24 | 0x000000,
        GECMD_TEXTURE_ENVMAP<<24 | 0x000000,
        GECMD_TEXTURE_MODE  <<24 | 0x000000,
        GECMD_TEXTURE_PIXFMT<<24 | 0x000000,
        GECMD_CLUT_LOAD     <<24 | 0x000000,
        GECMD_CLUT_MODE     <<24 | 0x000000,
        GECMD_TEXTURE_FILTER<<24 | 0x000000,
        GECMD_TEXTURE_WRAP  <<24 | 0x000000,
        GECMD_TEXTURE_BIAS  <<24 | 0x000000,
        GECMD_TEXTURE_FUNC  <<24 | 0x000000,
        GECMD_TEXTURE_COLOR <<24 | 0x000000,
        GECMD_TEXTURE_FLUSH <<24 | 0x000000,
        GECMD_COPY_SYNC     <<24 | 0x000000,
        GECMD_FOG_LIMIT     <<24 | 0x000000,
        GECMD_FOG_RANGE     <<24 | 0x000000,
        GECMD_FOG_COLOR     <<24 | 0x000000,
        GECMD_TEXTURE_SLOPE <<24 | 0x000000,
        GECMD_FRAME_PIXFMT  <<24 | 0x000000,
        GECMD_CLEAR_MODE    <<24 | 0x000000,
        GECMD_CLIP_MIN      <<24 | 0x000000,
        GECMD_CLIP_MAX      <<24 | 0x000000,
        GECMD_CLIP_NEAR     <<24 | 0x000000,
        GECMD_CLIP_FAR      <<24 | 0x000000,
        GECMD_COLORTEST_FUNC<<24 | 0x000000,
        GECMD_COLORTEST_REF <<24 | 0x000000,
        GECMD_COLORTEST_MASK<<24 | 0x000000,
        GECMD_ALPHATEST     <<24 | 0x000000,
        GECMD_STENCILTEST   <<24 | 0x000000,
        GECMD_STENCIL_OP    <<24 | 0x000000,
        GECMD_DEPTHTEST     <<24 | 0x000000,
        GECMD_BLEND_FUNC    <<24 | 0x000000,
        GECMD_BLEND_SRCFIX  <<24 | 0x000000,
        GECMD_BLEND_DSTFIX  <<24 | 0x000000,
        GECMD_DITHER0       <<24 | 0x000000,
        GECMD_DITHER1       <<24 | 0x000000,
        GECMD_DITHER2       <<24 | 0x000000,
        GECMD_DITHER3       <<24 | 0x000000,
        GECMD_LOGIC_OP      <<24 | 0x000000,
        GECMD_DEPTH_MASK    <<24 | 0x000000,
        GECMD_COLOR_MASK    <<24 | 0x000000,
        GECMD_ALPHA_MASK    <<24 | 0x000000,
        GECMD_COPY_S_POS    <<24 | 0x000000,
        GECMD_COPY_D_POS    <<24 | 0x000000,
        GECMD_COPY_SIZE     <<24 | 0x000000,
        GECMD_UNKNOWN_F0    <<24 | 0x000000,
        GECMD_UNKNOWN_F1    <<24 | 0x000000,
        GECMD_UNKNOWN_F2    <<24 | 0x000000,
        GECMD_UNKNOWN_F3    <<24 | 0x000000,
        GECMD_UNKNOWN_F4    <<24 | 0x000000,
        GECMD_UNKNOWN_F5    <<24 | 0x000000,
        GECMD_UNKNOWN_F6    <<24 | 0x000000,
        GECMD_UNKNOWN_F7    <<24 | 0x000000,
        GECMD_UNKNOWN_F8    <<24 | 0x000000,
        GECMD_UNKNOWN_F9    <<24 | 0x000000,
        GECMD_FINISH        <<24 | 0x000000,
        GECMD_END           <<24 | 0x000000,
        GECMD_NOP           <<24 | 0x000000,
        GECMD_NOP           <<24 | 0x000000,
    };

    /* GEを初期化する */
    int listid = sceGeListEnQueue(ge_init_list, NULL, -1, NULL);
    if (listid < 0) {
        DMSG("sceGeListEnQueue() failed: %s", psp_strerror(listid));
        return 0;
    }
    int res = sceGeListSync(listid, PSP_GE_LIST_DONE);
    if (res < 0) {
        DMSG("sceGeListSync() failed: %s", psp_strerror(res));
        return 0;
    }

    /* ライブラリを初期化する */
    gelist_base        = (uint32_t *)((uintptr_t)gelist | 0x40000000);
    gelist_ptr         = gelist_base;
    gelist_limit       = gelist_ptr + lenof(gelist);
    saved_gelist_ptr   = NULL;
    saved_gelist_limit = NULL;
    vertlist_base      = (uint32_t *)((uintptr_t)vertlist | 0x40000000);
    vertlist_ptr       = vertlist_base;
    vertlist_limit     = vertlist_ptr + lenof(vertlist);
    frame_started      = 0;
    display_bpp        = 32;

    return 1;
}

/*************************************************************************/

/**
 * ge_start_frame:  フレームの初期処理を行う。
 *
 * 【引　数】display_mode: 表示モード（GE_PIXFMT_*、負数＝変更なし）
 * 【戻り値】なし
 */
void ge_start_frame(int display_mode)
{
    /* リストポインタをリセットし、GEに登録 */
    gelist_ptr = gelist_base;
    gelist_id = sceGeListEnQueue(gelist, gelist, -1, NULL);
    if (gelist_id < 0) {
        DMSG("sceGeListEnQueue(): %s", psp_strerror(gelist_id));
    }
    frame_started = 1;

    /* 描画・奥行きバッファアドレスとライン長を設定する */
    uint32_t drawbuf_addr = (uint32_t)psp_draw_buffer();
    internal_add_command(GECMD_DRAW_ADDRESS, (drawbuf_addr & 0x00FFFFFF));
    internal_add_command(GECMD_DRAW_STRIDE,  (drawbuf_addr & 0xFF000000)>>8
                                             | DISPLAY_STRIDE);
    uint32_t depthbuf_addr = (uint32_t)psp_depth_buffer();
    internal_add_command(GECMD_DEPTH_ADDRESS, (depthbuf_addr & 0x00FFFFFF));
    internal_add_command(GECMD_DEPTH_STRIDE,  (depthbuf_addr & 0xFF000000)>>8
                                              | DISPLAY_STRIDE);

    /* ピクセル形式を設定する */
    if (display_mode >= 0) {
        internal_add_command(GECMD_FRAME_PIXFMT, display_mode);
        display_bpp = (display_mode==PSP_DISPLAY_PIXEL_FORMAT_8888 ? 32 : 16);
    }

    /* 描画領域を設定する */
    internal_add_command(GECMD_DRAWAREA_LOW, 0 | 0<<10);
    internal_add_command(GECMD_DRAWAREA_HIGH, (DISPLAY_WIDTH-1)
                                            | (DISPLAY_HEIGHT-1)<<10);
    internal_add_commandf(GECMD_XSCALE,  DISPLAY_WIDTH /2);
    internal_add_commandf(GECMD_YSCALE, -DISPLAY_HEIGHT/2);
    internal_add_commandf(GECMD_ZSCALE, -32767.5f);
    internal_add_commandf(GECMD_XPOS, 2048);
    internal_add_commandf(GECMD_YPOS, 2048);
    internal_add_commandf(GECMD_ZPOS, 32767.5f);
    internal_add_command(GECMD_XOFFSET, (2048 - DISPLAY_WIDTH /2) << 4);
    internal_add_command(GECMD_YOFFSET, (2048 - DISPLAY_HEIGHT/2) << 4);

    /* クリッピング領域を解除する（全画面に設定） */
    internal_add_command(GECMD_CLIP_MIN, 0 | 0<<10);
    internal_add_command(GECMD_CLIP_MAX, (DISPLAY_WIDTH-1)
                                       | (DISPLAY_HEIGHT-1)<<10);
    internal_add_command(GECMD_CLIP_NEAR, 0);
    internal_add_command(GECMD_CLIP_FAR, 65535);

    /* 頂点リストポインタもリセットする */
    vertlist_ptr = vertlist_base;
}

/*-----------------------------------------------------------------------*/

/**
 * ge_end_frame:  フレームの終了処理を行う。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void ge_end_frame(void)
{
    if (!frame_started) {
        return;
    }

    if (saved_gelist_ptr) {
        DMSG("Sublist not finished!");
        ge_finish_sublist();
    }

    if (UNLIKELY(gelist_ptr+2 > gelist_limit)) {
        DMSG("WARNING: list overflow on frame end");
        /* 仕方がないので最後の2命令を上書き */
        gelist_ptr = gelist_limit - 2;
    }
    internal_add_command(GECMD_FINISH, 0);
    internal_add_command(GECMD_END, 0);
    internal_commit();

#ifdef DEBUG
    gelist_used = gelist_ptr - gelist_base;
    vertlist_used = vertlist_ptr - vertlist_base;
    if (gelist_used > gelist_used_max) {
        gelist_used_max = gelist_used;
    }
    if (vertlist_used > vertlist_used_max) {
        vertlist_used_max = vertlist_used;
    }

    if (dumpflag) {
        DMSG("======== gelist ========");
        uint32_t *ptr = gelist_base;
        uint32_t address_base = 0;
        while (ptr < gelist_ptr) {
            const uint32_t insn = *ptr++;
            DMSG("%08X", insn);
            if (insn>>24 == GECMD_ADDRESS_BASE) {
                address_base = insn<<8;
            } else if (insn>>24 == GECMD_CALL) {
                const uint32_t address = address_base | (insn & 0xFFFFFF);
                const uint32_t *subptr = (const uint32_t *)address;
                DMSG("(call %p)", subptr);
                do {
                    DMSG("%08X", *subptr);
                } while ((*subptr++)>>24 != GECMD_RETURN);
                DMSG("(return)");
            }
        }
        DMSG("======== vertlist ========");
        uint16_t *vptr = (uint16_t *)vertlist_base;
        while (vptr < (uint16_t *)vertlist_ptr) {
            DMSG("%p: %04X %04X %04X %04X", vptr,
                 vptr[0], vptr[1], vptr[2], vptr[3]);
            vptr += 4;
        }
        DMSG("------------------------");
        dumpflag--;
    }
#endif

    int res = sceGeDrawSync(PSP_GE_LIST_DONE);
    if (res < 0) {
        DMSG("sceGeDrawSync(DONE) failed: %s", psp_strerror(res));
    }
    sceGeListDeQueue(gelist_id);
    frame_started = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * ge_commit:  これまでに登録されているGE命令の実行を開始する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void ge_commit(void)
{
    internal_commit();
}

/*-----------------------------------------------------------------------*/

/**
 * ge_sync:  実行中のコマンドが終了するのを待つ。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void ge_sync(void)
{
#if 0  // どうやらSTALL_REACHEDはカーネル専用なので、リストを終了させて再初期化
    //int res = sceGeDrawSync(PSP_GE_LIST_STALL_REACHED);
    int res = sceGeListSync(gelist_id, PSP_GE_LIST_STALL_REACHED);
    if (res < 0) {
        DMSG("sceGeListSync(STALL_REACHED): %s", psp_strerror(res));
    }
#else
    ge_end_frame();
    ge_start_frame(-1);
#endif
    /* VRAMデータがキャッシュに入っている場合、GEによる更新が反映されない
     * ので、関連キャッシュエントリーをフラッシュ（書き込み＋破棄）する */
    sceKernelDcacheWritebackInvalidateRange(sceGeEdramGetAddr(),
                                            sceGeEdramGetSize());
}

/*************************************************************************/

#ifdef DEBUG

/**
 * ge_get_debug_info:  デバッグ情報（各リストの使用ワード数）を取得する。
 *
 * 【引　数】      gelist_used_ret,
 * 　　　　　  gelist_used_max_ret,
 * 　　　　　      gelist_size_ret,
 * 　　　　　    vertlist_used_ret,
 * 　　　　　vertlist_used_max_ret,
 * 　　　　　    vertlist_size_ret: デバッグ情報を格納する変数へのポインタ
 * 【戻り値】なし
 */
void ge_get_debug_info(int *gelist_used_ret, int *gelist_used_max_ret,
                       int *gelist_size_ret, int *vertlist_used_ret,
                       int *vertlist_used_max_ret, int *vertlist_size_ret)
{
    PRECOND_SOFT(gelist_used_ret != NULL, return);
    PRECOND_SOFT(gelist_used_max_ret != NULL, return);
    PRECOND_SOFT(gelist_size_ret != NULL, return);
    PRECOND_SOFT(vertlist_used_ret != NULL, return);
    PRECOND_SOFT(vertlist_used_max_ret != NULL, return);
    PRECOND_SOFT(vertlist_size_ret != NULL, return);
    *gelist_used_ret = gelist_used;
    *gelist_used_max_ret = gelist_used_max;
    *gelist_size_ret = lenof(gelist);
    *vertlist_used_ret = vertlist_used;
    *vertlist_used_max_ret = vertlist_used_max;
    *vertlist_size_ret = lenof(vertlist);
}

#endif  // DEBUG

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * internal_commit:  これまでに登録されているGE命令の実行を開始する。命令
 * 追加の有無はチェックしない。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
static void internal_commit(void)
{
    if (saved_gelist_ptr) {
        /* サブリスト作成中は何もしない */
        return;
    }

    int res = sceGeListUpdateStallAddr(gelist_id, gelist_ptr);
    if (UNLIKELY(res < 0)) {
        DMSG("sceGeListUpdateStallAddr(): %s", psp_strerror(res));
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
