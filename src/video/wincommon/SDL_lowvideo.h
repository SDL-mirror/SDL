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

#ifndef _SDL_lowvideo_h
#define _SDL_lowvideo_h

#include <windows.h>

#include "SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

#define WINDIB_FULLSCREEN()						\
(									\
	SDL_VideoSurface &&						\
	((SDL_VideoSurface->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN) && \
	(((SDL_VideoSurface->flags & SDL_OPENGL   ) == SDL_OPENGL    ) || \
	 (strcmp(this->name, "windib") == 0))				\
)
#define DDRAW_FULLSCREEN() 						\
(									\
	SDL_VideoSurface &&						\
	((SDL_VideoSurface->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN) && \
	((SDL_VideoSurface->flags & SDL_OPENGL    ) != SDL_OPENGL    ) && \
	(strcmp(this->name, "directx") == 0)				\
)

#define DINPUT_FULLSCREEN()	DDRAW_FULLSCREEN()

/* The main window -- and a function to set it for the audio */
#ifdef _WIN32_WCE
extern LPWSTR SDL_Appname;
#else
extern LPSTR SDL_Appname;
#endif
extern HINSTANCE SDL_Instance;
extern HWND SDL_Window;
extern const char *SDL_windowid;

/* Variables and functions exported to other parts of the native video
   subsystem (SDL_sysevents.c)
*/
/* Called by windows message loop when system palette is available */
extern void (*WIN_RealizePalette)(_THIS);

/* Called by windows message loop when the system palette changes */
extern void (*WIN_PaletteChanged)(_THIS, HWND window);

/* Called by windows message loop when a portion of the screen needs update */
extern void (*WIN_WinPAINT)(_THIS, HDC hdc);

/* Called by windows message loop when the message isn't handled */
extern LONG (*HandleMessage)(_THIS, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* The window cursor (from SDL_sysmouse.c) */
extern HCURSOR SDL_hcursor;

/* The bounds of the window in screen coordinates */
extern RECT SDL_bounds;

/* The position of the window in windowed mode */
extern int SDL_windowX;
extern int SDL_windowY;

/* Flag -- SDL is performing a resize, rather than the user */
extern int SDL_resizing;

/* Flag -- the mouse is in relative motion mode */
extern int mouse_relative;

/* The GDI fullscreen mode currently active */
#ifndef NO_CHANGEDISPLAYSETTINGS
extern DEVMODE SDL_fullscreen_mode;
#endif

/* The system gamma ramp for GDI modes */
extern WORD *gamma_saved;

/* This is really from SDL_dx5audio.c */
extern void DX5_SoundFocus(HWND window);

/* DJM: This is really from SDL_sysevents.c, we need it in
   GDL_CreateWindow as well */
LONG CALLBACK WinMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif /* SDL_lowvideo_h */
