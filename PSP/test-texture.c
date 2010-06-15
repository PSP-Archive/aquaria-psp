/*compile-command:
set -e
set -x
psp-gcc -DSYSDEP_COMMON_H=\"sysdep-psp/common.h\" -Wall -Werror -g -O2 -G0 -I/usr/psp/sdk/include -specs=/usr/psp/sdk/lib/prxspecs -Wl,-q,-T/usr/psp/sdk/lib/linkfile.prx -L/usr/psp/sdk/lib "$0" src/{debugfont,graphics,init,input,malloc,memory,texture,timer,util}.c src/resource/?*.c src/sound/?*.c src/sysdep-psp/?*.c src/sysdep-psp/ge-util/?*.c -o `basename "$0" .c`.elf -lvorbisfile -lvorbis -logg -lpng -lz -lm -lc -lpspaudio -lpspaudiocodec -lpspctrl -lpspdisplay -lpspgu -lpspge -lpsppower -lpsputility -lpspuser
psp-fixup-imports `basename "$0" .c`.elf
psp-prxgen `basename "$0" .c`.{elf,prx}
exit 0
*/

#include "src/common.h"
#include "src/graphics.h"
#include "src/init.h"
#include "src/input.h"
#include "src/memory.h"
#include "src/sound.h"
#include "src/sysdep.h"
#include "src/texture.h"
#include "src/timer.h"
#include "src/sysdep-psp/ge-util.h"
#include "src/sysdep-psp/psplocal.h"

int main(int argc, char **argv)
{
    const char *file = NULL;
    int do_3d = 0;

    init_all(argc, argv);

    int n = 1;
    while (n < argc && argv[n][0] == '-') {
	if (argv[n][1] == '3') {
	    do_3d = 1;
	} else {
	    goto usage;
	}
	n++;
    }
    if (n >= argc) {
      usage:
	DMSG("Usage: %s [-3] file", argv[0]);
	exit_all(1);
    }
    file = argv[n];

    DMSG("Loading %s...", file);
    Texture *tex = texture_load(file, 0);
    if (!tex) {
	DMSG("Failed to load %s", file);
	exit_all(1);
    }
    DMSG("Done. (tex=%p)", tex);

    for (;;) {
	timer_wait();
	timer_mark(TIMER_MARK_PROCESS_START);
	input_update();
	const int button = input_pressed_button();
	if (button == 13) {  // Circle
	    break;
	}
	uint32_t color;
	if (input_button_state(15)) {  // Square
	    color = 0xFFFF00FF;
	} else {
	    color = 0xFF000000;
	}
	if (button == 12) {  // Triangle
	    extern int dumpflag;
	    dumpflag = 1;
	}

	graphics_start_frame();
	ge_fill(0, 0, 480, 272, color);
	ge_enable(GE_STATE_BLEND);
	ge_set_blend_mode(GE_BLENDSET_SRC_ALPHA);
	ge_enable(GE_STATE_TEXTURE);

	if (do_3d) {
	    Matrix4f m;
	    m._11 = 2/480.0f;
	    m._12 = 0;
	    m._13 = 0;
	    m._14 = 0;
	    m._21 = 0;
	    m._22 = -2/272.0f;
	    m._23 = 0;
	    m._24 = 0;
	    m._31 = 0;
	    m._32 = 0;
	    m._33 = -1;
	    m._34 = 0;
	    m._41 = -1;
	    m._42 = 1;
	    m._43 = 0;
	    m._44 = 1;
	    ge_set_projection_matrix(&m);
	}

	if (tex->indexed) {
	    ge_set_colortable(tex->palette, 256, GE_PIXFMT_8888, 0, 0xFF);
	}
	ge_set_texture_data(0, tex->pixels,
			    tex->width, tex->height, tex->stride);
	ge_set_texture_format(0, tex->swizzled,
			      tex->indexed ? GE_TEXFMT_T8 : GE_TEXFMT_8888);
	ge_set_texture_draw_mode(GE_TEXDRAWMODE_REPLACE, 1);
	ge_set_texture_filter(GE_TEXFILTER_LINEAR, GE_TEXFILTER_LINEAR,
			      GE_TEXMIPFILTER_NONE);

	if (do_3d) {
	    ge_set_vertex_format(GE_VERTEXFMT_TEXTURE_32BITF
			       | GE_VERTEXFMT_VERTEX_32BITF
			       | GE_VERTEXFMT_TRANSFORM_3D);
	} else {
	    ge_set_vertex_format(GE_VERTEXFMT_TEXTURE_16BIT
			       | GE_VERTEXFMT_VERTEX_16BIT
			       | GE_VERTEXFMT_TRANSFORM_2D);
	}
	ge_set_vertex_pointer(NULL);
#if 0
	if (do_3d) {
	    ge_add_uv_xyz_vertexf(0, 0, 0, 0, 0);
	    ge_add_uv_xyz_vertexf(1, 1, tex->width, tex->height, 0);
	} else {
	    ge_add_uv_xy_vertex(0, 0, 0, 0);
	    ge_add_uv_xy_vertex(tex->width, tex->height, tex->width, tex->height);
	}
	ge_draw_primitive(GE_PRIMITIVE_SPRITES, 2);
#else
	if (do_3d) {
	    ge_add_uv_xyz_vertexf(0, 0, 0, 0, 0);
	    ge_add_uv_xyz_vertexf(1, 0, tex->width, 0, 0);
	    ge_add_uv_xyz_vertexf(0, 1, 0, tex->height, 0);
	    ge_add_uv_xyz_vertexf(1, 1, tex->width, tex->height, 0);
	} else {
	    ge_add_uv_xy_vertex(0, 0, 0, 0);
	    ge_add_uv_xy_vertex(tex->width, 0, tex->width, 0);
	    ge_add_uv_xy_vertex(0, tex->height, 0, tex->height);
	    ge_add_uv_xy_vertex(tex->width, tex->height, tex->width, tex->height);
	}
	ge_draw_primitive(GE_PRIMITIVE_TRIANGLE_STRIP, 4);
#endif

	timer_mark(TIMER_MARK_PROCESS_END);

	graphics_finish_frame();
	timer_mark(TIMER_MARK_DISPLAY_END);
    }

    exit_all(0);
}
