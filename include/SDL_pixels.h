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

/**
 * \file SDL_pixels.h
 *
 * Header for the enumerated pixel format definitions
 */

#ifndef _SDL_pixels_h
#define _SDL_pixels_h

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

enum
{                               /* Pixel type */
    SDL_PixelType_Unknown,
    SDL_PixelType_Index1,
    SDL_PixelType_Index4,
    SDL_PixelType_Index8,
    SDL_PixelType_Packed8,
    SDL_PixelType_Packed16,
    SDL_PixelType_Packed32,
    SDL_PixelType_ArrayU8,
    SDL_PixelType_ArrayU16,
    SDL_PixelType_ArrayU32,
    SDL_PixelType_ArrayF16,
    SDL_PixelType_ArrayF32,
};

enum
{                               /* bitmap pixel order, high bit -> low bit */
    SDL_BitmapOrder_None,
    SDL_BitmapOrder_4321,
    SDL_BitmapOrder_1234,
};
enum
{                               /* packed component order, high bit -> low bit */
    SDL_PackedOrder_None,
    SDL_PackedOrder_XRGB,
    SDL_PackedOrder_RGBX,
    SDL_PackedOrder_ARGB,
    SDL_PackedOrder_RGBA,
    SDL_PackedOrder_XBGR,
    SDL_PackedOrder_BGRX,
    SDL_PackedOrder_ABGR,
    SDL_PackedOrder_BGRA,
};
enum
{                               /* array component order, low byte -> high byte */
    SDL_ArrayOrder_None,
    SDL_ArrayOrder_RGB,
    SDL_ArrayOrder_RGBA,
    SDL_ArrayOrder_ARGB,
    SDL_ArrayOrder_BGR,
    SDL_ArrayOrder_BGRA,
    SDL_ArrayOrder_ABGR,
};

enum
{                               /* Packed component layout */
    SDL_PackedLayout_None,
    SDL_PackedLayout_332,
    SDL_PackedLayout_4444,
    SDL_PackedLayout_1555,
    SDL_PackedLayout_5551,
    SDL_PackedLayout_565,
    SDL_PackedLayout_8888,
    SDL_PackedLayout_2101010,
    SDL_PackedLayout_1010102,
};

#define SDL_DEFINE_PIXELFOURCC(A, B, C, D) \
    ((A) | ((B) << 8) | ((C) << 16) | ((D) << 24))

#define SDL_DEFINE_PIXELFORMAT(type, order, layout, bits, bytes) \
    ((1 << 31) | ((type) << 24) | ((order) << 20) | ((layout) << 16) | \
     ((bits) << 8) | ((bytes) << 0))

#define SDL_PIXELTYPE(X)	(((X) >> 24) & 0x0F)
#define SDL_PIXELORDER(X)	(((X) >> 20) & 0x0F)
#define SDL_PIXELLAYOUT(X)	(((X) >> 16) & 0x0F)
#define SDL_BITSPERPIXEL(X)	(((X) >> 8) & 0xFF)
#define SDL_BYTESPERPIXEL(X)	(((X) >> 0) & 0xFF)

#define SDL_ISPIXELFORMAT_INDEXED(format)   \
    ((SDL_PIXELTYPE(format) == SDL_PixelType_Index1) || \
     (SDL_PIXELTYPE(format) == SDL_PixelType_Index4) || \
     (SDL_PIXELTYPE(format) == SDL_PixelType_Index8))

#define SDL_ISPIXELFORMAT_FOURCC(format)    \
    ((format) && !((format) & 0x80000000))

enum
{
    SDL_PixelFormat_Unknown,
    SDL_PixelFormat_Index1LSB =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Index1, SDL_BitmapOrder_1234, 0,
                               1, 0),
    SDL_PixelFormat_Index1MSB =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Index1, SDL_BitmapOrder_4321, 0,
                               1, 0),
    SDL_PixelFormat_Index4LSB =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Index4, SDL_BitmapOrder_1234, 0,
                               2, 0),
    SDL_PixelFormat_Index4MSB =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Index4, SDL_BitmapOrder_4321, 0,
                               2, 0),
    SDL_PixelFormat_Index8 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Index8, 0, 0, 8, 1),
    SDL_PixelFormat_RGB332 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed8, SDL_PackedOrder_XRGB,
                               SDL_PackedLayout_332, 8, 1),
    SDL_PixelFormat_RGB444 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed16, SDL_PackedOrder_XRGB,
                               SDL_PackedLayout_4444, 12, 2),
    SDL_PixelFormat_RGB555 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed16, SDL_PackedOrder_XRGB,
                               SDL_PackedLayout_1555, 15, 2),
    SDL_PixelFormat_ARGB4444 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed16, SDL_PackedOrder_ARGB,
                               SDL_PackedLayout_4444, 16, 2),
    SDL_PixelFormat_ARGB1555 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed16, SDL_PackedOrder_ARGB,
                               SDL_PackedLayout_1555, 16, 2),
    SDL_PixelFormat_RGB565 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed16, SDL_PackedOrder_XRGB,
                               SDL_PackedLayout_565, 16, 2),
    SDL_PixelFormat_RGB24 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_ArrayU8, SDL_ArrayOrder_RGB, 0,
                               24, 3),
    SDL_PixelFormat_BGR24 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_ArrayU8, SDL_ArrayOrder_BGR, 0,
                               24, 3),
    SDL_PixelFormat_RGB888 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed32, SDL_PackedOrder_XRGB,
                               SDL_PackedLayout_8888, 24, 4),
    SDL_PixelFormat_BGR888 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed32, SDL_PackedOrder_XBGR,
                               SDL_PackedLayout_8888, 24, 4),
    SDL_PixelFormat_ARGB8888 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed32, SDL_PackedOrder_ARGB,
                               SDL_PackedLayout_8888, 32, 4),
    SDL_PixelFormat_RGBA8888 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed32, SDL_PackedOrder_RGBA,
                               SDL_PackedLayout_8888, 32, 4),
    SDL_PixelFormat_ABGR8888 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed32, SDL_PackedOrder_ABGR,
                               SDL_PackedLayout_8888, 32, 4),
    SDL_PixelFormat_BGRA8888 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed32, SDL_PackedOrder_BGRA,
                               SDL_PackedLayout_8888, 32, 4),
    SDL_PixelFormat_ARGB2101010 =
        SDL_DEFINE_PIXELFORMAT(SDL_PixelType_Packed32, SDL_PackedOrder_ARGB,
                               SDL_PackedLayout_2101010, 32, 4),

    SDL_PixelFormat_YV12 =      /* Planar mode: Y + V + U  (3 planes) */
        SDL_DEFINE_PIXELFOURCC('Y', 'V', '1', '2'),
    SDL_PixelFormat_IYUV =      /* Planar mode: Y + U + V  (3 planes) */
        SDL_DEFINE_PIXELFOURCC('I', 'Y', 'U', 'V'),
    SDL_PixelFormat_YUY2 =      /* Packed mode: Y0+U0+Y1+V0 (1 plane) */
        SDL_DEFINE_PIXELFOURCC('Y', 'U', 'Y', '2'),
    SDL_PixelFormat_UYVY =      /* Packed mode: U0+Y0+V0+Y1 (1 plane) */
        SDL_DEFINE_PIXELFOURCC('U', 'Y', 'V', 'Y'),
    SDL_PixelFormat_YVYU =      /* Packed mode: Y0+V0+Y1+U0 (1 plane) */
        SDL_DEFINE_PIXELFOURCC('Y', 'V', 'Y', 'U'),
};

typedef struct SDL_Color
{
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 unused;
} SDL_Color;
#define SDL_Colour SDL_Color

typedef struct SDL_Palette SDL_Palette;
typedef int (*SDL_PaletteChangedFunc) (void *userdata, SDL_Palette * palette);

typedef struct SDL_PaletteWatch
{
    SDL_PaletteChangedFunc callback;
    void *userdata;
    struct SDL_PaletteWatch *next;
} SDL_PaletteWatch;

struct SDL_Palette
{
    int ncolors;
    SDL_Color *colors;

    int refcount;
    SDL_PaletteWatch *watch;
};

/* Everything in the pixel format structure is read-only */
typedef struct SDL_PixelFormat
{
    SDL_Palette *palette;
    Uint8 BitsPerPixel;
    Uint8 BytesPerPixel;
    Uint8 Rloss;
    Uint8 Gloss;
    Uint8 Bloss;
    Uint8 Aloss;
    Uint8 Rshift;
    Uint8 Gshift;
    Uint8 Bshift;
    Uint8 Ashift;
    Uint32 Rmask;
    Uint32 Gmask;
    Uint32 Bmask;
    Uint32 Amask;

    /* RGB color key information */
    Uint32 colorkey;
    /* Alpha value information (per-surface alpha) */
    Uint8 alpha;
} SDL_PixelFormat;

/**
 * \fn SDL_bool SDL_PixelFormatEnumToMasks(Uint32 format, int *bpp, Uint32 * Rmask, Uint32 * Gmask, Uint32 * Bmask, Uint32 * Amask)
 *
 * \brief Convert one of the enumerated pixel formats to a bpp and RGBA masks.
 *
 * \return SDL_TRUE, or SDL_FALSE if the conversion wasn't possible.
 *
 * \sa SDL_MasksToPixelFormatEnum()
 */
extern DECLSPEC SDL_bool SDLCALL SDL_PixelFormatEnumToMasks(Uint32 format,
                                                            int *bpp,
                                                            Uint32 * Rmask,
                                                            Uint32 * Gmask,
                                                            Uint32 * Bmask,
                                                            Uint32 * Amask);

/**
 * \fn Uint32 SDL_MasksToPixelFormatEnum(int bpp, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
 *
 * \brief Convert a bpp and RGBA masks to an enumerated pixel format.
 *
 * \return The pixel format, or SDL_PixelFormat_Unknown if the conversion wasn't possible.
 *
 * \sa SDL_PixelFormatEnumToMasks()
 */
extern DECLSPEC Uint32 SDLCALL SDL_MasksToPixelFormatEnum(int bpp,
                                                          Uint32 Rmask,
                                                          Uint32 Gmask,
                                                          Uint32 Bmask,
                                                          Uint32 Amask);

/**
 * \fn SDL_Palette *SDL_AllocPalette(int ncolors)
 *
 * \brief Create a palette structure with the specified number of color entries.
 *
 * \return A new palette, or NULL if there wasn't enough memory
 *
 * \note The palette entries are initialized to white.
 *
 * \sa SDL_FreePalette()
 */
extern DECLSPEC SDL_Palette *SDLCALL SDL_AllocPalette(int ncolors);

/**
 * \fn int SDL_AddPaletteWatch(SDL_Palette *palette, SDL_PaletteChangedFunc callback, void *userdata)
 *
 * \brief Add a callback function which is called when the palette changes.
 *
 * \sa SDL_DelPaletteWatch()
 */
extern DECLSPEC int SDLCALL SDL_AddPaletteWatch(SDL_Palette * palette,
                                                SDL_PaletteChangedFunc
                                                callback, void *userdata);

/**
 * \fn void SDL_DelPaletteWatch(SDL_Palette *palette, SDL_PaletteChangedFunc callback, void *userdata)
 *
 * \brief Remove a callback function previously added with SDL_AddPaletteWatch()
 *
 * \sa SDL_AddPaletteWatch()
 */
extern DECLSPEC void SDLCALL SDL_DelPaletteWatch(SDL_Palette * palette,
                                                 SDL_PaletteChangedFunc
                                                 callback, void *userdata);

/**
 * \fn int SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Colors *colors, int firstcolor, int numcolors)
 *
 * \brief Set a range of colors in a palette.
 *
 * \param palette The palette to modify
 * \param colors An array of colors to copy into the palette
 * \param firstcolor The index of the first palette entry to modify
 * \param ncolors The number of entries to modify
 *
 * \return 0 on success, or -1 if not all of the colors could be set
 */
extern DECLSPEC int SDLCALL SDL_SetPaletteColors(SDL_Palette * palette,
                                                 const SDL_Color * colors,
                                                 int firstcolor, int ncolors);

/**
 * \fn void SDL_FreePalette(SDL_Palette *palette)
 *
 * \brief Free a palette created with SDL_AllocPalette()
 *
 * \sa SDL_AllocPalette()
 */
extern DECLSPEC void SDLCALL SDL_FreePalette(SDL_Palette * palette);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_pixels_h */

/* vi: set ts=4 sw=4 expandtab: */
