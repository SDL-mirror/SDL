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

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#if defined (__SASC) || defined(STORMC4_WOS)
#include <proto/dos.h>
#include <proto/exec.h>
#else
#include <inline/dos.h>
#include <inline/exec.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mydebug.h"

extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;

#ifdef STORMC4_WOS
#include <proto/powerpc.h>

/* use powerpc.library functions instead og exec */
#define SYS_ThreadHandle struct TaskPPC *
#define Signal SignalPPC
#define Wait WaitPPC
#define Task TaskPPC
#define FindTask FindTaskPPC
#define SetSignal SetSignalPPC

#define InitSemaphore InitSemaphorePPC
#define ObtainSemaphore ObtainSemaphorePPC
#define AttemptSemaphore AttemptSemaphorePPC
#define ReleaseSemaphore ReleaseSemaphorePPC
#define SignalSemaphore SignalSemaphorePPC

#else

#define SYS_ThreadHandle struct Task *
#endif /*STORMC4_WOS*/

