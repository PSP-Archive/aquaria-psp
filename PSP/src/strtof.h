/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/strtof.h: Header for strtof.h that redefines strtof to psp_strtof
 * if needed to avoid link errors.
 */

#ifndef STRTOF_H
#define STRTOF_H

/*************************************************************************/

#ifndef CAN_OVERRIDE_STRTOF

/* We have to rename this to our own version (meaning, of course, that
 * libraries like Lua won't be able to reap its benefit). */
extern float psp_strtof(const char *s, char **endptr);
# undef strtof
# define strtof psp_strtof

#endif

/*************************************************************************/

#endif  // STRTOF_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
