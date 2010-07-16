/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/PSPInterface.h: Interface header for exporting to Aquaria code.
 */

#ifndef PSPINTERFACE_H
#define PSPINTERFACE_H

/*************************************************************************/

/**** Leading C++ junk. ****/

#ifdef __cplusplus
#define this _this
#define private _private
extern "C" {
#endif

/*-----------------------------------------------------------------------*/

/**** Include all the PSP-specific headers. ****/

#define SYSDEP_COMMON_H "sysdep-psp/common.h"

/* Aquaria/BBGE make use of various stdio functions, so we can't override
 * [v]snprintf(); include <stdio.h> here instead. */
#include <stdio.h>

/* GCC #include <cstdlib> needs various things from stdlib.h defined. */
#include <stdlib.h>

/* Make memset() available to Aquaria/BBGE code. */
#include <string.h>

/* Avoid type collisions with Aquaria/BBGE code. */
#define Texture PSPTexture

#include "common.h"
#include "fakegl.h"
#include "graphics.h"
#include "init.h"
#include "input.h"
#include "lalloc.h"
#include "malloc.h"
#include "memory.h"
#include "resource.h"
#include "savefile.h"
#include "sound.h"
#include "strtof.h"
#include "sysdep.h"
#include "texture.h"
#include "timer.h"

#undef Texture

/*-----------------------------------------------------------------------*/

/**** Redefine various library functions to our versions. ****/

/* We don't have exit(), so substitute sys_exit() instead. */
#undef exit
#define exit(code)  sys_exit((code))

/* Replace atof() with strtof() to avoid double->float conversions. */
#undef atof
#define atof(s)  strtof((s), NULL)

/* We need to override open/read/closedir() to look at package files. */
#include <dirent.h>  // We use struct dirent as is.
typedef struct psp_DIR psp_DIR;
psp_DIR *psp_opendir(const char *path);
struct dirent *psp_readdir(psp_DIR *dir);
int psp_closedir(psp_DIR *dir);
#define DIR      psp_DIR
#define opendir  psp_opendir
#define readdir  psp_readdir
#define closedir psp_closedir

/*-----------------------------------------------------------------------*/

/**** Trailing C++ junk. ****/

#ifdef __cplusplus

# undef this
# undef private
# undef min  // Conflicts with C++ STL
# undef max  // Conflicts with C++ STL
};  // extern "C"

# include "malloc-opnew.h"  // Debug implementations of operator new/delete

#endif  // __cplusplus

/*************************************************************************/

#endif  // PSPINTERFACE_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
