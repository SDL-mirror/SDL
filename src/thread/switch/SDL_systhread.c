/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2015 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_THREAD_SWITCH

/* SWITCH thread management routines for SDL */

#include <stdio.h>
#include <switch.h>

#include "SDL_error.h"
#include "SDL_thread.h"
#include "../SDL_systhread.h"

#define STACK_SIZE 0x5000

static void
SDL_SYS_RunThread(void *data)
{
    SDL_RunThread(data);
}

int SDL_SYS_CreateThread(SDL_Thread *thread, void *args)
{
    Result res = threadCreate(&thread->handle, SDL_SYS_RunThread, args, STACK_SIZE, 0x1C, -2);
    if (res != 0) {
        return SDL_SetError("threadCreate() failed: 0x%08X", res);
    }

    res = threadStart(&thread->handle);
    if (res != 0) {
        return SDL_SetError("threadStart() failed: 0x%08X", res);
    }

    return 0;
}

void SDL_SYS_SetupThread(const char *name)
{
    /* Do nothing. */
}

SDL_threadID SDL_ThreadID(void)
{
    return (SDL_threadID) armGetTls();
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
    threadWaitForExit(&thread->handle);
    threadClose(&thread->handle);
}

void SDL_SYS_DetachThread(SDL_Thread *thread)
{
    if (thread->handle.handle) {
        threadClose(&thread->handle);
    }
}

int SDL_SYS_SetThreadPriority(SDL_ThreadPriority priority)
{
    return 0;
}

#endif /* SDL_THREAD_SWITCH */
