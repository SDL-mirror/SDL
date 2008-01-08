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

#if SDL_VIDEO_RENDER_OGL

#include "SDL_video.h"
#include "SDL_opengl.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_rect_c.h"
#include "SDL_yuv_sw_c.h"

#ifdef __MACOSX__
#include <OpenGL/OpenGL.h>
#endif

/* OpenGL renderer implementation */

/* Details on optimizing the texture path on Mac OS X:
   http://developer.apple.com/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_texturedata/chapter_10_section_2.html
*/

static const float inv255f = 1.0f / 255.0f;

static SDL_Renderer *GL_CreateRenderer(SDL_Window * window, Uint32 flags);
static int GL_ActivateRenderer(SDL_Renderer * renderer);
static int GL_DisplayModeChanged(SDL_Renderer * renderer);
static int GL_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int GL_QueryTexturePixels(SDL_Renderer * renderer,
                                 SDL_Texture * texture, void **pixels,
                                 int *pitch);
static int GL_SetTexturePalette(SDL_Renderer * renderer,
                                SDL_Texture * texture,
                                const SDL_Color * colors, int firstcolor,
                                int ncolors);
static int GL_GetTexturePalette(SDL_Renderer * renderer,
                                SDL_Texture * texture, SDL_Color * colors,
                                int firstcolor, int ncolors);
static int GL_SetTextureColorMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int GL_SetTextureAlphaMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int GL_SetTextureBlendMode(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int GL_SetTextureScaleMode(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int GL_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * rect, const void *pixels,
                            int pitch);
static int GL_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * rect, int markDirty, void **pixels,
                          int *pitch);
static void GL_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void GL_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            int numrects, const SDL_Rect * rects);
static int GL_RenderFill(SDL_Renderer * renderer, Uint8 r, Uint8 g, Uint8 b,
                         Uint8 a, const SDL_Rect * rect);
static int GL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                         const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static void GL_RenderPresent(SDL_Renderer * renderer);
static void GL_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void GL_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver GL_RenderDriver = {
    GL_CreateRenderer,
    {
     "opengl",
     (SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTDISCARD |
      SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED),
     (SDL_TEXTUREMODULATE_NONE | SDL_TEXTUREMODULATE_COLOR |
      SDL_TEXTUREMODULATE_ALPHA),
     (SDL_TEXTUREBLENDMODE_NONE | SDL_TEXTUREBLENDMODE_MASK |
      SDL_TEXTUREBLENDMODE_BLEND | SDL_TEXTUREBLENDMODE_ADD |
      SDL_TEXTUREBLENDMODE_MOD),
     (SDL_TEXTURESCALEMODE_NONE | SDL_TEXTURESCALEMODE_FAST |
      SDL_TEXTURESCALEMODE_SLOW),
     16,
     {
      SDL_PIXELFORMAT_INDEX1LSB,
      SDL_PIXELFORMAT_INDEX1MSB,
      SDL_PIXELFORMAT_INDEX8,
      SDL_PIXELFORMAT_RGB332,
      SDL_PIXELFORMAT_RGB444,
      SDL_PIXELFORMAT_RGB555,
      SDL_PIXELFORMAT_ARGB4444,
      SDL_PIXELFORMAT_ARGB1555,
      SDL_PIXELFORMAT_RGB565,
      SDL_PIXELFORMAT_RGB24,
      SDL_PIXELFORMAT_BGR24,
      SDL_PIXELFORMAT_RGB888,
      SDL_PIXELFORMAT_BGR888,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_PIXELFORMAT_ABGR8888,
      SDL_PIXELFORMAT_ARGB2101010},
     0,
     0}
};

typedef struct
{
    SDL_GLContext context;
    SDL_bool updateSize;
    SDL_bool GL_ARB_texture_rectangle_supported;
    SDL_bool GL_EXT_paletted_texture_supported;
    int blendMode;
    int scaleMode;

    /* OpenGL functions */
#define SDL_PROC(ret,func,params) ret (APIENTRY *func) params;
#include "SDL_glfuncs.h"
#undef SDL_PROC

    PFNGLCOLORTABLEEXTPROC glColorTableEXT;
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
    Uint8 *palette;
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

static int
GL_LoadFunctions(GL_RenderData * data)
{
#if defined(__QNXNTO__) && (_NTO_VERSION < 630)
#define __SDL_NOGETPROCADDR__
#elif defined(__MINT__)
#define __SDL_NOGETPROCADDR__
#endif
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

#include "SDL_glfuncs.h"
#undef SDL_PROC
    return 0;
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
    GLint value;
    int doublebuffer;

    /* Render directly to the window, unless we're compositing */
#ifndef __MACOSX__
    if (flags & SDL_RENDERER_SINGLEBUFFER) {
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
    }
#endif
    if (!(window->flags & SDL_WINDOW_OPENGL)) {
        if (SDL_RecreateWindow(window, window->flags | SDL_WINDOW_OPENGL) < 0) {
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
    renderer->DisplayModeChanged = GL_DisplayModeChanged;
    renderer->CreateTexture = GL_CreateTexture;
    renderer->QueryTexturePixels = GL_QueryTexturePixels;
    renderer->SetTexturePalette = GL_SetTexturePalette;
    renderer->GetTexturePalette = GL_GetTexturePalette;
    renderer->SetTextureColorMod = GL_SetTextureColorMod;
    renderer->SetTextureAlphaMod = GL_SetTextureAlphaMod;
    renderer->SetTextureBlendMode = GL_SetTextureBlendMode;
    renderer->SetTextureScaleMode = GL_SetTextureScaleMode;
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
        (SDL_RENDERER_PRESENTDISCARD | SDL_RENDERER_ACCELERATED);

    if (GL_LoadFunctions(data) < 0) {
        GL_DestroyRenderer(renderer);
        return NULL;
    }

    data->context = SDL_GL_CreateContext(window->id);
    if (!data->context) {
        GL_DestroyRenderer(renderer);
        return NULL;
    }
    if (SDL_GL_MakeCurrent(window->id, data->context) < 0) {
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

    if (SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &doublebuffer) == 0) {
        if (!doublebuffer) {
            renderer->info.flags |= SDL_RENDERER_SINGLEBUFFER;
        }
    }

    data->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
    renderer->info.max_texture_width = value;
    data->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
    renderer->info.max_texture_height = value;

    if (SDL_GL_ExtensionSupported("GL_ARB_texture_rectangle")
        || SDL_GL_ExtensionSupported("GL_EXT_texture_rectangle")) {
        data->GL_ARB_texture_rectangle_supported = SDL_TRUE;
    }
    if (SDL_GL_ExtensionSupported("GL_EXT_paletted_texture")) {
        data->GL_EXT_paletted_texture_supported = SDL_TRUE;
        data->glColorTableEXT =
            (PFNGLCOLORTABLEEXTPROC) SDL_GL_GetProcAddress("glColorTableEXT");
    } else {
        /* Don't advertise support for 8-bit indexed texture format */
        Uint32 i, j;
        SDL_RendererInfo *info = &renderer->info;
        for (i = 0, j = 0; i < info->num_texture_formats; ++i) {
            if (info->texture_formats[i] != SDL_PIXELFORMAT_INDEX8) {
                info->texture_formats[j++] = info->texture_formats[i];
            }
        }
        --info->num_texture_formats;
    }
    if (SDL_GL_ExtensionSupported("GL_APPLE_texture_range")) {
        data->glTextureRangeAPPLE =
            (void (*)(GLenum, GLsizei, const GLvoid *))
            SDL_GL_GetProcAddress("glTextureRangeAPPLE");
    }

    /* Set up parameters for rendering */
    data->blendMode = -1;
    data->scaleMode = -1;
    data->glDisable(GL_DEPTH_TEST);
    data->glDisable(GL_CULL_FACE);
    if (data->GL_ARB_texture_rectangle_supported) {
        data->glEnable(GL_TEXTURE_RECTANGLE_ARB);
    } else {
        data->glEnable(GL_TEXTURE_2D);
    }
    data->updateSize = SDL_TRUE;

    return renderer;
}

static int
GL_ActivateRenderer(SDL_Renderer * renderer)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);

    if (SDL_GL_MakeCurrent(window->id, data->context) < 0) {
        return -1;
    }
    if (data->updateSize) {
        data->glMatrixMode(GL_PROJECTION);
        data->glLoadIdentity();
        data->glMatrixMode(GL_MODELVIEW);
        data->glLoadIdentity();
        data->glViewport(0, 0, window->w, window->h);
        data->glOrtho(0.0, (GLdouble) window->w, (GLdouble) window->h, 0.0,
                      0.0, 1.0);
        data->updateSize = SDL_FALSE;
    }
    return 0;
}

static int
GL_DisplayModeChanged(SDL_Renderer * renderer)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;

    data->updateSize = SDL_TRUE;
    return 0;
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
    case SDL_PIXELFORMAT_INDEX1LSB:
    case SDL_PIXELFORMAT_INDEX1MSB:
        internalFormat = GL_RGB;
        format = GL_COLOR_INDEX;
        type = GL_BITMAP;
        break;
    case SDL_PIXELFORMAT_INDEX8:
        if (!renderdata->GL_EXT_paletted_texture_supported) {
            SDL_SetError("Unsupported texture format");
            return -1;
        }
        internalFormat = GL_COLOR_INDEX8_EXT;
        format = GL_COLOR_INDEX;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PIXELFORMAT_RGB332:
        internalFormat = GL_R3_G3_B2;
        format = GL_RGB;
        type = GL_UNSIGNED_BYTE_3_3_2;
        break;
    case SDL_PIXELFORMAT_RGB444:
        internalFormat = GL_RGB4;
        format = GL_RGB;
        type = GL_UNSIGNED_SHORT_4_4_4_4;
        break;
    case SDL_PIXELFORMAT_RGB555:
        internalFormat = GL_RGB5;
        format = GL_RGB;
        type = GL_UNSIGNED_SHORT_5_5_5_1;
        break;
    case SDL_PIXELFORMAT_ARGB4444:
        internalFormat = GL_RGBA4;
        format = GL_BGRA;
        type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        break;
    case SDL_PIXELFORMAT_ARGB1555:
        internalFormat = GL_RGB5_A1;
        format = GL_BGRA;
        type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        break;
    case SDL_PIXELFORMAT_RGB565:
        internalFormat = GL_RGB8;
        format = GL_RGB;
        type = GL_UNSIGNED_SHORT_5_6_5;
        break;
    case SDL_PIXELFORMAT_RGB24:
        internalFormat = GL_RGB8;
        format = GL_RGB;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PIXELFORMAT_RGB888:
        internalFormat = GL_RGB8;
        format = GL_BGRA;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PIXELFORMAT_BGR24:
        internalFormat = GL_RGB8;
        format = GL_BGR;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PIXELFORMAT_BGR888:
        internalFormat = GL_RGB8;
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PIXELFORMAT_ARGB8888:
#ifdef __MACOSX__
        internalFormat = GL_RGBA;
        format = GL_BGRA;
        type = GL_UNSIGNED_INT_8_8_8_8_REV;
#else
        internalFormat = GL_RGBA8;
        format = GL_BGRA;
        type = GL_UNSIGNED_BYTE;
#endif
        break;
    case SDL_PIXELFORMAT_ABGR8888:
        internalFormat = GL_RGBA8;
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    case SDL_PIXELFORMAT_ARGB2101010:
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

    if (texture->format == SDL_PIXELFORMAT_INDEX8) {
        data->palette = (Uint8 *) SDL_malloc(3 * 256 * sizeof(Uint8));
        if (!data->palette) {
            SDL_OutOfMemory();
            SDL_free(data);
            return -1;
        }
        SDL_memset(data->palette, 0xFF, 3 * 256 * sizeof(Uint8));
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
    renderdata->glBindTexture(data->type, data->texture);
    renderdata->glTexParameteri(data->type, GL_TEXTURE_MIN_FILTER,
                                GL_NEAREST);
    renderdata->glTexParameteri(data->type, GL_TEXTURE_MAG_FILTER,
                                GL_NEAREST);
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
        /*
           if (renderdata->glTextureRangeAPPLE) {
           renderdata->glTextureRangeAPPLE(data->type,
           texture->h * data->pitch,
           data->pixels);
           }
         */
        renderdata->glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        renderdata->glTexImage2D(data->type, 0, internalFormat, texture_w,
                                 texture_h, 0, format, type, data->pixels);
    } else
#endif
    {
        renderdata->glTexImage2D(data->type, 0, internalFormat, texture_w,
                                 texture_h, 0, format, type, NULL);
    }
    result = renderdata->glGetError();
    if (result != GL_NO_ERROR) {
        GL_SetError("glTexImage2D()", result);
        return -1;
    }
    return 0;
}

static int
GL_QueryTexturePixels(SDL_Renderer * renderer, SDL_Texture * texture,
                      void **pixels, int *pitch)
{
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;

    *pixels = data->pixels;
    *pitch = data->pitch;
    return 0;
}

static int
GL_SetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                     const SDL_Color * colors, int firstcolor, int ncolors)
{
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;
    Uint8 *palette;

    if (!data->palette) {
        SDL_SetError("Texture doesn't have a palette");
        return -1;
    }
    palette = data->palette + firstcolor * 3;
    while (ncolors--) {
        *palette++ = colors->r;
        *palette++ = colors->g;
        *palette++ = colors->b;
        ++colors;
    }
    renderdata->glBindTexture(data->type, data->texture);
    renderdata->glColorTableEXT(data->type, GL_RGB8, 256, GL_RGB,
                                GL_UNSIGNED_BYTE, data->palette);
    return 0;
}

static int
GL_GetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                     SDL_Color * colors, int firstcolor, int ncolors)
{
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;
    Uint8 *palette;

    if (!data->palette) {
        SDL_SetError("Texture doesn't have a palette");
        return -1;
    }
    palette = data->palette + firstcolor * 3;
    while (ncolors--) {
        colors->r = *palette++;
        colors->g = *palette++;
        colors->b = *palette++;
        colors->unused = SDL_ALPHA_OPAQUE;
        ++colors;
    }
    return 0;
}

static void
SetupTextureUpdate(GL_RenderData * renderdata, SDL_Texture * texture,
                   int pitch)
{
    if (texture->format == SDL_PIXELFORMAT_INDEX1LSB) {
        renderdata->glPixelStorei(GL_UNPACK_LSB_FIRST, 1);
    } else if (texture->format == SDL_PIXELFORMAT_INDEX1MSB) {
        renderdata->glPixelStorei(GL_UNPACK_LSB_FIRST, 0);
    }
    renderdata->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    renderdata->glPixelStorei(GL_UNPACK_ROW_LENGTH,
                              pitch / SDL_BYTESPERPIXEL(texture->format));
}

static int
GL_SetTextureColorMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    return 0;
}

static int
GL_SetTextureAlphaMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    return 0;
}

static int
GL_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    switch (texture->blendMode) {
    case SDL_TEXTUREBLENDMODE_NONE:
    case SDL_TEXTUREBLENDMODE_MASK:
    case SDL_TEXTUREBLENDMODE_BLEND:
    case SDL_TEXTUREBLENDMODE_ADD:
    case SDL_TEXTUREBLENDMODE_MOD:
        return 0;
    default:
        SDL_Unsupported();
        texture->blendMode = SDL_TEXTUREBLENDMODE_NONE;
        return -1;
    }
}

static int
GL_SetTextureScaleMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    switch (texture->scaleMode) {
    case SDL_TEXTURESCALEMODE_NONE:
    case SDL_TEXTURESCALEMODE_FAST:
    case SDL_TEXTURESCALEMODE_SLOW:
        return 0;
    case SDL_TEXTURESCALEMODE_BEST:
        SDL_Unsupported();
        texture->scaleMode = SDL_TEXTURESCALEMODE_SLOW;
        return -1;
    default:
        SDL_Unsupported();
        texture->scaleMode = SDL_TEXTURESCALEMODE_NONE;
        return -1;
    }
}

static int
GL_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 const SDL_Rect * rect, const void *pixels, int pitch)
{
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;
    GLenum result;

    renderdata->glGetError();
    SetupTextureUpdate(renderdata, texture, pitch);
    renderdata->glBindTexture(data->type, data->texture);
    renderdata->glTexSubImage2D(data->type, 0, rect->x, rect->y, rect->w,
                                rect->h, data->format, data->formattype,
                                pixels);
    result = renderdata->glGetError();
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
GL_RenderFill(SDL_Renderer * renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a,
              const SDL_Rect * rect)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);

    data->glClearColor((GLclampf) r * inv255f, (GLclampf) g * inv255f,
                       (GLclampf) b * inv255f, (GLclampf) a * inv255f);
    data->glViewport(rect->x, window->h - rect->y, rect->w, rect->h);
    data->glClear(GL_COLOR_BUFFER_BIT);
    data->glViewport(0, 0, window->w, window->h);
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

    if (texturedata->dirty.list) {
        SDL_DirtyRect *dirty;
        void *pixels;
        int bpp = SDL_BYTESPERPIXEL(texture->format);
        int pitch = texturedata->pitch;

        SetupTextureUpdate(data, texture, pitch);
        data->glBindTexture(texturedata->type, texturedata->texture);
        for (dirty = texturedata->dirty.list; dirty; dirty = dirty->next) {
            SDL_Rect *rect = &dirty->rect;
            pixels =
                (void *) ((Uint8 *) texturedata->pixels + rect->y * pitch +
                          rect->x * bpp);
            data->glTexSubImage2D(texturedata->type, 0, rect->x, rect->y,
                                  rect->w, rect->h, texturedata->format,
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

    data->glBindTexture(texturedata->type, texturedata->texture);

    if (texture->modMode) {
        data->glColor4f((GLfloat) texture->r * inv255f,
                        (GLfloat) texture->g * inv255f,
                        (GLfloat) texture->b * inv255f,
                        (GLfloat) texture->a * inv255f);
    } else {
        data->glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (texture->blendMode != data->blendMode) {
        switch (texture->blendMode) {
        case SDL_TEXTUREBLENDMODE_NONE:
            data->glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
            data->glDisable(GL_BLEND);
            break;
        case SDL_TEXTUREBLENDMODE_MASK:
        case SDL_TEXTUREBLENDMODE_BLEND:
            data->glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            data->glEnable(GL_BLEND);
            data->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case SDL_TEXTUREBLENDMODE_ADD:
            data->glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            data->glEnable(GL_BLEND);
            data->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case SDL_TEXTUREBLENDMODE_MOD:
            data->glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            data->glEnable(GL_BLEND);
            data->glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            break;
        }
        data->blendMode = texture->blendMode;
    }

    if (texture->scaleMode != data->scaleMode) {
        switch (texture->scaleMode) {
        case SDL_TEXTURESCALEMODE_NONE:
        case SDL_TEXTURESCALEMODE_FAST:
            data->glTexParameteri(texturedata->type, GL_TEXTURE_MIN_FILTER,
                                  GL_NEAREST);
            data->glTexParameteri(texturedata->type, GL_TEXTURE_MAG_FILTER,
                                  GL_NEAREST);
            break;
        case SDL_TEXTURESCALEMODE_SLOW:
        case SDL_TEXTURESCALEMODE_BEST:
            data->glTexParameteri(texturedata->type, GL_TEXTURE_MIN_FILTER,
                                  GL_LINEAR);
            data->glTexParameteri(texturedata->type, GL_TEXTURE_MAG_FILTER,
                                  GL_LINEAR);
            break;
        }
        data->scaleMode = texture->scaleMode;
    }

    data->glBegin(GL_TRIANGLE_STRIP);
    data->glTexCoord2f(minu, minv);
    data->glVertex2i(minx, miny);
    data->glTexCoord2f(maxu, minv);
    data->glVertex2i(maxx, miny);
    data->glTexCoord2f(minu, maxv);
    data->glVertex2i(minx, maxy);
    data->glTexCoord2f(maxu, maxv);
    data->glVertex2i(maxx, maxy);
    data->glEnd();

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
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;

    if (!data) {
        return;
    }
    if (data->texture) {
        renderdata->glDeleteTextures(1, &data->texture);
    }
    if (data->palette) {
        SDL_free(data->palette);
    }
    if (data->pixels) {
        SDL_free(data->pixels);
    }
    SDL_FreeDirtyRects(&data->dirty);
    SDL_free(data);
    texture->driverdata = NULL;
}

static void
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

#endif /* SDL_VIDEO_RENDER_OGL */

/* vi: set ts=4 sw=4 expandtab: */
