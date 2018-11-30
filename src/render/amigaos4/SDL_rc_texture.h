/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#ifndef _SDL_rc_texture_h
#define _SDL_rc_texture_h

#include "../SDL_sysrender.h"

typedef struct
{
    struct BitMap *bitmap;
    struct BitMap *finalbitmap; /* Contains color modulated version of bitmap */
    APTR lock;
    Uint8 r, g, b; /* Last known color modulation parameters */
    Uint8 *rambuf; /* Work buffer for color modulation */
} OS4_TextureData;

extern int OS4_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);

extern int OS4_SetTextureColorMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);

extern int OS4_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * rect, const void *pixels,
                            int pitch);

extern int OS4_SetRenderTarget(SDL_Renderer * renderer, SDL_Texture * texture);

extern int OS4_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * rect, void **pixels, int *pitch);

extern void OS4_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);

extern void OS4_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture);

#endif

/* vi: set ts=4 sw=4 expandtab: */

