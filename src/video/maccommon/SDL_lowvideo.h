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

#if TARGET_API_MAC_CARBON
#include <Carbon.h>
#else
#include <Quickdraw.h>
#include <Palettes.h>
#include <Menus.h>
#include <DrawSprocket.h>
#endif

#ifdef HAVE_OPENGL
#ifdef MACOSX
#include <OpenGL/gl.h> /* OpenGL.framework */
#include <AGL/agl.h>   /* AGL.framework */
#else
#include <GL/gl.h>
#include <agl.h>
#endif /* MACOSX */
#endif /* HAVE_OPENGL */

#include "SDL_video.h"
#include "SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

#if !TARGET_API_MAC_CARBON  /* not available in OS X (or more accurately, Carbon) */
/* Global QuickDraw data */
extern QDGlobals *theQD;
#endif

/* Private display data */
struct SDL_PrivateVideoData {
	GDevice **SDL_Display;
	WindowRef SDL_Window;
	SDL_Rect **SDL_modelist;
	CTabHandle SDL_CTab;
	PaletteHandle SDL_CPal;

#if TARGET_API_MAC_CARBON
	/* For entering and leaving fullscreen mode */
	Ptr fullscreen_ctx;
#endif

	/* The current window document style */
	int current_style;

	/* Information about the last cursor position */
	Point last_where;

	/* Information about the last keys down */
	EventModifiers last_mods;
	KeyMap last_keys;

	/* A handle to the Apple Menu */
	MenuRef apple_menu;

	/* Information used by DrawSprocket driver */
	struct DSpInfo *dspinfo;

#ifdef HAVE_OPENGL
	AGLContext appleGLContext;

	void *libraryHandle;
#endif
};
/* Old variable names */
#define SDL_Display		(this->hidden->SDL_Display)
#define SDL_Window		(this->hidden->SDL_Window)
#define SDL_modelist		(this->hidden->SDL_modelist)
#define SDL_CTab		(this->hidden->SDL_CTab)
#define SDL_CPal		(this->hidden->SDL_CPal)
#define fullscreen_ctx		(this->hidden->fullscreen_ctx)
#define current_style		(this->hidden->current_style)
#define last_where		(this->hidden->last_where)
#define last_mods		(this->hidden->last_mods)
#define last_keys		(this->hidden->last_keys)
#define apple_menu		(this->hidden->apple_menu)
#define glContext		(this->hidden->appleGLContext)

#endif /* _SDL_lowvideo_h */
