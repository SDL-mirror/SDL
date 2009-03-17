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

    QNX Graphics Framework SDL driver
    Copyright (C) 2009 Mike Gorchak
    (mike@malva.ua, lestat@i.com.ua)
*/

#include "SDL_config.h"

#include "../SDL_sysvideo.h"
#include "SDL_version.h"
#include "SDL_syswm.h"

/* Include QNX Graphics Framework declarations */
#include <gf/gf.h>

#include "SDL_qnxgf.h"
#include "SDL_gf_render.h"
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
   {0, 1280, 1024, 60, NULL},  /* 1280x1024 modes are 60Hz, 75Hz, 85Hz and */
   {0, 1280, 1024, 75, NULL},  /* 100 Hz                                   */
   {0, 1280, 1024, 85, NULL},  /*                                          */
   {0, 1280, 1024, 100, NULL}, /*                                          */
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

/* Low level device graphics driver names, which they are reporting */
GF_DeviceCaps gf_devicename[]=
{
   /* ATI Rage 128 graphics driver (devg-ati_rage128)      */
   {"ati_rage128", SDL_GF_ACCELERATED},
   /* Fujitsu Carmine graphics driver (devg-carmine.so)    */
   {"carmine", SDL_GF_ACCELERATED},
   /* C&T graphics driver (devg-chips.so)                  */
   {"chips", SDL_GF_ACCELERATED},
   /* Fujitsu Coral graphics driver (devg-coral.so)        */
   {"coral", SDL_GF_ACCELERATED},
   /* Intel integrated graphics driver (devg-extreme2.so)  */
   {"extreme2", SDL_GF_ACCELERATED},
   /* Unaccelerated FB driver (devg-flat.so)               */
   {"flat", SDL_GF_UNACCELERATED},
   /* NS Geode graphics driver (devg-geode.so)             */
   {"geode", SDL_GF_ACCELERATED},
   /* Geode LX graphics driver (devg-geodelx.so)           */
   {"geodelx", SDL_GF_ACCELERATED},
   /* Intel integrated graphics driver (devg-gma9xx.so)    */
   {"gma", SDL_GF_ACCELERATED},
   /* Intel integrated graphics driver (devg-i810.so)      */
   {"i810", SDL_GF_ACCELERATED},
   /* Intel integrated graphics driver (devg-i830.so)      */
   {"i830", SDL_GF_ACCELERATED},
   /* Geode LX graphics driver (devg-lx800.so)             */
   {"lx800", SDL_GF_ACCELERATED},
   /* Matrox Gxx graphics driver (devg-matroxg.so)         */
   {"matroxg", SDL_GF_ACCELERATED},
   /* Intel Poulsbo graphics driver (devg-poulsbo.so)      */
   {"poulsbo", SDL_GF_ACCELERATED},
   /* ATI Radeon driver (devg-radeon.so)                   */
   {"radeon", SDL_GF_ACCELERATED},
   /* ATI Rage driver (devg-rage.so)                       */
   {"rage", SDL_GF_ACCELERATED},
   /* S3 Savage graphics driver (devg-s3_savage.so)        */
   {"s3_savage", SDL_GF_ACCELERATED},
   /* SiS630 integrated graphics driver (devg-sis630.so)   */
   {"sis630", SDL_GF_ACCELERATED},
   /* PowerVR SGX 535 graphics driver (devg-poulsbo.so)    */
   {"sgx", SDL_GF_ACCELERATED},
   /* SM Voyager GX graphics driver (devg-smi5xx.so)       */
   {"smi5xx", SDL_GF_ACCELERATED},
   /* Silicon Motion graphics driver (devg-smi7xx.so)      */
   {"smi7xx", SDL_GF_ACCELERATED},
   /* SVGA unaccelerated gfx driver (devg-svga.so)         */
   {"svga", SDL_GF_UNACCELERATED},
   /* nVidia TNT graphics driver (devg-tnt.so)             */
   {"tnt", SDL_GF_ACCELERATED},
   /* VIA integrated graphics driver (devg-tvia.so)        */
   {"tvia", SDL_GF_ACCELERATED},
   /* VIA UniChrome graphics driver (devg-unichrome.so)    */
   {"unichrome", SDL_GF_ACCELERATED},
   /* VESA unaccelerated gfx driver (devg-vesa.so)         */
   {"vesa", SDL_GF_UNACCELERATED},
   /* VmWare graphics driver (devg-volari.so)              */
   {"vmware", SDL_GF_ACCELERATED},
   /* XGI XP10 graphics driver (devg-volari.so)            */
   {"volari", SDL_GF_ACCELERATED},
   /* End of list */
   {NULL, 0x00000000}
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
      device->driverdata=NULL;
   }
}

static SDL_VideoDevice* qnxgf_create(int devindex)
{
   SDL_VideoDevice* device;
   SDL_VideoData*   gfdata;
   int              status;

   /* Initialize SDL_VideoDevice structure */
   device=(SDL_VideoDevice*)SDL_calloc(1, sizeof(SDL_VideoDevice));
   if (device==NULL)
   {
      SDL_OutOfMemory();
      return NULL;
   }

   /* Initialize internal GF specific data */
   gfdata=(SDL_VideoData*)SDL_calloc(1, sizeof(SDL_VideoData));
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
   uint32_t jt;
   char* override;

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
         SDL_SetError("display query failed");
         return -1;
      }

      /* Attach GF to selected display */
      status=gf_display_attach(&didata->display, gfdata->gfdev, it, NULL);
      if (status!=GF_ERR_OK)
      {
         /* video initialization problem */
         SDL_free(didata);
         SDL_SetError("couldn't attach to display");
         return -1;
      }

      /* Initialize status variables */
      didata->layer_attached=SDL_FALSE;

      /* Attach to main display layer */
      status=gf_layer_attach(&didata->layer, didata->display, didata->display_info.main_layer_index, 0);
      if (status!=GF_ERR_OK)
      {
         SDL_SetError("couldn't attach to main layer, it could be busy");

         /* Failed to attach to main layer */
         return -1;
      }

      /* Enable layer in case if hardware supports layer enable/disable */
      gf_layer_enable(didata->layer);

      /* Mark main display layer is attached */
      didata->layer_attached=SDL_TRUE;

      /* Copy device name for each display */
      SDL_strlcpy(didata->description, gfdata->gfdev_info.description, SDL_VIDEO_GF_DEVICENAME_MAX-1);

      /* Search device capabilities and possible workarounds */
      jt=0;
      do {
         if (gf_devicename[jt].name==NULL)
         {
            break;
         }
         if (SDL_strncmp(gf_devicename[jt].name, didata->description, SDL_strlen(gf_devicename[jt].name))==0)
         {
            didata->caps=gf_devicename[jt].caps;
         }
         jt++;
      } while(1);

      /* Initialize display structure */
      SDL_zero(display);
      display.desktop_mode = current_mode;
      display.current_mode = current_mode;
      display.driverdata = didata;
      didata->current_mode=current_mode;
      SDL_AddVideoDisplay(&display);

      /* Check for environment variables which could override some SDL settings */
      didata->custom_refresh=0;
      override = SDL_getenv("SDL_VIDEO_GF_REFRESH_RATE");
      if (override!=NULL)
      {
         if (SDL_sscanf(override, "%u", &didata->custom_refresh)!=1)
         {
            didata->custom_refresh=0;
         }
      }
   }

   /* Add GF renderer to SDL */
   gf_addrenderdriver(_this);

   /* video has been initialized successfully */
   return 1;
}

void qnxgf_videoquit(_THIS)
{
   SDL_DisplayData* didata;
   uint32_t it;

   /* SDL will restore our desktop mode on exit */
   for(it=0; it<_this->num_displays; it++)
   {
      didata=_this->displays[it].driverdata;

      /* Detach from selected display */
      gf_display_detach(didata->display);
   }
}

void qnxgf_getdisplaymodes(_THIS)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
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
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
   uint32_t refresh_rate=0;
   int result;

   /* Current display dimensions and bpp are no more valid */
   didata->current_mode.format=SDL_PIXELFORMAT_UNKNOWN;
   didata->current_mode.w=0;
   didata->current_mode.h=0;

   /* Check if custom refresh rate requested */
   if (didata->custom_refresh!=0)
   {
      refresh_rate=didata->custom_refresh;
   }
   else
   {
      refresh_rate=mode->refresh_rate;
   }

   /* Check if SDL GF driver needs to find appropriate refresh rate itself */
   if (refresh_rate==0)
   {
      uint32_t it;
      SDL_DisplayMode tempmode;

      /* Clear display mode structure */
      SDL_memset(&tempmode, 0x00, sizeof(SDL_DisplayMode));
      tempmode.refresh_rate=0x0000FFFF;

      /* Check if window width and height matches one of our modes */
      for (it=0; it<SDL_CurrentDisplay.num_display_modes; it++)
      {
         if ((SDL_CurrentDisplay.display_modes[it].w==mode->w) &&
             (SDL_CurrentDisplay.display_modes[it].h==mode->h) &&
             (SDL_CurrentDisplay.display_modes[it].format==mode->format))
         {
            /* Find the lowest refresh rate available */
            if (tempmode.refresh_rate>SDL_CurrentDisplay.display_modes[it].refresh_rate)
            {
               tempmode=SDL_CurrentDisplay.display_modes[it];
            }
         }
      }
      if (tempmode.refresh_rate!=0x0000FFFF)
      {
         refresh_rate=tempmode.refresh_rate;
      }
      else
      {
         /* Let video driver decide what to do with this */
         refresh_rate=0;
      }
   }

   /* Check if SDL GF driver needs to check custom refresh rate */
   if (didata->custom_refresh!=0)
   {
      uint32_t it;
      SDL_DisplayMode tempmode;

      /* Clear display mode structure */
      SDL_memset(&tempmode, 0x00, sizeof(SDL_DisplayMode));
      tempmode.refresh_rate=0x0000FFFF;

      /* Check if window width and height matches one of our modes */
      for (it=0; it<SDL_CurrentDisplay.num_display_modes; it++)
      {
         if ((SDL_CurrentDisplay.display_modes[it].w==mode->w) &&
             (SDL_CurrentDisplay.display_modes[it].h==mode->h) &&
             (SDL_CurrentDisplay.display_modes[it].format==mode->format))
         {
            /* Find the lowest refresh rate available */
            if (tempmode.refresh_rate>SDL_CurrentDisplay.display_modes[it].refresh_rate)
            {
               tempmode=SDL_CurrentDisplay.display_modes[it];
            }

            /* Check if requested refresh rate found */
            if (refresh_rate==SDL_CurrentDisplay.display_modes[it].refresh_rate)
            {
               tempmode=SDL_CurrentDisplay.display_modes[it];
               break;
            }
         }
      }
      if (tempmode.refresh_rate!=0x0000FFFF)
      {
         refresh_rate=tempmode.refresh_rate;
      }
      else
      {
         /* Let video driver decide what to do with this */
         refresh_rate=0;
      }
   }

   /* Detach layer before switch to new graphics mode */
   if (didata->layer_attached==SDL_TRUE)
   {
      /* Disable layer if hardware supports this */
      gf_layer_disable(didata->layer);

      /* Detach from layer, free it for others */
      gf_layer_detach(didata->layer);

      /* Mark it as detached */
      didata->layer_attached=SDL_FALSE;
   }

   /* Set new display video mode */
   result=gf_display_set_mode(didata->display, mode->w, mode->h, refresh_rate,
                              qnxgf_sdl_to_gf_pixelformat(mode->format), 0);
   if (result!=GF_ERR_OK)
   {
      /* Display mode/resolution switch has been failed */
      SDL_SetError("mode is not supported by graphics driver");
      return -1;
   }
   else
   {
      didata->current_mode=*mode;
      didata->current_mode.refresh_rate=refresh_rate;
   }

   /* Attach to main display layer */
   result=gf_layer_attach(&didata->layer, didata->display, didata->display_info.main_layer_index, 0);
   if (result!=GF_ERR_OK)
   {
      SDL_SetError("couldn't attach to main layer, it could be busy");

      /* Failed to attach to main displayable layer */
      return -1;
   }

   /* Enable layer in case if hardware supports layer enable/disable */
   gf_layer_enable(didata->layer);

   /* Mark main display layer is attached */
   didata->layer_attached=SDL_TRUE;

   return 0;
}

int qnxgf_setdisplaypalette(_THIS, SDL_Palette* palette)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;

   /* QNX GF doesn't have support for global palette changing, but we */
   /* could store it for usage in future */

   /* Setting display palette operation has been failed */
   return -1;
}

int qnxgf_getdisplaypalette(_THIS, SDL_Palette* palette)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;

   /* We can't provide current palette settings and looks like SDL          */
   /* do not call this function also, in such case this function returns -1 */

   /* Getting display palette operation has been failed */
   return -1;
}

int qnxgf_setdisplaygammaramp(_THIS, Uint16* ramp)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
   int status;

   /* Setup gamma ramp, for each color channel */
   status=gf_display_set_color_lut16(didata->display, (uint16_t*)ramp, (uint16_t*)ramp+256, (uint16_t*)ramp+512);
   if (status!=GF_ERR_OK)
   {
      /* Setting display gamma ramp operation has been failed */
      return -1;
   }

   return 0;
}

int qnxgf_getdisplaygammaramp(_THIS, Uint16* ramp)
{
   /* TODO: We need to return previous gamma set           */
   /*       Also we need some initial fake gamma to return */

   /* Getting display gamma ramp operation has been failed */
   return -1;
}

int qnxgf_createwindow(_THIS, SDL_Window* window)
{
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
   SDL_WindowData*  wdata;

   /* QNX GF supports fullscreen window modes only */
   if ((window->flags & SDL_WINDOW_FULLSCREEN)!=SDL_WINDOW_FULLSCREEN)
   {
      uint32_t it;
      SDL_DisplayMode mode;

      /* Clear display mode structure */
      SDL_memset(&mode, 0x00, sizeof(SDL_DisplayMode));
      mode.refresh_rate=0x0000FFFF;

      /* Check if window width and height matches one of our modes */
      for (it=0; it<SDL_CurrentDisplay.num_display_modes; it++)
      {
         if ((SDL_CurrentDisplay.display_modes[it].w==window->w) &&
             (SDL_CurrentDisplay.display_modes[it].h==window->h) &&
             (SDL_CurrentDisplay.display_modes[it].format==SDL_CurrentDisplay.desktop_mode.format))
         {
            /* Find the lowest refresh rate available */
            if (mode.refresh_rate>SDL_CurrentDisplay.display_modes[it].refresh_rate)
            {
               mode=SDL_CurrentDisplay.display_modes[it];
            }
         }
      }

      /* Check if end of display list has been reached */
      if (mode.refresh_rate==0x0000FFFF)
      {
         SDL_SetError("desired video mode is not supported");

         /* Failed to create new window */
         return -1;
      }
      else
      {
         /* Tell to the caller that mode will be fullscreen */
         window->flags|=SDL_WINDOW_FULLSCREEN;

         /* Setup fullscreen mode, bpp used from desktop mode in this case */
         qnxgf_setdisplaymode(_this, &mode);
      }
   }

   /* Setup our own window decorations, which are depend on fullscreen mode */
   window->flags|=SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS |
                  SDL_WINDOW_MAXIMIZED | SDL_WINDOW_INPUT_GRABBED |
                  SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;
   window->flags&=~(SDL_WINDOW_RESIZABLE | SDL_WINDOW_MINIMIZED);

   /* Ignore any window position settings */
   window->x=SDL_WINDOWPOS_UNDEFINED;
   window->y=SDL_WINDOWPOS_UNDEFINED;

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
   SDL_DisplayData* didata=(SDL_DisplayData*)SDL_CurrentDisplay.driverdata;
   SDL_WindowData*  wdata=(SDL_WindowData*)window->driverdata;

   if (wdata!=NULL)
   {
   }
}

/*****************************************************************************/
/* SDL Window Manager function                                               */
/*****************************************************************************/
SDL_bool qnxgf_getwindowwminfo(_THIS, SDL_Window* window, struct SDL_SysWMinfo* info)
{
   /* QNX GF do not operates at window level, this means we are have no */
   /* Window Manager available, no specific data in SDL_SysWMinfo too   */

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
