/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/texture.c: Texture management routines.
 */

/*
 * Textures in Aquaria are normally loaded from PNG files.  On the PSP,
 * however, the decompression required by PNG files is fairly expensive,
 * and furthermore the original textures--designed for an 800x600 screen--
 * are unwieldy on the PSP's 480x272 pixel display.  Instead of the PNGs,
 * we use a custom format which can be loaded asynchronously into memory.
 * The data in these files is scaled down to half width and height from the
 * original PNGs, and is swizzled into 16-byte-by-8-line blocks to greatly
 * improve drawing speed over standard PNG images.  Many textures are also
 * quantized to 8-bit indexed mode for an additional speed boost as well as
 * memory savings.
 *
 * When loading textures from data files, the program first checks for the
 * presence of a file with a ".tex" extension appended (replacing any
 * ".png" extension in the requested filename).  If found, that file will
 * be loaded instead of the PNG file.
 *
 * For each texture we load from a PNG file, we manually perform the
 * size reduction and swizzling.  This allows Aquaria to run (albeit at
 * significantly reduced performance) from the original data files or with
 * PNG data used in mods.
 */

#include "common.h"
#include "memory.h"
#include "resource.h"
#include "sysdep.h"
#include "texture.h"

#define PNG_USER_MEM_SUPPORTED
#include <libpng14/png.h>

#ifdef DEBUG
# undef texture_new
# undef texture_new_indexed
# undef texture_load
# undef texture_parse
# undef texture_destroy
#endif

/*************************************************************************/
/****************************** Local data *******************************/
/*************************************************************************/

/* jmp_buf used for libpng error handling.  (Sadly, libpng is too lazy to
 * implement transparent error handling, so we need to give it a helping
 * hand.) */
static jmp_buf png_jmpbuf;

/* Buffer/position structure used by png_read_handler(). */
static struct {
    const uint8_t *data;  // Data buffer
    uint32_t len;         // Length of data
    uint32_t cur_offset;  // Current read offset
} png_readinfo;

/*************************************************************************/

/* Local function declarations. */

static Texture *texture_parse_tex(uint8_t *data, uint32_t len,
                                  uint32_t mem_flags, int reuse
#ifdef DEBUG
                                  , const char *srcfile, int srcline
#endif
);
static Texture *texture_parse_png(uint8_t *data, uint32_t len,
                                  uint32_t mem_flags, int reuse
#ifdef DEBUG
                                  , const char *srcfile, int srcline
#endif
);
static void png_warning_callback(png_structp png, const char *message);
static void png_error_callback(png_structp png, const char *message);
static void png_read_callback(png_structp png, uint8_t *buf, size_t size);
static void *png_malloc_callback(png_structp png, size_t size);
static void png_free_callback(png_structp png, void *ptr);

/*************************************************************************/
/************************** Interface functions **************************/
/*************************************************************************/

/**
 * texture_new:  Create a new, empty 32-bit-color texture.  The width and
 * height may be any positive values, but the texture may not be suitable
 * for drawing if the width and height are not both powers of two.
 *
 * [Parameters]
 *         width: Texture width (pixels)
 *        height: Texture height (pixels)
 *     mem_flags: Memory allocation flags (MEM_ALLOC_*)
 * [Return value]
 *     New texture, or NULL on error
 */
Texture *texture_new(int width, int height, uint32_t mem_flags
#ifdef DEBUG
                     , const char *srcfile, int srcline
#endif
)
{
    if (UNLIKELY(width <= 0) || UNLIKELY(height <= 0)) {
        DMSG("Invalid parameters: %d %d", width, height);
        return NULL;
    }

    /* Set the line stride as a multiple of 16 bytes. */

    const int stride_bytes = align_up(width*4, 16);

    /* Allocate a single memory buffer for the Texture structure and
     * pixel data buffer. */

    Texture *texture;
    const uint32_t struct_size = align_up(sizeof(*texture), 64);
    texture = debug_mem_alloc(struct_size + (stride_bytes * height), 64,
                              mem_flags, srcfile, srcline, MEM_INFO_TEXTURE);
    if (UNLIKELY(!texture)) {
        DMSG("Failed to allocate %dx%d texture", width, height);
        return NULL;
    }

    /* Initialize and return the data structure.  (The pixel data buffer
     * is not cleared.) */

    mem_clear(texture, struct_size);
    texture->width    = width;
    texture->height   = height;
    texture->stride   = stride_bytes/4;
    texture->indexed  = 0;
    texture->swizzled = 0;
    texture->mipmaps  = 0;
    texture->pixels   = (uint8_t *)texture + struct_size;

    return texture;
}

/*************************************************************************/

/**
 * texture_new_indexed:  Create a new, empty 8-bit indexed-color texture.
 * The width and height may be any positive values, but the texture may not
 * be suitable for drawing if the width and height are not both powers of
 * two.  The color palette passed in is copied, so the palette buffer need
 * not remain available after the call.
 *
 * [Parameters]
 *         width: Texture width (pixels)
 *        height: Texture height (pixels)
 *       palette: Color palette (256 colors)
 *     mem_flags: Memory allocation flags (MEM_ALLOC_*)
 * [Return value]
 *     New texture, or NULL on error
 */
Texture *texture_new_indexed(int width, int height, const uint32_t *palette,
                             uint32_t mem_flags
#ifdef DEBUG
                             , const char *srcfile, int srcline
#endif
)
{
    if (UNLIKELY(width <= 0) || UNLIKELY(height <= 0)) {
        DMSG("Invalid parameters: %d %d", width, height);
        return NULL;
    }

    const int stride_bytes = align_up(width, 16);

    Texture *texture;
    const uint32_t struct_size = align_up(sizeof(*texture), 64);
    texture = debug_mem_alloc(struct_size + 256*4 + (stride_bytes * height), 64,
                              mem_flags, srcfile, srcline, MEM_INFO_TEXTURE);
    if (UNLIKELY(!texture)) {
        DMSG("Failed to allocate %dx%d texture", width, height);
        return NULL;
    }

    mem_clear(texture, sizeof(*texture));
    texture->width    = width;
    texture->height   = height;
    texture->stride   = stride_bytes;
    texture->indexed  = 1;
    texture->swizzled = 0;
    texture->empty_l  = 0;
    texture->empty_r  = 0;
    texture->empty_t  = 0;
    texture->empty_b  = 0;
    texture->mipmaps  = 0;
    texture->palette  = (uint32_t *)((uint8_t *)texture + struct_size);
    texture->pixels   = (uint8_t *)(&texture->palette[256]);
    memcpy(texture->palette, palette, 256*4);

    return texture;
}

/*************************************************************************/

/**
 * texture_load:  Load a texture from a file.
 *
 * [Parameters]
 *          path: Relative file path (.png extension is optional)
 *     mem_flags: Memory allocation flags (MEM_ALLOC_*)
 * [Return value]
 *     Loaded texture, or NULL on error
 */
Texture *texture_load(const char *path, uint32_t mem_flags
#ifdef DEBUG
                      , const char *srcfile, int srcline
#endif
)
{
    if (UNLIKELY(!path)) {
        DMSG("path == NULL!");
        return NULL;
    }

    /* Create a ResourceManager to use for loading this texture. */

    ResourceManager resmgr;
    mem_clear(&resmgr, sizeof(resmgr));
    resource_create(&resmgr, 1);

    /* Generate a *.tex filename for the requested texture. */

    char pathbuf[1000];
    int pathlen = snprintf(pathbuf, sizeof(pathbuf)-4, "%s", path);
    if (pathlen >= sizeof(pathbuf)-4) {
        DMSG("Pathname too long: %s", path);
        return NULL;
    }
    if (stricmp(pathbuf + pathlen - 4, ".png") == 0) {
        pathbuf[pathlen-4] = 0;
        pathlen -= 4;
    }
    memcpy(pathbuf + pathlen, ".tex", 5);

    /* Try to load the *.tex file. */

    void *data = NULL;
    uint32_t size;
    int is_tex = 1;
    int exists = resource_load_data(&resmgr, &data, &size, pathbuf, 64,
                                    (mem_flags & MEM_ALLOC_TEMP ? RES_ALLOC_TEMP : 0)
                                  | (mem_flags & MEM_ALLOC_TOP  ? RES_ALLOC_TOP  : 0));

    /* If that failed, try the *.png version instead.  In this case, we
     * load the data into a temporary buffer, since the actual pixel buffer
     * will have to be reallocated afterward. */

    if (!exists) {
        memcpy(pathbuf + pathlen, ".png", 5);
        is_tex = 0;
        exists = resource_load_data(&resmgr, &data, &size, pathbuf,
                                    0, RES_ALLOC_TEMP);
    }

    /* If we couldn't open either file, give up. */

    if (!exists) {
        DMSG("No *.tex or *.png found for %s", path);
        return NULL;
    }

    /* Wait for the read to complete. */

    const int mark = resource_mark(&resmgr);
    resource_wait(&resmgr, mark);
    if (!data) {
        DMSG("Failed to load %s", pathbuf);
        resource_delete(&resmgr);
        return NULL;
    }

    /* Deallocate the resource manager we created above.  To prevent it
     * from also deleting our data buffer, we copy it to a new variable
     * and set the original one to NULL. */

    uint8_t *buffer = (uint8_t *)data;
    data = NULL;
    resource_delete(&resmgr);

    /* Parse the file's data and convert it to a texture. */

    return texture_parse(buffer, size, mem_flags, 1
#ifdef DEBUG
                         , srcfile, srcline
#endif
    );
}

/*************************************************************************/

/**
 * texture_parse:  Parse the contents of a texture data file loaded into
 * memory and return a new texture.
 *
 * [Parameters]
 *          data: File data buffer
 *           len: File length (bytes)
 *     mem_flags: Memory allocation flags (MEM_ALLOC_*)
 *         reuse: If nonzero, the data buffer will be reused if possible
 *                   and freed otherwise.  If zero, the data buffer will
 *                   not be touched.
 * [Return value]
 *     Texture, or NULL on error
 */
Texture *texture_parse(void *data, uint32_t len, uint32_t mem_flags, int reuse
#ifdef DEBUG
                       , const char *srcfile, int srcline
#endif
)
{
    if (UNLIKELY(!data)) {
        DMSG("data == NULL!");
        return NULL;
    }

    if (len >= 4 && memcmp(data, TEX_FILE_MAGIC, 4) == 0) {
        return texture_parse_tex(data, len, mem_flags, reuse
#ifdef DEBUG
                                 , srcfile, srcline
#endif
        );
    } else if (len >= 8 && png_sig_cmp(data, 0, 8) == 0) {
        return texture_parse_png(data, len, mem_flags, reuse
#ifdef DEBUG
                                 , srcfile, srcline
#endif
        );
    } else {
        DMSG("Unrecognized file format");
        return NULL;
    }
}

/*************************************************************************/

/**
 * texture_destroy:  Destroy a texture.  Does nothing if texture == NULL.
 *
 * [Parameters]
 *     texture: Texture to destroy
 * [Return value]
 *     None
 */
void texture_destroy(Texture *texture
#ifdef DEBUG
                     , const char *srcfile, int srcline
#endif
)
{
    /* We allocate textures as a single memory block, so we just need to
     * free that one block. */
    debug_mem_free(texture, srcfile, srcline, MEM_INFO_TEXTURE);
}

/*************************************************************************/
/**************************** Local functions ****************************/
/*************************************************************************/

/**
 * texture_parse_tex:  Parse a texture file in our custom TEX format.
 *
 * [Parameters]
 *          data: File data buffer
 *           len: File length (bytes)
 *     mem_flags: Memory allocation flags (MEM_ALLOC_*)
 *         reuse: If nonzero, the data buffer will be reused if possible
 *                   and freed otherwise.  If zero, the data buffer will
 *                   not be touched.
 * [Return value]
 *     Texture, or NULL on error
 */
static Texture *texture_parse_tex(uint8_t *data, uint32_t len,
                                  uint32_t mem_flags, int reuse
#ifdef DEBUG
                                  , const char *srcfile, int srcline
#endif
)
{
    PRECOND_SOFT(data != NULL, return NULL);


    /* Parse the header data.  (The magic value is assumed to have already
     * been checked.) */

    const TexFileHeader *header = (const TexFileHeader *)data;

    /* We save these values in local registers because we'll overwrite the
     * header data with structure data if we reuse the buffer. */
    const int width    = be_to_s16(header->width);
    const int height   = be_to_s16(header->height);
    const int stride   = be_to_s16(header->stride);
    const int indexed  = header->indexed;
    const int swizzled = header->swizzled;
    const int empty_l  = be_to_u16(header->empty_l);
    const int empty_r  = be_to_u16(header->empty_r);
    const int empty_t  = be_to_u16(header->empty_t);
    const int empty_b  = be_to_u16(header->empty_b);
    const int mipmaps  = header->mipmaps;
    const uint32_t pixels_offset  = be_to_u32(header->pixels_offset);
    const uint32_t palette_offset = be_to_u32(header->palette_offset);

    /* Allocate a Texture structure for the texture, or reuse the data
     * buffer if requested. */

    Texture *texture;
    const uint32_t struct_size = align_up(sizeof(*texture), 64);
    uint32_t total_size = struct_size + (stride * height * (indexed ? 1 : 4));
    if (indexed) {
        total_size += 256*4;
    }
    if (reuse) {
        texture = (Texture *)data;
    } else {
        texture = mem_alloc(total_size, 64, mem_flags);
        if (!texture) {
            DMSG("Out of memory for texture (%ux%u, %u bytes)", width, height,
                 total_size);
            return NULL;
        }
    }

    texture->width    = width;
    texture->height   = height;
    texture->stride   = stride;
    texture->indexed  = indexed;
    texture->swizzled = swizzled;
    texture->empty_l  = empty_l;
    texture->empty_r  = empty_r;
    texture->empty_t  = empty_t;
    texture->empty_b  = empty_b;
    texture->mipmaps  = mipmaps;
    if (reuse) {
        texture->palette =
            indexed ? (uint32_t *)((uintptr_t)texture + palette_offset) : NULL;
        texture->pixels = (uint8_t *)((uintptr_t)texture + pixels_offset);
    } else if (indexed) {
        texture->palette = (uint32_t *)((uintptr_t)texture + struct_size);
        texture->pixels  = (uint8_t *)((uintptr_t)texture->pixels * 256*4);
    } else {
        texture->palette = NULL;
        texture->pixels  = (uint8_t *)((uintptr_t)texture + struct_size);
    }

    /* If we're not reusing the data buffer, copy the texture data into
     * the new buffer. */

    if (!reuse) {
        memcpy((void *)((uintptr_t)texture + struct_size), data + struct_size,
               total_size - struct_size);
    }

    /* All done!  That was easy. */

    return texture;
}

/*************************************************************************/

/**
 * texture_parse_png:  Parse a texture file in PNG format, reducing the
 * texture width and height by half before returning it.
 *
 * [Parameters]
 *          data: File data buffer
 *           len: File length (bytes)
 *     mem_flags: Memory allocation flags (MEM_ALLOC_*)
 *         reuse: If nonzero, the data buffer will be reused if possible
 *                   and freed otherwise.  If zero, the data buffer will
 *                   not be touched.
 * [Return value]
 *     Texture, or NULL on error
 */
static Texture *texture_parse_png(uint8_t *data, uint32_t len,
                                  uint32_t mem_flags, int reuse
#ifdef DEBUG
                                  , const char *srcfile, int srcline
#endif
)
{
    PRECOND_SOFT(data != NULL, return NULL);

    /* We have to be able to free these on error, so we need volatile
     * declarations. */
    volatile uint8_t *data_volatile = data;
    volatile png_structp png_volatile = NULL;
    volatile png_infop info_volatile = NULL;
    volatile Texture *texture_volatile = NULL;
    volatile void *row_buffer_volatile = NULL;

    if (setjmp(png_jmpbuf) != 0) {
        /* libpng jumped back here with an error, so return the error. */
      error:  // Let's reuse it for our own error handling, too.
        mem_free((void *)row_buffer_volatile);
        mem_free((void *)texture_volatile);
        png_destroy_read_struct((png_structpp)&png_volatile,
                                (png_infopp)&info_volatile, NULL);
        if (reuse) {
            mem_free((void *)data_volatile);
        }
        return NULL;
    }


    /* Set up the PNG reader instance. */

    png_structp png = png_create_read_struct_2(
        PNG_LIBPNG_VER_STRING,
        NULL, png_error_callback, png_warning_callback,
        NULL, png_malloc_callback, png_free_callback
    );
    png_volatile = png;
    png_infop info = png_create_info_struct(png);
    info_volatile = info;
    png_set_read_fn(png, NULL, png_read_callback);
    png_readinfo.data = data;
    png_readinfo.len = len;
    png_readinfo.cur_offset = 0;

    /* Read the image information. */

    png_read_info(png, info);
    const unsigned int width      = png_get_image_width(png, info);
    const unsigned int height     = png_get_image_height(png, info);
    const unsigned int bit_depth  = png_get_bit_depth(png, info);
    const unsigned int color_type = png_get_color_type(png, info);
    if (png_get_interlace_type(png, info) != PNG_INTERLACE_NONE) {
        DMSG("Interlaced images not supported");
        goto error;
    }
    if (bit_depth < 8) {
        DMSG("Bit depth %d not supported", bit_depth);
        goto error;
    }

    /* Set up image transformation parameters. */

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    } else if (color_type == PNG_COLOR_TYPE_GRAY) {
        png_set_gray_to_rgb(png);
    }
    if (bit_depth == 16) {
        png_set_strip_16(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }
    if (color_type == PNG_COLOR_TYPE_RGB
     || color_type == PNG_COLOR_TYPE_GRAY
    ) {
        png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
    }
    png_read_update_info(png, info);

    /* Create the Texture structure. */

    Texture *texture;
    const uint32_t tex_width    = (width > 1) ? width/2 : 1;
    const uint32_t tex_height   = (height > 1) ? height/2 : 1;
    const uint32_t alloc_width  = align_up(tex_width, 4);
    const uint32_t alloc_height = align_up(tex_height, 8);
    const uint32_t struct_size  = align_up(sizeof(*texture), 64);
    texture = mem_alloc(struct_size + (alloc_width * alloc_height * 4), 64,
                        mem_flags);
    if (!texture) {
        DMSG("Out of memory for texture (%ux%u, %u bytes)", tex_width,
             tex_height, struct_size + (alloc_width * alloc_height * 4));
        goto error;
    }
    texture->width    = tex_width;
    texture->height   = tex_height;
    texture->stride   = alloc_width;
    texture->indexed  = 0;
    texture->swizzled = 1;
    texture->empty_l  = 0;
    texture->empty_r  = 0;
    texture->empty_t  = 0;
    texture->empty_b  = 0;
    texture->mipmaps  = 0;
    texture->pixels   = (uint8_t *)texture + struct_size;

    texture_volatile = texture;

    /* Read the image in 16 rows at a time, and reduce/swizzle the pixel
     * data into the texture's pixel buffer. */

    uint8_t *rows[16];
    uint32_t rowbytes = png_get_rowbytes(png, info);
    /* We round up to a multiple of 32 so we can safely access data in
     * 8-pixel units without worrying about address errors if the width is
     * not a multiple of 8. */
    rowbytes = align_up(rowbytes, 32);
    rows[0] = mem_alloc(rowbytes*16, 0, MEM_ALLOC_TEMP);
    if (!rows[0]) {
        DMSG("Out of memory for pixel read buffer (%u*16 bytes)", rowbytes);
        goto error;
    }
    row_buffer_volatile = rows[0];
    unsigned int y;
    for (y = 1; y < 16; y++) {
        rows[y] = rows[y-1] + rowbytes;
    }
    const unsigned int rowpix = rowbytes / 4;

    uint8_t *dest = texture->pixels;
    for (y = 0; y < height; y += 16) {
        const unsigned int this_rows = ubound(16, height - y);
        png_read_rows(png, rows, NULL, this_rows);
        unsigned int x;
        for (x = 0; x < width; x += 8) {
            const uint32_t *in = (const uint32_t *)&rows[0][x*4];
            unsigned int line;
            for (line = 0; line < 16; line += 2, in += rowpix*2, dest += 16) {
                const uint32_t pixel0 = in[0];
                const uint32_t pixel2 = in[2];
                const uint32_t pixel4 = in[4];
                const uint32_t pixel6 = in[6];
                ((uint32_t *)dest)[0] = pixel0;
                ((uint32_t *)dest)[1] = pixel2;
                ((uint32_t *)dest)[2] = pixel4;
                ((uint32_t *)dest)[3] = pixel6;
            }
        }
    }

    /* Done!  Close down the PNG reader and return success. */

    png_read_end(png, NULL);
    png_destroy_read_struct(&png, &info, NULL);
    if (reuse) {
        mem_free(data);
    }
    return texture;
}

/*-----------------------------------------------------------------------*/

/**
 * png_warning_callback, png_error_callback:  Error handling functions for
 * libpng.
 *
 * [Parameters]
 *         png: PNG reader structure
 *     message: Warning or error message
 * [Return value]
 *     None (png_error_callback() does not return)
 */
static void png_warning_callback(png_structp png, const char *message)
{
    DMSG("libpng warning: %s", message);
}

static void png_error_callback(png_structp png, const char *message)
{
    DMSG("libpng error: %s", message);
    longjmp(png_jmpbuf, 1);
}

/*-----------------------------------------------------------------------*/

/**
 * png_read_callback:  Read callback for libpng.
 *
 * [Parameters]
 *      png: PNG reader structure
 *      buf: Destination buffer
 *     size: Number of bytes to read
 * [Return value]
 *     None
 */
static void png_read_callback(png_structp png, uint8_t *buf, size_t size)
{
    size = ubound(size, png_readinfo.len - png_readinfo.cur_offset);
    memcpy(buf, png_readinfo.data + png_readinfo.cur_offset, size);
    png_readinfo.cur_offset += size;
}

/*-----------------------------------------------------------------------*/

/**
 * png_malloc_callback, png_free_callback:  Memory allocation/deallocation
 * callbacks for libpng.
 *
 * [Parameters]
 *      png: PNG reader structure
 *     size: Size of memory to allocate (png_malloc_callback() only)
 *      ptr: Pointer to memory to free (png_free_callback() only)
 * [Return value]
 *     Allocated memory block (png_malloc_callback() only)
 */
static void *png_malloc_callback(png_structp png, size_t size)
{
    return mem_alloc(size, 0, MEM_ALLOC_TEMP);
}

static void png_free_callback(png_structp png, void *ptr)
{
    mem_free(ptr);
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
