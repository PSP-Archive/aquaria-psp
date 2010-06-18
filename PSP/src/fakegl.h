/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/fakegl.h: Header for BBGE/Aquaria GL interface.
 */

#ifndef FAKEGL_H
#define FAKEGL_H

/*************************************************************************/

/* Most of the GL definitions are already available from GL/{gl,glext}.h in
 * the BBGE source tree, so let's include those here: */

#define GL_GLEXT_LEGACY 1
#include "../../BBGE/GL/gl.h"
#include "../../BBGE/GL/glext.h"

/* Override the double-sized functions to float-sized. */

extern void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);

#define glNormal3d(nx,ny,nz)  glNormal3f((nx), (ny), (nz))
#define glOrtho(l,r,b,t,n,f)  glOrthof((l), (r), (b), (t), (n), (f))
#define glTexCoord2d(s,t)     glTexCoord2f((s), (t))

/* Override a few other functions to reduce the amount of code we have to
 * implement. */

#define glColor3f(r,g,b)      glColor3ub((int)((r)*255), (int)((g)*255), \
                                         (int)((b)*255))
#define glColor3ub(r,g,b)     glColor4ub((r), (g), (b), 255)
#define glColor4f(r,g,b,a)    glColor4ub((int)((r)*255), (int)((g)*255), \
                                         (int)((b)*255), (int)((a)*255))
#define glVertex2f(x,y)       glVertex3f((x), (y), 0)

/* We have a few additional PSP-specific functions, defined below. */

/*-----------------------------------------------------------------------*/

/**
 * fakeglBeginFrame:  Begin drawing a new frame.  This function must be
 * called prior to any rendering functions for a given frame.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
extern void fakeglBeginFrame(void);

/**
 * fakeglBeginOffscreenFrame:  Begin drawing a new frame in an offscreen
 * framebuffer.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
extern void fakeglBeginOffscreenFrame(void);

/**
 * fakeglEndFrame:  Finish drawing the current frame, and swap it to the
 * display buffer if it is not an offscreen frame.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
extern void fakeglEndFrame(void);

/**
 * fakeglTexImagePSP:  Associate a texture loaded into a Texture structure
 * by one of the texture_*() functions with GL_TEXTURE_2D.  After this
 * call, the Texture structure is owned by the GL library, and may not be
 * used by the caller (even if this function raises an error), except that
 * operations which would be permitted on a texture pointer returned by
 * fakeglGetTexPointerPSP() may be performed on the passed-in texture
 * after this function returns.
 *
 * [Parameters]
 *      target: Target GL texture ID (must be GL_TEXTURE_2D)
 *     texture: Texture structure to associate
 * [Return value]
 *     None
 */
void fakeglTexImagePSP(GLenum target, Texture *texture);

/**
 * fakeglGetTexPointerPSP:  Retrieve the pointer to the Texture structure
 * associated with GL_TEXTURE_2D.  The caller may read any fields from the
 * returned structure, and may modify the pixel data or (if an indexed
 * texture) the color palette, but must not modify any other fields or
 * free the structure.
 *
 * [Parameters]
 *     target: Target GL texture ID (must be GL_TEXTURE_2D)
 * [Return value]
 *     Texture structure pointer, or NULL if no texture is currently
 *     associated with the target or the associated texture hs no data
 */
const Texture *fakeglGetTexPointerPSP(GLenum target);

/*************************************************************************/

#endif  // FAKEGL_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
