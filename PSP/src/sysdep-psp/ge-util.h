/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/sysdep-psp/ge-util.h: Header for the GE utility library.
 */

#ifndef SYSDEP_PSP_GE_UTIL_H
#define SYSDEP_PSP_GE_UTIL_H

/*************************************************************************/
/*************************************************************************/

/**
 * GEList:  GE命令リストの型（ge_fast_add_command()用）。
 */
typedef struct GEList_ {
    uint32_t *ptr;
} GEList;

/* GEList構造体の宣言時初期化マクロ */
#define INIT_GE_LIST(pointer)  {.ptr = (pointer)}

/*************************************************************************/

/**
 * GEBlendFunc:  ブレンド関数を指定する定数。
 */
typedef enum GEBlendFunc_ {
    GE_BLEND_ADD              = 0,  // Cs*Bs + Cd*Bd
    GE_BLEND_SUBTRACT         = 1,  // Cs*Bs - Cd*Bd
    GE_BLEND_REVERSE_SUBTRACT = 2,  // Cd*Bd - Cs*Bs
    GE_BLEND_MIN              = 3,  // min(Cs,Cd)
    GE_BLEND_MAX              = 4,  // max(Cs,Cd)
    GE_BLEND_ABS              = 5,  // |Cs-Cd|
} GEBlendFunc;

/**
 * GEBlendParam:  ブレンドパラメータを指定する定数。
 */
typedef enum GEBlendParam_ {
    GE_BLEND_COLOR               = 0,  // 各色の値（Bs=Cs、Bd=Cd）
    GE_BLEND_ONE_MINUS_COLOR     = 1,  // 各色を反転した値（Bs=1-Cs、Bd=1-Cd）
    GE_BLEND_SRC_ALPHA           = 2,  // srcのアルファ値
    GE_BLEND_ONE_MINUS_SRC_ALPHA = 3,  // srcのアルファを反転した値
    GE_BLEND_DST_ALPHA           = 4,  // dstのアルファ値
    GE_BLEND_ONE_MINUS_DST_ALPHA = 5,  // dstのアルファを反転した値
    GE_BLEND_FIX                 = 10, // 定数
} GEBlendParam;

/**
 * GECopyMode:  ge_copy()のコピー単位を指定するてー数。
 */
typedef enum GECopyMode_ {
    GE_COPY_16BIT = 0,  // 16ビット単位でデータを転送する
    GE_COPY_32BIT = 1,  // 32ビット単位でデータを転送する
} GECopyMode;

/**
 * GECullMode:  ge_set_cull_mode()で使われるフェイス排除モード定数。
 */
typedef enum GECullMode_ {
    GE_CULL_NONE = 0,   // フェイス排除を行わない
    GE_CULL_CW,         // 頂点順が時計回りのフェイスを排除する
    GE_CULL_CCW,        // 頂点順が反時計回りのフェイスを排除する
} GECullMode;

/**
 * GELightComponent:  ge_set_light_color()で使われる光源成分定数。
 */
typedef enum GELightComponent_ {
    GE_LIGHT_COMPONENT_AMBIENT = 0,
    GE_LIGHT_COMPONENT_DIFFUSE,
    GE_LIGHT_COMPONENT_SPECULAR,
} GELightComponent;

/**
 * GELightMode:  ge_set_light_mode()で使われる光源処理モード定数。
 */
typedef enum GELightMode_ {
    GE_LIGHT_MODE_SINGLE_COLOR = 0,
    GE_LIGHT_MODE_SEPARATE_SPECULAR_COLOR,
} GELightMode;

/**
 * GELightType:  ge_set_light_type()で使われる光源種類定数。
 */
typedef enum GELightType_ {
    GE_LIGHT_TYPE_DIRECTIONAL = 0,  // 指向性光源
    GE_LIGHT_TYPE_POINT_LIGHT,      // 点光源
    GE_LIGHT_TYOE_SPOTLIGHT,        // スポット光源
} GELightType;

/**
 * GEPixelFormat:  ピクセル・色データ形式を指定する定数。
 */
typedef enum GEPixelFormat_ {
    GE_PIXFMT_5650 = 0,  // 16ビット（R:5 G:6 B:5 A:0）
    GE_PIXFMT_5551 = 1,  // 16ビット（R:5 G:5 B:5 A:1）
    GE_PIXFMT_4444 = 2,  // 16ビット（R:4 G:4 B:4 A:4）
    GE_PIXFMT_8888 = 3,  // 32ビット（R:8 G:8 B:8 A:8）
} GEPixelFormat;

/**
 * GEPrimitive:  プリミティブを指定する定数。
 */
typedef enum GEPrimitive_ {
    GE_PRIMITIVE_POINTS         = 0, // 点
    GE_PRIMITIVE_LINES          = 1, // 独立した直線
    GE_PRIMITIVE_LINE_STRIP     = 2, // 連続する直線
    GE_PRIMITIVE_TRIANGLES      = 3, // 独立した三角
    GE_PRIMITIVE_TRIANGLE_STRIP = 4, // 連続する三角（頂点順：012、213、234…）
    GE_PRIMITIVE_TRIANGLE_FAN   = 5, // 連続する三角（頂点順：012、123、234…）
    GE_PRIMITIVE_SPRITES        = 6, // 頂点2つで指定した長方形
} GEPrimitive;

/**
 * GEShadeMode:  シェードモード定数。
 */
typedef enum GEShadeMode_ {
    GE_SHADE_FLAT    = 0,  // 単色（シェードなし）
    GE_SHADE_GOURAUD = 1,  // 滑らかなシェード
} GEShadeMode;

/**
 * GEState:  ge_enable()、ge_disable()で使われる描画機能定数。
 */
typedef enum GEState_ {
    GE_STATE_LIGHTING,          // 有効＝光源を適用する
    GE_STATE_CLIP_PLANES,       // 有効＝奥行きクリップを行う
    GE_STATE_TEXTURE,           // 有効＝テキスチャを適用する
    GE_STATE_FOG,               // 有効＝フォグを適用する
    GE_STATE_DITHER,            // 有効＝ディザーを行う
    GE_STATE_BLEND,             // 有効＝ブレンドを行う
    GE_STATE_ALPHA_TEST,        // 有効＝アルファ値テスト失敗時に描画しない
    GE_STATE_DEPTH_TEST,        // 有効＝奥行きテスト失敗時に描画しない
    GE_STATE_DEPTH_WRITE,       // 有効＝奥行きバッファを更新する
    GE_STATE_STENCIL_TEST,      // 有効＝ステンシルテスト失敗時に描画しない
    GE_STATE_ANTIALIAS,         // 有効＝補間処理を行う
    GE_STATE_PATCH_CULL_FACE,   // 有効＝？？？
    GE_STATE_COLOR_TEST,        // 有効＝？？？
    GE_STATE_COLOR_LOGIC_OP,    // 有効＝描画時に色データの論理演算を行う
    GE_STATE_REVERSE_NORMALS,   // 有効＝法線ベクトルの符号を反転する
} GEState;

/**
 * GETestFunc:  各種テストの比較関数を指定する定数。
 */
typedef enum GETestFunc_ {
    GE_TEST_NEVER    = 0,
    GE_TEST_ALWAYS   = 1,
    GE_TEST_EQUAL    = 2,
    GE_TEST_NOTEQUAL = 3,
    GE_TEST_LESS     = 4,
    GE_TEST_LEQUAL   = 5,
    GE_TEST_GREATER  = 6,
    GE_TEST_GEQUAL   = 7,
} GETestFunc;

/**
 * GETexelFormat:  テキスチャデータ形式を指定する定数。
 */
typedef enum GETexelFormat_ {
    GE_TEXFMT_5650 = GE_PIXFMT_5650,
    GE_TEXFMT_5551 = GE_PIXFMT_5551,
    GE_TEXFMT_4444 = GE_PIXFMT_4444,
    GE_TEXFMT_8888 = GE_PIXFMT_8888,
    GE_TEXFMT_T4   = 4,  // 4ビットCLUT型
    GE_TEXFMT_T8   = 5,  // 8ビットCLUT型
    GE_TEXFMT_T16  = 6,  // 16ビットCLUT型
    GE_TEXFMT_T32  = 7,  // 32ビットCLUT型
    GE_TEXFMT_DXT1 = 8,  // 圧縮データ（DXT1形式）
    GE_TEXFMT_DXT3 = 9,  // 圧縮データ（DXT3形式）
    GE_TEXFMT_DXT5 = 10, // 圧縮データ（DXT5形式）
} GETexelFormat;

/**
 * GETextureDrawMode:  テキスチャ描画方式を指定する定数。以下の色変数は、
 * 　Cv・Av＝結果の色・濃度
 * 　Cf・Af＝フラグメント（テキスチャ適用前のピクセル）の色・濃度
 * 　Ct・At＝テキスチャデータの色・濃度
 * 　Cc＝テキスチャ色（ge_set_texture_color()で設定された色）
 */
typedef enum GETextureDrawMode_ {
    GE_TEXDRAWMODE_MODULATE = 0,  // Cv = Cf*Ct           | Av = Af*At
    GE_TEXDRAWMODE_DECAL    = 1,  // Cv = Cf*(1-At)+Ct*At | Av = Af
    GE_TEXDRAWMODE_BLEND    = 2,  // Cv = Cf*(1-Ct)+Cc*Ct | Av = Af*At
    GE_TEXDRAWMODE_REPLACE  = 3,  // Cv = Ct              | Av = At
    GE_TEXDRAWMODE_ADD      = 4,  // Cv = Cf+Ct           | Av = Af*At
} GETextureDrawMode;

/**
 * GETextureFilter:  テキスチャの拡大・縮小フィルタを指定する定数。
 */
typedef enum GETextureFilter_ {
    GE_TEXFILTER_NEAREST = 0,  // フィルターせず、一番近いピクセルを使う
    GE_TEXFILTER_LINEAR  = 1,  // 直線的平均
} GETextureFilter;

/**
 * GETextureMipFilter:  テキスチャのmipmap選択フィルタを指定する定数。
 */
typedef enum GETextureMipFilter_ {
    GE_TEXMIPFILTER_NONE    = 0,  // mipmapを使わない
    GE_TEXMIPFILTER_NEAREST = 4,  // 一番近いmipmapをそのまま使う
    GE_TEXMIPFILTER_LINEAR  = 6,  // 2つのmipmapをブレンドする
} GETextureMipFilter;

/**
 * GEMipmapMode:  テキスチャのmipmap選択モードを指定する定数。
 *
 * 注：AUTOモードではPSPハードの不具合(?)により、隣り合う三角形のmipmap
 * 　　レベルが極端に異なったり、画面の平面との角度が大きければ大きいほど
 * 　　レベルが余計に高く（解像度が低く）なったりすることがある。AUTOモード
 * 　　を使う場合は、負数のバイアスを指定することを推奨する。
 */
typedef enum GEMipmapMode_ {
    GE_MIPMAPMODE_AUTO  = 0,  // ハードに任せる（上記の注意を参照）
    GE_MIPMAPMODE_CONST = 1,  // レベルを固定する（バイアスで設定）
    GE_MIPMAPMODE_SLOPE = 2,  // カメラからの距離と傾斜で選択する
} GEMipmapMode;

/**
 * GETextureWrapMode:  テキスチャ座標のラップモードを指定する定数。
 */
typedef enum GETextureWrapMode_ {
    GE_TEXWRAPMODE_REPEAT = 0,  // テキスチャをリピートする
    GE_TEXWRAPMODE_CLAMP  = 1,  // 座標を[0,1]以内に収める
} GETextureWrapMode;

/*************************************************************************/

/**
 * GE_BLENDSET_*:  よく使うブレンド設定を、ge_set_blend_mode()の引数として
 * まとめたもの。ge_set_blend_mode(GE_BLENDSET_*)のように使う。
 */

/* 通常のアルファブレンド */
#define GE_BLENDSET_SRC_ALPHA \
    GE_BLEND_ADD, GE_BLEND_SRC_ALPHA, GE_BLEND_ONE_MINUS_SRC_ALPHA, 0, 0

/* ブレンド元のアルファデータを無視して、固定アルファ値でブレンドする。
 * 0 <= alpha <= 255（但しalphaに副作用のないこと） */
#define GE_BLENDSET_FIXED_ALPHA(alpha) \
    GE_BLEND_ADD, GE_BLEND_FIX, GE_BLEND_FIX, \
    (alpha) * 0x010101, (255-(alpha)) * 0x010101

/*************************************************************************/

/**
 * GE_VERTEXFMT_*:  頂点データ形式を指定する定数。各グループから最大1つを
 * 選んで、ビット和を求める。データ順は以下の通り。
 * 　・テキスチャ座標（U、Vの順）
 * 　・頂点色
 * 　・法線ベクトル（X、Y、Zの順）
 * 　・頂点座標（X、Y、Zの順）
 * 頂点データのアライメントはC言語のルールに準じる。
 */

/* テキスチャ座標形式 */
#define GE_VERTEXFMT_TEXTURE_8BIT   (1<<0)  // 符号付き8ビット整数
#define GE_VERTEXFMT_TEXTURE_16BIT  (2<<0)  // 符号付き16ビット整数
#define GE_VERTEXFMT_TEXTURE_32BITF (3<<0)  // 単精度浮動小数点

/* 頂点色 */
#define GE_VERTEXFMT_COLOR_5650     (4<<2)
#define GE_VERTEXFMT_COLOR_5551     (5<<2)
#define GE_VERTEXFMT_COLOR_4444     (6<<2)
#define GE_VERTEXFMT_COLOR_8888     (7<<2)

/* 法線ベクトル成分形式 */
#define GE_VERTEXFMT_NORMAL_8BIT    (1<<5)  // 符号付き8ビット整数
#define GE_VERTEXFMT_NORMAL_16BIT   (2<<5)  // 符号付き16ビット整数
#define GE_VERTEXFMT_NORMAL_32BITF  (3<<5)  // 単精度浮動小数点

/* 頂点座標形式 */
#define GE_VERTEXFMT_VERTEX_8BIT    (1<<7)  // 符号付き8ビット整数
#define GE_VERTEXFMT_VERTEX_16BIT   (2<<7)  // 符号付き16ビット整数
#define GE_VERTEXFMT_VERTEX_32BITF  (3<<7)  // 単精度浮動小数点

/* 座標変換方法 */
#define GE_VERTEXFMT_TRANSFORM_3D   (0<<23) // 通常の3次元変換
#define GE_VERTEXFMT_TRANSFORM_2D   (1<<23) // 変換なし

/*************************************************************************/
/*************************************************************************/

/* GE操作関数宣言 */

/********************* 基本操作関数 (ge-util/base.c) *********************/

/**
 * ge_init:  GEを初期化する。
 *
 * 【引　数】なし
 * 【戻り値】0以外＝成功、0＝失敗
 */
extern int ge_init(void);

/**
 * ge_start_frame:  フレームの初期処理を行う。
 *
 * 【引　数】display_mode: 表示モード（GE_PIXFMT_*、負数＝変更なし）
 * 【戻り値】なし
 */
extern void ge_start_frame(int display_mode);

/**
 * ge_end_frame:  フレームの終了処理を行う。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void ge_end_frame(void);

/**
 * ge_commit:  これまでに登録されているGE命令の実行を開始する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void ge_commit(void);

/**
 * ge_sync:  実行中のコマンドが終了するのを待つ。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void ge_sync(void);

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
extern void ge_get_debug_info(int *gelist_used_ret, int *gelist_used_max_ret,
                              int *gelist_size_ret, int *vertlist_used_ret,
                              int *vertlist_used_max_ret, int *vertlist_size_ret);
#endif

/********************* 描画関連関数 (ge-util/draw.c) *********************/

/**
 * ge_set_draw_buffer:  描画バッファを設定する。ptr==NULLの場合、フレーム
 * バッファを使う。
 *
 * 【引　数】buffer: 描画バッファへのポインタ（アドレスの下位6ビット切り捨て）
 * 　　　　　stride: 描画バッファのライン長（ピクセル）
 * 【戻り値】なし
 */
extern void ge_set_draw_buffer(void *buffer, int stride);

/**
 * ge_set_vertex_format:  頂点データ形式を設定する。
 *
 * 【引　数】format: 頂点データ形式（GE_VERTEXFMT_*）
 * 【戻り値】なし
 */
extern void ge_set_vertex_format(uint32_t format);

/**
 * ge_set_vertex_pointer:  プリミティブ描画用頂点ポインタを設定する。
 * ptr==NULLの場合、内部ポインタを使う。
 *
 * 【引　数】ptr: 頂点データへのポインタ
 * 【戻り値】なし
 */
extern void ge_set_vertex_pointer(const void *ptr);

/**
 * ge_draw_primitive:  プリミティブを描画する。事前に頂点データ形式と頂点
 * ポインタを設定しなければならない。ただし連続で呼び出した場合、頂点データ
 * も連続していれば、再設定する必要はない。
 *
 * 【引　数】   primitive: プリミティブ（GE_PRIMITIVE_*）
 * 　　　　　num_vertices: 頂点数
 * 【戻り値】なし
 */
extern void ge_draw_primitive(GEPrimitive primitive, uint16_t num_vertices);

/**************** 高レベル操作関数 (ge-util/high-level.c) ****************/

/**
 * ge_clear:  引数に応じて、画面と奥行きバッファをクリアする。
 *
 * 【引　数】clear_screen: 0以外＝画面をクリアする、0＝クリアしない
 * 　　　　　 clear_depth: 0以外＝奥行きバッファをクリアする、0＝クリアしない
 * 　　　　　       color: 画面をクリアする際の色（0xAABBGGRR）
 * 【戻り値】なし
 */
extern void ge_clear(int clear_screen, int clear_depth, uint32_t color);

/**
 * ge_copy:  srcからdestへ画像データをコピーする。各ポインタのアライメントは
 * 1ピクセルで良いが、ライン長は8ピクセルの倍数でなければならない。
 *
 * 【引　数】          src: コピー元ポインタ
 * 　　　　　   src_stride: コピー元のライン幅（ピクセル）、2048未満
 * 　　　　　         dest: コピー先ポインタ
 * 　　　　　  dest_stride: コピー先のライン幅（ピクセル）、2048未満
 * 　　　　　width, height: コピーする領域のサイズ（ピクセル）、512以下
 * 　　　　　         mode: コピーモード（ピクセルデータのサイズ、GE_COPY_*）
 * 【戻り値】なし
 */
extern void ge_copy(const uint32_t *src, uint32_t src_stride, uint32_t *dest,
                    uint32_t dest_stride, int width, int height, GECopyMode mode);

/**
 * ge_blend:  srcからdestへ画像データをブレンドしながらコピーする。各ポインタ
 * のアライメントは1ピクセルで良いが、コピー先はVRAM内でなければならない。
 * ブレンド方法は、ge_set_blend_mode()によって事前に設定しなければならない。
 *
 * 【引　数】          src: コピー元ポインタ
 * 　　　　　   src_stride: コピー元のライン幅（ピクセル）、2048未満
 * 　　　　　   srcx, srcy: コピーする部分の左上隅の座標（ピクセル）
 * 　　　　　         dest: コピー先ポインタ（VRAM内）
 * 　　　　　  dest_stride: コピー先のライン幅（ピクセル）、2048未満
 * 　　　　　width, height: コピーする領域のサイズ（ピクセル）、height<=512
 * 　　　　　      palette: コピー元が8ビットデータの場合、色パレット。
 * 　　　　　               　　コピー元が32ビットデータの場合はNULLを指定
 * 　　　　　     swizzled: 0以外の場合、コピー元のデータがswizzleしてある
 * 【戻り値】なし
 */
extern void ge_blend(const uint32_t *src, uint32_t src_stride, int srcx, int srcy,
                     uint32_t *dest, uint32_t dest_stride, int width, int height,
                     const uint32_t *palette, int swizzled);

/**
 * ge_fill:  VRAM領域をフィルする。
 *
 * 【引　数】x1, y1: 左上座標（ピクセル）
 * 　　　　　x2, y2: 右下座標+1（ピクセル）※x2,y2はフィル領域には入らない。
 * 　　　　　 color: フィル色（0xAABBGGRR）
 * 【戻り値】なし
 */
extern void ge_fill(int x1, int y1, int x2, int y2, uint32_t color);

/******************** 光源操作関数 (ge-util/light.c) *********************/

/**
 * ge_set_light_mode:  光源処理モードを設定する。
 *
 * 【引　数】mode: 光源処理モード（GU_LIGHT_MODE_*）
 * 【戻り値】なし
 */
extern void ge_set_light_mode(unsigned int mode);

/**
 * ge_enable_light:  指定された光源を有効にする。
 *
 * 【引　数】light: 有効にする光源インデックス（0〜3）
 * 【戻り値】なし
 */
extern void ge_enable_light(unsigned int light);

/**
 * ge_disable_light:  指定された光源を無効にする。
 *
 * 【引　数】light: 無効にする光源インデックス（0〜3）
 * 【戻り値】なし
 */
extern void ge_disable_light(unsigned int light);

/**
 * ge_set_light_type:  光源の種類を設定する。
 *
 * 【引　数】       light: 光源インデックス（0〜3）
 * 　　　　　        type: 光源の種類（GE_LIGHT_TYPE_*）
 * 　　　　　has_specular: 0以外＝スペキュラーの有無
 * 【戻り値】なし
 */
extern void ge_set_light_type(unsigned int light, GELightType type,
                              int has_specular);

/**
 * ge_set_light_position:  光源の位置を設定する。
 *
 * 【引　数】  light: 光源インデックス（0〜3）
 * 　　　　　x, y, z: 光源の位置
 * 【戻り値】なし
 */
extern void ge_set_light_position(unsigned int light, float x, float y, float z);

/**
 * ge_set_light_direction:  光源の方向を設定する。
 *
 * 【引　数】  light: 光源インデックス（0〜3）
 * 　　　　　x, y, z: 光源の方向
 * 【戻り値】なし
 */
extern void ge_set_light_direction(unsigned int light, float x, float y, float z);

/**
 * ge_set_light_attenuation:  光源の減衰率を設定する。
 *
 * 【引　数】    light: 光源インデックス（0〜3）
 * 　　　　　 constant: 定率減衰率
 * 　　　　　   linear: 直線減衰率
 * 　　　　　quadratic: 二乗減衰率
 * 【戻り値】なし
 */
extern void ge_set_light_attenuation(unsigned int light, float constant,
                                     float linear, float quadratic);

/**
 * ge_set_light_color:  光源の位置を設定する。
 *
 * 【引　数】    light: 光源インデックス（0〜3）
 * 　　　　　component: 色を設定する成分（GE_LIGHT_COMPONENT_*）
 * 　　　　　    color: 色（0xBBGGRR）
 * 【戻り値】なし
 */
extern void ge_set_light_color(unsigned int light, unsigned int component,
                               uint32_t color);

/**
 * ge_set_spotlight_exponent:  スポット光源の指数を設定する。
 *
 * 【引　数】   light: 光源インデックス（0〜3）
 * 　　　　　exponent: スポット指数
 * 【戻り値】なし
 */
extern void ge_set_spotlight_exponent(unsigned int light, float exponent);

/**
 * ge_set_spotlight_cutoff:  スポット光源の閾値を設定する。
 *
 * 【引　数】 light: 光源インデックス（0〜3）
 * 　　　　　cutoff: スポット閾値
 * 【戻り値】なし
 */
extern void ge_set_spotlight_cutoff(unsigned int light, float cutoff);

/**************** 描画命令リスト管理関数 (ge-util/list.c) ****************/

/**
 * ge_add_command, ge_add_commandf:  GE命令を登録する。
 *
 * 【引　数】  command: 命令（0〜255）
 * 　　　　　parameter: 命令パラメータ（24ビット整数または浮動小数点）
 * 【戻り値】なし
 */
extern void ge_add_command(uint8_t command, uint32_t parameter);
extern void ge_add_commandf(uint8_t command, float parameter);

/**
 * ge_start_sublist:  サブリストの作成を開始する。
 *
 * 【引　数】list: サブリストバッファへのポインタ
 * 　　　　　size: サブリストバッファのサイズ（ワード単位）
 * 【戻り値】0以外＝成功、0＝失敗（すでにサブリストを作成中）
 */
extern int ge_start_sublist(uint32_t *list, int size);

/**
 * ge_replace_sublist:  作成中のサブリストを別のメモリ領域に置き換える。
 * リスト満杯時、リストバッファを再確保したときに呼び出されることを想定
 * している。
 *
 * サブリスト作成中でない場合、何もしない。
 *
 * 【引　数】list: サブリストバッファへのポインタ
 * 　　　　　size: サブリストバッファのサイズ（ワード単位）
 * 【戻り値】なし
 */
extern void ge_replace_sublist(uint32_t *list, int size);

/**
 * ge_finish_sublist:  現在作成中のサブリストを終了させる。サブリスト作成中
 * でない場合、何もせずNULLを返す。
 *
 * 【引　数】なし
 * 【戻り値】リスト終端へのポインタ（最後の命令の直後を指す）
 */
extern uint32_t *ge_finish_sublist(void);

/**
 * ge_call_sublist:  サブリストを呼び出す。
 *
 * 【引　数】list: サブリストへのポインタ
 * 【戻り値】なし
 */
extern void ge_call_sublist(const uint32_t *list);

/**
 * ge_sublist_free:  現在作成中のサブリストの空きワード数を返す。
 *
 * 【引　数】なし
 * 【戻り値】空きワード数（サブリスト作成中でない場合は0）
 */
extern uint32_t ge_sublist_free(void);

/************* 3次元座標変換行列設定関数 (ge-util/matrix.c) *************/

/**
 * ge_set_projection_matrix:  投影座標変換行列を設定する。
 *
 * 【引　数】matrix: 設定する投影座標変換行列
 * 【戻り値】なし
 */
extern void ge_set_projection_matrix(const Matrix4f *matrix);

/**
 * ge_set_view_matrix:  視点座標変換行列を設定する。
 *
 * 【引　数】matrix: 設定する視点座標変換行列
 * 【戻り値】なし
 */
extern void ge_set_view_matrix(const Matrix4f *matrix);

/**
 * ge_set_model_matrix:  モデル座標変換行列を設定する。
 *
 * 【引　数】matrix: 設定するモデル座標変換行列
 * 【戻り値】なし
 */
extern void ge_set_model_matrix(const Matrix4f *matrix);

/**
 * ge_set_texture_matrix:  テキスチャ座標変換行列を設定する。
 *
 * 【引　数】matrix: 設定するテキスチャ座標変換行列
 * 【戻り値】なし
 */
extern void ge_set_texture_matrix(const Matrix4f *matrix);

/************* 描画機能切り替え・設定関数 (ge-util/state.c) **************/

/**
 * ge_enable:  指定された描画機能を有効にする。
 *
 * 【引　数】state: 有効にする機能（GE_STATE_*）
 * 【戻り値】なし
 */
extern void ge_enable(GEState state);

/**
 * ge_disable:  指定された描画機能を無効にする。
 *
 * 【引　数】state: 無効にする機能（GE_STATE_*）
 * 【戻り値】なし
 */
extern void ge_disable(GEState state);

/**
 * ge_set_alpha_mask:  アルファデータの書き込みマスク値を設定する。
 *
 * 【引　数】mask: 書き込みマスク（0xFF＝全ビット書き込み不可）
 * 【戻り値】なし
 */
extern void ge_set_alpha_mask(uint8_t mask);

/**
 * ge_set_alpha_test:  アルファテストの比較関数と基準値を設定する。
 *
 * 【引　数】test: 比較関数（GE_TEST_*）
 * 　　　　　 ref: 基準値（0〜255）
 * 【戻り値】なし
 */
extern void ge_set_alpha_test(GETestFunc test, uint8_t ref);

/**
 * ge_set_ambient_color:  描画時の環境色を設定する。
 *
 * 【引　数】color: 環境色（0xAABBGGRR）
 * 【戻り値】なし
 */
extern void ge_set_ambient_color(uint32_t color);

/**
 * ge_set_ambient_light:  光源モデルの環境色を設定する。
 *
 * 【引　数】color: 環境色（0xAABBGGRR）
 * 【戻り値】なし
 */
extern void ge_set_ambient_light(uint32_t color);

/**
 * ge_set_blend_mode:  ブレンド関数とパラメータを設定する。
 *
 * 【引　数】     func: ブレンド関数
 * 　　　　　src_param: ブレンド元（src）のブレンドパラメータ
 * 　　　　　dst_param: ブレンド先（dst）のブレンドパラメータ
 * 　　　　　  src_fix: src_param==GE_BLEND_FIXの場合、srcに適用する定数
 * 　　　　　  dst_fix: dst_param==GE_BLEND_FIXの場合、dstに適用する定数
 * 【戻り値】なし
 * 【注　意】src_param!=GE_BLEND_FIXの場合、src用定数レジスタを更新しない。
 * 　　　　　dst_param!=GE_BLEND_FIXの場合も同様。
 */
extern void ge_set_blend_mode(GEBlendFunc func,
                              GEBlendParam src_param, GEBlendParam dst_param,
                              uint32_t src_fix, uint32_t dst_fix);

/**
 * ge_set_clip_area, ge_unset_clip_area:  クリップ範囲を設定または解除する。
 *
 * 【引　数】x0, y0: クリップ範囲の左上隅の座標（ピクセル）
 * 　　　　　x1, y1: クリップ範囲の右下隅の座標（ピクセル）
 * 【戻り値】なし
 */
extern void ge_set_clip_area(int x0, int y0, int x1, int y1);
extern void ge_unset_clip_area(void);

/**
 * ge_set_color_mask:  色データの書き込みマスク値を設定する。
 *
 * 【引　数】mask: 書き込みマスク（0xFFFFFF＝全ビット書き込み不可）
 * 【戻り値】なし
 */
extern void ge_set_color_mask(uint32_t mask);

/**
 * ge_set_cull_mode:  フェイス排除モードを設定する。
 *
 * 【引　数】mode: フェイス排除モード（GE_CULL_*）
 * 【戻り値】なし
 */
extern void ge_set_cull_mode(GECullMode mode);

/**
 * ge_set_depth_test:  奥行きテストの比較関数を設定する。
 *
 * 【引　数】test: 比較関数（GE_TEST_*）
 * 【戻り値】なし
 */
extern void ge_set_depth_test(GETestFunc test);

/**
 * ge_set_depth_range:  奥行きバッファに格納する奥行き値の範囲を設定する。
 * デフォルトは0〜65535。
 *
 * 【引　数】near: 最も近い奥行きに与える値（0〜65535）
 * 　　　　　 far: 最も遠い奥行きに与える値（0〜65535）
 * 【戻り値】なし
 */
extern void ge_set_depth_range(uint16_t near, uint16_t far);

/**
 * ge_set_fog:  フォグの設定を行う。
 *
 * 【引　数】 near: フォグが発生し始める奥行き
 * 　　　　　  far: フォグ効果が最大に達する奥行き
 * 　　　　　color: フォグの色（0xBBGGRR）
 * 【戻り値】なし
 */
extern void ge_set_fog(float near, float far, uint32_t color);

/**
 * ge_set_shade_mode:  シェードモードを設定する。
 *
 * 【引　数】mode: シェードモード（GE_SHADE_*）
 * 【戻り値】なし
 */
extern void ge_set_shade_mode(GEShadeMode mode);

/**
 * ge_set_viewport:  表示領域を設定する。
 *
 * 【引　数】         x, y: 左下隅の座標（ピクセル）
 * 　　　　　width, height: 領域のサイズ（ピクセル）
 * 【戻り値】なし
 */
extern void ge_set_viewport(int x, int y, int width, int height);

/**************** テキスチャ操作関数 (ge-util/texture.c) *****************/

/**
 * ge_set_colortable:  CLUT型テキスチャのための色テーブルを設定する。
 *
 * 【引　数】 table: 色テーブルへのポインタ（64バイトアライメント必須）
 * 　　　　　 count: 色の数
 * 　　　　　format: 色データ形式（GE_PIXFMT_*）
 * 　　　　　 shift: テキスチャデータの右シフト量（0〜31）
 * 　　　　　  mask: テキスチャデータのマスク値
 * 【戻り値】なし
 */
extern void ge_set_colortable(const void *table, int count, GEPixelFormat pixfmt,
                              int shift, uint8_t mask);

/**
 * ge_flush_texture_cache:  テキスチャキャッシュをクリアする。データポインタ
 * を変更後、ge_texture_set_format()を呼び出さずに描画する場合、キャッシュを
 * クリアしなければならない。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
extern void ge_flush_texture_cache(void);

/**
 * ge_set_texture_data:  テキスチャのデータポインタとサイズを設定する。
 *
 * 【引　数】 index: テキスチャインデックス（0〜7）
 * 　　　　　  data: テキスチャデータ
 * 　　　　　 width: テキスチャの幅（ピクセル）
 * 　　　　　height: テキスチャの高さ（ピクセル）
 * 　　　　　stride: テキスチャデータのライン長（ピクセル）
 * 【戻り値】なし
 */
extern void ge_set_texture_data(int index, const void *data,
                                int width, int height, int stride);

/**
 * ge_set_texture_draw_mode:  テキスチャ描画方法を設定する。
 *
 * 【引　数】 mode: 描画方法（GE_TEXDRAWMODE_*）
 * 　　　　　alpha: 0以外＝アルファも考慮する、0＝アルファを考慮しない
 * 【戻り値】なし
 */
extern void ge_set_texture_draw_mode(GETextureDrawMode mode, int alpha);

/**
 * ge_set_texture_color:  GE_TEXDRAWMODE_BLENDで使われるテキスチャ色を設定
 * する。
 *
 * 【引　数】color: テキスチャ色（0xBBGGRR）
 * 【戻り値】なし
 */
extern void ge_set_texture_color(uint32_t color);

/**
 * ge_set_texture_filter:  テキスチャの拡大・縮小フィルタを設定する。
 *
 * 【引　数】mag_filter: 拡大時のフィルタ（GE_TEXFILTER_*）
 * 　　　　　min_filter: 縮小時のフィルタ（GE_TEXFILTER_*）
 * 　　　　　mip_filter: 縮小時のmipmap選択フィルタ（GE_TEXMIPFILTER_*）
 * 【戻り値】なし
 */
extern void ge_set_texture_filter(GETextureFilter mag_filter,
                                  GETextureFilter min_filter,
                                  GETextureMipFilter mip_filter);

/**
 * ge_set_texture_mipmap_mode:  テキスチャのmipmap選択モードとバイアスを設定
 * する。
 *
 * バイアスは、指定されたモードにより選択されたmipmapレベルに加算される値で、
 * [-8.0,8.0)の範囲内で設定できる（実際には1/16単位で設定される）。レベルが
 * 1.0上がるごとにテキスチャの解像度が半分になる。
 *
 * 【引　数】mode: mipmap選択モード（GE_MIPMAPMODE_*）
 * 　　　　　bias: mipmap選択バイアス
 * 【戻り値】なし
 */
extern void ge_set_texture_mipmap_mode(GEMipmapMode mode, float bias);

/**
 * ge_set_texture_mipmap_slope:  テキスチャの傾斜型mipmap選択モード
 * （GE_MIPMAPMODE_SLOPE）において、傾斜を設定する。
 *
 * カメラ位置からテキスチャへの距離をdとした場合、mipmapレベルLは以下のよう
 * に選択される。
 * 　　L = 1 + log2(d / slope)
 * つまり、slopeを1.0に設定した場合、カメラからの距離が0.5以下でmipmapレベル
 * 0、1.0でmipmapレベル1、2.0でレベル2、4.0でレベル3などとなり、128.0以上で
 * 最高（解像度最低）のレベル7となる。
 *
 * 【引　数】slope: 傾斜
 * 【戻り値】なし
 */
extern void ge_set_texture_mipmap_slope(float slope);

/**
 * ge_set_texture_format:  テキスチャ形式を設定する。
 *
 * 【引　数】  levels: テキスチャのレベル数（mipmap数）
 * 　　　　　swizzled: 0以外＝データがswizzleされている、0＝swizzleされていない
 * 　　　　　  format: テキスチャのデータ形式（GE_TEXFMT_*）
 * 【戻り値】なし
 */
extern void ge_set_texture_format(int levels, int swizzled, GETexelFormat format);

/**
 * ge_set_texture_wrap_mode:  テキスチャ描画方法を設定する。
 *
 * 【引　数】u_mode: 描画方法（GE_TEXWRAPMODE_*）
 * 　　　　　v_mode: 描画方法（GE_TEXWRAPMODE_*）
 * 【戻り値】なし
 */
extern void ge_set_texture_wrap_mode(GETextureWrapMode u_mode,
                                     GETextureWrapMode v_mode);

/**
 * ge_set_texture_scale:  テキスチャ座標倍率係数を設定する。
 *
 * 【引　数】u_scale: U座標倍率係数
 * 　　　　　v_scale: V座標倍率係数
 * 【戻り値】なし
 */
extern void ge_set_texture_scale(float u_scale, float v_scale);

/**
 * ge_set_texture_offset:  テキスチャ座標オフセットを設定する。
 *
 * 【引　数】u_offset: U座標オフセット
 * 　　　　　v_offset: V座標オフセット
 * 【戻り値】なし
 */
extern void ge_set_texture_offset(float u_offset, float v_offset);

/******************** 頂点登録関数 (ge-util/vertex.c) ********************/

/**
 * ge_add_color_xy_vertex:  色とXY座標を指定した頂点を登録する。
 *
 * 【引　数】color: 色 (0xAABBGGRR)
 * 　　　　　 x, y: 座標
 * 【戻り値】なし
 */
extern void ge_add_color_xy_vertex(uint32_t color, int16_t x, int16_t y);

/**
 * ge_add_color_xy_vertexf:  色と浮動小数点のXY座標を指定した頂点を登録する。
 *
 * 【引　数】color: 色 (0xAABBGGRR)
 * 　　　　　 x, y: 座標
 * 【戻り値】なし
 */
extern void ge_add_color_xy_vertexf(uint32_t color, float x, float y);

/**
 * ge_add_color_xyz_vertexf:  色と浮動小数点のXYZ座標を指定した頂点を
 * 登録する。
 *
 * 【引　数】  color: 色 (0xAABBGGRR)
 * 　　　　　x, y, z: 座標
 * 【戻り値】なし
 */
extern void ge_add_color_xyz_vertexf(uint32_t color, float x, float y, float z);

/**
 * ge_add_uv_xy_vertex:  UV座標とXY座標を指定した頂点を登録する。アライメント
 * の関係で、1つの処理について必ず偶数回呼び出さなければならない。（奇数回に
 * なるような場合はge_add_uv_xy_vertexf()を利用すること）
 *
 * 【引　数】u, v: テキスチャ座標
 * 　　　　　x, y: 座標
 * 【戻り値】なし
 */
extern void ge_add_uv_xy_vertex(int16_t u, int16_t v, int16_t x, int16_t y);

/**
 * ge_add_uv_xyz_vertexf:  浮動小数点のUV座標とXYZ座標を指定した頂点を登録
 * する。
 *
 * 【引　数】    u, v: テキスチャ座標
 * 　　　　　 x, y, z: 座標
 * 【戻り値】なし
 */
extern void ge_add_uv_xyz_vertexf(float u, float v, float x, float y, float z);

/**
 * ge_add_uv_color_xy_vertex:  UV座標、色、XY座標を指定した頂点を登録する。
 *
 * 【引　数】 u, v: テキスチャ座標
 * 　　　　　color: 色 (0xAABBGGRR)
 * 　　　　　 x, y: 座標
 * 【戻り値】なし
 */
extern void ge_add_uv_color_xy_vertex(int16_t u, int16_t v, uint32_t color,
                                      int16_t x, int16_t y);

/**
 * ge_reserve_vertexbytes:  指定されたデータ量を頂点バッファから確保し、
 * メモリ領域へのポインタを返す。
 *
 * 【引　数】size: 確保する領域のサイズ（バイト）
 * 【戻り値】頂点メモリ領域へのポインタ（NULL＝確保できなかった）
 */
extern void *ge_reserve_vertexbytes(int size);

/*************************************************************************/

#endif  // SYSDEP_PSP_GE_UTIL_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
