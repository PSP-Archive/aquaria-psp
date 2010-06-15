#
# Aquaria PSP port
# Copyright (C) 2010 Andrew Church <achurch@achurch.org>
#
# tools/Makefile: Build control file for tool programs.
#

###########################################################################

# System environment settings

EXEEXT =
# Be sure to define either IS_LITTLE_ENDIAN or IS_BIG_ENDIAN here.
SYS_CFLAGS = -DIS_LITTLE_ENDIAN

ARCH = $(shell uname -m)
ifeq ($(ARCH),i686)
SYS_CFLAGS += -msse2 -DUSE_SSE2
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

CFLAGS_png  =
LDFLAGS_png =
LIBS_png    = -lpng

CFLAGS_vorbis  =
LDFLAGS_vorbis =
LIBS_vorbis    = -lvorbisfile -lvorbis -logg

CFLAGS_lame  =
LDFLAGS_lame =
LIBS_lame    = -lmp3lame

######################################

# Programs to build and their sources/flags

PROGRAMS = build-pkg oggtomp3 pngtotex

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
	rm -f $(PROGRAMS:%=%$(EXEEXT))

build-pkg$(EXEEXT): $(build-pkg_SOURCES) $(build-pkg_DEPS) Makefile
	$(CC) $(build-pkg_CFLAGS) $(build-pkg_SOURCES) \
		$(build-pkg_LDFLAGS) $(build-pkg_LIBS) -o $@

oggtomp3$(EXEEXT): $(oggtomp3_SOURCES) $(oggtomp3_DEPS) Makefile
	$(CC) $(oggtomp3_CFLAGS) $(oggtomp3_SOURCES) \
		$(oggtomp3_LDFLAGS) $(oggtomp3_LIBS) -o $@

pngtotex$(EXEEXT): $(pngtotex_SOURCES) $(pngtotex_DEPS) Makefile
	$(CC) $(pngtotex_CFLAGS) $(pngtotex_SOURCES) \
		$(pngtotex_LDFLAGS) $(pngtotex_LIBS) -o $@

###########################################################################
