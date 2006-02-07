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

#ifndef _SDL_stdlib_h
#define _SDL_stdlib_h

#include "SDL_config.h"

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "SDL_types.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_MALLOC
#define SDL_malloc	malloc
#else
extern DECLSPEC void * SDLCALL SDL_malloc(size_t size);
#endif

#ifdef HAVE_REALLOC
#define SDL_realloc	realloc
#else
extern DECLSPEC void * SDLCALL SDL_realloc(void *mem, size_t size);
#endif

#ifdef HAVE_FREE
#define SDL_free	free
#else
extern DECLSPEC void SDLCALL SDL_free(void *mem);
#endif

#ifdef HAVE_ALLOCA
#define SDL_stack_alloc(type, count)    (type*)alloca(sizeof(type)*count)
#define SDL_stack_free(data)
#else
#define SDL_stack_alloc(type, count)    (type*)SDL_malloc(sizeof(type)*count)
#define SDL_stack_free(data)            SDL_free(data)
#endif

#ifdef HAVE_GETENV
#define SDL_getenv	getenv
#else
extern DECLSPEC char * SDLCALL SDL_getenv(const char *name);
#endif

#ifdef HAVE_PUTENV
#define SDL_putenv	putenv
#else
extern DECLSPEC int SDLCALL SDL_putenv(const char *variable);
#endif

#ifdef HAVE_QSORT
#define SDL_qsort	qsort
#else
extern DECLSPEC void SDLCALL SDL_qsort(void *base, size_t nmemb, size_t size,
           int (*compare)(const void *, const void *));
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_stdlib_h */
