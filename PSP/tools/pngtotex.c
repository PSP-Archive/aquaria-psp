/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * tools/pngtotex.c: Program to convert PNG images to the *.tex custom
 * texture format used by the PSP engine.
 */

/*
 * To use, run this program as:
 *
 *     pngtotex [-8] [-m] file1.png [file2.png...]
 *
 * This will convert all named PNG files to the PSP engine's custom
 * texture format, saving the converted files with an extension of .tex
 * replacing any .png extension in the input filenames.  All textures
 * will be scaled to half size and swizzled, and if the -8 option is
 * specified, the textures will be converted to 8-bit indexed format.
 * If the -m option is specified, mipmaps will be generated for the
 * texture down to a minimum width of 16 or height of 8.
 *
 * When converting to indexed format, all textures specified in a single
 * invocation of the program will use the same palette.  This can be used
 * to ensure that related textures do not end up with slightly different
 * colors, but care must be taken not to specify too many textures at once,
 * lest the resultant palette end up suboptimal for all of them.
 */

#include "../src/common.h"
#include "../src/texture.h"

#include "quantize.h"
#include "zoom.h"


#define PNG_USER_MEM_SUPPORTED
#include <png.h>

static jmp_buf png_jmpbuf;

/*************************************************************************/

static int convert_to_8bpp(const char **files, unsigned int num_files,
                           int mipmaps);
static int convert_to_32bpp(const char **files, unsigned int num_files,
                            int mipmaps);

static Texture *read_png(const char *path);
static void png_warning_callback(png_structp png, const char *message);
static void png_error_callback(png_structp png, const char *message);

static int shrink_texture(Texture *tex);
static int generate_mipmaps(Texture *tex, int mipmaps,
                            uint32_t *extra_pixels_ret);
static int quantize_texture(Texture *tex);
static int swizzle_texture(Texture *tex);

static int write_tex(Texture *tex, const char *path);

/*************************************************************************/
/*************************************************************************/

/**
 * main:  Program entry point.  Parses command-line parameters and calls
 * either the 8bpp or 32bpp texture converter.
 *
 * [Parameters]
 *     argc: Command line argument count
 *     argv: Command line argument vector
 * [Return value]
 *     Zero on successful completion, nonzero if an error occurred
 */
int main(int argc, char **argv)
{
    int indexed = 0;  // Convert to 8bpp indexed format?
    int mipmaps = 0;  // How many mipmaps to add?

    int argi = 1;
    while (argi < argc && argv[argi][0] == '-') {
        if (strcmp(argv[argi], "-8") == 0) {
            indexed = 1;
        } else if (strncmp(argv[argi], "-m", 2) == 0) {
            if (argv[argi][2] == 0) {
                mipmaps = 7;  // The PSP only supports up to 7 mipmaps
            } else {
                mipmaps = ubound(strtoul(argv[argi]+2, NULL, 10), 7);
            }
        } else {
            goto usage;
        }
        argi++;
    }
    if (argi >= argc) {
      usage:
        fprintf(stderr, "\n"
                "Usage: %s [-8] [-m[N]] file1.png [file2.png...]\n"
                "\n"
                "-8 will quantize to 8bpp indexed textures; all textures\n"
                "listed on the same command line will use the same palette.\n"
                "\n"
                "-m will generate mipmaps for each texture; adding a number\n"
                "(like -m2) limits the number of additional mipmaps to that\n"
                "number or fewer.\n"
                "\n",
                argv[0]);
        return 1;
    }

    int ok;
    if (indexed) {
        ok = convert_to_8bpp((const char **)&argv[argi], argc - argi, mipmaps);
    } else {
        ok = convert_to_32bpp((const char **)&argv[argi], argc - argi, mipmaps);
    }

    return ok ? 0 : 1;
}

/*-----------------------------------------------------------------------*/

/**
 * convert_to_8bpp:  Convert a list of PNG textures to the custom *.tex
 * format using 8bpp indexed-color pixels.
 *
 * [Parameters]
 *         files: Array of filenames
 *     num_files: Number of filenames in array
 *       mipmaps: Number of additional mipmaps to create for each texture
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int convert_to_8bpp(const char **files, unsigned int num_files,
                           int mipmaps)
{
    Texture **textures;
    uint32_t total_pixels = 0;
    unsigned int i;

    /* Allocate an array for holding the texture pointers. */

    textures = malloc(sizeof(*textures) * num_files);
    if (!textures) {
        fprintf(stderr, "Out of memory for texture array (%u entries)\n",
                num_files);
        goto error_return;
    }

    for (i = 0; i < num_files; i++) {
        textures[i] = NULL;
    }

    /* First load all the specified files into memory and shrink them
     * down to half size, adding mipmaps if requested. */

    for (i = 0; i < num_files; i++) {
        textures[i] = read_png(files[i]);
        if (!textures[i]) {
            fprintf(stderr, "Failed to read %s\n", files[i]);
            goto error_free_textures;
        }

        if (!shrink_texture(textures[i])) {
            fprintf(stderr, "Failed to shrink %s\n", files[i]);
            goto error_free_textures;
        }

        uint32_t extra_pixels = 0;
        if (mipmaps > 0
         && !generate_mipmaps(textures[i], mipmaps, &extra_pixels)
        ) {
            fprintf(stderr, "Failed to generate mipmaps for %s\n", files[i]);
            goto error_free_textures;
        }

        total_pixels += textures[i]->width * textures[i]->height + extra_pixels;
    }

    /* Copy all the color data into a linear array of pixels, then use
     * that to generate a shared color palette. */

    uint32_t palette[256];
    memset(palette, 0, sizeof(palette));

    void *temp_pixelbuf = malloc(4 * total_pixels);
    if (!temp_pixelbuf) {
        fprintf(stderr, "Out of memory for temporary pixel buffer (%u"
                " pixels)\n", total_pixels);
        goto error_free_textures;
    }

    uint32_t *dest = temp_pixelbuf;
    for (i = 0; i < num_files; i++) {
        const uint32_t *src = (const uint32_t *)textures[i]->pixels;
        unsigned int width  = textures[i]->width;
        unsigned int height = textures[i]->height;
        unsigned int stride = textures[i]->stride;
        unsigned int level;
        for (level = 0; level <= textures[i]->mipmaps; level++) {
            int y;
            for (y = 0; y < height; y++, src += stride, dest += width) {
                memcpy(dest, src, width * 4);
            }
            width  = (width +1) / 2;
            height = (height+1) / 2;
            stride = align_up(stride/2, 4);
        }
    }

    generate_palette(temp_pixelbuf, total_pixels, 1, total_pixels,
                     palette, 0, NULL);

    free(temp_pixelbuf);

    /* Copy the shared palette into each texture, then quantize and write
     * the texture to disk. */

    for (i = 0; i < num_files; i++) {
        textures[i]->palette = palette;

        if (!quantize_texture(textures[i])) {
            fprintf(stderr, "Failed to quantize %s\n", files[i]);
            goto error_free_textures;
        }
        if (!swizzle_texture(textures[i])) {
            fprintf(stderr, "Failed to swizzle %s\n", files[i]);
            goto error_free_textures;
        }

        char pathbuf[1000];
        int pathlen = snprintf(pathbuf, sizeof(pathbuf)-4, "%s", files[i]);
        if (pathlen >= sizeof(pathbuf)-4) {
            DMSG("Pathname too long: %s", files[i]);
            goto error_free_textures;
        }
        if (stricmp(pathbuf + pathlen - 4, ".png") == 0) {
            pathbuf[pathlen-4] = 0;
            pathlen -= 4;
        }
        memcpy(pathbuf + pathlen, ".tex", 5);
        if (!write_tex(textures[i], pathbuf)) {
            fprintf(stderr, "Failed to write %s\n", pathbuf);
            goto error_free_textures;
        }

        free(textures[i]);
    }

    /* All done! */

    free(textures);
    return 1;

    /* On error, clean up after ourselves. */

  error_free_textures:
    for (i = 0; i < num_files; i++) {
        free(textures[i]);
        free(textures);
    }
  error_return:
    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * convert_to_32bpp:  Convert a list of PNG textures to the custom *.tex
 * format using 32bpp pixels.
 *
 * [Parameters]
 *         files: Array of filenames
 *     num_files: Number of filenames in array
 *       mipmaps: Number of additional mipmaps to create for each texture
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int convert_to_32bpp(const char **files, unsigned int num_files,
                            int mipmaps)
{
    unsigned int i;

    for (i = 0; i < num_files; i++) {

        Texture *texture = read_png(files[i]);
        if (!texture) {
            fprintf(stderr, "Failed to read %s\n", files[i]);
            return 0;
        }

        if (!shrink_texture(texture)) {
            fprintf(stderr, "Failed to shrink %s\n", files[i]);
            free(texture);
            return 0;
        }
        if (mipmaps > 0 && !generate_mipmaps(texture, mipmaps, NULL)) {
            fprintf(stderr, "Failed to generate mipmaps for %s\n", files[i]);
            free(texture);
            return 0;
        }
        if (!swizzle_texture(texture)) {
            fprintf(stderr, "Failed to swizzle %s\n", files[i]);
            free(texture);
            return 0;
        }

        char pathbuf[1000];
        int pathlen = snprintf(pathbuf, sizeof(pathbuf)-4, "%s", files[i]);
        if (pathlen >= sizeof(pathbuf)-4) {
            DMSG("Pathname too long: %s", files[i]);
            free(texture);
            return 0;
        }
        if (stricmp(pathbuf + pathlen - 4, ".png") == 0) {
            pathbuf[pathlen-4] = 0;
            pathlen -= 4;
        }
        memcpy(pathbuf + pathlen, ".tex", 5);
        if (!write_tex(texture, pathbuf)) {
            fprintf(stderr, "Failed to write %s\n", pathbuf);
            free(texture);
            return 0;
        }

        free(texture);
    }

    return 1;
}

/*************************************************************************/

/**
 * read_png:  Read a PNG file into a Texture data structure.
 *
 * [Parameters]
 *     path: Pathname of file to read
 * [Return value]
 *     Texture, or NULL on error
 */
static Texture *read_png(const char *path)
{
    /* We have to be able to free these on error, so we need volatile
     * declarations. */
    volatile FILE *f_volatile = NULL;
    volatile png_structp png_volatile = NULL;
    volatile png_infop info_volatile = NULL;
    volatile Texture *texture_volatile = NULL;
    volatile void *row_buffer_volatile = NULL;

    if (setjmp(png_jmpbuf) != 0) {
        /* libpng jumped back here with an error, so return the error. */
      error:  // Let's reuse it for our own error handling, too.
        free((void *)row_buffer_volatile);
        free((void *)texture_volatile);
        png_destroy_read_struct((png_structpp)&png_volatile,
                                (png_infopp)&info_volatile, NULL);
        if (f_volatile) {
            fclose((FILE *)f_volatile);
        }
        return NULL;
    }


    /* Open the requested file. */

    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        return NULL;
    }
    f_volatile = f;

    /* Set up the PNG reader instance. */

    png_structp png = png_create_read_struct(
        PNG_LIBPNG_VER_STRING,
        (void *)path, png_error_callback, png_warning_callback
    );
    png_volatile = png;
    png_infop info = png_create_info_struct(png);
    info_volatile = info;
    png_init_io(png, f);

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

    /* Create the texture structure. */

    Texture *texture;
    const uint32_t tex_width    = width;
    const uint32_t tex_height   = height;
    const uint32_t alloc_width  = align_up(width, 4);
    const uint32_t alloc_height = align_up(height, 8);
    const uint32_t struct_size  = align_up(sizeof(*texture), 64);
    texture = malloc(struct_size + (alloc_width * alloc_height * 4));
    if (!texture) {
        DMSG("Out of memory for texture (%ux%u, %u bytes)", tex_width,
             tex_height, struct_size + (alloc_width * alloc_height * 4));
        goto error;
    }
    texture->width    = tex_width;
    texture->height   = tex_height;
    texture->stride   = alloc_width;
    texture->indexed  = 0;
    texture->swizzled = 0;
    texture->empty_l  = tex_width;
    texture->empty_r  = tex_width;
    texture->empty_t  = tex_height;
    texture->empty_b  = tex_height;
    texture->mipmaps  = 0;
    texture->pixels   = (uint8_t *)texture + struct_size;

    texture_volatile = texture;

    /* Read the image in one row at a time, updating the empty_[lrtb]
     * fields as we go. */

    uint8_t *row_buffer;
    uint32_t rowbytes = png_get_rowbytes(png, info);
    row_buffer = malloc(rowbytes);
    if (!row_buffer) {
        DMSG("Out of memory for pixel read buffer (%u bytes)", rowbytes);
        goto error;
    }
    row_buffer_volatile = row_buffer;

    uint8_t *dest = texture->pixels;
    unsigned int y;
    for (y = 0; y < height; y++, dest += texture->stride * 4) {
        png_read_row(png, row_buffer, NULL);
        memcpy(dest, row_buffer, rowbytes);
        int whole_row_empty = 1;
        unsigned int x;
        for (x = 0; x < width; x++) {
            if (dest[x*4+3] != 0) {
                whole_row_empty = 0;
                texture->empty_l = min(texture->empty_l, x);
                texture->empty_r = min(texture->empty_r, tex_width - (x+1));
            }
        }
        if (!whole_row_empty) {
            texture->empty_t = min(texture->empty_t, y);
            texture->empty_b = min(texture->empty_b, tex_height - (y+1));
        }
    }

    row_buffer_volatile = NULL;
    free(row_buffer);

    /* Decrement all the empty_[lrtb] fields by one (if they're not zero
     * already) to provide a 1-pixel transparent buffer around the texture
     * and ensure that graphics hardware rounding doesn't cut anything off. */

    if (texture->empty_l > 0) {
        texture->empty_l--;
    }
    if (texture->empty_r > 0) {
        texture->empty_r--;
    }
    if (texture->empty_t > 0) {
        texture->empty_t--;
    }
    if (texture->empty_b > 0) {
        texture->empty_b--;
    }

    /* Done!  Close down the PNG reader and return success. */

    png_read_end(png, NULL);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(f);
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
    fprintf(stderr, "libpng warning: %s: %s\n",
            (char *)png_get_error_ptr(png), message);
}

static void png_error_callback(png_structp png, const char *message)
{
    fprintf(stderr, "libpng error: %s: %s\n",
            (char *)png_get_error_ptr(png), message);
    longjmp(png_jmpbuf, 1);
}

/*************************************************************************/

/**
 * shrink_texture:  Shrink the given texture to half its current width and
 * height, unless the width or height is 1 (in which case nothing is done).
 * The texture buffer is _not_ reallocated.
 *
 * [Parameters]
 *     tex: Texture to shrink
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int shrink_texture(Texture *tex)
{
    if (tex->width == 1 || tex->height == 1) {
        return 1;
    }

    const int new_width  = tex->width / 2;
    const int new_height = tex->height / 2;
    const int new_stride = align_up(tex->stride / 2, 4);
    void *tempbuf = malloc(new_stride * new_height * 4);
    if (!tempbuf) {
        fprintf(stderr, "Out of memory for shrink buffer (%d bytes)\n",
                new_width * new_height * 4);
    }

    ZoomInfo *zi = zoom_init(tex->width, tex->height,
                             new_width, new_height,
                             4, tex->stride*4, new_stride*4,
                             1, TCV_ZOOM_CUBIC_KEYS4);
    if (!zi) {
        fprintf(stderr, "zoom_init() failed\n");
        free(tempbuf);
        return 0;
    }
    zoom_process(zi, tex->pixels, tempbuf);
    zoom_free(zi);

    tex->width  = new_width;
    tex->height = new_height;
    tex->stride = new_stride;
    memcpy(tex->pixels, tempbuf, new_stride * new_height * 4);
    free(tempbuf);

    /* Make sure we keep our 1-pixel transparent buffer at the new size. */
    if (tex->empty_l > 0) {
        tex->empty_l = (tex->empty_l - 1) / 2;
    }
    if (tex->empty_r > 0) {
        tex->empty_r = (tex->empty_r - 1) / 2;
    }
    if (tex->empty_t > 0) {
        tex->empty_t = (tex->empty_t - 1) / 2;
    }
    if (tex->empty_b > 0) {
        tex->empty_b = (tex->empty_b - 1) / 2;
    }

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * generate_mipmaps:  Generate mipmaps for the given texture through a
 * minimum width of 16 or height of 8 pixels, whichever is reached first.
 * If the texture's original size is less than 17 pixels wide or 9 pixels
 * high, nothing is done.  The texture buffer is _not_ reallocated, on the
 * assumption that shrink_texture() has already been called and therefore
 * sufficient buffer space is already available for the mipmaps.
 *
 * [Parameters]
 *                  tex: Texture to shrink
 *              mipmaps: Maximum number of mipmaps to generate
 *     extra_pixels_ret: Pointer to variable to receive the number of
 *                          pixels added, or NULL if not needed
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int generate_mipmaps(Texture *tex, int mipmaps,
                            uint32_t *extra_pixels_ret)
{
    if (extra_pixels_ret) {
        *extra_pixels_ret = 0;
    }

    unsigned int width  = tex->width;
    unsigned int height = tex->height;
    unsigned int stride = tex->stride;
    uint8_t *    pixels = tex->pixels;

    while (mipmaps-- > 0 && width > 16 && height > 8) {

        const unsigned int old_width  = width;
        const unsigned int old_height = height;
        const unsigned int old_stride = stride;
        uint8_t * const    old_pixels = pixels;

        pixels += stride * height * 4;
        width  = (width +1) / 2;
        height = (height+1) / 2;
        stride = align_up(stride/2, 4);

        ZoomInfo *zi = zoom_init(old_width, old_height, width, height,
                                 4, old_stride*4, stride*4,
                                 1, TCV_ZOOM_CUBIC_KEYS4);
        if (!zi) {
            fprintf(stderr, "zoom_init() failed\n");
            return 0;
        }
        zoom_process(zi, old_pixels, pixels);
        zoom_free(zi);

        const int expand_empty_buffer = 1 << tex->mipmaps;
        tex->empty_l = lbound(tex->empty_l - expand_empty_buffer, 0);
        tex->empty_r = lbound(tex->empty_r - expand_empty_buffer, 0);
        tex->empty_t = lbound(tex->empty_t - expand_empty_buffer, 0);
        tex->empty_b = lbound(tex->empty_b - expand_empty_buffer, 0);

        tex->mipmaps++;
        if (extra_pixels_ret) {
            *extra_pixels_ret += width * height;
        }

    }

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * quantize_texture:  Convert the given texture to indexed-color 8bpp by
 * quantizing the color palette down to the 256 colors specified in
 * tex->palette.  The texture buffer is _not_ reallocated.
 *
 * [Parameters]
 *     tex: Texture to quantize
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int quantize_texture(Texture *tex)
{
    unsigned int width  = tex->width;
    unsigned int height = tex->height;
    unsigned int stride = tex->stride;
    uint32_t *pixels_in = (uint32_t *)tex->pixels;
    uint8_t *pixels_out = tex->pixels;

    unsigned int level;
    for (level = 0; level <= tex->mipmaps; level++) {
        const int new_stride = align_up(stride, 16);
        if (!quantize_image(pixels_in, stride, pixels_out, new_stride,
                            width, height, tex->palette, 256, 0)) {
            fprintf(stderr, "quantize_image() failed for level %u\n", level);
            return 0;
        }
        if (level == 0) {
            tex->stride = new_stride;
        }
        pixels_in  += stride * height;
        pixels_out += new_stride * height;
        width  = (width +1) / 2;
        height = (height+1) / 2;
        stride = align_up(stride/2, 4);
    }

    tex->indexed = 1;
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * swizzle_texture:  Swizzle the given texture's pixel data.  The texture
 * buffer is _not_ reallocated (so it must be preallocated with a width
 * a multiple of 16 bytes and a height a multiple of 8 lines).
 *
 * [Parameters]
 *     tex: Texture to swizzle
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int swizzle_texture(Texture *tex)
{
    unsigned int width  = tex->width;
    unsigned int height = tex->height;
    unsigned int stride = tex->stride;
    /* The pixels can be either 8bpp or 32bpp, but we process them as
     * 32bpp for speed.  (The swizzle block width is 16 bytes either way.) */
    uint32_t *pixels = (uint32_t *)tex->pixels;
    unsigned int stride_words = (tex->indexed ? stride/4 : stride);

    uint32_t *tempbuf = malloc(8 * (stride_words*4));
    if (!tempbuf) {
        fprintf(stderr, "Out of memory for temporary buffer (%u bytes)\n",
                8 * (stride_words*4));
        return 0;
    }

    unsigned int level;
    for (level = 0; level <= tex->mipmaps; level++) {
        const uint32_t *src = pixels;
        uint32_t *dest = pixels;

        int y;
        for (y = 0; y < tex->height; y += 8, src += 8*stride_words) {
            memcpy(tempbuf, src, 8 * (stride_words*4));
            const uint32_t *tempsrc = tempbuf;
            int x;
            for (x = 0; x < stride_words; x += 4, tempsrc += 4) {
                const uint32_t *linesrc = tempsrc;
                int line;
                for (line = 0; line < 8;
                     line++, linesrc += stride_words, dest += 4
                ) {
                    const uint32_t pixel0 = linesrc[0];
                    const uint32_t pixel1 = linesrc[1];
                    const uint32_t pixel2 = linesrc[2];
                    const uint32_t pixel3 = linesrc[3];
                    dest[0] = pixel0;
                    dest[1] = pixel1;
                    dest[2] = pixel2;
                    dest[3] = pixel3;
                }
            }
        }

        pixels += stride_words * height;
        width  = (width +1) / 2;
        height = (height+1) / 2;
        stride_words = align_up(stride_words/2, 4);
    }

    free(tempbuf);
    tex->swizzled = 1;
    return 1;
}

/*************************************************************************/

/**
 * write_tex:  Write a custom-format *.tex file for the given texture.
 *
 * [Parameters]
 *      tex: Texture to write
 *     path: Pathname for new file
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int write_tex(Texture *tex, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror(path);
        return 0;
    }

    TexFileHeader header;
    memset(&header, 0, sizeof(header));
    const uint32_t header_aligned_size = align_up(sizeof(header), 64);
    memcpy(header.magic, TEX_FILE_MAGIC, sizeof(header.magic));
    header.width    = s16_to_be(tex->width);
    header.height   = s16_to_be(tex->height);
    header.stride   = s16_to_be(tex->stride);
    header.indexed  = tex->indexed;
    header.swizzled = tex->swizzled;
    header.empty_l  = s16_to_be(tex->empty_l);
    header.empty_r  = s16_to_be(tex->empty_r);
    header.empty_t  = s16_to_be(tex->empty_t);
    header.empty_b  = s16_to_be(tex->empty_b);
    header.mipmaps  = tex->mipmaps;
    if (tex->indexed) {
        header.palette_offset = u32_to_be(header_aligned_size);
        header.pixels_offset  = u32_to_be(header_aligned_size + 256*4);
    } else {
        header.palette_offset = u32_to_be(0);
        header.pixels_offset  = u32_to_be(header_aligned_size);
    }

    uint8_t header_buf[align_up(sizeof(header), 64)];
    memset(header_buf, 0, sizeof(header_buf));
    memcpy(header_buf, &header, sizeof(header));
    if (fwrite(header_buf, sizeof(header_buf), 1, f) != 1) {
        perror(path);
        fclose(f);
        remove(path);
        return 0;
    }

    if (tex->indexed) {
        if (fwrite(tex->palette, 4, 256, f) != 256) {
            perror(path);
            fclose(f);
            remove(path);
            return 0;
        }
    }

    unsigned int width = tex->width, height = tex->height;
    unsigned int stride = tex->stride;
    const uint8_t *pixels = tex->pixels;
    unsigned int level;
    for (level = 0; level <= tex->mipmaps; level++) {
        const unsigned int data_height =
            tex->swizzled ? align_up(height, 8) : height;
        if (fwrite(pixels, tex->indexed ? 1 : 4, stride * data_height, f)
            != stride * data_height
        ) {
            perror(path);
            fclose(f);
            remove(path);
            return 0;
        }
        pixels += stride * data_height * (tex->indexed ? 1 : 4);
        width  = (width+1)/2;
        height = (height+1)/2;
        stride = align_up(stride/2, tex->indexed ? 16 : 4);
    }

    fclose(f);
    return 1;
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
