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

#ifndef __SDL_QNXGF_H__
#define __SDL_QNXGF_H__

#include "../SDL_sysvideo.h"

#include <gf/gf.h>

typedef struct SDL_VideoData
{
   gf_dev_t      gfdev;              /* GF device handle                     */
   gf_dev_info_t gfdev_info;         /* GF device information                */
   SDL_bool      gfinitialized;      /* GF device initialization status      */
} SDL_VideoData;

typedef struct SDL_DisplayData
{
   gf_display_info_t display_info;   /* GF display information               */
   gf_display_t      display;        /* GF display handle                    */
} SDL_DisplayData;

/****************************************************************************/
/* SDL_VideoDevice functions declaration                                    */
/****************************************************************************/

/* Display and window functions */
int qnxgf_videoinit(_THIS);
void qnxgf_videoquit(_THIS);
void qnxgf_getdisplaymodes(_THIS);
int qnxgf_setdisplaymode(_THIS, SDL_DisplayMode* mode);
int qnxgf_setdisplaypalette(_THIS, SDL_Palette* palette);
int qnxgf_getdisplaypalette(_THIS, SDL_Palette* palette);
int qnxgf_setdisplaygammaramp(_THIS, Uint16* ramp);
int qnxgf_getdisplaygammaramp(_THIS, Uint16* ramp);
int qnxgf_createwindow(_THIS, SDL_Window* window);
int qnxgf_createwindowfrom(_THIS, SDL_Window* window, const void* data);
void qnxgf_setwindowtitle(_THIS, SDL_Window* window);
void qnxgf_setwindowicon(_THIS, SDL_Window* window, SDL_Surface* icon);
void qnxgf_setwindowposition(_THIS, SDL_Window* window);
void qnxgf_setwindowsize(_THIS, SDL_Window* window);
void qnxgf_showwindow(_THIS, SDL_Window* window);
void qnxgf_hidewindow(_THIS, SDL_Window* window);
void qnxgf_raisewindow(_THIS, SDL_Window* window);
void qnxgf_maximizewindow(_THIS, SDL_Window* window);
void qnxgf_minimizewindow(_THIS, SDL_Window* window);
void qnxgf_restorewindow(_THIS, SDL_Window* window);
void qnxgf_setwindowgrab(_THIS, SDL_Window* window);
void qnxgf_destroywindow(_THIS, SDL_Window* window);

/* Window manager function */
SDL_bool qnxgf_getwindowwminfo(_THIS, SDL_Window* window, struct SDL_SysWMinfo* info);

/* OpenGL/OpenGL ES functions */
int qnxgf_gl_loadlibrary(_THIS, const char* path);
void* qnxgf_gl_getprocaddres(_THIS, const char* proc);
void qnxgf_gl_unloadlibrary(_THIS);
SDL_GLContext qnxgf_gl_createcontext(_THIS, SDL_Window* window);
int qnxgf_gl_makecurrent(_THIS, SDL_Window* window, SDL_GLContext context);
int qnxgf_gl_setswapinterval(_THIS, int interval);
int qnxgf_gl_getswapinterval(_THIS);
void qnxgf_gl_swapwindow(_THIS, SDL_Window* window);
void qnxgf_gl_deletecontext(_THIS, SDL_GLContext context);

/* Event handling function */
void qnxgf_pumpevents(_THIS);

/* Screen saver related function */
void qnxgf_suspendscreensaver(_THIS);

#endif /* __SDL_QNXGF_H__ */
