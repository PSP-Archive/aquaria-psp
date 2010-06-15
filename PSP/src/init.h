/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/init.h: Header for program initialization and exit code.
 */

#ifndef INIT_H
#define INIT_H

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
extern void init_all(int argc, char **argv);

/**
 * exit_all:  Shut down the engine and terminate the program.
 *
 * [Parameters]
 *     exit_code: Program exit code (0 = successful exit)
 * [Return value]
 *     Does not return
 */
extern NORETURN void exit_all(int exit_code);

/*************************************************************************/

#endif  // INIT_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
