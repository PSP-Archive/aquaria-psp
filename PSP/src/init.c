/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/init.c: Program initialization and exit code.
 */

#include "common.h"
#include "debugfont.h"
#include "init.h"
#include "memory.h"
#include "resource.h"
#include "sysdep.h"
#include "test.h"
#include "timer.h"

/*************************************************************************/
/************************** Interface functions **************************/
/*************************************************************************/

/**
 * init_all:  Initialize the engine.
 *
 * [Parameters]
 *     argc: Command line argument count
 *     argv: Command line argument vector
 * [Return value]
 *     None
 */
void init_all(int argc, char **argv)
{
    sys_init(argv[0]);
#ifndef CXX_CONSTRUCTOR_HACK
    if (!mem_init()) {
        sys_exit(1);
    }
#endif
    resource_init();
    timer_init();
#ifdef DEBUG
    debugfont_init();
#endif

#ifdef INCLUDE_TESTS
    if (argc > 1 && strcmp(argv[1], "-test") == 0) {
        sys_exit(run_all_tests() ? 0 : 1);
    }
#endif
}

/*************************************************************************/

/**
 * exit_all:  Shut down the engine and terminate the program.
 *
 * [Parameters]
 *     exit_code: Program exit code (0 = successful exit)
 * [Return value]
 *     Does not return
 */
void exit_all(int exit_code)
{
    sys_exit(exit_code);
}

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
