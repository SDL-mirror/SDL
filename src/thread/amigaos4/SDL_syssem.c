/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

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

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

#include "SDL_thread.h"
#include "../../timer/amigaos4/SDL_os4timer_c.h"
#include "../../thread/amigaos4/SDL_systhread_c.h"

#include <proto/exec.h>
#include <dos/dos.h>

#define MUTEX_SIGNAL SIGBREAKF_CTRL_F
#define BREAK_SIGNAL SIGBREAKF_CTRL_C

struct SDL_semaphore
{
    APTR mutex; // Protects the control block
    Uint32 count; // Current value
    struct MinList waiters; // Task waiting on this semaphore
};

typedef struct OS4_WaiterNode
{
    struct MinNode node;
    struct Task* task;
} OS4_WaiterNode;

SDL_sem *
SDL_CreateSemaphore(Uint32 initial_value)
{
    SDL_sem* sem = (SDL_sem *) SDL_malloc(sizeof(*sem));
    if (sem) {
        sem->mutex = IExec->AllocSysObjectTags(ASOT_MUTEX,
            ASOMUTEX_Recursive, TRUE,
            TAG_DONE);

        if (!sem->mutex) {
            dprintf("Failed to allocate mutex\n");
            SDL_SetError("Failed to allocate mutex");
            SDL_free(sem);
            return NULL;
        }

        IExec->NewMinList(&sem->waiters);

        sem->count = initial_value;

        dprintf("Created semaphore %p with count %d\n", sem, sem->count);
    } else {
        SDL_OutOfMemory();
    }
    return (sem);
}

void
SDL_DestroySemaphore(SDL_sem * sem)
{
    if (sem) {
        dprintf("Destroying semaphore %p\n", sem);

        if (sem->mutex) {
            IExec->MutexObtain(sem->mutex);

			if (!IsMinListEmpty(&sem->waiters)) {
                dprintf("Semaphore %p has waiters\n");

                OS4_WaiterNode* node;

                while ((node = (OS4_WaiterNode *)IExec->RemHead((struct List *)&sem->waiters))) {
                    dprintf("Interrupting waiting task %p\n", node->task);

                    IExec->Signal(node->task, BREAK_SIGNAL);

                    /* Reschedule tasks */
                    IExec->MutexRelease(sem->mutex);
                    IExec->MutexObtain(sem->mutex);
                }
            }

            IExec->MutexRelease(sem->mutex);

            IExec->FreeSysObject(ASOT_MUTEX, sem->mutex);
            sem->mutex = NULL;
        }
        SDL_free(sem);
    }
}

int
SDL_SemWaitTimeout(SDL_sem * sem, Uint32 timeout)
{
    if (!sem) {
        return SDL_SetError("Passed a NULL sem");
    }

    BOOL wait = TRUE;

    struct Task* task = IExec->FindTask(NULL);

    ULONG alarmSignal = 0;

    if (timeout > 0) {
        alarmSignal = OS4_TimerSetAlarm(OS4_ThreadGetTimer(), timeout);
    }

    while (wait) {
        OS4_WaiterNode node;

        IExec->MutexObtain(sem->mutex);

        if (sem->count > 0) {
            sem->count--;
            wait = FALSE;
        } else {
            if (timeout == 0) {
                //dprintf("Semaphore %p trying timed out\n", sem);
                IExec->MutexRelease(sem->mutex);
                return SDL_MUTEX_TIMEDOUT;
            }

            node.task = task;
            IExec->AddTail((struct List *)&sem->waiters, (struct Node *)&node);
        }

        IExec->MutexRelease(sem->mutex);

        if (wait) {
            //dprintf("Semaphore %p signals before wait 0x%X, count %u\n", sem, IExec->SetSignal(0L, 0L), sem->count);

            const ULONG signals = IExec->Wait(MUTEX_SIGNAL | BREAK_SIGNAL | alarmSignal);

            IExec->MutexObtain(sem->mutex);
            IExec->Remove((struct Node *)&node);
            IExec->MutexRelease(sem->mutex);

            if (signals & BREAK_SIGNAL) {
                dprintf("Semaphore %p interrupted\n", sem);
                return SDL_MUTEX_TIMEDOUT;
            }

            if (signals & alarmSignal) {
                //dprintf("Semaphore %p timer triggered\n");
                return SDL_MUTEX_TIMEDOUT;
            }

            if (signals & MUTEX_SIGNAL) {
                dprintf("Semaphore %p got signal 0x%X\n", sem, signals);
            }
        }
    }

    if (timeout) {
        OS4_TimerClearAlarm(OS4_ThreadGetTimer());
    }

    //dprintf("Semaphore %p obtained\n", sem);

    return 0;
}

int
SDL_SemTryWait(SDL_sem * sem)
{
    return SDL_SemWaitTimeout(sem, 0);
}

int
SDL_SemWait(SDL_sem * sem)
{
    //dprintf("Called\n");

    return SDL_SemWaitTimeout(sem, SDL_MUTEX_MAXWAIT);
}

/* Returns the current count of the semaphore */
Uint32
SDL_SemValue(SDL_sem * sem)
{
    if (!sem) {
        SDL_SetError("Passed a NULL sem");
        return 0;
    }
    return (Uint32)sem->count;
}

int
SDL_SemPost(SDL_sem * sem)
{
    if (!sem) {
        return SDL_SetError("Passed a NULL sem");
    }

    //dprintf("Called\n");

    IExec->MutexObtain(sem->mutex);

    if (++sem->count == 1) {
        OS4_WaiterNode* node = (OS4_WaiterNode *)IExec->RemHead((struct List *)&sem->waiters);

        if (node) {
            dprintf("Signalling task %p for semaphore %p\n", node->task, sem);
            IExec->Signal(node->task, MUTEX_SIGNAL);
        }
    }

    IExec->MutexRelease(sem->mutex);

    dprintf("Semaphore %p value %u\n", sem, sem->count);

    return 0;
}

#endif /* SDL_THREAD_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */

