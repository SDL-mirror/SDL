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

#ifndef _SDL_config_h
#define _SDL_config_h

/* This is a set of defines to configure the SDL features */

#define HAVE_STDARG_H

/* Comment this if you want to build without any libc requirements */
#define HAVE_LIBC
#ifdef HAVE_LIBC

/* Various C library headers */
#ifndef HAVE_CTYPE_H
#define HAVE_CTYPE_H
#endif
#ifndef HAVE_STDIO_H
#define HAVE_STDIO_H
#endif
#ifndef HAVE_STDLIB_H
#define HAVE_STDLIB_H
#endif
#ifndef HAVE_MALLOC_H
#define HAVE_MALLOC_H
#endif
#ifndef HAVE_STRING_H
#define HAVE_STRING_H
#endif
#if !defined(_WIN32_WCE)
#ifndef HAVE_SIGNAL_H
#define HAVE_SIGNAL_H
#endif
#endif /* !_WIN32_WCE */

/* Features provided by SDL_stdlib.h */
#if !defined(_WIN32) /* Don't use C runtime versions of these on Windows */
#define HAVE_GETENV
#define HAVE_PUTENV
#endif
#define HAVE_MALLOC
#define HAVE_REALLOC
#define HAVE_FREE
#ifndef HAVE_ALLOCA
#define HAVE_ALLOCA
#endif
/*#define HAVE_QSORT*/

/* Features provided by SDL_string.h */
#define HAVE_MEMSET
#define HAVE_MEMCPY
#define HAVE_MEMMOVE
#define HAVE_MEMCMP
#define HAVE_STRLEN
#define HAVE_STRCPY
#define HAVE_STRNCPY
/*#define HAVE__STRREV*/
/*#define HAVE__STRUPR*/
/*#define HAVE__STRLWR*/
#define HAVE_STRCHR
#define HAVE_STRRCHR
#define HAVE_STRSTR
/*#define HAVE_ITOA*/
/*#define HAVE__LTOA*/
/*#define HAVE__UITOA*/
/*#define HAVE__ULTOA*/
/*#define HAVE_STRTOL*/
/*#define HAVE__I64TOA*/
/*#define HAVE__UI64TOA*/
/*#define HAVE_STRTOLL*/
#define HAVE_STRCMP
#define HAVE_STRNCMP
/*#define HAVE_STRICMP*/
/*#define HAVE_STRCASECMP*/
#define HAVE_SSCANF
/*#define HAVE_SNPRINTF*/
#define HAVE_VSNPRINTF

#endif /* HAVE_LIBC */

#endif /* _SDL_config_h */
