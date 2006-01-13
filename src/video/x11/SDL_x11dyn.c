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

#if 0
#define DEBUG_DYNAMIC_X11 1
#endif

#define __SDL_NO_REDEFINE_X11_HEADER_SYMS 1
#include "SDL_x11dyn.h"

#ifdef DEBUG_DYNAMIC_X11
#include <stdio.h>
#endif

#ifdef X11_DYNAMIC
#include <dlfcn.h>
#include "SDL_name.h"
#include "SDL_loadso.h"
static const char *x11_library = X11_DYNAMIC;
static void *x11_handle = NULL;
static const char *x11ext_library = X11EXT_DYNAMIC;
static void *x11ext_handle = NULL;

static void *X11_GetSym(int required, const char *fnname, int *rc)
{
	void *fn = NULL;
	if (*rc) {  /* haven't already failed on a previous lookup? */
		fn = SDL_LoadFunction(x11_handle, fnname);
		#if DEBUG_DYNAMIC_X11
		if (fn != NULL)
			printf("X11: Found '%s' in libX11 (%p)\n", fnname, fn);
		#endif

		if (fn == NULL) {  /* not found? Check libX11ext ... */
			fn = SDL_LoadFunction(x11ext_handle, fnname);
			#if DEBUG_DYNAMIC_X11
			if (fn != NULL)
				printf("X11: Found '%s' in libXext (%p)\n", fnname, fn);
			else
				printf("X11: Symbol '%s' NOT FOUND!%s\n", fnname,
				       required ? "" : " (...but not required!)");
			#endif
		}
		*rc = ((fn != NULL) || (!required));
	}

	return fn;
}
#endif  /* defined X11_DYNAMIC */

/* Define all the function pointers... */
#define SDL_X11_SYM(req,ret,fn,params) ret (*p##fn) params = NULL;
#include "SDL_x11sym.h"
#undef SDL_X11_SYM

static int x11_load_refcount = 0;

void SDL_X11_UnloadSymbols(void)
{
	/* Don't actually unload if more than one module is using the libs... */
	if (x11_load_refcount > 0) {
		if (--x11_load_refcount == 0) {
			/* set all the function pointers to NULL. */
			#define SDL_X11_SYM(req,ret,fn,params) p##fn = NULL;
			#include "SDL_x11sym.h"
			#undef SDL_X11_SYM

			#ifdef X11_DYNAMIC
			if (x11_handle != NULL) {
				SDL_UnloadObject(x11_handle);
				x11_handle = NULL;
			}
			if (x11ext_handle != NULL) {
				SDL_UnloadObject(x11ext_handle);
				x11ext_handle = NULL;
			}
			#endif
		}
	}
}

/* returns non-zero if all needed symbols were loaded. */
int SDL_X11_LoadSymbols(void)
{
	int rc = 1;

    /* deal with multiple modules (dga, x11, etc) needing these symbols... */
	if (x11_load_refcount++ == 0) {
		#ifdef X11_DYNAMIC
			x11_handle = SDL_LoadObject(x11_library);
			x11ext_handle = SDL_LoadObject(x11ext_library);
			rc = ((x11_handle != NULL) && (x11ext_handle != NULL));
			#define SDL_X11_SYM(req,r,fn,arg) p##fn = X11_GetSym(req,#fn, &rc);
			#include "SDL_x11sym.h"
			#undef SDL_X11_SYM

			if (!rc)
				SDL_X11_UnloadSymbols();  /* in case one of these loaded... */

		#else
			#define SDL_X11_SYM(req,r,fn,arg) p##fn = fn;
			#include "SDL_x11sym.h"
			#undef SDL_X11_SYM
		#endif
	}

	return rc;
}

/* end of SDL_x11dyn.c ... */

