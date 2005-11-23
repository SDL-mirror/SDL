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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */

#if !SDL_INTERNAL_BUILDING_LOADSO
#error Do not compile directly...compile src/SDL_loadso.c instead!
#endif

#if !defined(__OS2__)
#error Compiling for the wrong platform?
#endif

#include <stdio.h>
#define INCL_DOSERRORS
#define INCL_DOSMODULEMGR
#include <os2.h>

#include "SDL_types.h"
#include "SDL_error.h"
#include "SDL_loadso.h"

void *SDL_LoadObject(const char *sofile)
{
    HMODULE handle = NULL;
    char buf[512];
    APIRET ulrc = DosLoadModule(buf, sizeof (buf), (char *) sofile, &handle);

    /* Generate an error message if all loads failed */
    if ((ulrc != NO_ERROR) || (handle == NULL))
        SDL_SetError("Failed loading %s: %s", sofile, buf);

    return((void *) handle);
}

void *SDL_LoadFunction(void *handle, const char *name)
{
    const char *loaderror = "Unknown error";
    void *symbol = NULL;
    APIRET ulrc = DosQueryProcAddr((HMODULE)handle, 0, (char *)name, &symbol);
    if (ulrc == ERROR_INVALID_HANDLE)
        loaderror = "Invalid module handle";
    else if (ulrc == ERROR_INVALID_NAME)
        loaderror = "Symbol not found";

    if (symbol == NULL)
        SDL_SetError("Failed loading %s: %s", name, loaderror);

    return(symbol);
}

void SDL_UnloadObject(void *handle)
{
    if ( handle != NULL )
        DosFreeModule((HMODULE) handle);
}

