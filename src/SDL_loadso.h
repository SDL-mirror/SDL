/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */
/* Note: This API isn't meant for public release until SDL 1.3         */

#ifndef _SDL_loadso_h
#define _SDL_loadso_h

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* This function dynamically loads a shared object and returns a pointer
 * to the object handle (or NULL if there was an error).
 * The 'sofile' parameter is a system dependent name of the object file.
 */
extern DECLSPEC void *SDL_LoadObject(const char *sofile);

/* Given an object handle, this function looks up the address of the
 * named function in the shared object and returns it.  This address
 * is no longer valid after calling SDL_UnloadObject().
 */
extern DECLSPEC void *SDL_LoadFunction(void *handle, const char *name);

/* Unload a shared object from memory */
extern DECLSPEC void SDL_UnloadObject(void *handle);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_loadso_h */
