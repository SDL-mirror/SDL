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

/* This is the implementation of the YUV video surface support */

#include <stdlib.h>
#include <string.h>

#include "SDL_getenv.h"
#include "SDL_video.h"
#include "SDL_error.h"
#include "SDL_sysvideo.h"
#include "SDL_yuvfuncs.h"
#include "SDL_yuv_sw_c.h"


SDL_Overlay *SDL_CreateYUVOverlay(int w, int h, Uint32 format,
                                  SDL_Surface *display)
{
	SDL_VideoDevice *video = current_video;
	SDL_VideoDevice *this  = current_video;
	const char *yuv_hwaccel;
	SDL_Overlay *overlay;

	if ( (display->flags & SDL_OPENGL) == SDL_OPENGL ) {
		SDL_SetError("YUV overlays are not supported in OpenGL mode");
		return NULL;
	}

	/* Display directly on video surface, if possible */
	if ( getenv("SDL_VIDEO_YUV_DIRECT") ) {
		if ( (display == SDL_PublicSurface) &&
		     ((SDL_VideoSurface->format->BytesPerPixel == 2) ||
		      (SDL_VideoSurface->format->BytesPerPixel == 4)) ) {
			display = SDL_VideoSurface;
		}
	}
	overlay = NULL;
        yuv_hwaccel = getenv("SDL_VIDEO_YUV_HWACCEL");
	if ( ((display == SDL_VideoSurface) && video->CreateYUVOverlay) &&
	     (!yuv_hwaccel || (atoi(yuv_hwaccel) > 0)) ) {
		overlay = video->CreateYUVOverlay(this, w, h, format, display);
	}
	/* If hardware YUV overlay failed ... */
	if ( overlay == NULL ) {
		overlay = SDL_CreateYUV_SW(this, w, h, format, display);
	}
	return overlay;
}

int SDL_LockYUVOverlay(SDL_Overlay *overlay)
{
	return overlay->hwfuncs->Lock(current_video, overlay);
}

void SDL_UnlockYUVOverlay(SDL_Overlay *overlay)
{
	overlay->hwfuncs->Unlock(current_video, overlay);
}

int SDL_DisplayYUVOverlay(SDL_Overlay *overlay, SDL_Rect *dstrect)
{
	return overlay->hwfuncs->Display(current_video, overlay, dstrect);
}

void SDL_FreeYUVOverlay(SDL_Overlay *overlay)
{
	if ( overlay ) {
		if ( overlay->hwfuncs ) {
			overlay->hwfuncs->FreeHW(current_video, overlay);
		}
		free(overlay);
	}
}
