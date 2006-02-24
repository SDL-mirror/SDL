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

#define SDL_HAS_64BIT_TYPE	1

/* Useful headers */
#define HAVE_STDARG_H	1
#define HAVE_STDDEF_H	1
#define HAVE_INTTYPES_H	1

/* Enable various audio drivers */
#define SDL_AUDIO_DRIVER_DISK	1
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
#define SDL_VIDEO_DRIVER_DUMMY	1
#ifdef _WIN32_WCE
#define SDL_VIDEO_DRIVER_GAPI	1
#endif
#define SDL_VIDEO_DRIVER_WINDIB	1

/* Enable OpenGL support */
#define SDL_VIDEO_OPENGL	1
#define SDL_VIDEO_OPENGL_WGL	1

/* Enable assembly routines */
#define SDL_ASSEMBLY_ROUTINES	1

#endif /* _SDL_config_win32_h */
