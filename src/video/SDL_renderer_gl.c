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
#if 0
#include "SDL_win32video.h"

/* OpenGL renderer implementation */

static SDL_Renderer *GL_CreateRenderer(SDL_Window * window, Uint32 flags);
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
      SDL_TextureScaleMode_Best),
     12,
     {
      SDL_PixelFormat_Index8,
      SDL_PixelFormat_RGB332,
      SDL_PixelFormat_RGB444,
      SDL_PixelFormat_RGB555,
      SDL_PixelFormat_ARGB4444,
      SDL_PixelFormat_ARGB1555,
      SDL_PixelFormat_RGB565,
      SDL_PixelFormat_RGB888,
      SDL_PixelFormat_ARGB8888,
      SDL_PixelFormat_ARGB2101010,
      SDL_PixelFormat_UYVY,
      SDL_PixelFormat_YUY2},
     0,
     0}
};

typedef struct
{
    SDL_GLContext context;
    SDL_bool beginScene;
} GL_RenderData;

typedef struct
{
    GLuint texture;
    GLfloat texw;
    GLfloat texh;
    void *pixels;
    int pitch;
} GL_TextureData;

static GLFORMAT
PixelFormatToOpenGL(Uint32 format,)
{
    switch (format) {
    case SDL_PixelFormat_Index8:
        return GLFMT_P8;
    case SDL_PixelFormat_RGB332:
        return GLFMT_R3G3B2;
    case SDL_PixelFormat_RGB444:
        return GLFMT_X4R4G4B4;
    case SDL_PixelFormat_RGB555:
        return GLFMT_X1R5G5B5;
    case SDL_PixelFormat_ARGB4444:
        return GLFMT_A4R4G4B4;
    case SDL_PixelFormat_ARGB1555:
        return GLFMT_A1R5G5B5;
    case SDL_PixelFormat_RGB565:
        return GLFMT_R5G6B5;
    case SDL_PixelFormat_RGB888:
        return GLFMT_X8R8G8B8;
    case SDL_PixelFormat_ARGB8888:
        return GLFMT_A8R8G8B8;
    case SDL_PixelFormat_ARGB2101010:
        return GLFMT_A2R10G10B10;
    case SDL_PixelFormat_UYVY:
        return GLFMT_UYVY;
    case SDL_PixelFormat_YUY2:
        return GLFMT_YUY2;
    default:
        return GLFMT_UNKNOWN;
    }
}

void
GL_AddRenderDriver(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    if (data->d3d) {
        SDL_AddRenderDriver(0, &GL_RenderDriver);
    }
}

SDL_Renderer *
GL_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_VideoData *videodata = (SDL_VideoData *) display->device->driverdata;
    SDL_WindowData *windowdata = (SDL_WindowData *) window->driverdata;
    SDL_Renderer *renderer;
    GL_RenderData *data;
    HRESULT result;
    GLPRESENT_PARAMETERS pparams;
    IDirect3DSwapChain9 *chain;

    if (!(window->flags & SDL_WINDOW_OPENGL)) {
        SDL_SetError
            ("The OpenGL renderer can only be used on OpenGL windows");
        return NULL;
    }

    renderer = (SDL_Renderer *) SDL_malloc(sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(renderer);

    data = (GL_RenderData *) SDL_malloc(sizeof(*data));
    if (!data) {
        GL_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(data);

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
    data->beginScene = SDL_TRUE;

    if (flags & SDL_Renderer_PresentVSync) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }
    if (SDL_GL_GetSwapInterval() > 0) {
        renderer->info.flags |= SDL_Renderer_PresentVSync;
    }

    /* Set up parameters for rendering */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0, 0, window->w, window->h);
    glOrtho(0.0, (GLdouble) window->w, (GLdouble) window->h, 0.0, 0.0, 1.0);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    return renderer;
}

/* Quick utility function for texture creation */
static int
power_of_two(int input)
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
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    GL_TextureData *data;
    GLPOOL pool;
    HRESULT result;

    data = (GL_TextureData *) SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    SDL_zerop(data);

    texture->driverdata = data;

    if (texture->access == SDL_TextureAccess_Local) {
        pool = GLPOOL_MANAGED;
    } else {
        pool = GLPOOL_DEFAULT;
    }
    result =
        IDirect3DDevice9_CreateTexture(renderdata->device, texture->w,
                                       texture->h, 1, 0,
                                       PixelFormatToGLFMT(texture->format),
                                       pool, &data->texture, NULL);
    if (FAILED(result)) {
        SDL_free(data);
        GL_SetError("CreateTexture()", result);
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

static int
GL_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 const SDL_Rect * rect, const void *pixels, int pitch)
{
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;
    GL_RenderData *renderdata = (GL_RenderData *) renderer->driverdata;
    IDirect3DTexture9 *temp;
    RECT d3drect;
    GLLOCKED_RECT locked;
    const Uint8 *src;
    Uint8 *dst;
    int row, length;
    HRESULT result;

    result =
        IDirect3DDevice9_CreateTexture(renderdata->device, texture->w,
                                       texture->h, 1, 0,
                                       PixelFormatToGLFMT(texture->format),
                                       GLPOOL_SYSTEMMEM, &temp, NULL);
    if (FAILED(result)) {
        GL_SetError("CreateTexture()", result);
        return -1;
    }

    d3drect.left = rect->x;
    d3drect.right = rect->x + rect->w;
    d3drect.top = rect->y;
    d3drect.bottom = rect->y + rect->h;

    result = IDirect3DTexture9_LockRect(temp, 0, &locked, &d3drect, 0);
    if (FAILED(result)) {
        IDirect3DTexture9_Release(temp);
        GL_SetError("LockRect()", result);
        return -1;
    }

    src = pixels;
    dst = locked.pBits;
    length = rect->w * SDL_BYTESPERPIXEL(texture->format);
    for (row = 0; row < rect->h; ++row) {
        SDL_memcpy(dst, src, length);
        src += pitch;
        dst += locked.Pitch;
    }
    IDirect3DTexture9_UnlockRect(temp, 0);

    result =
        IDirect3DDevice9_UpdateTexture(renderdata->device,
                                       (IDirect3DBaseTexture9 *) temp,
                                       (IDirect3DBaseTexture9 *) data->
                                       texture);
    IDirect3DTexture9_Release(temp);
    if (FAILED(result)) {
        GL_SetError("UpdateTexture()", result);
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
    RECT d3drect;
    GLLOCKED_RECT locked;
    HRESULT result;

    if (texture->access != SDL_TextureAccess_Local) {
        SDL_SetError("Can't lock remote video memory");
        return -1;
    }

    d3drect.left = rect->x;
    d3drect.right = rect->x + rect->w;
    d3drect.top = rect->y;
    d3drect.bottom = rect->y + rect->h;

    result =
        IDirect3DTexture9_LockRect(data->texture, 0, &locked, &d3drect,
                                   markDirty ? 0 : GLLOCK_NO_DIRTY_UPDATE);
    if (FAILED(result)) {
        GL_SetError("LockRect()", result);
        return -1;
    }
    *pixels = locked.pBits;
    *pitch = locked.Pitch;
    return 0;
}

static void
GL_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;

    IDirect3DTexture9_UnlockRect(data->texture, 0);
}

static void
GL_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture, int numrects,
                const SDL_Rect * rects)
{
    GL_TextureData *data = (GL_TextureData *) texture->driverdata;
    RECT d3drect;
    int i;

    for (i = 0; i < numrects; ++i) {
        const SDL_Rect *rect = &rects[i];

        d3drect.left = rect->x;
        d3drect.right = rect->x + rect->w;
        d3drect.top = rect->y;
        d3drect.bottom = rect->y + rect->h;

        IDirect3DTexture9_AddDirtyRect(data->texture, &d3drect);
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

    minx = dstrect->x;
    miny = dstrect->y;
    maxx = dstrect->x + dstrect->w;
    maxy = dstrect->y + dstrect->h;

    minu = (GLfloat) srcrect->x / texture->w;
    maxu = (GLfloat) (srcrect->x + srcrect->w) / texture->w;
    minv = (GLfloat) srcrect->y / texture->h;
    maxv = (GLfloat) (srcrect->y + srcrect->h) / texture->h;

    glBindTexture(GL_TEXTURE_2D, texturedata->texture);

    switch (blendMode) {
    case SDL_TextureBlendMode_None:
        glDisable(GL_BLEND);
        break;
    case SDL_TextureBlendMode_Mask:
    case SDL_TextureBlendMode_Blend:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case SDL_TextureBlendMode_Add:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case SDL_TextureBlendMode_Mod:
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        break;
    }

    switch (scaleMode) {
    case SDL_TextureScaleMode_None:
    case SDL_TextureScaleMode_Fast:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;
    case SDL_TextureScaleMode_Slow:
    case SDL_TextureScaleMode_Best:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(minu, minv);
    glVertex2i(minx, miny);
    glTexCoord2f(maxu, minv);
    glVertex2i(maxx, miny);
    glTexCoord2f(minu, maxv);
    glVertex2i(miny, maxy);
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
        IDirect3DTexture9_Release(data->texture);
    }
    SDL_free(data);
    texture->driverdata = NULL;
}

void
GL_DestroyRenderer(SDL_Renderer * renderer)
{
    GL_RenderData *data = (GL_RenderData *) renderer->driverdata;

    if (data) {
        if (data->device) {
            IDirect3DDevice9_Release(data->device);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* 0 */
#endif /* SDL_VIDEO_OPENGL */

/* vi: set ts=4 sw=4 expandtab: */
