/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000  Sam Lantinga

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

#include "SDL_gsvideo.h"

/* Variables and functions exported by SDL_sysevents.c to other parts 
   of the native video subsystem (SDL_sysvideo.c)
*/
extern int GS_OpenKeyboard(_THIS);
extern void GS_CloseKeyboard(_THIS);
extern int GS_OpenMouse(_THIS);
extern void GS_CloseMouse(_THIS);
extern int GS_EnterGraphicsMode(_THIS);
extern int GS_InGraphicsMode(_THIS);
extern void GS_LeaveGraphicsMode(_THIS);

extern void GS_InitOSKeymap(_THIS);
extern void GS_PumpEvents(_THIS);
