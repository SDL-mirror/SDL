/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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
    slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#include <stdlib.h>

#include "SDL_error.h"
#include "SDL_endian.h"
#include "SDL_events_c.h"
#include "SDL_x11image_c.h"

#if defined(__USLC__)
#ifdef HAVE_KSTAT
#undef HAVE_KSTAT
#endif
#include <unistd.h>
#endif

#ifdef HAVE_KSTAT
#include <kstat.h>
#endif

#ifndef NO_SHARED_MEMORY

/* Shared memory information */
extern int XShmQueryExtension(Display *dpy);	/* Not in X11 headers */

/* Shared memory error handler routine */
static int shm_error;
static int (*X_handler)(Display *, XErrorEvent *) = NULL;
static int shm_errhandler(Display *d, XErrorEvent *e)
{
        if ( e->error_code == BadAccess ) {
        	++shm_error;
        	return(0);
        } else
		return(X_handler(d,e));
}
#endif /* ! NO_SHARED_MEMORY */

/* Various screen update functions available */
static void X11_NormalUpdate(_THIS, int numrects, SDL_Rect *rects);
static void X11_MITSHMUpdate(_THIS, int numrects, SDL_Rect *rects);

int X11_SetupImage(_THIS, SDL_Surface *screen)
{
#ifdef NO_SHARED_MEMORY
	screen->pixels = malloc(screen->h*screen->pitch);
#else
	/* Allocate shared memory if possible */
	if ( use_mitshm ) {
		shminfo.shmid = shmget(IPC_PRIVATE, screen->h*screen->pitch,
								IPC_CREAT|0777);
		if ( shminfo.shmid >= 0 ) {
			shminfo.shmaddr = (char *)shmat(shminfo.shmid, 0, 0);
			shminfo.readOnly = False;
			if ( shminfo.shmaddr != (char *)-1 ) {
				shm_error = False;
				X_handler = XSetErrorHandler(shm_errhandler);
				XShmAttach(SDL_Display, &shminfo);
				XSync(SDL_Display, True);
				XSetErrorHandler(X_handler);
				if ( shm_error == True )
					shmdt(shminfo.shmaddr);
			} else {
				shm_error = True;
			}
			shmctl(shminfo.shmid, IPC_RMID, NULL);
		} else {
			shm_error = True;
		}
		if ( shm_error == True )
			use_mitshm = 0;
	}
	if ( use_mitshm ) {
		screen->pixels = shminfo.shmaddr;
	} else {
		screen->pixels = malloc(screen->h*screen->pitch);
	}
#endif /* NO_SHARED_MEMORY */
	if ( screen->pixels == NULL ) {
		SDL_OutOfMemory();
		return(-1);
	}

#ifdef NO_SHARED_MEMORY
	{
 	        int bpp = screen->format->BytesPerPixel;
		SDL_Ximage = XCreateImage(SDL_Display, SDL_Visual,
					  this->hidden->depth, ZPixmap, 0,
					  (char *)screen->pixels, 
					  screen->w, screen->h,
					  (bpp == 3) ? 32 : bpp * 8,
					  0);
	}
#else
	if ( use_mitshm ) {
		SDL_Ximage = XShmCreateImage(SDL_Display, SDL_Visual,
					     this->hidden->depth, ZPixmap,
					     shminfo.shmaddr, &shminfo, 
					     screen->w, screen->h);
	} else {
 	        int bpp = screen->format->BytesPerPixel;
		SDL_Ximage = XCreateImage(SDL_Display, SDL_Visual,
					  this->hidden->depth, ZPixmap, 0,
					  (char *)screen->pixels, 
					  screen->w, screen->h,
					  (bpp == 3) ? 32 : bpp * 8,
					  0);
	}
#endif /* NO_SHARED_MEMORY */
	if ( SDL_Ximage == NULL ) {
		SDL_SetError("Couldn't create XImage");
#ifndef NO_SHARED_MEMORY
		if ( use_mitshm ) {
			XShmDetach(SDL_Display, &shminfo);
			XSync(SDL_Display, False);
			shmdt(shminfo.shmaddr);
			screen->pixels = NULL;
		}
#endif /* ! NO_SHARED_MEMORY */
		return(-1);
	}
	screen->pitch = SDL_Ximage->bytes_per_line;

	/* Determine what blit function to use */
#ifdef NO_SHARED_MEMORY
	this->UpdateRects = X11_NormalUpdate;
#else
	if ( use_mitshm ) {
		this->UpdateRects = X11_MITSHMUpdate;
	} else {
		this->UpdateRects = X11_NormalUpdate;
	}
#endif
	return(0);
}

void X11_DestroyImage(_THIS, SDL_Surface *screen)
{
	if ( SDL_Ximage ) {
		XDestroyImage(SDL_Ximage);
#ifndef NO_SHARED_MEMORY
		if ( use_mitshm ) {
			XShmDetach(SDL_Display, &shminfo);
			XSync(SDL_Display, False);
			shmdt(shminfo.shmaddr);
		}
#endif /* ! NO_SHARED_MEMORY */
		SDL_Ximage = NULL;
	}
	if ( screen ) {
		screen->pixels = NULL;
	}
}

/* This is a hack to see whether this system has more than 1 CPU */
static int num_CPU(void)
{
       static int num_cpus = 0;

       if(!num_cpus) {
#ifdef linux
           char line[BUFSIZ];
           FILE *pstat = fopen("/proc/stat", "r");
           if ( pstat ) {
               while ( fgets(line, sizeof(line), pstat) ) {
                   if (memcmp(line, "cpu", 3) == 0 && line[3] != ' ') {
                       ++num_cpus;
                   }
               }
               fclose(pstat);
           }
#elif defined(HAVE_KSTAT)
           kstat_ctl_t *kc = kstat_open();
           kstat_t *ks;
           kstat_named_t *kn;
           if(kc) {
               if((ks = kstat_lookup(kc, "unix", -1, "system_misc"))
                  && kstat_read(kc, ks, NULL) != -1
                  && (kn = kstat_data_lookup(ks, "ncpus")))
#ifdef KSTAT_DATA_UINT32
                   num_cpus = kn->value.ui32;
#else
                   num_cpus = kn->value.ul; /* needed in solaris <2.6 */
#endif
               kstat_close(kc);
           }
#elif defined(__USLC__)
           num_cpus = (int)sysconf(_SC_NPROCESSORS_CONF);
#endif
           if ( num_cpus <= 0 ) {
               num_cpus = 1;
           }
       }
       return num_cpus;
}

int X11_ResizeImage(_THIS, SDL_Surface *screen, Uint32 flags)
{
	int retval;

	X11_DestroyImage(this, screen);
        if ( flags & SDL_OPENGL ) {  /* No image when using GL */
        	retval = 0;
        } else {
		retval = X11_SetupImage(this, screen);
		/* We support asynchronous blitting on the display */
		if ( flags & SDL_ASYNCBLIT ) {
			/* This is actually slower on single-CPU systems,
			   probably because of CPU contention between the
			   X server and the application.
			   Note: Is this still true with XFree86 4.0?
			*/
			if ( num_CPU() > 1 ) {
				screen->flags |= SDL_ASYNCBLIT;
			}
		}
	}
	return(retval);
}

/* We don't actually allow hardware surfaces other than the main one */
int X11_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return(-1);
}
void X11_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

int X11_LockHWSurface(_THIS, SDL_Surface *surface)
{
	if ( (surface == SDL_VideoSurface) && blit_queued ) {
		XSync(GFX_Display, False);
		blit_queued = 0;
	}
	return(0);
}
void X11_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

int X11_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

/* Byte-swap the pixels in the display image */
static void X11_SwapAllPixels(SDL_Surface *screen)
{
	int x, y;

	switch (screen->format->BytesPerPixel) {
	    case 2: {
		Uint16 *spot;
		for ( y=0; y<screen->h; ++y ) {
			spot = (Uint16 *) ((Uint8 *)screen->pixels +
						y * screen->pitch);
			for ( x=0; x<screen->w; ++x, ++spot ) {
				*spot = SDL_Swap16(*spot);
			}
		}
	    }
	    break;

	    case 4: {
		Uint32 *spot;
		for ( y=0; y<screen->h; ++y ) {
			spot = (Uint32 *) ((Uint8 *)screen->pixels +
						y * screen->pitch);
			for ( x=0; x<screen->w; ++x, ++spot ) {
				*spot = SDL_Swap32(*spot);
			}
		}
	    }
	    break;

	    default:
		/* should never get here */
		break;
	}
}
static void X11_SwapPixels(SDL_Surface *screen, SDL_Rect *rect)
{
	int x, minx, maxx;
	int y, miny, maxy;

	switch (screen->format->BytesPerPixel) {
	    case 2: {
		Uint16 *spot;
		minx = rect->x;
		maxx = rect->x + rect->w;
		miny = rect->y;
		maxy = rect->y + rect->h;
		for ( y=miny; y<maxy; ++y ) {
		    spot = (Uint16 *) ((Uint8 *)screen->pixels +
				       y * screen->pitch + minx * 2);
		    for ( x=minx; x<maxx; ++x, ++spot ) {
			*spot = SDL_Swap16(*spot);
		    }
		}
	    }
	    break;

	    case 4: {
		Uint32 *spot;
		minx = rect->x;
		maxx = rect->x + rect->w;
		miny = rect->y;
		maxy = rect->y + rect->h;
		for ( y=miny; y<maxy; ++y ) {
		    spot = (Uint32 *) ((Uint8 *)screen->pixels +
				       y * screen->pitch + minx * 4);
		    for ( x=minx; x<maxx; ++x, ++spot ) {
			*spot = SDL_Swap32(*spot);
		    }
		}
	    }
	    break;

	    default:
		/* should never get here */
		break;
	}
}

static void X11_NormalUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	int i;

	/* Check for endian-swapped X server, swap if necessary (VERY slow!) */
	if ( swap_pixels &&
	     ((this->screen->format->BytesPerPixel%2) == 0) ) {
		for ( i=0; i<numrects; ++i ) {
			if ( ! rects[i].w ) { /* Clipped? */
				continue;
			}
		        X11_SwapPixels(this->screen, rects + i);
			XPutImage(GFX_Display, SDL_Window, SDL_GC, SDL_Ximage,
				rects[i].x, rects[i].y,
				rects[i].x, rects[i].y, rects[i].w, rects[i].h);
			X11_SwapPixels(this->screen, rects + i);
		}
	} else {
		for ( i=0; i<numrects; ++i ) {
			if ( ! rects[i].w ) { /* Clipped? */
				continue;
			}
			XPutImage(GFX_Display, SDL_Window, SDL_GC, SDL_Ximage,
				rects[i].x, rects[i].y,
				rects[i].x, rects[i].y, rects[i].w, rects[i].h);
		}
	}
	if ( SDL_VideoSurface->flags & SDL_ASYNCBLIT ) {
		XFlush(GFX_Display);
		++blit_queued;
	} else {
		XSync(GFX_Display, False);
	}
}

static void X11_MITSHMUpdate(_THIS, int numrects, SDL_Rect *rects)
{
#ifndef NO_SHARED_MEMORY
	int i;

	for ( i=0; i<numrects; ++i ) {
		if ( ! rects[i].w ) { /* Clipped? */
			continue;
		}
		XShmPutImage(GFX_Display, SDL_Window, SDL_GC, SDL_Ximage,
				rects[i].x, rects[i].y,
				rects[i].x, rects[i].y, rects[i].w, rects[i].h,
									False);
	}
	if ( SDL_VideoSurface->flags & SDL_ASYNCBLIT ) {
		XFlush(GFX_Display);
		++blit_queued;
	} else {
		XSync(GFX_Display, False);
	}
#endif /* ! NO_SHARED_MEMORY */
}

/* There's a problem with the automatic refreshing of the display.
   Even though the XVideo code uses the GFX_Display to update the
   video memory, it appears that updating the window asynchronously
   from a different thread will cause "blackouts" of the window.
   This is a sort of a hacked workaround for the problem.
*/
static int enable_autorefresh = 1;

void X11_DisableAutoRefresh(_THIS)
{
	--enable_autorefresh;
}

void X11_EnableAutoRefresh(_THIS)
{
	++enable_autorefresh;
}

void X11_RefreshDisplay(_THIS)
{
	/* Don't refresh a display that doesn't have an image (like GL)
	   Instead, post an expose event so the application can refresh.
	 */
	if ( ! SDL_Ximage || (enable_autorefresh <= 0) ) {
		SDL_PrivateExpose();
		return;
	}
#ifndef NO_SHARED_MEMORY
	if ( this->UpdateRects == X11_MITSHMUpdate ) {
		XShmPutImage(SDL_Display, SDL_Window, SDL_GC, SDL_Ximage,
				0, 0, 0, 0, this->screen->w, this->screen->h,
				False);
	} else {
#else
	{
#endif /* ! NO_SHARED_MEMORY */
		/* Check for endian-swapped X server, swap if necessary */
		if ( swap_pixels &&
		     ((this->screen->format->BytesPerPixel%2) == 0) ) {
			X11_SwapAllPixels(this->screen);
			XPutImage(SDL_Display, SDL_Window, SDL_GC, SDL_Ximage,
				0, 0, 0, 0, this->screen->w, this->screen->h);
			X11_SwapAllPixels(this->screen);
		} else {
			XPutImage(SDL_Display, SDL_Window, SDL_GC, SDL_Ximage,
				0, 0, 0, 0, this->screen->w, this->screen->h);
		}
	}
	XSync(SDL_Display, False);
}
