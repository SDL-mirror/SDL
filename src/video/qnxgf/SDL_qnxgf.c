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
#include "SDL_version.h"
#include "SDL_syswm.h"

/* Include QNX Graphics Framework declarations */
#include <gf/gf.h>

#include "SDL_qnxgf.h"
#include "SDL_gf_pixelfmt.h"

/******************************************************************************/
/* SDL Generic video modes, which could provide GF                            */
/* This is real pain in the ass. GF is just wrapper around a selected driver  */
/* some drivers could support double scan modes, like 320x200, 512x384, etc   */
/* but some drivers are not. Later we can distinguish one driver from another */
/* Feel free to add any new custom graphics mode                              */
/******************************************************************************/
static SDL_DisplayMode generic_mode[]=
{
   {0, 320, 200, 70, NULL},    /* 320x200 modes are 70Hz and 85Hz          */
   {0, 320, 200, 85, NULL},
   {0, 320, 240, 70, NULL},    /* 320x240 modes are 70Hz and 85Hz          */
   {0, 320, 240, 85, NULL},
   {0, 512, 384, 60, NULL},    /* 512x384 modes are 60Hz and 70Hz          */
   {0, 512, 384, 70, NULL},
   {0, 640, 480, 60, NULL},    /* 640x480 modes are 60Hz, 75Hz, 85Hz       */
   {0, 640, 480, 75, NULL},
   {0, 640, 480, 85, NULL},
   {0, 800, 600, 60, NULL},    /* 800x600 modes are 60Hz, 75Hz, 85Hz       */
   {0, 800, 600, 75, NULL},
   {0, 800, 600, 85, NULL},
   {0, 800, 480, 60, NULL},    /* 800x480 mode is 60Hz only                */
   {0, 1024, 640, 60, NULL},   /* 1024x640 mode is 60Hz only               */
   {0, 1024, 768, 60, NULL},   /* 1024x768 modes are 60Hz, 70Hz, 75Hz      */
   {0, 1024, 768, 70, NULL},
   {0, 1024, 768, 75, NULL},
   {0, 1280, 720, 60, NULL},   /* 1280x720 mode is 60Hz only               */
   {0, 1280, 768, 60, NULL},   /* 1280x768 mode is 60Hz only               */
   {0, 1280, 800, 60, NULL},   /* 1280x800 mode is 60Hz only               */
   {0, 1280, 960, 60, NULL},   /* 1280x960 mode is 60Hz only               */
   {0, 1280, 1024, 60, NULL},  /* 1280x1024 mode is 60Hz only              */
   {0, 1400, 1050, 60, NULL},  /* 1400x1050 mode is 60Hz only              */
   {0, 1440, 900, 60, NULL},   /* 1440x900 mode is 60Hz only               */
   {0, 1440, 960, 60, NULL},   /* 1440x960 mode is 60Hz only               */
   {0, 1600, 1200, 60, NULL},  /* 1600x1200 mode is 60Hz only              */
   {0, 1680, 1050, 60, NULL},  /* 1680x1050 mode is 60Hz only              */
   {0, 1920, 1080, 60, NULL},  /* 1920x1080 mode is 60Hz only              */
   {0, 1920, 1200, 60, NULL},  /* 1920x1200 mode is 60Hz only              */
   {0, 2048, 1536, 60, NULL},  /* 2048x1536 mode is 60Hz only              */
   {0, 2048, 1080, 60, NULL},  /* 2048x1080 mode is 60Hz only              */
   {0,    0,    0,  0, NULL}   /* End of generic mode list                 */
};

/*****************************************************************************/
/* SDL Video Device initialization functions                                 */
/*****************************************************************************/

static int qnxgf_available(void)
{
   gf_dev_t gfdev;
   gf_dev_info_t gfdev_info;
   int status;

   /* Try to attach to graphics device driver */
   status=gf_dev_attach(&gfdev, GF_DEVICE_INDEX(0), &gfdev_info);
   if (status!=GF_ERR_OK)
   {
      return 0;
   }

   /* Detach from graphics device driver for now */
   gf_dev_detach(gfdev);

   return 1;
}

static void qnxgf_destroy(SDL_VideoDevice* device)
{
   SDL_VideoData* gfdata=(SDL_VideoData*) device->driverdata;

   /* Detach from graphics device driver, if it was initialized */
   if (gfdata->gfinitialized!=SDL_FALSE)
   {
      gf_dev_detach(gfdata->gfdev);
   }

   if (device->driverdata!=NULL)
   {
      SDL_free(device->driverdata);
      device->driverdata=NULL;
   }
   SDL_free(device);
}

static SDL_VideoDevice* qnxgf_create(int devindex)
{
   SDL_VideoDevice* device;
   SDL_VideoData*   gfdata;
   int              status;

   /* Initialize SDL_VideoDevice structure */
   device = (SDL_VideoDevice*)SDL_calloc(1, sizeof(SDL_VideoDevice));
   if (device==NULL)
   {
      SDL_OutOfMemory();
      return NULL;
   }

   /* Initialize internal GF specific data */
   gfdata = (SDL_VideoData*)SDL_calloc(1, sizeof(SDL_VideoData));
   if (gfdata==NULL)
   {
      SDL_OutOfMemory();
      SDL_free(device);
      return NULL;
   }
   device->driverdata=gfdata;

   /* Try to attach to graphics device driver */
   status=gf_dev_attach(&gfdata->gfdev, GF_DEVICE_INDEX(devindex), &gfdata->gfdev_info);
   if (status!=GF_ERR_OK)
   {
      SDL_OutOfMemory();
      SDL_free(gfdata);
      SDL_free(device);
      return NULL;
   }

   /* Setup amount of available displays and current display */
   device->num_displays=0;
   device->current_display=0;

   /* Setup device shutdown function */
   gfdata->gfinitialized=SDL_TRUE;
   device->free=qnxgf_destroy;

   /* Setup all functions which we can handle */
   device->VideoInit=qnxgf_videoinit;
   device->VideoQuit=qnxgf_videoquit;
   device->GetDisplayModes=qnxgf_getdisplaymodes;
   device->SetDisplayMode=qnxgf_setdisplaymode;
   device->SetDisplayPalette=qnxgf_setdisplaypalette;
   device->GetDisplayPalette=qnxgf_getdisplaypalette;
   device->SetDisplayGammaRamp=qnxgf_setdisplaygammaramp;
   device->GetDisplayGammaRamp=qnxgf_getdisplaygammaramp;
   device->CreateWindow=qnxgf_createwindow;
   device->CreateWindowFrom=qnxgf_createwindowfrom;
   device->SetWindowTitle=qnxgf_setwindowtitle;
   device->SetWindowIcon=qnxgf_setwindowicon;
   device->SetWindowPosition=qnxgf_setwindowposition;
   device->SetWindowSize=qnxgf_setwindowsize;
   device->ShowWindow=qnxgf_showwindow;
   device->HideWindow=qnxgf_hidewindow;
   device->RaiseWindow=qnxgf_raisewindow;
   device->MaximizeWindow=qnxgf_maximizewindow;
   device->MinimizeWindow=qnxgf_minimizewindow;
   device->RestoreWindow=qnxgf_restorewindow;
   device->SetWindowGrab=qnxgf_setwindowgrab;
   device->DestroyWindow=qnxgf_destroywindow;
   device->GetWindowWMInfo=qnxgf_getwindowwminfo;
   device->GL_LoadLibrary=qnxgf_gl_loadlibrary;
   device->GL_GetProcAddress=qnxgf_gl_getprocaddres;
   device->GL_UnloadLibrary=qnxgf_gl_unloadlibrary;
   device->GL_CreateContext=qnxgf_gl_createcontext;
   device->GL_MakeCurrent=qnxgf_gl_makecurrent;
   device->GL_SetSwapInterval=qnxgf_gl_setswapinterval;
   device->GL_GetSwapInterval=qnxgf_gl_getswapinterval;
   device->GL_SwapWindow=qnxgf_gl_swapwindow;
   device->GL_DeleteContext=qnxgf_gl_deletecontext;
   device->PumpEvents=qnxgf_pumpevents;
   device->SuspendScreenSaver=qnxgf_suspendscreensaver;

   return device;
}

VideoBootStrap qnxgf_bootstrap=
{
   "qnxgf",
   "SDL QNX Graphics Framework (GF) video driver",
   qnxgf_available,
   qnxgf_create
};

/*****************************************************************************/
/* SDL Video and Display initialization/handling functions                   */
/*****************************************************************************/
int qnxgf_videoinit(_THIS)
{
   SDL_VideoData* gfdata=(SDL_VideoData*)_this->driverdata;
   uint32_t it;

   /* Add each detected output to SDL */
   for (it=0; it<gfdata->gfdev_info.ndisplays; it++)
   {
      SDL_VideoDisplay  display;
      SDL_DisplayMode   current_mode;
      SDL_DisplayData*  didata;
      int status;

      didata=(SDL_DisplayData*)SDL_calloc(1, sizeof(SDL_DisplayData));
      if (didata==NULL)
      {
         /* memory allocation problem */
         SDL_OutOfMemory();
         return -1;
      }

      status=gf_display_query(gfdata->gfdev, it, &didata->display_info);
      if (status==GF_ERR_OK)
      {
         SDL_zero(current_mode);
         current_mode.w=didata->display_info.xres;
         current_mode.h=didata->display_info.yres;
         current_mode.refresh_rate=didata->display_info.refresh;
         current_mode.format=qnxgf_gf_to_sdl_pixelformat(didata->display_info.format);
         current_mode.driverdata=NULL;
      }
      else
      {
         /* video initialization problem */
         SDL_free(didata);
         SDL_SetError("Display query failed");
         return -1;
      }

      /* Attach GF to selected display */
      status=gf_display_attach(&didata->display, gfdata->gfdev, it, NULL);
      if (status!=GF_ERR_OK)
      {
         /* video initialization problem */
         SDL_free(didata);
         SDL_SetError("Couldn't not attach to display");
         return -1;
      }

      SDL_zero(display);
      display.desktop_mode = current_mode;
      display.current_mode = current_mode;
      display.driverdata = didata;
      SDL_AddVideoDisplay(&display);
   }

   /* video has been initialized successfully */
   return 1;
}

void qnxgf_videoquit(_THIS)
{
   SDL_DisplayData* didata;
   uint32_t it;

   for(it=0; it<_this->num_displays; it++)
   {
      didata=_this->displays[it].driverdata;

      /* Detach from selected display */
      gf_display_detach(didata->display);
   }
}

void qnxgf_getdisplaymodes(_THIS)
{
   SDL_DisplayData* didata = (SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
   SDL_DisplayMode  mode;
   gf_modeinfo_t    modeinfo;
   uint32_t it=0;
   uint32_t jt=0;
   uint32_t kt=0;
   int      status;

   do {
      status=gf_display_query_mode(didata->display, it, &modeinfo);
      if (status==GF_ERR_OK)
      {
         /* Parsing current mode */
         if ((modeinfo.flags & GF_MODE_GENERIC)==GF_MODE_GENERIC)
         {
            /* This mode is generic, so we can add to SDL our resolutions */
            /* Only pixel format is fixed, refresh rate could be any      */
            jt=0;
            do {
               if (generic_mode[jt].w==0)
               {
                  break;
               }

               mode.w=generic_mode[jt].w;
               mode.h=generic_mode[jt].h;
               mode.refresh_rate=generic_mode[jt].refresh_rate;
               mode.format=qnxgf_gf_to_sdl_pixelformat(modeinfo.primary_format);
               mode.driverdata=NULL;
               SDL_AddDisplayMode(_this->current_display, &mode);

               jt++;
            } while(1);
         }
         else
         {
            /* Add this display mode as is in case if it is non-generic */
            /* But go through the each refresh rate, supported by gf    */
            jt=0;
            do {
               if (modeinfo.refresh[jt]!=0)
               {
                  mode.w=modeinfo.xres;
                  mode.h=modeinfo.yres;
                  mode.refresh_rate=modeinfo.refresh[jt];
                  mode.format=qnxgf_gf_to_sdl_pixelformat(modeinfo.primary_format);
                  mode.driverdata=NULL;
                  SDL_AddDisplayMode(_this->current_display, &mode);
                  jt++;
               }
               else
               {
                  break;
               }
            } while(1);
         }
      }
      else
      {
         if (status==GF_ERR_PARM)
         {
            /* out of available modes, all are listed */
            break;
         }

         /* Error occured during mode listing */
         break;
      }
      it++;
   } while(1);
}

int qnxgf_setdisplaymode(_THIS, SDL_DisplayMode* mode)
{
   SDL_DisplayData* didata = (SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
   int result;

   result=gf_display_set_mode(didata->display, mode->w, mode->h, mode->refresh_rate,
                              qnxgf_sdl_to_gf_pixelformat(mode->format), 0);
   if (result!=GF_ERR_OK)
   {
      /* Display mode/resolution switch has been failed */
      SDL_SetError("Mode is not supported by qnxgf driver");
      return -1;
   }

   return 0;
}

int qnxgf_setdisplaypalette(_THIS, SDL_Palette* palette)
{
   /* Palette must be set through the QNXGF renderer */
   /* It connected to surface, part of it            */

   /* Setting display palette operation has been failed */
   return -1;
}

int qnxgf_getdisplaypalette(_THIS, SDL_Palette* palette)
{
   /* We can give to upper level palette, which it set before */

   /* Getting display palette operation has been failed */
   return -1;
}

int qnxgf_setdisplaygammaramp(_THIS, Uint16* ramp)
{
   SDL_DisplayData* didata = (SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
   int status;

   /* GF can set Color LUT independently for each color channel, but SDL */
   /* uses combined gamma ramp, set it to each channel                   */
   status=gf_display_set_color_lut16(didata->display, (uint16_t*)ramp, (uint16_t*)ramp, (uint16_t*)ramp);
   if (status!=GF_ERR_OK)
   {
      /* Setting display gamma ramp operation has been failed */
      return -1;
   }

   return 0;
}

int qnxgf_getdisplaygammaramp(_THIS, Uint16* ramp)
{
   /* We need to return previous gamma set */

   /* Getting display gamma ramp operation has been failed */
   return -1;
}

int qnxgf_createwindow(_THIS, SDL_Window* window)
{
   /* Failed to create new window */
   return -1;
}

int qnxgf_createwindowfrom(_THIS, SDL_Window* window, const void* data)
{
   /* Failed to create window from another window */
   return -1;
}

void qnxgf_setwindowtitle(_THIS, SDL_Window* window)
{
}

void qnxgf_setwindowicon(_THIS, SDL_Window* window, SDL_Surface* icon)
{
}

void qnxgf_setwindowposition(_THIS, SDL_Window* window)
{
}

void qnxgf_setwindowsize(_THIS, SDL_Window* window)
{
}

void qnxgf_showwindow(_THIS, SDL_Window* window)
{
}

void qnxgf_hidewindow(_THIS, SDL_Window* window)
{
}

void qnxgf_raisewindow(_THIS, SDL_Window* window)
{
}

void qnxgf_maximizewindow(_THIS, SDL_Window* window)
{
}

void qnxgf_minimizewindow(_THIS, SDL_Window* window)
{
}

void qnxgf_restorewindow(_THIS, SDL_Window* window)
{
}

void qnxgf_setwindowgrab(_THIS, SDL_Window* window)
{
}

void qnxgf_destroywindow(_THIS, SDL_Window* window)
{
   printf("qnxgf_destroywindow()\n");
}

/*****************************************************************************/
/* SDL Window Manager function                                               */
/*****************************************************************************/
SDL_bool qnxgf_getwindowwminfo(_THIS, SDL_Window* window, struct SDL_SysWMinfo* info)
{
   if (info->version.major <= SDL_MAJOR_VERSION)
   {
      return SDL_TRUE;
   }
   else
   {
      SDL_SetError("Application not compiled with SDL %d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
      return SDL_FALSE;
   }

   /* Failed to get window manager information */
   return SDL_FALSE;
}

/*****************************************************************************/
/* SDL OpenGL/OpenGL ES functions                                            */
/*****************************************************************************/
int qnxgf_gl_loadlibrary(_THIS, const char* path)
{
   /* Failed to load new GL library */
   return -1;
}

void* qnxgf_gl_getprocaddres(_THIS, const char* proc)
{
   /* Failed to get GL function address pointer */
   return NULL;
}

void qnxgf_gl_unloadlibrary(_THIS)
{
}

SDL_GLContext qnxgf_gl_createcontext(_THIS, SDL_Window* window)
{
   /* Failed to create GL context */
   return NULL;
}

int qnxgf_gl_makecurrent(_THIS, SDL_Window* window, SDL_GLContext context)
{
   /* Failed to set current GL context */
   return -1;
}

int qnxgf_gl_setswapinterval(_THIS, int interval)
{
   /* Failed to set swap interval */
   return -1;
}

int qnxgf_gl_getswapinterval(_THIS)
{
   /* Failed to get default swap interval */
   return -1;
}

void qnxgf_gl_swapwindow(_THIS, SDL_Window* window)
{
}

void qnxgf_gl_deletecontext(_THIS, SDL_GLContext context)
{
}

/*****************************************************************************/
/* SDL Event handling function                                               */
/*****************************************************************************/
void qnxgf_pumpevents(_THIS)
{
}

/*****************************************************************************/
/* SDL screen saver related functions                                        */
/*****************************************************************************/
void qnxgf_suspendscreensaver(_THIS)
{
   /* There is no screensaver in pure console, it may exist when running */
   /* GF under Photon, but I do not know, how to disable screensaver     */
}

/* vi: set ts=4 sw=4 expandtab: */
