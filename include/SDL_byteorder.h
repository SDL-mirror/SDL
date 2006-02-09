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

/* Macros for determining the byte-order of this platform */

#ifndef _SDL_byteorder_h
#define _SDL_byteorder_h

#include "SDL_config.h"

/* The two types of endianness */
#define SDL_LIL_ENDIAN	1234
#define SDL_BIG_ENDIAN	4321

#ifndef SDL_BYTEORDER	/* Not defined in SDL_config.h? */
#if (defined(__i386__) || defined(__i386)) || \
     defined(__ia64__) || defined(__x86_64__) || \
    (defined(__alpha__) || defined(__alpha)) || \
    (defined(__arm__) || defined(__thumb__)) || \
    (defined(__sh__) || defined(__sh64__)) || \
    (defined(__mips__) && defined(__MIPSEL__)) || \
     defined(__SYMBIAN32__) || defined(__OS2__)
#define SDL_BYTEORDER	SDL_LIL_ENDIAN
#else
#define SDL_BYTEORDER	SDL_BIG_ENDIAN
#endif
#endif /* !SDL_BYTEORDER */

#endif /* _SDL_byteorder_h */
