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

#if SDL_VIDEO_RENDER_GDI

#include "SDL_win32video.h"
#include "../SDL_yuv_sw_c.h"

/* GDI renderer implementation */

static SDL_Renderer *SDL_GDI_CreateRenderer(SDL_Window * window,
                                            Uint32 flags);
static int SDL_GDI_CreateTexture(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int SDL_GDI_QueryTexturePixels(SDL_Renderer * renderer,
                                      SDL_Texture * texture, void **pixels,
                                      int *pitch);
static int SDL_GDI_SetTexturePalette(SDL_Renderer * renderer,
                                     SDL_Texture * texture,
                                     const SDL_Color * colors, int firstcolor,
                                     int ncolors);
static int SDL_GDI_GetTexturePalette(SDL_Renderer * renderer,
                                     SDL_Texture * texture,
                                     SDL_Color * colors, int firstcolor,
                                     int ncolors);
static int SDL_GDI_UpdateTexture(SDL_Renderer * renderer,
                                 SDL_Texture * texture, const SDL_Rect * rect,
                                 const void *pixels, int pitch);
static int SDL_GDI_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                               const SDL_Rect * rect, int markDirty,
                               void **pixels, int *pitch);
static void SDL_GDI_UnlockTexture(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static void SDL_GDI_DirtyTexture(SDL_Renderer * renderer,
                                 SDL_Texture * texture, int numrects,
                                 const SDL_Rect * rects);
static void SDL_GDI_SelectRenderTexture(SDL_Renderer * renderer,
                                        SDL_Texture * texture);
static int SDL_GDI_RenderFill(SDL_Renderer * renderer, const SDL_Rect * rect,
                              Uint32 color);
static int SDL_GDI_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                              const SDL_Rect * srcrect,
                              const SDL_Rect * dstrect, int blendMode,
                              int scaleMode);
static int SDL_GDI_RenderReadPixels(SDL_Renderer * renderer,
                                    const SDL_Rect * rect, void *pixels,
                                    int pitch);
static int SDL_GDI_RenderWritePixels(SDL_Renderer * renderer,
                                     const SDL_Rect * rect,
                                     const void *pixels, int pitch);
static void SDL_GDI_RenderPresent(SDL_Renderer * renderer);
static void SDL_GDI_DestroyTexture(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static void SDL_GDI_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver SDL_GDI_RenderDriver = {
    SDL_GDI_CreateRenderer,
    {
     "gdi",
     (SDL_Renderer_PresentDiscard |
      SDL_Renderer_PresentCopy | SDL_Renderer_RenderTarget),
     (SDL_TextureBlendMode_None |
      SDL_TextureBlendMode_Mask | SDL_TextureBlendMode_Blend),
     (SDL_TextureScaleMode_None | SDL_TextureScaleMode_Fast),
     11,
     {
      SDL_PixelFormat_Index8,
      SDL_PixelFormat_RGB555,
      SDL_PixelFormat_RGB565,
      SDL_PixelFormat_RGB888,
      SDL_PixelFormat_BGR888,
      SDL_PixelFormat_ARGB8888,
      SDL_PixelFormat_RGBA8888,
      SDL_PixelFormat_ABGR8888,
      SDL_PixelFormat_BGRA8888,
      SDL_PixelFormat_YUY2,
      SDL_PixelFormat_UYVY},
     0,
     0}
};

typedef struct
{
    HWND hwnd;
    HDC window_hdc;
    HDC render_hdc;
    HDC memory_hdc;
    HDC current_hdc;
    LPBITMAPINFO bmi;
    HBITMAP window_bmp;
    void *window_pixels;
    int window_pitch;
} SDL_GDI_RenderData;

typedef struct
{
    SDL_SW_YUVTexture *yuv;
    Uint32 format;
    HPALETTE hpal;
    HBITMAP hbm;
    void *pixels;
    int pitch;
} SDL_GDI_TextureData;

static void
UpdateYUVTextureData(SDL_Texture * texture)
{
    SDL_GDI_TextureData *data = (SDL_GDI_TextureData *) texture->driverdata;
    SDL_Rect rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = texture->w;
    rect.h = texture->h;
    SDL_SW_CopyYUVToRGB(data->yuv, &rect, data->format, texture->w,
                        texture->h, data->pixels, data->pitch);
}

void
GDI_AddRenderDriver(_THIS)
{
    SDL_AddRenderDriver(0, &SDL_GDI_RenderDriver);
}

SDL_Renderer *
SDL_GDI_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_WindowData *windowdata = (SDL_WindowData *) window->driverdata;
    SDL_Renderer *renderer;
    SDL_GDI_RenderData *data;
    int bmi_size;
    HBITMAP hbm;

    renderer = (SDL_Renderer *) SDL_malloc(sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(renderer);

    data = (SDL_GDI_RenderData *) SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_GDI_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(data);

    data->hwnd = windowdata->hwnd;
    data->window_hdc = GetDC(data->hwnd);
    data->render_hdc = CreateCompatibleDC(data->window_hdc);
    data->memory_hdc = CreateCompatibleDC(data->window_hdc);
    data->current_hdc = data->window_hdc;

    /* Fill in the compatible bitmap info */
    bmi_size = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
    data->bmi = (LPBITMAPINFO) SDL_malloc(bmi_size);
    if (!data->bmi) {
        SDL_GDI_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_memset(data->bmi, 0, bmi_size);
    data->bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    hbm = CreateCompatibleBitmap(data->window_hdc, 1, 1);
    GetDIBits(data->window_hdc, hbm, 0, 1, NULL, data->bmi, DIB_RGB_COLORS);
    GetDIBits(data->window_hdc, hbm, 0, 1, NULL, data->bmi, DIB_RGB_COLORS);
    DeleteObject(hbm);

    renderer->CreateTexture = SDL_GDI_CreateTexture;
    renderer->QueryTexturePixels = SDL_GDI_QueryTexturePixels;
    renderer->SetTexturePalette = SDL_GDI_SetTexturePalette;
    renderer->GetTexturePalette = SDL_GDI_GetTexturePalette;
    renderer->UpdateTexture = SDL_GDI_UpdateTexture;
    renderer->LockTexture = SDL_GDI_LockTexture;
    renderer->UnlockTexture = SDL_GDI_UnlockTexture;
    renderer->DirtyTexture = SDL_GDI_DirtyTexture;
    renderer->SelectRenderTexture = SDL_GDI_SelectRenderTexture;
    renderer->RenderFill = SDL_GDI_RenderFill;
    renderer->RenderCopy = SDL_GDI_RenderCopy;
    renderer->RenderReadPixels = SDL_GDI_RenderReadPixels;
    renderer->RenderWritePixels = SDL_GDI_RenderWritePixels;
    renderer->RenderPresent = SDL_GDI_RenderPresent;
    renderer->DestroyTexture = SDL_GDI_DestroyTexture;
    renderer->DestroyRenderer = SDL_GDI_DestroyRenderer;
    renderer->info = SDL_GDI_RenderDriver.info;
    renderer->window = window->id;
    renderer->driverdata = data;

    renderer->info.flags = SDL_Renderer_RenderTarget;

    return renderer;
}

static int
SDL_GDI_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_GDI_RenderData *renderdata =
        (SDL_GDI_RenderData *) renderer->driverdata;
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);
    SDL_GDI_TextureData *data;

    data = (SDL_GDI_TextureData *) SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    SDL_zerop(data);

    texture->driverdata = data;

    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        if (texture->access == SDL_TextureAccess_Render) {
            SDL_SetError("Rendering to YUV format textures is not supported");
            return -1;
        }
        data->yuv = SDL_SW_CreateYUVTexture(texture);
        if (!data->yuv) {
            SDL_GDI_DestroyTexture(renderer, texture);
            return -1;
        }
        data->format = display->current_mode.format;
    } else {
        data->format = texture->format;
    }
    data->pitch = (texture->w * SDL_BYTESPERPIXEL(data->format));

    if (data->yuv || texture->access == SDL_TextureAccess_Local
        || texture->format != SDL_GetCurrentDisplayMode()->format) {
        int bmi_size;
        LPBITMAPINFO bmi;

        bmi_size = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
        bmi = (LPBITMAPINFO) SDL_malloc(bmi_size);
        if (!bmi) {
            SDL_GDI_DestroyTexture(renderer, texture);
            SDL_OutOfMemory();
            return -1;
        }
        SDL_memset(bmi, 0, bmi_size);
        bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi->bmiHeader.biWidth = texture->w;
        bmi->bmiHeader.biHeight = -texture->h;  /* topdown bitmap */
        bmi->bmiHeader.biPlanes = 1;
        bmi->bmiHeader.biSizeImage = texture->h * data->pitch;
        bmi->bmiHeader.biXPelsPerMeter = 0;
        bmi->bmiHeader.biYPelsPerMeter = 0;
        bmi->bmiHeader.biClrUsed = 0;
        bmi->bmiHeader.biClrImportant = 0;
        bmi->bmiHeader.biBitCount = SDL_BYTESPERPIXEL(data->format) * 8;
        if (SDL_ISPIXELFORMAT_INDEXED(data->format)) {
            int i, ncolors;
            LOGPALETTE *palette;

            bmi->bmiHeader.biCompression = BI_RGB;
            ncolors = (1 << SDL_BITSPERPIXEL(data->format));
            palette =
                (LOGPALETTE *) SDL_malloc(sizeof(*palette) +
                                          ncolors * sizeof(PALETTEENTRY));
            if (!palette) {
                SDL_free(bmi);
                SDL_GDI_DestroyTexture(renderer, texture);
                SDL_OutOfMemory();
                return -1;
            }
            palette->palVersion = 0x300;
            palette->palNumEntries = ncolors;
            for (i = 0; i < ncolors; ++i) {
                palette->palPalEntry[i].peRed = 0xFF;
                palette->palPalEntry[i].peGreen = 0xFF;
                palette->palPalEntry[i].peBlue = 0xFF;
                palette->palPalEntry[i].peFlags = 0;
            }
            data->hpal = CreatePalette(palette);
            SDL_free(palette);
        } else {
            int bpp;
            Uint32 Rmask, Gmask, Bmask, Amask;

            bmi->bmiHeader.biCompression = BI_BITFIELDS;
            SDL_PixelFormatEnumToMasks(data->format, &bpp, &Rmask, &Gmask,
                                       &Bmask, &Amask);
            ((Uint32 *) bmi->bmiColors)[0] = Rmask;
            ((Uint32 *) bmi->bmiColors)[1] = Gmask;
            ((Uint32 *) bmi->bmiColors)[2] = Bmask;
            data->hpal = NULL;
        }
        data->hbm =
            CreateDIBSection(renderdata->memory_hdc, bmi, DIB_RGB_COLORS,
                             &data->pixels, NULL, 0);
    } else {
        data->hbm =
            CreateCompatibleBitmap(renderdata->window_hdc, texture->w,
                                   texture->h);
        data->pixels = NULL;
    }
    if (!data->hbm) {
        SDL_GDI_DestroyTexture(renderer, texture);
        WIN_SetError("Couldn't create bitmap");
        return -1;
    }
    return 0;
}

static int
SDL_GDI_QueryTexturePixels(SDL_Renderer * renderer, SDL_Texture * texture,
                           void **pixels, int *pitch)
{
    SDL_GDI_TextureData *data = (SDL_GDI_TextureData *) texture->driverdata;

    if (data->yuv) {
        return SDL_SW_QueryYUVTexturePixels(data->yuv, pixels, pitch);
    } else {
        *pixels = data->pixels;
        *pitch = texture->w * SDL_BYTESPERPIXEL(texture->format);
        return 0;
    }
}

static int
SDL_GDI_SetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Color * colors, int firstcolor,
                          int ncolors)
{
    SDL_GDI_RenderData *renderdata =
        (SDL_GDI_RenderData *) renderer->driverdata;
    SDL_GDI_TextureData *data = (SDL_GDI_TextureData *) texture->driverdata;

    if (data->yuv) {
        SDL_SetError("YUV textures don't have a palette");
        return -1;
    } else {
        PALETTEENTRY entries[256];
        int i;

        for (i = 0; i < ncolors; ++i) {
            entries[i].peRed = colors[i].r;
            entries[i].peGreen = colors[i].g;
            entries[i].peBlue = colors[i].b;
            entries[i].peFlags = 0;
        }
        if (!SetPaletteEntries(data->hpal, firstcolor, ncolors, entries)) {
            WIN_SetError("SetPaletteEntries()");
            return -1;
        }
        return 0;
    }
}

static int
SDL_GDI_GetTexturePalette(SDL_Renderer * renderer, SDL_Texture * texture,
                          SDL_Color * colors, int firstcolor, int ncolors)
{
    SDL_GDI_TextureData *data = (SDL_GDI_TextureData *) texture->driverdata;

    if (data->yuv) {
        SDL_SetError("YUV textures don't have a palette");
        return -1;
    } else {
        PALETTEENTRY entries[256];
        int i;

        if (!GetPaletteEntries(data->hpal, firstcolor, ncolors, entries)) {
            WIN_SetError("GetPaletteEntries()");
            return -1;
        }
        for (i = 0; i < ncolors; ++i) {
            colors[i].r = entries[i].peRed;
            colors[i].g = entries[i].peGreen;
            colors[i].b = entries[i].peBlue;
        }
        return 0;
    }
}

static int
SDL_GDI_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                      const SDL_Rect * rect, const void *pixels, int pitch)
{
    SDL_GDI_TextureData *data = (SDL_GDI_TextureData *) texture->driverdata;

    if (data->yuv) {
        if (SDL_SW_UpdateYUVTexture(data->yuv, rect, pixels, pitch) < 0) {
            return -1;
        }
        UpdateYUVTextureData(texture);
        return 0;
    } else {
        SDL_GDI_RenderData *renderdata =
            (SDL_GDI_RenderData *) renderer->driverdata;

        if (data->pixels) {
            Uint8 *src, *dst;
            int row;
            size_t length;

            src = (Uint8 *) pixels;
            dst =
                (Uint8 *) data->pixels + rect->y * data->pitch +
                rect->x * SDL_BYTESPERPIXEL(texture->format);
            length = rect->w * SDL_BYTESPERPIXEL(texture->format);
            for (row = 0; row < rect->h; ++row) {
                SDL_memcpy(dst, src, length);
                src += pitch;
                dst += data->pitch;
            }
        } else if (rect->w == texture->w && pitch == data->pitch) {
            if (!SetDIBits
                (renderdata->window_hdc, data->hbm, rect->y, rect->h, pixels,
                 renderdata->bmi, DIB_RGB_COLORS)) {
                WIN_SetError("SetDIBits()");
                return -1;
            }
        } else {
            SDL_SetError
                ("FIXME: Need to allocate temporary memory and do GetDIBits() followed by SetDIBits(), since we can only set blocks of scanlines at a time");
            return -1;
        }
        return 0;
    }
}

static int
SDL_GDI_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                    const SDL_Rect * rect, int markDirty, void **pixels,
                    int *pitch)
{
    SDL_GDI_TextureData *data = (SDL_GDI_TextureData *) texture->driverdata;

    if (data->yuv) {
        return SDL_SW_LockYUVTexture(data->yuv, rect, markDirty, pixels,
                                     pitch);
    } else {
        GdiFlush();
        *pixels =
            (void *) ((Uint8 *) data->pixels + rect->y * data->pitch +
                      rect->x * SDL_BYTESPERPIXEL(texture->format));
        *pitch = data->pitch;
        return 0;
    }
}

static void
SDL_GDI_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_GDI_TextureData *data = (SDL_GDI_TextureData *) texture->driverdata;

    if (data->yuv) {
        SDL_SW_UnlockYUVTexture(data->yuv);
        UpdateYUVTextureData(texture);
    }
}

static void
SDL_GDI_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                     int numrects, const SDL_Rect * rects)
{
}

static void
SDL_GDI_SelectRenderTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_GDI_RenderData *data = (SDL_GDI_RenderData *) renderer->driverdata;

    if (texture) {
        SDL_GDI_TextureData *texturedata =
            (SDL_GDI_TextureData *) texture->driverdata;
        SelectObject(data->render_hdc, texturedata->hbm);
        if (texturedata->hpal) {
            SelectPalette(data->render_hdc, texturedata->hpal, TRUE);
            RealizePalette(data->render_hdc);
        }
        data->current_hdc = data->render_hdc;
    } else {
        data->current_hdc = data->current_hdc;
    }
}

static int
SDL_GDI_RenderFill(SDL_Renderer * renderer, const SDL_Rect * rect,
                   Uint32 color)
{
    SDL_GDI_RenderData *data = (SDL_GDI_RenderData *) renderer->driverdata;
    Uint8 r, g, b;
    RECT rc;
    static HBRUSH brush;
    int status;

    r = (Uint8) ((color >> 16) & 0xFF);
    g = (Uint8) ((color >> 8) & 0xFF);
    b = (Uint8) (color & 0xFF);

    rc.left = rect->x;
    rc.top = rect->y;
    rc.right = rect->x + rect->w + 1;
    rc.bottom = rect->y + rect->h + 1;

    /* Should we cache the brushes? .. it looks like GDI does for us. :) */
    brush = CreateSolidBrush(RGB(r, g, b));
    SelectObject(data->current_hdc, brush);
    status = FillRect(data->current_hdc, &rc, brush);
    DeleteObject(brush);

    if (!status) {
        WIN_SetError("FillRect()");
        return -1;
    }
    return 0;
}

static int
SDL_GDI_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                   const SDL_Rect * srcrect, const SDL_Rect * dstrect,
                   int blendMode, int scaleMode)
{
    SDL_GDI_RenderData *data = (SDL_GDI_RenderData *) renderer->driverdata;
    SDL_GDI_TextureData *texturedata =
        (SDL_GDI_TextureData *) texture->driverdata;

    SelectObject(data->memory_hdc, texturedata->hbm);
    if (texturedata->hpal) {
        SelectPalette(data->memory_hdc, texturedata->hpal, TRUE);
        RealizePalette(data->memory_hdc);
    }
    if (blendMode & (SDL_TextureBlendMode_Mask | SDL_TextureBlendMode_Blend)) {
        static BLENDFUNCTION blendFunc = {
            AC_SRC_OVER,
            0,
            255,
            AC_SRC_ALPHA
        };
        /* FIXME: GDI uses premultiplied alpha! */
        if (!AlphaBlend
            (data->current_hdc, dstrect->x, dstrect->y, dstrect->w,
             dstrect->h, data->memory_hdc, srcrect->x, srcrect->y, srcrect->w,
             srcrect->h, blendFunc)) {
            WIN_SetError("AlphaBlend()");
            return -1;
        }
    } else {
        if (srcrect->w == dstrect->w && srcrect->h == dstrect->h) {
            if (!BitBlt
                (data->current_hdc, dstrect->x, dstrect->y, dstrect->w,
                 srcrect->h, data->memory_hdc, srcrect->x, srcrect->y,
                 SRCCOPY)) {
                WIN_SetError("BitBlt()");
                return -1;
            }
        } else {
            if (!StretchBlt
                (data->current_hdc, dstrect->x, dstrect->y, dstrect->w,
                 dstrect->h, data->memory_hdc, srcrect->x, srcrect->y,
                 srcrect->w, srcrect->h, SRCCOPY)) {
                WIN_SetError("StretchBlt()");
                return -1;
            }
        }
    }
    return 0;
}

static int
CreateWindowDIB(SDL_GDI_RenderData * data, SDL_Window * window)
{
    data->window_pitch = window->w * (data->bmi->bmiHeader.biBitCount / 8);
    data->bmi->bmiHeader.biWidth = window->w;
    data->bmi->bmiHeader.biHeight = -window->h;
    data->bmi->bmiHeader.biSizeImage =
        window->h * (data->bmi->bmiHeader.biBitCount / 8);
    data->window_bmp =
        CreateDIBSection(data->window_hdc, data->bmi, DIB_RGB_COLORS,
                         &data->window_pixels, NULL, 0);
    if (!data->window_bmp) {
        WIN_SetError("CreateDIBSection()");
        return -1;
    }
    return 0;
}

static int
SDL_GDI_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                         void *pixels, int pitch)
{
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_GDI_RenderData *data = (SDL_GDI_RenderData *) renderer->driverdata;

    if (!data->window_bmp) {
        if (CreateWindowDIB(data, window) < 0) {
            return -1;
        }
    }

    SelectObject(data->memory_hdc, data->window_bmp);
    BitBlt(data->memory_hdc, rect->x, rect->y, rect->w, rect->h,
           data->window_hdc, rect->x, rect->y, SRCCOPY);

    {
        int bpp = data->bmi->bmiHeader.biBitCount / 8;
        Uint8 *src =
            (Uint8 *) data->window_pixels + rect->y * data->window_pitch +
            rect->x * bpp;
        Uint8 *dst = (Uint8 *) pixels;
        int row;

        for (row = 0; row < rect->h; ++row) {
            SDL_memcpy(dst, src, rect->w * bpp);
            src += data->window_pitch;
            dst += pitch;
        }
    }

    return 0;
}

static int
SDL_GDI_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                          const void *pixels, int pitch)
{
    SDL_Window *window = SDL_GetWindowFromID(renderer->window);
    SDL_GDI_RenderData *data = (SDL_GDI_RenderData *) renderer->driverdata;

    if (!data->window_bmp) {
        if (CreateWindowDIB(data, window) < 0) {
            return -1;
        }
    }

    {
        int bpp = data->bmi->bmiHeader.biBitCount / 8;
        Uint8 *src = (Uint8 *) pixels;
        Uint8 *dst =
            (Uint8 *) data->window_pixels + rect->y * data->window_pitch +
            rect->x * bpp;
        int row;

        for (row = 0; row < rect->h; ++row) {
            SDL_memcpy(dst, src, rect->w * bpp);
            src += pitch;
            dst += data->window_pitch;
        }
    }

    SelectObject(data->memory_hdc, data->window_bmp);
    BitBlt(data->window_hdc, rect->x, rect->y, rect->w, rect->h,
           data->memory_hdc, rect->x, rect->y, SRCCOPY);

    return 0;
}

static void
SDL_GDI_RenderPresent(SDL_Renderer * renderer)
{
}

static void
SDL_GDI_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_GDI_TextureData *data = (SDL_GDI_TextureData *) texture->driverdata;

    if (!data) {
        return;
    }
    if (data->yuv) {
        SDL_SW_DestroyYUVTexture(data->yuv);
    }
    if (data->hpal) {
        DeleteObject(data->hpal);
    }
    if (data->hbm) {
        DeleteObject(data->hbm);
    }
    SDL_free(data);
    texture->driverdata = NULL;
}

void
SDL_GDI_DestroyRenderer(SDL_Renderer * renderer)
{
    SDL_GDI_RenderData *data = (SDL_GDI_RenderData *) renderer->driverdata;

    if (data) {
        ReleaseDC(data->hwnd, data->window_hdc);
        DeleteDC(data->render_hdc);
        DeleteDC(data->memory_hdc);
        if (data->bmi) {
            SDL_free(data->bmi);
        }
        if (data->window_bmp) {
            DeleteObject(data->window_bmp);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_RENDER_GDI */

/* vi: set ts=4 sw=4 expandtab: */
