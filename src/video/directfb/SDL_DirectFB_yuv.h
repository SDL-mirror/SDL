/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

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

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* This is the DirectFB implementation of YUV video overlays */

#include "SDL_video.h"
#include "SDL_DirectFB_video.h"

extern SDL_Overlay *DirectFB_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface *display);

extern int DirectFB_LockYUVOverlay(_THIS, SDL_Overlay *overlay);

extern void DirectFB_UnlockYUVOverlay(_THIS, SDL_Overlay *overlay);

extern int DirectFB_DisplayYUVOverlay(_THIS, SDL_Overlay *overlay, SDL_Rect *dstrect);

extern void DirectFB_FreeYUVOverlay(_THIS, SDL_Overlay *overlay);

