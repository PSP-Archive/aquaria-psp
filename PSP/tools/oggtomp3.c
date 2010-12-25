/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * tools/oggtomp3.c: Program to convert Ogg Vorbis audio to MP3 format,
 * which can be decoded much more quickly on the PSP.
 */

/*
 * To use, run this program as:
 *
 *     oggtomp3 file1.ogg [file2.ogg...]
 *
 * This will convert all named Ogg Vorbis files to MP3 format using the
 * LAME encoder, with settings equivalent to "lame -V2 -q1" (as of LAME
 * 3.98).  The converted files are saved with an extension of .mp3
 * replacing any .ogg extension in the input filenames.
 */

#include "../src/common.h"

#include <vorbis/vorbisfile.h>
#include <lame/lame.h>

/*************************************************************************/

static int convert_to_mp3(const char *file);

static void lame_error_callback(const char *format, va_list args);
static void lame_message_callback(const char *format, va_list args);
static void lame_debug_callback(const char *format, va_list args);

/*************************************************************************/
/*************************************************************************/

/**
 * main:  Program entry point.  Parses command-line parameters and calls
 * convert_to_mp3() for each file specified.
 *
 * [Parameters]
 *     argc: Command line argument count
 *     argv: Command line argument vector
 * [Return value]
 *     Zero on successful completion, nonzero if an error occurred
 */
int main(int argc, char **argv)
{
    if (argc < 2 || argv[1][0] == '-') {
        fprintf(stderr, "Usage: %s file1.ogg [file2.ogg...]\n", argv[0]);
        return 1;
    }

    int argi;
    for (argi = 1; argi < argc; argi++) {
        if (!convert_to_mp3(argv[argi])) {
            fprintf(stderr, "Failed to convert %s\n", argv[argi]);
            return 1;
        }
    }

    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * convert_to_mp3:  Convert an Ogg Vorbis audio file to MP3.
 *
 * [Parameters]
 *     path: Pathname of Ogg Vorbis file to convert
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int convert_to_mp3(const char *path)
{
    /* Generate the MP3 output filename. */

    char mp3_pathbuf[1000];
    int pathlen = snprintf(mp3_pathbuf, sizeof(mp3_pathbuf)-4, "%s", path);
    if (pathlen >= sizeof(mp3_pathbuf)-4) {
        DMSG("Pathname too long: %s", path);
        goto error_return;
    }
    if (stricmp(mp3_pathbuf + pathlen - 4, ".ogg") == 0) {
        mp3_pathbuf[pathlen-4] = 0;
        pathlen -= 4;
    }
    memcpy(mp3_pathbuf + pathlen, ".mp3", 5);

    /* Open the input and output files. */

    FILE *in = fopen(path, "rb");
    if (!in) {
        perror(path);
        goto error_return;
    }
    FILE *out = fopen(mp3_pathbuf, "wb");
    if (!out) {
        perror(mp3_pathbuf);
        goto error_close_in;
    }

    /* Initialize an Ogg Vorbis reader instance, and obtain the source
     * audio parameters. */

    OggVorbis_File vf;
    if (ov_open_callbacks(in, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) != 0) {
        fprintf(stderr, "%s: ov_open() failed\n", path);
        goto error_close_out;
    }

    vorbis_info *info = ov_info(&vf, -1);
    if (!info) {
        fprintf(stderr, "%s: ov_info() failed\n", path);
        goto error_close_vorbis;
    }
    if (info->channels != 1 && info->channels != 2) {
        fprintf(stderr, "%s: Bad channel count %d\n", path, info->channels);
        goto error_close_vorbis;
    }

    /* Read the PCM data into memory. */

    uint32_t total_size = 0;
    uint8_t *pcm_buffer = NULL;
    uint32_t buffer_size = 0;
    for (;;) {
        if (total_size >= buffer_size) {
            buffer_size += 1000000;
            uint8_t *new_buffer = realloc(pcm_buffer, buffer_size);
            if (!new_buffer) {
                fprintf(stderr, "%s: Out of memory for PCM data (%u bytes)\n",
                        path, buffer_size);
                goto error_free_pcm_buffer;
            }
            pcm_buffer = new_buffer;
        }

        int bitstream_unused;
        const int32_t nread = ov_read(
            &vf, (char *)(pcm_buffer + total_size), buffer_size - total_size,
            /*bigendianp*/ 0, /*word*/ 2, /*sgned*/ 1, &bitstream_unused
        );
        if (nread == 0 || nread == OV_EOF) {
            break;
        } else if (nread == OV_HOLE) {
            fprintf(stderr, "%s: Warning: Possible corrupt data at sample"
                    " %u, continuing anyway\n", path,
                    total_size / (info->channels*2));
        } else if (nread < 0) {
            fprintf(stderr, "%s: Error %d decompressing Ogg Vorbis data\n",
                    path, nread);
            goto error_free_pcm_buffer;
        } else {
            total_size += nread;
        }
    }

    const uint32_t num_samples = total_size / (info->channels * 2);

    /* Set up a LAME encoding context. */

    lame_global_flags *lame = lame_init();
    if (!lame) {
        fprintf(stderr, "%s: lame_init() failed\n", path);
        goto error_free_pcm_buffer;
    }
    lame_set_errorf(lame, lame_error_callback);
    lame_set_msgf  (lame, lame_message_callback);
    lame_set_debugf(lame, lame_debug_callback);
    if (lame_set_in_samplerate(lame, info->rate) != 0) {
        fprintf(stderr, "%s: lame_set_in_samplerate() failed\n", path);
        goto error_close_lame;
    }
    if (lame_set_num_channels(lame, info->channels) != 0) {
        fprintf(stderr, "%s: lame_set_num_channels() failed\n", path);
        goto error_close_lame;
    }
    if (lame_set_bWriteVbrTag(lame, 1) != 0) {
        fprintf(stderr, "%s: lame_set_bWriteVbrTag() failed\n", path);
        goto error_close_lame;
    }
    if (lame_set_mode(lame, info->channels==1 ? MONO : JOINT_STEREO) != 0) {
        fprintf(stderr, "%s: lame_set_mode() failed\n", path);
        goto error_close_lame;
    }
    if (lame_set_VBR(lame, vbr_default) != 0) {
        fprintf(stderr, "%s: lame_set_VBR() failed\n", path);
        goto error_close_lame;
    }
    if (lame_set_VBR_q(lame, 2) != 0) {
        fprintf(stderr, "%s: lame_set_VBR_q() failed\n", path);
        goto error_close_lame;
    }
    if (lame_set_quality(lame, 1) != 0) {
        fprintf(stderr, "%s: lame_set_quality() failed\n", path);
        goto error_close_lame;
    }
    if (lame_init_params(lame) != 0) {
        fprintf(stderr, "%s: lame_init_params() failed\n", path);
        goto error_close_lame;
    }

    /* Encode the audio as MP3 and write it to the output file. */

    const int blocksize = 8192;
#define blocksize 8192 //Avoid a spurious error from GCC 4.5 (bugzilla #46028).
    uint8_t mp3_buffer[blocksize*5/4 + 7200];
#undef blocksize
    uint32_t pos;

    for (pos = 0; pos < num_samples; pos += blocksize) {

        const uint32_t this_samples = ubound(blocksize, num_samples - pos);
        short *src = (short *)(pcm_buffer + pos * (info->channels*2));
        int res;
        if (info->channels == 1) {
            res = lame_encode_buffer(
                lame, src, src, this_samples, mp3_buffer, sizeof(mp3_buffer)
            );
        } else {
            res = lame_encode_buffer_interleaved(
                lame, src, this_samples, mp3_buffer, sizeof(mp3_buffer)
            );
        }
        if (res < 0) {
            fprintf(stderr, "%s: LAME encoding error (%d)\n", path, res);
            goto error_close_lame;
        }

        if (res > 0 && fwrite(mp3_buffer, 1, res, out) != res) {
            perror(mp3_pathbuf);
            goto error_close_lame;
        }

    }

    int res = lame_encode_flush(lame, mp3_buffer, sizeof(mp3_buffer));
    if (res < 0) {
        fprintf(stderr, "%s: LAME flush error (%d)\n", path, res);
        goto error_close_lame;
    }
    if (res > 0 && fwrite(mp3_buffer, 1, res, out) != res) {
        perror(mp3_pathbuf);
        goto error_close_lame;
    }

    /* Insert a Xing header at the beginning of the file so the engine
     * knows the true sample count (without padding). */

    if (fseek(out, 0, SEEK_SET) != 0) {
        perror(mp3_pathbuf);
        goto error_close_lame;
    }

    res = lame_get_lametag_frame(lame, mp3_buffer, sizeof(mp3_buffer));
    if (res < 0) {
        fprintf(stderr, "%s: LAME header genreation error (%d)\n", path, res);
        goto error_close_lame;
    }
    if (res > 0 && fwrite(mp3_buffer, 1, res, out) != res) {
        perror(mp3_pathbuf);
        goto error_close_lame;
    }

    /* All done! */

    lame_close(lame);
    free(pcm_buffer);
    ov_clear(&vf);
    fclose(out);
    fclose(in);
    return 1;

    /* Error handling. */

  error_close_lame:
    lame_close(lame);
  error_free_pcm_buffer:
    free(pcm_buffer);
  error_close_vorbis:
    ov_clear(&vf);
  error_close_out:
    fclose(out);
    remove(mp3_pathbuf);
  error_close_in:
    fclose(in);
  error_return:
    return 0;

}

/*-----------------------------------------------------------------------*/

/**
 * lame_error_callback, lame_message_callback, lame_debug_callback: LAME
 * logging callback functions.
 *
 * [Parameters]
 *     format: Format string
 *       args: Format arguments
 * [Return value]
 *     None
 */
static void lame_error_callback(const char *format, va_list args)
{
    fprintf(stderr, "LAME error: ");
    vfprintf(stderr, format, args);
}

static void lame_message_callback(const char *format, va_list args)
{
    fprintf(stderr, "LAME: ");
    vfprintf(stderr, format, args);
}

static void lame_debug_callback(const char *format, va_list args)
{
    /* Ignore these. */
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
