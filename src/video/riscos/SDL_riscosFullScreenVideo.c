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
    slouken@devolution.com
*/

/*
     File added by Alan Buckley (alan_baa@hotmail.com) for RISCOS compatability
	 27 March 2003

     Implements RISCOS full screen display.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"

#include "SDL_riscostask.h"
#include "SDL_riscosvideo.h"
#include "SDL_riscosevents_c.h"
#include "SDL_riscosmouse_c.h"

#include "kernel.h"
#include "swis.h"
#include "unixlib/os.h"
#include "unixlib/local.h"

/* Private structures */
typedef struct tagScreenModeBlock
{
   int flags;  // mode selector flags, bit 0 = 1, bit 1-7 format specifier, 8-31 reserved
   int x_pixels;
   int y_pixels;
   int pixel_depth;  // 2^pixel_depth = bpp,i.e. 0 = 1, 1 = 2, 4 = 16, 5 = 32
   int frame_rate;   // -1 use first match
   int mode_vars[5]; // array of index, value pairs terminated by -1
} SCREENMODEBLOCK;


/* Helper functions */
void FULLSCREEN_SetDeviceMode(_THIS);
int FULLSCREEN_SetMode(int width, int height, int bpp);
void FULLSCREEN_SetupBanks(_THIS);

/* SDL video device functions for fullscreen mode */
static int FULLSCREEN_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static int FULLSCREEN_FlipHWSurface(_THIS, SDL_Surface *surface);
static void FULLSCREEN_UpdateRects(_THIS, int numrects, SDL_Rect *rects);
void FULLSCREEN_SetWMCaption(_THIS, const char *title, const char *icon);
extern int RISCOS_GetWmInfo(_THIS, SDL_SysWMinfo *info);

/* Local helper functions */
static int cmpmodes(const void *va, const void *vb);
static int FULLSCREEN_AddMode(_THIS, int bpp, int w, int h);
void FULLSCREEN_SetWriteBank(int bank);
void FULLSCREEN_SetDisplayBank(int bank);
static void FULLSCREEN_DisableEscape();
static void FULLSCREEN_EnableEscape();
void FULLSCREEN_BuildModeList(_THIS);

/* Following variable is set up in riskosTask.c */
extern int riscos_backbuffer; /* Create a back buffer in system memory for full screen mode */



SDL_Surface *FULLSCREEN_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
   _kernel_swi_regs regs;
   Uint32 Rmask = 0;
   Uint32 Gmask = 0;
   Uint32 Bmask = 0;
   int create_back_buffer = riscos_backbuffer;

   switch(bpp)
   {
	case 8:
		flags |= SDL_HWPALETTE;
		break;

	case 15:
	case 16:
		Bmask = 0x00007c00;
		Gmask = 0x000003e0;
		Rmask = 0x0000001f;
		break;

	case 32:
		Bmask = 0x00ff0000;
		Gmask = 0x0000ff00;
		Rmask = 0x000000ff;
		break;

	default:
		SDL_SetError("Pixel depth not supported");
		return NULL;
		break;
   }

   if (FULLSCREEN_SetMode(width, height, bpp) == 0)
   {
	   SDL_SetError("Couldn't set requested mode");
	   return (NULL);
   }

/* 	printf("Setting mode %dx%d\n", width, height); */

	/* Allocate the new pixel format for the screen */
	if ( ! SDL_ReallocFormat(current, bpp, Rmask, Gmask, Bmask, 0) ) {
	    RISCOS_RestoreWimpMode();
		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return(NULL);
	}

	/* Set up the new mode framebuffer */
	current->w = width;
	this->hidden->height = current->h = height;

   regs.r[0] = -1; /* -1 for current screen mode */

   /* Get screen width in bytes */
   regs.r[1] = 6; // Screen Width in bytes
   _kernel_swi(OS_ReadModeVariable, &regs, &regs);

   current->pitch = regs.r[2];

   if (flags & SDL_DOUBLEBUF)
   {
	   regs.r[0] = 2; /* Screen area */
	   _kernel_swi(OS_ReadDynamicArea, &regs, &regs);
	   
	   /* Reg 1 has amount of memory currently used for display */
	   regs.r[0] = 2; /* Screen area */
	   regs.r[1] = (current->pitch * height * 2) - regs.r[1];
	   if (_kernel_swi(OS_ChangeDynamicArea, &regs, &regs) != NULL)
	   {
		   /* Can't allocate enough screen memory for double buffer */
		   flags &= ~SDL_DOUBLEBUF;
	   }
   }
   
  	current->flags = flags | SDL_FULLSCREEN | SDL_HWSURFACE | SDL_PREALLOC;
	

	/* Need to set display banks here for double buffering */
	if (flags & SDL_DOUBLEBUF)
	{
	   FULLSCREEN_SetWriteBank(0);
	   FULLSCREEN_SetDisplayBank(1);

         create_back_buffer = 0; /* Don't need a back buffer for a double buffered display */
    }

    FULLSCREEN_SetupBanks(this);

    if (create_back_buffer)
    {
       /* If not double buffered we may need to create a memory
       ** back buffer to simulate processing on other OS's. 
         ** This is turned on by setting the enviromental variable
         ** SDL$<name>$BackBuffer to 1
         */    
       this->hidden->bank[0] = malloc(height * current->pitch);
       if (this->hidden->bank[0] == 0)
       {
 	       RISCOS_RestoreWimpMode();
           SDL_SetError("Couldnt allocate memory for back buffer");
           return (NULL);
       }
       /* Surface updated in programs is now a software surface */
       current->flags &= ~SDL_HWSURFACE;
    }

	   /* Store address of allocated screen bank to be freed later */
	   if (this->hidden->alloc_bank) free(this->hidden->alloc_bank);
	   if (create_back_buffer)
	   {
		   this->hidden->alloc_bank = this->hidden->bank[0];
	   } else
		   this->hidden->alloc_bank = 0;

       // Clear both banks to black
       memset(this->hidden->bank[0], 0, height * current->pitch);
	   memset(this->hidden->bank[1], 0, height * current->pitch);

 	   this->hidden->current_bank = 0;
	   current->pixels = this->hidden->bank[0];

	/* Reset device functions for the wimp */
	FULLSCREEN_SetDeviceMode(this);

/*	FULLSCREEN_DisableEscape(); */

	/* We're done */
	return(current);
}

/* Reset any device functions that have been changed because we have run in WIMP mode */
void FULLSCREEN_SetDeviceMode(_THIS)
{
	if (this->SetColors == FULLSCREEN_SetColors) return; /* Already set up */

	this->SetColors   = FULLSCREEN_SetColors;
	this->UpdateRects = FULLSCREEN_UpdateRects;

	this->FlipHWSurface = FULLSCREEN_FlipHWSurface;

	this->SetCaption = FULLSCREEN_SetWMCaption;
	this->SetIcon = NULL;
	this->IconifyWindow = NULL;
	
	this->ShowWMCursor = RISCOS_ShowWMCursor;
	this->WarpWMCursor = FULLSCREEN_WarpWMCursor;

	this->PumpEvents = FULLSCREEN_PumpEvents;	
}

/* Query for the list of available video modes */
void FULLSCREEN_BuildModeList(_THIS)
{
	_kernel_swi_regs regs;
	char *enumInfo = NULL;
	char *enum_ptr;
	int *blockInfo;
	int j;
	int num_modes;

	/* Find out how much space we need */
	regs.r[0] = 2; /* Reason code */
	regs.r[2] = 0; /* Number of modes to skip */
	regs.r[6] = 0; /* pointer to block or 0 for count */
	regs.r[7] = 0; /* Size of block in bytes */
	_kernel_swi(OS_ScreenMode, &regs, &regs);

    num_modes = -regs.r[2];

	/* Video memory should be in r[5] */
	this->info.video_mem = regs.r[5]/1024;

	enumInfo = (unsigned char *)malloc(-regs.r[7]);
	if (enumInfo == NULL)
	{
		SDL_OutOfMemory();
		return;
	}
	/* Read mode information into block */
	regs.r[2] = 0;
	regs.r[6] = (int)enumInfo;
	regs.r[7] = -regs.r[7];
	_kernel_swi(OS_ScreenMode, &regs, &regs);

	enum_ptr = enumInfo;

	for (j =0; j < num_modes;j++)
	{
		blockInfo = (int *)enum_ptr;
		if ((blockInfo[1] & 255) == 1) /* We understand this format */
		{
			switch(blockInfo[4])
			{
			case 3: /* 8 bits per pixel */
				FULLSCREEN_AddMode(this, 8, blockInfo[2], blockInfo[3]);
				break;
			case 4: /* 15 bits per pixel */
				FULLSCREEN_AddMode(this, 15, blockInfo[2], blockInfo[3]);
				break;
			case 5: /* 32 bits per pixel */
				FULLSCREEN_AddMode(this, 32, blockInfo[2], blockInfo[3]);
				break;
			}
		}

		enum_ptr += blockInfo[0];
	}

	free(enumInfo);
		
	/* Sort the mode lists */
	for ( j=0; j<NUM_MODELISTS; ++j ) {
		if ( SDL_nummodes[j] > 0 ) {
			qsort(SDL_modelist[j], SDL_nummodes[j], sizeof *SDL_modelist[j], cmpmodes);
		}
	}
}

static int FULLSCREEN_FlipHWSurface(_THIS, SDL_Surface *surface)
{
   _kernel_swi_regs regs;
   regs.r[0] = 19;
   /* Wait for Vsync */
   _kernel_swi(OS_Byte, &regs, &regs);

   FULLSCREEN_SetDisplayBank(this->hidden->current_bank);
   this->hidden->current_bank ^= 1;
   FULLSCREEN_SetWriteBank(this->hidden->current_bank);
   surface->pixels = this->hidden->bank[this->hidden->current_bank];

	return(0);
}

static void FULLSCREEN_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
   if (riscos_backbuffer && (this->screen->flags & SDL_DOUBLEBUF) == 0)
   {
      /* If not double buffered copy rectangles to main screen now */
      int j;
      char *to, *from;
      int pitch = this->screen->pitch;
      int row;
      int xmult = this->screen->format->BytesPerPixel;
      for (j = 0; j < numrects; j++)
      {
         from = this->hidden->bank[0] + rects->x * xmult + rects->y * pitch;
         to  = this->hidden->bank[1] + rects->x * xmult + rects->y * pitch;
         for (row = 0; row < rects->h; row++)
         {
             memcpy(to, from, rects->w * xmult);
             from += pitch;
             to += pitch;
         }
         rects++;
      }
   }
}

int FULLSCREEN_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	_kernel_swi_regs regs;
	int palette[256];

	regs.r[0] = -1;
	regs.r[1] = -1;
	regs.r[2] = (int)palette;
	regs.r[3] = 1024;
	regs.r[4] = 0;
	_kernel_swi(ColourTrans_ReadPalette, &regs, &regs);

	while(ncolors--)
	{
		palette[firstcolor] = ((colors->b) << 24) | ((colors->g) << 16) | ((colors->r) << 8);
		firstcolor++;
		colors++;
	}

	regs.r[0] = -1;
	regs.r[1] = -1;
	regs.r[2] = (int)palette;
	regs.r[3] = 0;
	regs.r[4] = 0;
	_kernel_swi(ColourTrans_WritePalette, &regs, &regs);

	return(1);
}


static int cmpmodes(const void *va, const void *vb)
{
    SDL_Rect *a = *(SDL_Rect **)va;
    SDL_Rect *b = *(SDL_Rect **)vb;
    if(a->w > b->w)
        return -1;
    return b->h - a->h;
}

static int FULLSCREEN_AddMode(_THIS, int bpp, int w, int h)
{
	SDL_Rect *mode;
	int i, index;
	int next_mode;

	/* Check to see if we already have this mode */
	if ( bpp < 8 ) {  /* Not supported */
		return(0);
	}
	index = ((bpp+7)/8)-1;
	for ( i=0; i<SDL_nummodes[index]; ++i ) {
		mode = SDL_modelist[index][i];
		if ( (mode->w == w) && (mode->h == h) ) {
			return(0);
		}
	}

	/* Set up the new video mode rectangle */
	mode = (SDL_Rect *)malloc(sizeof *mode);
	if ( mode == NULL ) {
		SDL_OutOfMemory();
		return(-1);
	}
	mode->x = 0;
	mode->y = 0;
	mode->w = w;
	mode->h = h;

	/* Allocate the new list of modes, and fill in the new mode */
	next_mode = SDL_nummodes[index];
	SDL_modelist[index] = (SDL_Rect **)
	       realloc(SDL_modelist[index], (1+next_mode+1)*sizeof(SDL_Rect *));
	if ( SDL_modelist[index] == NULL ) {
		SDL_OutOfMemory();
		SDL_nummodes[index] = 0;
		free(mode);
		return(-1);
	}
	SDL_modelist[index][next_mode] = mode;
	SDL_modelist[index][next_mode+1] = NULL;
	SDL_nummodes[index]++;

	return(0);
}

void FULLSCREEN_SetWriteBank(int bank)
{
   _kernel_swi_regs regs;
   regs.r[0] = 112;
   regs.r[1] = bank+1;
   _kernel_swi(OS_Byte, &regs, &regs);
}

void FULLSCREEN_SetDisplayBank(int bank)
{
   _kernel_swi_regs regs;
   regs.r[0] = 113;
   regs.r[1] = bank+1;
   _kernel_swi(OS_Byte, &regs, &regs);
}


/** Disable special escape key processing */
static void FULLSCREEN_DisableEscape()
{
   _kernel_swi_regs regs;
   regs.r[0] = 229;
   regs.r[1] = 1;
   regs.r[2] = 0;
   _kernel_swi(OS_Byte, &regs, &regs);
  
}

/** Enable special escape key processing */
static void FULLSCREEN_EnableEscape()
{
   _kernel_swi_regs regs;
   regs.r[0] = 229;
   regs.r[1] = 0;
   regs.r[2] = 0;
   _kernel_swi(OS_Byte, &regs, &regs);
  
}

/** Store caption in case this is called before we create a window */
void FULLSCREEN_SetWMCaption(_THIS, const char *title, const char *icon)
{
	strncpy(this->hidden->title, title, 255);
	this->hidden->title[255] = 0;
}

/* Set screen mode
*
*  Returns 1 if mode is set ok, otherwise 0
*/

int FULLSCREEN_SetMode(int width, int height, int bpp)
{
   SCREENMODEBLOCK smb;
   _kernel_swi_regs regs;

   smb.flags = 1;
   smb.x_pixels = width;
   smb.y_pixels = height;
   smb.mode_vars[0] = -1;

   switch(bpp)
   {
	case 8:
		smb.pixel_depth = 3;
		/* Note: Need to set ModeFlags to 128 and NColour variables to 255 get full 8 bit palette */
		smb.mode_vars[0] = 0; smb.mode_vars[1] = 128; /* Mode flags */
		smb.mode_vars[2] = 3; smb.mode_vars[3] = 255; /* NColour (number of colours -1) */
		smb.mode_vars[4] = -1; /* End of list */
		break;

	case 15:
	case 16:
		smb.pixel_depth = 4;
		break;

	case 32:
		smb.pixel_depth = 5;
		break;

	default:
		SDL_SetError("Pixel depth not supported");
		return 0;
		break;
   }
   
   smb.frame_rate = -1;

   regs.r[0] = 0;
   regs.r[1] = (int)&smb;

   if (_kernel_swi(OS_ScreenMode, &regs, &regs) != 0)
   {
	   SDL_SetError("Couldn't set requested mode");
	   return 0;
   }

    /* Turn cursor off*/
    _kernel_oswrch(23);_kernel_oswrch(1);_kernel_oswrch(0);
    _kernel_oswrch(0);_kernel_oswrch(0);_kernel_oswrch(0);
    _kernel_oswrch(0);_kernel_oswrch(0);_kernel_oswrch(0);
    _kernel_oswrch(0);_kernel_oswrch(0);

   return 1;
}

/* Get Start addresses for the screen banks */
void FULLSCREEN_SetupBanks(_THIS)
{
   _kernel_swi_regs regs;
   int block[5];
   block[0] = 148; /* Write screen start */
   block[1] = 149; /* Display screen start */
   block[2] = 4;  /* X eig factor */
   block[3] = 5;  /* Y eig factor */
   block[4] = -1;  /* End of list of variables to request */

   regs.r[0] = (int)block;
   regs.r[1] = (int)block;
   _kernel_swi(OS_ReadVduVariables, &regs, &regs);

   this->hidden->bank[0] = (void *)block[0];
   this->hidden->bank[1] = (void *)block[1];
   this->hidden->xeig = block[2];
   this->hidden->yeig = block[3];
}

/* Toggle to full screen mode from the WIMP */

int FULLSCREEN_ToggleFromWimp(_THIS)
{
   int width = this->screen->w;
   int height = this->screen->h;
   int bpp = this->screen->format->BitsPerPixel;

   RISCOS_StoreWimpMode();
   if (FULLSCREEN_SetMode(width, height, bpp))
   {
       char *buffer = this->hidden->alloc_bank; /* This is start of sprite data */
       /* Support back buffer mode only */
       riscos_backbuffer = 1;

       FULLSCREEN_SetupBanks(this);

       this->hidden->bank[0] = buffer + 60; /* Start of sprite data */
       if (bpp == 8) this->hidden->bank[0] += 2048; /* 8bpp sprite have palette first */

	   this->hidden->current_bank = 0;
	   this->screen->pixels = this->hidden->bank[0];

       /* Copy back buffer to screen memory */
       memcpy(this->hidden->bank[1], this->hidden->bank[0], width * height * this->screen->format->BytesPerPixel);

       FULLSCREEN_SetDeviceMode(this);
       return 1;
   } else
      RISCOS_RestoreWimpMode();

   return 0;
}
