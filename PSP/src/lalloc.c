/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/lalloc.c: Optimized memory allocator for Lua.
 */

/*
 * The vast majority of memory allocations made by the Lua interpreter are
 * tiny, on the order of 16-32 bytes.  Since even a low-overhead general
 * allocator requires 8-15 bytes of overhead per allocation, we suffer
 * significant losses to overhead; for example, a 20-byte TKey (assuming
 * the use of "float" instead of "double" for Lua numeric values) requires
 * a 32-byte malloc() block, for 60% overhead.
 *
 * Our solution to this is a custom allocator, lalloc(), tailored
 * specifically to Lua's allocation patterns.  On receiving a new
 * allocation request for a sufficiently small size, lalloc() takes the
 * next available slot from an array of units of that specific allocation
 * size (rounded up to the next multiple of 4 to avoid alignment errors).
 * Within a single array, there are no free lists or other per-block
 * management information; instead, a simple free bitmap is used to track
 * which slots are available for allocation.  Since the size of every
 * allocation in a given array is constant, there is no need to record
 * block sizes or other management information, and thus the overhead of
 * word-aligned allocations is only 1 bit (plus the block's fraction of
 * the overall array overhead, typically 1 bit or less).  Furthermore,
 * free operations run in constant time, and while allocation time is
 * linear in the size of a single array, the linear portion only depends
 * on the number of words in the bitmap and can typically be executed
 * quickly.
 *
 * Allocations larger than the maximum supported here, reallocations of
 * existing blocks, and frees that do not fall within an lalloc() array
 * are passed back to malloc(), realloc(), or free() respectively.
 *
 * Note:  This implementation does some magic with pointer addresses to
 * let free() run in constant time, which may not work well on systems
 * with large amounts of memory.
 */

/*************************************************************************/
/************************ Configuration settings *************************/
/*************************************************************************/

/**
 * LALLOC_MIN_SIZE:  Minimum size block to support.  Allocations smaller
 * than this will be rounded up to this size, so it should be matched to
 * the smallest actual allocation.  Must be a multiple of 4.
 */
#define LALLOC_MIN_SIZE  16

/**
 * LALLOC_MAX_SIZE:  Maximum size block to support.  Allocations larger
 * than this will be redirected to malloc().  Must be a multiple of 4.
 */
#define LALLOC_MAX_SIZE  48

/**
 * LALLOC_ARRAY_SIZE:  Size of a single array allocated for a single block
 * size, in bytes.  This will be rounded up to the next multiple of 32x the
 * particular block size; for example, the default of 16384 would result in
 * an array of 16640 bytes (832 slots) being allocated for block size 20.
 * This value must be no greater than 65504 * LALLOC_MIN_SIZE (since slot
 * indices are stored as 16-bit values).
 *
 * This size is also used in indexing array_map[] (see below), and larger
 * values of this constant will result in less memory used by array_map[].
 * It is also recommended to set this constant to a power of 2 for faster
 * indexing into array_map[], as otherwise every such index operation will
 * require a divide instruction.
 */
#define LALLOC_ARRAY_SIZE  16384

/**
 * LALLOC_MIN_ADDRESS, LALLOC_MAX_ADDRESS:  Minimum and maximum addresses
 * that can be contained in a block returned from malloc().  These are
 * used to generate a lookup table that allows do_free() to determine in
 * constant time the array to which a given pointer belongs.
 */
#define LALLOC_MIN_ADDRESS  ((uintptr_t)0x8800000)
#define LALLOC_MAX_ADDRESS  ((uintptr_t)0xBBFFFFF)

/*-------- Debugging options --------*/

/**
 * TRACE_ALLOCS:  Define this to log every allocation and free with DMSG().
 * Only functions when DEBUG is defined.
 */
// #define TRACE_ALLOCS

/*************************************************************************/
/*************** Internal data structures and declarations ***************/
/*************************************************************************/

/* Include common headers. */

#include "common.h"
#include "lalloc.h"
#include "malloc.h"

#include <stdlib.h>  // For malloc(), realloc(), free()

/*************************************************************************/

/**
 * LallocArray:  Management structure for a single array of fixed-size
 * slots.  These arrays are kept in linked lists, one list per block size,
 * sorted by block address.
 */
typedef struct LallocArray_ LallocArray;
struct LallocArray_ {
    LallocArray *next, *prev;   // Array list pointers
    uint8_t *slot_base;         // Pointer to first slot in array
    uint8_t *slot_top;          // Pointer to last slot + 1 in array
    uint16_t slot_size;         // Size of each slot, in bytes
    uint16_t total_slots;       // Total number of slots in array
    uint16_t slots_free;        // Number of free slots in array
    uint16_t first_free;        // Index of first free slot (undefined if none)
    uint32_t free_bitmap[0];    // Free bitmap (immediately follows this
                                //    structure in memory)
};

/**
 * SIZE_INDEX:  Macro used to compute the index for accessing array_lists[]
 * and first_free[] from the allocation block size.
 */
#define SIZE_INDEX(size)  (((size) - LALLOC_MIN_SIZE) / 4)

/**
 * ADDRESS_INDEX:  Macro used to compute the index for accessing
 * array_map[] from the address.  "ptr" can be either a pointer or an
 * integer address.
 */
#define ADDRESS_INDEX(ptr)  (((uintptr_t)(ptr) - LALLOC_MIN_ADDRESS) \
                             / LALLOC_ARRAY_SIZE)

/**
 * array_list:  Array of lists of slot arrays for each block size, indexed
 * by (block_size - LALLOC_MIN_SIZE) / 4; an entry is NULL if no arrays
 * have yet been allocated for that block size  Use SIZE_INDEX(block_size)
 * for convenient computation of the array_list[] index.
 */
static LallocArray *array_list[SIZE_INDEX(LALLOC_MAX_SIZE) + 1];

/**
 * first_free:  Array indicating the first (in address order) allocated
 * slot array with a free slot for each block size; an entry is NULL if
 * all arrays for that block size are full, or if no arrays have yet been
 * allocated for that size.
 */
static LallocArray *first_free[SIZE_INDEX(LALLOC_MAX_SIZE) + 1];

/**
 * array_map:  Lookup table indicating the array or arrays to which an
 * address may belong.  The array is indexed by multiples of
 * LALLOC_ARRAY_SIZE starting from LALLOC_MIN_ADDRESS, which ensures that
 * there can never be more than two arrays associated with each element
 * (one ending in that address range and another starting in the same
 * range).
 *
 * If there are no arrays covered by the address range associated with a
 * particular entry, both pointers for that entry will be NULL.  If there
 * is only one such array, it may be in either the first or the second
 * pointer.
 *
 * Use ADDRESS_INDEX(ptr) for convenient computation of the array_map[]
 * index.
 */
static LallocArray *array_map[ADDRESS_INDEX(LALLOC_MAX_ADDRESS) + 1][2];

/*-----------------------------------------------------------------------*/

/* Inline function for "count trailing zeros".  GCC implements this as a
 * compiler intrinsic. */

#ifdef __GNUC__

# define CTZ(value)  __builtin_ctz((value))

#else  // !__GNUC__, so do it the hard way

static inline unsigned int CTZ(uint32_t value) {
    unsigned int bits = 0;
    uint32_t mask = 1;
    while (!(value & mask) && mask != 0) {
        mask <<= 1;
        bits++;
    }
    return bits;
}

#endif

/*-----------------------------------------------------------------------*/

/* Helper function declarations. */

static void *do_alloc(unsigned int block_size
#ifdef TRACE_ALLOCS
                      , LallocArray **array_ret
#endif
                     );
static void do_free(LallocArray *array, void *ptr);

static LallocArray *create_array(unsigned int block_size);
static void delete_array(LallocArray *array);
static LallocArray *find_containing_array(void *ptr);

/*************************************************************************/
/************** Interface function (pass to lua_newstate()) **************/
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
void *lalloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    if (nsize == 0) {

        if (ptr != NULL) {
            LallocArray *array;
            if ((array = find_containing_array(ptr)) != NULL) {
#ifdef TRACE_ALLOCS
                DMSG("free(%p) array %p", ptr, array);
#endif
                do_free(array, ptr);
            } else {
                free(ptr);
            }
        }
        return NULL;

    } else {  // nsize != 0

        if (ptr != NULL) {
            /* This is a resize, which we don't handle.  Typically, blocks
             * that get resized by Lua will have been larger to start with
             * than this allocator processes, but check anyway just in case. */

            LallocArray *array;
            if (UNLIKELY((array = find_containing_array(ptr)) != NULL)) {
                /* If it's a shrink operation, just return the current
                 * block.  This avoids the case where we fail to allocate
                 * a new block and would return NULL, which Lua assumes
                 * cannot happen on a shrink.  Of course, if we fail to
                 * allocate a block at the sizes handled by this allocator,
                 * we probably have more serious problems to deal with... */
                if (nsize <= osize) {
                    return ptr;
                }
                void *newptr = malloc(nsize);
                if (!newptr) {
#ifdef TRACE_ALLOCS
                    DMSG("realloc(%p,%u) -> FAILED to get a block!",
                         ptr, nsize);
#endif
                    return NULL;
                }
                memcpy(newptr, ptr, osize);  // osize < nsize
#ifdef TRACE_ALLOCS
                DMSG("realloc(%p,%u) -> free(%p) array %p",
                     ptr, nsize, ptr, array);
#endif
                do_free(array, ptr);
                return newptr;
            }  // if (UNLIKELY(find_containing_array(ptr)))

            return realloc(ptr, nsize);

        } else {  // ptr == NULL

            if (nsize <= LALLOC_MAX_SIZE) {
                unsigned int aligned_size = align_up(nsize, 4);
                if (UNLIKELY(aligned_size < LALLOC_MIN_SIZE)) {
#ifdef TRACE_ALLOCS
                    DMSG("WARNING: nsize (%u) < LALLOC_MIN_SIZE (%u),"
                         " some bytes will be wasted!", nsize,
                         LALLOC_MIN_SIZE);
#endif
                    aligned_size = LALLOC_MIN_SIZE;
                }
#ifdef TRACE_ALLOCS
                LallocArray *array = NULL;
                void *ptr = do_alloc(aligned_size, &array);
#else
                void *ptr = do_alloc(aligned_size);
#endif
                if (ptr) {
#ifdef TRACE_ALLOCS
                    DMSG("malloc(%u) -> %p (block size %u, array %p)",
                         nsize, ptr, aligned_size, array);
#endif
                    return ptr;
                } else {
#ifdef TRACE_ALLOCS
                    DMSG("malloc(%u) -> FAILED to get a block!", nsize);
#endif
                    return malloc(nsize);
                }
            } else {  // nsize > LALLOC_MAX_SIZE
#ifdef TRACE_ALLOCS
                DMSG("malloc(%u) -> too big, passing to system", nsize);
#endif
                return malloc(nsize);
            }

        }  // if (ptr != NULL)
    }  // if (nsize == 0)
}

/*************************************************************************/
/*************************** Helper functions ****************************/
/*************************************************************************/

/**
 * do_alloc:  Allocate a single block of the given block size, creating a
 * new slot array if necessary.
 *
 * [Parameters]
 *     block_size: Block size to allocate (must be a multiple of 4)
 *      array_ret: Pointer to variable to receive array containing
 *                    allocated block (only when TRACE_ALLOCS is defined)
 * [Return value]
 *     Allocated block, or NULL on failure
 */
static void *do_alloc(unsigned int block_size
#ifdef TRACE_ALLOCS
                      , LallocArray **array_ret
#endif
                     )
{
    /* This precondition should always be satisfied anyway, but we
     * intentionally don't provide a soft failure mode for non-debug
     * builds in order to improve performance. */
    PRECOND(block_size % 4 == 0);

    /* Find (or create) the array to allocate from. */

    const unsigned int index = SIZE_INDEX(block_size);
    LallocArray *array;
    if (!(array = first_free[index])) {
        if (!(array = create_array(block_size))) {
            return NULL;
        }
    }

    /* Mark the next free slot as used. */

    const unsigned int slot_num = array->first_free;
    array->free_bitmap[slot_num/32] &= ~(1 << (slot_num%32));
    array->slots_free--;

    /* Look for the next free slot.  If there are no more free slots,
     * update the first_free[] array with the next non-full slot array. */

    if (array->slots_free > 0) {
        unsigned int bitmap_word = (slot_num+1)/32;
        while (array->free_bitmap[bitmap_word] == 0) {
            bitmap_word++;
        }
        array->first_free =
            bitmap_word*32 + CTZ(array->free_bitmap[bitmap_word]);
    } else {  // array->slots_free == 0
        /* Ensure array->first_free gets updated at the next do_free(). */
        array->first_free = array->total_slots;
        LallocArray *next_array = array;
        do {
            next_array = next_array->next;
        } while (next_array != NULL && next_array->slots_free == 0);
        first_free[index] = next_array;
    }

    /* Generate and return the pointer for this slot. */

#ifdef TRACE_ALLOCS
    *array_ret = array;
#endif
    return (void *)(array->slot_base + (slot_num * block_size));
}

/*-----------------------------------------------------------------------*/

/**
 * do_free:  Free the given block from its slot array, also freeing the
 * slot array if it becomes empty.
 *
 * [Parameters]
 *     array: Slot array containing block to free
 *       ptr: Block to free (must be non-NULL)
 * [Return value]
 *     None
 */
static void do_free(LallocArray *array, void *ptr)
{
    /* As with do_alloc(), we intentionally don't provide a soft failure
     * mode here. */
    PRECOND(array != NULL);
    PRECOND(ptr != NULL);

    const uintptr_t address_diff =
        (uintptr_t)ptr - (uintptr_t)array->slot_base;
    const unsigned int slot_num =
        (unsigned int)address_diff / array->slot_size;
    array->free_bitmap[slot_num/32] |= 1 << (slot_num%32);
    array->slots_free++;
    if (slot_num < array->first_free) {
        array->first_free = slot_num;
    }

    if (UNLIKELY(array->slots_free >= array->total_slots)) {
        delete_array(array);
    } else if (UNLIKELY(array->slots_free == 1)) {
        /* If slots_free was already nonzero, then either this array or a
         * previous one was already in first_free[], so we don't need to
         * do the update check. */
        const unsigned int index = SIZE_INDEX(array->slot_size);
        if (array < first_free[index]) {
            first_free[index] = array;
        }
    }
}

/*************************************************************************/

/**
 * create_array:  Create a new slot array for the given block size and
 * link it into the array list, also updating first_free[] and array_map[]
 * appropriately.
 *
 * [Parameters]
 *     block_size: Allocation block size for array (bytes)
 * [Return value]
 *     Newly-created array, or NULL on failure
 */
static LallocArray *create_array(unsigned int block_size)
{
    PRECOND_SOFT(block_size % 4 == 0, return 0);
    PRECOND_SOFT(block_size >= LALLOC_MIN_SIZE && block_size <= LALLOC_MAX_SIZE,
                 return NULL);

    /* Determine the number of slots and total array data size, and
     * allocate a memory block for the array from the system. */

    const unsigned int num_slots =
        align_down((LALLOC_ARRAY_SIZE + 32*block_size - 1) / block_size, 32);
    const unsigned int bitmap_size = num_slots / 8;
    const unsigned int array_size =
        sizeof(LallocArray) + bitmap_size + (num_slots * block_size);

    LallocArray *array = malloc(array_size);
    if (!array) {
#ifdef TRACE_ALLOCS
        DMSG("Failed to allocate new array for block size %u (%u slots,"
             " array size %u)", block_size, num_slots, array_size);
#endif
        return NULL;
    }
#ifdef TRACE_ALLOCS
    DMSG("Allocated new array at %p for block size %u (%u slots,"
         " array size %u)", array, block_size, num_slots, array_size);
#endif

    /* Set up the array management data. */

    array->slot_base   = (uint8_t *)array + sizeof(LallocArray) + bitmap_size;
    array->slot_top    = array->slot_base + (num_slots * block_size);
    array->slot_size   = block_size;
    array->total_slots = num_slots;
    array->slots_free  = num_slots;
    array->first_free  = 0;
    mem_fill32(array->free_bitmap, ~(uint32_t)0, bitmap_size);

    /* Insert the array into the global list, keeping the list in order
     * by address. */

    const unsigned int index = SIZE_INDEX(block_size);
    LallocArray *prev, *next;
    for (prev = NULL, next = array_list[index];
         next != NULL && next < array;
         prev = next, next = next->next
    ) {
        /* nothing */
    }
    array->next = next;
    array->prev = prev;
    if (next) {
        next->prev = array;
    }
    if (prev) {
        prev->next = array;
    } else {
        array_list[index] = array;
    }

    /* If this array is earlier than the current first-free array for this
     * block size, update the first_free[] entry. */

    if (!first_free[index] || first_free[index] > array) {
        first_free[index] = array;
    }

    /* Store the array in the address lookup table.  We use a for() loop
     * because the array could potentially touch up to three regions,
     * depending on how it's aligned. */

    const unsigned int first_region = ADDRESS_INDEX(array);
    const unsigned int last_region = ADDRESS_INDEX((uintptr_t)array
                                                   + array_size - 1);
    unsigned int region;
    for (region = first_region; region <= last_region; region++) {
        if (!array_map[region][0]) {
            array_map[region][0] = array;
        } else {
            if (array_map[region][1]) {
                DMSG("BUG: Region %u (0x%X-0x%X) already has two arrays:"
                     " %p, %p!  Overwriting %p with %p.", region,
                     LALLOC_MIN_ADDRESS + region*LALLOC_ARRAY_SIZE,
                     LALLOC_MIN_ADDRESS + (region+1)*LALLOC_ARRAY_SIZE - 1,
                     array_map[region][0], array_map[region][1],
                     array_map[region][1], array);
            }
            array_map[region][1] = array;
        }
    }

    /* Success!  Return the new array. */

    return array;
}

/*-----------------------------------------------------------------------*/

/**
 * delete_array:  Delete the given slot array, unlinking it from the array
 * list and removing all other references.
 *
 * [Parameters]
 *     array: Slot array to delete
 * [Return value]
 *     None
 */
static void delete_array(LallocArray *array)
{
    PRECOND_SOFT(array != NULL, return);

    const unsigned int index = SIZE_INDEX(array->slot_size);

    /* Remove this array from array_map[]. */

    const unsigned int first_region = ADDRESS_INDEX(array);
    const unsigned int last_region =
        ADDRESS_INDEX((uintptr_t)(array->slot_top) - 1);
    unsigned int region;
    for (region = first_region; region <= last_region; region++) {
        if (array_map[region][0] == array) {
            array_map[region][0] = NULL;
        } else {
            if (array_map[region][1] != array) {
                DMSG("BUG: Array %p isn't recorded in region %u (0x%X-0x%X,"
                     " currently has %p and %p)!  Angrily stomping on %p.",
                     array, region,
                     LALLOC_MIN_ADDRESS + region*LALLOC_ARRAY_SIZE,
                     LALLOC_MIN_ADDRESS + (region+1)*LALLOC_ARRAY_SIZE - 1,
                     array_map[region][0], array_map[region][1],
                     array_map[region][1]);
            }
            array_map[region][1] = NULL;
        }
    }

    /* If this array is listed in first_free[], replace it with the next
     * array for this size containing free slots. */

    if (first_free[index] == array) {
        do {
            first_free[index] = first_free[index]->next;
        } while (first_free[index] != NULL && first_free[index]->slots_free == 0);
    }

    /* Unlink the array from its list. */

    if (array->next) {
        array->next->prev = array->prev;
    }
    if (array->prev) {
        array->prev->next = array->next;
    } else {
        array_list[index] = array;
    }

    /* Free the array memory and return. */

    free(array);
#ifdef TRACE_ALLOCS
    DMSG("Deleted array at %p", array);
#endif
}

/*-----------------------------------------------------------------------*/

/**
 * find_containing_array:  Determine whether or not the given pointer is
 * one allocated by lalloc(), and if so, return the slot array in which it
 * is contained.
 *
 * [Parameters]
 *     ptr: Pointer to check
 * [Return value]
 *     Slot array in which the pointer is contained, or NULL if the
 *     pointer is not one allocated by lalloc()
 */
static LallocArray *find_containing_array(void *ptr)
{
    PRECOND_SOFT((uintptr_t)ptr >= LALLOC_MIN_ADDRESS && (uintptr_t)ptr <= LALLOC_MAX_ADDRESS,
                 return NULL);

    const uint32_t region = ADDRESS_INDEX(ptr);
    if (array_map[region][0]
        && (uintptr_t)ptr >= (uintptr_t)(array_map[region][0]->slot_base)
        && (uintptr_t)ptr <  (uintptr_t)(array_map[region][0]->slot_top)
    ) {
        return array_map[region][0];
    }
    if (array_map[region][1]
        && (uintptr_t)ptr >= (uintptr_t)(array_map[region][1]->slot_base)
        && (uintptr_t)ptr <  (uintptr_t)(array_map[region][1]->slot_top)
    ) {
        return array_map[region][1];
    }
    return NULL;
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
