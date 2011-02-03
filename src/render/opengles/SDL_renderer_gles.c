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

#if SDL_VIDEO_RENDER_OGL_ES

#include "SDL_opengles.h"
#include "../SDL_sysrender.h"

#if defined(SDL_VIDEO_DRIVER_PANDORA)

/* Empty function stub to get OpenGL ES 1.x support without  */
/* OpenGL ES extension GL_OES_draw_texture supported         */
GL_API void GL_APIENTRY
glDrawTexiOES(GLint x, GLint y, GLint z, GLint width, GLint height)
{
    return;
}

#endif /* PANDORA */

/* OpenGL ES 1.1 renderer implementation, based on the OpenGL renderer */

/* Used to re-create the window with OpenGL capability */
extern int SDL_RecreateWindow(SDL_Window * window, Uint32 flags);

static const float inv255f = 1.0f / 255.0f;

static SDL_Renderer *GLES_CreateRenderer(SDL_Window * window, Uint32 flags);
static void GLES_WindowEvent(SDL_Renderer * renderer,
                             const SDL_WindowEvent *event);
static int GLES_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int GLES_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                              const SDL_Rect * rect, const void *pixels,
                              int pitch);
static int GLES_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * rect, void **pixels, int *pitch);
static void GLES_UnlockTexture(SDL_Renderer * renderer,
                               SDL_Texture * texture);
static int GLES_RenderDrawPoints(SDL_Renderer * renderer,
                                 const SDL_Point * points, int count);
static int GLES_RenderDrawLines(SDL_Renderer * renderer,
                                const SDL_Point * points, int count);
static int GLES_RenderFillRects(SDL_Renderer * renderer,
                                const SDL_Rect ** rects, int count);
static int GLES_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                           const SDL_Rect * srcrect,
                           const SDL_Rect * dstrect);
static void GLES_RenderPresent(SDL_Renderer * renderer);
static void GLES_DestroyTexture(SDL_Renderer * renderer,
                                SDL_Texture * texture);
static void GLES_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver GL_ES_RenderDriver = {
    GLES_CreateRenderer,
    {
     "opengl_es",
     (SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED),
     1,
     {SDL_PIXELFORMAT_ABGR8888},
     0,
     0}
};

typedef struct
{
    SDL_GLContext context;
    SDL_bool updateSize;
    int blendMode;

#ifndef APIENTRY
#define APIENTRY
#endif

    SDL_bool useDrawTexture;
    SDL_bool GL_OES_draw_texture_supported;

    /* OpenGL ES functions */
#define SDL_PROC(ret,func,params) ret (APIENTRY *func) params;
#include "../../video/SDL_glesfuncs.h"
#undef SDL_PROC

} GLES_RenderData;

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
} GLES_TextureData;

static void
GLES_SetError(const char *prefix, GLenum result)
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
    default:
        error = "UNKNOWN";
        break;
    }
    SDL_SetError("%s: %s", prefix, error);
}

static int
GLES_LoadFunctions(GLES_RenderData * data)
{

#define SDL_PROC(ret,func,params) \
    data->func = func;
#include "../../video/SDL_glesfuncs.h"
#undef SDL_PROC

    return 0;
}

SDL_Renderer *
GLES_CreateRenderer(SDL_Window * window, Uint32 flags)
{

    SDL_Renderer *renderer;
    GLES_RenderData *data;
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

    data = (GLES_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        GLES_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }

    renderer->WindowEvent = GLES_WindowEvent;
    renderer->CreateTexture = GLES_CreateTexture;
    renderer->UpdateTexture = GLES_UpdateTexture;
    renderer->LockTexture = GLES_LockTexture;
    renderer->UnlockTexture = GLES_UnlockTexture;
    renderer->RenderDrawPoints = GLES_RenderDrawPoints;
    renderer->RenderDrawLines = GLES_RenderDrawLines;
    renderer->RenderFillRects = GLES_RenderFillRects;
    renderer->RenderCopy = GLES_RenderCopy;
    renderer->RenderPresent = GLES_RenderPresent;
    renderer->DestroyTexture = GLES_DestroyTexture;
    renderer->DestroyRenderer = GLES_DestroyRenderer;
    renderer->info = GL_ES_RenderDriver.info;
    renderer->driverdata = data;

    renderer->info.flags = SDL_RENDERER_ACCELERATED;

    if (GLES_LoadFunctions(data) < 0) {
        GLES_DestroyRenderer(renderer);
        return NULL;
    }

    data->context = SDL_GL_CreateContext(window);
    if (!data->context) {
        GLES_DestroyRenderer(renderer);
        return NULL;
    }
    if (SDL_GL_MakeCurrent(window, data->context) < 0) {
        GLES_DestroyRenderer(renderer);
        return NULL;
    }

    if (flags & SDL_RENDERER_PRESENTVSYNC) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }
    if (SDL_GL_GetSwapInterval() > 0) {
        renderer->info.flags |= SDL_RENDERER_PRESENTVSYNC;
    }

#if SDL_VIDEO_DRIVER_PANDORA
    data->GL_OES_draw_texture_supported = SDL_FALSE;
    data->useDrawTexture = SDL_FALSE;
#else
    if (SDL_GL_ExtensionSupported("GL_OES_draw_texture")) {
        data->GL_OES_draw_texture_supported = SDL_TRUE;
        data->useDrawTexture = SDL_TRUE;
    } else {
        data->GL_OES_draw_texture_supported = SDL_FALSE;
        data->useDrawTexture = SDL_FALSE;
    }
#endif

    data->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
    renderer->info.max_texture_width = value;
    data->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
    renderer->info.max_texture_height = value;

    /* Set up parameters for rendering */
    data->blendMode = -1;
    data->glDisable(GL_DEPTH_TEST);
    data->glDisable(GL_CULL_FACE);
    data->updateSize = SDL_TRUE;

    data->glEnableClientState(GL_VERTEX_ARRAY);
    data->glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    return renderer;
}

static SDL_GLContext SDL_CurrentContext = NULL;

static int
GLES_ActivateRenderer(SDL_Renderer * renderer)
{

    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;
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
        data->glOrthof(0.0, (GLfloat) w, (GLfloat) h, 0.0, 0.0, 1.0);
        data->updateSize = SDL_FALSE;
    }
    return 0;
}

static void
GLES_WindowEvent(SDL_Renderer * renderer, const SDL_WindowEvent *event)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;

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

static int
GLES_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GLES_RenderData *renderdata = (GLES_RenderData *) renderer->driverdata;
    GLES_TextureData *data;
    GLint internalFormat;
    GLenum format, type;
    int texture_w, texture_h;
    GLenum result;

    GLES_ActivateRenderer(renderer);

    switch (texture->format) {
    case SDL_PIXELFORMAT_ABGR8888:
        internalFormat = GL_RGBA;
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    default:
        SDL_SetError("Texture format %s not supported by OpenGL ES",
                     SDL_GetPixelFormatName(texture->format));
        return -1;
    }

    data = (GLES_TextureData *) SDL_calloc(1, sizeof(*data));
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
    renderdata->glEnable(GL_TEXTURE_2D);
    renderdata->glGenTextures(1, &data->texture);

    data->type = GL_TEXTURE_2D;
    /* no NPOV textures allowed in OpenGL ES (yet) */
    texture_w = power_of_2(texture->w);
    texture_h = power_of_2(texture->h);
    data->texw = (GLfloat) texture->w / texture_w;
    data->texh = (GLfloat) texture->h / texture_h;

    data->format = format;
    data->formattype = type;
    renderdata->glBindTexture(data->type, data->texture);
    renderdata->glTexParameteri(data->type, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);
    renderdata->glTexParameteri(data->type, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);
    renderdata->glTexParameteri(data->type, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE);
    renderdata->glTexParameteri(data->type, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE);

    renderdata->glTexImage2D(data->type, 0, internalFormat, texture_w,
                             texture_h, 0, format, type, NULL);
    renderdata->glDisable(GL_TEXTURE_2D);

    result = renderdata->glGetError();
    if (result != GL_NO_ERROR) {
        GLES_SetError("glTexImage2D()", result);
        return -1;
    }
    return 0;
}

static void
SetupTextureUpdate(GLES_RenderData * renderdata, SDL_Texture * texture,
                   int pitch)
{
    renderdata->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

static int
GLES_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                   const SDL_Rect * rect, const void *pixels, int pitch)
{
    GLES_RenderData *renderdata = (GLES_RenderData *) renderer->driverdata;
    GLES_TextureData *data = (GLES_TextureData *) texture->driverdata;
    GLenum result;
    int bpp = SDL_BYTESPERPIXEL(texture->format);
    void * temp_buffer;
    void * temp_ptr;
    int i;

    GLES_ActivateRenderer(renderer);

    renderdata->glGetError();
    SetupTextureUpdate(renderdata, texture, pitch);
    renderdata->glEnable(data->type);
    renderdata->glBindTexture(data->type, data->texture);

    if( rect->w * bpp == pitch ) {
         temp_buffer = (void *)pixels; /* No need to reformat */
    } else {
         /* Reformatting of mem area required */
         temp_buffer = SDL_malloc(rect->w * rect->h * bpp);
         temp_ptr = temp_buffer;
         for (i = 0; i < rect->h; i++) {
             SDL_memcpy(temp_ptr, pixels, rect->w * bpp);
             temp_ptr += rect->w * bpp;
             pixels += pitch;
         }
    }

    renderdata->glTexSubImage2D(data->type, 0, rect->x, rect->y, rect->w,
                                rect->h, data->format, data->formattype,
                                temp_buffer);

    if( temp_buffer != pixels ) {
        SDL_free(temp_buffer);
    }

    renderdata->glDisable(data->type);
    result = renderdata->glGetError();
    if (result != GL_NO_ERROR) {
        GLES_SetError("glTexSubImage2D()", result);
        return -1;
    }
    return 0;
}

static int
GLES_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 const SDL_Rect * rect, void **pixels, int *pitch)
{
    GLES_TextureData *data = (GLES_TextureData *) texture->driverdata;

    *pixels =
        (void *) ((Uint8 *) data->pixels + rect->y * data->pitch +
                  rect->x * SDL_BYTESPERPIXEL(texture->format));
    *pitch = data->pitch;
    return 0;
}

static void
GLES_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GLES_RenderData *renderdata = (GLES_RenderData *) renderer->driverdata;
    GLES_TextureData *data = (GLES_TextureData *) texture->driverdata;

    GLES_ActivateRenderer(renderer);

    SetupTextureUpdate(renderdata, texture, data->pitch);
    renderdata->glEnable(data->type);
    renderdata->glBindTexture(data->type, data->texture);
    renderdata->glTexSubImage2D(data->type, 0, 0, 0, texture->w,
                                texture->h, data->format, data->formattype,
                                data->pixels);
    renderdata->glDisable(data->type);
}

static void
GLES_SetBlendMode(GLES_RenderData * data, int blendMode)
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
GLES_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                      int count)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;
    int i;
    GLshort *vertices;

    GLES_ActivateRenderer(renderer);

    GLES_SetBlendMode(data, renderer->blendMode);

    data->glColor4f((GLfloat) renderer->r * inv255f,
                    (GLfloat) renderer->g * inv255f,
                    (GLfloat) renderer->b * inv255f,
                    (GLfloat) renderer->a * inv255f);

    vertices = SDL_stack_alloc(GLshort, count*2);
    for (i = 0; i < count; ++i) {
        vertices[2*i+0] = (GLshort)points[i].x;
        vertices[2*i+1] = (GLshort)points[i].y;
    }
    data->glVertexPointer(2, GL_SHORT, 0, vertices);
    data->glDrawArrays(GL_POINTS, 0, count);
    SDL_stack_free(vertices);

    return 0;
}

static int
GLES_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                     int count)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;
    int i;
    GLshort *vertices;

    GLES_ActivateRenderer(renderer);

    GLES_SetBlendMode(data, renderer->blendMode);

    data->glColor4f((GLfloat) renderer->r * inv255f,
                    (GLfloat) renderer->g * inv255f,
                    (GLfloat) renderer->b * inv255f,
                    (GLfloat) renderer->a * inv255f);

    vertices = SDL_stack_alloc(GLshort, count*2);
    for (i = 0; i < count; ++i) {
        vertices[2*i+0] = (GLshort)points[i].x;
        vertices[2*i+1] = (GLshort)points[i].y;
    }
    data->glVertexPointer(2, GL_SHORT, 0, vertices);
    if (count > 2 && 
        points[0].x == points[count-1].x && points[0].y == points[count-1].y) {
        /* GL_LINE_LOOP takes care of the final segment */
        --count;
        data->glDrawArrays(GL_LINE_LOOP, 0, count);
    } else {
        data->glDrawArrays(GL_LINE_STRIP, 0, count);
    }
    SDL_stack_free(vertices);

    return 0;
}

static int
GLES_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects,
                     int count)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;
    int i;

    GLES_ActivateRenderer(renderer);

    GLES_SetBlendMode(data, renderer->blendMode);

    data->glColor4f((GLfloat) renderer->r * inv255f,
                    (GLfloat) renderer->g * inv255f,
                    (GLfloat) renderer->b * inv255f,
                    (GLfloat) renderer->a * inv255f);

    for (i = 0; i < count; ++i) {
        const SDL_Rect *rect = rects[i];
        GLshort minx = rect->x;
        GLshort maxx = rect->x + rect->w;
        GLshort miny = rect->y;
        GLshort maxy = rect->y + rect->h;
        GLshort vertices[8];
        vertices[0] = minx;
        vertices[1] = miny;
        vertices[2] = maxx;
        vertices[3] = miny;
        vertices[4] = minx;
        vertices[5] = maxy;
        vertices[6] = maxx;
        vertices[7] = maxy;

        data->glVertexPointer(2, GL_SHORT, 0, vertices);
        data->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    return 0;
}

static int
GLES_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{

    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;
    GLES_TextureData *texturedata = (GLES_TextureData *) texture->driverdata;
    int minx, miny, maxx, maxy;
    GLfloat minu, maxu, minv, maxv;
    int i;
    void *temp_buffer;          /* used for reformatting dirty rect pixels */
    void *temp_ptr;

    GLES_ActivateRenderer(renderer);

    data->glEnable(GL_TEXTURE_2D);

    data->glBindTexture(texturedata->type, texturedata->texture);

    if (texture->modMode) {
        data->glColor4f((GLfloat) texture->r * inv255f,
                        (GLfloat) texture->g * inv255f,
                        (GLfloat) texture->b * inv255f,
                        (GLfloat) texture->a * inv255f);
    } else {
        data->glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    GLES_SetBlendMode(data, texture->blendMode);

    if (data->GL_OES_draw_texture_supported && data->useDrawTexture) {
        /* this code is a little funny because the viewport is upside down vs SDL's coordinate system */
        GLint cropRect[4];
        int w, h;
        SDL_Window *window = renderer->window;

        SDL_GetWindowSize(window, &w, &h);
        cropRect[0] = srcrect->x;
        cropRect[1] = srcrect->y + srcrect->h;
        cropRect[2] = srcrect->w;
        cropRect[3] = -srcrect->h;
        data->glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES,
                               cropRect);
        data->glDrawTexiOES(dstrect->x, h - dstrect->y - dstrect->h, 0,
                            dstrect->w, dstrect->h);
    } else {

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

        GLshort vertices[8];
        GLfloat texCoords[8];

        vertices[0] = minx;
        vertices[1] = miny;
        vertices[2] = maxx;
        vertices[3] = miny;
        vertices[4] = minx;
        vertices[5] = maxy;
        vertices[6] = maxx;
        vertices[7] = maxy;

        texCoords[0] = minu;
        texCoords[1] = minv;
        texCoords[2] = maxu;
        texCoords[3] = minv;
        texCoords[4] = minu;
        texCoords[5] = maxv;
        texCoords[6] = maxu;
        texCoords[7] = maxv;

        data->glVertexPointer(2, GL_SHORT, 0, vertices);
        data->glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
        data->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    data->glDisable(GL_TEXTURE_2D);

    return 0;
}

static void
GLES_RenderPresent(SDL_Renderer * renderer)
{
    GLES_ActivateRenderer(renderer);

    SDL_GL_SwapWindow(renderer->window);
}

static void
GLES_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GLES_TextureData *data = (GLES_TextureData *) texture->driverdata;

    GLES_ActivateRenderer(renderer);

    if (!data) {
        return;
    }
    if (data->texture) {
        glDeleteTextures(1, &data->texture);
    }
    if (data->pixels) {
        SDL_free(data->pixels);
    }
    SDL_free(data);
    texture->driverdata = NULL;
}

static void
GLES_DestroyRenderer(SDL_Renderer * renderer)
{
    GLES_RenderData *data = (GLES_RenderData *) renderer->driverdata;

    if (data) {
        if (data->context) {
            SDL_GL_DeleteContext(data->context);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_RENDER_OGL_ES */

/* vi: set ts=4 sw=4 expandtab: */
