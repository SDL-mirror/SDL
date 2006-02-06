/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

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

#ifndef _SDL_getenv_h
#define _SDL_getenv_h

#include "SDL_config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_GETENV
#define SDL_getenv	getenv
#else
#define getenv		SDL_getenv
extern DECLSPEC char * SDLCALL SDL_getenv(const char *name);
#endif

#ifdef HAVE_PUTENV
#define SDL_putenv	putenv
#else
#define putenv		SDL_putenv
extern DECLSPEC int SDLCALL SDL_putenv(const char *variable);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_getenv_h */
