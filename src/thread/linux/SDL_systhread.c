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

/* Linux thread management routines for SDL */

#include "SDL_error.h"
#include "SDL_thread.h"
#include "SDL_systhread.h"

#ifdef FORK_HACK

#include <unistd.h>

int SDL_SYS_CreateThread(SDL_Thread *thread, void *args)
{
	SDL_SetError("Threads are not supported on this platform");
	return(-1);
}
void SDL_SYS_SetupThread(void)
{
	return;
}
Uint32 SDL_ThreadID(void)
{
	return((Uint32)getpid());
}
void SDL_SYS_WaitThread(SDL_Thread *thread)
{
	return;
}
void SDL_SYS_KillThread(SDL_Thread *thread)
{
	return;
}

#else

#include <signal.h>

#if !defined(MACOSX) /* pthread_sigmask seems to be missing on MacOS X? */
/* List of signals to mask in the subthreads */
static int sig_list[] = {
	SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGALRM, SIGTERM, SIGCHLD, SIGWINCH,
	SIGVTALRM, SIGPROF, 0
};
#endif /* !MACOSX */

#ifdef SDL_USE_PTHREADS

#include <pthread.h>


static void *RunThread(void *data)
{
	SDL_RunThread(data);
	pthread_exit((void*)0);
	return((void *)0);		/* Prevent compiler warning */
}

int SDL_SYS_CreateThread(SDL_Thread *thread, void *args)
{
	pthread_attr_t type;

	/* Set the thread attributes */
	if ( pthread_attr_init(&type) != 0 ) {
		SDL_SetError("Couldn't initialize pthread attributes");
		return(-1);
	}
	pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);

	/* Create the thread and go! */
	if ( pthread_create(&thread->handle, &type, RunThread, args) != 0 ) {
		SDL_SetError("Not enough resources to create thread");
		return(-1);
	}
	return(0);
}

void SDL_SYS_SetupThread(void)
{
#if !defined(MACOSX) /* pthread_sigmask seems to be missing on MacOS X? */
	int i;
	sigset_t mask;

	/* Mask asynchronous signals for this thread */
	sigemptyset(&mask);
	for ( i=0; sig_list[i]; ++i ) {
		sigaddset(&mask, sig_list[i]);
	}
	pthread_sigmask(SIG_BLOCK, &mask, 0);
#endif /* !MACOSX */

#ifdef PTHREAD_CANCEL_ASYNCHRONOUS
	/* Allow ourselves to be asynchronously cancelled */
	{ int oldstate;
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate);
	}
#endif
}

/* WARNING:  This may not work for systems with 64-bit pid_t */
Uint32 SDL_ThreadID(void)
{
	return((Uint32)pthread_self());
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
	pthread_join(thread->handle, 0);
}

void SDL_SYS_KillThread(SDL_Thread *thread)
{
#ifdef PTHREAD_CANCEL_ASYNCHRONOUS
	pthread_cancel(thread->handle);
#else
#ifdef __FreeBSD__
#warning For some reason, this doesnt actually kill a thread - FreeBSD 3.2
#endif
	pthread_kill(thread->handle, SIGKILL);
#endif
}

#else /* Linux-specific clone() based implementation */

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>


/* Stack size for child thread */
#define STACKSIZE 16384*4		/* 16384 is too small */

#ifdef __GLIBC__
#include <sched.h>
#else
/* From <linux/sched.h> */
#define CLONE_VM      0x00000100   /* set if VM shared */
#define CLONE_FS      0x00000200   /* set if fs info shared */
#define CLONE_FILES   0x00000400   /* set if open files shared */
#define CLONE_SIGHAND 0x00000800   /* set if signal handlers shared */
#define CLONE_PID     0x00001000   /* set if pid shared */

/* The infamous "start_thread" function, courtesy Linus Torvalds */
extern int clone(int (*fn)(void *arg), void *child_stack, int flags, void *arg);
#endif

static int RunThread(void *data)
{
	SDL_RunThread(data);
	return(0);
}

int SDL_SYS_CreateThread(SDL_Thread *thread, void *args)
{
	void *stack;

	/* Allocate memory for thread stack */
	stack = malloc(STACKSIZE);
	if ( stack == (void *)0 ) {
		SDL_OutOfMemory();
		return(-1);
	}
	thread->data = stack;

	/* Adjust the stack since it actually grows down */
        stack = (void *) ((char *)stack + STACKSIZE);

	/* Create the thread and go! */
	thread->handle = clone(RunThread, stack,
			(CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND), args);
	if ( thread->handle < 0 ) {
		free(thread->data);
		SDL_SetError("Not enough resources to create thread");
		return(-1);
	}
	return(0);
}

void SDL_SYS_SetupThread(void)
{
	int i;
	sigset_t mask;

	/* Mask asynchronous signals for this thread */
	sigemptyset(&mask);
	for ( i=0; sig_list[i]; ++i ) {
		sigaddset(&mask, sig_list[i]);
	}
	sigprocmask(SIG_BLOCK, &mask, 0);
}

/* WARNING:  This may not work for systems with 64-bit pid_t */
Uint32 SDL_ThreadID(void)
{
	return((Uint32)getpid());
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
#ifdef __WCLONE
	errno = 0;
	while ( errno != ECHILD ) {
		waitpid(thread->handle, 0, __WCLONE);
	}
#else
	/* Ack, ugly ugly hack --
	   wait() doesn't work, waitpid() doesn't work, and ignoring SIG_CHLD
	   doesn't work .. and the child thread is still a zombie, so kill()
	   doesn't work.
	*/
	char command[1024];

	sprintf(command,
		"ps ax|fgrep -v fgrep|fgrep -v '<zombie>'|fgrep %d >/dev/null",
								thread->handle);
	while ( system(command) == 0 )
		sleep(1);
#endif
	free(thread->data);
}

void SDL_SYS_KillThread(SDL_Thread *thread)
{
	kill(thread->handle, SIGKILL);
}

#endif /* SDL_USE_PTHREADS */

#endif /* FORK_HACK */
