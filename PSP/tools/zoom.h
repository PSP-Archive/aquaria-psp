/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * tools/zoom.h: Header for image scaling routine.  Borrowed from transcode
 * (GPL2+).
 */

#ifndef TOOLS_ZOOM_H
#define TOOLS_ZOOM_H

/*************************************************************************/

/* Filter IDs for tcv_zoom(): */
typedef enum {
    TCV_ZOOM_HERMITE = 1,
    TCV_ZOOM_BOX,
    TCV_ZOOM_TRIANGLE,
    TCV_ZOOM_BELL,
    TCV_ZOOM_B_SPLINE,
    TCV_ZOOM_LANCZOS3,
    TCV_ZOOM_MITCHELL,
    TCV_ZOOM_CUBIC_KEYS4,
    TCV_ZOOM_SINC8,
} TCVZoomFilter;

/* Internal data used by zoom_process(). (opaque to caller) */
typedef struct zoominfo ZoomInfo;

/* Create a ZoomInfo structure for the given parameters. */
ZoomInfo *zoom_init(int old_w, int old_h, int new_w, int new_h, int Bpp,
                    int old_stride, int new_stride, int alpha_mode,
                    TCVZoomFilter filter);

/* The resizing function itself. */
void zoom_process(const ZoomInfo *zi, const uint8_t *src, uint8_t *dest);

/* Free a ZoomInfo structure. */
void zoom_free(ZoomInfo *zi);

/*************************************************************************/

#endif  /* TOOLS_ZOOM_H */

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
