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

#include "SDL_version.h"
#include "SDL_error.h"
#include "SDL_timer.h"
#include "SDL_video.h"
#include "SDL_syswm.h"
#include "SDL_events_c.h"
#include "SDL_pixels_c.h"
#include "SDL_cgxmodes_c.h"
#include "SDL_cgxwm_c.h"

/* This is necessary for working properly with Enlightenment, etc. */
#define USE_ICON_WINDOW

void CGX_SetIcon(_THIS, SDL_Surface *icon, Uint8 *mask)
{
#if 0
	SDL_Surface *sicon;
	XWMHints *wmhints;
	XImage *icon_image;
	Pixmap icon_pixmap;
	Pixmap mask_pixmap;
#ifdef USE_ICON_WINDOW
	Window icon_window;
#endif
	GC GC;
	XGCValues GCvalues;
	int i, b, dbpp;
	SDL_Rect bounds;
	Uint8 *LSBmask, *color_tried;
	Visual *dvis;

	/* Lock the event thread, in multi-threading environments */
	SDL_Lock_EventThread();

	/* The icon must use the default visual, depth and colormap of the
	   screen, so it might need a conversion */
	dbpp = DefaultDepth(SDL_Display, SDL_Screen);
	switch(dbpp) {
	case 15:
	    dbpp = 16; break;
	case 24:
	    dbpp = 32; break;
	}
	dvis = DefaultVisual(SDL_Display, SDL_Screen);

	/* The Visual struct is supposed to be opaque but we cheat a little */
	sicon = SDL_CreateRGBSurface(SDL_SWSURFACE, icon->w, icon->h,
				     dbpp,
				     dvis->red_mask, dvis->green_mask,
				     dvis->blue_mask, 0);

	if ( sicon == NULL ) {
		goto done;
	}
	/* If we already have allocated colours from the default colormap,
	   copy them */
	if(SDL_Visual == dvis && SDL_XColorMap == SDL_DisplayColormap
	   && this->screen->format->palette && sicon->format->palette) {
	    memcpy(sicon->format->palette->colors,
		   this->screen->format->palette->colors,
		   this->screen->format->palette->ncolors * sizeof(SDL_Color));
	}

	bounds.x = 0;
	bounds.y = 0;
	bounds.w = icon->w;
	bounds.h = icon->h;
	if ( SDL_LowerBlit(icon, &bounds, sicon, &bounds) < 0 )
		goto done;

	/* Lock down the colors used in the colormap */
	color_tried = NULL;
	if ( sicon->format->BitsPerPixel == 8 ) {
		SDL_Palette *palette;
		Uint8 *p;
		XColor wanted;

		palette = sicon->format->palette;
		color_tried = malloc(palette->ncolors);
		if ( color_tried == NULL ) {
			goto done;
		}
		if ( SDL_iconcolors != NULL ) {
			free(SDL_iconcolors);
		}
		SDL_iconcolors = malloc(palette->ncolors
					* sizeof(*SDL_iconcolors));
		if ( SDL_iconcolors == NULL ) {
			free(color_tried);
			goto done;
		}
		memset(color_tried, 0, palette->ncolors);
		memset(SDL_iconcolors, 0,
		       palette->ncolors * sizeof(*SDL_iconcolors));

		p = (Uint8 *)sicon->pixels; 
		for ( i = sicon->w*sicon->h; i > 0; --i, ++p ) {
			if ( ! color_tried[*p] ) {
				wanted.pixel = *p;
				wanted.red   = (palette->colors[*p].r<<8);
				wanted.green = (palette->colors[*p].g<<8);
				wanted.blue  = (palette->colors[*p].b<<8);
				wanted.flags = (DoRed|DoGreen|DoBlue);
				if (XAllocColor(SDL_Display,
						SDL_DisplayColormap, &wanted)) {
					++SDL_iconcolors[wanted.pixel];
				}
				color_tried[*p] = 1;
			}
		}
	}
	if ( color_tried != NULL ) {
		free(color_tried);
	}

	/* Translate mask data to LSB order and set the icon mask */
	i = (sicon->w/8)*sicon->h;
	LSBmask = (Uint8 *)malloc(i);
	if ( LSBmask == NULL ) {
		goto done;
	}
	memset(LSBmask, 0, i);
	while ( --i >= 0 ) {
		for ( b=0; b<8; ++b )
			LSBmask[i] |= (((mask[i]>>b)&0x01)<<(7-b));
	}
	mask_pixmap = XCreatePixmapFromBitmapData(SDL_Display, WMwindow,
					LSBmask, sicon->w, sicon->h, 1L, 0L, 1);

	/* Transfer the image to an X11 pixmap */
	icon_image = XCreateImage(SDL_Display,
			DefaultVisual(SDL_Display, SDL_Screen),
			DefaultDepth(SDL_Display, SDL_Screen),
			ZPixmap, 0, (char *)sicon->pixels, sicon->w, sicon->h,
			((sicon->format)->BytesPerPixel == 3) ? 32 :
				(sicon->format)->BytesPerPixel*8, 0);
	icon_pixmap = XCreatePixmap(SDL_Display, SDL_Root, sicon->w, sicon->h,
			DefaultDepth(SDL_Display, SDL_Screen));
	GC = XCreateGC(SDL_Display, icon_pixmap, 0, &GCvalues);
	XPutImage(SDL_Display, icon_pixmap, GC, icon_image,
					0, 0, 0, 0, sicon->w, sicon->h);
	XFreeGC(SDL_Display, GC);
	XDestroyImage(icon_image);
	free(LSBmask);
	sicon->pixels = NULL;

#ifdef USE_ICON_WINDOW
	/* Create an icon window and set the pixmap as its background */
	icon_window = XCreateSimpleWindow(SDL_Display, SDL_Root,
					0, 0, sicon->w, sicon->h, 0,
					CopyFromParent, CopyFromParent);
	XSetWindowBackgroundPixmap(SDL_Display, icon_window, icon_pixmap);
	XClearWindow(SDL_Display, icon_window);
#endif

	/* Set the window icon to the icon pixmap (and icon window) */
	wmhints = XAllocWMHints();
	wmhints->flags = (IconPixmapHint | IconMaskHint);
	wmhints->icon_pixmap = icon_pixmap;
	wmhints->icon_mask = mask_pixmap;
#ifdef USE_ICON_WINDOW
	wmhints->flags |= IconWindowHint;
	wmhints->icon_window = icon_window;
#endif
	XSetWMHints(SDL_Display, WMwindow, wmhints);
	XFree(wmhints);
	XSync(SDL_Display, False);

  done:
	SDL_Unlock_EventThread();
	if ( sicon != NULL ) {
		SDL_FreeSurface(sicon);
	}
#endif
	return;
}

void CGX_SetCaption(_THIS, const char *title, const char *icon)
{
	if(SDL_Window)
		SetWindowTitles(SDL_Window,(char *)title,NULL);
}

/* Iconify the window */
int CGX_IconifyWindow(_THIS)
{
#if 0
	int result;

	SDL_Lock_EventThread();
	result = XIconifyWindow(SDL_Display, WMwindow, SDL_Screen);
	XSync(SDL_Display, False);
	SDL_Unlock_EventThread();
	return(result);
#else
	return 0;
#endif
}

#if 0
SDL_GrabMode X11_GrabInputNoLock(_THIS, SDL_GrabMode mode)
{
	int numtries, result;

	if ( this->screen == NULL ) {
		return(SDL_GRAB_OFF);
	}
	if ( ! SDL_Window ) {
		return(mode);	/* Will be set later on mode switch */
	}
	if ( mode == SDL_GRAB_OFF ) {
		XUngrabPointer(SDL_Display, CurrentTime);
		if ( this->screen->flags & SDL_FULLSCREEN ) {
			/* Rebind the mouse to the fullscreen window */
			for ( numtries = 0; numtries < 10; ++numtries ) {
				result = XGrabPointer(SDL_Display, FSwindow,
						True, 0,
						GrabModeAsync, GrabModeAsync,
						FSwindow, None, CurrentTime);
				if ( result == AlreadyGrabbed ) {
					break;
				}
				SDL_Delay(100);
			}
		}
#ifdef GRAB_FULLSCREEN
		if ( !(this->screen->flags & SDL_FULLSCREEN) )
#endif
		XUngrabKeyboard(SDL_Display, CurrentTime);
	} else {
		if ( this->screen->flags & SDL_FULLSCREEN ) {
			/* Unbind the mouse from the fullscreen window */
			XUngrabPointer(SDL_Display, CurrentTime);
		}
		/* Try to grab the mouse */
		for ( numtries = 0; numtries < 10; ++numtries ) {
			result = XGrabPointer(SDL_Display, SDL_Window, True, 0,
						GrabModeAsync, GrabModeAsync,
						SDL_Window, None, CurrentTime);
			if ( result != AlreadyGrabbed ) {
				break;
			}
			SDL_Delay(100);
		}
#ifdef GRAB_FULLSCREEN
		if ( !(this->screen->flags & SDL_FULLSCREEN) )
#endif
		XGrabKeyboard(SDL_Display, WMwindow, True,
			GrabModeAsync, GrabModeAsync, CurrentTime);
	}
	XSync(SDL_Display, False);

	return(mode);
}

SDL_GrabMode X11_GrabInput(_THIS, SDL_GrabMode mode)
{
	SDL_Lock_EventThread();
	mode = X11_GrabInputNoLock(this, mode);
	SDL_Unlock_EventThread();

	return(mode);
}

/* If 'info' is the right version, this function fills it and returns 1.
   Otherwise, in case of a version mismatch, it returns -1.
*/
static void lock_display(void)
{
	SDL_Lock_EventThread();
}
static void unlock_display(void)
{
	/* Make sure any X11 transactions are completed */
	SDL_VideoDevice *this = current_video;
	XSync(SDL_Display, False);
	SDL_Unlock_EventThread();
}

#endif

int CGX_GetWMInfo(_THIS, SDL_SysWMinfo *info)
{
	if ( info->version.major <= SDL_MAJOR_VERSION ) {
#if 0
		info->subsystem = SDL_SYSWM_X11;
		info->info.x11.display = SDL_Display;
		info->info.x11.window = SDL_Window;
		if ( SDL_VERSIONNUM(info->version.major,
		                    info->version.minor,
		                    info->version.patch) >= 1002 ) {
			info->info.x11.fswindow = FSwindow;
			info->info.x11.wmwindow = WMwindow;
		}
		info->info.x11.lock_func = lock_display;
		info->info.x11.unlock_func = unlock_display;
#endif
		return(1);
	} else {
		SDL_SetError("Application not compiled with SDL %d.%d\n",
					SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
		return(-1);
	}
}
