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

extern void *debug_malloc(size_t size, const char *file, int line);
extern void *debug_calloc(size_t nmemb, size_t size, const char *file, int line);
extern void *debug_realloc(void *ptr, size_t size, const char *file, int line);
extern void debug_free(void *ptr, const char *file, int line);

/* Redefine malloc(), etc. to point to the debug functions. */
# define malloc(size)        debug_malloc(size, __FILE__, __LINE__)
# define calloc(nmemb,size)  debug_calloc(nmemb, size, __FILE__, __LINE__)
# define realloc(ptr,size)   debug_realloc(ptr, size, __FILE__, __LINE__)
# define free(ptr)           debug_free(ptr, __FILE__, __LINE__)

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

#endif  // DEBUG

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
