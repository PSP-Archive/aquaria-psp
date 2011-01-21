#
# Aquaria PSP port
# Copyright (C) 2010 Andrew Church <achurch@achurch.org>
#
# tools/Makefile: Build control file for tool programs.
#

###########################################################################

# System environment settings
#
# Note: Define GENERIC=1 to build AquariaPSPBuilder for a generic system.
# This disables the use of SSE2 on 32-bit x86 and links the PNG/Ogg/LAME
# libraries statically.

EXEEXT =
# Be sure to define either IS_LITTLE_ENDIAN or IS_BIG_ENDIAN here.
SYS_CFLAGS = -DIS_LITTLE_ENDIAN

ARCH = $(shell uname -m)
ifeq ($(ARCH),i686)
ifneq ($(GENERIC),)
SYS_CFLAGS += -mtune=generic
else
SYS_CFLAGS += -msse2 -DUSE_SSE2
endif
endif
ifeq ($(ARCH),x86_64)
SYS_CFLAGS += -DUSE_SSE2 -DARCH_AMD64
endif

# Common programs/flags

CC      = gcc
CFLAGS  = -g -O2 -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror
LDFLAGS =
LIBS    =

# Flags for various libraries

ifeq ($(shell uname -s | cut -c1-5 | tr A-Z a-z),mingw)
GTKSYS = win32
else
GTKSYS = x11
endif
CFLAGS_gtk  = $(shell pkg-config --cflags gtk+-$(GTKSYS)-2.0)
LDFLAGS_gtk =
LIBS_gtk    = $(filter-out -lpng%,$(shell pkg-config --libs gtk+-$(GTKSYS)-2.0))

CFLAGS_png  =
LDFLAGS_png =
LIBS_png    = -lpng -lz

CFLAGS_vorbis  =
LDFLAGS_vorbis =
LIBS_vorbis    = -lvorbisfile -lvorbis -logg

CFLAGS_lame  =
LDFLAGS_lame =
LIBS_lame    = -lmp3lame

ifneq ($(GENERIC),)
LIBS_png    := $(LIBS_png:-l%=/usr/lib/lib%.a)
LIBS_vorbis := $(LIBS_vorbis:-l%=/usr/lib/lib%.a)
LIBS_lame   := $(LIBS_lame:-l%=/usr/lib/lib%.a)
endif

######################################

# Programs to build and their sources/flags

PROGRAMS = AquariaPSPBuilder build-pkg oggtomp3 pngtotex

AquariaPSPBuilder_SOURCES = AquariaPSPBuilder.c quantize.c zoom.c
AquariaPSPBuilder_AUTOGEN = AquariaPSPBuilder-eboot.i \
                            AquariaPSPBuilder-scripts.i \
                            AquariaPSPBuilder-ui.i
AquariaPSPBuilder_DEPS    = ../src/common.h ../src/resource/package-pkg.h \
                            ../src/texture.h quantize.h sse2.h zoom.h \
                            $(AquariaPSPBuilder_AUTOGEN)
AquariaPSPBuilder_CFLAGS  = $(SYS_CFLAGS) $(CFLAGS) -DIN_TOOL $(CFLAGS_gtk) \
                            $(CFLAGS_vorbis) $(CFLAGS_lame) $(CFLAGS_png) \
                            -DHG_REVISION=\"$(shell hg id -n)\"
AquariaPSPBuilder_LDFLAGS = $(LDFLAGS) $(LDFLAGS_gtk) \
                            $(LDFLAGS_vorbis) $(LDFLAGS_lame) $(LDFLAGS_png)
AquariaPSPBuilder_LIBS    = $(LIBS) $(LIBS_gtk) \
                            $(LIBS_vorbis) $(LIBS_lame) $(LIBS_png)

build-pkg_SOURCES = build-pkg.c
build-pkg_DEPS    = ../src/common.h ../src/resource/package-pkg.h
build-pkg_CFLAGS  = $(SYS_CFLAGS) $(CFLAGS) -DIN_TOOL

oggtomp3_SOURCES  = oggtomp3.c
oggtomp3_DEPS     = ../src/common.h
oggtomp3_CFLAGS   = $(SYS_CFLAGS) $(CFLAGS) -DIN_TOOL $(CFLAGS_vorbis) $(CFLAGS_lame)
oggtomp3_LDFLAGS  = $(LDFLAGS) $(LDFLAGS_vorbis) $(LDFLAGS_lame)
oggtomp3_LIBS     = $(LIBS)    $(LIBS_vorbis)    $(LIBS_lame)

pngtotex_SOURCES  = pngtotex.c quantize.c zoom.c
pngtotex_DEPS     = ../src/common.h ../src/texture.h quantize.h sse2.h zoom.h
pngtotex_CFLAGS   = $(SYS_CFLAGS) $(CFLAGS) -DIN_TOOL $(CFLAGS_png)
pngtotex_LDFLAGS  = $(LDFLAGS) $(LDFLAGS_png)
pngtotex_LIBS     = $(LIBS)    $(LIBS_png)

###########################################################################

# Build rules

.PHONY: all clean
all: $(PROGRAMS:%=%$(EXEEXT))
clean:
	rm -f $(PROGRAMS:%=%$(EXEEXT)) $(AquariaPSPBuilder_AUTOGEN)

AquariaPSPBuilder$(EXEEXT): $(AquariaPSPBuilder_SOURCES) $(AquariaPSPBuilder_DEPS) Makefile
	$(CC) $(AquariaPSPBuilder_CFLAGS) $(AquariaPSPBuilder_SOURCES) \
		$(AquariaPSPBuilder_LDFLAGS) $(AquariaPSPBuilder_LIBS) -o $@

build-pkg$(EXEEXT): $(build-pkg_SOURCES) $(build-pkg_DEPS) Makefile
	$(CC) $(build-pkg_CFLAGS) $(build-pkg_SOURCES) \
		$(build-pkg_LDFLAGS) $(build-pkg_LIBS) -o $@

oggtomp3$(EXEEXT): $(oggtomp3_SOURCES) $(oggtomp3_DEPS) Makefile
	$(CC) $(oggtomp3_CFLAGS) $(oggtomp3_SOURCES) \
		$(oggtomp3_LDFLAGS) $(oggtomp3_LIBS) -o $@

pngtotex$(EXEEXT): $(pngtotex_SOURCES) $(pngtotex_DEPS) Makefile
	$(CC) $(pngtotex_CFLAGS) $(pngtotex_SOURCES) \
		$(pngtotex_LDFLAGS) $(pngtotex_LIBS) -o $@

#-------------------------------------

AquariaPSPBuilder-eboot.i: ../aquaria.prx ../PARAM.SFO Makefile
	perl \
		-e 'foreach (@ARGV) {' \
		-e '    my ($$id, $$file) = split(/=/, $$_);' \
		-e '    print "static const unsigned char $${id}[] = {\n";' \
		-e '    my $$size = 0;' \
		-e '    open F, "<$$file" or die "$$file: $$!\n";' \
		-e '    while (read(F, $$_, 16) > 0) {' \
		-e '        print join(",",unpack("C*",$$_)) . ",\n";' \
		-e '        $$size += length($$_);' \
		-e '    }' \
		-e '    close F;' \
		-e '    print "};\nstatic const unsigned int $${id}_size = $$size;\n";' \
		-e '}' \
		'aquaria_prx=../aquaria.prx' 'param_sfo=../PARAM.SFO' >$@
../PARAM.SFO: ../aquaria.prx
../aquaria.prx:
	make -C ..

AquariaPSPBuilder-scripts.i: FORCE
	find ../../game_scripts/scripts -name \*.lua \
	| perl \
		-e 'print "static const struct {const char *path, *data;} script_data[] = {\n";' \
		-e 'while (<STDIN>) {' \
		-e '    my $$path = $$_;' \
		-e '    chomp $$path;' \
		-e '    open F, "<$$path" or die "$$path: $$!\n";' \
		-e '    my $$data;' \
		-e '    {local $$/ = undef; $$data = <F>;}' \
		-e '    close F;' \
		-e '    $$path =~ s,^\.\./\.\./game_scripts/,, or die "Bad path: $$path\n";' \
		-e '    $$data =~ s/([\\"])/\\$$1/g;' \
		-e '    $$data =~ s/\n/\\n\\\n/g;' \
		-e '    print "{\"$$path\", \"$$data\"},\n"' \
		-e '}' \
		-e 'print "};\n";' \
		>$@

.PHONY: FORCE
FORCE:

AquariaPSPBuilder-ui.i: AquariaPSPBuilder.glade Makefile
	perl \
		-e 'print "static const char ui_glade[] = \"\\\n";' \
		-e 'while (<STDIN>) {' \
		-e '    s/([\\"])/\\$$1/g;' \
		-e '    s/\n/\\n\\\n/;' \
		-e '    print;' \
		-e '}' \
		-e 'print "\";\n";' \
		<$< >$@

###########################################################################
