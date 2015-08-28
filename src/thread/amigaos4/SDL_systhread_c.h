/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

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

#ifndef SDL_SYSTHREAD_C_H
#define SDL_SYSTHREAD_C_H

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/lists.h>

typedef struct Process * SYS_ThreadHandle;

struct PList
{
	struct List list;
	struct SignalSemaphore *sem;
};

typedef int (*plistForEachFn)(struct Node *, struct Node *);

#ifndef SIGNAL_CHILD_TERM
#define SIGNAL_CHILD_TERM SIGBREAKF_CTRL_D
#endif

#ifndef SIGNAL_SEMAPHORE
#define SIGNAL_SEMAPHORE SIGBREAKF_CTRL_E
#endif

//#include "../main/amigaos4/SDL_os4timer_c.h"


//os4timer_Instance *os4thread_GetTimer(void);

#endif
