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

#ifndef _SDL_x11dyn_h
#define _SDL_x11dyn_h

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xlibint.h>
#include <X11/extensions/extutil.h>

#ifndef NO_SHARED_MEMORY
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

/*
 * Never reference Xlib directly...we might load it dynamically at runtime.
 *  Even if we don't, for readability, we still use the function pointers
 *  (although the symbol resolution will be done by the loader in that case).
 *
 * We define SDL_X11_SYM and include SDL_x11sym.h to accomplish various
 *  goals, without having to duplicate those function signatures.
 */
#ifdef __cplusplus
extern "C" {
#endif

/* evil function signatures... */
typedef Bool (*SDL_X11_XESetWireToEventRetType)(Display*,XEvent*,xEvent*);
typedef int (*SDL_X11_XSynchronizeRetType)(Display*);
typedef Status (*SDL_X11_XESetEventToWireRetType)(Display*,XEvent*,xEvent*);

#define SDL_X11_SYM(ret,fn,params) extern ret (*p##fn) params;
#include "SDL_x11sym.h"
#undef SDL_X11_SYM

/* Macro in the xlib headers, not an actual symbol... */
#define pXDestroyImage XDestroyImage

int SDL_X11_LoadSymbols(void);
void SDL_X11_UnloadSymbols(void);

#ifdef __cplusplus
}
#endif

#endif  /* !defined _SDL_x11dyn_h */

