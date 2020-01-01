/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

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

#ifndef _SDL_render_compositing_h
#define _SDL_render_compositing_h

#include "../SDL_sysrender.h"

#include "graphics/rastport.h"

typedef struct
{
    struct GraphicsIFace *iGraphics;
    struct LayersIFace * iLayers;
    struct BitMap *bitmap;
    struct BitMap *target;
    struct BitMap *solidcolor;
    struct RastPort rastport;

    SDL_Rect cliprect;
    SDL_bool cliprect_enabled;

    SDL_Rect viewport;

    SDL_bool vsyncEnabled;
} OS4_RenderData;

extern struct BitMap * OS4_ActivateRenderer(SDL_Renderer * renderer);
extern struct BitMap * OS4_AllocBitMap(SDL_Renderer * renderer, int width, int height, int depth);
extern SDL_bool OS4_IsColorModEnabled(SDL_Texture * texture);

#endif

/* vi: set ts=4 sw=4 expandtab: */

