/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/timer.c: Timing routines.
 */

#include "common.h"
#include "sysdep.h"
#include "timer.h"

#ifdef DEBUG
# include "debugfont.h"  // デバッグ情報表示のため
# include "graphics.h"   // 同上
#endif

/*************************************************************************/
/*************************** グローバルデータ ****************************/
/*************************************************************************/

#ifdef DEBUG

/* CPU表示フラグ（グローバル） */
int debug_cpu_display_flag;

#endif

/*************************************************************************/
/**************************** ローカルデータ *****************************/
/*************************************************************************/

/* プログラムループ開始時刻 */
static double start_time;

/* 描画されたフレーム数。60fpsの場合、約2年の連続実行で0にリセットされて
 * しまい、タイミングが狂うのだが…… */
static unsigned int frames;

/* 現在のフレームレート */
static double framerate;

/************************************/

#ifdef DEBUG  // CPU使用率測定関連

/* このフレームの処理開始時刻 */
static double frame_start;

/* 直近のフレームにおける処理時間の使用率 */
#define MAX_USAGE  100
static struct {
    double time;
    int id;
} mark[MAX_USAGE];

/* mark[]に登録されているエントリー数 */
static volatile int nmarks;  // 複数スレッドからアクセスされる可能性がある

/* 前のフレームのmark[]データを種類別に分けたCPU使用率データ */
static float usage[TIMER_MARK__NUM];

#endif

/*************************************************************************/
/**************************** インタフェース *****************************/
/*************************************************************************/

/**
 * timer_init:  タイマーを初期化する。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void timer_init(void)
{
    start_time = sys_time_now();
    frames = 0;
    framerate = sys_time_base_framerate();
    if (framerate <= 0) {
        DMSG("sys_time_base_framerate() returned invalid value %.3f,"
             " assuming 20fps", framerate);
        framerate = 0.05;
    }
#ifdef DEBUG
    mem_clear(usage, sizeof(usage));
    mark[0].id = TIMER_MARK_START;  // 0番エントリは固定
    nmarks = 1;
    frame_start = start_time;
    mark[0].time = frame_start;
#endif
}

/*************************************************************************/

/**
 * timer_left:  現在のフレームの残り処理時間（フレームスキップを起こさない
 * ために処理を終えなければならない時間）を返す。
 *
 * 【引　数】なし
 * 【戻り値】フレームの残り処理時間（秒）
 */
float timer_left(void)
{
    const double target = start_time + (frames * framerate);
    const double now    = sys_time_now();
    return (float)(target - now);
}

/*************************************************************************/

/**
 * timer_wait:  次のフレーム処理時刻まで待つ。
 *
 * 【引　数】なし
 * 【戻り値】前のフレーム処理開始からの名目経過時間（実値をフレームレートの
 * 　　　　　倍数に切り下げたもの。秒単位）
 */
float timer_wait(void)
{
    float retval = framerate;

    frames++;
    const double target = start_time + (frames * framerate);
    const double now    = sys_time_now();
    const double delay  = target - now;

    if (delay < 0) {
        const int skipped_frames = itrunc(-delay / framerate);
        if (skipped_frames > 0) {
            DMSG("Lost %d frame%s", skipped_frames, skipped_frames==1?"":"s");
            frames += skipped_frames;
            retval +=
                ubound(skipped_frames,MAX_SKIPPED_FRAMES) * (float)framerate;
        }
        sys_time_delay(0);  // 必ず呼び出す
    } else {
        sys_time_delay(delay);
    }

#ifdef DEBUG
    const int nmarks_ = nmarks;
    const int totmarks = ubound(nmarks_, lenof(mark));
    mem_clear(usage, sizeof(usage));
    int curtype = mark[0].id;
    int i;
    for (i = 1; i < totmarks; i++) {
        usage[curtype] += lbound(mark[i].time - mark[i-1].time, 0) / framerate;
        curtype = mark[i].id;
    }
    usage[curtype] += (now - mark[totmarks-1].time) / framerate;

    nmarks = 1;
    frame_start = sys_time_now();
    mark[0].id = TIMER_MARK_START;
    mark[0].time = 0;
#endif

    return retval;
}

/*************************************************************************/

/**
 * timer_reset:  現時点までに蓄積した遅延をクリアする。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void timer_reset(void)
{
    DMSG("Frame %d: TIMER RESET", frames);
    start_time = sys_time_now() - frames * framerate;
#ifdef DEBUG
    frame_start = sys_time_now();
#endif
}

/*************************************************************************/

/**
 * timer_setrate:  フレームレートを設定する。0を指定すると、基本フレーム
 * レート（sys_time_base_framerate()の戻り値）に戻す。
 *
 * 【引　数】rate: フレームレート（秒／フレーム）
 * 【戻り値】なし
 */
void timer_setrate(double rate)
{
    if (rate < 0) {
        DMSG("Invalid rate: %.3f", rate);
        return;
    } else if (rate == 0) {
        rate = sys_time_base_framerate();
        if (rate <= 0) {
            DMSG("sys_time_base_framerate() returned invalid rate: %.3f",
                 rate);
            return;
        }
    }
    framerate = rate;
    DMSG("TIMER SETRATE 1/%.2fs, resetting", 1/framerate);
    timer_reset();
}

/*************************************************************************/

/**
 * timer_getrate, timer_getratef:  現在のフレームレートを返す。timer_getrate()
 * では倍精度で、timer_getratef()では単精度で返す。
 *
 * 【引　数】なし
 * 【戻り値】現在のフレームレート（秒／フレーム）
 */
double timer_getrate(void)
{
    POSTCOND(framerate > 0);
    return framerate;
}

float timer_getratef(void)
{
    POSTCOND((float)framerate > 0);
    return (float)framerate;
}

/*************************************************************************/
/*************************************************************************/

#ifdef DEBUG

/*************************************************************************/

/**
 * timer_mark:  現時点までの処理時間使用率を記録する。
 *
 * 【引　数】id: マークの種類（TIMER_MARK_*）
 * 【戻り値】なし
 */
void timer_mark(int id)
{
    if (id < 0 || id >= TIMER_MARK__NUM) {
        DMSG("Invalid ID: %d", id);
        return;
    }
    const double time = sys_time_now() - frame_start;
    /* 排他処理として完璧ではないが、デバッグ機能としては充分 */
    const int index = nmarks++;
    if (index >= 0 && index < lenof(mark)) {
        mark[index].time = (float)time;
        mark[index].id = id;
    }
}

/*************************************************************************/

/**
 * timer_display_debuginfo:  Ctrl+Cを監視し、押された時にCPU使用状況を表示
 * する。デバッグ時のみ実装。
 *
 * 【引　数】なし
 * 【戻り値】なし
 */
void timer_display_debuginfo(void)
{
    if (!debug_cpu_display_flag) {
        return;
    }

    /* この関数で使われた時間を計測し、表示から除く */
    static float avg_used_here = 0;
    const double starttime = sys_time_now();


    /* 表示処理 */

    static double last_cpu_digits;  // 前回数値を描画した時刻
    static float usage_avg[TIMER_MARK__NUM];  // 各処理種類の平均使用率
    static float usage_max;         // 記録中の最大使用率
    static int usage_displayed;     // 現在、数値として表示されている使用率
    const unsigned int x = 52;
    const unsigned int y = graphics_display_height() - 5;
    const unsigned int w = (graphics_display_width()-4) - x;
    unsigned int i;
    char buf[16];

    /* 表示領域を黒にクリア */
    graphics_fill_box(0, y, graphics_display_width(), 5, 0xFF000000);

    /* この関数の、これまでの平均処理時間をPROCESS_ENDから除く */
    usage[TIMER_MARK_PROCESS_END] =
        lbound(usage[TIMER_MARK_PROCESS_END] - (avg_used_here/framerate), 0);

    /* 平均使用率を更新。細かい変動の影響はなるべく抑える */
    for (i = 0; i < TIMER_MARK__NUM; i++) {
        const float this_usage = usage[i];
        const float old_avg = usage_avg[i];
        const float factor = ubound(fabsf(this_usage - old_avg), 1) * 0.75f;
        usage_avg[i] = old_avg*(1-factor) + this_usage*factor;
    }
    /* 最大使用率を更新 */
    float usage_total = usage[TIMER_MARK_START] +
                        usage[TIMER_MARK_PROCESS_START] +
                        usage[TIMER_MARK_PROCESS_MID] +
                        usage[TIMER_MARK_PROCESS_END];
    if (usage_total > usage_max) {
        usage_max = usage_total;
    }
    /* 1秒に2回、表示されている使用率（数値）を更新 */
    const double testval = floor(sys_time_now()*2);
    if (last_cpu_digits != testval) {
        last_cpu_digits = testval;
        usage_displayed = iroundf(ubound(usage_max,10.0f) * 1000);
        usage_max = 0;
    }

    /* 数値を描画 */
    if (usage_displayed < 10000) {
        snprintf(buf, sizeof(buf), "CPU:%3d.%d%%",
                 usage_displayed/10, usage_displayed%10);
    } else {
        snprintf(buf, sizeof(buf), "CPU:---.-%%");
    }
    debugfont_draw_text(buf, 0, y, 0xFFFFFF, 1, 1, 0);

    /* 使用率バーを描画。
     * 　　白：ゲーム前処理（入力など）
     * 　　赤：ゲーム処理
     * 　　緑：ゲーム描画処理
     * 　　青：描画最終処理
     */
    const int whitebar  = iroundf(ubound(usage_avg[TIMER_MARK_START],
                                         1) * w);
    const int redbar    = iroundf(ubound(usage_avg[TIMER_MARK_START] +
                                         usage_avg[TIMER_MARK_PROCESS_START],
                                         1) * w);
    const int greenbar  = iroundf(ubound(usage_avg[TIMER_MARK_START] +
                                         usage_avg[TIMER_MARK_PROCESS_START] +
                                         usage_avg[TIMER_MARK_PROCESS_MID],
                                         1) * w);
    const int bluebar   = iroundf(ubound(usage_avg[TIMER_MARK_START] +
                                         usage_avg[TIMER_MARK_PROCESS_START] +
                                         usage_avg[TIMER_MARK_PROCESS_MID] +
                                         usage_avg[TIMER_MARK_PROCESS_END],
                                         1) * w);
    /* w==0はgraphics_fill_box()で検出されるので、ここでチェックしなくていい */
    graphics_fill_box(x,           y+1, whitebar,          3, 0xFFFFFFFF);
    graphics_fill_box(x+whitebar,  y+1, redbar-whitebar,   3, 0xFFFF0000);
    graphics_fill_box(x+redbar,    y+1, greenbar-redbar,   3, 0xFF00FF00);
    graphics_fill_box(x+greenbar,  y+1, bluebar-greenbar,  3, 0xFF0000FF);
    graphics_fill_box(x+bluebar,   y+1, w-bluebar,         3, 0xFF555555);

    /* 使用率10%毎に目盛りを入れる */
    for (i = 1; i <= 9; i++) {
        const int thisx = x + (w*i+5)/10;
        graphics_draw_line(thisx, y+1, thisx, y+3, 0xFF000000);
    }

    /* 処理時間を計算して記録しておく */
    const float timeused = (float)(sys_time_now() - starttime);
    avg_used_here = (avg_used_here * 0.9f) + (timeused * 0.1f);
}

/*************************************************************************/

#endif  // DEBUG

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
