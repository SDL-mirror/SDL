/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

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

#if SDL_THREAD_AMIGAOS4

/* AmigaOS 4 thread management routines for SDL */

#include "SDL_thread.h"
#include "../SDL_thread_c.h"
#include "../SDL_systhread.h"
#include "SDL_systhread_c.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"
#include "../../video/amigaos4/SDL_os4library.h"

#include <proto/exec.h>
#include <proto/dos.h>

#define CHILD_SIGNAL SIGBREAKF_CTRL_D
#define BREAK_SIGNAL SIGBREAKF_CTRL_C

static struct DOSIFace* iDOS; // TODO: try to make centralized interface storage for SDL2 - now it's a mess with too many symbols all over the place
static struct DOSBase* dosBase;

typedef struct OS4_SafeList
{
    APTR mutex;
    struct MinList list;
} OS4_SafeList;

typedef struct OS4_ThreadNode
{
    struct MinNode node;
    struct Task* task;
    SDL_Thread* thread;
    OS4_TimerInstance timer;
} OS4_ThreadNode;

typedef struct OS4_ThreadControl
{
    OS4_ThreadNode primary;
    OS4_SafeList children;
    OS4_SafeList waiters;
} OS4_ThreadControl;

static OS4_ThreadControl control;

static BOOL initialized = FALSE;

// NOTE: Init and Quit are called from SDL.c at the moment.
void OS4_InitThreadSubSystem(void)
{
    if (initialized) {
        dprintf("Already initialized\n");
        return;
    }

    control.primary.task = IExec->FindTask(NULL);

    dprintf("Main task %p\n", control.primary.task);

    control.children.mutex = IExec->AllocSysObjectTags(ASOT_MUTEX, TAG_DONE);
    control.waiters.mutex = IExec->AllocSysObjectTags(ASOT_MUTEX, TAG_DONE);

    dprintf("Children mutex %p, waiters mutex %p\n", control.children.mutex, control.waiters.mutex);

    IExec->NewMinList((struct MinList *)&control.children.list);
    IExec->NewMinList((struct MinList *)&control.waiters.list);

    dosBase = (struct DOSBase *)OS4_OpenLibrary(DOSNAME, 50);
    iDOS = (struct DOSIFace *)OS4_GetInterface((struct Library *)dosBase);

    dprintf("dosBase %p, iDos %p\n", dosBase, iDOS);

    OS4_InitTimerSubSystem();
    OS4_TimerCreate(&control.primary.timer);

    control.primary.task->tc_UserData = &control.primary; // Timer lookup requires this

    initialized = TRUE;
}

void OS4_QuitThreadSubSystem(void)
{
    struct MinNode* iter;

    if (!initialized) {
        dprintf("Not initialized\n");
        return;
    }

    dprintf("Called from task %p\n", IExec->FindTask(NULL));

    do {
        IExec->MutexObtain(control.children.mutex);

        if (IsMinListEmpty(&control.children.list)) {
            dprintf("No child threads left - proceed with SDL2 shutdown\n");
            IExec->MutexRelease(control.children.mutex);
            break;
        } else {
            for (iter = control.children.list.mlh_Head; iter->mln_Succ; iter = iter->mln_Succ) {
                IExec->Signal(((OS4_ThreadNode *)iter)->task, SIGBREAKF_CTRL_C);
            }
        }

        IExec->MutexRelease(control.children.mutex);
    } while (TRUE);

    OS4_TimerDestroy(&control.primary.timer);

    OS4_QuitTimerSubSystem();

    dprintf("Freeing mutexes\n");

    IExec->FreeSysObject(ASOT_MUTEX, control.children.mutex);
    IExec->FreeSysObject(ASOT_MUTEX, control.waiters.mutex);

    control.children.mutex = NULL;
    control.waiters.mutex = NULL;

    dprintf("Dropping iDOS\n");

    OS4_DropInterface((struct Interface **)&iDOS);
    OS4_CloseLibrary((struct Library **)&dosBase);

    initialized = FALSE;

    dprintf("All done\n");
}

static LONG
OS4_RunThread(STRPTR args, int32 length, APTR execbase)
{
    struct Task* thisTask = IExec->FindTask(NULL);

    OS4_ThreadNode* node = thisTask->tc_UserData;

    node->task = thisTask;

    dprintf("This task %p, node %p, SDL_Thread %p\n", thisTask, node, node->thread);

    OS4_TimerCreate(&node->timer);

    IExec->MutexObtain(control.children.mutex);
    IExec->AddTail((struct List *)&control.children.list, (struct Node *)node);
    IExec->MutexRelease(control.children.mutex);

	SDL_RunThread(node->thread);

    return RETURN_OK;
}

static void
OS4_ExitThread(int32 returnCode, int32 finalData)
{
    struct Task* thisTask = IExec->FindTask(NULL);
	
    OS4_ThreadNode *node = (OS4_ThreadNode *)finalData; // TODO: cannot use thisTask->tc_UserData if this is sometimes called from other process' context

    dprintf("Called from task %p, finalData %p\n", thisTask, finalData);

    IExec->MutexObtain(control.children.mutex);

    dprintf("Removing node %p from children list\n", node);

    IExec->Remove((struct Node *)node);

    dprintf("Signalling waiters\n");

    IExec->MutexObtain(control.waiters.mutex);

    if (IsMinListEmpty(&control.waiters.list)) {
        dprintf("Waiters list is empty\n");
    } else {
        struct MinNode* iter;

        for (iter = control.waiters.list.mlh_Head; iter->mln_Succ; iter = iter->mln_Succ) {
            //dprintf("iter %p, sending CHILD_SIGNAL\n", iter);
            IExec->Signal(((OS4_ThreadNode *)iter)->task, CHILD_SIGNAL);
        }
    }

    IExec->MutexRelease(control.waiters.mutex);

    OS4_TimerDestroy(&node->timer);

    IExec->FreeVec(node);

    dprintf("Exiting\n");

    // Hold the mutex until end, to prevent parent task shutting down the thread subsystem
    IExec->MutexRelease(control.children.mutex);
}

int
SDL_SYS_CreateThread(SDL_Thread * thread)
{
    char nameBuffer[128];
    struct Task* thisTask = IExec->FindTask(NULL);

    OS4_ThreadNode* node = IExec->AllocVecTags(sizeof(OS4_ThreadNode),
        AVT_ClearWithValue, 0,
        TAG_DONE);

    if (!node) {
        dprintf("Failed to allocated thread node\n");
        return SDL_SetError("Not enough resources to create thread");
    }

    dprintf("Node %p\n", node);

    node->thread = thread;

    BPTR inputStream = iDOS->DupFileHandle(iDOS->Input());
    BPTR outputStream = iDOS->DupFileHandle(iDOS->Output());
    BPTR errorStream = iDOS->DupFileHandle(iDOS->ErrorOutput());

    snprintf(nameBuffer, sizeof(nameBuffer), "SDL thread %s (%p)", thread->name, thread);

    struct Process* child = iDOS->CreateNewProcTags(
        NP_Child,       TRUE,
        NP_Entry,       OS4_RunThread,
        NP_FinalCode,   OS4_ExitThread,
        // HACK: when running testthread, sometimes child process is calling exit() and SDL cleanup fails
        // because ExitThread() is called from other context. By passing the node pointer, it can be removed
        // from the list and quit doesn't hang.
        NP_FinalData,   (int32)node,
        NP_Input,       inputStream,
        NP_CloseInput,  inputStream != ZERO,
        NP_Output,      outputStream,
        NP_CloseOutput, outputStream != ZERO,
        NP_Error,       errorStream,
        NP_CloseError,  errorStream != ZERO,
        NP_Name,        nameBuffer,
        NP_Priority,    thisTask->tc_Node.ln_Pri,
        NP_StackSize,   thread->stacksize,
        NP_UserData,    (APTR)node,
        TAG_DONE);

    if (!child) {
        dprintf("Failed to create a new thread '%s'\n", thread->name);
        return SDL_SetError("Not enough resources to create thread");
    }

    dprintf("Created new thread '%s' (task %p)\n", thread->name, child);

    return 0;
}

void
SDL_SYS_SetupThread(const char * name)
{
    //dprintf("Called for '%s'\n", name);
}

SDL_threadID
SDL_ThreadID(void)
{
    return (SDL_threadID) IExec->FindTask(NULL);
}

int
SDL_SYS_SetThreadPriority(SDL_ThreadPriority priority)
{
    int value;

    switch (priority) {
        case SDL_THREAD_PRIORITY_LOW:
            value = -5;
            break;
        case SDL_THREAD_PRIORITY_HIGH:
            value = 5;
            break;
        case SDL_THREAD_PRIORITY_TIME_CRITICAL:
            value = 10;
            break;
        default:
            value = 0;
            break;
    }

    struct Task* task = IExec->FindTask(NULL);
    const BYTE old = IExec->SetTaskPri(task, value);

    dprintf("Changed task %p priority from %d to %d\n", task, old, value);

    return 0;
}

static BOOL
OS4_StartJoining(OS4_ThreadNode * waiterNode, SDL_Thread * thread)
{
    BOOL found = FALSE;

    struct MinNode* iter;

    //dprintf("Start\n");

    IExec->MutexObtain(control.children.mutex);

    if (IsMinListEmpty(&control.children.list)) {
        dprintf("Children list is empty\n");
    } else {
        for (iter = control.children.list.mlh_Head; iter->mln_Succ; iter = iter->mln_Succ) {
            if (((OS4_ThreadNode *)iter)->thread == thread) {
                found = TRUE;
                IExec->MutexObtain(control.waiters.mutex);
                IExec->AddTail((struct List *)&control.waiters.list, (struct Node *)waiterNode);
                IExec->MutexRelease(control.waiters.mutex);
                break;
            }
        }
    }

    IExec->MutexRelease(control.children.mutex);

    //dprintf("End\n");

    return found;
}

static void
OS4_StopJoining(OS4_ThreadNode * node)
{
    //dprintf("Start\n");
    IExec->MutexObtain(control.waiters.mutex);
    IExec->Remove((struct Node *)node);
    IExec->MutexRelease(control.waiters.mutex);
    //dprintf("End\n");
}

void
SDL_SYS_WaitThread(SDL_Thread * thread)
{
    dprintf("Waiting for '%s'\n", thread->name);

    OS4_ThreadNode node;
    node.task = IExec->FindTask(NULL);
    node.thread = ((OS4_ThreadNode *)node.task->tc_UserData)->thread;

    do {
        const BOOL found = OS4_StartJoining(&node, thread);

        if (found) {
            const ULONG signals = IExec->Wait(CHILD_SIGNAL | BREAK_SIGNAL);

            OS4_StopJoining(&node);

            if (signals & BREAK_SIGNAL) {
                dprintf("Break signal\n");
                return;
            }

            dprintf("Some child thread terminated\n");
        } else {
            dprintf("Thread '%s' doesn't exist\n", thread->name);
            return;
        }
    } while (TRUE);

    dprintf("Waiting over\n");
}

void
SDL_SYS_DetachThread(SDL_Thread * thread)
{
    dprintf("Called for '%s'\n", thread->name);

    // NOTE: not removing from child thread list because child tasks should exit
    // before their parent (NP_Child, TRUE)

#if 0
    IExec->MutexObtain(control.children.mutex);

    if (IsMinListEmpty(&control.children.list)) {
        dprintf("Children list is empty\n");
    } else {
        struct MinNode* iter;

        for (iter = control.children.list.mlh_Head; iter->mln_Succ; iter = iter->mln_Succ) {
            if (((OS4_ThreadNode *)iter)->thread == thread) {
                IExec->Remove((struct Node *)iter);
                IExec->FreeVec(iter);
                break;
            }
        }
    }

    IExec->MutexRelease(control.children.mutex);
#endif
}

OS4_TimerInstance*
OS4_ThreadGetTimer(void)
{
    struct Task* task = IExec->FindTask(NULL);
    OS4_ThreadNode* node = task->tc_UserData;

    //dprintf("Task %p, timer %p\n", task, &node->timer);

    return &node->timer;
}

#endif /* SDL_THREAD_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */

