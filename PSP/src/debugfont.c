/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/debugfont.c: A small debug font for memory and CPU usage displays.
 */

#include "common.h"
#include "debugfont.h"
#include "resource.h"
#include "sysdep.h"

#ifdef DEBUG  // ファイル全体にかかる

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/*
 * Font:  フォントデータをまとめた構造体。
 *
 * フォントには、通常（アルファ型）の8ビットフォントと画像型の32ビットフォ
 * ントがある。両方を同じ構造体で管理し、8ビットフォントの場合はdataフィー
 * ルドを、32ビットフォントの場合はimagedataフィールドにデータへのポインタ
 * を格納する。もう一方のポインタを必ずNULLに設定し、NULLになっていない方
 * を確認することでフォントの種類を判定する。
 *
 * 実際の描画データのほか、各文字には文字幅と、文字を描画する前・描画した後
 * に移動する距離（カーニングデータ）がある。これらのデータは、Unicode文字
 * 番号を基に以下の4つの配列に格納される。
 * 　・dataofs[]  : dataまたはimagedataから文字のデータまでのピクセルオフセット
 * 　・charwidth[]: 文字幅（描画データのピクセル幅）
 * 　・prekern[]  : 描画前、右に移動するピクセル数（負数＝左へ移動）
 * 　・postkern[] : 描画後、右に移動するピクセル数（負数＝左へ移動）
 *
 * 各配列は2階層からなり、文字番号の上位8ビットが示す上位配列要素が、256エ
 * ントリーの下位配列を指す。文字番号の下位8ビットがその下位配列の添え字と
 * なり、文字データを取得する。たとえば、Unicode文字0x1234の場合、文字の描
 * 画データは &data[dataofs[0x12][0x34]] で求められる。これによって、必要メ
 * モリ量を抑えながら全てのUnicode文字に対応できる。
 *
 * なお、文字が一つも登録されていないページ(※)の場合、全ての上位ポインタが
 * NULLとなるので、データを取得する際にはデータの有無を確認しておかなければ
 * ならない。
 * （※）ここで「ページ」は、連続した256文字のブロックを指す。たとえば
 * 　　　Unicode文字0x0000〜0x00FFは0番ページ、0x3000〜0x30FFは0x30番ページ
 * 　　　など。
 */
typedef struct Font_ Font;
struct Font_ {
    uint8_t id;                 // フォントID (FONT_*)
    uint8_t height;             // フォントの高さ（ピクセル）
    uint8_t *data;              // 通常フォントデータ (0 = 透明、255 = 真っ白)
    uint32_t *imagedata;        // 画像型フォントデータ
    int data_stride;            // フォントデータの1ラインの長さ（ピクセル）
    int data_height;            // フォントデータの総ライン数（length/stride）
    uint32_t *dataofs[256];     // フォントデータ内の文字オフセット
                                // （例：Unicode文字0x1234のデータは
                                // 　　　&data[dataofs[0x12][0x34]]にある。
                                // 　　　imagedataの場合も同様）
    uint8_t *charwidth[256];    // 文字幅データ
    int8_t *prekern[256];       // カーニングデータ
    int8_t *postkern[256];
    Font *italic;               // 斜体フォントデータ。斜体フォントでは設定禁止
};

/* デバッグフォントのデータ（実行時に生成） */
static Font *debugfont;

/* フォントデータ用リソース管理構造体 */
DEFINE_STATIC_RESOURCEMANAGER(font_resmgr, 1);

/*************************************************************************/

/* 斜体文字の変形率（3：縦3ピクセル毎に右へ1ピクセルシフト）。
 * USE_COMPUTED_ITALICが無効の場合は無意味。 */
#define ITALICSLANT 3

/* USE_COMPUTED_ITALICを定義すると、英語フォントにおいて、専用の斜体フォント
 * を用いず、通常フォントを傾けて斜体にする。専用フォントより文字が多少ぼや
 * けてしまうが、どこが斜体か、どこが通常かがはっきりみえる。
 *
 * 注：処理はすでに省かれているので意味無し */
//#define USE_COMPUTED_ITALIC

/*************************************************************************/

/* ローカル関数宣言 */

static float font_text_width(const Font * const font, const char *str,
			     const float scale, const int style,
			     float * const lastkern_ptr);
static float font_draw_text(const Font * const font, Texture * const dest,
			    const char *str, float x, float y, uint32_t color,
			    const float alpha, const float scale,
			    const int style);

static int getchar(const char **strptr);
static float internal_drawtext_image(const Font * const basefont,
                                     Texture * const dest, const char *str,
                                     float x, float y, const float alpha,
                                     const float scale, const int style);
#ifdef Not_used_in_Aquaria
static float internal_drawtext_plain(const Font * const basefont,
                                     Texture * const dest, const char *str,
                                     float x, float y,
                                     uint32_t color, const float alpha,
                                     const float scale, const int style);
#endif

/*************************************************************************/
/************************** インタフェース関数 ***************************/
/*************************************************************************/

/**
 * debugfont_init:  デバッグフォント機能の初期化を行う。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
int debugfont_init(void)
{
    /* 文字データ。先頭（高位）の1..10ビットを無視して、残りをビットマップ
     * として利用する。（つまりラインデータが0xC0...0xDFの場合、文字幅が
     * 5ピクセルとなる。） */
    static const struct {
        uint8_t ch;
        uint8_t data[5];
    } chars[] = {
        {' ', {0xE0,0xE0,0xE0,0xE0,0xE0}},
        {'!', {0xE4,0xE4,0xE4,0xE0,0xE4}},
        {'"', {0xEA,0xEA,0xE0,0xE0,0xE0}},
        {'#', {0x94,0xBE,0x94,0xBE,0x94}},
        {'$', {0x9C,0xA8,0x9C,0x8A,0x9C}},
        {'%', {0xB2,0xB4,0x88,0x96,0xA6}},
        {'&', {0x98,0xA4,0x98,0xA6,0x9A}},
        {'\'',{0xE4,0xE4,0xE0,0xE0,0xE0}},
        {'(', {0xE2,0xE4,0xE4,0xE4,0xE2}},
        {')', {0xE8,0xE4,0xE4,0xE4,0xE8}},
        {'*', {0x88,0xAA,0x9C,0xAA,0x88}},
        {'+', {0xE0,0xE4,0xEE,0xE4,0xE0}},
        {',', {0xE0,0xE0,0xE0,0xE6,0xEC}},
        {'-', {0xC0,0xC0,0xDE,0xC0,0xC0}},
        {'.', {0xF0,0xF0,0xF0,0xF6,0xF6}},
        {'/', {0x82,0x84,0x88,0x90,0xA0}},
        {'0', {0xCC,0xD2,0xD2,0xD2,0xCC}},
        {'1', {0xC4,0xCC,0xC4,0xC4,0xCE}},
        {'2', {0xCC,0xD2,0xC4,0xC8,0xDE}},
        {'3', {0xDC,0xC2,0xCC,0xC2,0xDC}},
        {'4', {0xC2,0xC6,0xCA,0xDE,0xC2}},
        {'5', {0xDE,0xD0,0xDC,0xC2,0xDC}},
        {'6', {0xC4,0xC8,0xDC,0xD2,0xCC}},
        {'7', {0xDE,0xC2,0xC4,0xC4,0xC4}},
        {'8', {0xCC,0xD2,0xCC,0xD2,0xCC}},
        {'9', {0xCC,0xD2,0xCE,0xC4,0xC8}},
        {':', {0xC0,0xC8,0xC0,0xC8,0xC0}},
        {';', {0xC0,0xC8,0xC0,0xC8,0xD0}},
        {'<', {0xE2,0xE4,0xE8,0xE4,0xE2}},
        {'=', {0xC0,0xDE,0xC0,0xDE,0xC0}},
        {'>', {0xE8,0xE4,0xE2,0xE4,0xE8}},
        {'?', {0xCC,0xD2,0xC4,0xC0,0xC4}},
        {'@', {0xCC,0xD6,0xD6,0xD0,0xCC}},
        {'A', {0xCC,0xD2,0xDE,0xD2,0xD2}},
        {'B', {0xDC,0xD2,0xDC,0xD2,0xDC}},
        {'C', {0xCE,0xD0,0xD0,0xD0,0xCE}},
        {'D', {0xDC,0xD2,0xD2,0xD2,0xDC}},
        {'E', {0xDE,0xD0,0xDC,0xD0,0xDE}},
        {'F', {0xDE,0xD0,0xDC,0xD0,0xD0}},
        {'G', {0xCC,0xD0,0xD6,0xD2,0xCC}},
        {'H', {0xD2,0xD2,0xDE,0xD2,0xD2}},
        {'I', {0xEE,0xE4,0xE4,0xE4,0xEE}},
        {'J', {0xC2,0xC2,0xC2,0xD2,0xCC}},
        {'K', {0xD2,0xD4,0xD8,0xD4,0xD2}},
        {'L', {0xD0,0xD0,0xD0,0xD0,0xDE}},
        {'M', {0xA2,0xB6,0xAA,0xAA,0xA2}},
        {'N', {0xD2,0xDA,0xD6,0xD2,0xD2}},
        {'O', {0xCC,0xD2,0xD2,0xD2,0xCC}},
        {'P', {0xDC,0xD2,0xDC,0xD0,0xD0}},
        {'Q', {0xCC,0xD2,0xD2,0xD6,0xCE}},
        {'R', {0xDC,0xD2,0xDC,0xD4,0xD2}},
        {'S', {0xCE,0xD0,0xCC,0xC2,0xDC}},
        {'T', {0xBE,0x88,0x88,0x88,0x88}},
        {'U', {0xD2,0xD2,0xD2,0xD2,0xCC}},
        {'V', {0xA2,0xA2,0xA2,0x94,0x88}},
        {'W', {0xA2,0xA2,0xAA,0xAA,0x94}},
        {'X', {0xA2,0x94,0x88,0x94,0xA2}},
        {'Y', {0xA2,0x94,0x88,0x88,0x88}},
        {'Z', {0xBE,0x84,0x88,0x90,0xBE}},
        {'[', {0xEE,0xE8,0xE8,0xE8,0xEE}},
        {'\\',{0xA0,0x90,0x88,0x84,0x82}},
        {']', {0xEE,0xE2,0xE2,0xE2,0xEE}},
        {'^', {0xE4,0xEA,0xE0,0xE0,0xE0}},
        {'_', {0x80,0x80,0x80,0x80,0xBE}},
        {'`', {0xE8,0xE4,0xE0,0xE0,0xE0}},
        {'{', {0xE2,0xE4,0xEC,0xE4,0xE2}},
        {'|', {0xE4,0xE4,0xE4,0xE4,0xE4}},
        {'}', {0xE8,0xE4,0xE6,0xE4,0xE8}},
        {'~', {0x9A,0xAC,0x80,0x80,0x80}},
    };
    /* use integer; if (/^$/) {printf "},\n{"; $s="";} else {$n=0xFFFFFFFE; while (s/^([^|])//) {$n = ($n<<1 | ($1 eq " " ? 0 : 1)) & 0xFFFFFFFF} printf "%s0x%08X",$s,$n; $s=",";} */

    Font *font;
    int i;

    resource_create(&font_resmgr, 1);

    const int width = 8;
    const int height = 6;
    const uint32_t datasize = align_up(sizeof(*font), 64)
        + 256*6 + (width * height * lenof(chars) * 4);
    if (!resource_new_data(&font_resmgr, (void **)&debugfont, datasize, 64,
                           RES_ALLOC_TOP | RES_ALLOC_CLEAR)) {
        DMSG("Out of memory for font structure");
        return 0;
    }
    font = debugfont;
    font->height = height;
    font->data_stride = width;
    font->data_height = height * lenof(chars);
    font->data = NULL;
    font->dataofs  [0x00] =
        (uint32_t *)((uint8_t *)font + align_up(sizeof(*font), 64));
    font->charwidth[0x00] = (uint8_t *)font->dataofs[0] + 256*4;
    font->prekern  [0x00] = ( int8_t *)font->dataofs[0] + 256*5;
    font->imagedata       = (uint32_t *)((uint8_t *)font->dataofs[0] + 256*6);

    for (i = 0; i < lenof(chars); i++) {
        int ch = chars[i].ch;
      dochar:
        font->dataofs[0][ch] = i * font->height * font->data_stride;
        unsigned int x, y;
        for (y = 0; y < 5; y++) {
            uint32_t *dest = font->imagedata + font->dataofs[0][ch]
                                             + y * font->data_stride;
            uint8_t n = chars[i].data[y];
            unsigned int c = 8;
            while (n & 0x80) {
                n <<= 1;
                c--;
            }
            n <<= 1;
            x = 0;
            while (--c) {
                if (n & 0x80) {
                    dest[x] = 0xFFFFFFFF;
                }
                n <<= 1;
                x++;
            }
        }
        font->charwidth[0][ch] = x;
        if (ch >= 'A' && ch <= 'Z') {
            /* 大文字のデータを小文字としても登録 */
            ch += 'a' - 'A';
            goto dochar;
        }
    }

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * debugfont_cleanup:  デバッグフォントデータを破棄する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void debugfont_cleanup(void)
{
    resource_delete(&font_resmgr);
}

/*************************************************************************/

/**
 * debugfont_height:  デバッグフォントの文字の高さを返す。
 *
 * 【引　数】scale: 文字の拡大倍率（1.0＝拡大・縮小しない）
 * 【戻り値】文字の高さ（ピクセル）
 */
float debugfont_height(float scale)
{
    return debugfont->height * scale;
}

/*-----------------------------------------------------------------------*/

/**
 * debugfont_textwidth:  UTF-8文字列の幅を計算して返す。style引数における
 * 描画フラグ（FONTSTYLE_ALIGN_*）は無視される。
 *
 * 【引　数】         str: 文字列（UTF-8）
 * 　　　　　       scale: 文字の拡大倍率（1.0＝拡大・縮小しない）
 * 　　　　　       style: 文字スタイル（FONTSTYLE_*）
 * 　　　　　lastkern_ptr: 最終文字の右側カーニング値を格納する変数への
 * 　　　　　              ポインタ（NULL可）。この値は戻り値には含まれない。
 * 【戻り値】文字列を描画した際の合計幅（0＝エラーまたは空文字列）
 */
float debugfont_textwidth(const char *str, float scale, int style,
			  float *lastkern_ptr)
{
    if (UNLIKELY(!str)) {
        DMSG("str == NULL");
        return 0;
    }
    if (UNLIKELY(!debugfont)) {
        DMSG("Font not initialized");
        return 0;
    }
    return font_text_width(debugfont, str, scale, style, lastkern_ptr);
}

/*-----------------------------------------------------------------------*/

/**
 * debugfont_draw_text:  UTF-8文字列を描画する。
 *
 * 【引　数】  str: 文字列（UTF-8）
 * 　　　　　 x, y: 画面座標（左上隅、小数点以下有意）
 * 　　　　　color: 文字色（0xRRGGBB）
 * 　　　　　alpha: 濃度（0.0〜1.0、0.0＝透明）
 * 　　　　　scale: 文字の拡大倍率（1.0＝拡大・縮小しない）
 * 　　　　　style: 文字スタイル・描画フラグ（FONTSTYLE_*）
 * 【戻り値】文字列の幅（ピクセル、0＝エラーまたは空文字列）
 */
float debugfont_draw_text(const char *str, float x, float y, uint32_t color,
			  float alpha, float scale, int style)
{
    if (UNLIKELY(!str)) {
        DMSG("str == NULL");
        return 0;
    }
    if (UNLIKELY(!*str)) {
        return 0;  // 空文字列
    }
    if (UNLIKELY(!debugfont)) {
        DMSG("Font not initialized");
        return 0;
    }
    return font_draw_text(debugfont, NULL, str, x, y, color, alpha, scale,
                          style);
}

/*************************************************************************/
/***************************** ローカル関数 ******************************/
/*************************************************************************/

/**
 * font_text_width:  UTF-8文字列の、指定されたフォントで描画したときの幅を
 * 計算して返す。style引数における描画フラグ（FONTSTYLE_ALIGN_*）は無視される。
 *
 * 【引　数】        font: 基礎（非斜体）フォント構造体へのポインタ
 * 　　　　　         str: 文字列（UTF-8）
 * 　　　　　       scale: 文字の拡大倍率（1.0＝拡大・縮小しない）
 * 　　　　　       style: 文字スタイル（FONTSTYLE_*）
 * 　　　　　lastkern_ptr: 最終文字の右側カーニング値を格納する変数への
 * 　　　　　              ポインタ（NULL可）。この値は戻り値には含まれない。
 * 【戻り値】文字列を描画した際の合計幅（0＝エラーまたは空文字列）
 */
static float font_text_width(const Font * const font, const char *str,
			     const float scale, const int style,
			     float * const lastkern_ptr)
{
    PRECOND_SOFT(font != NULL, return 0);
    PRECOND_SOFT(str != NULL, return 0);

    float width = 0, lastkern = 0;
    unsigned int ch;
    while ((ch = getchar(&str)) != 0) {
        const Font *thisfont;
        if (!(style & FONTSTYLE_ITALIC)
         || !font->italic
         || !font->italic->dataofs[ch>>8]
         || !font->italic->charwidth[ch>>8]
        ) {
            thisfont = font;
        } else {
            thisfont = font->italic;
        }
        width += lastkern;
        width += (font->prekern  [ch>>8] ? font->prekern  [ch>>8][ch&0xFF] : 0)
                 * scale;
        width += (font->charwidth[ch>>8] ? font->charwidth[ch>>8][ch&0xFF] : 0)
                 * scale;
        lastkern=(font->postkern [ch>>8] ? font->postkern [ch>>8][ch&0xFF] : 0)
                 * scale;
    }

    if (lastkern_ptr) {
        *lastkern_ptr = lastkern;
    }
    return width;
}

/*************************************************************************/

/**
 * font_draw_text:  指定されたフォントでUTF-8文字列を描画する。
 *
 * 【引　数】 font: 基礎（非斜体）フォント構造体へのポインタ
 * 　　　　　 dest: 描画バッファ（NULLで画面に描画。ツールではNULL禁止）
 * 　　　　　  str: 文字列（UTF-8）
 * 　　　　　 x, y: 画面座標（左上隅、小数点以下有意）
 * 　　　　　color: 文字色（0xRRGGBB）
 * 　　　　　alpha: 濃度（0〜1、0＝透明）
 * 　　　　　scale: 文字の拡大倍率（1.0＝拡大・縮小しない）
 * 　　　　　style: 文字スタイル・描画フラグ（FONTSTYLE_*）
 * 【戻り値】文字列の幅（ピクセル、0＝エラーまたは空文字列）
 */
static float font_draw_text(const Font * const font, Texture * const dest,
			    const char *str, float x, float y, uint32_t color,
			    const float alpha, const float scale,
			    const int style)
{
    PRECOND_SOFT(font != NULL, return 0);
    PRECOND_SOFT(str != NULL, return 0);
#ifdef IN_TOOL
    PRECOND_SOFT(dest != NULL, return 0);
#else
    PRECOND_SOFT(dest == NULL, return 0);
#endif

    if (font->imagedata != NULL) {
        if (style & FONTSTYLE_SHADOW) {
            DMSG("Warning: FONTSTYLE_SHADOW not supported on image fonts");
        }
        return internal_drawtext_image(font, dest, str, x, y, alpha,
                                       scale, style);
    } else {
#ifdef Not_used_in_Aquaria
        if (style & FONTSTYLE_SHADOW) {
             internal_drawtext_plain(font, dest, str, iroundf(x+scale),
                                     iroundf(y+scale), 0, alpha,
                                     scale, style & ~FONTSTYLE_SHADOW);
        }
        return internal_drawtext_plain(font, dest, str, x, y, color, alpha,
                                       scale, style & ~FONTSTYLE_SHADOW);
#else
	DMSG("Non-image font drawing disabled");
	return 0;
#endif
    }
}

/*************************************************************************/

/**
 * getchar:  UTF-8文字列の頭にある文字を取得し、文字列ポインタを次の文字に
 * 進める。
 *
 * 【引　数】strptr: UTF-8文字列ポインタへのポインタ
 * 【戻り値】先頭文字の文字コード、0＝失敗（文字列の終端など）
 */
static int getchar(const char **strptr)
{
    int a, nbytes;
    uint16_t ch;

    PRECOND(strptr != NULL);
    PRECOND(*strptr != NULL);

  next:
    a = ((const uint8_t *)(*strptr))[0];
    if (!a) {
        return 0;
    } else if (a < 0x80) {
        ch = a;
        nbytes = 1;
        (*strptr)++;
    } else if (a < 0xC0) {
        /* 無効なバイト */
        (*strptr)++;
        goto next;
    } else if (a < 0xE0) {
        ch = a & 0x1F;
        nbytes = 2;
        (*strptr)++;
    } else if (a < 0xF0) {
        ch = a & 0x1F;
        nbytes = 3;
        (*strptr)++;
    } else {
        /* 0x10000以上の文字は使わない */
        (*strptr)++;
        goto next;
    }
    while (--nbytes > 0) {
        a = ((const uint8_t *)(*strptr))[0];
        if (a < 0x80 || a >= 0xC0) {
            /* 無効なバイト。先頭バイトとして処理し直す */
            goto next;
        }
        ch = ch<<6 | (a & 0x3F);
        (*strptr)++;
    }

    return ch;
}

/*************************************************************************/

/**
 * internal_drawtext_image:  画像型フォントを使って、UTF-8文字列を描画する。
 *
 * 【引　数】basefont: 基礎（非斜体）フォント構造体へのポインタ
 * 　　　　　    dest: 画像バッファ（NULLで画面に描画）
 * 　　　　　     str: 文字列（UTF-8）
 * 　　　　　    x, y: 画面座標（左上隅、小数点以下有意）
 * 　　　　　   alpha: 濃度（0〜1、0＝透明）
 * 　　　　　   scale: 文字の拡大倍率（1.0＝拡大・縮小しない）
 * 　　　　　   style: 文字スタイル・描画フラグ（FONTSTYLE_*）
 * 【戻り値】文字列の幅（ピクセル、0＝エラーまたは空文字列）
 */
static float internal_drawtext_image(const Font * const basefont,
                                     Texture * const dest, const char *str,
                                     float x, float y, const float alpha,
                                     const float scale, const int style)
{
    PRECOND_SOFT(basefont != NULL, return 0);
    PRECOND_SOFT(basefont->imagedata != NULL, return 0);
    PRECOND_SOFT(str != NULL, return 0);
#ifdef IN_TOOL
    PRECOND_SOFT(dest != NULL, return 0);
#else
    PRECOND_SOFT(dest == NULL, return 0);
#endif

    const Font *font;
    if (style & FONTSTYLE_ITALIC) {
        if (basefont->italic && basefont->italic->imagedata) {
            font = basefont->italic;
        } else {
            DMSG("Italic font not available & computed italic disabled");
            font = basefont;
        }
    } else {
        font = basefont;
    }
    const int height          = font->height;
    const uint32_t *data      = font->imagedata;
    const int32_t data_stride = font->data_stride;
#ifndef IN_TOOL
    const int32_t data_height = font->data_height;
#endif

    struct {
        int32_t dataofs, datawidth;
        float outofs, outwidth;
    } chararray[300];  // 充分に長い
#ifndef IN_TOOL
    SysBlitList blitlist[lenof(chararray)];
#endif
    unsigned int ch;
    int i = 0;
    float width = 0;
    while ((ch = getchar(&str)) != 0 && i < lenof(chararray)-1) {
        const int32_t dataofs = font->dataofs[ch>>8]
                                ? font->dataofs[ch>>8][ch&0xFF] : 0;
        const int charwidth   = font->charwidth[ch>>8]
                                ? font->charwidth[ch>>8][ch&0xFF] : 0;
        const int prekern     = font->prekern[ch>>8]
                                ? font->prekern[ch>>8][ch&0xFF] : 0;
        const int postkern    = font->postkern[ch>>8]
                                ? font->postkern[ch>>8][ch&0xFF] : 0;
        width += prekern * scale;
        chararray[i].dataofs   = dataofs;
        chararray[i].datawidth = charwidth;
        chararray[i].outofs    = width;
        chararray[i].outwidth  = charwidth * scale;
        width += (charwidth + postkern) * scale;
        i++;
    }
    chararray[i].dataofs = -1;  // 文字列終端

    if (style & FONTSTYLE_ALIGN_RIGHT) {
        x -= roundf(width);  // ぼやけ回避のため、調整量を整数とする
    } else if (style & FONTSTYLE_ALIGN_CENTER) {
        x -= roundf(width/2);
    }

    int blitcount = 0;
    for (i = 0; chararray[i].dataofs >= 0; i++) {
        int srcx = chararray[i].dataofs;
        int srcw = chararray[i].datawidth;
        const float destx = x + chararray[i].outofs;
        const float desty = y;
        const float destw = chararray[i].outwidth;
        const float desth = height * scale;
        const uint32_t color = iroundf(alpha*255)<<24 | 0xFFFFFF;
        blitlist[blitcount].srcx  = srcx % data_stride;
        blitlist[blitcount].srcy  = srcx / data_stride;
        blitlist[blitcount].srcw  = srcw;
        blitlist[blitcount].srch  = height;
        blitlist[blitcount].dest[0].x = destx;
        blitlist[blitcount].dest[0].y = desty;
        blitlist[blitcount].dest[0].color = color;
        blitlist[blitcount].dest[1].x = destx + destw;
        blitlist[blitcount].dest[1].y = desty;
        blitlist[blitcount].dest[1].color = color;
        blitlist[blitcount].dest[2].x = destx;
        blitlist[blitcount].dest[2].y = desty + desth;
        blitlist[blitcount].dest[2].color = color;
        blitlist[blitcount].dest[3].x = destx + destw;
        blitlist[blitcount].dest[3].y = desty + desth;
        blitlist[blitcount].dest[3].color = color;
        blitcount++;
    }
    if (blitcount > 0) {
        sys_display_blit_list(data, NULL, data_stride, data_height,
                              blitlist, blitcount, 0);
    }

    return width;
}

/*************************************************************************/

#ifdef Not_used_in_Aquaria

/**
 * internal_drawtext_plain:  非画像型フォントを使って、UTF-8文字列を描画する。
 *
 * 【引　数】basefont: 基礎（非斜体）フォント構造体へのポインタ
 * 　　　　　    dest: 画像バッファ（NULLで画面に描画）
 * 　　　　　     str: 文字列（UTF-8）
 * 　　　　　    x, y: 画面座標（左上隅、小数点以下有意）
 * 　　　　　   color: 文字色（0xRRGGBB）
 * 　　　　　   alpha: 濃度（0〜1、0＝透明）
 * 　　　　　   scale: 文字の拡大倍率（1.0＝拡大・縮小しない）
 * 　　　　　   style: 文字スタイル・描画フラグ（FONTSTYLE_*）
 * 【戻り値】文字列の幅（ピクセル、0＝エラーまたは空文字列）
 */
static float internal_drawtext_plain(const Font * const basefont,
                                     Texture * const dest, const char *str,
                                     float x, float y,
                                     uint32_t color, const float alpha,
                                     const float scale, const int style)
{
    PRECOND_SOFT(basefont != NULL, return 0);
    PRECOND_SOFT(basefont->data != NULL, return 0);
    PRECOND_SOFT(str != NULL, return 0);
#ifdef IN_TOOL
    PRECOND_SOFT(dest != NULL, return 0);
#else
    PRECOND_SOFT(dest == NULL, return 0);
#endif

    const Font *font;
    if (style & FONTSTYLE_ITALIC) {
        if (basefont->italic && basefont->italic->data) {
            font = basefont->italic;
        } else {
            DMSG("Italic font not available & computed italic disabled");
            font = basefont;
        }
    } else {
        font = basefont;
    }
    const int height     = font->height;
    const uint8_t *data  = font->data;
    const int data_stride = font->data_stride;
#ifdef USE_ALLEGREX
    const int data_stride_height = data_stride * height;  // アセンブリ用定数
#endif
#ifdef IN_TOOL
    /* 描画用に、色値を各成分に分割する */
    const uint32_t cR = color>>16 & 0xFF;
    const uint32_t cG = color>> 8 & 0xFF;
    const uint32_t cB = color>> 0 & 0xFF;
#else
    color = PACK_ARGB(0, color>>16 & 0xFF, color>>8 & 0xFF, color>>0 & 0xFF);
#endif

    struct {
        const uint8_t *data;
        float outofs, charwidth;
    } chararray[500];  // 充分に長い
    unsigned int ch;
    int i = 0, width = 0, last_postkern = 0;
    int left_overflow = 0;  // prekernで左側よりオーバフローした分（ピクセル）
    while ((ch = getchar(&str)) != 0 && i < lenof(chararray)-1) {
        chararray[i].data = data + (font->dataofs[ch>>8]
                                    ? font->dataofs[ch>>8][ch&0xFF] : 0);
        const int charwidth = font->charwidth[ch>>8]
                              ? font->charwidth[ch>>8][ch&0xFF] : 0;
        const int prekern   = font->prekern[ch>>8]
                              ? font->prekern[ch>>8][ch&0xFF] : 0;
        const int postkern  = font->postkern[ch>>8]
                              ? font->postkern[ch>>8][ch&0xFF] : 0;
        width += prekern;
        if (width < -left_overflow) {
            left_overflow = -width;
        }
        chararray[i].outofs = width;
        chararray[i].charwidth = charwidth;
        width += charwidth + postkern;
        /* 最後の文字のpostkern値が負数の場合、文字の右端が切り捨てられるの
         * で、その場合は下のright_overflowに登録して画像バッファを広くする */
        last_postkern = postkern;
        i++;
    }
    chararray[i].data = NULL;  // 文字列終端

#ifndef IN_TOOL
    const int right_overflow = lbound(-last_postkern, 0);
    Image *outimage = graphics_image_new_temp(
        left_overflow + width + right_overflow, height
    );
    if (UNLIKELY(!outimage)) {
        DMSG("Failed to allocate image for drawing text (%dx%d)",
             left_overflow + width - last_postkern, height);
        return 0;
    }
    const int outstride = outimage->stride;
    mem_fill32(outimage->data, 0, 4 * outstride * outimage->height);
#endif  // IN_TOOL

    for (i = 0; chararray[i].data != NULL; i++) {
        const uint8_t *src = chararray[i].data;
        const int charwidth = chararray[i].charwidth;
        uint32_t *destptr =
            outimage->data + left_overflow + iroundf(chararray[i].outofs);
# ifdef USE_ALLEGREX
        /* 注：ソースデータのリードに1バイトのオーバーランが生じる場合あり */
        asm(".set push; .set noreorder\n\
            addu $a3, %[src], %[data_stride_height] # $a3 <- 終端ポインタ\n\
         0: addu $a2, %[src], %[charwidth]  # $a2 <- ライン終端ポインタ \n\
         1: beq %[src], $a2, 8f     # ループ終了チェック                \n\
         2: lbu $t0, 0(%[src])      # $t0 <- ソースピクセル             \n\
            addiu %[src], %[src], 1                                     \n\
            addiu %[dest], %[dest], 4                                   \n\
            beqz $t0, 1b            # 透明ピクセルなら次に進む          \n\
            sll $t1, $t0, 24        # $t1 <- 出力ピクセル               \n\
            or $t1, $t1, %[color]   # 色を追加                          \n\
            bne %[src], $a2, 2b                                         \n\
            sw $t1, -4(%[dest])     # 遅延スロットでピクセルを格納      \n\
         8: addu %[src], %[src], %[src_skip]                            \n\
            bne %[src], $a3, 0b                                         \n\
            addu %[dest], %[dest], %[dest_skip]                         \n\
            .set pop"
            : [src] "=r" (src), [dest] "=r" (destptr), "=m" (*destptr)
            : "0" (src), [src_skip] "r" (data_stride - charwidth),
              [data_stride_height] "r" (data_stride_height),
              "1" (destptr), [dest_skip] "r" ((outstride - charwidth) * 4),
              [charwidth] "r" (charwidth), [color] "r" (color)
            : "a2", "a3", "t0", "t1"
        );
# else
        int yy;
        for (yy = 0; yy < height; yy++, src+=data_stride, destptr+=outstride) {
            int xx;
            for (xx = 0; xx < charwidth; xx++) {
                // 1バイト、0〜255を4バイト、0x00xxxxxx〜0xFFxxxxxxに変換
                const uint32_t val = src[xx];
                if (val > 0) {
                    destptr[xx] = val<<24 | color;
                }
            }
        }
# endif  // USE_ALLEGREX
    }

    if (style & FONTSTYLE_ALIGN_RIGHT) {
        x -= roundf(width * scale);
    } else if (style & FONTSTYLE_ALIGN_CENTER) {
        x -= roundf((width * scale) / 2);
    }
    x -= left_overflow;
    const int w = outimage->width, h = outimage->height;
    graphics_image_draw_ext(outimage, 0, 0, w, h, x, y, w*scale, h*scale,
                            iroundf(alpha*255)<<24 | 0xFFFFFF, 0, 0);

    return width * scale;
}

#endif  // Not_used_in_Aquaria

/*************************************************************************/
/*************************************************************************/

#endif  // DEBUG

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
