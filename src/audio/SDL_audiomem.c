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

/* Functions to allocate audio buffer memory, shareable across threads
	(necessary because SDL audio emulates threads with fork()
 */

#ifdef FORK_HACK
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stddef.h>
#endif

#include "SDL_stdinc.h"
#include "SDL_audiomem.h"

/* Allocate memory that will be shared between threads (freed on exit) */
void *SDL_AllocAudioMem(int size)
{
	void *chunk;

#ifdef FORK_HACK
	int   semid;
	
	/* Create and get the address of a shared memory segment */
	semid = shmget(IPC_PRIVATE, size, (IPC_CREAT|0600));
	if ( semid < 0 ) {
		return(NULL);
	}
	chunk = shmat(semid, NULL, 0);
	
	/* Set the segment for deletion when it is detatched */
	shmctl(semid, IPC_RMID, NULL);	/* Delets semid if shmat() fails */
#else
	chunk = SDL_malloc(size);
#endif
	return((void *)chunk);
}

void SDL_FreeAudioMem(void *chunk)
{
#ifdef FORK_HACK
	shmdt(chunk);
#else
	SDL_free(chunk);
#endif
}
