/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * src/fakegl.c: GL interface between BBGE/Aquaria and the PSP engine's
 * graphics functionality.
 */

#include "common.h"
#include "fakegl.h"
#include "graphics.h"
#include "memory.h"
#include "texture.h"
#include "vector.h"
#include "sysdep-psp/ge-util.h"
#include "sysdep-psp/psplocal.h"

/*
 * This source code implements the subset of OpenGL necessary for Aquaria
 * to run properly, along with a few PSP-specific interface functions
 * (identifiable by a "fakegl" instead of "gl" name prefix) used to
 * improve performance.  This is _not_ intended as a general-purpose
 * OpenGL implementation.
 */

/*************************************************************************/
/***************************** Configuration *****************************/
/*************************************************************************/

/**
 * STORE_MODELVIEW_MATRIX_IN_VFPU:  If defined, the modelview matrix will
 * be cached in the VFPU matrix registers while a frame is being rendered,
 * allowing faster transformation operations.  (The projection matrix is
 * left in memory on the assumption that it will rarely be transformed.)
 * The current modelview matrix is stored in M700, and other VFPU matrices
 * are used as temporaries.  Note that the "C" and "R" vector prefixes are
 * treated to mean "row" and "column" respectively, so that the numeric
 * element indices match those of the Matrix4f structure.
 *
 * No other code may use the VFPU between fakeglBegin{,Offscreen}Frame()
 * and fakeglEndFrame() if this is defined.
 */
#define STORE_MODELVIEW_MATRIX_IN_VFPU

/*************************************************************************/
/****************************** Local data *******************************/
/*************************************************************************/

/******** Global state ********/

/* Most recent error code (returned by glGetError()). */
static GLenum last_error = GL_NO_ERROR;

/* Are we currently rendering a frame (i.e., between glBegin{,Offscreen}Frame()
 * and glEndFrame())? */
static uint8_t in_frame = 0;

/* Is the current frame being rendered to an offscreen framebuffer?  (Set
 * by calling fakeglBeginOffscreenFrame() instead of fakeglBeginFrame().) */
static uint8_t is_offscreen = 0;

/*-----------------------------------------------------------------------*/

/******** Active render state ********/

/* Current glEnable() flags. */
static uint8_t enable_alpha_test = 0;
static uint8_t enable_blend = 0;
static uint8_t enable_cull_face = 0;
static uint8_t enable_depth_test = 0;
static uint8_t enable_lighting = 0;
static uint8_t enable_light[4] = {0, 0, 0, 0};
static uint8_t enable_line_smooth = 0;
static uint8_t enable_scissor_test = 0;
static uint8_t enable_texture_2D = 0;

/* Current blending functions. */
static GLenum blend_sfactor = GL_ONE, blend_dfactor = GL_ONE;

/* Current GL_COLOR_MATERIAL state. */
static uint8_t color_material_state = 0;

/* Current clear color, as set by glClearColor(). */
static uint32_t clear_color = 0x00000000;

/* Light state for each light. */
static uint32_t light_ambient[4];
static uint32_t light_diffuse[4];
static uint32_t light_specular[4];
static Vector3f light_position[4];
static Vector3f light_direction[4];
static float light_exponent[4];
static float light_cutoff[4];

/* Scissor region (inclusive) to use when GL_SCISSOR_TEST is enabled. */
static uint16_t scissor_x0 = 0, scissor_y0 = 0;
static uint16_t scissor_x1 = DISPLAY_WIDTH-1, scissor_y1 = DISPLAY_HEIGHT-1;

/* Current viewport region. */
static uint16_t viewport_x = 0, viewport_y = 0;
static uint16_t viewport_w = DISPLAY_WIDTH, viewport_h = DISPLAY_HEIGHT;

/*-----------------------------------------------------------------------*/

/******** GL state stack (for gl{Push,Pop}Attrib()) ********/

typedef struct GLStateStack_ {

    /* glPushAttrib() flag mask for this push. */
    GLbitfield mask;

    /* glEnable() flags. */
    uint8_t enable_alpha_test;
    uint8_t enable_blend;
    uint8_t enable_cull_face;
    uint8_t enable_depth_test;
    uint8_t enable_lighting;
    uint8_t enable_light[4];
    uint8_t enable_line_smooth;
    uint8_t enable_scissor_test;
    uint8_t enable_texture_2D;

    /* Blending functions. */
    GLenum blend_sfactor, blend_dfactor;

    /* GL_COLOR_MATERIAL state. */
    uint8_t color_material_state;

    /* Clear color, as set by glClearColor(). */
    uint32_t clear_color;

    /* Light state for each light. */
    uint32_t light_ambient[4];
    uint32_t light_diffuse[4];
    uint32_t light_specular[4];
    Vector3f light_position[4];
    Vector3f light_direction[4];
    float light_exponent[4];
    float light_cutoff[4];

    /* Scissor region (inclusive) to use when GL_SCISSOR_TEST is enabled. */
    uint16_t scissor_x0, scissor_x1, scissor_y0, scissor_y1;

    /* Viewport region. */
    uint16_t viewport_x, viewport_y;
    uint16_t viewport_w, viewport_h;

    /* Matrix mode, as specified by glMatrixMode(). */
    GLenum matrix_mode;

    /* Texture ID currently bound to GL_TEXTURE_2D, or 0 for none. */
    uint32_t bound_texture;

    /* Texture settings. */
    uint8_t texture_mag_filter;
    uint8_t texture_min_filter;
    uint8_t texture_mip_filter;
    uint8_t texture_wrap_u;
    uint8_t texture_wrap_v;

     /* Current color, texture coordinates, and normal coordinates. */
    uint32_t current_color;
    float current_u, current_v;
    float current_nx, current_ny, current_nz;

} GLStateStack;

static GLStateStack state_stack[16];
static unsigned int state_stack_top = 0;

/*-----------------------------------------------------------------------*/

/******** Transformation matrix management ********/

/* Current matrix mode, as specified by glMatrixMode(). */
static GLenum matrix_mode = GL_MODELVIEW;

/* Matrix stacks. */
static Matrix4f projection_matrix_stack[2] = {
    {.m = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}}
};
static Matrix4f modelview_matrix_stack[32] = {
    {.m = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}}
};
static unsigned int projection_matrix_top = 0, modelview_matrix_top = 0;

/* Flags indicating whether either of the matrices has been changed since
 * the last primitive drawn. */
static uint8_t projection_matrix_changed = 0, modelview_matrix_changed = 0;

/* Pointer to the current matrix (the top matrix on the current matrix
 * mode's stack). */
static Matrix4f *current_matrix = &modelview_matrix_stack[0];

/* Temporary matrices for multiplication in the transformation functions.
 * (We need these declared static because GCC isn't capable of aligning
 * the stack.  See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=16660) */
static Matrix4f temp_matrix1, temp_matrix2;

/* Constant identity matrix, to simplify matrix initialization. */
static const Matrix4f identity_matrix =
    {.m = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}};

/*-----------------------------------------------------------------------*/

/******** Texture management ********/

/* Dynamically-resized array of textures associated with texture IDs.
 * Texture ID 0 is never used.  The "to_free" and "next_free" fields are
 * used when deleting textures; since the texture data may still be in use
 * by the GE's rendering pipeline, deleted textures are not destroyed until
 * the end of the frame. */
typedef struct TextureInfo_ {
    Texture *texture;
    uint32_t to_free:1;     // 1 = Texture has been deleted and is awaiting
                            //     destruction
    uint32_t next_free:31;  // Index of next texture to free, or 0 if none
} TextureInfo;
static TextureInfo *texture_array = NULL;
static uint32_t texture_array_size = 0;
static uint32_t first_texture_to_free = 0;  // 0 = none

/* Value used in texture_array[] to indicate that an ID has been allocated
 * but the texture itself has not yet been created. */
#define UNDEFINED_TEXTURE  ((Texture *)-1)

/* Texture ID currently bound to GL_TEXTURE_2D, or 0 for none. */
static uint32_t bound_texture = 0;

/* Flag indicating whether the bound texture has changed since last used
 * for rendering. */
static uint8_t texture_changed = 0;

/* Various texture settings which we have to buffer because we need to
 * set them simultaneously on the PSP but GL sets them separately. */
static uint8_t texture_mag_filter = GE_TEXFILTER_LINEAR;
static uint8_t texture_min_filter = GE_TEXFILTER_NEAREST;
static uint8_t texture_mip_filter = GE_TEXMIPFILTER_LINEAR;
static uint8_t texture_filter_changed = 0;
static uint8_t texture_wrap_u = GE_TEXWRAPMODE_REPEAT;
static uint8_t texture_wrap_v = GE_TEXWRAPMODE_REPEAT;
static uint8_t texture_wrap_mode_changed = 0;

/*-----------------------------------------------------------------------*/

/******** Primitive rendering and vertex management ********/

/* The primitive currently being generated by a glBegin()/glEnd() pair
 * (0 if not between glBegin() and glEnd()). */
static GLenum current_primitive = 0;

/* The GE primitive (GE_PRIMITIVE_*) to be used for drawing the current
 * GL primitive. */
static unsigned int ge_primitive;

/* The number of vertices seen so far. */
static unsigned int num_vertices;

/* The GE vertex format flags for this glBegin()/glEnd() pair.  These are
 * set based on the functions called between glBegin() and the first
 * glVertex(), on the assumption that subsequent vertices will use the
 * same format. */
static uint32_t vertex_format;

/* The number of words in the current vertex format, set at the time the
 * first vertex after a glBegin() is registered. */
static uint32_t vertex_words;

/* The current color, texture coordinates, and normal coordinates. */
static uint32_t current_color = 0xFFFFFFFF;
static float current_u, current_v;
static float current_nx, current_ny, current_nz;

/* Vertex data type, allowing us to store both ints and floats. */
typedef union VertexData_ {
    uint32_t i;
    float f;
} VertexData;

/* Pointer to the first vertex stored for the current primitive. */
static VertexData *first_vertex;

/* Single-vertex buffer used by GL_QUADS to swap the order of the third and
 * fourth vertices for drawing as a triangle strip. */
static uint32_t quad_saved_color;
static float quad_saved_u, quad_saved_v;
static float quad_saved_nx, quad_saved_ny, quad_saved_nz;
static float quad_saved_x, quad_saved_y, quad_saved_z;

/*----------------------------------*/

/* The number of vertices sent to the GE since the last ge_commit() call. */
static unsigned int uncached_vertices;

/* The number of uncached vertices at which we issue a ge_commit() call
 * to kickstart the GE's processing. */
#define UNCACHED_VERTEX_LIMIT  100

/*-----------------------------------------------------------------------*/

/******** Display list management ********/

/* Vertex buffer structure.  Since we don't know ahead of time how many
 * vertices will be registered between a glBegin() and glEnd() call, and
 * we can't (easily) move vertex buffers in memory because of the pointers
 * stored in the GE command words, we give each GL display list a linked
 * list of fixed-size vertex buffers, allocating a new buffer when the
 * previous one is full and freeing all buffers when the display list is
 * destroyed. */
typedef struct VertexBuffer_ VertexBuffer;
struct VertexBuffer_ {
    VertexBuffer *next; // Next vertex buffer in list
    uint32_t size;      // Size of this vertex buffer (in bytes)
    uint32_t used;      // Number of bytes used in this vertex buffer
    uint8_t data[0];    // Actual vertex data
};

/* Size of a single vertex buffer (in bytes).  For efficiency, this should
 * be large enough both to minimize the allocation overhead ratio and to
 * fit all vertices from any single glBegin()/glEnd() pair. */
#define VERTEX_BUFFER_SIZE  16384

/* Dynamically-resized array of display lists associated with list IDs.
 * List ID 0 is never used.  As with textures, display lists are not
 * deleted immediately, and are instead linked using the "to_free" and
 * "next_free" fields for delayed freeing. */
typedef struct DisplayList_ {
    uint32_t *list;         // List address pointer
    VertexBuffer *vertex_buffers; // Vertex buffer list
    uint32_t size;          // List size (in words, not bytes)
    uint32_t to_free:1;     // 1 = List has been deleted and is awaiting
                            //     destruction
    uint32_t in_use:1;      // 1 = List has been allocated with glNewLists()
    uint32_t next_free:30;  // Index of next list to free, or 0 if none
} DisplayList;
static DisplayList *dlist_array = NULL;
static uint32_t dlist_array_size = 0;
static uint32_t first_dlist_to_free = 0;  // 0 = none

/* ID of display list currently being generated, or 0 for none. */
static uint32_t current_dlist;

/* Saved modelview matrix (see glNewList()). */
static Matrix4f dlist_saved_matrix;
/* Saved modelview matrix stack pointer (see glNewList()). */
static unsigned int dlist_saved_matrix_top;

/* Memory allocation size increment for display lists (in words). */
#define DLIST_SIZE_INCREMENT  4096

/* Free space threshold (in words) below which we expand the list buffer. */
#define DLIST_EXPAND_THRESHOLD  100

/*************************************************************************/

/******** Error-setting macro including a debug log message ********/

#define SET_ERROR(error)  do {          \
    last_error = (error);               \
    DMSG("GL error: %s", #error);       \
} while (0)

/*-----------------------------------------------------------------------*/

/******** Internal helper routine/macro declarations ********/

/* Always call this by the check_dlist_size() macro, not directly. */
#define check_dlist_size() \
    do {if (current_dlist) _check_dlist_size(current_dlist);} while (0)
static void _check_dlist_size(unsigned int dlist);

static void *dlist_reserve_vertexbytes(uint32_t bytes);
#define reserve_vertexbytes(bytes) \
    (current_dlist ? dlist_reserve_vertexbytes((bytes)) \
                   : ge_reserve_vertexbytes((bytes)))

static void update_render_state(void);

static void copy_indexed(const void *data, Texture *texture,
                         unsigned int x0, unsigned int y0,
                         unsigned int width, unsigned int height);
static void copy_RGB(const void *data, Texture *texture,
                     unsigned int x0, unsigned int y0,
                     unsigned int width, unsigned int height);
static void copy_RGBA(const void *data, Texture *texture,
                      unsigned int x0, unsigned int y0,
                      unsigned int width, unsigned int height);

static void fb_to_luminance(unsigned int x0, unsigned int y0,
                            unsigned int width, unsigned int height,
                            uint8_t *dest, unsigned int dest_stride,
                            int vflip, int swizzle);
static void fb_to_RGB(unsigned int x0, unsigned int y0,
                      unsigned int width, unsigned int height,
                      uint8_t *dest, unsigned int dest_stride, int vflip);
static void fb_to_RGBA(unsigned int x0, unsigned int y0,
                       unsigned int width, unsigned int height,
                       uint32_t *dest, unsigned int dest_stride, int vflip,
                       int swizzle);

/*************************************************************************/
/*************************** General operations **************************/
/*************************************************************************/

GLenum glGetError(void)
{
    GLenum retval = last_error;
    last_error = GL_NO_ERROR;
    return retval;
}

/*************************************************************************/
/************************** Render state control *************************/
/*************************************************************************/

void glPushAttrib(GLbitfield mask)
{
    check_dlist_size();

    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (state_stack_top >= lenof(state_stack)) {
        SET_ERROR(GL_STACK_OVERFLOW);
        return;
    }

    state_stack[state_stack_top].mask = mask;

    #define PUSH(x)  (state_stack[state_stack_top].x = x)

    if (mask & GL_COLOR_BUFFER_BIT) {
        PUSH(enable_alpha_test);
        PUSH(enable_blend);
        PUSH(blend_sfactor);
        PUSH(blend_dfactor);
        PUSH(clear_color);
    }

    if (mask & GL_CURRENT_BIT) {
        PUSH(current_color);
        PUSH(current_u);
        PUSH(current_v);
        PUSH(current_nx);
        PUSH(current_ny);
        PUSH(current_nz);
    }

    if (mask & GL_DEPTH_BUFFER_BIT) {
        PUSH(enable_depth_test);
    }

    if (mask & GL_ENABLE_BIT) {
        PUSH(enable_alpha_test);
        PUSH(enable_blend);
        PUSH(color_material_state);
        PUSH(enable_cull_face);
        PUSH(enable_depth_test);
        PUSH(enable_lighting);
        unsigned int light;
        for (light = 0; light < 4; light++) {
            PUSH(enable_light[light]);
        }
        PUSH(enable_line_smooth);
        PUSH(enable_scissor_test);
        PUSH(enable_texture_2D);
    }

    if (mask & GL_LIGHTING_BIT) {
        PUSH(color_material_state);
        PUSH(enable_lighting);
        unsigned int light;
        for (light = 0; light < 4; light++) {
            PUSH(enable_light[light]);
            PUSH(light_ambient[light]);
            PUSH(light_diffuse[light]);
            PUSH(light_specular[light]);
            PUSH(light_position[light]);
            PUSH(light_direction[light]);
            PUSH(light_exponent[light]);
            PUSH(light_cutoff[light]);
        }
    }

    if (mask & GL_LINE_BIT) {
        PUSH(enable_line_smooth);
    }

    if (mask & GL_POLYGON_BIT) {
        PUSH(enable_cull_face);
    }

    if (mask & GL_SCISSOR_BIT) {
        PUSH(enable_scissor_test);
        PUSH(scissor_x0);
        PUSH(scissor_y0);
        PUSH(scissor_x1);
        PUSH(scissor_y1);
    }

    if (mask & GL_TEXTURE_BIT) {
        PUSH(bound_texture);
        PUSH(texture_mag_filter);
        PUSH(texture_min_filter);
        PUSH(texture_mip_filter);
        PUSH(texture_wrap_u);
        PUSH(texture_wrap_v);
    }

    if (mask & GL_TRANSFORM_BIT) {
        PUSH(matrix_mode);
    }

    if (mask & GL_VIEWPORT_BIT) {
        PUSH(viewport_x);
        PUSH(viewport_y);
        PUSH(viewport_w);
        PUSH(viewport_h);
    }

    #undef PUSH

    state_stack_top++;
}

/*-----------------------------------------------------------------------*/

void glPopAttrib(void)
{
    check_dlist_size();

    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (state_stack_top == 0) {
        SET_ERROR(GL_STACK_UNDERFLOW);
        return;
    }

    state_stack_top--;
    const GLbitfield mask = state_stack[state_stack_top].mask;

    #define POP(x)  (x = state_stack[state_stack_top].x)
    #define POP_ENABLE(x,state) \
        (state_stack[state_stack_top].enable_##x ? glEnable(GL_##state) \
                                                 : glDisable(GL_##state))

    if (mask & GL_COLOR_BUFFER_BIT) {
        POP_ENABLE(alpha_test, ALPHA_TEST);
        POP_ENABLE(blend, BLEND);
        glBlendFunc(state_stack[state_stack_top].blend_sfactor,
                    state_stack[state_stack_top].blend_dfactor);
        POP(clear_color);
    }

    if (mask & GL_CURRENT_BIT) {
        POP(current_color);
        POP(current_u);
        POP(current_v);
        POP(current_nx);
        POP(current_ny);
        POP(current_nz);
    }

    if (mask & GL_DEPTH_BUFFER_BIT) {
        POP_ENABLE(depth_test, DEPTH_TEST);
    }

    if (mask & GL_ENABLE_BIT) {
        POP_ENABLE(alpha_test, ALPHA_TEST);
        POP_ENABLE(blend, BLEND);
        POP(color_material_state);
        POP_ENABLE(cull_face, CULL_FACE);
        POP_ENABLE(depth_test, DEPTH_TEST);
        POP_ENABLE(lighting, LIGHTING);
        unsigned int light;
        for (light = 0; light < 4; light++) {
            POP_ENABLE(light[light], LIGHT0 + light);
        }
        POP_ENABLE(line_smooth, LINE_SMOOTH);
        POP_ENABLE(scissor_test, SCISSOR_TEST);
        POP_ENABLE(texture_2D, TEXTURE_2D);
    }

    if (mask & GL_LIGHTING_BIT) {
        POP(color_material_state);
        POP_ENABLE(lighting, LIGHTING);
        unsigned int light;
        for (light = 0; light < 4; light++) {
            POP_ENABLE(light[light], LIGHT0 + light);
            POP(light_ambient[light]);
            ge_set_light_color(light, GE_LIGHT_COMPONENT_AMBIENT,
                               light_ambient[light]);
            POP(light_diffuse[light]);
            ge_set_light_color(light, GE_LIGHT_COMPONENT_DIFFUSE,
                               light_ambient[light]);
            POP(light_specular[light]);
            ge_set_light_color(light, GE_LIGHT_COMPONENT_SPECULAR,
                               light_ambient[light]);
            POP(light_position[light]);
            ge_set_light_position(light, light_position[light].x,
                                         light_position[light].y,
                                         light_position[light].z);
            POP(light_direction[light]);
            ge_set_light_direction(light, light_direction[light].x,
                                          light_direction[light].y,
                                          light_direction[light].z);
            POP(light_exponent[light]);
            ge_set_spotlight_exponent(light, light_exponent[light]);
            POP(light_cutoff[light]);
            ge_set_spotlight_cutoff(light, light_cutoff[light]);
        }
    }

    if (mask & GL_LINE_BIT) {
        POP_ENABLE(line_smooth, LINE_SMOOTH);
    }

    if (mask & GL_POLYGON_BIT) {
        POP_ENABLE(cull_face, CULL_FACE);
    }

    if (mask & GL_SCISSOR_BIT) {
        POP(scissor_x0);
        POP(scissor_y0);
        POP(scissor_x1);
        POP(scissor_y1);
        POP_ENABLE(scissor_test, SCISSOR_TEST);
    }

    if (mask & GL_TEXTURE_BIT) {
        POP(bound_texture);
        if (bound_texture > 0
         && (!texture_array[bound_texture].texture
             || texture_array[bound_texture].texture == UNDEFINED_TEXTURE)
        ) {
            bound_texture = 0;
        }
        texture_changed = 1;
        POP(texture_mag_filter);
        POP(texture_min_filter);
        POP(texture_mip_filter);
        texture_filter_changed = 1;
        POP(texture_wrap_u);
        POP(texture_wrap_v);
        texture_wrap_mode_changed = 1;
    }

    if (mask & GL_TRANSFORM_BIT) {
        glMatrixMode(state_stack[state_stack_top].matrix_mode);
    }

    if (mask & GL_VIEWPORT_BIT) {
        POP(viewport_x);
        POP(viewport_y);
        POP(viewport_w);
        POP(viewport_h);
        ge_set_viewport(viewport_x, viewport_y, viewport_w, viewport_h);
    }

    #undef POP
    #undef POP_ENABLE
}

/*************************************************************************/

void glPushClientAttrib(GLbitfield mask)
{
    if (mask != GL_CLIENT_PIXEL_STORE_BIT) {
        DMSG("Invalid/unsupported mask 0x%X", (int)mask);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    /* We don't actually support changing the pixel transfer/storage
     * attributes, so there's nothing to do here. */
}

/*-----------------------------------------------------------------------*/

void glPopClientAttrib(void)
{
    /* Nothing to do (see glPushClientAttrib()). */
}

/*************************************************************************/

void glEnable(GLenum cap)
{
    check_dlist_size();

    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    switch (cap) {
      case GL_ALPHA_TEST:
        ge_enable(GE_STATE_ALPHA_TEST);
        enable_alpha_test = 1;
        break;
      case GL_BLEND:
        ge_enable(GE_STATE_BLEND);
        enable_blend = 1;
        break;
      case GL_COLOR_MATERIAL:
        color_material_state = 1;
        break;
      case GL_CULL_FACE:
        /* Aquaria never calls glFrontFace() or glCullFace(), so we can
         * just set the default of culling back (clockwise) faces. */
        ge_set_cull_mode(GE_CULL_CW);
        enable_cull_face = 1;
        break;
      case GL_DEPTH_TEST:
        ge_enable(GE_STATE_DEPTH_TEST);
        enable_depth_test = 1;
        break;
      case GL_LIGHTING:
        ge_enable(GE_STATE_LIGHTING);
        enable_lighting = 1;
        break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
        ge_enable_light(cap - GL_LIGHT0);
        enable_light[cap - GL_LIGHT0] = 1;
        break;
      case GL_LINE_SMOOTH:
        ge_enable(GE_STATE_ANTIALIAS); //FIXME: is this right? (also glDisable)
        enable_line_smooth = 1;
        break;
      case GL_SCISSOR_TEST:
        ge_set_clip_area(scissor_x0, scissor_y0, scissor_x1, scissor_y1);
        enable_scissor_test = 1;
        break;
      case GL_TEXTURE_2D:
        ge_enable(GE_STATE_TEXTURE);
        enable_texture_2D = 1;
        break;
      default:
        DMSG("Invalid/unsupported capability 0x%X", (int)cap);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }
}

/*-----------------------------------------------------------------------*/

void glDisable(GLenum cap)
{
    check_dlist_size();

    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    switch (cap) {
      case GL_ALPHA_TEST:
        ge_disable(GE_STATE_ALPHA_TEST);
        enable_alpha_test = 0;
        break;
      case GL_BLEND:
        ge_disable(GE_STATE_BLEND);
        enable_blend = 0;
        break;
      case GL_COLOR_MATERIAL:
        color_material_state = 0;
        break;
      case GL_CULL_FACE:
        ge_set_cull_mode(GE_CULL_NONE);
        enable_cull_face = 0;
        break;
      case GL_DEPTH_TEST:
        ge_disable(GE_STATE_DEPTH_TEST);
        enable_depth_test = 0;
        break;
      case GL_LIGHTING:
        ge_disable(GE_STATE_LIGHTING);
        enable_lighting = 0;
        break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
        ge_disable_light(cap - GL_LIGHT0);
        enable_light[cap - GL_LIGHT0] = 0;
        break;
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
        /* We only support four light sources, but since disabling an
         * unavailable light is a no-op, don't complain. */
        break;
      case GL_LINE_SMOOTH:
        ge_disable(GE_STATE_ANTIALIAS);
        enable_line_smooth = 0;
        break;
      case GL_SCISSOR_TEST:
        ge_unset_clip_area();
        enable_scissor_test = 0;
        break;
      case GL_TEXTURE_2D:
        ge_disable(GE_STATE_TEXTURE);
        enable_texture_2D = 0;
        break;
      case GL_DITHER:
      case GL_FOG:
      case GL_INDEX_LOGIC_OP:
      case GL_STENCIL_TEST:
      case GL_TEXTURE_1D:
        /* These are all cleared when capturing a screenshot for a save
         * file, so don't warn about trying to disable them. */
        break;
      default:
        DMSG("Invalid/unsupported capability 0x%X", (int)cap);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }
}

/*************************************************************************/

void glGetFloatv(GLenum pname, GLfloat *params)
{
    if (!params) {
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }

    switch (pname) {
      case GL_CURRENT_RASTER_COLOR:
        params[0] = (current_color>> 0 & 0xFF) / 255.0f;
        params[1] = (current_color>> 8 & 0xFF) / 255.0f;
        params[2] = (current_color>>16 & 0xFF) / 255.0f;
        params[3] = (current_color>>24 & 0xFF) / 255.0f;
        break;
      case GL_MODELVIEW_MATRIX:
        if (current_dlist) {
            DMSG("WARNING: glGetFloat(GL_MODELVIEW_MATRIX) is unreliable"
                 " in a display list");
        }
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
        asm(".set push; .set noreorder\n"
            "usv.q C700,  0(%[matrix])\n"
            "usv.q C710, 16(%[matrix])\n"
            "usv.q C720, 32(%[matrix])\n"
            "usv.q C730, 48(%[matrix])\n"
            ".set pop"
            : /* no outputs */
            : [matrix] "r" (params)
        );
#else
        memcpy(params, &modelview_matrix_stack[modelview_matrix_top], 16*4);
#endif
        break;
      default:
        DMSG("Invalid/unsupported parameter 0x%X", (int)pname);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }
}

/*-----------------------------------------------------------------------*/

void glGetIntegerv(GLenum pname, GLint *params)
{
    if (!params) {
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }

    switch (pname) {
      case GL_VIEWPORT:
        params[0] = viewport_x;
        params[1] = viewport_y;
        params[2] = viewport_w;
        params[3] = viewport_h;
        break;
      case GL_MAX_TEXTURE_SIZE:
        params[0] = 512;
        break;
      default:
        DMSG("Invalid/unsupported parameter 0x%X", (int)pname);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }
}

/*************************************************************************/

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    check_dlist_size();

    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (sfactor == blend_sfactor && dfactor == blend_dfactor) {
        return;  // No change from current state.
    }

    unsigned int ge_sfactor, ge_dfactor;
    uint32_t ge_sfix = 0, ge_dfix = 0;

    switch (sfactor) {
      case GL_ZERO:
        ge_sfactor = GE_BLEND_FIX;
        ge_sfix = 0x000000;
        break;
      case GL_ONE:
        ge_sfactor = GE_BLEND_FIX;
        ge_sfix = 0xFFFFFF;
        break;
      case GL_SRC_COLOR:
        ge_sfactor = GE_BLEND_COLOR;
        break;
      case GL_ONE_MINUS_SRC_COLOR: 
        ge_sfactor = GE_BLEND_ONE_MINUS_COLOR;
        break;
      case GL_SRC_ALPHA:
        ge_sfactor = GE_BLEND_SRC_ALPHA;
        break;
      case GL_ONE_MINUS_SRC_ALPHA:
        ge_sfactor = GE_BLEND_ONE_MINUS_SRC_ALPHA;
        break;
      case GL_DST_ALPHA:
        ge_sfactor = GE_BLEND_DST_ALPHA;
        break;
      case GL_ONE_MINUS_DST_ALPHA:
        ge_sfactor = GE_BLEND_ONE_MINUS_DST_ALPHA;
        break;
      default:
        DMSG("Source function 0x%X not supported", (int)sfactor);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }

    switch (dfactor) {
      case GL_ZERO:
        ge_dfactor = GE_BLEND_FIX;
        ge_dfix = 0x000000;
        break;
      case GL_ONE:
        ge_dfactor = GE_BLEND_FIX;
        ge_dfix = 0xFFFFFF;
        break;
      case GL_SRC_COLOR:
        ge_dfactor = GE_BLEND_COLOR;
        break;
      case GL_ONE_MINUS_SRC_COLOR: 
        ge_dfactor = GE_BLEND_ONE_MINUS_COLOR;
        break;
      case GL_SRC_ALPHA:
        ge_dfactor = GE_BLEND_SRC_ALPHA;
        break;
      case GL_ONE_MINUS_SRC_ALPHA:
        ge_dfactor = GE_BLEND_ONE_MINUS_SRC_ALPHA;
        break;
      case GL_DST_ALPHA:
        ge_dfactor = GE_BLEND_DST_ALPHA;
        break;
      case GL_ONE_MINUS_DST_ALPHA:
        ge_dfactor = GE_BLEND_ONE_MINUS_DST_ALPHA;
        break;
      default:
        DMSG("Destination function 0x%X not supported", (int)dfactor);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }

    /* Aquaria never calls glBlendEquation(), so we can just set the
     * default GE_BLEND_ADD. */
    ge_set_blend_mode(GE_BLEND_ADD, ge_sfactor, ge_dfactor, ge_sfix, ge_dfix);

    blend_sfactor = sfactor;
    blend_dfactor = dfactor;
}

/*************************************************************************/

void glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    check_dlist_size();

    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (light < GL_LIGHT0 || light > GL_LIGHT3) {
        DMSG("Invalid light source 0x%X", (int)light);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }
    light -= GL_LIGHT0;

    switch (pname) {

      case GL_AMBIENT:
        if (params[3] != 1.0f) {
            DMSG("Ambient alpha %.3f not supported", params[3]);
        }
        light_ambient[light] = iroundf(params[0]*255) <<  0
                             | iroundf(params[1]*255) <<  8
                             | iroundf(params[2]*255) << 16;
        ge_set_light_color(light, GE_LIGHT_COMPONENT_AMBIENT,
                           light_ambient[light]);
        break;

      case GL_DIFFUSE:
        if (params[3] != 1.0f) {
            DMSG("Diffuse alpha %.3f not supported", params[3]);
        }
        light_diffuse[light] = iroundf(params[0]*255) <<  0
                             | iroundf(params[1]*255) <<  8
                             | iroundf(params[2]*255) << 16;
        ge_set_light_color(light, GE_LIGHT_COMPONENT_DIFFUSE,
                           light_diffuse[light]);
        break;

      case GL_SPECULAR:
        if (params[3] != 1.0f) {
            DMSG("Specular alpha %.3f not supported", params[3]);
        }
        light_specular[light] = iroundf(params[0]*255) <<  0
                              | iroundf(params[1]*255) <<  8
                              | iroundf(params[2]*255) << 16;
        ge_set_light_color(light, GE_LIGHT_COMPONENT_SPECULAR,
                           light_specular[light]);
        break;

      case GL_POSITION:
        /* BBGE only uses directional lights. */
        if (params[3] != 0.0f) {
            DMSG("W position %.3f not supported", params[3]);
        }
        ge_set_light_type(light, GE_LIGHT_TYPE_DIRECTIONAL, 0);
        ge_set_light_position(light, params[0], params[1], params[2]);
        light_position[light].x = params[0];
        light_position[light].y = params[1];
        light_position[light].z = params[2];
        break;

      case GL_SPOT_DIRECTION:
        ge_set_light_direction(light, params[0], params[1], params[2]);
        light_direction[light].x = params[0];
        light_direction[light].y = params[1];
        light_direction[light].z = params[2];
        break;

      case GL_SPOT_EXPONENT:
        ge_set_spotlight_exponent(light, params[0]);
        light_exponent[light] = params[0];
        break;

      case GL_SPOT_CUTOFF:
        ge_set_spotlight_cutoff(light, params[0]);
        light_cutoff[light] = params[0];
        break;

      default:
        DMSG("Invalid/unsupported parameter 0x%X", (int)pname);
        SET_ERROR(GL_INVALID_ENUM);
        return;

    }  // switch (pname)
}

/*************************************************************************/

void glLineWidth(GLfloat width)
{
    if (width != 1) {
        // FIXME: Disable this for now to stop it from complaining every frame.
        //DMSG("Line width %.3f not supported", width);
    }
}

/*-----------------------------------------------------------------------*/

void glPointSize(GLfloat size)
{
    if (size != 1) {
        DMSG("Point size %.3f not supported", size);
    }
}

/*************************************************************************/

void glPixelStorei(GLenum pname, GLint param)
{
    switch (pname) {
      case GL_PACK_ALIGNMENT:
        if (param != 1) {
            DMSG("GL_PACK_ALIGNMENT(%d) not supported", (int)param);
        }
        break;
      case GL_UNPACK_ALIGNMENT:
        if (param != 1) {
            DMSG("GL_UNPACK_ALIGNMENT(%d) not supported", (int)param);
        }
        break;
      case GL_UNPACK_ROW_LENGTH:
        if (param != 0) {
            DMSG("GL_UNPACK_ROW_LENGTH(%d) not supported", (int)param);
        }
        break;
      case GL_UNPACK_LSB_FIRST:
        if (param) {
            DMSG("GL_UNPACK_LSB_FIRST(true) not supported");
        }
        break;
      default:
        DMSG("Invalid/unsupported parameter 0x%X", (int)pname);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }
}

/*-----------------------------------------------------------------------*/

void glPixelTransferi(GLenum pname, GLint param)
{
    switch (pname) {
      case GL_MAP_COLOR:
        if (param) {
            DMSG("GL_MAP_COLOR(true) not supported");
        }
        break;
      case GL_RED_SCALE:
        if (param != 1) {
            DMSG("GL_RED_SCALE(%d) not supported", (int)param);
        }
        break;
      case GL_GREEN_SCALE:
        if (param != 1) {
            DMSG("GL_GREEN_SCALE(%d) not supported", (int)param);
        }
        break;
      case GL_BLUE_SCALE:
        if (param != 1) {
            DMSG("GL_BLUE_SCALE(%d) not supported", (int)param);
        }
        break;
      case GL_ALPHA_SCALE:
        if (param != 1) {
            DMSG("GL_ALPHA_SCALE(%d) not supported", (int)param);
        }
        break;
      case GL_RED_BIAS:
        if (param != 0) {
            DMSG("GL_RED_BIAS(%d) not supported", (int)param);
        }
        break;
      case GL_GREEN_BIAS:
        if (param != 0) {
            DMSG("GL_GREEN_BIAS(%d) not supported", (int)param);
        }
        break;
      case GL_BLUE_BIAS:
        if (param != 0) {
            DMSG("GL_BLUE_BIAS(%d) not supported", (int)param);
        }
        break;
      case GL_ALPHA_BIAS:
        if (param != 0) {
            DMSG("GL_ALPHA_BIAS(%d) not supported", (int)param);
        }
        break;
      default:
        DMSG("Invalid/unsupported parameter 0x%X", (int)pname);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }
}

/*-----------------------------------------------------------------------*/

void glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    /* BBGE only uses this function for generating resized screenshots,
     * which we won't support on the PSP. */
    if (xfactor != 1 || yfactor != 1) {
        DMSG("Pixel zoom factor %.3f,%.3f not supported", xfactor, yfactor);
    }
}

/*************************************************************************/

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    check_dlist_size();

    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    viewport_x = x;
    viewport_y = y;
    viewport_w = width;
    viewport_h = height;
    ge_set_viewport(x, y, width, height);
}

/*************************************************************************/
/******************* Transformation matrix manipulation ******************/
/*************************************************************************/

void glMatrixMode(GLenum mode)
{
    if (mode != GL_PROJECTION && mode != GL_MODELVIEW) {
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }

    matrix_mode = mode;
    switch (matrix_mode) {
      case GL_PROJECTION:
        if (current_dlist) {
            DMSG("Setting projection matrix not allowed in a display list");
            SET_ERROR(GL_INVALID_OPERATION);
            return;
        }
        current_matrix = &projection_matrix_stack[projection_matrix_top];
        break;
      case GL_MODELVIEW:
        current_matrix = &modelview_matrix_stack[modelview_matrix_top];
        break;
    }
}

/*-----------------------------------------------------------------------*/

void glLoadMatrixf(const GLfloat *m)
{
    if (current_dlist && modelview_matrix_top == dlist_saved_matrix_top) {
        DMSG("WARNING: display list set matrix without push");
    }

#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
    if (matrix_mode == GL_MODELVIEW) {
        asm(".set push; .set noreorder\n"
            "ulv.q C700,  0(%[matrix])\n"
            "ulv.q C710, 16(%[matrix])\n"
            "ulv.q C720, 32(%[matrix])\n"
            "ulv.q C730, 48(%[matrix])\n"
            ".set pop"
            : /* no outputs */
            : [matrix] "r" (m)
        );
    } else
#endif
    {
        float *dest = &current_matrix->a[0];
        unsigned int i;
        for (i = 0; i < 16; i += 4) {
            /* Unrolled to help out the optimizer. */
            const float x = m[i+0];
            const float y = m[i+1];
            const float z = m[i+2];
            const float w = m[i+3];
            dest[i+0] = x;
            dest[i+1] = y;
            dest[i+2] = z;
            dest[i+3] = w;
        }
    }

    switch (matrix_mode) {
        case GL_PROJECTION: projection_matrix_changed = 1; break;
        case GL_MODELVIEW:  modelview_matrix_changed = 1;  break;
    }
}

/*-----------------------------------------------------------------------*/

void glLoadIdentity(void)
{
    glLoadMatrixf(identity_matrix.a);
}

/*-----------------------------------------------------------------------*/

void glPushMatrix(void)
{
    switch (matrix_mode) {
      case GL_PROJECTION:
        if (projection_matrix_top + 1 >= lenof(projection_matrix_stack)) {
            SET_ERROR(GL_STACK_OVERFLOW);
            return;
        }
        projection_matrix_top++;
        projection_matrix_stack[projection_matrix_top] = *current_matrix;
        current_matrix = &projection_matrix_stack[projection_matrix_top];
        break;

      case GL_MODELVIEW:
        if (modelview_matrix_top + 1 >= lenof(modelview_matrix_stack)) {
            SET_ERROR(GL_STACK_OVERFLOW);
            return;
        }
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
        asm(".set push; .set noreorder\n"
            "sv.q C700,  0(%[matrix])\n"
            "sv.q C710, 16(%[matrix])\n"
            "sv.q C720, 32(%[matrix])\n"
            "sv.q C730, 48(%[matrix])\n"
            ".set pop"
            : /* no outputs */
            : [matrix] "r" (&modelview_matrix_stack[modelview_matrix_top])
        );
#endif
        modelview_matrix_top++;
#ifndef STORE_MODELVIEW_MATRIX_IN_VFPU
        modelview_matrix_stack[modelview_matrix_top] = *current_matrix;
#endif
        current_matrix = &modelview_matrix_stack[modelview_matrix_top];
        break;
    }
}

/*-----------------------------------------------------------------------*/

void glPopMatrix(void)
{
    switch (matrix_mode) {
      case GL_PROJECTION:
        if (projection_matrix_top == 0) {
            SET_ERROR(GL_STACK_UNDERFLOW);
            return;
        }
        projection_matrix_top--;
        current_matrix = &projection_matrix_stack[projection_matrix_top];
        projection_matrix_changed = 1;
        break;

      case GL_MODELVIEW:
        if (modelview_matrix_top == 0) {
            SET_ERROR(GL_STACK_UNDERFLOW);
            return;
        }
        if (current_dlist && modelview_matrix_top <= dlist_saved_matrix_top) {
            DMSG("WARNING: display list tried to pop past the original stack"
                 " pointer, ignoring");
            SET_ERROR(GL_STACK_UNDERFLOW);
            return;
        }
        modelview_matrix_top--;
        current_matrix = &modelview_matrix_stack[modelview_matrix_top];
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
        asm(".set push; .set noreorder\n"
            "lv.q C700,  0(%[matrix])\n"
            "lv.q C710, 16(%[matrix])\n"
            "lv.q C720, 32(%[matrix])\n"
            "lv.q C730, 48(%[matrix])\n"
            ".set pop"
            : /* no outputs */
            : [matrix] "r" (&modelview_matrix_stack[modelview_matrix_top])
        );
#endif
        modelview_matrix_changed = 1;
        break;
    }
}

/*************************************************************************/

void glMultMatrixf(const GLfloat *m)
{
    if (current_dlist && modelview_matrix_top == dlist_saved_matrix_top) {
        DMSG("WARNING: display list set matrix without push");
    }

#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
    if (matrix_mode == GL_MODELVIEW) {
        asm(".set push; .set noreorder\n"
            "ulv.q C000,  0(%[matrix])\n"
            "ulv.q C010, 16(%[matrix])\n"
            "ulv.q C020, 32(%[matrix])\n"
            "ulv.q C030, 48(%[matrix])\n"
            "vmmov.q M100, M700\n"
            "vmmul.q M700, M000, M100\n"
            ".set pop"
            : /* no outputs */
            : [matrix] "r" (m)
        );
    } else
#endif
    {
        temp_matrix1 = *current_matrix;
        memcpy(&temp_matrix2, m, 16*4);
        mat4_mul(current_matrix, &temp_matrix2, &temp_matrix1);
    }

    switch (matrix_mode) {
        case GL_PROJECTION: projection_matrix_changed = 1; break;
        case GL_MODELVIEW:  modelview_matrix_changed = 1;  break;
    }
}

/*-----------------------------------------------------------------------*/

void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
    if (current_dlist && modelview_matrix_top == dlist_saved_matrix_top) {
        DMSG("WARNING: display list set matrix without push");
    }

    temp_matrix1 = *current_matrix;
    temp_matrix2._11 = 2.0f / (right - left);
    temp_matrix2._12 = 0;
    temp_matrix2._13 = 0;
    temp_matrix2._14 = 0;
    temp_matrix2._21 = 0;
    temp_matrix2._22 = 2.0f / (top - bottom);
    temp_matrix2._23 = 0;
    temp_matrix2._24 = 0;
    temp_matrix2._31 = 0;
    temp_matrix2._32 = 0;
    temp_matrix2._33 = -2.0f / (zFar - zNear);
    temp_matrix2._34 = 0;
    temp_matrix2._41 = -(right + left) / (right - left);
    temp_matrix2._42 = -(top + bottom) / (top - bottom);
    temp_matrix2._43 = -(zFar + zNear) / (zFar - zNear);
    temp_matrix2._44 = 1;
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
    if (matrix_mode == GL_MODELVIEW) {  // Unlikely, but just in case
        asm(".set push; .set noreorder\n"
            "lv.q C000,  0(%[matrix])\n"
            "lv.q C010, 16(%[matrix])\n"
            "lv.q C020, 32(%[matrix])\n"
            "lv.q C030, 48(%[matrix])\n"
            "vmmov.q M100, M700\n"
            "vmmul.q M700, M000, M100\n"
            ".set pop"
            : /* no outputs */
            : [matrix] "r" (&temp_matrix2)
        );
    } else
#endif
    mat4_mul(current_matrix, &temp_matrix2, &temp_matrix1);

    switch (matrix_mode) {
        case GL_PROJECTION: projection_matrix_changed = 1; break;
        case GL_MODELVIEW:  modelview_matrix_changed = 1;  break;
    }
}

/*-----------------------------------------------------------------------*/

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    if (current_dlist && modelview_matrix_top == dlist_saved_matrix_top) {
        DMSG("WARNING: display list set matrix without push");
    }

    if (angle == 0) {
        return;
    }

#ifndef STORE_MODELVIEW_MATRIX_IN_VFPU
    float c, s;
#endif
    asm(".set push; .set noreorder\n"
        "mtv %[angle], S100\n"
        "mtv %[rcp_90], S102\n"
        "vmul.s S110, S100, S102\n"
        "vone.s S103\n"
        "vadd.s S111, S110, S103\n"
        "vsin.p C120, C110\n"
#ifndef STORE_MODELVIEW_MATRIX_IN_VFPU
        "mfv %[s], S120\n"
        "mfv %[c], S121\n"
#endif
        ".set pop"
        :
#ifndef STORE_MODELVIEW_MATRIX_IN_VFPU
          [s] "=r" (s), [c] "=r" (c)
#endif
        : [angle] "r" (angle), [rcp_90] "r" (1/90.0f)
    );

    if (x == 0 && y == 0 && z == 1) {
        /* [+c +s  0  0]
         * [-s +c  0  0]
         * [ 0  0  1  0]
         * [ 0  0  0  1] */
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
        if (matrix_mode == GL_MODELVIEW) {
            asm(".set push; .set noreorder\n"
                "vmov.q C000, C120[y,x,0,0]\n"
                "vmov.q C010, C120[-x,y,0,0]\n"
                /* Save the first product row separately so we don't
                 * overwrite the source values for the second product row. */
                "vdot.t S100, C000, R700\n"
                "vdot.t S101, C000, R701\n"
                "vdot.t S102, C000, R702\n"
                /* The GE doesn't use the fourth column of the modelview
                 * matrix, so skip it.
                 * "vdot.t S103, C000, R703\n" */
                "vdot.t S710, C010, R700\n"
                "vdot.t S711, C010, R701\n"
                "vdot.t S712, C010, R702\n"
                /* Copy the saved product row back to the matrix. */
                "vmov.t C700, C100\n"
                ".set pop"
            );
        } else
#endif
        {
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
            float s, c;
            asm(".set push; .set noreorder\n"
                "mfv %[s], S120\n"
                "mfv %[c], S121\n"
                ".set pop"
                : [s] "=r" (s), [c] "=r" (c)
            );
#endif
            const float m11 = current_matrix->_11;
            const float m12 = current_matrix->_12;
            const float m13 = current_matrix->_13;
            const float m21 = current_matrix->_21;
            const float m22 = current_matrix->_22;
            const float m23 = current_matrix->_23;
            current_matrix->_11 =  m11*c + m21*s;
            current_matrix->_12 =  m12*c + m22*s;
            current_matrix->_13 =  m13*c + m23*s;
            current_matrix->_21 = -m11*s + m21*c;
            current_matrix->_22 = -m12*s + m22*c;
            current_matrix->_23 = -m13*s + m23*c;
        }

    } else if (x == 0 && y == 1 && z == 0) {
        /* [+c  0 -s  0]
         * [ 0  1  0  0]
         * [+s  0 +c  0]
         * [ 0  0  0  1] */
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
        if (matrix_mode == GL_MODELVIEW) {
            asm(".set push; .set noreorder\n"
                "vmov.q C000, C120[y,0,-x,0]\n"
                "vmov.q C020, C120[x,0,y,0]\n"
                "vdot.t S100, C000, R700\n"
                "vdot.t S101, C000, R701\n"
                "vdot.t S102, C000, R702\n"
                "vdot.t S720, C020, R700\n"
                "vdot.t S721, C020, R701\n"
                "vdot.t S722, C020, R702\n"
                "vmov.t C700, C100\n"
                ".set pop"
            );
        } else
#endif
        {
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
            float s, c;
            asm(".set push; .set noreorder\n"
                "mfv %[s], S120\n"
                "mfv %[c], S121\n"
                ".set pop"
                : [s] "=r" (s), [c] "=r" (c)
            );
#endif
            const float m11 = current_matrix->_11;
            const float m12 = current_matrix->_12;
            const float m13 = current_matrix->_13;
            const float m31 = current_matrix->_31;
            const float m32 = current_matrix->_32;
            const float m33 = current_matrix->_33;
            current_matrix->_11 = m11*c - m31*s;
            current_matrix->_12 = m12*c - m32*s;
            current_matrix->_13 = m13*c - m33*s;
            current_matrix->_31 = m11*s + m31*c;
            current_matrix->_32 = m12*s + m32*c;
            current_matrix->_33 = m13*s + m33*c;
        }

    } else if (x == 1 && y == 0 && z == 0) {
        /* [ 1  0  0  0]
         * [ 0 +c +s  0]
         * [ 0 -s +c  0]
         * [ 0  0  0  1] */
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
        if (matrix_mode == GL_MODELVIEW) {
            asm(".set push; .set noreorder\n"
                "vmov.q C010, C120[0,y,x,0]\n"
                "vmov.q C020, C120[0,-x,y,0]\n"
                "vdot.t S110, C010, R700\n"
                "vdot.t S111, C010, R701\n"
                "vdot.t S112, C010, R702\n"
                "vdot.t S720, C020, R700\n"
                "vdot.t S721, C020, R701\n"
                "vdot.t S722, C020, R702\n"
                "vmov.t C710, C110\n"
                ".set pop"
            );
        } else
#endif
        {
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
            float s, c;
            asm(".set push; .set noreorder\n"
                "mfv %[s], S120\n"
                "mfv %[c], S121\n"
                ".set pop"
                : [s] "=r" (s), [c] "=r" (c)
            );
#endif
            const float m21 = current_matrix->_21;
            const float m22 = current_matrix->_22;
            const float m23 = current_matrix->_23;
            const float m31 = current_matrix->_31;
            const float m32 = current_matrix->_32;
            const float m33 = current_matrix->_33;
            current_matrix->_21 =  m21*c + m31*s;
            current_matrix->_22 =  m22*c + m32*s;
            current_matrix->_23 =  m23*c + m33*s;
            current_matrix->_31 = -m21*s + m31*c;
            current_matrix->_32 = -m22*s + m32*c;
            current_matrix->_33 = -m23*s + m33*c;
        }

    } else {
        /* Arbitrary axis */

        const float scale = 1/sqrtf(x*x + y*y + z*z);
        x *= scale;
        y *= scale;
        z *= scale;

#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
        float s, c;
        asm(".set push; .set noreorder\n"
            "mfv %[s], S120\n"
            "mfv %[c], S121\n"
            ".set pop"
            : [s] "=r" (s), [c] "=r" (c)
        );
#endif

        temp_matrix1 = *current_matrix;
        temp_matrix2._11 = x*x*(1-c) + c;
        temp_matrix2._12 = y*x*(1-c) + z*s;
        temp_matrix2._13 = z*x*(1-c) - y*s;
        temp_matrix2._14 = 0;
        temp_matrix2._21 = x*y*(1-c) - z*s;
        temp_matrix2._22 = y*y*(1-c) + c;
        temp_matrix2._23 = z*y*(1-c) + x*s;
        temp_matrix2._24 = 0;
        temp_matrix2._31 = x*z*(1-c) + y*s;
        temp_matrix2._32 = y*z*(1-c) - x*s;
        temp_matrix2._33 = z*z*(1-c) + c;
        temp_matrix2._34 = 0;
        temp_matrix2._41 = 0;
        temp_matrix2._42 = 0;
        temp_matrix2._43 = 0;
        temp_matrix2._44 = 1;
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
        if (matrix_mode == GL_MODELVIEW) {
            asm(".set push; .set noreorder\n"
                "lv.q C000,  0(%[matrix])\n"
                "lv.q C010, 16(%[matrix])\n"
                "lv.q C020, 32(%[matrix])\n"
                "lv.q C030, 48(%[matrix])\n"
                "vmmov.q M100, M700\n"
                "vmmul.q M700, M000, M100\n"
                ".set pop"
                : /* no outputs */
                : [matrix] "r" (&temp_matrix2)
            );
        } else
#endif
        mat4_mul(current_matrix, &temp_matrix2, &temp_matrix1);
    }

    switch (matrix_mode) {
        case GL_PROJECTION: projection_matrix_changed = 1; break;
        case GL_MODELVIEW:  modelview_matrix_changed = 1;  break;
    }
}

/*-----------------------------------------------------------------------*/

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    if (current_dlist && modelview_matrix_top == dlist_saved_matrix_top) {
        DMSG("WARNING: display list set matrix without push");
    }

#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
    if (matrix_mode == GL_MODELVIEW) {
        asm(".set push; .set noreorder\n"
            "mtv %[x], S000\n"
            "mtv %[y], S011\n"
            "mtv %[z], S022\n"
            "vscl.q C700, C700, S000\n"
            "vscl.q C710, C710, S011\n"
            "vscl.q C720, C720, S022\n"
            ".set pop"
            : /* no outputs */
            : [x] "r" (x), [y] "r" (y), [z] "r" (z)
        );
    } else
#endif
    {
        if (x != 1) {
            current_matrix->_11 *= x;
            current_matrix->_12 *= x;
            current_matrix->_13 *= x;
            current_matrix->_14 *= x;
        }
        if (y != 1) {
            current_matrix->_21 *= y;
            current_matrix->_22 *= y;
            current_matrix->_23 *= y;
            current_matrix->_24 *= y;
        }
        if (z != 1) {
            current_matrix->_31 *= z;
            current_matrix->_32 *= z;
            current_matrix->_33 *= z;
            current_matrix->_34 *= z;
        }
    }

    switch (matrix_mode) {
        case GL_PROJECTION: projection_matrix_changed = 1; break;
        case GL_MODELVIEW:  modelview_matrix_changed = 1;  break;
    }
}

/*-----------------------------------------------------------------------*/

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    if (current_dlist && modelview_matrix_top == dlist_saved_matrix_top) {
        DMSG("WARNING: display list set matrix without push");
    }

#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
    if (matrix_mode == GL_MODELVIEW) {
        asm(".set push; .set noreorder\n"
            "mtv %[x], S030\n"
            "mtv %[y], S031\n"
            "mtv %[z], S032\n"
            "vone.s S033\n"
            "vdot.q S730, C030, R700\n"
            "vdot.q S731, C030, R701\n"
            "vdot.q S732, C030, R702\n"
            ".set pop"
            : /* no outputs */
            : [x] "r" (x), [y] "r" (y), [z] "r" (z)
        );
    } else
#endif
    {
        float m41 = current_matrix->_41;
        float m42 = current_matrix->_42;
        float m43 = current_matrix->_43;
        if (x != 0) {
            m41 += x * current_matrix->_11;
            m42 += x * current_matrix->_12;
            m43 += x * current_matrix->_13;
        }
        if (y != 0) {
            m41 += y * current_matrix->_21;
            m42 += y * current_matrix->_22;
            m43 += y * current_matrix->_23;
        }
        if (z != 0) {
            m41 += z * current_matrix->_31;
            m42 += z * current_matrix->_32;
            m43 += z * current_matrix->_33;
        }
        current_matrix->_41 = m41;
        current_matrix->_42 = m42;
        current_matrix->_43 = m43;
    }

    switch (matrix_mode) {
        case GL_PROJECTION: projection_matrix_changed = 1; break;
        case GL_MODELVIEW:  modelview_matrix_changed = 1;  break;
    }
}

/*************************************************************************/
/************************** Texture manipulation *************************/
/*************************************************************************/

void glGenTextures(GLsizei n, GLuint *textures)
{
    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (n < 0) {
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    uint32_t id = 1;
    GLsizei i;
    for (i = 0; i < n; i++) {
        for (; id < texture_array_size; id++) {
            if (!texture_array[id].texture) {
                break;
            }
        }
        if (id >= texture_array_size) {
            GLsizei textures_left = n - i;
            const uint32_t new_size = id + textures_left;
            TextureInfo *new_array =
                mem_realloc(texture_array, sizeof(*new_array) * new_size, 0);
            if (!new_array) {
                DMSG("Failed to realloc texture array from %u to %u entries",
                     texture_array_size, new_size);
                /* Free all the textures we allocated so far this time
                 * around before returning. */
                while (i > 0) {
                    i--;
                    texture_array[textures[i]].texture = NULL;
                    textures[i] = 0;
                }
                SET_ERROR(GL_OUT_OF_MEMORY);
                return;
            }
            texture_array = new_array;
            texture_array_size = new_size;
        }
        textures[i] = id++;
        texture_array[textures[i]].texture = UNDEFINED_TEXTURE;
        texture_array[textures[i]].to_free = 0;
    }
}

/*-----------------------------------------------------------------------*/

void glBindTexture(GLenum target, GLuint texture)
{
    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (texture >= texture_array_size) {
        DMSG("Invalid texture ID %u (limit %u)",
             (int)texture, texture_array_size);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }
    if (texture != 0
     && (!texture_array[texture].texture || texture_array[texture].to_free)
    ) {
        DMSG("Invalid texture ID %u (deleted)", (int)texture);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    bound_texture = texture;
    texture_changed = 1;
}

/*-----------------------------------------------------------------------*/

void glDeleteTextures(GLsizei n, const GLuint *textures)
{
    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (n < 0) {
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    GLsizei i;
    for (i = 0; i < n; i++) {
        if (textures[i] > 0 && textures[i] < texture_array_size) {
            if (textures[i] == bound_texture) {
                bound_texture = 0;
                texture_changed = 1;
            }
            if (!texture_array[textures[i]].to_free) {
                texture_array[textures[i]].to_free = 1;
                texture_array[textures[i]].next_free = first_texture_to_free;
                first_texture_to_free = textures[i];
            }
        }
    }
}

/*************************************************************************/

void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid *pixels)
{
    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D
     || level != 0
     || (internalformat != 3 && internalformat != 4
         && internalformat != GL_ALPHA)
     || width < 1 || width > 512
     || height < 1 || height > 512
     || border != 0
     || (format != GL_RGB && format != GL_RGBA && format != GL_ALPHA)
     || type != GL_UNSIGNED_BYTE
    ) {
        DMSG("Invalid/unsupported parameters: 0x%X %d 0x%X %d %d %d 0x%X 0x%X"
             " %p", (int)target, (int)level, (int)internalformat, (int)width,
             (int)height, (int)border, (int)format, (int)type, pixels);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    if ((internalformat == GL_ALPHA && format != GL_ALPHA)
     || (internalformat != GL_ALPHA && format == GL_ALPHA)
    ) {
        DMSG("Format mismatch: texture %s indexed, data %s indexed",
             internalformat==GL_ALPHA ? "IS" : "is NOT",
             format==GL_ALPHA ? "IS" : "is NOT");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (!bound_texture || !texture_array[bound_texture].texture) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }


    Texture *new_texture;
    if (internalformat == GL_ALPHA) {
        static const uint32_t alpha_palette[256] = {
              0<<24,  1<<24,  2<<24,  3<<24,  4<<24,  5<<24,  6<<24,  7<<24,
              8<<24,  9<<24, 10<<24, 11<<24, 12<<24, 13<<24, 14<<24, 15<<24,
             16<<24, 17<<24, 18<<24, 19<<24, 20<<24, 21<<24, 22<<24, 23<<24,
             24<<24, 25<<24, 26<<24, 27<<24, 28<<24, 29<<24, 30<<24, 31<<24,
             32<<24, 33<<24, 34<<24, 35<<24, 36<<24, 37<<24, 38<<24, 39<<24,
             40<<24, 41<<24, 42<<24, 43<<24, 44<<24, 45<<24, 46<<24, 47<<24,
             48<<24, 49<<24, 50<<24, 51<<24, 52<<24, 53<<24, 54<<24, 55<<24,
             56<<24, 57<<24, 58<<24, 59<<24, 60<<24, 61<<24, 62<<24, 63<<24,
             64<<24, 65<<24, 66<<24, 67<<24, 68<<24, 69<<24, 70<<24, 71<<24,
             72<<24, 73<<24, 74<<24, 75<<24, 76<<24, 77<<24, 78<<24, 79<<24,
             80<<24, 81<<24, 82<<24, 83<<24, 84<<24, 85<<24, 86<<24, 87<<24,
             88<<24, 89<<24, 90<<24, 91<<24, 92<<24, 93<<24, 94<<24, 95<<24,
             96<<24, 97<<24, 98<<24, 99<<24,100<<24,101<<24,102<<24,103<<24,
            104<<24,105<<24,106<<24,107<<24,108<<24,109<<24,110<<24,111<<24,
            112<<24,113<<24,114<<24,115<<24,116<<24,117<<24,118<<24,119<<24,
            120<<24,121<<24,122<<24,123<<24,124<<24,125<<24,126<<24,127<<24,
            128<<24,129<<24,130<<24,131<<24,132<<24,133<<24,134<<24,135<<24,
            136<<24,137<<24,138<<24,139<<24,140<<24,141<<24,142<<24,143<<24,
            144<<24,145<<24,146<<24,147<<24,148<<24,149<<24,150<<24,151<<24,
            152<<24,153<<24,154<<24,155<<24,156<<24,157<<24,158<<24,159<<24,
            160<<24,161<<24,162<<24,163<<24,164<<24,165<<24,166<<24,167<<24,
            168<<24,169<<24,170<<24,171<<24,172<<24,173<<24,174<<24,175<<24,
            176<<24,177<<24,178<<24,179<<24,180<<24,181<<24,182<<24,183<<24,
            184<<24,185<<24,186<<24,187<<24,188<<24,189<<24,190<<24,191<<24,
            192<<24,193<<24,194<<24,195<<24,196<<24,197<<24,198<<24,199<<24,
            200<<24,201<<24,202<<24,203<<24,204<<24,205<<24,206<<24,207<<24,
            208<<24,209<<24,210<<24,211<<24,212<<24,213<<24,214<<24,215<<24,
            216<<24,217<<24,218<<24,219<<24,220<<24,221<<24,222<<24,223<<24,
            224<<24,225<<24,226<<24,227<<24,228<<24,229<<24,230<<24,231<<24,
            232<<24,233<<24,234<<24,235<<24,236<<24,237<<24,238<<24,239<<24,
            240<<24,241<<24,242<<24,243<<24,244<<24,245<<24,246<<24,247<<24,
            248<<24,249<<24,250<<24,251<<24,252<<24,253<<24,254<<24,255<<24,
        };
        new_texture = texture_new_indexed(width, height, alpha_palette,
                                          MEM_ALLOC_TOP);
    } else {
        new_texture = texture_new(width, height, MEM_ALLOC_TOP);
    }
    if (!new_texture) {
        SET_ERROR(GL_OUT_OF_MEMORY);
        return;
    }

    if (texture_array[bound_texture].texture != UNDEFINED_TEXTURE) {
        texture_destroy(texture_array[bound_texture].texture);
    }
    texture_array[bound_texture].texture = new_texture;

    if (pixels) {
        if (format == GL_ALPHA) {
            copy_indexed(pixels, new_texture, 0, 0, width, height);
        } else if (format == GL_RGB) {
            copy_RGB(pixels, new_texture, 0, 0, width, height);
        } else {
            copy_RGBA(pixels, new_texture, 0, 0, width, height);
        }
    } else {
        if (format == GL_ALPHA) {
            mem_clear(new_texture->pixels, new_texture->stride * height);
        } else {
            mem_clear(new_texture->pixels, new_texture->stride * height * 4);
        }
    }
}

/*-----------------------------------------------------------------------*/

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                     GLint yoffset, GLsizei width, GLsizei height,
                     GLenum format, GLenum type, const GLvoid *pixels)
{
    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D
     || level != 0
     || (format != GL_RGB && format != GL_RGBA && format != GL_ALPHA)
     || type != GL_UNSIGNED_BYTE
     || pixels == NULL
    ) {
        DMSG("Invalid/unsupported parameters: 0x%X %d %d %d %d %d 0x%X 0x%X"
             " %p", (int)target, (int)level, (int)xoffset, (int)yoffset,
             (int)width, (int)height, (int)format, (int)type, pixels);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    if (!bound_texture || !texture_array[bound_texture].texture
     || texture_array[bound_texture].texture == UNDEFINED_TEXTURE
    ) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (xoffset < 0 || yoffset < 0
     || xoffset+width > texture_array[bound_texture].texture->width
     || yoffset+height > texture_array[bound_texture].texture->height
    ) {
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    if ((texture_array[bound_texture].texture->indexed && format != GL_ALPHA)
     || (!texture_array[bound_texture].texture->indexed && format == GL_ALPHA)
    ) {
        DMSG("Format mismatch: texture %s indexed, data %s indexed",
             texture_array[bound_texture].texture->indexed ? "IS" : "is NOT",
             format==GL_ALPHA ? "IS" : "is NOT");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }


    if (format == GL_ALPHA) {
        copy_indexed(pixels, texture_array[bound_texture].texture,
                     xoffset, yoffset, width, height);
    } else if (format == GL_RGB) {
        copy_RGB(pixels, texture_array[bound_texture].texture,
                 xoffset, yoffset, width, height);
    } else {
        copy_RGBA(pixels, texture_array[bound_texture].texture,
                  xoffset, yoffset, width, height);
    }
}

/*-----------------------------------------------------------------------*/

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLint border)
{
    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D
     || level != 0
     || (internalformat != GL_RGB && internalformat != GL_LUMINANCE)
     || width < 1 || width > 512
     || height < 1 || height > 512
     || border != 0
    ) {
        DMSG("Invalid/unsupported parameters: 0x%X %d 0x%X %d %d %d %d %d",
             (int)target, (int)level, (int)internalformat, (int)x, (int)y,
             (int)width, (int)height, (int)border);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    if (!bound_texture || !texture_array[bound_texture].texture) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (x < 0 || y < 0 || x+width > DISPLAY_WIDTH || y+height > DISPLAY_HEIGHT) {
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }


    Texture *new_texture;
    if (internalformat == GL_LUMINANCE) {
        static const uint32_t luminance_palette[256] = {
            0xFF000000, 0xFF010101, 0xFF020202, 0xFF030303,
            0xFF040404, 0xFF050505, 0xFF060606, 0xFF070707,
            0xFF080808, 0xFF090909, 0xFF0A0A0A, 0xFF0B0B0B,
            0xFF0C0C0C, 0xFF0D0D0D, 0xFF0E0E0E, 0xFF0F0F0F,
            0xFF101010, 0xFF111111, 0xFF121212, 0xFF131313,
            0xFF141414, 0xFF151515, 0xFF161616, 0xFF171717,
            0xFF181818, 0xFF191919, 0xFF1A1A1A, 0xFF1B1B1B,
            0xFF1C1C1C, 0xFF1D1D1D, 0xFF1E1E1E, 0xFF1F1F1F,
            0xFF202020, 0xFF212121, 0xFF222222, 0xFF232323,
            0xFF242424, 0xFF252525, 0xFF262626, 0xFF272727,
            0xFF282828, 0xFF292929, 0xFF2A2A2A, 0xFF2B2B2B,
            0xFF2C2C2C, 0xFF2D2D2D, 0xFF2E2E2E, 0xFF2F2F2F,
            0xFF303030, 0xFF313131, 0xFF323232, 0xFF333333,
            0xFF343434, 0xFF353535, 0xFF363636, 0xFF373737,
            0xFF383838, 0xFF393939, 0xFF3A3A3A, 0xFF3B3B3B,
            0xFF3C3C3C, 0xFF3D3D3D, 0xFF3E3E3E, 0xFF3F3F3F,
            0xFF404040, 0xFF414141, 0xFF424242, 0xFF434343,
            0xFF444444, 0xFF454545, 0xFF464646, 0xFF474747,
            0xFF484848, 0xFF494949, 0xFF4A4A4A, 0xFF4B4B4B,
            0xFF4C4C4C, 0xFF4D4D4D, 0xFF4E4E4E, 0xFF4F4F4F,
            0xFF505050, 0xFF515151, 0xFF525252, 0xFF535353,
            0xFF545454, 0xFF555555, 0xFF565656, 0xFF575757,
            0xFF585858, 0xFF595959, 0xFF5A5A5A, 0xFF5B5B5B,
            0xFF5C5C5C, 0xFF5D5D5D, 0xFF5E5E5E, 0xFF5F5F5F,
            0xFF606060, 0xFF616161, 0xFF626262, 0xFF636363,
            0xFF646464, 0xFF656565, 0xFF666666, 0xFF676767,
            0xFF686868, 0xFF696969, 0xFF6A6A6A, 0xFF6B6B6B,
            0xFF6C6C6C, 0xFF6D6D6D, 0xFF6E6E6E, 0xFF6F6F6F,
            0xFF707070, 0xFF717171, 0xFF727272, 0xFF737373,
            0xFF747474, 0xFF757575, 0xFF767676, 0xFF777777,
            0xFF787878, 0xFF797979, 0xFF7A7A7A, 0xFF7B7B7B,
            0xFF7C7C7C, 0xFF7D7D7D, 0xFF7E7E7E, 0xFF7F7F7F,
            0xFF808080, 0xFF818181, 0xFF828282, 0xFF838383,
            0xFF848484, 0xFF858585, 0xFF868686, 0xFF878787,
            0xFF888888, 0xFF898989, 0xFF8A8A8A, 0xFF8B8B8B,
            0xFF8C8C8C, 0xFF8D8D8D, 0xFF8E8E8E, 0xFF8F8F8F,
            0xFF909090, 0xFF919191, 0xFF929292, 0xFF939393,
            0xFF949494, 0xFF959595, 0xFF969696, 0xFF979797,
            0xFF989898, 0xFF999999, 0xFF9A9A9A, 0xFF9B9B9B,
            0xFF9C9C9C, 0xFF9D9D9D, 0xFF9E9E9E, 0xFF9F9F9F,
            0xFFA0A0A0, 0xFFA1A1A1, 0xFFA2A2A2, 0xFFA3A3A3,
            0xFFA4A4A4, 0xFFA5A5A5, 0xFFA6A6A6, 0xFFA7A7A7,
            0xFFA8A8A8, 0xFFA9A9A9, 0xFFAAAAAA, 0xFFABABAB,
            0xFFACACAC, 0xFFADADAD, 0xFFAEAEAE, 0xFFAFAFAF,
            0xFFB0B0B0, 0xFFB1B1B1, 0xFFB2B2B2, 0xFFB3B3B3,
            0xFFB4B4B4, 0xFFB5B5B5, 0xFFB6B6B6, 0xFFB7B7B7,
            0xFFB8B8B8, 0xFFB9B9B9, 0xFFBABABA, 0xFFBBBBBB,
            0xFFBCBCBC, 0xFFBDBDBD, 0xFFBEBEBE, 0xFFBFBFBF,
            0xFFC0C0C0, 0xFFC1C1C1, 0xFFC2C2C2, 0xFFC3C3C3,
            0xFFC4C4C4, 0xFFC5C5C5, 0xFFC6C6C6, 0xFFC7C7C7,
            0xFFC8C8C8, 0xFFC9C9C9, 0xFFCACACA, 0xFFCBCBCB,
            0xFFCCCCCC, 0xFFCDCDCD, 0xFFCECECE, 0xFFCFCFCF,
            0xFFD0D0D0, 0xFFD1D1D1, 0xFFD2D2D2, 0xFFD3D3D3,
            0xFFD4D4D4, 0xFFD5D5D5, 0xFFD6D6D6, 0xFFD7D7D7,
            0xFFD8D8D8, 0xFFD9D9D9, 0xFFDADADA, 0xFFDBDBDB,
            0xFFDCDCDC, 0xFFDDDDDD, 0xFFDEDEDE, 0xFFDFDFDF,
            0xFFE0E0E0, 0xFFE1E1E1, 0xFFE2E2E2, 0xFFE3E3E3,
            0xFFE4E4E4, 0xFFE5E5E5, 0xFFE6E6E6, 0xFFE7E7E7,
            0xFFE8E8E8, 0xFFE9E9E9, 0xFFEAEAEA, 0xFFEBEBEB,
            0xFFECECEC, 0xFFEDEDED, 0xFFEEEEEE, 0xFFEFEFEF,
            0xFFF0F0F0, 0xFFF1F1F1, 0xFFF2F2F2, 0xFFF3F3F3,
            0xFFF4F4F4, 0xFFF5F5F5, 0xFFF6F6F6, 0xFFF7F7F7,
            0xFFF8F8F8, 0xFFF9F9F9, 0xFFFAFAFA, 0xFFFBFBFB,
            0xFFFCFCFC, 0xFFFDFDFD, 0xFFFEFEFE, 0xFFFFFFFF,
        };
        new_texture = texture_new_indexed(width, height, luminance_palette,
                                          MEM_ALLOC_TOP);
    } else {
        new_texture = texture_new(width, height, MEM_ALLOC_TOP);
    }
    if (!new_texture) {
        SET_ERROR(GL_OUT_OF_MEMORY);
        return;
    }

    if (texture_array[bound_texture].texture != UNDEFINED_TEXTURE) {
        texture_destroy(texture_array[bound_texture].texture);
    }
    texture_array[bound_texture].texture = new_texture;

    /* Swizzle the new texture's data if possible for faster drawing. */
    const int swizzle = (internalformat == GL_LUMINANCE
                         ? width%16 == 0
                         : width%4 == 0) && height%8 == 0;
    new_texture->swizzled = swizzle;

    /* When copying to textures, we need to flip the image vertically. */
    if (internalformat == GL_LUMINANCE) {
        fb_to_luminance(x, y, width, height, new_texture->pixels,
                        new_texture->stride, 1, swizzle);
    } else {
        fb_to_RGBA(x, y, width, height, (uint32_t *)new_texture->pixels,
                   new_texture->stride, 1, swizzle);
    }
}

/*-----------------------------------------------------------------------*/

void glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset,
                          GLint yoffset, GLint x, GLint y, GLsizei width,
                          GLsizei height)
{
    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D
     || level != 0
    ) {
        DMSG("Invalid/unsupported parameters: 0x%X %d %d %d %d %d %d %d",
             (int)target, (int)level, (int)xoffset, (int)yoffset,
             (int)x, (int)y, (int)width, (int)height);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    if (!bound_texture || !texture_array[bound_texture].texture
     || texture_array[bound_texture].texture == UNDEFINED_TEXTURE
    ) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }
    Texture *texture = texture_array[bound_texture].texture;

    if (x < 0 || y < 0 || x+width > DISPLAY_WIDTH || y+height > DISPLAY_HEIGHT
     || xoffset < 0 || yoffset < 0
     || xoffset+width > texture->width || yoffset+height > texture->height
    ) {
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    /* Abort the operation if the texture is swizzled but the coordinates
     * aren't properly aligned.  (We could technically implement this if
     * we wanted to, but we don't need to for Aquaria.) */
    const int can_swizzle = (texture->indexed ? width%16 == 0 : width%4 == 0)
        && (texture->indexed ? xoffset%16 == 0 : xoffset%4 == 0)
        && height%8 == 0
        && yoffset%8 == 0;
    if (texture->swizzled && !can_swizzle) {
        DMSG("Texture is swizzled but can't swizzle these coordinates!"
             " (offset=%d,%d size=%dx%d)", xoffset, yoffset, width, height);
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (texture->indexed) {
        fb_to_luminance(x, y, width, height,
                        texture->pixels
                            + yoffset * texture->stride
                            + xoffset,
                        texture->stride, 1, texture->swizzled);
    } else {
        fb_to_RGBA(x, y, width, height,
                   (uint32_t *)texture->pixels
                       + yoffset * texture->stride
                       + xoffset,
                   texture->stride, 1, texture->swizzled);
    }
}

/*-----------------------------------------------------------------------*/

void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type,
                   GLvoid *pixels)
{
    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D
     || level != 0
     || format != GL_RGBA
     || type != GL_UNSIGNED_BYTE
     || !pixels
    ) {
        DMSG("Invalid/unsupported parameters: 0x%X %d 0x%X 0x%X %p",
             (int)target, (int)level, (int)format, (int)type, pixels);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    if (!bound_texture || !texture_array[bound_texture].texture
     || texture_array[bound_texture].texture == UNDEFINED_TEXTURE
    ) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    const Texture * const texture = texture_array[bound_texture].texture;

    /* These combinations should be impossible, but check anyway to avoid
     * overrunning the source buffer. */
    if (texture->swizzled
     && ((texture->height & 7) != 0
         || (texture->indexed
             ? (texture->stride & 15) != 0
             : (texture->stride & 3) != 0))
    ) {
        DMSG("Can't handle getting swizzled textures with unaligned height"
             "or stride (%p, %dx%d, stride=%d)", texture, texture->width,
             texture->height, texture->stride);
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    /* If the width is unaligned for a swizzled texture, we'll overflow
     * the destination buffer, so allocate a temporary, aligned-width
     * buffer for unswizzling. */
    uint32_t *temp_buffer = NULL;
    if (texture->width != texture->stride) {
        const uint32_t size = texture->stride * texture->height * 4;
        temp_buffer = mem_alloc(size, 64, MEM_ALLOC_TEMP);
        if (UNLIKELY(!temp_buffer)) {
            DMSG("Can't allocate temporary buffer for unswizzling (%u bytes)",
                 size);
            SET_ERROR(GL_OUT_OF_MEMORY);
            return;
        }
    }

    uint32_t *dest = temp_buffer ? temp_buffer : (uint32_t *)pixels;
    const unsigned int width       = texture->width;
    const unsigned int height      = texture->height;
    const unsigned int stride      = texture->stride;
    const uint32_t * const palette = texture->palette;

    if (texture->swizzled) {

        if (texture->indexed) {

            const uint8_t *src = texture->pixels;
            unsigned int y;
            for (y = 0; y < height; y += 8, dest += stride*7) {
                unsigned int x;
                for (x = 0; x < width; x += 16, dest += 16) {
                    uint32_t *destline = dest;
                    unsigned int line;
                    for (line = 0; line < 8; line++, destline += stride) {
                        unsigned int i;
                        for (i = 0; i < 16; i += 4, src += 4) {
                            const uint8_t pixel0 = src[0];
                            const uint8_t pixel1 = src[1];
                            const uint8_t pixel2 = src[2];
                            const uint8_t pixel3 = src[3];
                            destline[i+0] = palette[pixel0];
                            destline[i+1] = palette[pixel1];
                            destline[i+2] = palette[pixel2];
                            destline[i+3] = palette[pixel3];
                        }
                    }
                }
            }

        } else {  // !indexed

            const uint32_t *src = (const uint32_t *)texture->pixels;
            unsigned int y;
            for (y = 0; y < height; y += 8, dest += stride*7) {
                unsigned int x;
                for (x = 0; x < width; x += 16, dest += 16) {
                    uint32_t *destline = dest;
                    unsigned int line;
                    for (line = 0; line < 8; line++, destline += stride) {
                        const uint32_t pixel0 = src[0];
                        const uint32_t pixel1 = src[1];
                        const uint32_t pixel2 = src[2];
                        const uint32_t pixel3 = src[3];
                        destline[0] = pixel0;
                        destline[1] = pixel1;
                        destline[2] = pixel2;
                        destline[3] = pixel3;
                    }
                }
            }

        }  // if (indexed)

    } else {  // !swizzled

        if (texture->indexed) {

            const uint8_t *src = texture->pixels;
            unsigned int y;
            for (y = 0; y < height; y++, src += stride, dest += stride) {
                unsigned int x;
                for (x = 0; x < width; x++) {
                    dest[x] = palette[src[x]];
                }
            }

        } else {  // !indexed

            const uint32_t *src =
                (const uint32_t *)texture->pixels;
            unsigned int y;
            for (y = 0; y < height; y++, src += stride, dest += stride) {
                memcpy(dest, src, width*4);
            }

        }  // if (indexed)

    }  // if (swizzled)

    if (temp_buffer) {
        const uint32_t *copy_src = temp_buffer;
        uint32_t *copy_dest = (uint32_t *)pixels;
        unsigned int y;
        for (y = 0; y < height; y++, copy_src += stride, copy_dest += width) {
            memcpy(copy_dest, copy_src, width*4);
        }
        mem_free(temp_buffer);
    }
}

/*************************************************************************/

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    if (target != GL_TEXTURE_2D) {
        DMSG("Invalid/unsupported parameters: 0x%X %d %d",
             (int)target, (int)pname, (int)param);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {

      case GL_TEXTURE_MAG_FILTER:
        switch (param) {
          case GL_NEAREST:
            texture_mag_filter = GE_TEXFILTER_NEAREST;
            break;
          case GL_LINEAR:
            texture_mag_filter = GE_TEXFILTER_LINEAR;
            break;
          default:
            DMSG("Invalid/unsupported type 0x%X for GL_TEXTURE_MAG_FILTER",
                 (int)param);
            SET_ERROR(GL_INVALID_ENUM);
            return;
        }
        texture_filter_changed = 1;
        break;

      case GL_TEXTURE_MIN_FILTER:
        switch (param) {
          case GL_NEAREST:
            texture_min_filter = GE_TEXFILTER_NEAREST;
            texture_mip_filter = GE_TEXMIPFILTER_NONE;
            break;
          case GL_LINEAR:
            texture_min_filter = GE_TEXFILTER_LINEAR;
            texture_mip_filter = GE_TEXMIPFILTER_NONE;
            break;
          case GL_NEAREST_MIPMAP_NEAREST:
            texture_min_filter = GE_TEXFILTER_NEAREST;
            texture_mip_filter = GE_TEXMIPFILTER_NEAREST;
            break;
          case GL_LINEAR_MIPMAP_NEAREST:
            texture_min_filter = GE_TEXFILTER_LINEAR;
            texture_mip_filter = GE_TEXMIPFILTER_NEAREST;
            break;
          case GL_NEAREST_MIPMAP_LINEAR:
            texture_min_filter = GE_TEXFILTER_NEAREST;
            texture_mip_filter = GE_TEXMIPFILTER_LINEAR;
            break;
          case GL_LINEAR_MIPMAP_LINEAR:
            texture_min_filter = GE_TEXFILTER_LINEAR;
            texture_mip_filter = GE_TEXMIPFILTER_LINEAR;
            break;
          default:
            DMSG("Invalid/unsupported type 0x%X for GL_TEXTURE_MIN_FILTER",
                 (int)param);
            SET_ERROR(GL_INVALID_ENUM);
            return;
        }
        texture_filter_changed = 1;
        break;

      case GL_TEXTURE_WRAP_S:
        switch (param) {
          case GL_CLAMP:
          case GL_CLAMP_TO_BORDER:
          case GL_CLAMP_TO_EDGE:
            texture_wrap_u = GE_TEXWRAPMODE_CLAMP;
            break;
          case GL_REPEAT:
            texture_wrap_u = GE_TEXWRAPMODE_REPEAT;
            break;
          default:
            DMSG("Invalid/unsupported type 0x%X for GL_TEXTURE_WRAP_S",
                 (int)param);
            SET_ERROR(GL_INVALID_ENUM);
            return;
        }
        texture_wrap_mode_changed = 1;
        break;

      case GL_TEXTURE_WRAP_T:
        switch (param) {
          case GL_CLAMP:
          case GL_CLAMP_TO_BORDER:
          case GL_CLAMP_TO_EDGE:
            texture_wrap_v = GE_TEXWRAPMODE_CLAMP;
            break;
          case GL_REPEAT:
            texture_wrap_v = GE_TEXWRAPMODE_REPEAT;
            break;
          default:
            DMSG("Invalid/unsupported type 0x%X for GL_TEXTURE_WRAP_T",
                 (int)param);
            SET_ERROR(GL_INVALID_ENUM);
            return;
        }
        texture_wrap_mode_changed = 1;
        break;

      default:
        DMSG("Invalid/unsupported parameter 0x%X", (int)pname);
        SET_ERROR(GL_INVALID_ENUM);
        return;

    }  // switch (pname)
}

/*-----------------------------------------------------------------------*/

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname,
                              GLfloat *params)
{
    if (target != GL_TEXTURE_2D
     || level != 0
     || !params
    ) {
        DMSG("Invalid/unsupported parameters: 0x%X %d 0x%X %p",
             (int)target, (int)level, (int)pname, params);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    if (!bound_texture || !texture_array[bound_texture].texture
     || texture_array[bound_texture].texture == UNDEFINED_TEXTURE
    ) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    switch (pname) {
      case GL_TEXTURE_WIDTH:
        params[0] = texture_array[bound_texture].texture->width;
        break;
      case GL_TEXTURE_HEIGHT:
        params[0] = texture_array[bound_texture].texture->height;
        break;
      case GL_TEXTURE_COMPONENTS:
        params[0] = 4;
        break;
      default:
        DMSG("Invalid/unsupported parameter 0x%X", (int)pname);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }
}

/*************************************************************************/
/***************** Graphics primitive and vertex handling ****************/
/*************************************************************************/

void glBegin(GLenum mode)
{
    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    switch (mode) {
      case GL_POINTS:         ge_primitive = GE_PRIMITIVE_POINTS;        break;
      case GL_LINES:          ge_primitive = GE_PRIMITIVE_LINES;         break;
      case GL_LINE_STRIP:     ge_primitive = GE_PRIMITIVE_LINE_STRIP;    break;
      case GL_LINE_LOOP:      ge_primitive = GE_PRIMITIVE_LINE_STRIP;    break;
      case GL_TRIANGLES:      ge_primitive = GE_PRIMITIVE_TRIANGLES;     break;
      case GL_TRIANGLE_STRIP: ge_primitive = GE_PRIMITIVE_TRIANGLE_STRIP;break;
      case GL_TRIANGLE_FAN:   ge_primitive = GE_PRIMITIVE_TRIANGLE_FAN;  break;
      case GL_QUADS:          ge_primitive = GE_PRIMITIVE_TRIANGLE_STRIP;break;
      case GL_QUAD_STRIP:     ge_primitive = GE_PRIMITIVE_TRIANGLE_STRIP;break;
      default:
        DMSG("Invalid/unsupported primitive type 0x%X", (int)mode);
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }

    current_primitive = mode;
    vertex_format = GE_VERTEXFMT_TRANSFORM_3D | GE_VERTEXFMT_VERTEX_32BITF
                    // The color might have been set ahead of time, so
                    // always include vertex colors to be safe.
                  | GE_VERTEXFMT_COLOR_8888;
    num_vertices = 0;
    first_vertex = NULL;
}

/*-----------------------------------------------------------------------*/

void glEnd(void)
{
    check_dlist_size();

    if (!current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (num_vertices == 0 || first_vertex == NULL) {
        /* No vertices to render! */
        current_primitive = 0;
        return;
    }

    if (current_primitive == GL_LINE_LOOP) {
        uint32_t *last_vertex = reserve_vertexbytes(vertex_words * 4);
        if (!last_vertex) {
            SET_ERROR(GL_OUT_OF_MEMORY);
            current_primitive = 0;
            return;
        }
        memcpy(last_vertex, first_vertex, vertex_words * 4);
        num_vertices++;
    }

    update_render_state();
    if (enable_texture_2D
     && (bound_texture == 0
         || texture_array[bound_texture].texture == UNDEFINED_TEXTURE)
    ) {
        ge_disable(GE_STATE_TEXTURE);
    }

    /* Check whether this is a full-screen quad, which needs to be
     * optimized to avoid rendering slowdown. */
    int is_full_screen_quad = 0;
    if (!current_dlist
     && current_primitive == GL_QUADS
     && num_vertices == 4
    ) {
        const Matrix4f *Mview = &modelview_matrix_stack[modelview_matrix_top];
        const Matrix4f *Mproj = &projection_matrix_stack[projection_matrix_top];
        if (Mview->_12 == 0 && Mview->_21 == 0) {
            const float xscale = Mview->_11 * Mproj->_11 * (480/2);
            const float yscale = Mview->_22 * Mproj->_22 * (272/2);
            const float xofs = ((Mview->_41 * Mproj->_11) + (Mproj->_41 + 1)) * (480/2);
            const float yofs = ((Mview->_42 * Mproj->_22) + (Mproj->_42 + 1)) * (272/2);
            const VertexData *vptr = first_vertex;
            const Texture *texture = texture_array[bound_texture].texture;
            if (vertex_format == (GE_VERTEXFMT_COLOR_8888
                                  | GE_VERTEXFMT_VERTEX_32BITF)) {
                const float x0 = vptr[0*4+1].f * xscale + xofs;
                const float y0 = vptr[0*4+2].f * yscale + yofs;
                const float x3 = vptr[3*4+1].f * xscale + xofs;
                const float y3 = vptr[3*4+2].f * yscale + yofs;
                if (x0 < 0.5f && y0 < 0.5f
                 && x3 > 479.5f && y3 > 271.5f
                 && vptr[0*4+0].i == current_color
                 && vptr[1*4+0].i == current_color
                 && vptr[1*4+1].f == vptr[3*4+1].f
                 && vptr[1*4+2].f == vptr[0*4+2].f
                 && vptr[2*4+0].i == current_color
                 && vptr[2*4+1].f == vptr[0*4+1].f
                 && vptr[2*4+2].f == vptr[3*4+2].f
                 && vptr[3*4+0].i == current_color
                ) {
                    is_full_screen_quad = 2;
                }
            } else if (vertex_format == (GE_VERTEXFMT_TEXTURE_32BITF
                                         | GE_VERTEXFMT_COLOR_8888
                                         | GE_VERTEXFMT_VERTEX_32BITF)
                       && texture != NULL && texture != UNDEFINED_TEXTURE) {
                const float u0 = vptr[0*6+0].f * texture->width;
                const float v0 = vptr[0*6+1].f * texture->height;
                const float x0 = vptr[0*6+3].f * xscale + xofs;
                const float y0 = vptr[0*6+4].f * yscale + yofs;
                const float u3 = vptr[3*6+0].f * texture->width;
                const float v3 = vptr[3*6+1].f * texture->height;
                const float x3 = vptr[3*6+3].f * xscale + xofs;
                const float y3 = vptr[3*6+4].f * yscale + yofs;
                if (fabsf(u0) < 0.001f && fabsf(v0) < 0.001f
                 && fabsf(x0) < 0.001f && fabsf(y0) < 0.001f
                 && fabsf(u3-480) < 0.001f && fabsf(v3-272) < 0.001f
                 && fabsf(x3-480) < 0.001f && fabsf(y3-272) < 0.001f
                 && vptr[1*6+0].f == vptr[3*6+0].f
                 && vptr[1*6+1].f == vptr[0*6+1].f
                 && vptr[1*6+3].f == vptr[3*6+3].f
                 && vptr[1*6+4].f == vptr[0*6+4].f
                 && vptr[2*6+0].f == vptr[0*6+0].f
                 && vptr[2*6+1].f == vptr[3*6+1].f
                 && vptr[2*6+3].f == vptr[0*6+3].f
                 && vptr[2*6+4].f == vptr[3*6+4].f
                ) {
                    is_full_screen_quad = 1;
                }
            }
        }
    }
    if (is_full_screen_quad) {
        ge_set_texture_filter(GE_TEXFILTER_NEAREST, GE_TEXFILTER_NEAREST,
                              GE_TEXMIPFILTER_NONE);
        texture_filter_changed = 1;  // Restore it next time around.
        ge_set_vertex_format(GE_VERTEXFMT_TRANSFORM_2D
                             | GE_VERTEXFMT_TEXTURE_16BIT
                             | GE_VERTEXFMT_COLOR_8888
                             | GE_VERTEXFMT_VERTEX_16BIT);
        ge_set_vertex_pointer(NULL);
        int x;
        for (x = 0; x < 480; x += 16) {
            ge_add_uv_color_xy_vertex(x, 272, current_color, x, 0);
            ge_add_uv_color_xy_vertex(x+16, 0, current_color, x+16, 272);
        }
        ge_draw_primitive(GE_PRIMITIVE_SPRITES, (480/16)*2);
    } else {
        ge_set_vertex_format(vertex_format);
        ge_set_vertex_pointer(first_vertex);
        if (current_primitive == GL_QUADS) {
            unsigned int i;
            for (i = 0; i+4 <= num_vertices; i += 4) {
                ge_draw_primitive(GE_PRIMITIVE_TRIANGLE_STRIP, 4);
            }
        } else {
            ge_draw_primitive(ge_primitive, num_vertices);
        }
    }

    uncached_vertices += num_vertices;
    if (uncached_vertices >= UNCACHED_VERTEX_LIMIT) {
        ge_commit();
        uncached_vertices = 0;
    }

    if (enable_texture_2D && bound_texture == 0) {
        ge_enable(GE_STATE_TEXTURE);
    }

    current_primitive = 0;
}

/*************************************************************************/

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    check_dlist_size();

    current_color = red<<0 | green<<8 | blue<<16 | alpha<<24;

    if (color_material_state) {
        ge_set_ambient_color(current_color);
        ge_set_ambient_light(current_color);
    }
}

/*-----------------------------------------------------------------------*/

void glNormal3f(float nx, float ny, float nz)
{
    current_nx = nx;
    current_ny = ny;
    current_nz = nz;

    if (current_primitive) {
        if (num_vertices == 0) {
            vertex_format |= GE_VERTEXFMT_NORMAL_32BITF;
        } else if (!(vertex_format & GE_VERTEXFMT_NORMAL_32BITF)) {
            DMSG("NORMAL not set in vertex format");
        }
    }
}

/*-----------------------------------------------------------------------*/

void glTexCoord2f(float s, float t)
{
    current_u = s;
    current_v = t;

    if (current_primitive && bound_texture != 0) {
        if (num_vertices == 0) {
            vertex_format |= GE_VERTEXFMT_TEXTURE_32BITF;
        } else if (!(vertex_format & GE_VERTEXFMT_TEXTURE_32BITF)) {
            DMSG("TEXTURE not set in vertex format");
        }
    }
}

/*-----------------------------------------------------------------------*/

void glVertex3f(float x, float y, float z)
{
    check_dlist_size();

    if (!current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (current_primitive == GL_QUADS && (num_vertices % 4) == 2) {
        /* Save this vertex so we can swap it with the next one. */
        quad_saved_color = current_color;
        quad_saved_u = current_u;
        quad_saved_v = current_v;
        quad_saved_nx = current_nx;
        quad_saved_ny = current_ny;
        quad_saved_nz = current_nz;
        quad_saved_x = x;
        quad_saved_y = y;
        quad_saved_z = z;
        num_vertices++;
        return;
    }

    VertexData *vertex_buffer;
    if (num_vertices == 0) {
        vertex_words = 3;
        if (vertex_format & GE_VERTEXFMT_TEXTURE_32BITF) {
            vertex_words += 2;
        }
        if (vertex_format & GE_VERTEXFMT_COLOR_8888) {
            vertex_words += 1;
        }
        if (vertex_format & GE_VERTEXFMT_NORMAL_32BITF) {
            vertex_words += 3;
        }
        first_vertex = reserve_vertexbytes(vertex_words * 4);
        vertex_buffer = first_vertex;
    } else {
        vertex_buffer = reserve_vertexbytes(vertex_words * 4);
    }
    if (UNLIKELY(!vertex_buffer)) {
        DMSG("Vertex buffer overflow!");
        SET_ERROR(GL_OUT_OF_MEMORY);
        return;
    }

    if (vertex_format & GE_VERTEXFMT_TEXTURE_32BITF) {
        (vertex_buffer++)->f = current_u;
        (vertex_buffer++)->f = current_v;
    }
    if (vertex_format & GE_VERTEXFMT_COLOR_8888) {
        (vertex_buffer++)->i = current_color;
    }
    if (vertex_format & GE_VERTEXFMT_NORMAL_32BITF) {
        (vertex_buffer++)->f = current_nx;
        (vertex_buffer++)->f = current_ny;
        (vertex_buffer++)->f = current_nz;
    }
    (vertex_buffer++)->f = x;
    (vertex_buffer++)->f = y;
    (vertex_buffer++)->f = z;

    if (current_primitive == GL_QUADS && (num_vertices % 4) == 3) {
        /* Store the previous vertex immediately after this one. */
        vertex_buffer = reserve_vertexbytes(vertex_words * 4);
        if (UNLIKELY(!vertex_buffer)) {
            DMSG("Vertex buffer overflow!");
            SET_ERROR(GL_OUT_OF_MEMORY);
            return;
        }
        if (vertex_format & GE_VERTEXFMT_TEXTURE_32BITF) {
            (vertex_buffer++)->f = quad_saved_u;
            (vertex_buffer++)->f = quad_saved_v;
        }
        if (vertex_format & GE_VERTEXFMT_COLOR_8888) {
            (vertex_buffer++)->i = quad_saved_color;
        }
        if (vertex_format & GE_VERTEXFMT_NORMAL_32BITF) {
            (vertex_buffer++)->f = quad_saved_nx;
            (vertex_buffer++)->f = quad_saved_ny;
            (vertex_buffer++)->f = quad_saved_nz;
        }
        (vertex_buffer++)->f = quad_saved_x;
        (vertex_buffer++)->f = quad_saved_y;
        (vertex_buffer++)->f = quad_saved_z;
    }

    num_vertices++;
}

/*************************************************************************/
/*********************** Display list manipulation ***********************/
/*************************************************************************/

GLuint glGenLists(GLsizei range)
{
    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return 0;
    }

    /* For simplicity, we only handle the case of a single allocation. */
    if (range != 1) {
        SET_ERROR(GL_INVALID_VALUE);
        return 0;
    }

    uint32_t id;
    for (id = 1; id < dlist_array_size; id++) {
        if (!dlist_array[id].list) {
            break;
        }
    }
    if (id >= dlist_array_size) {
        const uint32_t new_size = id+1;
        DisplayList *new_array =
            mem_realloc(dlist_array, sizeof(*new_array) * new_size, 0);
        if (!new_array) {
            DMSG("Failed to realloc dlist array from %u to %u entries",
                 dlist_array_size, new_size);
            SET_ERROR(GL_OUT_OF_MEMORY);
            return 0;
        }
        dlist_array = new_array;
        dlist_array_size = new_size;
    }

    dlist_array[id].list = NULL;
    dlist_array[id].vertex_buffers = NULL;
    dlist_array[id].to_free = 0;
    dlist_array[id].in_use = 1;
    return id;
}

/*-----------------------------------------------------------------------*/

void glDeleteLists(GLuint list, GLsizei range)
{
    if (current_primitive || current_dlist) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    GLsizei i;
    for (i = 0; i < range; i++, list++) {
        if (list == 0 || list >= dlist_array_size) {
            DMSG("Invalid display list ID %u (limit %u)",
                 (int)list, dlist_array_size);
            continue;
        }
        if (!dlist_array[list].in_use) {
            DMSG("Invalid display list ID %u (not allocated)", (int)list);
            SET_ERROR(GL_INVALID_VALUE);
            return;
        }
        if (dlist_array[list].to_free) {
            /* Already marked for deletion, so skip it. */
        } else if (dlist_array[list].list) {
            dlist_array[list].to_free = 1;
            dlist_array[list].next_free = first_dlist_to_free;
            first_dlist_to_free = list;
        } else {
            /* No list was created, so we can free the entry immediately. */
            dlist_array[list].in_use = 0;
        }
    }
}

/*************************************************************************/

void glNewList(GLuint list, GLenum mode)
{
    if (current_primitive || current_dlist) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (list == 0 || list >= dlist_array_size) {
        DMSG("Invalid display list ID %u (limit %u)",
             (int)list, dlist_array_size);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }
    if (!dlist_array[list].in_use) {
        DMSG("Invalid display list ID %u (not allocated)", (int)list);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }
    if (dlist_array[list].to_free) {
        DMSG("Invalid display list ID %u (deleted)", (int)list);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }
    if (dlist_array[list].list) {
        DMSG("Invalid display list ID %u (already created)", (int)list);
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    dlist_array[list].size = DLIST_SIZE_INCREMENT;
    dlist_array[list].list = mem_alloc(dlist_array[list].size * 4, 4, 0);
    if (!dlist_array[list].list) {
        SET_ERROR(GL_OUT_OF_MEMORY);
        return;
    }

    if (!ge_start_sublist(dlist_array[list].list, dlist_array[list].size)) {
        SET_ERROR(GL_OUT_OF_MEMORY);
        mem_free(dlist_array[list].list);
        dlist_array[list].list = NULL;
        return;
    }

    /* Since we store actual matrices in the display list rather than
     * matrix operations, we save the current modelview matrix and replace
     * it with an identity matrix; we then apply the (possibly transformed)
     * new matrix as the GE model matrix, leaving the GE view matrix
     * untouched so the list caller can transform it as appropriate. */
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
    asm(".set push; .set noreorder\n"
        "sv.q C700,  0(%[matrix])\n"
        "sv.q C710, 16(%[matrix])\n"
        "sv.q C720, 32(%[matrix])\n"
        "sv.q C730, 48(%[matrix])\n"
        ".set pop"
        : /* no outputs */
        : [matrix] "r" (&modelview_matrix_stack[modelview_matrix_top])
    );
#endif
    dlist_saved_matrix = modelview_matrix_stack[modelview_matrix_top];
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
    asm(".set push; .set noreorder\n"
        "vidt.q C700\n"
        "vidt.q C710\n"
        "vidt.q C720\n"
        "vidt.q C730\n"
        ".set pop"
    );
#else
    modelview_matrix_stack[modelview_matrix_top] = identity_matrix;
#endif
    modelview_matrix_changed = 0;
    /* Also save the current modelview matrix stack pointer, and warn if a
     * command tries to modify the current matrix without first pushing it
     * or if the stack isn't properly restored at the end of the list. */
    dlist_saved_matrix_top = modelview_matrix_top;

    /* Clear the texture change flags for a clean slate (we don't want to
     * have any previous changes applied within the list). */
    texture_changed = 0;
    texture_filter_changed = 0;
    texture_wrap_mode_changed = 0;

    /* Save the current state, and clear cached blend factor values so
     * the next glBlendFunc() call will always write a GE command into the
     * sublist. */
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    blend_sfactor = blend_dfactor = -1;

    if (matrix_mode != GL_MODELVIEW) {
        DMSG("Forcing modelview matrix mode");
        glMatrixMode(GL_MODELVIEW);
    }

    current_dlist = list;
}

/*-----------------------------------------------------------------------*/

void glEndList(void)
{
    if (!current_dlist) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    /* Make sure to reset the model matrix to the identity when we're done,
     * so it doesn't affect subsequent renders. */
    check_dlist_size();
    ge_set_model_matrix(&identity_matrix);

    uint32_t *list_end = ge_finish_sublist();

    DisplayList *dlist = &dlist_array[current_dlist];
    const uint32_t list_size = list_end - dlist->list;
    if (list_size > dlist->size) {
        DMSG("BUG?  List end %p - base %p > size %u*4", list_end,
             dlist->list, dlist->size);
    } else {
        uint32_t *list = mem_realloc(dlist->list, list_size*4, 0);
        if (list) {
            dlist->list = list;
        }
    }
    sceKernelDcacheWritebackRange(dlist->list, list_size*4);
    if (dlist->vertex_buffers) {
        VertexBuffer *buffer = dlist->vertex_buffers;
        buffer = mem_realloc(buffer, sizeof(VertexBuffer) + buffer->used, 0);
        if (buffer) {
            buffer->size = buffer->used;
            dlist->vertex_buffers = buffer;
        }
        for (buffer = dlist->vertex_buffers; buffer; buffer = buffer->next) {
            sceKernelDcacheWritebackRange(buffer->data, buffer->used);
        }
    }

    current_dlist = 0;

    glPopAttrib();

    if (modelview_matrix_top != dlist_saved_matrix_top) {
        DMSG("WARNING: display list left matrix stack unbalanced"
             " (old=%u new=%u), restoring", dlist_saved_matrix_top,
             modelview_matrix_top);
        modelview_matrix_top = dlist_saved_matrix_top;
    }
    modelview_matrix_stack[modelview_matrix_top] = dlist_saved_matrix;
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
    asm(".set push; .set noreorder\n"
        "lv.q C700,  0(%[matrix])\n"
        "lv.q C710, 16(%[matrix])\n"
        "lv.q C720, 32(%[matrix])\n"
        "lv.q C730, 48(%[matrix])\n"
        ".set pop"
        : /* no outputs */
        : [matrix] "r" (&modelview_matrix_stack[modelview_matrix_top])
    );
#endif
    modelview_matrix_changed = 1;
    texture_changed = 1;
    texture_filter_changed = 1;
    texture_wrap_mode_changed = 1;
    blend_sfactor = blend_dfactor = -1;
}

/*************************************************************************/

void glCallList(GLuint list)
{
    if (current_primitive || current_dlist) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (list == 0 || list >= dlist_array_size) {
        DMSG("Invalid display list ID %u (limit %u)",
             (int)list, dlist_array_size);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }
    if (!dlist_array[list].list || dlist_array[list].to_free) {
        DMSG("Invalid display list ID %u (undefined or deleted)", (int)list);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    update_render_state();
    ge_call_sublist(dlist_array[list].list);

    modelview_matrix_changed = 1;
    texture_changed = 1;
    texture_filter_changed = 1;
    texture_wrap_mode_changed = 1;
    blend_sfactor = blend_dfactor = -1;
}

/*************************************************************************/
/************************ Miscellaneous routines *************************/
/*************************************************************************/

void glClear(GLbitfield mask)
{
    check_dlist_size();

    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    ge_clear(mask & GL_COLOR_BUFFER_BIT, mask & GL_DEPTH_BUFFER_BIT,
             clear_color);
}

/*-----------------------------------------------------------------------*/

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    clear_color = iroundf(red  *255) <<  0
                | iroundf(green*255) <<  8
                | iroundf(blue *255) << 16
                | iroundf(alpha*255) << 24;
}

/*-----------------------------------------------------------------------*/

void glClearDepth(GLclampd depth)
{
    if (depth != 1.0) {
        DMSG("Clear depth %.3f not supported", depth);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }
}

/*************************************************************************/

void glRasterPos2i(GLint x, GLint y)
{
    /* Aquaria only calls this function to reset the raster position to 0,0. */
    if (x != 0 || y != 0) {
        DMSG("glRasterPos() not supported for nonzero coordinates %d,%d",
             (int)x, (int)y);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }
}

/*-----------------------------------------------------------------------*/

void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    check_dlist_size();

    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (type != GL_COLOR) {
        DMSG("Copy type 0x%X not supported", (int)type);
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (x < 0 || y < 0 || x+width > DISPLAY_WIDTH || y+height > DISPLAY_HEIGHT) {
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    ge_copy(psp_work_pixel_address(x,y), DISPLAY_STRIDE,
            psp_work_pixel_address(0,0), DISPLAY_STRIDE,
            width, height, GE_COPY_32BIT);
}

/*-----------------------------------------------------------------------*/

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels)
{
    if (!in_frame) {
        DMSG("Called outside a frame!");
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if ((format != GL_LUMINANCE && format != GL_RGB && format != GL_RGBA)
     || type != GL_UNSIGNED_BYTE
     || !pixels
    ) {
        DMSG("Invalid/unsupported parameters: %d %d %d %d 0x%X 0x%X %p",
             (int)x, (int)y, (int)width, (int)height, (int)format,
             (int)type, pixels);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    if (x < 0 || y < 0 || x+width > DISPLAY_WIDTH || y+height > DISPLAY_HEIGHT) {
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    const uint32_t dest_stride = width;
    if (format == GL_LUMINANCE) {
        fb_to_luminance(x, y, width, height, pixels, dest_stride, 0, 0);
    } else if (format == GL_RGB) {
        fb_to_RGB(x, y, width, height, pixels, dest_stride, 0);
    } else {
        fb_to_RGBA(x, y, width, height, (uint32_t *)pixels, dest_stride, 0, 0);
    }
}

/*************************************************************************/

void glFlush(void)
{
    if (current_dlist) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (in_frame) {
        ge_commit();
        uncached_vertices = 0;
    }
}

/*-----------------------------------------------------------------------*/

void glFinish(void)
{
    if (current_dlist) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (in_frame) {
        ge_sync();
        uncached_vertices = 0;
    }
}

/*************************************************************************/
/************************* PSP-specific routines *************************/
/*************************************************************************/

/**
 * fakeglBeginFrame:  Begin drawing a new frame.  This function must be
 * called prior to any rendering functions for a given frame.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void fakeglBeginFrame(void)
{
    if (in_frame) {
        DMSG("Already rendering a frame!");
        return;
    }

    in_frame = 1;

    if (is_offscreen) {
        graphics_sync();
        ge_start_frame(GE_PIXFMT_8888);
        ge_set_draw_buffer(psp_vram_spare_ptr(), DISPLAY_STRIDE);
    } else {
        graphics_start_frame();
        ge_set_draw_buffer(NULL, 0);
    }

    /* We now know the GE is done rendering the previous frame, so destroy
     * all textures and display lists that were deleted during that frame. */
    uint32_t tex_id = first_texture_to_free;
    while (tex_id != 0) {
        if (texture_array[tex_id].texture != UNDEFINED_TEXTURE) {
            texture_destroy(texture_array[tex_id].texture);
        }
        texture_array[tex_id].texture = NULL;
        texture_array[tex_id].to_free = 0;
        tex_id = texture_array[tex_id].next_free;
    }
    first_texture_to_free = 0;
    uint32_t dlist_id = first_dlist_to_free;
    while (dlist_id != 0) {
        if (dlist_array[dlist_id].list) {
            VertexBuffer *vbuf, *next;
            for (vbuf = dlist_array[dlist_id].vertex_buffers; vbuf; vbuf = next) {
                next = vbuf->next;
                mem_free(vbuf);
            }
            mem_free(dlist_array[dlist_id].list);
        }
        dlist_array[dlist_id].list = NULL;
        dlist_array[dlist_id].vertex_buffers = NULL;
        dlist_array[dlist_id].to_free = 0;
        dlist_array[dlist_id].in_use = 0;
        dlist_id = dlist_array[dlist_id].next_free;
    }
    first_dlist_to_free = 0;

    uncached_vertices = 0;

#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
    asm(".set push; .set noreorder\n"
        "lv.q C700,  0(%[matrix])\n"
        "lv.q C710, 16(%[matrix])\n"
        "lv.q C720, 32(%[matrix])\n"
        "lv.q C730, 48(%[matrix])\n"
        ".set pop"
        : /* no outputs */
        : [matrix] "r" (&modelview_matrix_stack[modelview_matrix_top])
    );
#endif

    ge_set_projection_matrix(&projection_matrix_stack[projection_matrix_top]);
    ge_set_view_matrix(&modelview_matrix_stack[modelview_matrix_top]);
    ge_set_viewport(viewport_x, viewport_y, viewport_w, viewport_h);

    enable_alpha_test ? ge_enable(GE_STATE_ALPHA_TEST)
                      : ge_disable(GE_STATE_ALPHA_TEST);
    enable_blend ? ge_enable(GE_STATE_BLEND) : ge_disable(GE_STATE_BLEND);
    enable_depth_test ? ge_enable(GE_STATE_DEPTH_TEST)
                      : ge_disable(GE_STATE_DEPTH_TEST);
    enable_texture_2D ? ge_enable(GE_STATE_TEXTURE)
                      : ge_disable(GE_STATE_TEXTURE);
    if (enable_scissor_test) {
        ge_set_clip_area(scissor_x0, scissor_y0, scissor_x1, scissor_y1);
    }

    const GLenum sfactor = blend_sfactor, dfactor = blend_dfactor;
    blend_sfactor = blend_dfactor = 0;  // Force re-setting of the GE command.
    glBlendFunc(sfactor, dfactor);

    ge_set_texture_filter(texture_mag_filter, texture_min_filter,
                          texture_mip_filter);
    ge_set_texture_wrap_mode(texture_wrap_u, texture_wrap_v);
}

/*-----------------------------------------------------------------------*/

/**
 * fakeglBeginOffscreenFrame:  Begin drawing a new frame in an offscreen
 * framebuffer.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void fakeglBeginOffscreenFrame(void)
{
    if (in_frame) {
        DMSG("Already rendering a frame!");
        return;
    }

    is_offscreen = 1;
    fakeglBeginFrame();
}

/*-----------------------------------------------------------------------*/

/**
 * fakeglEndFrame:  Finish drawing the current frame, and swap it to the
 * display buffer if it is not an offscreen frame.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void fakeglEndFrame(void)
{
    if (!in_frame) {
        DMSG("Not rendering a frame!");
        return;
    }

    if (current_dlist) {
        DMSG("WARNING: Aborting unfinished display list %u", current_dlist);
        glEndList();
        current_dlist = 0;
    }
    if (current_primitive) {
        DMSG("WARNING: Aborting unfinished primitive %u", current_primitive);
        current_primitive = 0;
    }

    in_frame = 0;

    if (is_offscreen) {
        ge_end_frame();
        is_offscreen = 0;
    } else {
        graphics_finish_frame();
    }

#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
    asm(".set push; .set noreorder\n"
        "sv.q C700,  0(%[matrix])\n"
        "sv.q C710, 16(%[matrix])\n"
        "sv.q C720, 32(%[matrix])\n"
        "sv.q C730, 48(%[matrix])\n"
        ".set pop"
        : /* no outputs */
        : [matrix] "r" (&modelview_matrix_stack[modelview_matrix_top])
    );
#endif

    uncached_vertices = 0;
}

/*************************************************************************/

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
void fakeglTexImagePSP(GLenum target, Texture *texture)
{
    if (current_primitive) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D) {
        SET_ERROR(GL_INVALID_ENUM);
        return;
    }

    if (!texture) {
        DMSG("Attempt to bind NULL texture to ID %u", bound_texture);
        SET_ERROR(GL_INVALID_VALUE);
        return;
    }

    if (!bound_texture || !texture_array[bound_texture].texture) {
        SET_ERROR(GL_INVALID_OPERATION);
        return;
    }

    if (texture_array[bound_texture].texture != UNDEFINED_TEXTURE) {
        texture_destroy(texture_array[bound_texture].texture);
    }
    texture_array[bound_texture].texture = texture;
}

/*-----------------------------------------------------------------------*/

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
const Texture *fakeglGetTexPointerPSP(GLenum target)
{
    if (target != GL_TEXTURE_2D) {
        SET_ERROR(GL_INVALID_ENUM);
        return NULL;
    }


    if (!bound_texture || !texture_array[bound_texture].texture
     || texture_array[bound_texture].texture == UNDEFINED_TEXTURE
    ) {
        SET_ERROR(GL_INVALID_OPERATION);
        return NULL;
    }

    return texture_array[bound_texture].texture;
}

/*************************************************************************/
/*********************** Internal helper routines ************************/
/*************************************************************************/

/**
 * _check_dlist_size:  Check whether the current display list is close to
 * full, and expand it if so.  This function should be called at the
 * beginning of any routine which may add commands to the GE display list.
 *
 * [Parameters]
 *     dlist: Current value of current_dlist (passed in for speed)
 * [Return value]
 *     None
 * [Preconditions]
 *     current_dlist != 0
 */
static void _check_dlist_size(unsigned int dlist)
{
    if (ge_sublist_free() < DLIST_EXPAND_THRESHOLD) {
        uint32_t new_size = dlist_array[dlist].size + DLIST_SIZE_INCREMENT;
        uint32_t *new_list =
            mem_realloc(dlist_array[dlist].list, new_size*4, 0);
        if (!new_list) {
            DMSG("Out of memory trying to expand list %p: %u -> %u",
                 dlist_array[dlist].list, dlist_array[dlist].size, new_size);
            return;
        }
        ge_replace_sublist(new_list, new_size);
        dlist_array[dlist].list = new_list;
        dlist_array[dlist].size = new_size;
    }
}

/*************************************************************************/

/**
 * dlist_reserve_vertexbytes:  Allocate space for vertex data to be
 * associated with the current GL display list.  Allocates a new vertex
 * buffer if the current one is full, or expands the current buffer if it
 * has not yet been used in any GE commands (determined by checking whether
 * first_vertex == VertexBuffer.data).
 *
 * [Parameters]
 *     bytes: Number of bytes to reserve
 * [Return value]
 *     Address of allocated memory, or NULL if out of memory
 * [Preconditions]
 *     current_dlist != 0
 */
static void *dlist_reserve_vertexbytes(uint32_t bytes)
{
    DisplayList *dlist = &dlist_array[current_dlist];

    /* First try to allocate from the current vertex buffer, if any. */

    if (dlist->vertex_buffers) {
        VertexBuffer *buffer = dlist->vertex_buffers;
        if (buffer->used + bytes <= buffer->size) {
            void *retval = &buffer->data[buffer->used];
            buffer->used += bytes;
            return retval;
        }

        /* If the vertex buffer is only being used for the current
         * primitive, then we can't split this vertex into a new buffer,
         * but we can expand the current buffer safely. */

        if (first_vertex == (VertexData *)buffer->data) {
            uint32_t new_size = buffer->size + VERTEX_BUFFER_SIZE;
            VertexBuffer *new_buffer =
                mem_realloc(buffer, sizeof(VertexBuffer) + new_size, 0);
            if (!new_buffer) {
                DMSG("Failed to expand vertex buffer %p from %u to %u bytes",
                     buffer, buffer->size, new_size);
                return NULL;
            }
            buffer = new_buffer;
            buffer->size = new_size;
            dlist->vertex_buffers = buffer;
            first_vertex = (VertexData *)buffer->data;
            void *retval = &buffer->data[buffer->used];
            buffer->used += bytes;
            return retval;
        }
    }  // if (dlist->vertex_buffers)

    /* The current buffer is full or there was no current buffer, so
     * allocate a new one, making sure it will have enough room for the
     * new vertex. */

    VertexBuffer *old_buffer = dlist->vertex_buffers;
    uint32_t bytes_to_move = 0;
    uint32_t move_offset = 0;
    if (old_buffer && first_vertex) {
        move_offset = (uint8_t *)first_vertex - old_buffer->data;
        if (move_offset >= old_buffer->used) {
            DMSG("BUG? move_offset %u (first_vertex %p) >= old_buffer->used %u"
                 " (data %p)", move_offset, first_vertex, old_buffer->used,
                 old_buffer->data);
            move_offset = 0;
        } else {
            bytes_to_move = old_buffer->used - move_offset;
        }
    }

    uint32_t size = max(VERTEX_BUFFER_SIZE, bytes_to_move + bytes);
    VertexBuffer *buffer = mem_alloc(sizeof(VertexBuffer) + size, 0, 0);
    if (!buffer) {
        DMSG("No memory for new vertex buffer (%u bytes)", size);
        return NULL;
    }
    buffer->size = VERTEX_BUFFER_SIZE;
    buffer->used = 0;

    /* If there was a vertex buffer, copy any pending vertex data for the
     * current primitive into the new buffer, then truncate the old buffer
     * to the minimum needed size to conserve memory. */

    if (old_buffer) {
        if (bytes_to_move) {
            memcpy(buffer->data, &old_buffer->data[move_offset], bytes_to_move);
            old_buffer->used -= bytes_to_move;
            buffer->used = bytes_to_move;
            first_vertex = (VertexData *)buffer->data;
        }
        VertexBuffer *temp =
            mem_realloc(old_buffer, sizeof(VertexBuffer) + old_buffer->used, 0);
        if (temp) {
            temp->size = temp->used;
            dlist->vertex_buffers = temp;
        }
    }

    /* Link the new buffer into the display list's vertex buffer list and
     * return the allocated vertex data pointer. */

    buffer->next = dlist->vertex_buffers;
    dlist->vertex_buffers = buffer;
    void *retval = &buffer->data[buffer->used];
    buffer->used += bytes;
    return retval;
}

/*************************************************************************/

/**
 * update_render_state:  Apply any transformation matrix or texture state
 * changes to the current GE state, and clear the relevant "changed" flags.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void update_render_state(void)
{
    if (projection_matrix_changed) {
        ge_set_projection_matrix(&projection_matrix_stack[projection_matrix_top]);
        projection_matrix_changed = 0;
    }

    if (modelview_matrix_changed) {
#ifdef STORE_MODELVIEW_MATRIX_IN_VFPU
        asm(".set push; .set noreorder\n"
            "sv.q C700,  0(%[matrix])\n"
            "sv.q C710, 16(%[matrix])\n"
            "sv.q C720, 32(%[matrix])\n"
            "sv.q C730, 48(%[matrix])\n"
            ".set pop"
            : /* no outputs */
            : [matrix] "r" (&modelview_matrix_stack[modelview_matrix_top])
        );
#endif
        if (current_dlist) {
            ge_set_model_matrix(&modelview_matrix_stack[modelview_matrix_top]);
        } else {
            ge_set_view_matrix(&modelview_matrix_stack[modelview_matrix_top]);
        }
        modelview_matrix_changed = 0;
    }

    if (texture_changed) {
        if (bound_texture != 0
         && texture_array[bound_texture].texture != UNDEFINED_TEXTURE
        ) {
            Texture *tex = texture_array[bound_texture].texture;
            if (tex->indexed) {
                ge_set_colortable(tex->palette, 256, GE_PIXFMT_8888, 0, 0xFF);
            }
            unsigned int width = tex->width, height = tex->height;
            unsigned int stride = tex->stride;
            const unsigned int pixel_size = (tex->indexed ? 1 : 4);
            const uint8_t *pixels = tex->pixels;
            ge_set_texture_data(0, pixels, width, height, stride);
            unsigned int level;
            for (level = 1; level <= tex->mipmaps; level++) {
                pixels += stride * height * pixel_size;
                width  = (width+1)/2;
                height = (height+1)/2;
                stride = align_up(stride/2, pixel_size==1 ? 16 : 4);
                ge_set_texture_data(level, pixels, width, height, stride);
            }
            ge_set_texture_format(level, tex->swizzled,
                                  tex->indexed ? GE_TEXFMT_T8 : GE_TEXFMT_8888);
            ge_set_texture_draw_mode(GE_TEXDRAWMODE_MODULATE, 1);
            /* Use texture coordinate scaling to adjust texture coordinates
             * when the texture width or height is not a power of 2. */
            const int log2_width  = tex->width==1 ? 0 :
                ubound(32 - __builtin_clz(tex->width-1), 9);
            const int log2_height = tex->height==1 ? 0 :
                ubound(32 - __builtin_clz(tex->height-1), 9);
            ge_set_texture_scale((float)tex->width  / (float)(1<<log2_width),
                                 (float)tex->height / (float)(1<<log2_height));
        }
        texture_changed = 0;
    }

    if (texture_filter_changed) {
        ge_set_texture_filter(texture_mag_filter, texture_min_filter,
                              texture_mip_filter);
        texture_filter_changed = 0;
    }

    if (texture_wrap_mode_changed) {
        ge_set_texture_wrap_mode(texture_wrap_u, texture_wrap_v);
        texture_wrap_mode_changed = 0;
    }
}

/*************************************************************************/

/**
 * copy_indexed:  Copy 8-bit indexed data into a texture.
 *
 * [Parameters]
 *              data: Source data buffer
 *           texture: Destination texture
 *            x0, y0: First texel in destination region
 *     width, height: Size of destination region
 * [Return value]
 *     None
 */
static void copy_indexed(const void *data, Texture *texture,
                         unsigned int x0, unsigned int y0,
                         unsigned int width, unsigned int height)
{
    PRECOND_SOFT(texture->indexed, return);
    PRECOND_SOFT(x0 + width <= texture->width, return);
    PRECOND_SOFT(y0 + height <= texture->height, return);

    const uint32_t src_stride = width;
    const uint32_t dest_stride = texture->stride;
    const uint8_t *src = (const uint8_t *)data;
    uint8_t *dest = texture->pixels + (y0 * dest_stride) + x0;
    unsigned int y;
    for (y = 0; y < height; y++, src += src_stride, dest += dest_stride) {
        memcpy(dest, src, width);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * copy_RGB:  Copy RGB data into a texture.
 *
 * [Parameters]
 *              data: Source data buffer
 *           texture: Destination texture
 *            x0, y0: First texel in destination region
 *     width, height: Size of destination region
 * [Return value]
 *     None
 */
static void copy_RGB(const void *data, Texture *texture,
                     unsigned int x0, unsigned int y0,
                     unsigned int width, unsigned int height)
{
    PRECOND_SOFT(!texture->indexed, return);
    PRECOND_SOFT(x0 + width <= texture->width, return);
    PRECOND_SOFT(y0 + height <= texture->height, return);

    const uint32_t src_stride = width * 4;
    const uint32_t dest_stride = texture->stride * 4;
    const uint8_t *src = (const uint8_t *)data;
    uint32_t *dest =
        (uint32_t *)(texture->pixels + (y0 * dest_stride) + (x0 * 4));
    unsigned int y;
    for (y = 0; y < height; y++, src += src_stride, dest += dest_stride) {
        unsigned int x;
        for (x = 0; x < width; x++) {
            const unsigned int r = src[x*3+0];
            const unsigned int g = src[x*3+1];
            const unsigned int b = src[x*3+2];
            dest[x] = r<<0 | g<<8 | b<<16 | 0xFF000000;
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * copy_RGBA:  Copy RGBA data into a texture.
 *
 * [Parameters]
 *              data: Source data buffer
 *           texture: Destination texture
 *            x0, y0: First texel in destination region
 *     width, height: Size of destination region
 * [Return value]
 *     None
 */
static void copy_RGBA(const void *data, Texture *texture,
                      unsigned int x0, unsigned int y0,
                      unsigned int width, unsigned int height)
{
    PRECOND_SOFT(!texture->indexed, return);
    PRECOND_SOFT(x0 + width <= texture->width, return);
    PRECOND_SOFT(y0 + height <= texture->height, return);

    const uint32_t src_stride = width * 4;
    const uint32_t dest_stride = texture->stride * 4;
    const uint8_t *src = (const uint8_t *)data;
    uint8_t *dest = texture->pixels + (y0 * dest_stride) + (x0 * 4);
    unsigned int y;
    for (y = 0; y < height; y++, src += src_stride, dest += dest_stride) {
        memcpy(dest, src, width * 4);
    }
}

/*************************************************************************/

/**
 * fb_to_luminance:  Convert framebuffer data to grayscale and store into
 * an 8bpp indexed buffer.
 *
 * [Parameters]
 *            x0, y0: Coordinates of first pixel to copy
 *     width, height: Size of region to copy
 *              dest: Destination buffer
 *       dest_stride: Destination buffer line length (pixels)
 *             vflip: Nonzero to flip the image data vertically
 *           swizzle: Nonzero to swizzle output data (requires width%16 == 0,
 *                       height%8 == 0, dest_stride%16 == 0)
 * [Return value]
 *     None
 */
static void fb_to_luminance(unsigned int x0, unsigned int y0,
                            unsigned int width, unsigned int height,
                            uint8_t *dest, unsigned int dest_stride,
                            int vflip, int swizzle)
{
    PRECOND_SOFT(x0 + width <= DISPLAY_WIDTH, return);
    PRECOND_SOFT(y0 + height <= DISPLAY_HEIGHT, return);
    PRECOND_SOFT(dest != NULL, return);
    PRECOND(!swizzle || (width%16 == 0 && height%8 == 0 && dest_stride%16 == 0));

    uint32_t src_stride = DISPLAY_STRIDE;
    const uint32_t *src;
    if (is_offscreen) {
        src = (uint32_t *)psp_vram_spare_ptr() + (y0 * src_stride) + x0;
    } else {
        src = psp_work_pixel_address(x0, y0);
    }

    sceKernelDcacheWritebackInvalidateRange(src, src_stride * height * 4);
    ge_sync();

    if (vflip) {
        src += src_stride * (height-1);
        src_stride = -src_stride;
    }

    if (swizzle) {
        unsigned int y;
        for (y = 0; y < height; y += 8, src += src_stride*8) {
            unsigned int x;
            for (x = 0; x < width; x += 16) {
                const uint32_t *src_line = &src[x];
                unsigned int line;
                for (line = 0; line < 8; line++, src_line += src_stride, dest += 16) {
                    unsigned int pixel;
                    for (pixel = 0; pixel < width; pixel++) {
                        const uint32_t pixval = src_line[pixel];
                        const unsigned int r = pixval>> 0 & 0xFF;
                        const unsigned int g = pixval>> 8 & 0xFF;
                        const unsigned int b = pixval>>16 & 0xFF;
                        dest[x] = (r*19595 + g*38470 + b*7471 + 32768) >> 16;
                    }
                }
            }
        }
    } else {
        unsigned int y;
        for (y = 0; y < height; y++, src += src_stride, dest += dest_stride) {
            unsigned int x;
            for (x = 0; x < width; x++) {
                const uint32_t pixval = src[x];
                const unsigned int r = pixval>> 0 & 0xFF;
                const unsigned int g = pixval>> 8 & 0xFF;
                const unsigned int b = pixval>>16 & 0xFF;
                dest[x] = (r*19595 + g*38470 + b*7471 + 32768) >> 16;
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * fb_to_RGB:  Copy framebuffer data into an RGB (24bpp) buffer.
 *
 * [Parameters]
 *            x0, y0: Coordinates of first pixel to copy
 *     width, height: Size of region to copy
 *              dest: Destination buffer
 *       dest_stride: Destination buffer line length (pixels)
 *             vflip: Nonzero to flip the image data vertically
 * [Return value]
 *     None
 */
static void fb_to_RGB(unsigned int x0, unsigned int y0,
                      unsigned int width, unsigned int height,
                      uint8_t *dest, unsigned int dest_stride, int vflip)
{
    PRECOND_SOFT(x0 + width <= DISPLAY_WIDTH, return);
    PRECOND_SOFT(y0 + height <= DISPLAY_HEIGHT, return);
    PRECOND_SOFT(dest != NULL, return);

    uint32_t src_stride = DISPLAY_STRIDE;
    const uint32_t *src;
    if (is_offscreen) {
        src = (uint32_t *)psp_vram_spare_ptr() + (y0 * src_stride) + x0;
    } else {
        src = psp_work_pixel_address(x0, y0);
    }

    sceKernelDcacheWritebackInvalidateRange(src, src_stride * height * 4);
    ge_sync();

    if (vflip) {
        src += src_stride * (height-1);
        src_stride = -src_stride;
    }

    dest_stride *= 3;

    unsigned int y;
    for (y = 0; y < height; y++, src += src_stride, dest += dest_stride) {
        unsigned int x;
        for (x = 0; x < width; x++) {
            const uint32_t pixel = src[x];
            dest[x*3+0] = pixel>> 0 & 0xFF;
            dest[x*3+1] = pixel>> 8 & 0xFF;
            dest[x*3+2] = pixel>>16 & 0xFF;
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * fb_to_RGBA:  Copy framebuffer data into an RGBA buffer.
 *
 * [Parameters]
 *            x0, y0: Coordinates of first pixel to copy
 *     width, height: Size of region to copy
 *              dest: Destination buffer
 *       dest_stride: Destination buffer line length (pixels)
 *             vflip: Nonzero to flip the image data vertically
 *           swizzle: Nonzero to swizzle output data (requires width%4 == 0,
 *                       height%8 == 0, dest_stride%4 == 0)
 * [Return value]
 *     None
 */
static void fb_to_RGBA(unsigned int x0, unsigned int y0,
                       unsigned int width, unsigned int height,
                       uint32_t *dest, unsigned int dest_stride, int vflip,
                       int swizzle)
{
    PRECOND_SOFT(x0 + width <= DISPLAY_WIDTH, return);
    PRECOND_SOFT(y0 + height <= DISPLAY_HEIGHT, return);
    PRECOND_SOFT(dest != NULL, return);
    PRECOND(!swizzle || (width%4 == 0 && height%8 == 0 && dest_stride%4 == 0));

    uint32_t src_stride = DISPLAY_STRIDE;
    const uint32_t *src;
    if (is_offscreen) {
        src = (uint32_t *)psp_vram_spare_ptr() + (y0 * src_stride) + x0;
    } else {
        src = psp_work_pixel_address(x0, y0);
    }

    sceKernelDcacheWritebackInvalidateRange(src, src_stride * height * 4);
    ge_sync();

    if (vflip) {
        src += src_stride * (height-1);
        src_stride = -src_stride;
    }

    if (swizzle) {
        unsigned int y;
        for (y = 0; y < height; y += 8, src += src_stride*8) {
            unsigned int x;
            for (x = 0; x < width; x += 4) {
                const uint32_t *src_line = &src[x];
                unsigned int line;
                for (line = 0; line < 8; line++, src_line += src_stride, dest += 4) {
                    const uint32_t pixel0 = src_line[0];
                    const uint32_t pixel1 = src_line[1];
                    const uint32_t pixel2 = src_line[2];
                    const uint32_t pixel3 = src_line[3];
                    dest[0] = pixel0 | 0xFF000000;
                    dest[1] = pixel1 | 0xFF000000;
                    dest[2] = pixel2 | 0xFF000000;
                    dest[3] = pixel3 | 0xFF000000;
                }
            }
        }
    } else if (dest_stride % 4 == 0) {
        unsigned int y;
        for (y = 0; y < height; y++, src += src_stride, dest += dest_stride) {
            unsigned int x;
            for (x = 0; x < width; x += 4) {
                const uint32_t pixel0 = src[x+0];
                const uint32_t pixel1 = src[x+1];
                const uint32_t pixel2 = src[x+2];
                const uint32_t pixel3 = src[x+3];
                dest[x+0] = pixel0 | 0xFF000000;
                dest[x+1] = pixel1 | 0xFF000000;
                dest[x+2] = pixel2 | 0xFF000000;
                dest[x+3] = pixel3 | 0xFF000000;
            }
        }
    } else {
        unsigned int y;
        for (y = 0; y < height; y++, src += src_stride, dest += dest_stride) {
            unsigned int x;
            for (x = 0; x < width; x++) {
                dest[x] = src[x] | 0xFF000000;
            }
        }
    }
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
