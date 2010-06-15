/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/lalloc.h: Header for optimized memory allocator for Lua.
 */

#ifndef LALLOC_H
#define LALLOC_H

/*************************************************************************/

/**
 * lalloc:  Lua memory allocation function, called for all allocation,
 * resize, and free requests.
 *
 * [Parameters]
 *        ud: Opaque user data pointer (not used)
 *       ptr: Pointer to block to resize or free; NULL for a new allocation
 *     osize: Current size of block (not used)
 *     nsize: Requested new size of block; 0 to free the block
 * [Return value]
 *     Pointer to allocated memory region; NULL on failure or block free
 */
extern void *lalloc(void *ud, void *ptr, size_t osize, size_t nsize);

/*************************************************************************/

#endif  // LALLOC_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
