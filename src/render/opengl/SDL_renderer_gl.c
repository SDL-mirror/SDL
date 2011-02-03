/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#if SDL_VIDEO_RENDER_OGL

#include "SDL_opengl.h"
#include "../SDL_sysrender.h"

#ifdef __MACOSX__
#include <OpenGL/OpenGL.h>
#endif


/* OpenGL renderer implementation */

/* Details on optimizing the texture path on Mac OS X:
   http://developer.apple.com/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_texturedata/chapter_10_section_2.html
*/

/* Used to re-create the window with OpenGL capability */
extern int SDL_RecreateWindow(SDL_Window * window, Uint32 flags);

static const float inv255f = 1.0f / 255.0f;

static SDL_Renderer *GL_CreateRenderer(SDL_Window * window, Uint32 flags);
static void GL_WindowEvent(SDL_Renderer * renderer,
                           const SDL_WindowEvent *event);
static int GL_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int GL_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * rect, const void *pixels,
                            int pitch);
static int GL_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * rect, void **pixels, int *pitch);
static void GL_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int GL_RenderClear(SDL_Renderer * renderer);
static int GL_RenderDrawPoints(SDL_Renderer * renderer,
                               const SDL_Point * points, int count);
static int GL_RenderDrawLines(SDL_Renderer * renderer,
                              const SDL_Point * points, int count);
static int GL_RenderFillRects(SDL_Renderer * renderer,
                              const SDL_Rect ** rects, int count);
static int GL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                         const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static int GL_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                               Uint32 pixel_format, void * pixels, int pitch);
static void GL_RenderPresent(SDL_Renderer * renderer);
static void GL_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void GL_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver GL_RenderDriver = {
    GL_CreateRenderer,
    {
     "opengl",
     (SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED),
     1,
     {SDL_PIXELFORMAT_ARGB8888},
     0,
     0}
};

typedef struct
{
    SDL_GLContext context;
    SDL_bool updateSize;
    SDL_bool GL_ARB_texture_rectangle_supported;
    int blendMode;

    /* OpenGL functions */
#define SDL_PROC(ret,func,params) ret (APIENTRY *func) params;
#include "../../video/SDL_glfuncs.h"
#undef SDL_PROC

    void (*glTextureRangeAPPLE) (GLenum target, GLsizei length,
                                 const GLvoid * pointer);
} GL_RenderData;

typedef struct
{
    GLuint texture;
    GLenum type;
    GLfloat texw;
    GLfloat texh;
    GLenum format;
    GLenum formattype;
    void *pixels;
    int pitch;
} GL_TextureData;


static void
GL_SetError(const char *prefix, GLenum result)
{
    const char *error;

    switch (result) {
    case GL_NO_ERROR:
        error = "GL_NO_ERROR";
        break;
    case GL_INVALID_ENUM:
        error = "GL_INVALID_ENUM";
        break;
    case GL_INVALID_VALUE:
        error = "GL_INVALID_VALUE";
        break;
    case GL_INVALID_OPERATION:
        error = "GL_INVALID_OPERATION";
        break;
    case GL_STACK_OVERFLOW:
        error = "GL_STACK_OVERFLOW";
        break;
    case GL_STACK_UNDERFLOW:
        error = "GL_STACK_UNDERFLOW";
        break;
    case GL_OUT_OF_MEMORY:
        error = "GL_OUT_OF_MEMORY";
        break;
    case GL_TABLE_TOO_LARGE:
        error = "GL_TABLE_TOO_LARGE";
        break;
    default:
        error = "UNKNOWN";
        break;
    }
    SDL_SetError("%s: %s", prefix, error);
}

static int
GL_LoadFunctions(GL_RenderData * data)
{
#ifdef __SDL_NOGETPROCADDR__
#define SDL_PROC(ret,func,params) data->func=func;
#else
#define SDL_PROC(ret,func,params) \
    do { \
        data->func = SDL_GL_GetProcAddress(#func); \
        if ( ! data->func ) { \
            SDL_SetError("Couldn't load GL function %s: %s\n", #func, SDL_GetError()); \
            return -1; \
        } \
    } while ( 0 );
#endif /* __SDL_NOGETPROCADDR__ */

#include "../../video/SDL_glfuncs.h"
#undef SDL_PROC
    return 0;
}

SDL_Renderer *
GL_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_Renderer *renderer;
    GL_RenderData *data;
    GLint value;
    Uint32 window_flags;

    window_flags = SDL_GetWindowFlags(window);
    if (!(window_flags & SDL_WINDOW_OPENGL)) {
        if (SDL_RecreateWindow(window, window_flags | SDL_WINDOW_OPENGL) < 0) {
            return NULL;
        }
    }

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (GL_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        GL_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }

    renderer->WindowEvent = GL_WindowEvent;
    renderer->CreateTexture = GL_CreateTexture;
    renderer->UpdateTexture = GL_UpdateTexture;
    renderer->LockTexture = GL_LockTexture;
    renderer->UnlockTexture = GL_UnlockTexture;
    renderer->RenderClear = GL_RenderClear;
    renderer->RenderDrawPoints = GL_RenderDrawPoints;
    renderer->RenderDrawLines = GL_RenderDrawLines;
    renderer->RenderFillRects = GL_RenderFillRects;
    renderer->RenderCopy = GL_RenderCopy;
    renderer->RenderReadPixels = GL_RenderReadPixels;
    renderer->RenderPresent = GL_RenderPresent;
    renderer->DestroyTexture = GL_DestroyTexture;
    renderer->DestroyRenderer = GL_DestroyRenderer;
    renderer->info = GL_RenderDriver.info;
    renderer->driverdata = data;

    renderer->info.flags = SDL_RENDERER_ACCELERATED;

    if (GL_LoadFunctions(data) < 0) {
        GL_DestroyRenderer(renderer);
        return NULL;
    }

    data->context = SDL_GL_CreateContext(window);
    if (!data->context) {
        GL_DestroyRenderer(renderer);
        return NULL;
    }
    if (SDL_GL_MakeCurrent(window, data->context) < 0) {
        GL_DestroyRenderer(renderer);
        return NULL;
    }
#ifdef __MACOSX__
    /* Enable multi-threaded rendering */
    /* Disabled until Ryan finishes his VBO/PBO code...
       CGLEnable(CGLGetCurrentContext(), kCGLCEMPEngine);
     */
#endif

    if (flags & SDL_RENDERER_PRESENTVSYNC) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }
    if (SDL_GL_GetSwapInterval() > 0) {
        renderer->info.flags |= SDL_RENDERER_PRESENTVSYNC;
    }

    data->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
    renderer->info.max_texture_width = value;
    data->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
    renderer->info.max_texture_height = value;

    if (SDL_GL_ExtensionSupported("GL_ARB_texture_rectangle")
        || SDL_GL_ExtensionSupported("GL_EXT_texture_rectangle")) {
        data->GL_ARB_texture_rectangle_supported = SDL_TRUE;
    }
    if (SDL_GL_ExtensionSupported("GL_APPLE_texture_range")) {
        data->glTextureRangeAPPLE =
            (void (*)(GLenum, GLsizei, const GLvoid *))
            SDL_GL_GetProcAddress("glTextureRangeAPPLE");
    }

    /* Set up parameters for rendering */
    data->blendMode = -1;
    data->glDisable(GL_DEPTH_TEST);
    data->glDisable(GL_CULL_FACE);
    /* This ended up causing video discrepancies between OpenGL and Direct3D */
    /*data->glEnable(GL_LINE_SMOOTH);*/
    if (data->GL_ARB_texture_rectangle_supported) {
        data->glEnable(GL_TEXTURE_RECTANGLE_ARB);
    } else {
        data->glEnable(GL_TEXTURE_2D);
    }
    data->updateSize = SDL_TRUE;

    return renderer;
}

static SDL_GLContext SDL_CurrentContext = NULL;

static int
GL_ActivateRenderer(SDL_Renderer * renderer)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;

    if (SDL_CurrentContext != data->context) {
        if (SDL_GL_MakeCurrent(window, data->context) < 0) {
            return -1;
        }
        SDL_CurrentContext = data->context;
    }
    if (data->updateSize) {
        int w, h;

        SDL_GetWindowSize(window, &w, &h);
        data->glMatrixMode(GL_PROJECTION);
        data->glLoadIdentity();
        data->glMatrixMode(GL_MODELVIEW);
        data->glLoadIdentity();
        data->glViewport(0, 0, w, h);
        data->glOrtho(0.0, (GLdouble) w, (GLdouble) h, 0.0, 0.0, 1.0);
        data->updateSize = SDL_FALSE;
    }
    return 0;
}

static void
GL_WindowEvent(SDL_Renderer * renderer, const SDL_WindowEvent *event)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;

    if (event->event == SDL_WINDOWEVENT_RESIZED) {
        /* Rebind the context to the window area and update matrices */
        SDL_CurrentContext = NULL;
        data->updateSize = SDL_TRUE;
    }
}

static __inline__ int
power_of_2(int input)
{
    int value = 1;

    while (value < input) {
        value <<= 1;
    }
    return value;
}

static __inline__ SDL_bool
convert_format(GL_RenderData *renderdata, Uint32 pixel_format,
               GLint* internalFormat, GLenum* format, GLenum* type)
{
    switch (pixel_format) {
    case SDL_PIXELFORMAT_RGB888:
    case SDL_PIXELFORMAT_ARGB8888:
        *internalFormat = GL_RGBA8;
        *format = GL_BGRA;
        *type = GL_UNSIGNED_INT_8_8_8_8_REV;
        break;
    default:
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static int
GL_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *data;
    GLint internalFormat;
    GLenum format, type;
    int texture_w, texture_h;
    GLenum result;

    GL_ActivateRenderer(renderer);

    if (!convert_format(renderdata, texture->format, &internalFormat,
                        &format, &type)) {
        SDL_SetError("Texture format %s not supported by OpenGL",
                     SDL_GetPixelFormatName(texture->format));
        return -1;
    }

    data = (GL_TextureData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }

    if (texture->access == SDL_TEXTUREACCESS_STREAMING) {
        data->pitch = texture->w * SDL_BYTESPERPIXEL(texture->format);
        data->pixels = SDL_malloc(texture->h * data->pitch);
        if (!data->pixels) {
            SDL_OutOfMemory();
            SDL_free(data);
            return -1;
        }
    }

    texture->driverdata = data;

    renderdata->glGetError();
    renderdata->glGenTextures(1, &data->texture);
    if (renderdata->GL_ARB_texture_rectangle_supported) {
        data->type = GL_TEXTURE_RECTANGLE_ARB;
        texture_w = texture->w;
        texture_h = texture->h;
        data->texw = (GLfloat) texture_w;
        data->texh = (GLfloat) texture_h;
    } else {
        data->type = GL_TEXTURE_2D;
        texture_w = power_of_2(texture->w);
        texture_h = power_of_2(texture->h);
        data->texw = (GLfloat) (texture->w) / texture_w;
        data->texh = (GLfloat) texture->h / texture_h;
    }

    data->format = format;
    data->formattype = type;
    renderdata->glEnable(data->type);
    renderdata->glBindTexture(data->type, data->texture);
    renderdata->glTexParameteri(data->type, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);
    renderdata->glTexParameteri(data->type, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);
    renderdata->glTexParameteri(data->type, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE);
    renderdata->glTexParameteri(data->type, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE);
#ifdef __MACOSX__
#ifndef GL_TEXTURE_STORAGE_HINT_APPLE
#define GL_TEXTURE_STORAGE_HINT_APPLE       0x85BC
#endif
#ifndef STORAGE_CACHED_APPLE
#define STORAGE_CACHED_APPLE                0x85BE
#endif
#ifndef STORAGE_SHARED_APPLE
#define STORAGE_SHARED_APPLE                0x85BF
#endif
    if (texture->access == SDL_TEXTUREACCESS_STREAMING) {
        renderdata->glTexParameteri(data->type, GL_TEXTURE_STORAGE_HINT_APPLE,
                                    GL_STORAGE_SHARED_APPLE);
    } else {
        renderdata->glTexParameteri(data->type, GL_TEXTURE_STORAGE_HINT_APPLE,
                                    GL_STORAGE_CACHED_APPLE);
    }
    if (texture->access == SDL_TEXTUREACCESS_STREAMING
        && texture->format == SDL_PIXELFORMAT_ARGB8888) {
        renderdata->glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        renderdata->glTexImage2D(data->type, 0, internalFormat, texture_w,
                                 texture_h, 0, format, type, data->pixels);
    }
    else
#endif
    {
        renderdata->glTexImage2D(data->type, 0, internalFormat, texture_w,
                                 texture_h, 0, format, type, NULL);
    }
    renderdata->glDisable(data->type);
    result = renderdata->glGetError();
    if (result != GL_NO_ERROR) {
        GL_SetError("glTexImage2D()", result);
        return -1;
    }
    return 0;
}

static void
SetupTextureUpdate(GL_RenderData * renderdata, SDL_Texture * texture,
                   int pitch)
{
    renderdata->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    renderdata->glPixelStorei(GL_UNPACK_ROW_LENGTH,
                              (pitch / SDL_BYTESPERPIXEL(texture->format)));
}

static int
GL_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 const SDL_Rect * rect, const void *pixels, int pitch)
{
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;
    GLenum result;

    GL_ActivateRenderer(renderer);

    renderdata->glGetError();
    SetupTextureUpdate(renderdata, texture, pitch);
    renderdata->glEnable(data->type);
    renderdata->glBindTexture(data->type, data->texture);
    renderdata->glTexSubImage2D(data->type, 0, rect->x, rect->y, rect->w,
                                rect->h, data->format, data->formattype,
                                pixels);
    renderdata->glDisable(data->type);
    result = renderdata->glGetError();
    if (result != GL_NO_ERROR) {
        GL_SetError("glTexSubImage2D()", result);
        return -1;
    }
    return 0;
}

static int
GL_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * rect, void **pixels, int *pitch)
{
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;

    *pixels =
        (void *) ((Uint8 *) data->pixels + rect->y * data->pitch +
                  rect->x * SDL_BYTESPERPIXEL(texture->format));
    *pitch = data->pitch;
    return 0;
}

static void
GL_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;

    GL_ActivateRenderer(renderer);

    SetupTextureUpdate(renderdata, texture, data->pitch);
    renderdata->glEnable(data->type);
    renderdata->glBindTexture(data->type, data->texture);
    renderdata->glTexSubImage2D(data->type, 0, 0, 0, texture->w, texture->h,
                                data->format, data->formattype, data->pixels);
    renderdata->glDisable(data->type);
}

static void
GL_SetBlendMode(GL_RenderData * data, int blendMode)
{
    if (blendMode != data->blendMode) {
        switch (blendMode) {
        case SDL_BLENDMODE_NONE:
            data->glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            data->glDisable(GL_BLEND);
            break;
        case SDL_BLENDMODE_BLEND:
            data->glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            data->glEnable(GL_BLEND);
            data->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case SDL_BLENDMODE_ADD:
            data->glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            data->glEnable(GL_BLEND);
            data->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        }
        data->blendMode = blendMode;
    }
}

static int
GL_RenderClear(SDL_Renderer * renderer)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;

    GL_ActivateRenderer(renderer);

    data->glClearColor((GLfloat) renderer->r * inv255f,
                       (GLfloat) renderer->g * inv255f,
                       (GLfloat) renderer->b * inv255f,
                       (GLfloat) renderer->a * inv255f);

    data->glClear(GL_COLOR_BUFFER_BIT);

    return 0;
}

static int
GL_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                    int count)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    int i;

    GL_ActivateRenderer(renderer);

    GL_SetBlendMode(data, renderer->blendMode);

    data->glColor4f((GLfloat) renderer->r * inv255f,
                    (GLfloat) renderer->g * inv255f,
                    (GLfloat) renderer->b * inv255f,
                    (GLfloat) renderer->a * inv255f);

    data->glBegin(GL_POINTS);
    for (i = 0; i < count; ++i) {
        data->glVertex2f(0.5f + points[i].x, 0.5f + points[i].y);
    }
    data->glEnd();

    return 0;
}

static int
GL_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                   int count)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    int i;

    GL_ActivateRenderer(renderer);

    GL_SetBlendMode(data, renderer->blendMode);

    data->glColor4f((GLfloat) renderer->r * inv255f,
                    (GLfloat) renderer->g * inv255f,
                    (GLfloat) renderer->b * inv255f,
                    (GLfloat) renderer->a * inv255f);

    if (count > 2 && 
        points[0].x == points[count-1].x && points[0].y == points[count-1].y) {
        data->glBegin(GL_LINE_LOOP);
        /* GL_LINE_LOOP takes care of the final segment */
        --count;
        for (i = 0; i < count; ++i) {
            data->glVertex2f(0.5f + points[i].x, 0.5f + points[i].y);
        }
        data->glEnd();
    } else {
#if defined(__APPLE__) || defined(__WIN32__)
#else
        int x1, y1, x2, y2;
#endif

        data->glBegin(GL_LINE_STRIP);
        for (i = 0; i < count; ++i) {
            data->glVertex2f(0.5f + points[i].x, 0.5f + points[i].y);
        }
        data->glEnd();

        /* The line is half open, so we need one more point to complete it.
         * http://www.opengl.org/documentation/specs/version1.1/glspec1.1/node47.html
         * If we have to, we can use vertical line and horizontal line textures
         * for vertical and horizontal lines, and then create custom textures
         * for diagonal lines and software render those.  It's terrible, but at
         * least it would be pixel perfect.
         */
        data->glBegin(GL_POINTS);
#if defined(__APPLE__) || defined(__WIN32__)
        /* Mac OS X and Windows seem to always leave the second point open */
        data->glVertex2f(0.5f + points[count-1].x, 0.5f + points[count-1].y);
#else
        /* Linux seems to leave the right-most or bottom-most point open */
        x1 = points[0].x;
        y1 = points[0].y;
        x2 = points[count-1].x;
        y2 = points[count-1].y;

        if (x1 > x2) {
            data->glVertex2f(0.5f + x1, 0.5f + y1);
        } else if (x2 > x1) {
            data->glVertex2f(0.5f + x2, 0.5f + y2);
        } else if (y1 > y2) {
            data->glVertex2f(0.5f + x1, 0.5f + y1);
        } else if (y2 > y1) {
            data->glVertex2f(0.5f + x2, 0.5f + y2);
        }
#endif
        data->glEnd();
    }

    return 0;
}

static int
GL_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects, int count)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    int i;

    GL_ActivateRenderer(renderer);

    GL_SetBlendMode(data, renderer->blendMode);

    data->glColor4f((GLfloat) renderer->r * inv255f,
                    (GLfloat) renderer->g * inv255f,
                    (GLfloat) renderer->b * inv255f,
                    (GLfloat) renderer->a * inv255f);

    for (i = 0; i < count; ++i) {
        const SDL_Rect *rect = rects[i];

        data->glRecti(rect->x, rect->y, rect->x + rect->w, rect->y + rect->h);
    }

    return 0;
}

static int
GL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
              const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *texturedata = (GL_TextureData *) texture->driverdata;
    int minx, miny, maxx, maxy;
    GLfloat minu, maxu, minv, maxv;

    GL_ActivateRenderer(renderer);

    minx = dstrect->x;
    miny = dstrect->y;
    maxx = dstrect->x + dstrect->w;
    maxy = dstrect->y + dstrect->h;

    minu = (GLfloat) srcrect->x / texture->w;
    minu *= texturedata->texw;
    maxu = (GLfloat) (srcrect->x + srcrect->w) / texture->w;
    maxu *= texturedata->texw;
    minv = (GLfloat) srcrect->y / texture->h;
    minv *= texturedata->texh;
    maxv = (GLfloat) (srcrect->y + srcrect->h) / texture->h;
    maxv *= texturedata->texh;

    data->glEnable(texturedata->type);
    data->glBindTexture(texturedata->type, texturedata->texture);

    if (texture->modMode) {
        data->glColor4f((GLfloat) texture->r * inv255f,
                        (GLfloat) texture->g * inv255f,
                        (GLfloat) texture->b * inv255f,
                        (GLfloat) texture->a * inv255f);
    } else {
        data->glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    GL_SetBlendMode(data, texture->blendMode);

    data->glBegin(GL_TRIANGLE_STRIP);
    data->glTexCoord2f(minu, minv);
    data->glVertex2f((GLfloat) minx, (GLfloat) miny);
    data->glTexCoord2f(maxu, minv);
    data->glVertex2f((GLfloat) maxx, (GLfloat) miny);
    data->glTexCoord2f(minu, maxv);
    data->glVertex2f((GLfloat) minx, (GLfloat) maxy);
    data->glTexCoord2f(maxu, maxv);
    data->glVertex2f((GLfloat) maxx, (GLfloat) maxy);
    data->glEnd();

    data->glDisable(texturedata->type);

    return 0;
}

static int
GL_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                    Uint32 pixel_format, void * pixels, int pitch)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    GLint internalFormat;
    GLenum format, type;
    Uint8 *src, *dst, *tmp;
    int w, h, length, rows;

    GL_ActivateRenderer(renderer);

    if (!convert_format(data, pixel_format, &internalFormat, &format, &type)) {
        /* FIXME: Do a temp copy to a format that is supported */
        SDL_SetError("Unsupported pixel format");
        return -1;
    }

    SDL_GetWindowSize(window, &w, &h);

    data->glPixelStorei(GL_PACK_ALIGNMENT, 1);
    data->glPixelStorei(GL_PACK_ROW_LENGTH,
                        (pitch / SDL_BYTESPERPIXEL(pixel_format)));

    data->glReadPixels(rect->x, (h-rect->y)-rect->h, rect->w, rect->h,
                       format, type, pixels);

    /* Flip the rows to be top-down */
    length = rect->w * SDL_BYTESPERPIXEL(pixel_format);
    src = (Uint8*)pixels + (rect->h-1)*pitch;
    dst = (Uint8*)pixels;
    tmp = SDL_stack_alloc(Uint8, length);
    rows = rect->h / 2;
    while (rows--) {
        SDL_memcpy(tmp, dst, length);
        SDL_memcpy(dst, src, length);
        SDL_memcpy(src, tmp, length);
        dst += pitch;
        src -= pitch;
    }
    SDL_stack_free(tmp);

    return 0;
}

static void
GL_RenderPresent(SDL_Renderer * renderer)
{
    GL_ActivateRenderer(renderer);

    SDL_GL_SwapWindow(renderer->window);
}

static void
GL_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;

    GL_ActivateRenderer(renderer);

    if (!data) {
        return;
    }
    if (data->texture) {
        renderdata->glDeleteTextures(1, &data->texture);
    }
    if (data->pixels) {
        SDL_free(data->pixels);
    }
    SDL_free(data);
    texture->driverdata = NULL;
}

static void
GL_DestroyRenderer(SDL_Renderer * renderer)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;

    if (data) {
        if (data->context) {
            /* SDL_GL_MakeCurrent(0, NULL); *//* doesn't do anything */
            SDL_GL_DeleteContext(data->context);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_RENDER_OGL */

/* vi: set ts=4 sw=4 expandtab: */
