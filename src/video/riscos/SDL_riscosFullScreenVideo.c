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

     Implements RISC OS full screen display.
*/

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_riscostask.h"
#include "SDL_riscosvideo.h"
#include "SDL_riscosevents_c.h"
#include "SDL_riscosmouse_c.h"

#include "kernel.h"
#include "swis.h"
#include "unixlib/local.h"

/* Private structures */
typedef struct tagScreenModeBlock
{
   int flags;  // mode selector flags, bit 0 = 1, bit 1-7 format specifier, 8-31 reserved
   int x_pixels;
   int y_pixels;
   int pixel_depth;  // 2^pixel_depth = bpp,i.e. 0 = 1, 1 = 2, 4 = 16, 5 = 32
   int frame_rate;   // -1 use first match
   int mode_vars[7]; // array of index, value pairs terminated by -1
} SCREENMODEBLOCK;


/* Helper functions */
static void FULLSCREEN_SetupBanks(_THIS);

/* SDL video device functions for fullscreen mode */
static int FULLSCREEN_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static int FULLSCREEN_FlipHWSurface(_THIS, SDL_Surface *surface);
static void FULLSCREEN_SetWMCaption(_THIS, const char *title, const char *icon);

/* UpdateRects variants */
static void FULLSCREEN_UpdateRects(_THIS, int numrects, SDL_Rect *rects);
static void FULLSCREEN_UpdateRectsMemCpy(_THIS, int numrects, SDL_Rect *rects);
static void FULLSCREEN_UpdateRects8bpp(_THIS, int numrects, SDL_Rect *rects);
static void FULLSCREEN_UpdateRects16bpp(_THIS, int numrects, SDL_Rect *rects);
static void FULLSCREEN_UpdateRects32bpp(_THIS, int numrects, SDL_Rect *rects);
static void FULLSCREEN_UpdateRectsOS(_THIS, int numrects, SDL_Rect *rects);

/* Local helper functions */
static int cmpmodes(const void *va, const void *vb);
static int FULLSCREEN_AddMode(_THIS, int bpp, int w, int h);
static void FULLSCREEN_SetWriteBank(int bank);
static void FULLSCREEN_SetDisplayBank(int bank);
static void FULLSCREEN_DisableEscape();
static void FULLSCREEN_EnableEscape();

SDL_Surface *FULLSCREEN_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
   _kernel_swi_regs regs;
   int create_back_buffer = riscos_backbuffer;

   const RISCOS_SDL_PixelFormat *fmt = FULLSCREEN_SetMode(width, height, bpp);
   if (fmt == NULL)
   {
	   return (NULL);
   }

/* 	printf("Setting mode %dx%d\n", width, height); */

	/* Allocate the new pixel format for the screen */
	if ( ! SDL_ReallocFormat(current, fmt->sdl_bpp, fmt->rmask, fmt->gmask, fmt->bmask, 0) ) {
		RISCOS_RestoreWimpMode();
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

   if (fmt->sdl_bpp == 8)
   {
      flags |= SDL_HWPALETTE;
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
         ** back buffer to simulate processing on other OSes.
         ** This is turned on by setting the enviromental variable
         ** SDL$<name>$BackBuffer >= 1
         */
       if (riscos_backbuffer == 3) {
          this->hidden->bank[0] = WIMP_CreateBuffer(width, height, &fmt->ro);
       } else {
          this->hidden->bank[0] = SDL_malloc(height * current->pitch);
          if (this->hidden->bank[0] == 0)
             SDL_OutOfMemory();
       }
       if (this->hidden->bank[0] == 0)
       {
           RISCOS_RestoreWimpMode();
           return (NULL);
       }
       /* Surface updated in programs is now a software surface */
       current->flags &= ~SDL_HWSURFACE;
    }

    /* Store address of allocated screen bank to be freed later */
    if (this->hidden->alloc_bank) SDL_free(this->hidden->alloc_bank);
    if (create_back_buffer)
    {
        this->hidden->alloc_bank = this->hidden->bank[0];
        if (riscos_backbuffer == 3)
        {
           this->hidden->bank[0] += 60; /* Start of sprite data */
           if (bpp == 8) this->hidden->bank[0] += 2048; /* 8bpp sprite have palette first */
        }
    } else
	  this->hidden->alloc_bank = 0;

    // Clear both banks to black
    SDL_memset(this->hidden->bank[0], 0, height * current->pitch);
    SDL_memset(this->hidden->bank[1], 0, height * current->pitch);

 	   this->hidden->current_bank = 0;
	   current->pixels = this->hidden->bank[0];
	   this->hidden->format = fmt;

    /* Have to set the screen here, so SetDeviceMode will pick it up */
    this->screen = current;

	/* Reset device functions for the wimp */
	FULLSCREEN_SetDeviceMode(this);

/*	FULLSCREEN_DisableEscape(); */

	/* We're done */
	return(current);
}

/* Reset any device functions that have been changed because we have run in WIMP mode */
void FULLSCREEN_SetDeviceMode(_THIS)
{
	/* Update rects is different if we have a backbuffer */

	if (riscos_backbuffer && (this->screen->flags & SDL_DOUBLEBUF) == 0)
      {
	   switch(riscos_backbuffer)
         {
            case 2: /* ARM code full word copy */
               switch(this->screen->format->BytesPerPixel)
               {
                  case 1: /* 8bpp modes */
               	   this->UpdateRects = FULLSCREEN_UpdateRects8bpp;
                     break;
                  case 2: /* 15/16bpp modes */
               	   this->UpdateRects = FULLSCREEN_UpdateRects16bpp;
                     break;
                  case 4: /* 32 bpp modes */
               	   this->UpdateRects = FULLSCREEN_UpdateRects32bpp;
                     break;

                  default: /* Just default to the memcpy routine */
               	   this->UpdateRects = FULLSCREEN_UpdateRectsMemCpy;
                     break;
                }
               break;

            case 3: /* Use OS sprite plot routine */
               this->UpdateRects = FULLSCREEN_UpdateRectsOS;
               break;

            default: /* Old but safe memcpy */
               this->UpdateRects = FULLSCREEN_UpdateRectsMemCpy;
               break;
         }
      } else
	   this->UpdateRects = FULLSCREEN_UpdateRects; /* Default do nothing implementation */

	this->SetColors   = FULLSCREEN_SetColors;

	this->FlipHWSurface = FULLSCREEN_FlipHWSurface;

	this->SetCaption = FULLSCREEN_SetWMCaption;
	this->SetIcon = NULL;
	this->IconifyWindow = NULL;
	
	this->ShowWMCursor = RISCOS_ShowWMCursor;
	this->WarpWMCursor = FULLSCREEN_WarpWMCursor;

	this->PumpEvents = FULLSCREEN_PumpEvents;	
}

static void FULLSCREEN_CheckModeListEntry(_THIS, const int *blockInfo)
{
	if ((blockInfo[1] & 255) == 1) /* Old format mode entry */
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
	else if ((blockInfo[1] & 255) == 3) /* Newer format mode entry */
	{
		/* Check it's RGB colourspace */
		if ((blockInfo[5] & 0x3000) != 0)
		{
			return;
		}

		switch(blockInfo[6])
		{
		case 3: /* 8 bits per pixel */
			FULLSCREEN_AddMode(this, 8, blockInfo[2], blockInfo[3]);
			break;
		case 4: /* 12, 15 or 16 bits per pixel */
			FULLSCREEN_AddMode(this, 15, blockInfo[2], blockInfo[3]);
			break;
		case 5: /* 32 bits per pixel */
			FULLSCREEN_AddMode(this, 32, blockInfo[2], blockInfo[3]);
			break;
		}
	}
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

	enumInfo = (char *)SDL_malloc(-regs.r[7]);
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
		FULLSCREEN_CheckModeListEntry(this, blockInfo);

		enum_ptr += blockInfo[0];
	}

	SDL_free(enumInfo);
		
	/* Sort the mode lists */
	for ( j=0; j<NUM_MODELISTS; ++j ) {
		if ( SDL_nummodes[j] > 0 ) {
			SDL_qsort(SDL_modelist[j], SDL_nummodes[j], sizeof *SDL_modelist[j], cmpmodes);
		}
	}
}

static int FULLSCREEN_FlipHWSurface(_THIS, SDL_Surface *surface)
{
   FULLSCREEN_SetDisplayBank(this->hidden->current_bank);
   this->hidden->current_bank ^= 1;
   FULLSCREEN_SetWriteBank(this->hidden->current_bank);
   surface->pixels = this->hidden->bank[this->hidden->current_bank];

   /* Wait for Vsync */
   _kernel_osbyte(19, 0, 0);

	return(0);
}

/* Nothing to do if we are writing direct to hardware */
static void FULLSCREEN_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
}

/* Safe but slower Memory copy from our allocated back buffer */
static void FULLSCREEN_UpdateRectsMemCpy(_THIS, int numrects, SDL_Rect *rects)
{
      int j;
      unsigned char *to, *from;
      int pitch = this->screen->pitch;
      int row;
      int xmult = this->screen->format->BytesPerPixel;
      for (j = 0; j < numrects; j++)
      {
         from = this->hidden->bank[0] + rects->x * xmult + rects->y * pitch;
         to  = this->hidden->bank[1] + rects->x * xmult + rects->y * pitch;
         for (row = 0; row < rects->h; row++)
         {
             SDL_memcpy(to, from, rects->w * xmult);
             from += pitch;
             to += pitch;
         }
         rects++;
      }
}

/* Use optimized assembler memory copy. Deliberately copies extra columns if
   necessary to ensure the rectangle is word aligned. */
static void FULLSCREEN_UpdateRects8bpp(_THIS, int numrects, SDL_Rect *rects)
{
   int j;
   unsigned char *to, *from;
   int pitch = this->screen->pitch;
   int width_bytes;
   int src_skip_bytes;

   for (j = 0; j < numrects; j++)
   {
      from = this->hidden->bank[0] + rects->x + rects->y * pitch;
      to  = this->hidden->bank[1] + rects->x + rects->y * pitch;
      width_bytes = rects->w;
      if ((int)from & 3)
      {
         int extra = ((int)from & 3);
         from -= extra;
         to -= extra;
         width_bytes += extra;
      }
      if (width_bytes & 3) width_bytes += 4 - (width_bytes & 3);
      src_skip_bytes = pitch - width_bytes;
               
      RISCOS_Put32(to, (width_bytes >> 2), pitch, (int)rects->h, from, src_skip_bytes);
      rects++;
   }
}

/* Use optimized assembler memory copy. Deliberately copies extra columns if
   necessary to ensure the rectangle is word aligned. */
static void FULLSCREEN_UpdateRects16bpp(_THIS, int numrects, SDL_Rect *rects)
{
   int j;
   unsigned char *to, *from;
   int pitch = this->screen->pitch;
   int width_bytes;
   int src_skip_bytes;

   for (j = 0; j < numrects; j++)
   {
      from = this->hidden->bank[0] + (rects->x << 1) + rects->y * pitch;
      to  = this->hidden->bank[1] + (rects->x << 1) + rects->y * pitch;
      width_bytes = (((int)rects->w) << 1);
      if ((int)from & 3)
      {
         from -= 2;
         to -= 2;
         width_bytes += 2;
      }
      if (width_bytes & 3) width_bytes += 2;
      src_skip_bytes = pitch - width_bytes;
               
      RISCOS_Put32(to, (width_bytes >> 2), pitch, (int)rects->h, from, src_skip_bytes);
      rects++;
   }
}

/* Use optimized assembler memory copy. 32 bpp modes are always word aligned */
static void FULLSCREEN_UpdateRects32bpp(_THIS, int numrects, SDL_Rect *rects)
{
   int j;
   unsigned char *to, *from;
   int pitch = this->screen->pitch;
   int width;

   for (j = 0; j < numrects; j++)
   {
      from = this->hidden->bank[0] + (rects->x << 2) + rects->y * pitch;
      to  = this->hidden->bank[1] + (rects->x << 2) + rects->y * pitch;
      width = (int)rects->w ;
               
      RISCOS_Put32(to, width, pitch, (int)rects->h, from, pitch - (width << 2));
      rects++;
   }
}

/* Use operating system sprite plots. Currently this is much slower than the
   other variants however accelerated sprite plotting can be seen on the horizon
   so this prepares for it. */
static void FULLSCREEN_UpdateRectsOS(_THIS, int numrects, SDL_Rect *rects)
{
   _kernel_swi_regs regs;
   _kernel_oserror *err;
   int j;
   int y;

   regs.r[0] = 28 + 512;
   regs.r[1] = (unsigned int)this->hidden->alloc_bank;
   regs.r[2] = (unsigned int)this->hidden->alloc_bank+16;
   regs.r[5] = 0;

   for (j = 0; j < numrects; j++)
   {
      y = this->screen->h - rects->y; /* top of clipping region */
      _kernel_oswrch(24); /* Set graphics clip region */
      _kernel_oswrch((rects->x << this->hidden->xeig) & 0xFF); /* left */
      _kernel_oswrch(((rects->x << this->hidden->xeig) >> 8) & 0xFF);
      _kernel_oswrch(((y - rects->h) << this->hidden->yeig) & 0xFF); /* bottom */
      _kernel_oswrch((((y - rects->h) << this->hidden->yeig)>> 8) & 0xFF);
      _kernel_oswrch(((rects->x + rects->w - 1) << this->hidden->xeig) & 0xFF); /* right */
      _kernel_oswrch((((rects->x + rects->w - 1)<< this->hidden->xeig) >> 8) & 0xFF);
      _kernel_oswrch(((y-1) << this->hidden->yeig) & 0xFF); /* top */
      _kernel_oswrch((((y-1) << this->hidden->yeig) >> 8) & 0xFF);

      regs.r[3] = 0;
      regs.r[4] = 0;

      if ((err = _kernel_swi(OS_SpriteOp, &regs, &regs)) != 0)
      {
         printf("OS_SpriteOp failed \n%s\n",err->errmess);
      }

      rects++;

      /* Reset to full screen clipping */
      _kernel_oswrch(24); /* Set graphics clip region */
      _kernel_oswrch(0); /* left */
      _kernel_oswrch(0);
      _kernel_oswrch(0); /* bottom */
      _kernel_oswrch(0);
      _kernel_oswrch(((this->screen->w-1) << this->hidden->xeig) & 0xFF); /* right */
      _kernel_oswrch((((this->screen->w-1) << this->hidden->xeig) >> 8) & 0xFF);
      _kernel_oswrch(((this->screen->h-1) << this->hidden->yeig) & 0xFF); /* top */
      _kernel_oswrch((((this->screen->h-1) << this->hidden->yeig) >> 8) & 0xFF);
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
    if(a->w == b->w)
        return b->h - a->h;
    else
        return b->w - a->w;
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
	mode = (SDL_Rect *)SDL_malloc(sizeof *mode);
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
	       SDL_realloc(SDL_modelist[index], (1+next_mode+1)*sizeof(SDL_Rect *));
	if ( SDL_modelist[index] == NULL ) {
		SDL_OutOfMemory();
		SDL_nummodes[index] = 0;
		SDL_free(mode);
		return(-1);
	}
	SDL_modelist[index][next_mode] = mode;
	SDL_modelist[index][next_mode+1] = NULL;
	SDL_nummodes[index]++;

	return(0);
}

void FULLSCREEN_SetWriteBank(int bank)
{
	_kernel_osbyte(112, bank+1, 0);
}

void FULLSCREEN_SetDisplayBank(int bank)
{
	_kernel_osbyte(113, bank+1, 0);
}


/** Disable special escape key processing */
static void FULLSCREEN_DisableEscape()
{
	_kernel_osbyte(229, 1, 0);
}

/** Enable special escape key processing */
static void FULLSCREEN_EnableEscape()
{
	_kernel_osbyte(229, 0, 0);
}

/** Store caption in case this is called before we create a window */
void FULLSCREEN_SetWMCaption(_THIS, const char *title, const char *icon)
{
	SDL_strlcpy(this->hidden->title, title, SDL_arraysize(this->hidden->title));
}

/* Try and set the exact screen mode specified */
static int FULLSCREEN_TrySetMode(int width,int height,const RISCOS_PixelFormat *fmt)
{
   SCREENMODEBLOCK smb;
   _kernel_swi_regs regs;

   /* To cope with dodgy validation by the OS or video drivers, check that the OS understands the corresponding sprite format */
   if (WIMP_IsSupportedSpriteFormat(fmt))
   {
      return -1;
   }

   smb.flags = 1;
   smb.x_pixels = width;
   smb.y_pixels = height;
   smb.pixel_depth = fmt->log2bpp;
   smb.frame_rate = -1;
   smb.mode_vars[0] = 3; /* NColour */
   smb.mode_vars[1] = fmt->ncolour;
   smb.mode_vars[2] = 0; /* ModeFlags */
   smb.mode_vars[3] = fmt->modeflags;
   smb.mode_vars[4] = -1;

   regs.r[0] = 0;
   regs.r[1] = (int)&smb;

   if (_kernel_swi(OS_ScreenMode, &regs, &regs) == 0)
   {
      /* Turn cursor off*/
      _kernel_oswrch(23);_kernel_oswrch(1);_kernel_oswrch(0);
      _kernel_oswrch(0);_kernel_oswrch(0);_kernel_oswrch(0);
      _kernel_oswrch(0);_kernel_oswrch(0);_kernel_oswrch(0);
      _kernel_oswrch(0);_kernel_oswrch(0);

      return 0;
   }
   
   return -1;
}

/* Set screen mode
*
*  Returns ptr to pixel format if mode is set ok, otherwise 0
*/

const RISCOS_SDL_PixelFormat *FULLSCREEN_SetMode(int width, int height, int bpp)
{
   int pass;
   const RISCOS_SDL_PixelFormat *fmt;

   for(pass=0;pass<2;pass++)
   {
      for (fmt = RISCOS_SDL_PixelFormats;fmt->sdl_bpp;fmt++)
      {
         if (fmt->sdl_bpp != bpp)
         {
            continue;
         }
         
         if (!FULLSCREEN_TrySetMode(width,height,&fmt->ro))
         {
            return fmt;
         }
      }

      /* Fuzzy matching for 15bpp & 16bpp */
      if (bpp == 15) { bpp = 16; }
      else if (bpp == 16) { bpp = 15; }
      else { break; }
   }

   SDL_SetError("Couldn't set requested mode");
   return NULL;
}

/* Get Start addresses for the screen banks */
void FULLSCREEN_SetupBanks(_THIS)
{
   _kernel_swi_regs regs;
   int block[7];
   block[0] = 148; /* Write screen start */
   block[1] = 149; /* Display screen start */
   block[2] = 4;   /* X eig factor */
   block[3] = 5;   /* Y eig factor */
   block[4] = 11;  /* Screen Width - 1 */
   block[5] = 12;  /* Screen Height - 1 */
   block[6] = -1;  /* End of list of variables to request */

   regs.r[0] = (int)block;
   regs.r[1] = (int)block;
   _kernel_swi(OS_ReadVduVariables, &regs, &regs);

   this->hidden->bank[0] = (void *)block[0];
   this->hidden->bank[1] = (void *)block[1];
   this->hidden->xeig = block[2];
   this->hidden->yeig = block[3];
   this->hidden->screen_width = block[4];
   this->hidden->screen_height = block[5];
}

/* Toggle to full screen mode from the WIMP */

int FULLSCREEN_ToggleFromWimp(_THIS)
{
   int width = this->screen->w;
   int height = this->screen->h;
   int bpp = this->screen->format->BitsPerPixel;

   RISCOS_StoreWimpMode();
   if (!FULLSCREEN_TrySetMode(width, height, &this->hidden->format->ro))
   {
       unsigned char *buffer = this->hidden->alloc_bank; /* This is start of sprite data */
       /* Support back buffer mode only */
       if (riscos_backbuffer == 0) riscos_backbuffer = 1;

       FULLSCREEN_SetupBanks(this);

       this->hidden->bank[0] = buffer + 60; /* Start of sprite data */
       if (bpp == 8)
       {
           /* Retain the SDL palette */
           _kernel_swi_regs regs;
           regs.r[0] = -1;
           regs.r[1] = -1;
           regs.r[2] = (int) this->hidden->bank[0];
           regs.r[3] = 0;
           regs.r[4] = 2; /* Flashing colours provided (source is sprite palette) */
           _kernel_swi(ColourTrans_WritePalette,&regs,&regs);

           this->hidden->bank[0] += 2048;
       }

	   this->hidden->current_bank = 0;
	   this->screen->pixels = this->hidden->bank[0];

       /* Copy back buffer to screen memory */
       SDL_memcpy(this->hidden->bank[1], this->hidden->bank[0], width * height * this->screen->format->BytesPerPixel);

       FULLSCREEN_SetDeviceMode(this);
       return 1;
   } else
      RISCOS_RestoreWimpMode();

   return 0;
}
