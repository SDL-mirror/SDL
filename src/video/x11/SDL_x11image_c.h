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

#include "SDL_x11video.h"

extern int X11_SetupImage(_THIS, SDL_Surface *screen);
extern void X11_DestroyImage(_THIS, SDL_Surface *screen);
extern int X11_ResizeImage(_THIS, SDL_Surface *screen, Uint32 flags);

extern int X11_AllocHWSurface(_THIS, SDL_Surface *surface);
extern void X11_FreeHWSurface(_THIS, SDL_Surface *surface);
extern int X11_LockHWSurface(_THIS, SDL_Surface *surface);
extern void X11_UnlockHWSurface(_THIS, SDL_Surface *surface);
extern int X11_FlipHWSurface(_THIS, SDL_Surface *surface);

extern void X11_DisableAutoRefresh(_THIS);
extern void X11_EnableAutoRefresh(_THIS);
extern void X11_RefreshDisplay(_THIS);
