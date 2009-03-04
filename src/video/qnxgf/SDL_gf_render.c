/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

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

#include "../SDL_pixels_c.h"
#include "../SDL_yuv_sw_c.h"

#include "SDL_video.h"

#include "SDL_gf_render.h"

static SDL_Renderer* GF_CreateRenderer(SDL_Window* window, Uint32 flags)
{
}

SDL_RenderDriver GF_RenderDriver=
{
    GF_CreateRenderer,
    {
       "qnxgf",
       (SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTCOPY |
        SDL_RENDERER_PRESENTFLIP2 | SDL_RENDERER_PRESENTFLIP3 |
        SDL_RENDERER_PRESENTDISCARD | SDL_RENDERER_PRESENTVSYNC |
        SDL_RENDERER_ACCELERATED),
       (SDL_TEXTUREMODULATE_NONE | SDL_TEXTUREMODULATE_COLOR |
        SDL_TEXTUREMODULATE_ALPHA),
       (SDL_BLENDMODE_NONE | SDL_BLENDMODE_MASK |
        SDL_BLENDMODE_BLEND | SDL_BLENDMODE_ADD | SDL_BLENDMODE_MOD),
       (SDL_TEXTURESCALEMODE_NONE | SDL_TEXTURESCALEMODE_SLOW),
       13,
       {
          SDL_PIXELFORMAT_INDEX8,
          SDL_PIXELFORMAT_RGB555,
          SDL_PIXELFORMAT_RGB565,
          SDL_PIXELFORMAT_RGB888,
          SDL_PIXELFORMAT_BGR888,
          SDL_PIXELFORMAT_ARGB8888,
          SDL_PIXELFORMAT_RGBA8888,
          SDL_PIXELFORMAT_ABGR8888,
          SDL_PIXELFORMAT_BGRA8888,
          SDL_PIXELFORMAT_YV12,
          SDL_PIXELFORMAT_YUY2,
          SDL_PIXELFORMAT_UYVY,
          SDL_PIXELFORMAT_YVYU
       },
       0,
       0
    }
};
