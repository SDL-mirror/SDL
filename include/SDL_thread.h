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

#ifndef _SDL_thread_h
#define _SDL_thread_h

/* Header for the SDL thread management routines 

	These are independent of the other SDL routines.
*/

#include "SDL_main.h"
#include "SDL_types.h"

/* Thread synchronization primitives */
#include "SDL_mutex.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* The SDL thread structure, defined in SDL_thread.c */
struct SDL_Thread;
typedef struct SDL_Thread SDL_Thread;

/* Create a thread */
#ifdef __OS2__
/*
   We compile SDL into a DLL on OS/2. This means, that it's the DLL which
   creates a new thread for the calling process with the SDL_CreateThread()
   API. There is a problem with this, that only the RTL of the SDL.DLL will
   be initialized for those threads, and not the RTL of the calling application!
   To solve this, we make a little hack here.
   We'll always use the caller's _beginthread() and _endthread() APIs to
   start a new thread. This way, it it's the SDL.DLL which uses this API,
   then the RTL of SDL.DLL will be used to create the new thread, and if it's
   the application, then the RTL of the application will be used.
   So, in short:
   Always use the _beginthread() and _endthread() of the calling runtime library!
*/

#ifdef __WATCOMC__
#include <process.h> // This has _beginthread() and _endthread() defined!
#endif
#ifdef __EMX__
#include <stdlib.h> // This has _beginthread() and _endthread() defined, if -Zmt flag is used!
#endif

typedef Uint32 SDLCALL (*pfnSDL_CurrentBeginThread)(void (*pfnThreadFn)(void *), Uint32 uiStackSize, void *pParam);
typedef void   SDLCALL (*pfnSDL_CurrentEndThread)(void);

extern DECLSPEC SDL_Thread * SDLCALL SDL_CreateThread_Core(int (*fn)(void *), void *data, pfnSDL_CurrentBeginThread pfnBeginThread, pfnSDL_CurrentEndThread pfnEndThread);

// Disable warnings about unreferenced symbol!
#pragma disable_message (202)
static Uint32 SDLCALL SDL_CurrentBeginThread(void (*pfnThreadFn)(void *), Uint32 uiStackSize, void *pParam)
{
  return _beginthread(pfnThreadFn, NULL, uiStackSize, pParam);
}

static void   SDLCALL SDL_CurrentEndThread(void)
{
  _endthread();
}

#define SDL_CreateThread(fn, data) SDL_CreateThread_Core(fn, data, SDL_CurrentBeginThread, SDL_CurrentEndThread)

#else
extern DECLSPEC SDL_Thread * SDLCALL SDL_CreateThread(int (SDLCALL *fn)(void *), void *data);
#endif

/* Get the 32-bit thread identifier for the current thread */
extern DECLSPEC Uint32 SDLCALL SDL_ThreadID(void);

/* Get the 32-bit thread identifier for the specified thread,
   equivalent to SDL_ThreadID() if the specified thread is NULL.
 */
extern DECLSPEC Uint32 SDLCALL SDL_GetThreadID(SDL_Thread *thread);

/* Wait for a thread to finish.
   The return code for the thread function is placed in the area
   pointed to by 'status', if 'status' is not NULL.
 */
extern DECLSPEC void SDLCALL SDL_WaitThread(SDL_Thread *thread, int *status);

/* Forcefully kill a thread without worrying about its state */
extern DECLSPEC void SDLCALL SDL_KillThread(SDL_Thread *thread);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_thread_h */
