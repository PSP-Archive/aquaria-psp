/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/texture.h: Texture management header.
 */

#ifndef TEXTURE_H
#define TEXTURE_H

/*************************************************************************/

/* Data structure used for textures. */

// typedef struct Texture_ Texture;  -- in common.h
struct Texture_ {
    int16_t width, height;  // Texture size (pixels)
    int16_t stride;         // Texture line stride (pixels, always a
                            //    multiple of 16 bytes)
    uint8_t indexed;        // 1 = 8-bit indexed color, 0 = 32-bit color
    uint8_t swizzled;       // 1 = data is swizzled
    int16_t empty_l;        // Number of transparent columns on left edge
    int16_t empty_r;        // Number of transparent columns on right edge
    int16_t empty_t;        // Number of transparent rows on top edge
    int16_t empty_b;        // Number of transparent rows on bottom edge
    uint8_t mipmaps;        // Number of mipmap levels, _not_ including
                            //    primary texture data (0-7); odd sizes
                            //    are rounded up when halving to compute
                            //    mipmap width/height
    uint8_t *pixels;        // Pixel data; mipmaps are appended immediately
                            //    following the primary texture data in
                            //    decreasing size order
    uint32_t *palette;      // Color palette (for indexed-color images)
};


/* File header for custom-format texture files.  All integer values are
 * stored in big-endian format, and the pixel and color palette data must
 * be aligned to a multiple of 64 bytes. */

typedef struct TexFileHeader_ {
    char magic[4];          // File identifier (TEX_FILE_MAGIC)
    int16_t width, height;
    int16_t stride;
    uint8_t indexed;
    uint8_t swizzled;
    int16_t empty_l;
    int16_t empty_r;
    int16_t empty_t;
    int16_t empty_b;
    uint8_t mipmaps;
    uint8_t pad[3];
    uint32_t pixels_offset;
    uint32_t palette_offset;
} TexFileHeader;

#define TEX_FILE_MAGIC  "TEX\012"

/*-----------------------------------------------------------------------*/

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
extern Texture *texture_new(int width, int height, uint32_t mem_flags
#ifdef DEBUG
                            , const char *srcfile, int srcline
#endif
);

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
extern Texture *texture_new_indexed(int width, int height, const uint32_t *palette,
                                    uint32_t mem_flags
#ifdef DEBUG
                                    , const char *srcfile, int srcline
#endif
);

/**
 * texture_load:  Load a texture from a file.
 *
 * [Parameters]
 *          path: Relative file path (.png extension is optional)
 *     mem_flags: Memory allocation flags (MEM_ALLOC_*)
 * [Return value]
 *     Loaded texture, or NULL on error
 */
extern Texture *texture_load(const char *path, uint32_t mem_flags
#ifdef DEBUG
                             , const char *srcfile, int srcline
#endif
);

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
extern Texture *texture_parse(void *data, uint32_t len, uint32_t mem_flags, int reuse
#ifdef DEBUG
                              , const char *srcfile, int srcline
#endif
);

/**
 * texture_destroy:  Destroy a texture.  Does nothing if texture == NULL.
 *
 * [Parameters]
 *     texture: Texture to destroy
 * [Return value]
 *     None
 */
extern void texture_destroy(Texture *texture
#ifdef DEBUG
                            , const char *srcfile, int srcline
#endif
);

/*-----------------------------------------------------------------------*/

/* Define replacement macros for the function calls in debug mode to pass
 * source line information down. */

#ifdef DEBUG
# define texture_new(width,height,mem_flags) \
    texture_new((width), (height), (mem_flags), __FILE__, __LINE__)
# define texture_new_indexed(width,height,palette,mem_flags) \
    texture_new_indexed((width), (height), (palette), (mem_flags), \
                        __FILE__, __LINE__)
# define texture_load(path,mem_flags) \
    texture_load((path), (mem_flags), __FILE__, __LINE__)
# define texture_parse(data,len,mem_flags,reuse) \
    texture_parse((data), (len), (mem_flags), (reuse), __FILE__, __LINE__)
# define texture_destroy(texture) \
    texture_destroy((texture), __FILE__, __LINE__)
#endif

/*************************************************************************/

#endif  // TEXTURE_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
