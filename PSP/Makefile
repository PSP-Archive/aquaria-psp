#
# Aquaria PSP port
# Copyright (C) 2010 Andrew Church <achurch@achurch.org>
#
# Makefile: Main build control file for the PSP port.
#

###########################################################################

# The following variables can be customized (either by giving them on the
# "make" command line or by editing this file directly) for your build
# environment:

#-------------------------------------

# $(PSPROOT):  Base directory containing headers and libraries used in
# cross-compilation for the PSP.  This should contain "include/stdio.h"
# and "lib/libc.a", for example.

PSPROOT = /usr/psp

#-------------------------------------

# $(PSPSDK):  Directory into which the unofficial SDK from http://ps2dev.org
# has been installed.  This should contain "include/pspuser.h", for example.

PSPSDK = $(PSPROOT)/sdk

#-------------------------------------

# $(USE_INSTALLED_...):  Each of these variables selects whether to use the
# version of a specific package included in the Aquaria source code
# distribution or a version you have installed on your system.  Setting
# each variable to 1 causes the build to search for the package's headers
# and libraries under $(PSPROOT); any other value causes the build to use
# the included source.  Using preinstalled libraries will naturally reduce
# compilation time, but may also introduce subtle bugs if there are any
# interface incompatibilities between Aquaria and the package version you
# have installed.

USE_INSTALLED_FREETYPE = 1
USE_INSTALLED_LIBOGG = 1
USE_INSTALLED_LIBVORBIS = 1
USE_INSTALLED_LUA = 1

#-------------------------------------

# CAN_OVERRIDE_STRTOF:  If set to 1, the code will assume it can override
# the strtof() function provided by libc with a custom, optimized version.
# This is not the case for the stock distribution of newlib (at least
# through 1.16.0), but can be fixed by adding __attribute__((weak)) to the
# strtof() definition in newlib/libc/stdlib/strtod.c.
#
# If you set this to 1 and attempt to build with a library that does not
# allow overriding, you'll get "multiple definition" errors during the
# final link.
#
# Setting this to 0 will cause the build to use the standard library's
# version of strtof().  For Newlib, this is significantly slower due to
# the function's internal use of double-precision floating point values.

CAN_OVERRIDE_STRTOF = 1

#-------------------------------------

# WARNINGS_AS_ERRORS:  If set to 1, the source code will be compiled with
# -Werror to cause the build to abort if a warning is detected.

WARNINGS_AS_ERRORS = 0

#-------------------------------------

# ALLOW_SETTINGS_OVERRIDE:  If set to 1, the program will attempt to open
# "usersettings.xml" from the installation directory before loading the
# standard settings file from the PSP save data directory.  If the
# "usersettings.xml" file is present, its settings will be used instead
# of the settings from the standard file.
#
# Note that "usersettings.xml" is _not_ updated when options are changed,
# so the file must be removed or renamed for changed settings to be
# recognized.

ALLOW_SETTINGS_OVERRIDE = 0

#-------------------------------------

# DISABLE_SOUND_CACHE:  If set to 1, caching of sound effects in memory
# will be disabled.  This frees up precious memory for game data, but
# it will cause the game to freeze every time a sound effect is played
# unless you use a package file for game data and you have a sufficiently
# fast Memory Stick.

DISABLE_SOUND_CACHE = 1

#-------------------------------------

# ICON0_SOURCE_PNG, ICON0_LEFT, ICON0_TOP, ICON0_WIDTH, ICON0_HEIGHT:
# Set these to define the PNG source file for the icon used in the PSP game
# menu as well as the user settings ("system data") and achievements files
# in the save data folder.  The specified portion of the image (left/top/
# width/height) will be extracted from the PNG file, resized to 144x80
# pixels, and used as the icon.
#
# If ICON0_SOURCE_PNG is empty, no icon will be generated.

# Example using the "Aquaria" logo from the title screen:
#ICON0_SOURCE_PNG = /data/games/aquaria/gfx/title/logo.png
#ICON0_LEFT   = 210
#ICON0_TOP    =  89
#ICON0_WIDTH  = 630
#ICON0_HEIGHT = 350

#-------------------------------------

# CUSTOM_BUILD_ID:  Set this to append custom text to the Aquaria version
# number displayed on the Aquaria title screen.  No separator is inserted
# between the version number and this text, so it should include any
# necessary leading punctuation or space.
#
# Note that "make" automatically trims leading space from all variable
# values, so if you want to include an initial space, you must prefix it
# with a reference to an empty variable, as in the second example below
# (which adds " (custom)" to the version number).

#CUSTOM_BUILD_ID = -custom
#CUSTOM_BUILD_ID = $(emptystring) (custom)

###########################################################################

# Common programs/flags.

CC       = psp-gcc
CXX      = psp-g++
CFLAGS   = -pipe -g -O3 -G0 -I$(PSPSDK)/include \
           -Wall -Wstrict-prototypes -Wmissing-prototypes $(-Werror)
CXXFLAGS = -pipe -g -O3 -G0 -I$(PSPSDK)/include -fsigned-char -Wall $(-Werror)
LDFLAGS  = -specs=$(PSPSDK)/lib/prxspecs -Wl,-q,-T$(PSPSDK)/lib/linkfile.prx \
           -L$(PSPSDK)/lib
LIBS     = $(-llua) $(-lfreetype) $(-lvorbisfile) $(-lvorbis) $(-logg) \
           -lpng -lz -lstdc++ -lm -lc \
           -lpspaudio -lpspaudiocodec -lpspctrl -lpspdisplay -lpspge \
           -lpsppower -lpsputility -lpspuser

ifeq ($(CAN_OVERRIDE_STRTOF),1)
-DCAN_OVERRIDE_STRTOF = -DCAN_OVERRIDE_STRTOF
else
-DCAN_OVERRIDE_STRTOF =
endif

ifeq ($(WARNINGS_AS_ERRORS),1)
-Werror = -Werror
else
-Werror =
endif

ifeq ($(ALLOW_SETTINGS_OVERRIDE),1)
-DAQUARIA_ALLOW_PSP_SETTINGS_OVERRIDE = -DAQUARIA_ALLOW_PSP_SETTINGS_OVERRIDE
else
-DAQUARIA_ALLOW_PSP_SETTINGS_OVERRIDE =
endif

ifeq ($(DISABLE_SOUND_CACHE),1)
-DBBGE_DISABLE_SOUND_CACHE = -DBBGE_DISABLE_SOUND_CACHE
else
-DBBGE_DISABLE_SOUND_CACHE =
endif

ifeq ($(CUSTOM_BUILD_ID),)
-DAQUARIA_CUSTOM_BUILD_ID =
else
-DAQUARIA_CUSTOM_BUILD_ID = -DAQUARIA_CUSTOM_BUILD_ID='"$(CUSTOM_BUILD_ID)"'
endif

######################################

# Source sets and set-specific flags.

EXTLIB_DIR       = ../ExternalLibs

BBGE_DIR         = ../BBGE
BBGE_SOURCES     = $(BBGE_DIR)/ActionInput.cpp \
                   $(BBGE_DIR)/ActionMapper.cpp \
                   $(BBGE_DIR)/ActionSet.cpp \
                   $(BBGE_DIR)/AfterEffect.cpp \
                   $(BBGE_DIR)/AnimatedSprite.cpp \
                   $(BBGE_DIR)/Base.cpp \
                   $(BBGE_DIR)/BitmapFont.cpp \
                   $(BBGE_DIR)/Collision.cpp \
                   $(BBGE_DIR)/Core.cpp \
                   $(BBGE_DIR)/Cube.cpp \
                   $(BBGE_DIR)/DarkLayer.cpp \
                   $(BBGE_DIR)/Datafile.cpp \
                   $(BBGE_DIR)/DebugFont.cpp \
                   $(BBGE_DIR)/DFSprite.cpp \
                   $(BBGE_DIR)/Effects.cpp \
                   $(BBGE_DIR)/Emitter.cpp \
                   $(BBGE_DIR)/Event.cpp \
                   $(BBGE_DIR)/Flags.cpp \
                   $(BBGE_DIR)/FrameBuffer.cpp \
                   $(BBGE_DIR)/Gradient.cpp \
                   $(BBGE_DIR)/Interpolator.cpp \
                   $(BBGE_DIR)/Joystick.cpp \
                   $(BBGE_DIR)/LensFlare.cpp \
                   $(BBGE_DIR)/LightCone.cpp \
                   $(BBGE_DIR)/Light.cpp \
                   $(BBGE_DIR)/Math.cpp \
                   $(BBGE_DIR)/PackRead.cpp \
                   $(BBGE_DIR)/ParticleEffect.cpp \
                   $(BBGE_DIR)/ParticleManager.cpp \
                   $(BBGE_DIR)/Particles.cpp \
                   $(BBGE_DIR)/Precacher.cpp \
                   $(BBGE_DIR)/ProfRender.cpp \
                   $(BBGE_DIR)/Quad.cpp \
                   $(BBGE_DIR)/QuadTrail.cpp \
                   $(BBGE_DIR)/RenderObject.cpp \
                   $(BBGE_DIR)/RenderObjectLayer.cpp \
                   $(BBGE_DIR)/RenderRect.cpp \
                   $(BBGE_DIR)/Resource.cpp \
                   $(BBGE_DIR)/RoundedRect.cpp \
                   $(BBGE_DIR)/ScreenTransition.cpp \
                   $(BBGE_DIR)/Shader.cpp \
                   $(BBGE_DIR)/SkeletalSprite.cpp \
                   $(BBGE_DIR)/Slider.cpp \
                   $(BBGE_DIR)/SoundManager.cpp \
                   $(BBGE_DIR)/SpawnParticleData.cpp \
                   $(BBGE_DIR)/StateMachine.cpp \
                   $(BBGE_DIR)/StateManager.cpp \
                   $(BBGE_DIR)/Strings.cpp \
                   $(BBGE_DIR)/Texture.cpp \
                   $(BBGE_DIR)/TTFFont.cpp \
                   $(BBGE_DIR)/Vector.cpp \
                   $(BBGE_DIR)/FmodPSPBridge.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTCharmap.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTContour.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTFace.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTFont.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTGLTextureFont.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTGlyph.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTGlyphContainer.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTLibrary.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTPoint.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTSize.cpp \
                   $(EXTLIB_DIR)/FTGL/src/FTTextureGlyph.cpp \
                   $(EXTLIB_DIR)/glfont2/glfont2.cpp \
                   $(EXTLIB_DIR)/tinyxml.cpp \
                   $(EXTLIB_DIR)/tinyxmlerror.cpp \
                   $(EXTLIB_DIR)/tinyxmlparser.cpp
BBGE_OBJECTS    := $(BBGE_SOURCES:%.cpp=%.o)
BBGE_CXXFLAGS    = $(CXXFLAGS) -Wno-reorder -Wno-sign-compare \
                   -Wno-strict-overflow -Wno-switch \
                   $(-I_freetype) $(-I_libogg) $(-I_libvorbis) \
                   -Isrc $(-DCAN_OVERRIDE_STRTOF) \
                   -I$(BBGE_DIR) -I$(BBGE_DIR)/GL \
                   -I$(EXTLIB_DIR)/FTGL/include \
                   -DHAVE_STRCASECMP=1 -DTIXML_USE_STL=1 \
                   -DBBGE_BUILD_PSP=1 -DBBGE_BUILD_OPENGL=1 \
                   -DGL_GLEXT_LEGACY=1 -DBBGE_BUILD_WIDESCREEN=1 \
                   -DBBGE_BUILD_FMOD_PSP_BRIDGE=1 \
                   -DBBGE_BUILD_ACHIEVEMENTS_INTERNAL=1 \
                   $(-DBBGE_DISABLE_SOUND_CACHE)

#-------------------------------------

Aquaria_DIR      = ../Aquaria
Aquaria_SOURCES  = $(Aquaria_DIR)/AnimationEditor.cpp \
                   $(Aquaria_DIR)/AquariaComboBox.cpp \
                   $(Aquaria_DIR)/AquariaMenuItem.cpp \
                   $(Aquaria_DIR)/AquariaProgressBar.cpp \
                   $(Aquaria_DIR)/AquariaSaveSlot.cpp \
                   $(Aquaria_DIR)/AutoMap.cpp \
                   $(Aquaria_DIR)/Avatar.cpp \
                   $(Aquaria_DIR)/Beam.cpp \
                   $(Aquaria_DIR)/BitBlotLogo.cpp \
                   $(Aquaria_DIR)/BoxElement.cpp \
                   $(Aquaria_DIR)/CollideEntity.cpp \
                   $(Aquaria_DIR)/Continuity.cpp \
                   $(Aquaria_DIR)/Credits.cpp \
                   $(Aquaria_DIR)/CurrentRender.cpp \
                   $(Aquaria_DIR)/Demo.cpp \
                   $(Aquaria_DIR)/DSQ.cpp \
                   $(Aquaria_DIR)/Element.cpp \
                   $(Aquaria_DIR)/Emote.cpp \
                   $(Aquaria_DIR)/Entity.cpp \
                   $(Aquaria_DIR)/FlockEntity.cpp \
                   $(Aquaria_DIR)/Game.cpp \
                   $(Aquaria_DIR)/GameplayVariables.cpp \
                   $(Aquaria_DIR)/GasCloud.cpp \
                   $(Aquaria_DIR)/GridRender.cpp \
                   $(Aquaria_DIR)/Hair.cpp \
                   $(Aquaria_DIR)/Ingredient.cpp \
                   $(Aquaria_DIR)/Intro.cpp \
                   $(Aquaria_DIR)/Logo.cpp \
                   $(Aquaria_DIR)/Main.cpp \
                   $(Aquaria_DIR)/ManaBall.cpp \
                   $(Aquaria_DIR)/MiniMapRender.cpp \
                   $(Aquaria_DIR)/Mod.cpp \
                   $(Aquaria_DIR)/ModSelector.cpp \
                   $(Aquaria_DIR)/ParticleEditor.cpp \
                   $(Aquaria_DIR)/Path.cpp \
                   $(Aquaria_DIR)/PathFinding.cpp \
                   $(Aquaria_DIR)/PathRender.cpp \
                   $(Aquaria_DIR)/Protect.cpp \
                   $(Aquaria_DIR)/RecipeMenuEntry.cpp \
                   $(Aquaria_DIR)/SceneEditor.cpp \
                   $(Aquaria_DIR)/SchoolFish.cpp \
                   $(Aquaria_DIR)/ScriptedEntity.cpp \
                   $(Aquaria_DIR)/ScriptInterface.cpp \
                   $(Aquaria_DIR)/Segmented.cpp \
                   $(Aquaria_DIR)/SFXLoops.cpp \
                   $(Aquaria_DIR)/Shot.cpp \
                   $(Aquaria_DIR)/Spore.cpp \
                   $(Aquaria_DIR)/States.cpp \
                   $(Aquaria_DIR)/StatsAndAchievements.cpp \
                   $(Aquaria_DIR)/SteamRender.cpp \
                   $(Aquaria_DIR)/Strand.cpp \
                   $(Aquaria_DIR)/StringBank.cpp \
                   $(Aquaria_DIR)/SubtitlePlayer.cpp \
                   $(Aquaria_DIR)/ToolTip.cpp \
                   $(Aquaria_DIR)/UserSettings.cpp \
                   $(Aquaria_DIR)/WaterFont.cpp \
                   $(Aquaria_DIR)/WaterSurfaceRender.cpp \
                   $(Aquaria_DIR)/Web.cpp \
                   $(Aquaria_DIR)/WorldMapRender.cpp \
                   $(Aquaria_DIR)/WorldMapTiles.cpp
Aquaria_OBJECTS := $(Aquaria_SOURCES:%.cpp=%.o)
Aquaria_CXXFLAGS = $(BBGE_CXXFLAGS) $(-I_lua) -I$(Aquaria_DIR) \
                   -DAQUARIA_NODRM -DAQUARIA_FULL -DAQUARIA_SAVE_MAPVIS_RAW \
                   $(-DAQUARIA_ALLOW_PSP_SETTINGS_OVERRIDE) \
                   $(-DAQUARIA_CUSTOM_BUILD_ID)

#-------------------------------------

PSP_SOURCES     := $(wildcard src/*.c) \
                   $(wildcard src/resource/*.c) \
                   $(wildcard src/sound/*.c) \
                   $(wildcard src/sysdep-psp/*.c) \
                   $(wildcard src/sysdep-psp/ge-util/*.c) \
                   $(wildcard src/test/*.c)
PSP_OBJECTS     := $(PSP_SOURCES:%.c=%.o)
PSP_CFLAGS       = $(CFLAGS) $(-I_libogg) $(-I_libvorbis) \
                   -DSYSDEP_COMMON_H=\"sysdep-psp/common.h\" \
                   -DCXX_CONSTRUCTOR_HACK $(-DCAN_OVERRIDE_STRTOF)

######################################

# Optional package handling.

PACKAGE_OBJECTS :=

#-------------------------------------

freetype_DIR       = $(EXTLIB_DIR)/freetype2
freetype_SOURCES   = $(freetype_DIR)/src/base/ftsystem.c \
                     $(freetype_DIR)/src/base/ftdebug.c \
                     $(freetype_DIR)/src/base/ftinit.c \
                     $(freetype_DIR)/src/base/ftbase.c \
                     $(freetype_DIR)/src/base/ftbbox.c \
                     $(freetype_DIR)/src/base/ftbdf.c \
                     $(freetype_DIR)/src/base/ftbitmap.c \
                     $(freetype_DIR)/src/base/ftcid.c \
                     $(freetype_DIR)/src/base/ftfstype.c \
                     $(freetype_DIR)/src/base/ftgasp.c \
                     $(freetype_DIR)/src/base/ftglyph.c \
                     $(freetype_DIR)/src/base/ftgxval.c \
                     $(freetype_DIR)/src/base/ftlcdfil.c \
                     $(freetype_DIR)/src/base/ftmm.c \
                     $(freetype_DIR)/src/base/ftotval.c \
                     $(freetype_DIR)/src/base/ftpatent.c \
                     $(freetype_DIR)/src/base/ftpfr.c \
                     $(freetype_DIR)/src/base/ftstroke.c \
                     $(freetype_DIR)/src/base/ftsynth.c \
                     $(freetype_DIR)/src/base/fttype1.c \
                     $(freetype_DIR)/src/base/ftwinfnt.c \
                     $(freetype_DIR)/src/base/ftxf86.c \
                     $(freetype_DIR)/src/truetype/truetype.c \
                     $(freetype_DIR)/src/type1/type1.c \
                     $(freetype_DIR)/src/cff/cff.c \
                     $(freetype_DIR)/src/cid/type1cid.c \
                     $(freetype_DIR)/src/pfr/pfr.c \
                     $(freetype_DIR)/src/type42/type42.c \
                     $(freetype_DIR)/src/winfonts/winfnt.c \
                     $(freetype_DIR)/src/pcf/pcf.c \
                     $(freetype_DIR)/src/bdf/bdf.c \
                     $(freetype_DIR)/src/sfnt/sfnt.c \
                     $(freetype_DIR)/src/autofit/autofit.c \
                     $(freetype_DIR)/src/pshinter/pshinter.c \
                     $(freetype_DIR)/src/raster/raster.c \
                     $(freetype_DIR)/src/smooth/smooth.c \
                     $(freetype_DIR)/src/cache/ftcache.c \
                     $(freetype_DIR)/src/gzip/ftgzip.c \
                     $(freetype_DIR)/src/lzw/ftlzw.c \
                     $(freetype_DIR)/src/psaux/psaux.c \
                     $(freetype_DIR)/src/psnames/psmodule.c
freetype_OBJECTS  := $(freetype_SOURCES:%.c=%.o)
freetype_CFLAGS   := $(filter-out -W%,$(CFLAGS)) \
                     -DFT_CONFIG_OPTION_SYSTEM_ZLIB \
                     -DFT_CONFIG_CONFIG_H='"$(shell cd $(freetype_DIR); pwd)/include/freetype/config/ftconfig.h"' \
                     -DFT2_BUILD_LIBRARY \
                     -DFT_CONFIG_MODULES_H='"$(shell cd $(freetype_DIR); pwd)/include/freetype/config/ftmodule.h"' \
                     -I$(freetype_DIR)/src \
                     -I$(freetype_DIR)/include \
                     -I$(freetype_DIR)/include/freetype/config \
                     -DHAVE_FCNTL_H

ifeq ($(USE_INSTALLED_FREETYPE),1)
-I_freetype = -I$(PSPROOT)/include/freetype2
-lfreetype = -lfreetype
else
-I_freetype = -I$(freetype_DIR)/include
-lfreetype =
PACKAGE_OBJECTS += $(freetype_OBJECTS)
endif

#-------------------------------------

libogg_DIR         = $(EXTLIB_DIR)/libogg-1.2.0
libogg_SOURCES     = $(libogg_DIR)/src/bitwise.c \
                     $(libogg_DIR)/src/framing.c
libogg_OBJECTS    := $(libogg_SOURCES:%.c=%.o)
libogg_CFLAGS     := $(filter-out -W%,$(CFLAGS)) -I$(libogg_DIR)/include

ifeq ($(USE_INSTALLED_LIBOGG),1)
-I_libogg =
-logg = -logg
else
-I_libogg = -I$(libogg_DIR)/include
-logg =
PACKAGE_OBJECTS += $(libogg_OBJECTS)
endif

#-------------------------------------

libvorbis_DIR      = $(EXTLIB_DIR)/libvorbis-1.3.1
libvorbis_SOURCES  = $(libvorbis_DIR)/lib/analysis.c \
                     $(libvorbis_DIR)/lib/bitrate.c \
                     $(libvorbis_DIR)/lib/block.c \
                     $(libvorbis_DIR)/lib/codebook.c \
                     $(libvorbis_DIR)/lib/envelope.c \
                     $(libvorbis_DIR)/lib/floor0.c \
                     $(libvorbis_DIR)/lib/floor1.c \
                     $(libvorbis_DIR)/lib/info.c \
                     $(libvorbis_DIR)/lib/lpc.c \
                     $(libvorbis_DIR)/lib/lsp.c \
                     $(libvorbis_DIR)/lib/mapping0.c \
                     $(libvorbis_DIR)/lib/mdct.c \
                     $(libvorbis_DIR)/lib/psy.c \
                     $(libvorbis_DIR)/lib/registry.c \
                     $(libvorbis_DIR)/lib/res0.c \
                     $(libvorbis_DIR)/lib/sharedbook.c \
                     $(libvorbis_DIR)/lib/smallft.c \
                     $(libvorbis_DIR)/lib/synthesis.c \
                     $(libvorbis_DIR)/lib/vorbisfile.c \
                     $(libvorbis_DIR)/lib/window.c
libvorbis_OBJECTS := $(libvorbis_SOURCES:%.c=%.o)
libvorbis_CFLAGS  := $(filter-out -W%,$(CFLAGS)) -I$(libvorbis_DIR)/include

ifeq ($(USE_INSTALLED_LIBVORBIS),1)
-I_libvorbis =
-lvorbis = -lvorbis
-lvorbisfile = -lvorbisfile
else
-I_libvorbis = -I$(libvorbis_DIR)/include
-lvorbis =
PACKAGE_OBJECTS += $(libvorbis_OBJECTS)
endif

#-------------------------------------

lua_DIR            = $(EXTLIB_DIR)/lua-5.1.4
lua_SOURCES        = $(lua_DIR)/src/lapi.c \
                     $(lua_DIR)/src/ldebug.c \
                     $(lua_DIR)/src/ldo.c \
                     $(lua_DIR)/src/ldump.c \
                     $(lua_DIR)/src/lfunc.c \
                     $(lua_DIR)/src/lgc.c \
                     $(lua_DIR)/src/lmem.c \
                     $(lua_DIR)/src/lobject.c \
                     $(lua_DIR)/src/lopcodes.c \
                     $(lua_DIR)/src/lstate.c \
                     $(lua_DIR)/src/lstring.c \
                     $(lua_DIR)/src/ltable.c \
                     $(lua_DIR)/src/ltm.c \
                     $(lua_DIR)/src/lundump.c \
                     $(lua_DIR)/src/lvm.c \
                     $(lua_DIR)/src/lzio.c \
                     $(lua_DIR)/src/lauxlib.c \
                     $(lua_DIR)/src/lbaselib.c \
                     $(lua_DIR)/src/lstrlib.c \
                     $(lua_DIR)/src/ltablib.c \
                     $(lua_DIR)/src/lparser.c \
                     $(lua_DIR)/src/llex.c \
                     $(lua_DIR)/src/lcode.c \
                     $(lua_DIR)/src/lmathlib.c
lua_OBJECTS       := $(lua_SOURCES:%.c=%.o)
lua_CFLAGS        := $(filter-out -W%,$(CFLAGS)) -I$(lua_DIR)/include

ifeq ($(USE_INSTALLED_LUA),1)
-I_lua =
-llua = -llua
else
-I_lua = -I$(lua_DIR)/src
-llua =
PACKAGE_OBJECTS += $(lua_OBJECTS)
endif

######################################

ALL_OBJECTS = $(BBGE_OBJECTS) $(Aquaria_OBJECTS) $(PSP_OBJECTS) \
              $(PACKAGE_OBJECTS)

REALLY_ALL_OBJECTS = $(BBGE_OBJECTS) $(Aquaria_OBJECTS) $(PSP_OBJECTS) \
                     $(freetype_OBJECTS) $(libogg_OBJECTS) \
                     $(libvorbis_OBJECTS) $(lua_OBJECTS)

# We move stdio.o to immediately before libc so that all stdio references
# are resolved before libc is loaded, thus preventing any of libc's stdio
# symbols from colliding with ours.  (Unfortunately, we can't do this with
# strtof.o because strtof() is defined in the same file as strtod() in
# Newlib, which means it's automatically pulled in by any strtod()
# references.)

OBJECTS_AND_LIBS = $(filter-out src/stdio.o,$(ALL_OBJECTS)) \
                   $(subst -lc,src/stdio.o -lc,$(LIBS))

###########################################################################

# Variables used for abbreviated command output.  By writing commands like:
#     $(ECHO) "    CMD target-file"
#     $(Q)command-1 target-file source-file
#     $(Q)command-2 target-file other-file
#     $(Q)...
# Make will output only "CMD target-file" normally, but will output each
# command executed if "V=1" is given on the Make command line.

ifneq ($(V),)
Q =
ECHO = @:
else
Q = @
ECHO = @echo
endif

###########################################################################

# Build rules.

.PHONY: all clean tools
all: EBOOT.PBP
clean:
	$(ECHO) "Removing executable files"
	$(Q)rm -f aquaria.elf aquaria.prx PARAM.SFO ICON0.PNG EBOOT.PBP
	$(ECHO) "Removing object files"
	$(Q)rm -f $(REALLY_ALL_OBJECTS)
	$(ECHO) "Removing temporary files"
	$(Q)rm -f ICON0-alpha.pnm
	$(MAKE) -C tools clean

spotless: clean
	$(ECHO) "Removing dependency files"
	$(Q)rm -f $(REALLY_ALL_OBJECTS:%.o=%.d)

tools:
	$(MAKE) -C tools

######################################

ifneq ($(ICON0_SOURCE_PNG),)

ICON0.PNG: $(ICON0_SOURCE_PNG) ICON0-alpha.pnm Makefile
	$(ECHO) "Generating $@"
	$(Q)pngtopnm <$< \
		| pamcut $(ICON0_LEFT) $(ICON0_TOP) $(ICON0_WIDTH) $(ICON0_HEIGHT) \
		| pamscale -width=144 -height=80 \
		| pnmtopng -alpha=ICON0-alpha.pnm -compression=9 \
		>$@

ICON0-alpha.pnm: $(ICON0_SOURCE_PNG) Makefile
	$(Q)pngtopnm -alpha <$< \
		| pamcut $(ICON0_LEFT) $(ICON0_TOP) $(ICON0_WIDTH) $(ICON0_HEIGHT) \
		| pamscale -width=144 -height=80 \
		>$@

else

ICON0.PNG:
	touch $@

endif

EBOOT.PBP: aquaria.prx PARAM.SFO ICON0.PNG Makefile
	$(ECHO) "Packing $@"
	$(Q)pack-pbp $@ PARAM.SFO ICON0.PNG NULL NULL NULL NULL aquaria.prx NULL
	$(ECHO) "Finished building Aquaria for PSP."

PARAM.SFO: Makefile
	$(ECHO) "Generating $@"
	$(Q)mksfoex -d MEMSIZE=1 "Aquaria" $@

aquaria.prx: aquaria.elf
	$(ECHO) "Creating $@"
	$(Q)psp-prxgen $< $@
	@# psp-prxgen doesn't return an error code on failure, so check for
	@# the file's existence.
	@test -f $@

aquaria.elf: $(ALL_OBJECTS)
	$(ECHO) "Linking $@ (this may take a while...)"
	$(Q)$(CC) $(LDFLAGS) $(OBJECTS_AND_LIBS) -o $@
	$(Q)psp-fixup-imports $@

######################################

$(BBGE_OBJECTS) : %.o: %.cpp Makefile
	$(ECHO) "Compiling $<"
	$(Q)$(CXX) $(BBGE_CXXFLAGS) -c $< -o $@

$(Aquaria_OBJECTS) : %.o: %.cpp Makefile
	$(ECHO) "Compiling $<"
	$(Q)$(CXX) $(Aquaria_CXXFLAGS) -c $< -o $@

$(PSP_OBJECTS) : %.o: %.c Makefile
	$(ECHO) "Compiling $<"
	$(Q)$(CC) $(PSP_CFLAGS) -c $< -o $@
# src/sysdep-psp/exports.c can be regenerated from exports.exp if necessary.
src/sysdep-psp/exports.c: src/sysdep-psp/exports.exp
	$(ECHO) "Regenerating $@"
	$(Q)psp-build-exports -b $< >$@

$(freetype_OBJECTS) : %.o: %.c Makefile
	$(ECHO) "Compiling $<"
	$(Q)$(CC) $(freetype_CFLAGS) -c $< -o $@

$(libogg_OBJECTS) : %.o: %.c Makefile
	$(ECHO) "Compiling $<"
	$(Q)$(CC) $(libogg_CFLAGS) -c $< -o $@

$(libvorbis_OBJECTS) : %.o: %.c Makefile
	$(ECHO) "Compiling $<"
	$(Q)$(CC) $(libvorbis_CFLAGS) -c $< -o $@

$(lua_OBJECTS) : %.o: %.c Makefile
	$(ECHO) "Compiling $<"
	$(Q)$(CC) $(lua_CFLAGS) -c $< -o $@

###########################################################################

# Dependency generation rules.

$(BBGE_OBJECTS:%.o=%.d) : %.d: %.cpp Makefile
	$(ECHO) "Generating dependencies for $<"
	$(Q)$(CXX) $(BBGE_CXXFLAGS) -MM $< -MT $(@:%.d=%.o) -o $@.tmp
	$(Q)sed -e 's:$(@:%.d=%.o):$(@:%.d=%.o) $@:' <$@.tmp >$@
	$(Q)rm -f $@.tmp

$(Aquaria_OBJECTS:%.o=%.d) : %.d: %.cpp Makefile
	$(ECHO) "Generating dependencies for $<"
	$(Q)$(CXX) $(Aquaria_CXXFLAGS) -MM $< -MT $(@:%.d=%.o) -o $@.tmp
	$(Q)sed -e 's:$(@:%.d=%.o):$(@:%.d=%.o) $@:' <$@.tmp >$@
	$(Q)rm -f $@.tmp

$(PSP_OBJECTS:%.o=%.d) : %.d: %.c Makefile
	$(ECHO) "Generating dependencies for $<"
	$(Q)$(CC) $(PSP_CFLAGS) -MM $< -MT $(@:%.d=%.o) -o $@.tmp
	$(Q)sed -e 's:$(@:%.d=%.o):$(@:%.d=%.o) $@:' <$@.tmp >$@
	$(Q)rm -f $@.tmp

$(freetype_OBJECTS:%.o=%.d) : %.d: %.c Makefile
	$(ECHO) "Generating dependencies for $<"
	$(Q)$(CC) $(freetype_CFLAGS) -MM $< -MT $(@:%.d=%.o) -o $@.tmp
	$(Q)sed -e 's|$(@:%.d=%.o) *:|$(@:%.d=%.o) $@:|' <$@.tmp >$@
	$(Q)rm -f $@.tmp

$(libogg_OBJECTS:%.o=%.d) : %.d: %.c Makefile
	$(ECHO) "Generating dependencies for $<"
	$(Q)$(CC) $(libogg_CFLAGS) -MM $< -MT $(@:%.d=%.o) -o $@.tmp
	$(Q)sed -e 's|$(@:%.d=%.o) *:|$(@:%.d=%.o) $@:|' <$@.tmp >$@
	$(Q)rm -f $@.tmp

$(libvorbis_OBJECTS:%.o=%.d) : %.d: %.c Makefile
	$(ECHO) "Generating dependencies for $<"
	$(Q)$(CC) $(libvorbis_CFLAGS) -MM $< -MT $(@:%.d=%.o) -o $@.tmp
	$(Q)sed -e 's|$(@:%.d=%.o) *:|$(@:%.d=%.o) $@:|' <$@.tmp >$@
	$(Q)rm -f $@.tmp

$(lua_OBJECTS:%.o=%.d) : %.d: %.c Makefile
	$(ECHO) "Generating dependencies for $<"
	$(Q)$(CC) $(lua_CFLAGS) -MM $< -MT $(@:%.d=%.o) -o $@.tmp
	$(Q)sed -e 's|$(@:%.d=%.o) *:|$(@:%.d=%.o) $@:|' <$@.tmp >$@
	$(Q)rm -f $@.tmp

include $(ALL_OBJECTS:%.o=%.d)

###########################################################################
