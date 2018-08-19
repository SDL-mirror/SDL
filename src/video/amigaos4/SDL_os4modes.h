/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#ifndef _SDL_os4modes_h
#define _SDL_os4modes_h

typedef struct
{
    struct Screen *			screen;
} SDL_DisplayData;

typedef struct
{
    ULONG					modeid;
    LONG					x;
    LONG					y;
} SDL_DisplayModeData;

extern int OS4_InitModes(_THIS);
extern int OS4_GetDisplayBounds(_THIS, SDL_VideoDisplay * display, SDL_Rect * rect);
extern void OS4_GetDisplayModes(_THIS, SDL_VideoDisplay * display);
extern int OS4_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
extern void OS4_QuitModes(_THIS);

extern void OS4_CloseScreen(_THIS, struct Screen *screen);

#endif /* _SDL_os4modes_h */

/* vi: set ts=4 sw=4 expandtab: */
