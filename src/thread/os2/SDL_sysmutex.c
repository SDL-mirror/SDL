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

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* Mutex functions using the OS/2 API */

#include <stdio.h>
#include <stdlib.h>
#define INCL_DOSERRORS
#define INCL_DOSSEMAPHORES
#include <os2.h>

#include "SDL_error.h"
#include "SDL_mutex.h"


struct SDL_mutex {
	HMTX hmtxID;
};

/* Create a mutex */
DECLSPEC SDL_mutex * SDLCALL SDL_CreateMutex(void)
{
  SDL_mutex *mutex;
  APIRET ulrc;

  /* Allocate mutex memory */
  mutex = (SDL_mutex *)malloc(sizeof(*mutex));
  if (mutex)
  {
    /* Create the mutex, with initial value signaled */
    ulrc = DosCreateMutexSem(NULL,                  // Create unnamed semaphore
                             &(mutex->hmtxID),      // Pointer to handle
                             0L,                    // Flags: create it private (not shared)
                             FALSE);                // Initial value: unowned
    if (ulrc!=NO_ERROR)
    {
      SDL_SetError("Couldn't create mutex");
      free(mutex);
      mutex = NULL;
    }
  } else {
    SDL_OutOfMemory();
  }
  return(mutex);
}

/* Free the mutex */
DECLSPEC void SDLCALL SDL_DestroyMutex(SDL_mutex *mutex)
{
  if ( mutex )
  {
    if ( mutex->hmtxID )
    {
      DosCloseMutexSem(mutex->hmtxID);
      mutex->hmtxID = 0;
    }
    free(mutex);
  }
}

/* Lock the mutex */
DECLSPEC int SDLCALL SDL_mutexP(SDL_mutex *mutex)
{
  if ( mutex == NULL )
  {
    SDL_SetError("Passed a NULL mutex");
    return -1;
  }
  if ( DosRequestMutexSem(mutex->hmtxID, SEM_INDEFINITE_WAIT) != NO_ERROR )
  {
    SDL_SetError("Couldn't wait on mutex");
    return -1;
  }
  return(0);
}

/* Unlock the mutex */
DECLSPEC int SDLCALL SDL_mutexV(SDL_mutex *mutex)
{
  if ( mutex == NULL )
  {
    SDL_SetError("Passed a NULL mutex");
    return -1;
  }
  if ( DosReleaseMutexSem(mutex->hmtxID) != NO_ERROR )
  {
    SDL_SetError("Couldn't release mutex");
    return -1;
  }
  return(0);
}
