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

/* This is the XFree86 Xv extension implementation of YUV video overlays */

#ifdef XFREE86_XV

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_x11yuv_c.h"
#include "SDL_yuvfuncs.h"

/* The functions used to manipulate software video overlays */
static struct private_yuvhwfuncs x11_yuvfuncs = {
	X11_LockYUVOverlay,
	X11_UnlockYUVOverlay,
	X11_DisplayYUVOverlay,
	X11_FreeYUVOverlay
};

struct private_yuvhwdata {
	int port;
	XShmSegmentInfo yuvshm;
	XvImage *image;
};


SDL_Overlay *X11_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface *display)
{
	SDL_Overlay *overlay;
	struct private_yuvhwdata *hwdata;
	int xv_port;
	int i, j;
	int adaptors;
	XvAdaptorInfo *ainfo;
	XShmSegmentInfo *yuvshm;

	xv_port = -1;
	if ( (Success == XvQueryExtension(GFX_Display, &j, &j, &j, &j, &j)) &&
	     (Success == XvQueryAdaptors(GFX_Display,
	                                 RootWindow(GFX_Display, SDL_Screen),
	                                 &adaptors, &ainfo)) ) {
		for ( i=0; (i<adaptors) && (xv_port == -1); ++i ) {
			if ( (ainfo[i].type & XvInputMask) &&
			     (ainfo[i].type & XvImageMask) ) {
				int num_formats;
				XvImageFormatValues *formats;
				formats = XvListImageFormats(GFX_Display,
				              ainfo[i].base_id, &num_formats);
				for ( j=0; j<num_formats; ++j ) {
					if ( (Uint32)formats[j].id == format ) {
						xv_port = ainfo[i].base_id;
						break;
					}
				}
			}
		}
	}
	if ( xv_port == -1 ) {
		SDL_SetError("No available video ports for requested format");
		return(NULL);
	}

	/* Create the overlay structure */
	overlay = (SDL_Overlay *)malloc(sizeof *overlay);
	if ( overlay == NULL ) {
		SDL_OutOfMemory();
		return(NULL);
	}
	memset(overlay, 0, (sizeof *overlay));

	/* Fill in the basic members */
	overlay->format = format;
	overlay->w = width;
	overlay->h = height;

	/* Set up the YUV surface function structure */
	overlay->hwfuncs = &x11_yuvfuncs;

	/* Create the pixel data and lookup tables */
	hwdata = (struct private_yuvhwdata *)malloc(sizeof *hwdata);
	overlay->hwdata = hwdata;
	if ( hwdata == NULL ) {
		SDL_OutOfMemory();
		SDL_FreeYUVOverlay(overlay);
		return(NULL);
	}
	yuvshm = &hwdata->yuvshm;
	memset(yuvshm, 0, sizeof(*yuvshm));
	hwdata->port = xv_port;
	hwdata->image = XvShmCreateImage(GFX_Display, xv_port, format,
	                                 0, width, height, yuvshm);
	if ( hwdata->image == NULL ) {
		SDL_OutOfMemory();
		SDL_FreeYUVOverlay(overlay);
		return(NULL);
	}
	yuvshm->shmid = shmget(IPC_PRIVATE, hwdata->image->data_size,
	                       IPC_CREAT | 0777);
	if ( yuvshm->shmid < 0 ) {
		SDL_SetError("Unable to get %d bytes shared memory",
		             hwdata->image->data_size);
		SDL_FreeYUVOverlay(overlay);
		return(NULL);
	}
	yuvshm->shmaddr  = (caddr_t) shmat(yuvshm->shmid, 0, 0);
	yuvshm->readOnly = False;
	hwdata->image->data = yuvshm->shmaddr;

	XShmAttach(GFX_Display, yuvshm);
	XSync(GFX_Display, False);
	shmctl(yuvshm->shmid, IPC_RMID, 0);

	/* We're all done.. */
	return(overlay);
}

int X11_LockYUVOverlay(_THIS, SDL_Overlay *overlay)
{
	overlay->pixels = overlay->hwdata->image->data;
	/* What should the pitch be set to? */
	return(0);
}

void X11_UnlockYUVOverlay(_THIS, SDL_Overlay *overlay)
{
	overlay->pixels = NULL;
}

int X11_DisplayYUVOverlay(_THIS, SDL_Overlay *overlay, SDL_Rect *dstrect)
{
	struct private_yuvhwdata *hwdata;

	hwdata = overlay->hwdata;
	XvShmPutImage(GFX_Display, hwdata->port, SDL_Window, SDL_GC,
	              hwdata->image, 0, 0, overlay->w, overlay->h,
	              dstrect->x, dstrect->y, dstrect->w, dstrect->h, False);
	XSync(GFX_Display, False);
	return(0);
}

void X11_FreeYUVOverlay(_THIS, SDL_Overlay *overlay)
{
	struct private_yuvhwdata *hwdata;

	hwdata = overlay->hwdata;
	if ( hwdata ) {
		if ( hwdata->yuvshm.shmaddr ) {
			XShmDetach(GFX_Display, &hwdata->yuvshm);
			shmdt(hwdata->yuvshm.shmaddr);
		}
		if ( hwdata->image ) {
			XFree(hwdata->image);
		}
		free(hwdata);
	}
}

#endif /* XFREE86_XV */
