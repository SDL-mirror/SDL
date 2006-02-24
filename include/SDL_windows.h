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

#ifndef _SDL_windows_h
#define _SDL_windows_h

#include "SDL_stdinc.h"

/* This includes only the windows headers needed by SDL, with no C runtime */
#define WIN32_LEAN_AND_MEAN
#ifndef HAVE_LIBC
#ifdef _MSC_VER
#ifndef __FLTUSED__
#define __FLTUSED__
#ifdef __cplusplus
   extern "C"
#endif
	   __declspec(selectany) int _fltused=1;
#endif
#endif /* _MSC_VER */
#endif/* !HAVE_LIBC */
#include <windows.h>

#endif /* _SDL_windows_h */
