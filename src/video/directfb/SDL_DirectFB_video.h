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
#include "SDL_config.h"

#ifndef _SDL_DirectFB_video_h
#define _SDL_DirectFB_video_h

#include <directfb.h>
#include <directfb_version.h>

#define LOG_CHANNEL 	stdout

#if (DIRECTFB_MAJOR_VERSION == 0) && (DIRECTFB_MINOR_VERSION == 9) && (DIRECTFB_MICRO_VERSION < 23)
#error "SDL_DIRECTFB: Please compile against libdirectfb version >=0.9.24"
#endif

#if (DIRECTFB_MAJOR_VERSION >= 1) && (DIRECTFB_MINOR_VERSION >= 0) && (DIRECTFB_MICRO_VERSION >= 0 )
#define SDL_DIRECTFB_OPENGL 1
#include <directfbgl.h>
#endif

#if SDL_DIRECTFB_OPENGL
#include "SDL_loadso.h"
#endif

#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"

#define DEBUG 1

#define SDL_DFB_RELEASE(x) do { if ( x ) { x->Release(x); x = NULL; } } while (0)
#define SDL_DFB_FREE(x) do { if ( x ) { SDL_free(x); x = NULL; } } while (0)
#define SDL_DFB_UNLOCK(x) do { if ( x ) { x->Unlock(x); } } while (0)

#if DEBUG
#define SDL_DFB_DEBUG(x...) do { fprintf(LOG_CHANNEL, "%s:", __FUNCTION__); fprintf(LOG_CHANNEL, x); } while (0)
#define SDL_DFB_DEBUGC(x...) do { fprintf(LOG_CHANNEL, x); } while (0)
#else
#define SDL_DFB_DEBUG(x...) do { } while (0)
#define SDL_DFB_DEBUGC(x...) do { } while (0)
#endif

#define SDL_DFB_CONTEXT "SDL_DirectFB"

#define SDL_DFB_ERR(x...) 							\
	do {											\
		fprintf(LOG_CHANNEL, "%s: %s <%d>:\n\t",			\
			SDL_DFB_CONTEXT, __FILE__, __LINE__ );	\
		fprintf(LOG_CHANNEL, x );						\
	} while (0)

#define SDL_DFB_CHECK(x...) \
     do {                                                                \
          ret = x;                                                    \
          if (ret != DFB_OK) {                                        \
               fprintf(LOG_CHANNEL, "%s <%d>:\n\t", __FILE__, __LINE__ ); 	      \
               SDL_SetError( #x, DirectFBErrorString (ret) );         \
          }                                                           \
     } while (0)

#define SDL_DFB_CHECKERR(x...) \
     do {                                                                \
          ret = x;                                                    \
          if (ret != DFB_OK) {                                        \
               fprintf(LOG_CHANNEL, "%s <%d>:\n", __FILE__, __LINE__ ); \
               fprintf(LOG_CHANNEL, "\t%s\n", #x ); \
               fprintf(LOG_CHANNEL, "\t%s\n", DirectFBErrorString (ret) ); \
               SDL_SetError( #x, DirectFBErrorString (ret) );         \
               goto error; 					      \
          }                                                           \
     } while (0)

#define SDL_DFB_CALLOC(r, n, s) \
     do {                                                                \
          r = SDL_calloc (n, s);                                      \
          if (!(r)) {                                                 \
               fprintf( LOG_CHANNEL, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
               SDL_OutOfMemory();                                     \
               goto error; 					      \
          }                                                           \
     } while (0)

/* Private display data */

#define SDL_DFB_DEVICEDATA(dev)  DFB_DeviceData *devdata = (DFB_DeviceData *) ((dev)->driverdata)
#define SDL_DFB_WINDOWDATA(win)  DFB_WindowData *windata = ((win) ? (DFB_WindowData *) ((win)->driverdata) : NULL)
#define SDL_DFB_DISPLAYDATA(dev, win)  DFB_DisplayData *dispdata = ((win && dev) ? (DFB_DisplayData *) (dev)->displays[(win)->display].driverdata : NULL)

typedef struct _DFB_DisplayData DFB_DisplayData;

#define DFB_MAX_SCREENS 10
#define DFB_MAX_MODES 50

struct _DFB_DisplayData
{
    IDirectFBDisplayLayer *layer;
    DFBSurfacePixelFormat pixelformat;
    //FIXME: support for multiple layer ...
    DFBDisplayLayerID vidID;

    int vidIDinuse;

    int cw;
    int ch;

    int nummodes;
    SDL_DisplayMode *modelist;

#if 0
    WMcursor *last_cursor;
    WMcursor *blank_cursor;
    WMcursor *default_cursor;
#endif
};


typedef struct _DFB_WindowData DFB_WindowData;
struct _DFB_WindowData
{
    IDirectFBSurface *surface;
    IDirectFBPalette *palette;
    IDirectFBWindow *window;
    IDirectFBGL *gl_context;
    IDirectFBEventBuffer *eventbuffer;
    DFBWindowID windowID;
    int id;                     /* SDL window id */
    DFB_WindowData *next;
    Uint8 opacity;
};

typedef struct _DFB_DeviceData DFB_DeviceData;
struct _DFB_DeviceData
{
    int initialized;

    IDirectFB *dfb;
    int mouse;
    int keyboard;
    int kbdgeneric;
    DFB_WindowData *firstwin;

    int numscreens;
    DFBScreenID screenid[DFB_MAX_SCREENS];
    DFBDisplayLayerID gralayer[DFB_MAX_SCREENS];
    DFBDisplayLayerID vidlayer[DFB_MAX_SCREENS];

    int aux;                    /* auxiliary integer for callbacks */

    /* OpenGL */
    void (*glFinish) (void);
    void (*glFlush) (void);
};

struct SDL_GLDriverData
{
    int gl_active;              /* to stop switching drivers while we have a valid context */

#if SDL_DIRECTFB_OPENGL
    IDirectFBGL *gl_context;
#endif                          /* SDL_DIRECTFB_OPENGL */
};

#endif /* _SDL_DirectFB_video_h */
