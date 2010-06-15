/*compile-command:
set -e
set -x
psp-gcc -DSYSDEP_COMMON_H=\"sysdep-psp/common.h\" -Wall -Werror -g -O2 -G0 -I/usr/psp/sdk/include -specs=/usr/psp/sdk/lib/prxspecs -Wl,-q,-T/usr/psp/sdk/lib/linkfile.prx -L/usr/psp/sdk/lib "$0" src/{debugfont,graphics,init,input,malloc,memory,texture,timer,util}.c src/resource/?*.c src/sound/?*.c src/sysdep-psp/?*.c src/sysdep-psp/ge-util/?*.c -o `basename "$0" .c`.elf -lvorbisfile -lvorbis -logg -lpng -lz -lm -lc -lpspaudio -lpspaudiocodec -lpspctrl -lpspdisplay -lpspgu -lpspge -lpsppower -lpsputility -lpspuser
psp-fixup-imports `basename "$0" .c`.elf
psp-prxgen `basename "$0" .c`.{elf,prx}
exit 0
*/

#include "src/common.h"
#include "src/init.h"
#include "src/input.h"
#include "src/memory.h"
#include "src/sound.h"
#include "src/sysdep.h"
#include "src/sysdep-psp/psplocal.h"

int main(int argc, char **argv)
{
    const char *file = NULL, *file2 = NULL;
    int loop = 0;
    int stream = 0;

    init_all(argc, argv);

    int n = 1;
    while (n < argc && argv[n][0] == '-') {
	if (argv[n][1] == 'l') {
	    loop = 1;
	} else if (argv[n][1] == 's') {
	    stream = 1;
	} else {
	    goto usage;
	}
	n++;
    }
    if (n >= argc) {
      usage:
	DMSG("Usage: %s [-l] [-s] file [file2]", argv[0]);
	exit_all(1);
    }
    file = argv[n];
    if (n+1 < argc) {
	file2 = argv[n+1];
    }

    SoundFormat format = 0, format2 = 0;
    if (strstr(file, ".ogg")) {
	format = SOUND_FORMAT_OGG;
    } else if (strstr(file, ".mp3")) {
	format = SOUND_FORMAT_MP3;
    } else {
	format = SOUND_FORMAT_WAV;
    }
    if (file2) {
	if (strstr(file2, ".ogg")) {
	    format2 = SOUND_FORMAT_OGG;
	} else if (strstr(file2, ".mp3")) {
	    format2 = SOUND_FORMAT_MP3;
	} else {
	    format2 = SOUND_FORMAT_WAV;
	}
    }

    void *data2 = NULL;
    uint32_t filesize2 = 0;
    if (file2) {
	SysFile *f2 = sys_file_open(file2);
	if (!f2) {
	    DMSG("%s: %s", file2, sys_last_errstr());
	    exit_all(1);
	}
	filesize2 = sys_file_size(f2);
	data2 = mem_alloc(filesize2, 0, 0);
	if (!data2) {
	    DMSG("No memory for %s (%u bytes)", file2, filesize2);
	    exit_all(1);
	}
	DMSG("Reading %s...", file2);
	if (sys_file_read(f2, data2, filesize2) != filesize2) {
	    DMSG("Failed to read data from %s", file2);
	    exit_all(1);
	}
	DMSG("done.");
	sys_file_close(f2);
    }

    SysFile *f = sys_file_open(file);
    if (!f) {
	DMSG("%s: %s", file, sys_last_errstr());
	exit_all(1);
    }
    const uint32_t filesize = sys_file_size(f);
    int id;
    if (stream) {
	id = sound_play_file(0, format, f, 0, filesize, 1.0, 0.0, loop);
    } else {
	void *data = mem_alloc(filesize, 0, 0);
	if (!data) {
	    DMSG("No memory for %s (%u bytes)", file, filesize);
	    exit_all(1);
	}
	DMSG("Reading %s...", file);
	if (sys_file_read(f, data, filesize) != filesize) {
	    DMSG("Failed to read data from %s", file);
	    exit_all(1);
	}
	DMSG("done.");
	sys_file_close(f);
	id = sound_play_buffer(0, format, data, filesize, 1.0, 0.0, loop);
    }

    if (!id) {
	DMSG("Failed to start sound");
	exit_all(1);
    }

    int fading = 0, select = 0;
    for (;;) {
	input_update();
	const int button = input_pressed_button();
	if (button == 13) {  // Circle
	    break;
	}
	if (button == 14 && !fading) {  // Cross
	    sound_fade(id, 1.0f);
	    fading = 1;
	}
	if (button == 15 && data2) {  // Square
	    sound_play_buffer(0, format2, data2, filesize2, 1.0, 0.0, 0);
	}
	if (button == 12) {  // Triangle
	    static volatile int counter;
	    auto void counter_thread(void);
	    auto void counter_thread(void) {for(;;) counter++;}
	    counter = 0;
	    int thread = psp_start_thread("counter", (void *)counter_thread,
					  50, 4096, 0, NULL);
	    sceKernelDelayThread(1000000);
	    sceKernelTerminateDeleteThread(thread);
	    DMSG("%.1f%% CPU time free", counter / (222000000/8.084f) * 100);
	}
	if (button == 8) {  // L
	    sys_sound_lock();
	    sceKernelDelayThread(10000);
	    sys_sound_unlock();
	}
	if (sys_input_buttonstate(0)) {  // Select
	    if (!select) {
		sound_adjust_volume(id, 0.3f, 3.0f);
		select = 1;
	    }
	} else {
	    if (select) {
		sound_adjust_volume(id, 1.0f, 2.0f);
		select = 0;
	    }
	}
	if (button == 3) {  // Start
	    DMSG("Playback position: %.3f", sound_playback_pos(id));
	}
	sceDisplayWaitVblankStart();
    }

    exit_all(0);
}
