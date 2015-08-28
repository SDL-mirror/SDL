/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#ifndef SDL_os4utils_h
#define SDL_os4utils_h

#include <exec/types.h>
#include "../SDL_sysvideo.h"

uint32		os4video_PFtoPPF				(const SDL_PixelFormat *vf);
BOOL		os4video_PPFtoPF				(SDL_PixelFormat *vformat, RGBFTYPE p96Format);
BOOL		os4video_PixelFormatFromModeID	(SDL_PixelFormat *vformat, uint32 displayID);
uint32		os4video_RTGFB2Bits				(RGBFTYPE rgbfmt);
RGBFTYPE   *os4video_GetP96FormatsForBpp	(uint32 bpp);
SDL_Rect  **os4video_MakeResArray			(const SDL_PixelFormat *format);
uint32		os4video_FindMode				(uint32 w, uint32 h, uint32 bpp, uint32 flags);

void	   *SaveAllocPooled					(struct SDL_PrivateVideoData *hidden, uint32 size);
void	   *SaveAllocVecPooled				(struct SDL_PrivateVideoData *hidden, uint32 size);
void		SaveFreePooled					(struct SDL_PrivateVideoData *hidden, void *mem, uint32 size);
void		SaveFreeVecPooled				(struct SDL_PrivateVideoData *hidden, void *mem);

#endif
