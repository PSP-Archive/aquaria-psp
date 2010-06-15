/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/malloc.h: Substitutes for malloc() and related functions that use
 * our custom memory allocator.
 */

#ifndef MALLOC_H
#define MALLOC_H

/*************************************************************************/

extern void *malloc(size_t size);
extern void *calloc(size_t nmemb, size_t size);
extern void *realloc(void *ptr, size_t size);
extern void free(void *ptr);

#ifdef DEBUG
/**
 * malloc_display_debuginfo:  Display debug information about malloc()
 * heaps.  Only implemented when DEBUG is defined.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
extern void malloc_display_debuginfo(void);
#endif

/*************************************************************************/

#endif  // MALLOC_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
