/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

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

#if SDL_VIDEO_OPENGL

#include "SDL_video.h"
#include "SDL_opengl.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_rect_c.h"
#include "SDL_yuv_sw_c.h"

/* OpenGL renderer implementation */

static SDL_Renderer *GL_CreateRenderer(SDL_Window * window, Uint32 flags);
static int GL_ActivateRenderer(SDL_Renderer * renderer);
static int GL_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int GL_SetTexturePalette(SDL_Renderer * renderer,
                                SDL_Texture * texture,
                                const SDL_Color * colors, int firstcolor,
                                int ncolors);
static int GL_GetTexturePalette(SDL_Renderer * renderer,
                                SDL_Texture * texture, SDL_Color * colors,
                                int firstcolor, int ncolors);
static int GL_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * rect, const void *pixels,
                            int pitch);
static int GL_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * rect, int markDirty,
                          void **pixels, int *pitch);
static void GL_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void GL_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            int numrects, const SDL_Rect * rects);
static int GL_RenderFill(SDL_Renderer * renderer, const SDL_Rect * rect,
                         Uint32 color);
static int GL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                         const SDL_Rect * srcrect, const SDL_Rect * dstrect,
                         int blendMode, int scaleMode);
static void GL_RenderPresent(SDL_Renderer * renderer);
static void GL_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void GL_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver GL_RenderDriver = {
    GL_CreateRenderer,
    {
     "opengl",
     (SDL_Renderer_PresentDiscard | SDL_Renderer_PresentVSync |
      SDL_Renderer_Accelerated),
     (SDL_TextureBlendMode_None | SDL_TextureBlendMode_Mask |
      SDL_TextureBlendMode_Blend | SDL_TextureBlendMode_Add |
      SDL_TextureBlendMode_Mod),
     (SDL_TextureScaleMode_None | SDL_TextureScaleMode_Fast |
      SDL_TextureScaleMode_Slow),
     16,
     {
      SDL_PixelFormat_Index1LSB,
      SDL_PixelFormat_Index1MSB,
      SDL_PixelFormat_Index8,
      SDL_PixelFormat_RGB332,
      SDL_PixelFormat_RGB444,
      SDL_PixelFormat_RGB555,
      SDL_PixelFormat_ARGB4444,
      SDL_PixelFormat_ARGB1555,
      SDL_PixelFormat_RGB565,
      SDL_PixelFormat_RGB24,
      SDL_PixelFormat_BGR24,
      SDL_PixelFormat_RGB888,
      SDL_PixelFormat_BGR888,
      SDL_PixelFormat_ARGB8888,
      SDL_PixelFormat_ABGR8888,
      SDL_PixelFormat_ARGB2101010},
     0,
     0}
};

typedef struct
{
    SDL_GLContext context;
    SDL_bool GL_ARB_texture_rectangle_supported;
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
    SDL_DirtyRectList dirty;
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

void
GL_AddRenderDriver(_THIS)
{
    if (_this->GL_CreateContext) {
        SDL_AddRenderDriver(0, &GL_RenderDriver);
    }
}

SDL_Renderer *
GL_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_Renderer *renderer;
    GL_RenderData *data;

    if (!(window->flags & SDL_WINDOW_OPENGL)) {
        window->flags |= SDL_WINDOW_OPENGL;
        if (SDL_RecreateWindow(window) < 0) {
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

    renderer->ActivateRenderer = GL_ActivateRenderer;
    renderer->CreateTexture = GL_CreateTexture;
    renderer->SetTexturePalette = GL_SetTexturePalette;
    renderer->GetTexturePalette = GL_GetTexturePalette;
    renderer->UpdateTexture = GL_UpdateTexture;
    renderer->LockTexture = GL_LockTexture;
    renderer->UnlockTexture = GL_UnlockTexture;
    renderer->DirtyTexture = GL_DirtyTexture;
    renderer->RenderFill = GL_RenderFill;
    renderer->RenderCopy = GL_RenderCopy;
    renderer->RenderPresent = GL_RenderPresent;
    renderer->DestroyTexture = GL_DestroyTexture;
    renderer->DestroyRenderer = GL_DestroyRenderer;
    renderer->info = GL_RenderDriver.info;
    renderer->window = window->id;
    renderer->driverdata = data;

    renderer->info.flags =
        (SDL_Renderer_PresentDiscard | SDL_Renderer_Accelerated);

    data->context = SDL_GL_CreateContext(window->id);
    if (!data->context) {
        GL_DestroyRenderer(renderer);
        return NULL;
    }
    if (SDL_GL_MakeCurrent(window->id, data->context) < 0) {
        GL_DestroyRenderer(renderer);
        return NULL;
    }

    if (flags & SDL_Renderer_PresentVSync) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }
    if (SDL_GL_GetSwapInterval() > 0) {
        renderer->info.flags |= SDL_Renderer_PresentVSync;
    }

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &renderer->info.max_texture_width);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &renderer->info.max_texture_height);

    if (SDL_GL_ExtensionSupported("GL_ARB_texture_rectangle")
        || SDL_GL_ExtensionSupported("GL_EXT_texture_rectangle")) {
        data->GL_ARB_texture_rectangle_supported = SDL_TRUE;
    }

    /* Set up parameters for rendering */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    if (data->GL_ARB_texture_rectangle_supported) {
        glEnable(GL_TEXTURE_RECTANGLE_ARB);
    } else {
        glEnable(GL_TEXTURE_2D);
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0, 0, window->w, window->h);
    glOrtho(0.0, (GLdouble) window->w, (GLdouble) window->h, 0.0, 0.0, 1.0);

    return renderer;
}

static int
GL_ActivateRenderer(SDL_Renderer * renderer)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);

    return SDL_GL_MakeCurrent(window->id, data->context);
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
GL_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    GL_TextureData *data;
    GLint internalFormat;
    GLenum format, type;
    int texture_w, texture_h;
    GLenum result;

    switch (texture->format) {
    case SDL_PixelFormat_Index1LSB:
    case SDL_PixelFormat_Index1MSB:
        internalFormat = GL_RGB;
        format = GL_COLOR_INDEX;
        type = GL_BITMAP;
        break;
    case SDL_PixelFormat_Index8:
        internalFormat = GL_RGB;
        format = GL_COLOR_INDEX;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PixelFormat_RGB332:
        internalFormat = GL_R3_G3_B2;
        format = GL_RGB;
        type = GL_UNSIGNED_BYTE_3_3_2;
        break;
    case SDL_PixelFormat_RGB444:
        internalFormat = GL_RGB4;
        format = GL_RGB;
        type = GL_UNSIGNED_SHORT_4_4_4_4;
        break;
    case SDL_PixelFormat_RGB555:
        internalFormat = GL_RGB5;
        format = GL_RGB;
        type = GL_UNSIGNED_SHORT_5_5_5_1;
        break;
    case SDL_PixelFormat_ARGB4444:
        internalFormat = GL_RGBA4;
        format = GL_BGRA;
        type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        break;
    case SDL_PixelFormat_ARGB1555:
        internalFormat = GL_RGB5_A1;
        format = GL_BGRA;
        type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        break;
    case SDL_PixelFormat_RGB565:
        internalFormat = GL_RGB8;
        format = GL_RGB;
        type = GL_UNSIGNED_SHORT_5_6_5;
        break;
    case SDL_PixelFormat_RGB24:
        internalFormat = GL_RGB8;
        format = GL_RGB;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PixelFormat_RGB888:
        internalFormat = GL_RGB8;
        format = GL_BGRA;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PixelFormat_BGR24:
        internalFormat = GL_RGB8;
        format = GL_BGR;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PixelFormat_BGR888:
        internalFormat = GL_RGB8;
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PixelFormat_ARGB8888:
        internalFormat = GL_RGBA8;
        format = GL_BGRA;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PixelFormat_ABGR8888:
        internalFormat = GL_RGBA8;
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PixelFormat_ARGB2101010:
        internalFormat = GL_RGB10_A2;
        format = GL_BGRA;
        type = GL_UNSIGNED_INT_2_10_10_10_REV;
        break;
    default:
        SDL_SetError("Unsupported texture format");
        return -1;
    }

    data = (GL_TextureData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }

    texture->driverdata = data;

    glGetError();
    glGenTextures(1, &data->texture);
    if (renderdata->GL_ARB_texture_rectangle_supported) {
        data->type = GL_TEXTURE_RECTANGLE_ARB;
        texture_w = texture->w;
        texture_h = texture->h;
        data->texw = (GLfloat) texture->w;
        data->texh = (GLfloat) texture->h;
    } else {
        data->type = GL_TEXTURE_2D;
        texture_w = power_of_2(texture->w);
        texture_h = power_of_2(texture->h);
        data->texw = (GLfloat) texture->w / texture_w;
        data->texh = (GLfloat) texture->h / texture_h;
    }
    data->format = format;
    data->formattype = type;
    glBindTexture(data->type, data->texture);
    glTexImage2D(data->type, 0, internalFormat, texture_w, texture_h, 0,
                 format, type, NULL);
    result = glGetError();
    if (result != GL_NO_ERROR) {
        GL_SetError("glTexImage2D()", result);
        return -1;
    }
    return 0;
}

static int
GL_SetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                     const SDL_Color * colors, int firstcolor, int ncolors)
{
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;

    return 0;
}

static int
GL_GetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                     SDL_Color * colors, int firstcolor, int ncolors)
{
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;

    return 0;
}

static void
SetupTextureUpdate(SDL_Texture * texture, int pitch)
{
    if (texture->format == SDL_PixelFormat_Index1LSB) {
        glPixelStorei(GL_UNPACK_LSB_FIRST, 1);
    } else if (texture->format == SDL_PixelFormat_Index1MSB) {
        glPixelStorei(GL_UNPACK_LSB_FIRST, 0);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,
                  pitch / SDL_BYTESPERPIXEL(texture->format));
}

static int
GL_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 const SDL_Rect * rect, const void *pixels, int pitch)
{
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;
    GLenum result;

    glGetError();
    SetupTextureUpdate(texture, pitch);
    glBindTexture(data->type, data->texture);
    glTexSubImage2D(data->type, 0, rect->x, rect->y, rect->w, rect->h,
                    data->format, data->formattype, pixels);
    result = glGetError();
    if (result != GL_NO_ERROR) {
        GL_SetError("glTexSubImage2D()", result);
        return -1;
    }
    return 0;
}

static int
GL_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * rect, int markDirty, void **pixels,
               int *pitch)
{
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;

    if (!data->pixels) {
        data->pitch = texture->w * SDL_BYTESPERPIXEL(texture->format);
        data->pixels = SDL_malloc(texture->h * data->pitch);
        if (!data->pixels) {
            SDL_OutOfMemory();
            return -1;
        }
    }

    if (markDirty) {
        SDL_AddDirtyRect(&data->dirty, rect);
    }

    *pixels =
        (void *) ((Uint8 *) data->pixels + rect->y * data->pitch +
                  rect->x * SDL_BYTESPERPIXEL(texture->format));
    *pitch = data->pitch;
    return 0;
}

static void
GL_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
}

static void
GL_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture, int numrects,
                const SDL_Rect * rects)
{
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;
    int i;

    for (i = 0; i < numrects; ++i) {
        SDL_AddDirtyRect(&data->dirty, &rects[i]);
    }
}

static int
GL_RenderFill(SDL_Renderer * renderer, const SDL_Rect * rect, Uint32 color)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    GLclampf r, g, b, a;

    a = ((GLclampf) ((color >> 24) & 0xFF)) / 255.0f;
    r = ((GLclampf) ((color >> 16) & 0xFF)) / 255.0f;
    g = ((GLclampf) ((color >> 8) & 0xFF)) / 255.0f;
    b = ((GLclampf) (color & 0xFF)) / 255.0f;

    glClearColor(r, g, b, a);
    glViewport(rect->x, window->h - rect->y, rect->w, rect->h);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, window->w, window->h);
    return 0;
}

static int
GL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
              const SDL_Rect * srcrect, const SDL_Rect * dstrect,
              int blendMode, int scaleMode)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *texturedata = (GL_TextureData *) texture->driverdata;
    int minx, miny, maxx, maxy;
    GLfloat minu, maxu, minv, maxv;

    if (texturedata->dirty.count > 0) {
        SDL_DirtyRect *dirty;
        void *pixels;
        int bpp = SDL_BYTESPERPIXEL(texture->format);
        int pitch = texturedata->pitch;

        SetupTextureUpdate(texture, pitch);
        glBindTexture(texturedata->type, texturedata->texture);
        for (dirty = texturedata->dirty.list; dirty; dirty = dirty->next) {
            SDL_Rect *rect = &dirty->rect;
            pixels =
                (void *) ((Uint8 *) texturedata->pixels + rect->y * pitch +
                          rect->x * bpp);
            glTexSubImage2D(texturedata->type, 0, rect->x, rect->y, rect->w,
                            rect->h, texturedata->format,
                            texturedata->formattype, pixels);
        }
        SDL_ClearDirtyRects(&texturedata->dirty);
    }

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

    glBindTexture(texturedata->type, texturedata->texture);

    switch (blendMode) {
    case SDL_TextureBlendMode_None:
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glDisable(GL_BLEND);
        break;
    case SDL_TextureBlendMode_Mask:
    case SDL_TextureBlendMode_Blend:
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case SDL_TextureBlendMode_Add:
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case SDL_TextureBlendMode_Mod:
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        break;
    }

    switch (scaleMode) {
    case SDL_TextureScaleMode_None:
    case SDL_TextureScaleMode_Fast:
        glTexParameteri(texturedata->type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(texturedata->type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;
    case SDL_TextureScaleMode_Slow:
    case SDL_TextureScaleMode_Best:
        glTexParameteri(texturedata->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(texturedata->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(minu, minv);
    glVertex2i(minx, miny);
    glTexCoord2f(maxu, minv);
    glVertex2i(maxx, miny);
    glTexCoord2f(minu, maxv);
    glVertex2i(minx, maxy);
    glTexCoord2f(maxu, maxv);
    glVertex2i(maxx, maxy);
    glEnd();

    return 0;
}

static void
GL_RenderPresent(SDL_Renderer * renderer)
{
    SDL_GL_SwapWindow(renderer->window);
}

static void
GL_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;

    if (!data) {
        return;
    }
    if (data->texture) {
        glDeleteTextures(1, &data->texture);
    }
    if (data->pixels) {
        SDL_free(data->pixels);
    }
    SDL_FreeDirtyRects(&data->dirty);
    SDL_free(data);
    texture->driverdata = NULL;
}

void
GL_DestroyRenderer(SDL_Renderer * renderer)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;

    if (data) {
        if (data->context) {
            SDL_GL_MakeCurrent(0, NULL);
            SDL_GL_DeleteContext(data->context);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_OPENGL */

/* vi: set ts=4 sw=4 expandtab: */
