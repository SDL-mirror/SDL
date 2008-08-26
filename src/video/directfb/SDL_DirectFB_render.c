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
    
    SDL1.3 implementation by couriersud@arcor.de
    
*/
#include "SDL_config.h"

#include "SDL_DirectFB_video.h"
#include "SDL_DirectFB_render.h"
#include "../SDL_rect_c.h"
#include "../SDL_yuv_sw_c.h"

/* GDI renderer implementation */

static SDL_Renderer *DirectFB_CreateRenderer(SDL_Window * window,
                                             Uint32 flags);
static int DirectFB_DisplayModeChanged(SDL_Renderer * renderer);
static int DirectFB_CreateTexture(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int DirectFB_QueryTexturePixels(SDL_Renderer * renderer,
                                       SDL_Texture * texture, void **pixels,
                                       int *pitch);
static int DirectFB_SetTexturePalette(SDL_Renderer * renderer,
                                      SDL_Texture * texture,
                                      const SDL_Color * colors,
                                      int firstcolor, int ncolors);
static int DirectFB_GetTexturePalette(SDL_Renderer * renderer,
                                      SDL_Texture * texture,
                                      SDL_Color * colors, int firstcolor,
                                      int ncolors);
static int DirectFB_SetTextureAlphaMod(SDL_Renderer * renderer,
                                       SDL_Texture * texture);
static int DirectFB_SetTextureColorMod(SDL_Renderer * renderer,
                                       SDL_Texture * texture);
static int DirectFB_SetTextureBlendMode(SDL_Renderer * renderer,
                                        SDL_Texture * texture);
static int DirectFB_SetTextureScaleMode(SDL_Renderer * renderer,
                                        SDL_Texture * texture);
static int DirectFB_UpdateTexture(SDL_Renderer * renderer,
                                  SDL_Texture * texture,
                                  const SDL_Rect * rect, const void *pixels,
                                  int pitch);
static int DirectFB_LockTexture(SDL_Renderer * renderer,
                                SDL_Texture * texture, const SDL_Rect * rect,
                                int markDirty, void **pixels, int *pitch);
static void DirectFB_UnlockTexture(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static void DirectFB_DirtyTexture(SDL_Renderer * renderer,
                                  SDL_Texture * texture, int numrects,
                                  const SDL_Rect * rects);
static int DirectFB_RenderFill(SDL_Renderer * renderer, Uint8 r, Uint8 g,
                               Uint8 b, Uint8 a, const SDL_Rect * rect);
static int DirectFB_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                               const SDL_Rect * srcrect,
                               const SDL_Rect * dstrect);
static void DirectFB_RenderPresent(SDL_Renderer * renderer);
static void DirectFB_DestroyTexture(SDL_Renderer * renderer,
                                    SDL_Texture * texture);
static void DirectFB_DestroyRenderer(SDL_Renderer * renderer);

SDL_RenderDriver DirectFB_RenderDriver = {
    DirectFB_CreateRenderer,
    {
     "directfb",
     (SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTCOPY |
      SDL_RENDERER_PRESENTFLIP2 | SDL_RENDERER_PRESENTFLIP3 |
      SDL_RENDERER_PRESENTDISCARD | SDL_RENDERER_ACCELERATED),
     (SDL_TEXTUREMODULATE_NONE | SDL_TEXTUREMODULATE_COLOR |
      SDL_TEXTUREMODULATE_ALPHA),
     (SDL_TEXTUREBLENDMODE_NONE | SDL_TEXTUREBLENDMODE_MASK |
      SDL_TEXTUREBLENDMODE_BLEND | SDL_TEXTUREBLENDMODE_ADD |
      SDL_TEXTUREBLENDMODE_MOD),
     (SDL_TEXTURESCALEMODE_NONE | SDL_TEXTURESCALEMODE_FAST),
     14,
     {
      SDL_PIXELFORMAT_INDEX8,
      SDL_PIXELFORMAT_INDEX4LSB,
      SDL_PIXELFORMAT_RGB332,
      SDL_PIXELFORMAT_RGB555,
      SDL_PIXELFORMAT_RGB565,
      SDL_PIXELFORMAT_RGB888,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_PIXELFORMAT_ARGB4444,
      SDL_PIXELFORMAT_ARGB1555,
      SDL_PIXELFORMAT_RGB24,
      SDL_PIXELFORMAT_YV12,
      SDL_PIXELFORMAT_IYUV,
      SDL_PIXELFORMAT_YUY2,
      SDL_PIXELFORMAT_UYVY},
     0,
     0}
};

typedef struct
{
    IDirectFBSurface *surface;
    DFBSurfaceFlipFlags flipflags;
    int isyuvdirect;
} DirectFB_RenderData;

typedef struct
{
    IDirectFBDisplayLayer *vidlayer;
    IDirectFBSurface *surface;
    Uint32 format;
    void *pixels;
    int pitch;
    IDirectFBPalette *palette;
    DFB_DisplayData *display;
} DirectFB_TextureData;

void
DirectFB_AddRenderDriver(_THIS)
{
    int i;
    for (i = 0; i < _this->num_displays; i++)
        SDL_AddRenderDriver(i, &DirectFB_RenderDriver);
}

SDL_Renderer *
DirectFB_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_DFB_WINDOWDATA(window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_DFB_DEVICEDATA(display->device);
    SDL_Renderer *renderer = NULL;
    DirectFB_RenderData *data = NULL;
    DFBResult ret;
    DFBSurfaceDescription dsc;
    DFBSurfaceCapabilities scaps;
    char *p;
    int i, n;

    SDL_DFB_CALLOC(renderer, 1, sizeof(*renderer));
    SDL_DFB_CALLOC(data, 1, sizeof(*data));

    renderer->DisplayModeChanged = DirectFB_DisplayModeChanged;
    renderer->CreateTexture = DirectFB_CreateTexture;
    renderer->QueryTexturePixels = DirectFB_QueryTexturePixels;
    renderer->SetTexturePalette = DirectFB_SetTexturePalette;
    renderer->GetTexturePalette = DirectFB_GetTexturePalette;
    renderer->SetTextureAlphaMod = DirectFB_SetTextureAlphaMod;
    renderer->SetTextureColorMod = DirectFB_SetTextureColorMod;
    renderer->SetTextureBlendMode = DirectFB_SetTextureBlendMode;
    renderer->SetTextureScaleMode = DirectFB_SetTextureScaleMode;
    renderer->UpdateTexture = DirectFB_UpdateTexture;
    renderer->LockTexture = DirectFB_LockTexture;
    renderer->UnlockTexture = DirectFB_UnlockTexture;
    renderer->DirtyTexture = DirectFB_DirtyTexture;
    renderer->RenderFill = DirectFB_RenderFill;
    renderer->RenderCopy = DirectFB_RenderCopy;
    renderer->RenderPresent = DirectFB_RenderPresent;
    renderer->DestroyTexture = DirectFB_DestroyTexture;
    renderer->DestroyRenderer = DirectFB_DestroyRenderer;
    renderer->info = DirectFB_RenderDriver.info;
    renderer->window = window->id;      /* SDL window id */
    renderer->driverdata = data;

    renderer->info.flags =
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTDISCARD;

    data->surface = windata->surface;
    data->flipflags = 0;

    if (flags & SDL_RENDERER_PRESENTVSYNC) {
        data->flipflags = DSFLIP_ONSYNC;
        renderer->info.flags |= SDL_RENDERER_PRESENTVSYNC;
    }

    data->surface->GetCapabilities(data->surface, &scaps);
    if (scaps & DSCAPS_DOUBLE)
        renderer->info.flags |= SDL_RENDERER_PRESENTFLIP2;
    else if (scaps & DSCAPS_TRIPLE)
        renderer->info.flags |= SDL_RENDERER_PRESENTFLIP3;
    else
        renderer->info.flags |= SDL_RENDERER_SINGLEBUFFER;

    data->isyuvdirect = 1;      /* default is on! */
    p = getenv("SDL_DIRECTFB_YUV_DIRECT");
    if (p)
        data->isyuvdirect = atoi(p);

    return renderer;

  error:
    SDL_DFB_FREE(renderer);
    SDL_DFB_FREE(data);
    return NULL;
}

static DFBSurfacePixelFormat
SDLToDFBPixelFormat(Uint32 format)
{
    switch (format) {
    case SDL_PIXELFORMAT_INDEX4LSB:
        return DSPF_ALUT44;
    case SDL_PIXELFORMAT_INDEX8:
        return DSPF_LUT8;
    case SDL_PIXELFORMAT_RGB332:
        return DSPF_RGB332;
    case SDL_PIXELFORMAT_RGB555:
        return DSPF_ARGB1555;
    case SDL_PIXELFORMAT_ARGB4444:
        return DSPF_ARGB4444;
    case SDL_PIXELFORMAT_ARGB1555:
        return DSPF_ARGB1555;
    case SDL_PIXELFORMAT_RGB565:
        return DSPF_RGB16;
    case SDL_PIXELFORMAT_RGB24:
        return DSPF_RGB24;
    case SDL_PIXELFORMAT_RGB888:
        return DSPF_RGB32;
    case SDL_PIXELFORMAT_ARGB8888:
        return DSPF_ARGB;
    case SDL_PIXELFORMAT_YV12:
        return DSPF_YV12;       /* Planar mode: Y + V + U  (3 planes) */
    case SDL_PIXELFORMAT_IYUV:
        return DSPF_I420;       /* Planar mode: Y + U + V  (3 planes) */
    case SDL_PIXELFORMAT_YUY2:
        return DSPF_YUY2;       /* Packed mode: Y0+U0+Y1+V0 (1 plane) */
    case SDL_PIXELFORMAT_UYVY:
        return DSPF_UYVY;       /* Packed mode: U0+Y0+V0+Y1 (1 plane) */
    case SDL_PIXELFORMAT_YVYU:
        return DSPF_UNKNOWN;    /* Packed mode: Y0+V0+Y1+U0 (1 plane) */
    case SDL_PIXELFORMAT_INDEX1LSB:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_INDEX1MSB:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_INDEX4MSB:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_RGB444:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_BGR24:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_BGR888:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_RGBA8888:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_ABGR8888:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_BGRA8888:
        return DSPF_UNKNOWN;
    case SDL_PIXELFORMAT_ARGB2101010:
        return DSPF_UNKNOWN;
    default:
        return DSPF_UNKNOWN;
    }
}

static int
DirectFB_DisplayModeChanged(SDL_Renderer * renderer)
{
    SDL_DFB_RENDERERDATA(renderer);
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_DFB_WINDOWDATA(window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_DFB_DEVICEDATA(display->device);
    DFBResult ret;
    DFBSurfaceDescription dsc;
    int i, n;

    /*
     * Nothing to do here
     */
    return 0;
  error:
    return -1;
}

static int
DirectFB_AcquireVidLayer(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_DFB_RENDERERDATA(renderer);
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_DFB_DEVICEDATA(display->device);
    DFB_DisplayData *dispdata = (DFB_DisplayData *) display->driverdata;
    DirectFB_TextureData *data = texture->driverdata;
    DFBDisplayLayerConfig layconf;
    int ret;

    if (renddata->isyuvdirect && (dispdata->vidID >= 0)
        && (!dispdata->vidIDinuse)
        && SDL_ISPIXELFORMAT_FOURCC(data->format)) {
        layconf.flags = DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT;
        layconf.width = texture->w;
        layconf.height = texture->h;
        layconf.pixelformat = SDLToDFBPixelFormat(data->format);

        SDL_DFB_CHECKERR(devdata->dfb->
                         GetDisplayLayer(devdata->dfb, dispdata->vidID,
                                         &data->vidlayer));
        SDL_DFB_CHECKERR(data->vidlayer->
                         SetCooperativeLevel(data->vidlayer,
                                             DLSCL_EXCLUSIVE));
        SDL_DFB_CHECKERR(data->vidlayer->
                         SetConfiguration(data->vidlayer, &layconf));
        SDL_DFB_CHECKERR(data->vidlayer->
                         GetSurface(data->vidlayer, &data->surface));
        //SDL_DFB_CHECKERR(data->vidlayer->GetDescription(data->vidlayer, laydsc));
        dispdata->vidIDinuse = 1;
        data->display = dispdata;
        SDL_DFB_DEBUG("Created HW YUV surface\n");

        return 0;
    }
    return 1;
  error:
    if (data->vidlayer) {
        SDL_DFB_RELEASE(data->surface);
        SDL_DFB_CHECKERR(data->vidlayer->
                         SetCooperativeLevel(data->vidlayer,
                                             DLSCL_ADMINISTRATIVE));
        SDL_DFB_RELEASE(data->vidlayer);
    }
    return 1;
}

static int
DirectFB_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_DFB_RENDERERDATA(renderer);
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_DFB_WINDOWDATA(window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_DFB_DEVICEDATA(display->device);
    DirectFB_TextureData *data;
    DFBResult ret;
    DFBSurfaceDescription dsc;
    DFBDisplayLayerDescription laydsc;
    DFBDisplayLayerConfig layconf;

    SDL_DFB_CALLOC(data, 1, sizeof(*data));
    texture->driverdata = data;

    data->format = texture->format;
    data->pitch = (texture->w * SDL_BYTESPERPIXEL(data->format));
    data->vidlayer = NULL;

    if (DirectFB_AcquireVidLayer(renderer, texture) != 0) {
        /* fill surface description */
        dsc.flags =
            DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS;
        dsc.width = texture->w;
        dsc.height = texture->h;
        /* <1.2 Never use DSCAPS_VIDEOONLY here. It kills performance
         * No DSCAPS_SYSTEMONLY either - let dfb decide
         * 1.2: DSCAPS_SYSTEMONLY boosts performance by factor ~8
         */
        dsc.caps = DSCAPS_PREMULTIPLIED;

        if (texture->access == SDL_TEXTUREACCESS_STREAMING)
            dsc.caps |= DSCAPS_SYSTEMONLY;
        else
            dsc.caps |= DSCAPS_VIDEOONLY;

        /* find the right pixelformat */

        dsc.pixelformat = SDLToDFBPixelFormat(data->format);
        if (dsc.pixelformat == DSPF_UNKNOWN) {
            SDL_SetError("Unknown pixel format %d\n", data->format);
            goto error;
        }

        data->pixels = NULL;

        /* Create the surface */
        SDL_DFB_CHECKERR(devdata->dfb->
                         CreateSurface(devdata->dfb, &dsc, &data->surface));
        if (SDL_ISPIXELFORMAT_INDEXED(data->format)
            && !SDL_ISPIXELFORMAT_FOURCC(data->format)) {
            SDL_DFB_CHECKERR(data->surface->
                             GetPalette(data->surface, &data->palette));
        }

    }
    return 0;

  error:
    SDL_DFB_RELEASE(data->palette);
    SDL_DFB_RELEASE(data->surface);
    SDL_DFB_FREE(texture->driverdata);
    return -1;
}

static int
DirectFB_QueryTexturePixels(SDL_Renderer * renderer, SDL_Texture * texture,
                            void **pixels, int *pitch)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;

    /*
     * Always fail here so in compat mode SDL_HWSURFACE is set !
     */

    return -1;
}

static int
DirectFB_SetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                           const SDL_Color * colors, int firstcolor,
                           int ncolors)
{
    DirectFB_RenderData *renderdata =
        (DirectFB_RenderData *) renderer->driverdata;
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;
    DFBResult ret;

    if (SDL_ISPIXELFORMAT_INDEXED(data->format)
        && !SDL_ISPIXELFORMAT_FOURCC(data->format)) {
        DFBColor entries[256];
        int i;

        for (i = 0; i < ncolors; ++i) {
            entries[i].r = colors[i].r;
            entries[i].g = colors[i].g;
            entries[i].b = colors[i].b;
            entries[i].a = 0xFF;
        }
        SDL_DFB_CHECKERR(data->palette->
                         SetEntries(data->palette, entries, ncolors,
                                    firstcolor));
        return 0;
    } else {
        SDL_SetError("YUV textures don't have a palette");
        return -1;
    }
  error:
    return -1;
}

static int
DirectFB_GetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                           SDL_Color * colors, int firstcolor, int ncolors)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;
    DFBResult ret;

    if (SDL_ISPIXELFORMAT_INDEXED(data->format)
        && !SDL_ISPIXELFORMAT_FOURCC(data->format)) {
        DFBColor entries[256];
        int i;

        SDL_DFB_CHECKERR(data->palette->
                         GetEntries(data->palette, entries, ncolors,
                                    firstcolor));

        for (i = 0; i < ncolors; ++i) {
            colors[i].r = entries[i].r;
            colors[i].g = entries[i].g;
            colors[i].b = entries[i].b;
        }
        return 0;
    } else {
        SDL_SetError("YUV textures don't have a palette");
        return -1;
    }
  error:
    return -1;
}

static int
DirectFB_SetTextureAlphaMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    return 0;
}

static int
DirectFB_SetTextureColorMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    return 0;
}

static int
DirectFB_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
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
DirectFB_SetTextureScaleMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    switch (texture->scaleMode) {
    case SDL_TEXTURESCALEMODE_NONE:
    case SDL_TEXTURESCALEMODE_FAST:
        return 0;
    case SDL_TEXTURESCALEMODE_SLOW:
    case SDL_TEXTURESCALEMODE_BEST:
        SDL_Unsupported();
        texture->scaleMode = SDL_TEXTURESCALEMODE_FAST;
        return -1;
    default:
        SDL_Unsupported();
        texture->scaleMode = SDL_TEXTURESCALEMODE_NONE;
        return -1;
    }
    return 0;
}

static int
DirectFB_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                       const SDL_Rect * rect, const void *pixels, int pitch)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;
    DirectFB_RenderData *renderdata =
        (DirectFB_RenderData *) renderer->driverdata;
    DFBResult ret;
    Uint8 *dpixels;
    int dpitch;
    Uint8 *src, *dst;
    int row;
    size_t length;

    SDL_DFB_CHECKERR(data->surface->Lock(data->surface,
                                         DSLF_WRITE | DSLF_READ,
                                         ((void **) &dpixels), &dpitch));
    src = (Uint8 *) pixels;
    dst =
        (Uint8 *) dpixels + rect->y * dpitch +
        rect->x * SDL_BYTESPERPIXEL(texture->format);
    length = rect->w * SDL_BYTESPERPIXEL(texture->format);
    for (row = 0; row < rect->h; ++row) {
        SDL_memcpy(dst, src, length);
        src += pitch;
        dst += dpitch;
    }
    SDL_DFB_CHECKERR(data->surface->Unlock(data->surface));
    return 0;
  error:
    return 1;

}

static int
DirectFB_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                     const SDL_Rect * rect, int markDirty, void **pixels,
                     int *pitch)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;
    DFBResult ret;
    void *fdata;
    int fpitch;

    SDL_DFB_CHECKERR(data->surface->Lock(data->surface,
                                         DSLF_WRITE | DSLF_READ, &fdata,
                                         &fpitch));
    data->pixels = fdata;
    data->pitch = fpitch;

    switch (texture->format) {
    case SDL_PIXELFORMAT_YV12:
    case SDL_PIXELFORMAT_IYUV:
        if (rect
            && (rect->x != 0 || rect->y != 0 || rect->w != texture->w
                || rect->h != texture->h)) {
            SDL_SetError
                ("YV12 and IYUV textures only support full surface locks");
            return -1;
        }
        break;
    default:
        /* Only one plane, no worries */
        break;
    }

    *pitch = data->pitch;
    *pixels = data->pixels;

    return 0;
  error:
    return -1;
}

static void
DirectFB_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;

    data->surface->Unlock(data->surface);
    data->pixels = NULL;
}

static void
DirectFB_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                      int numrects, const SDL_Rect * rects)
{
    //TODO: DirtyTexture
}

static int
DirectFB_RenderFill(SDL_Renderer * renderer, Uint8 r, Uint8 g, Uint8 b,
                    Uint8 a, const SDL_Rect * rect)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    DFBResult ret;

    SDL_DFB_CHECKERR(data->surface->SetColor(data->surface, r, g, b, a));
    SDL_DFB_CHECKERR(data->surface->
                     FillRectangle(data->surface, rect->x, rect->y, rect->w,
                                   rect->h));

    return 0;
  error:
    return -1;
}

static int
DirectFB_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                    const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    DirectFB_TextureData *texturedata =
        (DirectFB_TextureData *) texture->driverdata;
    DFBResult ret;

    if (texturedata->vidlayer) {
        int px, py;
        SDL_Window *window = SDL_GetWindowFromID(renderer->window);
        SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
        SDL_DFB_DEVICEDATA(display->device);
        DFB_DisplayData *dispdata = (DFB_DisplayData *) display->driverdata;
        SDL_DFB_WINDOWDATA(window);

        SDL_DFB_CHECKERR(texturedata->vidlayer->
                         SetSourceRectangle(texturedata->vidlayer, srcrect->x,
                                            srcrect->y, srcrect->w,
                                            srcrect->h));
        windata->window->GetPosition(windata->window, &px, &py);
        SDL_DFB_CHECKERR(texturedata->vidlayer->
                         SetScreenRectangle(texturedata->vidlayer,
                                            px + dstrect->x, py + dstrect->y,
                                            dstrect->w, dstrect->h));
    } else {
        DFBRectangle sr, dr;
        DFBSurfaceBlittingFlags flags = 0;

        sr.x = srcrect->x;
        sr.y = srcrect->y;
        sr.w = srcrect->w;
        sr.h = srcrect->h;

        dr.x = dstrect->x;
        dr.y = dstrect->y;
        dr.w = dstrect->w;
        dr.h = dstrect->h;

        if (texture->
            modMode & (SDL_TEXTUREMODULATE_COLOR | SDL_TEXTUREMODULATE_ALPHA))
        {
            Uint8 alpha = 0xFF;
            if (texture->modMode & SDL_TEXTUREMODULATE_ALPHA) {
                alpha = texture->a;
                flags |= DSBLIT_SRC_PREMULTCOLOR;
                SDL_DFB_CHECKERR(data->surface->SetColor(data->surface, 0xFF,
                                                         0xFF, 0xFF, alpha));
            }
            if (texture->modMode & SDL_TEXTUREMODULATE_COLOR) {
                SDL_DFB_CHECKERR(data->surface->
                                 SetColor(data->surface, texture->r,
                                          texture->g, texture->b, alpha));
                /* Only works together .... */
                flags |= DSBLIT_COLORIZE | DSBLIT_SRC_PREMULTCOLOR;
            }
        }

        switch (texture->blendMode) {
        case SDL_TEXTUREBLENDMODE_NONE: /**< No blending */
            flags |= DSBLIT_NOFX;
            data->surface->SetSrcBlendFunction(data->surface, DSBF_ONE);
            data->surface->SetDstBlendFunction(data->surface, DSBF_ZERO);
            break;
        case SDL_TEXTUREBLENDMODE_MASK: /**< dst = A ? src : dst (alpha is mask) */
            flags |= DSBLIT_BLEND_ALPHACHANNEL;
            data->surface->SetSrcBlendFunction(data->surface, DSBF_SRCALPHA);
            data->surface->SetDstBlendFunction(data->surface,
                                               DSBF_INVSRCALPHA);
            break;
        case SDL_TEXTUREBLENDMODE_BLEND:/**< dst = (src * A) + (dst * (1-A)) */
            flags |= DSBLIT_BLEND_ALPHACHANNEL;
            data->surface->SetSrcBlendFunction(data->surface, DSBF_SRCALPHA);
            data->surface->SetDstBlendFunction(data->surface,
                                               DSBF_INVSRCALPHA);
            break;
        case SDL_TEXTUREBLENDMODE_ADD:  /**< dst = (src * A) + dst */
            flags |= DSBLIT_BLEND_ALPHACHANNEL;
            data->surface->SetSrcBlendFunction(data->surface, DSBF_SRCALPHA);
            data->surface->SetDstBlendFunction(data->surface, DSBF_ONE);
            break;
        case SDL_TEXTUREBLENDMODE_MOD:  /**< dst = src * dst */
            flags |= DSBLIT_BLEND_ALPHACHANNEL;
            data->surface->SetSrcBlendFunction(data->surface, DSBF_DESTCOLOR);
            data->surface->SetDstBlendFunction(data->surface, DSBF_ZERO);
            break;
        }

        SDL_DFB_CHECKERR(data->surface->
                         SetBlittingFlags(data->surface, flags));
        if (srcrect->w == dstrect->w && srcrect->h == dstrect->h) {
            SDL_DFB_CHECKERR(data->surface->
                             Blit(data->surface, texturedata->surface, &sr,
                                  dr.x, dr.y));
        } else {
            SDL_DFB_CHECKERR(data->surface->
                             StretchBlit(data->surface, texturedata->surface,
                                         &sr, &dr));
        }
    }
    return 0;
  error:
    return -1;
}

static void
DirectFB_RenderPresent(SDL_Renderer * renderer)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);

    SDL_DirtyRect *dirty;
    DFBRectangle sr;
    DFBResult ret;

    sr.x = 0;
    sr.y = 0;
    sr.w = window->w;
    sr.h = window->h;

    /* Send the data to the display */
    SDL_DFB_CHECKERR(data->surface->
                     Flip(data->surface, NULL, data->flipflags));

    return;
  error:
    return;
}

static void
DirectFB_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;

    if (!data) {
        return;
    }
    SDL_DFB_RELEASE(data->palette);
    SDL_DFB_RELEASE(data->surface);
    if (data->display) {
        data->display->vidIDinuse = 0;
        data->vidlayer->SetCooperativeLevel(data->vidlayer,
                                            DLSCL_ADMINISTRATIVE);
    }
    SDL_DFB_RELEASE(data->vidlayer);
    SDL_free(data);
    texture->driverdata = NULL;
}

static void
DirectFB_DestroyRenderer(SDL_Renderer * renderer)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    int i;

    if (data) {
        data->surface = NULL;
        SDL_free(data);
    }
    SDL_free(renderer);
}

/* vi: set ts=4 sw=4 expandtab: */
