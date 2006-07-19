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

#if SDL_VIDEO_RENDER_D3D

#include "SDL_win32video.h"

/* Direct3D renderer implementation */

static SDL_Renderer *D3D_CreateRenderer(SDL_Window * window, Uint32 flags);
static int D3D_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int D3D_SetTexturePalette(SDL_Renderer * renderer,
                                 SDL_Texture * texture,
                                 const SDL_Color * colors, int firstcolor,
                                 int ncolors);
static int D3D_GetTexturePalette(SDL_Renderer * renderer,
                                 SDL_Texture * texture, SDL_Color * colors,
                                 int firstcolor, int ncolors);
static int D3D_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                             const SDL_Rect * rect, const void *pixels,
                             int pitch);
static int D3D_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                           const SDL_Rect * rect, int markDirty,
                           void **pixels, int *pitch);
static void D3D_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void D3D_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                             int numrects, const SDL_Rect * rects);
static int D3D_RenderFill(SDL_Renderer * renderer, const SDL_Rect * rect,
                          Uint32 color);
static int D3D_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * srcrect, const SDL_Rect * dstrect,
                          int blendMode, int scaleMode);
static void D3D_RenderPresent(SDL_Renderer * renderer);
static void D3D_DestroyTexture(SDL_Renderer * renderer,
                               SDL_Texture * texture);
static void D3D_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver D3D_RenderDriver = {
    D3D_CreateRenderer,
    {
     "d3d",
     (SDL_Renderer_SingleBuffer | SDL_Renderer_PresentCopy |
      SDL_Renderer_PresentFlip2 | SDL_Renderer_PresentFlip3 |
      SDL_Renderer_PresentDiscard | SDL_Renderer_PresentVSync),
     (SDL_TextureBlendMode_None | SDL_TextureBlendMode_Mask | SDL_TextureBlendMode_Blend),      /* FIXME */
     (SDL_TextureScaleMode_None | SDL_TextureScaleMode_Fast),   /* FIXME */
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
    IDirect3DDevice9 *device;
    SDL_bool beginScene;
} D3D_RenderData;

typedef struct
{
    IDirect3DTexture9 *texture;
} D3D_TextureData;

typedef struct
{
    float x, y, z;
    float rhw;
    float u, v;
} Vertex;

static void
D3D_SetError(const char *prefix, HRESULT result)
{
    const char *error;

    switch (result) {
    case D3DERR_WRONGTEXTUREFORMAT:
        error = "WRONGTEXTUREFORMAT";
        break;
    case D3DERR_UNSUPPORTEDCOLOROPERATION:
        error = "UNSUPPORTEDCOLOROPERATION";
        break;
    case D3DERR_UNSUPPORTEDCOLORARG:
        error = "UNSUPPORTEDCOLORARG";
        break;
    case D3DERR_UNSUPPORTEDALPHAOPERATION:
        error = "UNSUPPORTEDALPHAOPERATION";
        break;
    case D3DERR_UNSUPPORTEDALPHAARG:
        error = "UNSUPPORTEDALPHAARG";
        break;
    case D3DERR_TOOMANYOPERATIONS:
        error = "TOOMANYOPERATIONS";
        break;
    case D3DERR_CONFLICTINGTEXTUREFILTER:
        error = "CONFLICTINGTEXTUREFILTER";
        break;
    case D3DERR_UNSUPPORTEDFACTORVALUE:
        error = "UNSUPPORTEDFACTORVALUE";
        break;
    case D3DERR_CONFLICTINGRENDERSTATE:
        error = "CONFLICTINGRENDERSTATE";
        break;
    case D3DERR_UNSUPPORTEDTEXTUREFILTER:
        error = "UNSUPPORTEDTEXTUREFILTER";
        break;
    case D3DERR_CONFLICTINGTEXTUREPALETTE:
        error = "CONFLICTINGTEXTUREPALETTE";
        break;
    case D3DERR_DRIVERINTERNALERROR:
        error = "DRIVERINTERNALERROR";
        break;
    case D3DERR_NOTFOUND:
        error = "NOTFOUND";
        break;
    case D3DERR_MOREDATA:
        error = "MOREDATA";
        break;
    case D3DERR_DEVICELOST:
        error = "DEVICELOST";
        break;
    case D3DERR_DEVICENOTRESET:
        error = "DEVICENOTRESET";
        break;
    case D3DERR_NOTAVAILABLE:
        error = "NOTAVAILABLE";
        break;
    case D3DERR_OUTOFVIDEOMEMORY:
        error = "OUTOFVIDEOMEMORY";
        break;
    case D3DERR_INVALIDDEVICE:
        error = "INVALIDDEVICE";
        break;
    case D3DERR_INVALIDCALL:
        error = "INVALIDCALL";
        break;
    case D3DERR_DRIVERINVALIDCALL:
        error = "DRIVERINVALIDCALL";
        break;
    case D3DERR_WASSTILLDRAWING:
        error = "WASSTILLDRAWING";
        break;
    default:
        error = "UNKNOWN";
        break;
    }
    SDL_SetError("%s: %s", prefix, error);
}

static D3DFORMAT
PixelFormatToD3DFMT(Uint32 format)
{
    switch (format) {
    case SDL_PixelFormat_Index8:
        return D3DFMT_P8;
    case SDL_PixelFormat_RGB332:
        return D3DFMT_R3G3B2;
    case SDL_PixelFormat_RGB444:
        return D3DFMT_X4R4G4B4;
    case SDL_PixelFormat_RGB555:
        return D3DFMT_X1R5G5B5;
    case SDL_PixelFormat_ARGB4444:
        return D3DFMT_A4R4G4B4;
    case SDL_PixelFormat_ARGB1555:
        return D3DFMT_A1R5G5B5;
    case SDL_PixelFormat_RGB565:
        return D3DFMT_R5G6B5;
    case SDL_PixelFormat_RGB888:
        return D3DFMT_X8R8G8B8;
    case SDL_PixelFormat_ARGB8888:
        return D3DFMT_A8R8G8B8;
    case SDL_PixelFormat_ARGB2101010:
        return D3DFMT_A2R10G10B10;
    case SDL_PixelFormat_UYVY:
        return D3DFMT_UYVY;
    case SDL_PixelFormat_YUY2:
        return D3DFMT_YUY2;
    default:
        return D3DFMT_UNKNOWN;
    }
}

void
D3D_AddRenderDriver(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    if (data->d3d) {
        SDL_AddRenderDriver(0, &D3D_RenderDriver);
    }
}

SDL_Renderer *
D3D_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_VideoData *videodata = (SDL_VideoData *) display->device->driverdata;
    SDL_WindowData *windowdata = (SDL_WindowData *) window->driverdata;
    SDL_Renderer *renderer;
    D3D_RenderData *data;
    HRESULT result;
    D3DPRESENT_PARAMETERS pparams;
    IDirect3DSwapChain9 *chain;

    renderer = (SDL_Renderer *) SDL_malloc(sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(renderer);

    data = (D3D_RenderData *) SDL_malloc(sizeof(*data));
    if (!data) {
        D3D_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(data);

    renderer->CreateTexture = D3D_CreateTexture;
    renderer->SetTexturePalette = D3D_SetTexturePalette;
    renderer->GetTexturePalette = D3D_GetTexturePalette;
    renderer->UpdateTexture = D3D_UpdateTexture;
    renderer->LockTexture = D3D_LockTexture;
    renderer->UnlockTexture = D3D_UnlockTexture;
    renderer->DirtyTexture = D3D_DirtyTexture;
    renderer->RenderFill = D3D_RenderFill;
    renderer->RenderCopy = D3D_RenderCopy;
    renderer->RenderPresent = D3D_RenderPresent;
    renderer->DestroyTexture = D3D_DestroyTexture;
    renderer->DestroyRenderer = D3D_DestroyRenderer;
    renderer->info = D3D_RenderDriver.info;
    renderer->window = window->id;
    renderer->driverdata = data;

    renderer->info.flags = SDL_Renderer_Accelerated;

    SDL_zero(pparams);
    pparams.BackBufferWidth = window->w;
    pparams.BackBufferHeight = window->h;
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        pparams.BackBufferFormat =
            PixelFormatToD3DFMT(display->fullscreen_mode->format);
    } else {
        pparams.BackBufferFormat = D3DFMT_UNKNOWN;
    }
    if (flags & SDL_Renderer_PresentFlip2) {
        pparams.BackBufferCount = 2;
        pparams.SwapEffect = D3DSWAPEFFECT_FLIP;
    } else if (flags & SDL_Renderer_PresentFlip3) {
        pparams.BackBufferCount = 3;
        pparams.SwapEffect = D3DSWAPEFFECT_FLIP;
    } else if (flags & SDL_Renderer_PresentCopy) {
        pparams.BackBufferCount = 1;
        pparams.SwapEffect = D3DSWAPEFFECT_COPY;
    } else {
        pparams.BackBufferCount = 1;
        pparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    }
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        pparams.Windowed = FALSE;
        pparams.FullScreen_RefreshRateInHz =
            display->fullscreen_mode->refresh_rate;
    } else {
        pparams.Windowed = TRUE;
        pparams.FullScreen_RefreshRateInHz = 0;
    }
    if (flags & SDL_Renderer_PresentVSync) {
        pparams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    } else {
        pparams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    result = IDirect3D9_CreateDevice(videodata->d3d, D3DADAPTER_DEFAULT,        /* FIXME */
                                     D3DDEVTYPE_HAL,
                                     windowdata->hwnd,
                                     D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                     &pparams, &data->device);
    if (FAILED(result)) {
        D3D_DestroyRenderer(renderer);
        D3D_SetError("CreateDevice()", result);
        return NULL;
    }
    data->beginScene = SDL_TRUE;

    /* Get presentation parameters to fill info */
    result = IDirect3DDevice9_GetSwapChain(data->device, 0, &chain);
    if (FAILED(result)) {
        D3D_DestroyRenderer(renderer);
        D3D_SetError("GetSwapChain()", result);
        return NULL;
    }
    result = IDirect3DSwapChain9_GetPresentParameters(chain, &pparams);
    if (FAILED(result)) {
        IDirect3DSwapChain9_Release(chain);
        D3D_DestroyRenderer(renderer);
        D3D_SetError("GetPresentParameters()", result);
        return NULL;
    }
    IDirect3DSwapChain9_Release(chain);
    switch (pparams.SwapEffect) {
    case D3DSWAPEFFECT_COPY:
        renderer->info.flags |= SDL_Renderer_PresentCopy;
        break;
    case D3DSWAPEFFECT_FLIP:
        switch (pparams.BackBufferCount) {
        case 2:
            renderer->info.flags |= SDL_Renderer_PresentFlip2;
            break;
        case 3:
            renderer->info.flags |= SDL_Renderer_PresentFlip3;
            break;
        }
        break;
    case D3DSWAPEFFECT_DISCARD:
        renderer->info.flags |= SDL_Renderer_PresentDiscard;
        break;
    }
    if (pparams.PresentationInterval == D3DPRESENT_INTERVAL_ONE) {
        renderer->info.flags |= SDL_Renderer_PresentVSync;
    }

    /* Set up parameters for rendering */
    IDirect3DDevice9_SetVertexShader(data->device, NULL);
    IDirect3DDevice9_SetFVF(data->device, D3DFVF_XYZRHW | D3DFVF_TEX1);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_CULLMODE,
                                    D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(data->device, D3DRS_LIGHTING, FALSE);

    return renderer;
}

static int
D3D_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    D3D_RenderData *renderdata = (D3D_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    D3D_TextureData *data;
    D3DPOOL pool;
    HRESULT result;

    data = (D3D_TextureData *) SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    SDL_zerop(data);

    texture->driverdata = data;

    if (texture->access == SDL_TextureAccess_Local) {
        pool = D3DPOOL_MANAGED;
    } else {
        pool = D3DPOOL_DEFAULT;
    }
    result =
        IDirect3DDevice9_CreateTexture(renderdata->device, texture->w,
                                       texture->h, 1, 0,
                                       PixelFormatToD3DFMT(texture->format),
                                       pool, &data->texture, NULL);
    if (FAILED(result)) {
        SDL_free(data);
        D3D_SetError("CreateTexture()", result);
        return -1;
    }

    return 0;
}

static int
D3D_SetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                      const SDL_Color * colors, int firstcolor, int ncolors)
{
    D3D_RenderData *renderdata = (D3D_RenderData *) renderer->driverdata;
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    return 0;
}

static int
D3D_GetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                      SDL_Color * colors, int firstcolor, int ncolors)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    return 0;
}

static int
D3D_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                  const SDL_Rect * rect, const void *pixels, int pitch)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;
    D3D_RenderData *renderdata = (D3D_RenderData *) renderer->driverdata;
    IDirect3DTexture9 *temp;
    RECT d3drect;
    D3DLOCKED_RECT locked;
    const Uint8 *src;
    Uint8 *dst;
    int row, length;
    HRESULT result;

    result =
        IDirect3DDevice9_CreateTexture(renderdata->device, texture->w,
                                       texture->h, 1, 0,
                                       PixelFormatToD3DFMT(texture->format),
                                       D3DPOOL_SYSTEMMEM, &temp, NULL);
    if (FAILED(result)) {
        D3D_SetError("CreateTexture()", result);
        return -1;
    }

    d3drect.left = rect->x;
    d3drect.right = rect->x + rect->w;
    d3drect.top = rect->y;
    d3drect.bottom = rect->y + rect->h;

    result = IDirect3DTexture9_LockRect(temp, 0, &locked, &d3drect, 0);
    if (FAILED(result)) {
        IDirect3DTexture9_Release(temp);
        D3D_SetError("LockRect()", result);
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
        D3D_SetError("UpdateTexture()", result);
        return -1;
    }
    return 0;
}

static int
D3D_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                const SDL_Rect * rect, int markDirty, void **pixels,
                int *pitch)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;
    RECT d3drect;
    D3DLOCKED_RECT locked;
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
                                   markDirty ? 0 : D3DLOCK_NO_DIRTY_UPDATE);
    if (FAILED(result)) {
        D3D_SetError("LockRect()", result);
        return -1;
    }
    *pixels = locked.pBits;
    *pitch = locked.Pitch;
    return 0;
}

static void
D3D_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

    IDirect3DTexture9_UnlockRect(data->texture, 0);
}

static void
D3D_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture, int numrects,
                 const SDL_Rect * rects)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;
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
D3D_RenderFill(SDL_Renderer * renderer, const SDL_Rect * rect, Uint32 color)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    D3DRECT d3drect;
    HRESULT result;

    if (data->beginScene) {
        IDirect3DDevice9_BeginScene(data->device);
        data->beginScene = SDL_FALSE;
    }

    d3drect.x1 = rect->x;
    d3drect.x2 = rect->x + rect->w;
    d3drect.y1 = rect->y;
    d3drect.y2 = rect->y + rect->h;

    result =
        IDirect3DDevice9_Clear(data->device, 1, &d3drect, D3DCLEAR_TARGET,
                               (D3DCOLOR) color, 1.0f, 0);
    if (FAILED(result)) {
        D3D_SetError("Clear()", result);
        return -1;
    }
    return 0;
}

static int
D3D_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * srcrect, const SDL_Rect * dstrect,
               int blendMode, int scaleMode)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    D3D_TextureData *texturedata = (D3D_TextureData *) texture->driverdata;
    float minx, miny, maxx, maxy;
    float minu, maxu, minv, maxv;
    Vertex vertices[4];
    HRESULT result;

    if (data->beginScene) {
        IDirect3DDevice9_BeginScene(data->device);
        data->beginScene = SDL_FALSE;
    }

    minx = (float) dstrect->x - 0.5f;
    miny = (float) dstrect->y - 0.5f;
    maxx = (float) dstrect->x + dstrect->w - 0.5f;
    maxy = (float) dstrect->y + dstrect->h - 0.5f;

    minu = (float) srcrect->x / texture->w;
    maxu = (float) (srcrect->x + srcrect->w) / texture->w;
    minv = (float) srcrect->y / texture->h;
    maxv = (float) (srcrect->y + srcrect->h) / texture->h;

    vertices[0].x = minx;
    vertices[0].y = miny;
    vertices[0].z = 0.0f;
    vertices[0].rhw = 1.0f;
    vertices[0].u = minu;
    vertices[0].v = minv;

    vertices[1].x = maxx;
    vertices[1].y = miny;
    vertices[1].z = 0.0f;
    vertices[1].rhw = 1.0f;
    vertices[1].u = maxu;
    vertices[1].v = minv;

    vertices[2].x = maxx;
    vertices[2].y = maxy;
    vertices[2].z = 0.0f;
    vertices[2].rhw = 1.0f;
    vertices[2].u = maxu;
    vertices[2].v = maxv;

    vertices[3].x = minx;
    vertices[3].y = maxy;
    vertices[3].z = 0.0f;
    vertices[3].rhw = 1.0f;
    vertices[3].u = minu;
    vertices[3].v = maxv;

    switch (blendMode) {
    case SDL_TextureBlendMode_None:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        FALSE);
        break;
    case SDL_TextureBlendMode_Mask:
    case SDL_TextureBlendMode_Blend:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        TRUE);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_SRCBLEND,
                                        D3DBLEND_SRCALPHA);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_DESTBLEND,
                                        D3DBLEND_INVSRCALPHA);
        break;
    case SDL_TextureBlendMode_Add:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        TRUE);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_SRCBLEND,
                                        D3DBLEND_SRCALPHA);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_DESTBLEND,
                                        D3DBLEND_ONE);
        break;
    case SDL_TextureBlendMode_Mod:
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_ALPHABLENDENABLE,
                                        TRUE);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_SRCBLEND,
                                        D3DBLEND_ZERO);
        IDirect3DDevice9_SetRenderState(data->device, D3DRS_DESTBLEND,
                                        D3DBLEND_SRCCOLOR);
        break;
    }

    result =
        IDirect3DDevice9_SetTexture(data->device, 0,
                                    (IDirect3DBaseTexture9 *) texturedata->
                                    texture);
    if (FAILED(result)) {
        D3D_SetError("SetTexture()", result);
        return -1;
    }
    result =
        IDirect3DDevice9_DrawPrimitiveUP(data->device, D3DPT_TRIANGLEFAN, 2,
                                         vertices, sizeof(*vertices));
    if (FAILED(result)) {
        D3D_SetError("DrawPrimitiveUP()", result);
        return -1;
    }
    return 0;
}

static void
D3D_RenderPresent(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;
    HRESULT result;

    if (!data->beginScene) {
        IDirect3DDevice9_EndScene(data->device);
        data->beginScene = SDL_TRUE;
    }

    result = IDirect3DDevice9_Present(data->device, NULL, NULL, NULL, NULL);
    if (FAILED(result)) {
        D3D_SetError("Present()", result);
    }
}

static void
D3D_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    D3D_TextureData *data = (D3D_TextureData *) texture->driverdata;

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
D3D_DestroyRenderer(SDL_Renderer * renderer)
{
    D3D_RenderData *data = (D3D_RenderData *) renderer->driverdata;

    if (data) {
        if (data->device) {
            IDirect3DDevice9_Release(data->device);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_RENDER_D3D */

/* vi: set ts=4 sw=4 expandtab: */
