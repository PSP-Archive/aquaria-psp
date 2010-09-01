/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/malloc-opnew.h: Debugging versions of the C++ "new" and "delete"
 * operators that pass source file and line number information to malloc()
 * and free().
 */

#ifndef MALLOC_OPNEW_H
#define MALLOC_OPNEW_H

#ifdef DEBUG

/*************************************************************************/

/* First include some system headers that use weird new() syntax, at least
 * in GCC. */
#include <algorithm>
#include <string>
#include <vector>

/* Define the debugging versions of the operators. */
inline void *operator new(size_t size, const char *file, int line) {
    return debug_malloc(size, file, line);
}
inline void *operator new[](size_t size, const char *file, int line) {
    return debug_malloc(size, file, line);
}
inline void operator delete(void *ptr, const char *file, int line) {
    return debug_free(ptr, file, line);
}
inline void operator delete[](void *ptr, const char *file, int line) {
    return debug_free(ptr, file, line);
}

/* Define a macro that converts ordinary "new" into a call to the debugging
 * version.  For some reason, this does _not_ work with "delete". */
#define new new(__FILE__,__LINE__)
//#define delete delete(__FILE__,__LINE__)

/*************************************************************************/

#endif  // DEBUG
#endif  // MALLOC_OPNEW_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
