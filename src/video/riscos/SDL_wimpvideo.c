/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

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
#include "SDL_config.h"

/*
     File added by Alan Buckley (alan_baa@hotmail.com) for RISC OS compatability
	 27 March 2003

     Implements RISC OS Wimp display.
*/

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_timer.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_riscostask.h"
#include "SDL_riscosvideo.h"
#include "SDL_riscosevents_c.h"
#include "SDL_riscosmouse_c.h"

#include "kernel.h"
#include "swis.h"

static int WIMP_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void WIMP_SetWMCaption(_THIS, const char *title, const char *icon);
static int WIMP_IconifyWindow(_THIS);
static void WIMP_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

/* RISC OS Wimp handling helpers */
static unsigned int WIMP_SetupWindow(_THIS, SDL_Surface *surface);
static void WIMP_SetDeviceMode(_THIS);


SDL_Surface *WIMP_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	unsigned char *buffer = NULL;
	int bytesPerPixel;
	const RISCOS_SDL_PixelFormat *fmt;

	/* Don't support double buffering in Wimp mode */
	flags &= ~SDL_DOUBLEBUF;
	flags &= ~SDL_HWSURFACE;

	/* Identify the current pixel format */
	fmt = RISCOS_CurrentPixelFormat();
	/* If it's the same (approximate) BPP as the desired BPP, use it directly (less overhead in sprite rendering) */
	if ((fmt == NULL) || (fmt->sdl_bpp != ((bpp+1)&~1)))
	{
		/* Not a good match - look for a supported format which is correct */
		fmt = WIMP_FindSupportedSpriteFormat(bpp);
		if (fmt == NULL)
		{
			SDL_SetError("Pixel depth %d not supported", bpp);
			return NULL;
		}
	}

	if (fmt->sdl_bpp == 8)
	{
		/* Emulated palette using ColourTrans */
		flags |= SDL_HWPALETTE;
	}
	bytesPerPixel = 1 << (fmt->ro.log2bpp - 3);

/* 	printf("Setting mode %dx%d\n", width, height);*/

	/* Allocate the new pixel format for the screen */
	if ( ! SDL_ReallocFormat(current, fmt->sdl_bpp, fmt->rmask, fmt->gmask, fmt->bmask, 0) ) {
		return(NULL);
	}

	/* Set up the new mode framebuffer */
	current->w = width;
	this->hidden->height = current->h = height;

	if (bpp == 15) bpp = 16;
	buffer = WIMP_CreateBuffer(width, height, &fmt->ro);
	if (buffer == NULL)
	{
		return (NULL);
	}

	this->hidden->bank[0] = buffer + 60; /* Start of sprite data */
	if (bpp == 8) this->hidden->bank[0] += 2048; /* 8bpp sprite have palette first */

	this->hidden->bank[1] = buffer;      /* Start of buffer */

	/* Remember sprite buffer so it can be freed later */
	if (this->hidden->alloc_bank) SDL_free(this->hidden->alloc_bank);
	this->hidden->alloc_bank = buffer;

	current->pitch = width * bytesPerPixel;
	if ((current->pitch & 3))
	{
		/* Sprites are 32bit word aligned */
		current->pitch += (4 - (current->pitch & 3));
	}
	this->hidden->format = fmt;

  	current->flags = flags | SDL_PREALLOC;

	WIMP_ReadModeInfo(this);
	
    SDL_memset(this->hidden->bank[0], 0, height * current->pitch);

	this->hidden->current_bank = 0;
	current->pixels = this->hidden->bank[0];


	if (WIMP_SetupWindow(this, current) == 0)
	{
		return NULL;
	}

	/* Reset device functions for the wimp */
	WIMP_SetDeviceMode(this);

	/* Needs to set up plot info after window has been created */
	/* Not sure why, but plots don't work if I do it earlier */
	WIMP_SetupPlotInfo(this);

	/* Poll until window is shown */
	{
	   /* We wait until it gets the focus, but give up after 5 seconds
	      in case the focus is prevented in any way.
	   */
	   Uint32 now = SDL_GetTicks();
	   while (!hasFocus && SDL_GetTicks() - now < 5000)
	   {
	      WIMP_Poll(this, 0);
	   }
	}

	/* We're done */
	return(current);
}


void WIMP_ReadModeInfo(_THIS)
{
	_kernel_swi_regs regs;
	int vars[6];
	int vals[5];

	vars[0] = 4;  /* XEig */
	vars[1] = 5;  /* YEig */
	vars[2] = 9;  /* Log base 2 bpp */
	vars[3] = 11; /* Screen Width - 1 */
	vars[4] = 12; /* Screen Height - 1 */
	vars[5] = -1; /* Terminate list */

	regs.r[0] = (int)vars;
	regs.r[1] = (int)vals;
	_kernel_swi(OS_ReadVduVariables, &regs, &regs);
	this->hidden->xeig = vals[0];
	this->hidden->yeig = vals[1];
	this->hidden->screen_width = vals[3];
	this->hidden->screen_height = vals[4];
}

/* Set device function to call the correct versions for running
   in a wimp window */

void WIMP_SetDeviceMode(_THIS)
{
	if (this->UpdateRects == WIMP_UpdateRects) return; /* Already set up */

	this->SetColors   = WIMP_SetColors;
	this->UpdateRects = WIMP_UpdateRects;

	this->FlipHWSurface = NULL;

	this->SetCaption = WIMP_SetWMCaption;
	this->SetIcon = NULL;
	this->IconifyWindow = WIMP_IconifyWindow;
	
	this->ShowWMCursor = WIMP_ShowWMCursor;
	this->WarpWMCursor = WIMP_WarpWMCursor;

/* Currently need to set this up here as it only works if you
   start up in a Wimp mode */
        this->ToggleFullScreen = RISCOS_ToggleFullScreen;

	this->PumpEvents = WIMP_PumpEvents;	
}

/* Setup the Window to display the surface */
unsigned int WIMP_SetupWindow(_THIS, SDL_Surface *surface)
{
	_kernel_swi_regs regs;
	_kernel_oserror *error;
	int window_data[23];
    int	*window_block = window_data+1;
	int x = (((this->hidden->screen_width + 1) << this->hidden->xeig) - (surface->w << 1)) / 2;
	int y = (((this->hidden->screen_height + 1) << this->hidden->yeig) - (surface->h << 1)) / 2;

	/* Always delete the window and recreate on a change */
	if (this->hidden->window_handle) WIMP_DeleteWindow(this);

	/* Setup window co-ordinates */
   window_block[0] = x;
   window_block[1] = y;
   window_block[2] = window_block[0] + (surface->w << 1);
   window_block[3] = window_block[1] + (surface->h << 1);

   
   window_block[4] = 0;				  /* Scroll offsets */
   window_block[5] = 0;
   window_block[6] = -1;			  /* Open on top of window stack */

   window_block[7] = 0x80040242;      /* Window flags */
   if (!(surface->flags & SDL_NOFRAME)) {
      window_block[7] |= 0x5000000;
      if (riscos_closeaction != 0) window_block[7] |= 0x2000000;
   }

   /* TODO: Take into account surface->flags */

   window_block[8] = 0xff070207;      /* Window colours and extra flags */
   window_block[9] = 0x020c0103;
   window_block[10] = 0;                    /* Work area minimum */
   window_block[11] = -surface->h << 1;
   window_block[12] = surface->w << 1;   /* Work area maximum */
   window_block[13] = 0;
   window_block[14] = 0x2700013d;    /* Title icon flags */
   window_block[15] = 0x00003000;	 /* Work area flags - Mouse click down reported */
   window_block[16] = 1;             /* Sprite area control block pointer */
   window_block[17] = 0x00100010;	 /* Minimum window size (width & height) (16x16)*/
   window_block[18] = (int)this->hidden->title;    /* Title data */
   window_block[19] = -1;
   window_block[20] = 256;
   window_block[21] = 0;			 /* Number of icons */

   regs.r[1] = (unsigned int)(window_block);
   
   /* Create the window */
   error = _kernel_swi(Wimp_CreateWindow, &regs, &regs);
   if (error == NULL)
   {
	   this->hidden->window_handle = window_data[0] = regs.r[0];

	   /* Show the window on the screen */
	   regs.r[1] = (unsigned int)window_data;
       if (_kernel_swi(Wimp_OpenWindow, &regs, &regs) == NULL)
       {
          WIMP_SetFocus(this->hidden->window_handle);
       } else
       {
		  WIMP_DeleteWindow(this);
	   }
   } else {
       SDL_SetError("Unable to create window: %s (%i)", error->errmess, error->errnum);
   }

   return this->hidden->window_handle;
}

/* Destroy the Window */

void WIMP_DeleteWindow(_THIS)
{
	_kernel_swi_regs regs;
    regs.r[1] = (unsigned int)&(this->hidden->window_handle);
	_kernel_swi(Wimp_DeleteWindow, &regs, &regs);
	this->hidden->window_handle = 0;
}


void WIMP_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	_kernel_swi_regs regs;
	int update_block[12];
	int j;
	update_block[0] = this->hidden->window_handle;

	for (j = 0; j < numrects; j++)
	{
		update_block[1] = rects[j].x << 1; /* Min X */
		update_block[4] = -(rects[j].y << 1);
		update_block[3] = update_block[1] + (rects[j].w << 1);
		update_block[2] = update_block[4] - (rects[j].h << 1);

		regs.r[1] = (int)update_block;
		/* Update window can fail if called before first poll */
		if (_kernel_swi(Wimp_UpdateWindow, &regs, &regs) == 0)
		{
			while (regs.r[0])
			{
				WIMP_PlotSprite(this, update_block[1], update_block[2]);
				_kernel_swi(Wimp_GetRectangle, &regs, &regs);
			}
		}
	}
}


int WIMP_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
   unsigned int *pal = (unsigned int *)(this->hidden->bank[1]+60);
   int j;
   SDL_Rect update;

   pal += firstcolor*2;
   for (j = 0; j < ncolors; j++)
   {
      *pal = (((unsigned int)colors->r) << 8)
             + (((unsigned int)colors->g) << 16)
             + (((unsigned int)colors->b) << 24);
      pal[1] = *pal;
      pal += 2;
      colors++;
   }

   WIMP_SetupPlotInfo(this);

   /* Need to refresh the window */
   update.x = 0;
   update.y = 0;
   update.w = SDL_VideoSurface->w;
   update.h = SDL_VideoSurface->h;
   WIMP_UpdateRects(this, 1, &update);
      
	return 1;
}

void WIMP_SetWMCaption(_THIS, const char *title, const char *icon)
{
	_kernel_swi_regs regs;

	SDL_strlcpy(this->hidden->title, title, SDL_arraysize(this->hidden->title));

	if (RISCOS_GetWimpVersion() < 380)
	{
		int block[6];

		regs.r[1] = (int)block;
		_kernel_swi(Wimp_GetCaretPosition, &regs, &regs);
		if (block[0] == (int)this->hidden->window_handle)
		{
			regs.r[0] = -1;
			_kernel_swi(Wimp_SetCaretPosition, &regs,&regs);
		} else
		{
			regs.r[0] = this->hidden->window_handle;
			regs.r[1] = -1;
			regs.r[2] = -1;
			regs.r[3] = -1;
			_kernel_swi(Wimp_SetCaretPosition, &regs,&regs);
		}
		regs.r[0] = block[0];
		regs.r[1] = block[1];
		regs.r[2] = block[2];
		regs.r[3] = block[3];
		regs.r[4] = block[4];
		regs.r[5] = block[5];
		_kernel_swi(Wimp_SetCaretPosition, &regs,&regs);
	} else
	{
		regs.r[0] = this->hidden->window_handle;
		regs.r[1] = 0x4b534154; /* "TASK" */
		regs.r[2] = 3; /* Redraw title */
		_kernel_swi(Wimp_ForceRedraw, &regs, &regs);
	}
}

int WIMP_IconifyWindow(_THIS)
{
	_kernel_swi_regs regs;

	int block[12];
	block[0] = 48;
	block[1] = RISCOS_GetTaskHandle();
	block[2] = 0;
	block[3] = 0;
	block[4] = 0x400ca; /* Message_Iconize */
	block[5] = this->hidden->window_handle;
	block[6] = RISCOS_GetTaskHandle();

	SDL_strlcpy((char *)&block[7], this->hidden->title, 20);

	regs.r[0] = 17; /* User_Message */
	regs.r[1] = (int)block;
	regs.r[2] = 0;
	_kernel_swi(Wimp_SendMessage, &regs, &regs);

	return 1;
}

/* Toggle to window from full screen */
int WIMP_ToggleFromFullScreen(_THIS)
{     
   int width = this->screen->w;
   int height = this->screen->h;
   int bpp = this->screen->format->BitsPerPixel;
   unsigned char *buffer = NULL;
   unsigned char *old_bank[2];
   unsigned char *old_alloc_bank;

   /* Ensure flags are OK */
   this->screen->flags &= ~(SDL_DOUBLEBUF|SDL_HWSURFACE);

   if (this->hidden->bank[0] == this->hidden->alloc_bank || riscos_backbuffer == 0)
   {
      /* Need to create a sprite for the screen and copy the data to it */
      unsigned char *data;
      buffer = WIMP_CreateBuffer(width, height, &this->hidden->format->ro);
      data = buffer + 60;         /* Start of sprite data */
      if (bpp == 8) data += 2048;  /* 8bpp sprite have palette first */

      if (buffer == NULL) return 0;
      SDL_memcpy(data, this->hidden->bank[0], width * height * this->screen->format->BytesPerPixel);
   }
   /* else We've switch to full screen before so we already have a sprite */

   old_bank[0] = this->hidden->bank[0];
   old_bank[1] = this->hidden->bank[1];
   old_alloc_bank = this->hidden->alloc_bank;

   if (buffer != NULL) this->hidden->alloc_bank = buffer;

   this->hidden->bank[1] = this->hidden->alloc_bank;
   this->hidden->bank[0] = this->hidden->bank[1] + 60; /* Start of sprite data */
   if (bpp == 8) this->hidden->bank[0] += 2048; /* 8bpp sprite have palette first */

   this->hidden->current_bank = 0;
   this->screen->pixels = this->hidden->bank[0];

   RISCOS_RestoreWimpMode();
   WIMP_ReadModeInfo(this);
   if (WIMP_SetupWindow(this, this->screen))
   {
      WIMP_SetDeviceMode(this);
      WIMP_SetupPlotInfo(this);

      if (riscos_backbuffer == 0) riscos_backbuffer = 1;

      if (buffer && old_alloc_bank) SDL_free(old_alloc_bank);

      return 1;
   } else
   {
      /* Drop back to full screen mode on failure */
      this->hidden->bank[0] = old_bank[0];
      this->hidden->bank[1] = old_bank[1];
      this->hidden->alloc_bank = old_alloc_bank;
      if (buffer) SDL_free(buffer);
      
      RISCOS_StoreWimpMode();
      FULLSCREEN_SetMode(width, height, bpp);
   }

   return 0;
}
