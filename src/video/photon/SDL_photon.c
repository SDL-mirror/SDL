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

    QNX Photon GUI SDL driver
    Copyright (C) 2009 Mike Gorchak
    (mike@malva.ua, lestat@i.com.ua)
*/

#include "SDL_config.h"

#include "../SDL_sysvideo.h"
#include "SDL_version.h"
#include "SDL_syswm.h"

#include "../SDL_sysvideo.h"

#include "SDL_photon.h"

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
   SDL_VideoDevice* device;
   SDL_VideoData*   phdata;
   int              status;

   /* Check if photon could be initialized */
   status=photon_available();
   if (status==0)
   {
      /* Photon could not be used */
      return NULL;
   }

   /* Initialize SDL_VideoDevice structure */
   device=(SDL_VideoDevice*)SDL_calloc(1, sizeof(SDL_VideoDevice));
   if (device==NULL)
   {
      SDL_OutOfMemory();
      return NULL;
   }

   /* Initialize internal photon specific data */
   phdata=(SDL_VideoData*)SDL_calloc(1, sizeof(SDL_VideoData));
   if (phdata==NULL)
   {
      SDL_OutOfMemory();
      SDL_free(device);
      return NULL;
   }
   device->driverdata=phdata;

   /* Setup amount of available displays and current display */
   device->num_displays=0;
   device->current_display=0;
}

VideoBootStrap photon_bootstrap=
{
   "photon",
   "SDL QNX Photon video driver",
   photon_available,
   photon_create
};

/*****************************************************************************/
/* SDL Video and Display initialization/handling functions                   */
/*****************************************************************************/
int photon_videoinit(_THIS)
{
   SDL_VideoData* phdata=(SDL_VideoData*)_this->driverdata;

   /* Check for environment variables which could override some SDL settings */
//   didata->custom_refresh=0;
//   override = SDL_getenv("SDL_VIDEO_PHOTON_REFRESH_RATE");
//   if (override!=NULL)
//   {
//      if (SDL_sscanf(override, "%u", &didata->custom_refresh)!=1)
//      {
//         didata->custom_refresh=0;
//      }
//   }

   /* Add photon renderer to SDL */
   photon_addrenderdriver(_this);

   /* video has been initialized successfully */
   return 1;
}

void photon_videoquit(_THIS)
{
   SDL_DisplayData* didata;
   uint32_t it;

   /* SDL will restore our desktop mode on exit */
   for(it=0; it<_this->num_displays; it++)
   {
      didata=_this->displays[it].driverdata;
   }
}

void photon_getdisplaymodes(_THIS)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
   SDL_DisplayMode  mode;

}

int photon_setdisplaymode(_THIS, SDL_DisplayMode* mode)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;

   return 0;
}

int photon_setdisplaypalette(_THIS, SDL_Palette* palette)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;

   /* Setting display palette operation has been failed */
   return -1;
}

int photon_getdisplaypalette(_THIS, SDL_Palette* palette)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;

   /* Getting display palette operation has been failed */
   return -1;
}

int photon_setdisplaygammaramp(_THIS, Uint16* ramp)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;

   /* Setting display gamma ramp operation has been failed */
   return -1;
}

int photon_getdisplaygammaramp(_THIS, Uint16* ramp)
{
   /* Getting display gamma ramp operation has been failed */
   return -1;
}

int photon_createwindow(_THIS, SDL_Window* window)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
   SDL_WindowData*  wdata;

   /* Allocate window internal data */
   wdata=(SDL_WindowData*)SDL_calloc(1, sizeof(SDL_WindowData));
   if (wdata==NULL)
   {
      SDL_OutOfMemory();
      return -1;
   }

   /* Setup driver data for this window */
   window->driverdata=wdata;

   /* Check if window must support OpenGL ES rendering */
   if ((window->flags & SDL_WINDOW_OPENGL)==SDL_WINDOW_OPENGL)
   {
      /* Mark this window as OpenGL ES compatible */
      wdata->uses_gles=SDL_TRUE;
   }

   /* Window has been successfully created */
   return 0;
}

int photon_createwindowfrom(_THIS, SDL_Window* window, const void* data)
{
   /* Failed to create window from another window */
   return -1;
}

void photon_setwindowtitle(_THIS, SDL_Window* window)
{
}

void photon_setwindowicon(_THIS, SDL_Window* window, SDL_Surface* icon)
{
}

void photon_setwindowposition(_THIS, SDL_Window* window)
{
}

void photon_setwindowsize(_THIS, SDL_Window* window)
{
}

void photon_showwindow(_THIS, SDL_Window* window)
{
}

void photon_hidewindow(_THIS, SDL_Window* window)
{
}

void photon_raisewindow(_THIS, SDL_Window* window)
{
}

void photon_maximizewindow(_THIS, SDL_Window* window)
{
}

void photon_minimizewindow(_THIS, SDL_Window* window)
{
}

void photon_restorewindow(_THIS, SDL_Window* window)
{
}

void photon_setwindowgrab(_THIS, SDL_Window* window)
{
}

void photon_destroywindow(_THIS, SDL_Window* window)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
   SDL_WindowData*  wdata=(SDL_WindowData*)window->driverdata;

   if (wdata!=NULL)
   {
   }
}

/*****************************************************************************/
/* SDL Window Manager function                                               */
/*****************************************************************************/
SDL_bool photon_getwindowwminfo(_THIS, SDL_Window* window, struct SDL_SysWMinfo* info)
{
   if (info->version.major<=SDL_MAJOR_VERSION)
   {
      return SDL_TRUE;
   }
   else
   {
      SDL_SetError("application not compiled with SDL %d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
      return SDL_FALSE;
   }

   /* Failed to get window manager information */
   return SDL_FALSE;
}

/*****************************************************************************/
/* SDL OpenGL/OpenGL ES functions                                            */
/*****************************************************************************/
int photon_gl_loadlibrary(_THIS, const char* path)
{
   /* Failed to load new GL library */
   return -1;
}

void* photon_gl_getprocaddres(_THIS, const char* proc)
{
   /* Failed to get GL function address pointer */
   return NULL;
}

void photon_gl_unloadlibrary(_THIS)
{
}

SDL_GLContext photon_gl_createcontext(_THIS, SDL_Window* window)
{
   /* Failed to create GL context */
   return NULL;
}

int photon_gl_makecurrent(_THIS, SDL_Window* window, SDL_GLContext context)
{
   /* Failed to set current GL context */
   return -1;
}

int photon_gl_setswapinterval(_THIS, int interval)
{
   /* Failed to set swap interval */
   return -1;
}

int photon_gl_getswapinterval(_THIS)
{
   /* Failed to get default swap interval */
   return -1;
}

void photon_gl_swapwindow(_THIS, SDL_Window* window)
{
}

void photon_gl_deletecontext(_THIS, SDL_GLContext context)
{
}

/*****************************************************************************/
/* SDL Event handling function                                               */
/*****************************************************************************/
void photon_pumpevents(_THIS)
{
}

/*****************************************************************************/
/* SDL screen saver related functions                                        */
/*****************************************************************************/
void photon_suspendscreensaver(_THIS)
{
   /* There is no screensaver in pure console, it may exist when running */
   /* GF under Photon, but I do not know, how to disable screensaver     */
}

/* vi: set ts=4 sw=4 expandtab: */
