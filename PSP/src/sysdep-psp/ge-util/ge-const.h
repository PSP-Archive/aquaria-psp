/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util/ge-const.h: Header defining internal constants
 * for the GE utility library.
 */

#ifndef GE_CONST_H
#define GE_CONST_H

/*************************************************************************/

/* GE命令 */

typedef enum GECommand_ {
    GECMD_NOP           = 0x00, // No OPeration（処理なし）
    GECMD_VERTEX_POINTER= 0x01, // 頂点ポインタ（下位24bit）
    GECMD_INDEX_POINTER = 0x02, // indexポインタ（下位24bit）
    /* 0x03未定義 */
    GECMD_DRAW_PRIMITIVE= 0x04, // プリミティブを描画する
    GECMD_DRAW_BEZIER   = 0x05, // ベジェ曲線を描画する
    GECMD_DRAW_SPLINE   = 0x06, // スプラインを描画する
    GECMD_TEST_BBOX     = 0x07, // 頂点の範囲テスト
    GECMD_JUMP          = 0x08, // 無条件分岐（下位24bitを指定）
    GECMD_COND_JUMP     = 0x09, // 条件付き分岐（下位24bitを指定）
    GECMD_CALL          = 0x0A, // サブルーチン呼び出し（下位24bitを指定）
    GECMD_RETURN        = 0x0B, // サブルーチンから戻る
    GECMD_END           = 0x0C, // 処理終了
    /* 0x0D未定義 */
    GECMD_SIGNAL        = 0x0E, // シグナルを発生させる
    GECMD_FINISH        = 0x0F, // 描画完了を待つ

    GECMD_ADDRESS_BASE  = 0x10, // 各種アドレスの上位8bit
    /* 0x11未定義 */
    GECMD_VERTEX_FORMAT = 0x12, // 頂点のデータ形式
    GECMD_UNKNOWN_13    = 0x13, // psp_doc: Offset Address (BASE)
    GECMD_UNKNOWN_14    = 0x14, // psp_doc: Origin Address (BASE)
    GECMD_DRAWAREA_LOW  = 0x15, // 描画領域の左上隅の座標
    GECMD_DRAWAREA_HIGH = 0x16, // 描画領域の右下隅の座標
    GECMD_ENA_LIGHTING  = 0x17, // 光源すべてを切り替える
    GECMD_ENA_LIGHT0    = 0x18, // 光源0を切り替える
    GECMD_ENA_LIGHT1    = 0x19, // 光源1を切り替える
    GECMD_ENA_LIGHT2    = 0x1A, // 光源2を切り替える
    GECMD_ENA_LIGHT3    = 0x1B, // 光源3を切り替える
    GECMD_ENA_ZCLIP     = 0x1C, // 奥行きクリップを切り替える
    GECMD_ENA_FACE_CULL = 0x1D, // フェイス排除を切り替える
    GECMD_ENA_TEXTURE   = 0x1E, // テキスチャ適用を切り替える
    GECMD_ENA_FOG       = 0x1F, // フォグ適用を切り替える

    GECMD_ENA_DITHER    = 0x20, // ディザー処理を切り替える
    GECMD_ENA_BLEND     = 0x21, // ブレンド処理を切り替える
    GECMD_ENA_ALPHA_TEST= 0x22, // アルファテストを切り替える
    GECMD_ENA_DEPTH_TEST= 0x23, // 奥行きテストを切り替える
    GECMD_ENA_STENCIL   = 0x24, // ステンシル処理を切り替える
    GECMD_ENA_ANTIALIAS = 0x25, // アンチエイリアスを切り替える
    GECMD_ENA_PATCH_CULL= 0x26,
    GECMD_ENA_COLOR_TEST= 0x27, // 色テストを切り替える
    GECMD_ENA_LOGIC_OP  = 0x28, // 論理演算を切り替える
    /* 0x29未定義 */
    GECMD_BONE_OFFSET   = 0x2A,
    GECMD_BONE_UPLOAD   = 0x2B,
    GECMD_MORPH_0       = 0x2C,
    GECMD_MORPH_1       = 0x2D,
    GECMD_MORPH_2       = 0x2E,
    GECMD_MORPH_3       = 0x2F,

    GECMD_MORPH_4       = 0x30,
    GECMD_MORPH_5       = 0x31,
    GECMD_MORPH_6       = 0x32,
    GECMD_MORPH_7       = 0x33,
    /* 0x34未定義 */
    /* 0x35未定義 */
    GECMD_PATCH_SUBDIV  = 0x36,
    GECMD_PATCH_PRIM    = 0x37,
    GECMD_PATCH_FRONT   = 0x38,
    /* 0x39未定義 */
    GECMD_MODEL_START   = 0x3A, // モデル座標変換行列の設定を開始する
    GECMD_MODEL_UPLOAD  = 0x3B, // モデル座標変換行列の次の値をロードする
    GECMD_VIEW_START    = 0x3C, // 視点座標変換行列の設定を開始する
    GECMD_VIEW_UPLOAD   = 0x3D, // 視点座標変換行列の次の値をロードする
    GECMD_PROJ_START    = 0x3E, // 投影座標変換行列の設定を開始する
    GECMD_PROJ_UPLOAD   = 0x3F, // 投影座標変換行列の次の値をロードする

    GECMD_TEXTURE_START = 0x40, // テキスチャ座標変換行列の設定を開始する
    GECMD_TEXTURE_UPLOAD= 0x41, // テキスチャ座標変換行列の次の値をロードする
    GECMD_XSCALE        = 0x42, // X座標倍率係数
    GECMD_YSCALE        = 0x43, // Y座標倍率係数
    GECMD_ZSCALE        = 0x44, // Z座標倍率係数
    GECMD_XPOS          = 0x45, // 中心点のX座標
    GECMD_YPOS          = 0x46, // 中心点のY座標
    GECMD_ZPOS          = 0x47, // 中心点のZ座標
    GECMD_USCALE        = 0x48, // テキスチャのU座標倍率係数
    GECMD_VSCALE        = 0x49, // テキスチャのV座標倍率係数
    GECMD_UOFFSET       = 0x4A, // テキスチャのU座標オフセット
    GECMD_VOFFSET       = 0x4B, // テキスチャのV座標オフセット
    GECMD_XOFFSET       = 0x4C, // 画面左上隅のX座標
    GECMD_YOFFSET       = 0x4D, // 画面左上隅のY座標
    /* 0x4E未定義 */
    /* 0x4F未定義 */

    GECMD_SHADE_MODE    = 0x50, // シェードの種類
    GECMD_REV_NORMALS   = 0x51, // 法線ベクトルの符号反転を切り替える
    /* 0x52未定義 */
    GECMD_COLOR_MATERIAL= 0x53, // psp_doc: Color Material
    GECMD_EMISSIVE_COLOR= 0x54, // モデルの発光色
    GECMD_AMBIENT_COLOR = 0x55, // モデルの環境色
    GECMD_DIFFUSE_COLOR = 0x56, // モデルの頂点色
    GECMD_SPECULAR_COLOR= 0x57, // モデルの鏡面反射色
    GECMD_AMBIENT_ALPHA = 0x58, // モデルの環境色のアルファ値
    /* 0x59未定義 */
    /* 0x5A未定義 */
    GECMD_SPECULAR_POWER= 0x5B, // モデルの鏡面反射係数
    GECMD_LIGHT_AMBCOLOR= 0x5C, // 光源処理の環境色
    GECMD_LIGHT_AMBALPHA= 0x5D, // 光源処理の環境色のアルファ値
    GECMD_LIGHT_MODEL   = 0x5E, // 光源処理モデル
    GECMD_LIGHT0_TYPE   = 0x5F, // 光源0の種類

    GECMD_LIGHT1_TYPE   = 0x60, // 光源1の種類
    GECMD_LIGHT2_TYPE   = 0x61, // 光源2の種類
    GECMD_LIGHT3_TYPE   = 0x62, // 光源3の種類
    GECMD_LIGHT0_XPOS   = 0x63, // 光源0のX座標
    GECMD_LIGHT0_YPOS   = 0x64, // 光源0のY座標
    GECMD_LIGHT0_ZPOS   = 0x65, // 光源0のZ座標
    GECMD_LIGHT1_XPOS   = 0x66, // 光源1のX座標
    GECMD_LIGHT1_YPOS   = 0x67, // 光源1のY座標
    GECMD_LIGHT1_ZPOS   = 0x68, // 光源1のZ座標
    GECMD_LIGHT2_XPOS   = 0x69, // 光源2のX座標
    GECMD_LIGHT2_YPOS   = 0x6A, // 光源2のY座標
    GECMD_LIGHT2_ZPOS   = 0x6B, // 光源2のZ座標
    GECMD_LIGHT3_XPOS   = 0x6C, // 光源3のX座標
    GECMD_LIGHT3_YPOS   = 0x6D, // 光源3のY座標
    GECMD_LIGHT3_ZPOS   = 0x6E, // 光源3のZ座標
    GECMD_LIGHT0_XDIR   = 0x6F, // 光源0の方向ベクトルのX成分

    GECMD_LIGHT0_YDIR   = 0x70, // 光源0の方向ベクトルのY成分
    GECMD_LIGHT0_ZDIR   = 0x71, // 光源0の方向ベクトルのZ成分
    GECMD_LIGHT1_XDIR   = 0x72, // 光源1の方向ベクトルのX成分
    GECMD_LIGHT1_YDIR   = 0x73, // 光源1の方向ベクトルのY成分
    GECMD_LIGHT1_ZDIR   = 0x74, // 光源1の方向ベクトルのZ成分
    GECMD_LIGHT2_XDIR   = 0x75, // 光源2の方向ベクトルのX成分
    GECMD_LIGHT2_YDIR   = 0x76, // 光源2の方向ベクトルのY成分
    GECMD_LIGHT2_ZDIR   = 0x77, // 光源2の方向ベクトルのZ成分
    GECMD_LIGHT3_XDIR   = 0x78, // 光源3の方向ベクトルのX成分
    GECMD_LIGHT3_YDIR   = 0x79, // 光源3の方向ベクトルのY成分
    GECMD_LIGHT3_ZDIR   = 0x7A, // 光源3の方向ベクトルのZ成分
    GECMD_LIGHT0_CATT   = 0x7B, // 光源0の定率減衰率
    GECMD_LIGHT0_LATT   = 0x7C, // 光源0の直線的減衰率
    GECMD_LIGHT0_QATT   = 0x7D, // 光源0の二次減衰率
    GECMD_LIGHT1_CATT   = 0x7E, // 光源1の定率減衰率
    GECMD_LIGHT1_LATT   = 0x7F, // 光源1の直線的減衰率

    GECMD_LIGHT1_QATT   = 0x80, // 光源1の二次減衰率
    GECMD_LIGHT2_CATT   = 0x81, // 光源2の定率減衰率
    GECMD_LIGHT2_LATT   = 0x82, // 光源2の直線減衰率
    GECMD_LIGHT2_QATT   = 0x83, // 光源2の二乗減衰率
    GECMD_LIGHT3_CATT   = 0x84, // 光源3の定率減衰率
    GECMD_LIGHT3_LATT   = 0x85, // 光源3の直線減衰率
    GECMD_LIGHT3_QATT   = 0x86, // 光源3の二乗減衰率
    GECMD_LIGHT0_SPOTEXP= 0x87, // 光源0のスポット係数
    GECMD_LIGHT1_SPOTEXP= 0x88, // 光源1のスポット係数
    GECMD_LIGHT2_SPOTEXP= 0x89, // 光源2のスポット係数
    GECMD_LIGHT3_SPOTEXP= 0x8A, // 光源3のスポット係数
    GECMD_LIGHT0_SPOTLIM= 0x8B, // 光源0のスポット限界
    GECMD_LIGHT1_SPOTLIM= 0x8C, // 光源1のスポット限界
    GECMD_LIGHT2_SPOTLIM= 0x8D, // 光源2のスポット限界
    GECMD_LIGHT3_SPOTLIM= 0x8E, // 光源3のスポット限界
    GECMD_LIGHT0_ACOL   = 0x8F, // 光源0の環境色

    GECMD_LIGHT0_DCOL   = 0x90, // 光源0の頂点色
    GECMD_LIGHT0_SCOL   = 0x91, // 光源0の鏡面反射色
    GECMD_LIGHT1_ACOL   = 0x92, // 光源1の環境色
    GECMD_LIGHT1_DCOL   = 0x93, // 光源1の頂点色
    GECMD_LIGHT1_SCOL   = 0x94, // 光源1の鏡面反射色
    GECMD_LIGHT2_ACOL   = 0x95, // 光源2の環境色
    GECMD_LIGHT2_DCOL   = 0x96, // 光源2の頂点色
    GECMD_LIGHT2_SCOL   = 0x97, // 光源2の鏡面反射色
    GECMD_LIGHT3_ACOL   = 0x98, // 光源3の環境色
    GECMD_LIGHT3_DCOL   = 0x99, // 光源3の頂点色
    GECMD_LIGHT3_SCOL   = 0x9A, // 光源3の鏡面反射色
    GECMD_FACE_ORDER    = 0x9B, // フェイス排除で、前面フェイスと判定する頂点順
    GECMD_DRAW_ADDRESS  = 0x9C, // 描画バッファアドレス（下位24bit）
    GECMD_DRAW_STRIDE   = 0x9D, // 描画バッファアドレス（上位8bit）とライン長
    GECMD_DEPTH_ADDRESS = 0x9E, // 奥行きバッファアドレス（下位24bit）
    GECMD_DEPTH_STRIDE  = 0x9F, // 奥行きバッファアドレス（上位8bit）とライン長

    GECMD_TEX0_ADDRESS  = 0xA0, // テキスチャ0アドレス（下位24bit）
    GECMD_TEX1_ADDRESS  = 0xA1, // テキスチャ1アドレス（下位24bit）
    GECMD_TEX2_ADDRESS  = 0xA2, // テキスチャ2アドレス（下位24bit）
    GECMD_TEX3_ADDRESS  = 0xA3, // テキスチャ3アドレス（下位24bit）
    GECMD_TEX4_ADDRESS  = 0xA4, // テキスチャ4アドレス（下位24bit）
    GECMD_TEX5_ADDRESS  = 0xA5, // テキスチャ5アドレス（下位24bit）
    GECMD_TEX6_ADDRESS  = 0xA6, // テキスチャ6アドレス（下位24bit）
    GECMD_TEX7_ADDRESS  = 0xA7, // テキスチャ7アドレス（下位24bit）
    GECMD_TEX0_STRIDE   = 0xA8, // テキスチャ0アドレス（上位8bit）とライン長
    GECMD_TEX1_STRIDE   = 0xA9, // テキスチャ1アドレス（上位8bit）とライン長
    GECMD_TEX2_STRIDE   = 0xAA, // テキスチャ2アドレス（上位8bit）とライン長
    GECMD_TEX3_STRIDE   = 0xAB, // テキスチャ3アドレス（上位8bit）とライン長
    GECMD_TEX4_STRIDE   = 0xAC, // テキスチャ4アドレス（上位8bit）とライン長
    GECMD_TEX5_STRIDE   = 0xAD, // テキスチャ5アドレス（上位8bit）とライン長
    GECMD_TEX6_STRIDE   = 0xAE, // テキスチャ6アドレス（上位8bit）とライン長
    GECMD_TEX7_STRIDE   = 0xAF, // テキスチャ7アドレス（上位8bit）とライン長

    GECMD_CLUT_ADDRESS_L= 0xB0, // 色テーブルアドレス（下位24bit）
    GECMD_CLUT_ADDRESS_H= 0xB1, // 色テーブルアドレス（上位8bit）
    GECMD_COPY_S_ADDRESS= 0xB2, // データ転送元アドレス（下位24bit）
    GECMD_COPY_S_STRIDE = 0xB3, // データ転送元アドレス（上位8bit）とライン長
    GECMD_COPY_D_ADDRESS= 0xB4, // データ転送先アドレス（下位24bit）
    GECMD_COPY_D_STRIDE = 0xB5, // データ転送先アドレス（上位8bit）とライン長
    /* 0xB6未定義 */
    /* 0xB7未定義 */
    GECMD_TEX0_SIZE     = 0xB8, // テキスチャ0サイズ
    GECMD_TEX1_SIZE     = 0xB9, // テキスチャ1サイズ
    GECMD_TEX2_SIZE     = 0xBA, // テキスチャ2サイズ
    GECMD_TEX3_SIZE     = 0xBB, // テキスチャ3サイズ
    GECMD_TEX4_SIZE     = 0xBC, // テキスチャ4サイズ
    GECMD_TEX5_SIZE     = 0xBD, // テキスチャ5サイズ
    GECMD_TEX6_SIZE     = 0xBE, // テキスチャ6サイズ
    GECMD_TEX7_SIZE     = 0xBF, // テキスチャ7サイズ

    GECMD_TEXTURE_MAP   = 0xC0, // テキスチャマップモード
    GECMD_TEXTURE_ENVMAP= 0xC1, // テキスチャ環境マップ
    GECMD_TEXTURE_MODE  = 0xC2, // テキスチャモード
    GECMD_TEXTURE_PIXFMT= 0xC3, // テキスチャデータ形式
    GECMD_CLUT_LOAD     = 0xC4, // 色テーブルをロードする
    GECMD_CLUT_MODE     = 0xC5, // 色テーブルのデータ形式や使用モード
    GECMD_TEXTURE_FILTER= 0xC6, // テキスチャフィルタ
    GECMD_TEXTURE_WRAP  = 0xC7, // テキスチャ座標ラップモード
    GECMD_TEXTURE_BIAS  = 0xC8, // mipmap選択モードとバイアス
    GECMD_TEXTURE_FUNC  = 0xC9, // テキスチャ適用関数
    GECMD_TEXTURE_COLOR = 0xCA, // テキスチャ環境色
    GECMD_TEXTURE_FLUSH = 0xCB, // テキスチャキャッシュをクリアする
    GECMD_COPY_SYNC     = 0xCC, // データ転送完了を待つ
    GECMD_FOG_LIMIT     = 0xCD, // フォグ奥行き限界
    GECMD_FOG_RANGE     = 0xCE, // フォグ範囲
    GECMD_FOG_COLOR     = 0xCF, // フォグ色

    GECMD_TEXTURE_SLOPE = 0xD0, // テキスチャ傾斜
    /* 0xD1未定義 */
    GECMD_FRAME_PIXFMT  = 0xD2, // フレームバッファのデータ形式
    GECMD_CLEAR_MODE    = 0xD3, // バッファクリア切り替えとフラグ
    GECMD_CLIP_MIN      = 0xD4, // クリップ処理の最小XY座標
    GECMD_CLIP_MAX      = 0xD5, // クリップ処理の最大XY座標
    GECMD_CLIP_NEAR     = 0xD6, // クリップ処理の最小奥行き値
    GECMD_CLIP_FAR      = 0xD7, // クリップ処理の最大奥行き値
    GECMD_COLORTEST_FUNC= 0xD8, // 色テスト関数
    GECMD_COLORTEST_REF = 0xD9, // 色テストの基準値
    GECMD_COLORTEST_MASK= 0xDA, // 色テストのマスク値
    GECMD_ALPHATEST     = 0xDB, // アルファテスト関数と基準値・マスク
    GECMD_STENCILTEST   = 0xDC, // ステンシルテスト関数と基準値・マスク
    GECMD_STENCIL_OP    = 0xDD, // ステンシル処理
    GECMD_DEPTHTEST     = 0xDE, // 奥行きテスト関数
    GECMD_BLEND_FUNC    = 0xDF, // ブレンド関数

    GECMD_BLEND_SRCFIX  = 0xE0, // ブレンド定数（ブレンド元用）
    GECMD_BLEND_DSTFIX  = 0xE1, // ブレンド定数（ブレンド先用）
    GECMD_DITHER0       = 0xE2, // ディザー行列（0行目）
    GECMD_DITHER1       = 0xE3, // ディザー行列（1行目）
    GECMD_DITHER2       = 0xE4, // ディザー行列（2行目）
    GECMD_DITHER3       = 0xE5, // ディザー行列（3行目）
    GECMD_LOGIC_OP      = 0xE6, // 色の論理演算
    GECMD_DEPTH_MASK    = 0xE7, // 奥行きバッファ更新抑制を切り替える
    GECMD_COLOR_MASK    = 0xE8, // 色データのマスク値
    GECMD_ALPHA_MASK    = 0xE9, // アルファデータのマスク値
    GECMD_COPY          = 0xEA, // データ転送を開始する
    GECMD_COPY_S_POS    = 0xEB, // データ転送元のコピー開始座標
    GECMD_COPY_D_POS    = 0xEC, // データ転送先のコピー開始座標
    /* 0xED未定義 */
    GECMD_COPY_SIZE     = 0xEE, // データ転送サイズ
    /* 0xEF未定義 */

    GECMD_UNKNOWN_F0    = 0xF0,
    GECMD_UNKNOWN_F1    = 0xF1,
    GECMD_UNKNOWN_F2    = 0xF2,
    GECMD_UNKNOWN_F3    = 0xF3,
    GECMD_UNKNOWN_F4    = 0xF4,
    GECMD_UNKNOWN_F5    = 0xF5,
    GECMD_UNKNOWN_F6    = 0xF6,
    GECMD_UNKNOWN_F7    = 0xF7,
    GECMD_UNKNOWN_F8    = 0xF8,
    GECMD_UNKNOWN_F9    = 0xF9,
    /* 0xFA〜0xFF未定義 */
} GECommand;

/*************************************************************************/

/* クリアフラグ（GECMD_CLEAR_MODE用） */
#define GECLEAR_OFF     0x0000  // 通常描画モード
#define GECLEAR_ON      0x0001  // クリアモード
#define GECLEAR_DRAW    0x0100  // フレームバッファをクリアする
#define GECLEAR_STENCIL 0x0200  // ステンシルバッファをクリアする
#define GECLEAR_DEPTH   0x0400  // 奥行きバッファをクリアする

/* 頂点順序定数（GECMD_FACE_ORDER用） */
typedef enum GEVertexOrder_ {
    GEVERT_CCW = 0,
    GEVERT_CW  = 1,
} GEVertexOrder;

/* シグナル処理動作 */
typedef enum GESignalBehavior_ {
    GESIG_SUSPEND = 1,   // コールバック完了までGE処理を中断する
    GESIG_CONTINUE = 2,  // コールバック完了を待たずGE処理を継続する
} GESignalBehavior;

/*************************************************************************/

#endif  // GE_CONST_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
