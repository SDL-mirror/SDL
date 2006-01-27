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

#ifndef _SDL_getenv_h
#define _SDL_getenv_h

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Not all environments have a working getenv()/putenv() */

#if defined(macintosh) || defined(WIN32) || defined(_WIN32_WCE)
#define NEED_SDL_GETENV
#endif

#ifdef NEED_SDL_GETENV

/* Put a variable of the form "name=value" into the environment */
extern DECLSPEC int SDLCALL SDL_putenv(const char *variable);
#define putenv(X)   SDL_putenv(X)

/* Retrieve a variable named "name" from the environment */
extern DECLSPEC char * SDLCALL SDL_getenv(const char *name);
#define getenv(X)     SDL_getenv(X)

#endif /* NEED_GETENV */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_getenv_h */
