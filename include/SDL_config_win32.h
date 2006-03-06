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

#ifndef _SDL_config_win32_h
#define _SDL_config_win32_h

/* This is a set of defines to configure the SDL features */

#ifdef _MSC_VER
typedef signed __int8		int8_t;
typedef unsigned __int8		uint8_t;
typedef signed __int16		int16_t;
typedef unsigned __int16	uint16_t;
typedef signed __int32		int32_t;
typedef unsigned __int32	uint32_t;
typedef signed __int64		int64_t;
typedef unsigned __int64	uint64_t;
#ifndef _UINTPTR_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    uintptr_t;
#else
typedef unsigned int   uintptr_t;
#endif
#define _UINTPTR_T_DEFINED
#endif
#else
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned int size_t;
typedef unsigned int uintptr_t;
#endif /* _MSC_VER */
#define SDL_HAS_64BIT_TYPE	1

/* Useful headers */
#define HAVE_STDARG_H	1
#define HAVE_STDDEF_H	1

/* Enable various audio drivers */
#ifndef _WIN32_WCE
#define SDL_AUDIO_DRIVER_DSOUND	1
#endif
#define SDL_AUDIO_DRIVER_WAVEOUT	1

/* Enable various cdrom drivers */
#define SDL_CDROM_WIN32	1

/* Enable various input drivers */
#define SDL_JOYSTICK_WINMM	1

/* Enable various shared object loading systems */
#define SDL_LOADSO_WIN32	1

/* Enable various threading systems */
#define SDL_THREAD_WIN32	1

/* Enable various timer systems */
#ifdef _WIN32_WCE
#define SDL_TIMER_WINCE	1
#else
#define SDL_TIMER_WIN32	1
#endif

/* Enable various video drivers */
#ifndef _WIN32_WCE
#define SDL_VIDEO_DRIVER_DDRAW	1
#endif
#ifdef _WIN32_WCE
#define SDL_VIDEO_DRIVER_GAPI	1
#endif
#define SDL_VIDEO_DRIVER_WINDIB	1

/* Enable OpenGL support */
#ifndef _WIN32_WCE
#define SDL_VIDEO_OPENGL	1
#define SDL_VIDEO_OPENGL_WGL	1
#endif

/* Enable assembly routines */
#define SDL_ASSEMBLY_ROUTINES	1

#endif /* _SDL_config_win32_h */
