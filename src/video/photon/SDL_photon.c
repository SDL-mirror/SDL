/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

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

#include "SDL_config.h"

#include "../SDL_sysvideo.h"

static SDL_bool photon_initialized=SDL_FALSE;

static int photon_available(void)
{
   int status;

   /* Check if Photon was initialized before */
   if (photon_initialized==SDL_FALSE)
   {
      /* Initialize Photon widget library and open channel to Photon */
      status=PtInit(NULL);
      if (status==0)
      {
         photon_initialized=SDL_TRUE;
         return 1;
      }
      else
      {
         photon_initialized=SDL_FALSE;
         return 0;
      }
   }

   return 1;
}

static void photon_destroy(SDL_VideoDevice* device)
{
}

static SDL_VideoDevice* photon_create(int devindex)
{
}

VideoBootStrap photon_bootstrap=
{
   "photon",
   "SDL Photon video driver",
   photon_available,
   photon_create
};

/* vi: set ts=4 sw=4 expandtab: */
