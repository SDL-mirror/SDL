/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

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

#include "../SDL_sysvideo.h"

#include <directfb.h>
#include <directfb_version.h>

#include "SDL_mouse.h"


/* Set below to 1 to compile with (old) multi mice/keyboard api. Code left in
 * in case we see this again ... 
 */

#define USE_MULTI_API	(0)

#define DEBUG 1
#define LOG_CHANNEL 	stdout

#define DFB_VERSIONNUM(X, Y, Z)						\
	((X)*1000 + (Y)*100 + (Z))

#define DFB_COMPILEDVERSION \
	DFB_VERSIONNUM(DIRECTFB_MAJOR_VERSION, DIRECTFB_MINOR_VERSION, DIRECTFB_MICRO_VERSION)

#define DFB_VERSION_ATLEAST(X, Y, Z) \
	(DFB_COMPILEDVERSION >= DFB_VERSIONNUM(X, Y, Z))

#if (DFB_VERSION_ATLEAST(1,0,0))
#define SDL_DIRECTFB_OPENGL 1
#include <directfbgl.h>
#else
#error "SDL_DIRECTFB: Please compile against libdirectfb version >= 1.0.0"
#endif

#if SDL_DIRECTFB_OPENGL
#include "SDL_loadso.h"
#endif

#include "SDL_DirectFB_events.h"
/*
 * #include "SDL_DirectFB_gamma.h"
 * #include "SDL_DirectFB_keyboard.h"
 */
#include "SDL_DirectFB_modes.h"
#include "SDL_DirectFB_mouse.h"
#include "SDL_DirectFB_opengl.h"
#include "SDL_DirectFB_window.h"
#include "SDL_DirectFB_WM.h"

#define DFBENV_USE_YUV_UNDERLAY 	"SDL_DIRECTFB_YUV_UNDERLAY"     /* Default: off */
#define DFBENV_USE_YUV_DIRECT   	"SDL_DIRECTFB_YUV_DIRECT"       /* Default: off */
#define DFBENV_USE_X11_CHECK		"SDL_DIRECTFB_X11_CHECK"        /* Default: on  */
#define DFBENV_USE_LINUX_INPUT		"SDL_DIRECTFB_LINUX_INPUT"      /* Default: on  */
#define DFBENV_USE_WM				"SDL_DIRECTFB_WM"       /* Default: off  */

#define SDL_DFB_RELEASE(x) do { if ( (x) != NULL ) { SDL_DFB_CHECK(x->Release(x)); x = NULL; } } while (0)
#define SDL_DFB_FREE(x) do { if ( (x) != NULL ) { SDL_free(x); x = NULL; } } while (0)
#define SDL_DFB_UNLOCK(x) do { if ( (x) != NULL ) { x->Unlock(x); } } while (0)

#if DEBUG
/* FIXME: do something with DEBUG */
#endif

#define SDL_DFB_CONTEXT "SDL_DirectFB"

static inline DFBResult sdl_dfb_check(DFBResult ret, const char *src_file, int src_line, const char *src_code) {
	if (ret != DFB_OK) {
		fprintf(LOG_CHANNEL, "%s <%d>:\n\t", src_file, src_line );
		fprintf(LOG_CHANNEL, "\t%s\n", src_code );
		fprintf(LOG_CHANNEL, "\t%s\n", DirectFBErrorString (ret) );
		SDL_SetError( src_code, DirectFBErrorString (ret) );
	}
	return ret;
}

#define SDL_DFB_CHECK(x...) sdl_dfb_check( x, __FILE__, __LINE__, #x )

#define SDL_DFB_CHECKERR(x...) if ( sdl_dfb_check( x, __FILE__, __LINE__, #x ) != DFB_OK ) goto error

#define SDL_DFB_DEBUG(x...) 							\
	do {											\
		fprintf(LOG_CHANNEL, "%s: %s <%d>:\n\t",			\
			SDL_DFB_CONTEXT, __FILE__, __LINE__ );	\
        fprintf(LOG_CHANNEL, x ); \
	} while (0)

#define SDL_DFB_ERR(x...) SDL_DFB_DEBUG( x )

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

#define SDL_DFB_DEVICEDATA(dev)  DFB_DeviceData *devdata = (dev ? (DFB_DeviceData *) ((dev)->driverdata) : NULL)

#define DFB_MAX_SCREENS 10

typedef struct _DFB_DeviceData DFB_DeviceData;
struct _DFB_DeviceData
{
    int initialized;

    IDirectFB *dfb;
    int num_mice;
    int mouse_id[0x100];
    int num_keyboard;
    struct
    {
        int is_generic;
        int id;
    } keyboard[10];
    DFB_WindowData *firstwin;

    int use_yuv_underlays;
    int use_linux_input;
    int has_own_wm;

	/* window grab */
	SDL_Window *grabbed_window;

    /* global events */
    IDirectFBEventBuffer *events;
};

#endif /* _SDL_DirectFB_video_h */
