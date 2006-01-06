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

/*
 * Mac OS X >= 10.3 are guaranteed to have dlopen support in a system
 *  framework, which means we don't have to roll our own on non-PowerPC
 *  systems to guarantee compatibility (x86 Macs started at 10.4).   --ryan.
 */
#if ( (defined(MACOSX)) && (!defined(__POWERPC__)) )
#  define USE_DLOPEN 1
#endif

/* !!! FIXME: includes so I don't have to update all the project files... */
#define SDL_INTERNAL_BUILDING_LOADSO 1
#if defined(USE_DUMMY_LOADSO)
# include "loadso/dummy/SDL_loadso.c"
#elif defined(USE_DLOPEN)
# include "loadso/dlopen/SDL_loadso.c"
#elif defined(MACOSX)
# include "loadso/macosx/SDL_loadso.c"
#elif defined(macintosh)
# include "loadso/macos/SDL_loadso.c"
#elif defined(WIN32) || defined(_WIN32_WCE)
# include "loadso/windows/SDL_loadso.c"
#elif defined(__BEOS__)
# include "loadso/beos/SDL_loadso.c"
#elif defined(__MINT__) && defined(ENABLE_LDG)
# include "loadso/mint/SDL_loadso.c"
#elif defined(__OS2__)
# include "loadso/os2/SDL_loadso.c"
#else
# include "loadso/dummy/SDL_loadso.c"
#endif /* system type */

