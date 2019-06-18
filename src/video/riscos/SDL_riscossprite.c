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

     Implements Sprite plotting code for wimp display.window
*/

#include "kernel.h"
#include "swis.h"

#include "SDL_stdinc.h"
#include "SDL_riscosvideo.h"

/* Check if the given pixel format is supported as a sprite */
int WIMP_IsSupportedSpriteFormat(const RISCOS_PixelFormat *fmt)
{
    _kernel_swi_regs r;
    int c;

    /* HACK: Mode 28 won't report as having a full palette. Just assume it's supported */
    if (fmt->sprite_mode_word == 28) return 0;

    /* OS_ReadModeVariable supports sprite mode words, so we can use that to sanity check things */
    r.r[0] = fmt->sprite_mode_word;
    r.r[1] = 3; /* NColour */
    _kernel_swi_c(OS_ReadModeVariable,&r,&r,&c);
    if (c || (r.r[2] != fmt->ncolour)) return -1;
    r.r[1] = 0; /* ModeFlags */
    _kernel_swi_c(OS_ReadModeVariable,&r,&r,&c);
    if (c || ((r.r[2] & 0xf280) != fmt->modeflags)) return -1;
    r.r[1] = 9; /* Log2BPP */
    _kernel_swi_c(OS_ReadModeVariable,&r,&r,&c);
    if (c || (r.r[2] != fmt->log2bpp)) return -1;

    /* Looks good */
    return 0;
}

/* Find a supported sprite format in the given BPP */
const RISCOS_SDL_PixelFormat *WIMP_FindSupportedSpriteFormat(int bpp)
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
         
         if (!WIMP_IsSupportedSpriteFormat(&fmt->ro))
         {
            return fmt;
         }
      }

      /* Fuzzy matching for 15bpp & 16bpp */
      if (bpp == 15) { bpp = 16; }
      else if (bpp == 16) { bpp = 15; }
      else { break; }
   }
   return NULL;   
}

/* Create sprite buffer for screen */

unsigned char *WIMP_CreateBuffer(int width, int height, const RISCOS_PixelFormat *format)
{
   int size;
   char sprite_name[12] = "display";
   unsigned char *buffer;
   _kernel_swi_regs regs;
   _kernel_oserror *error;
   int bytesPerPixel = 1 << (format->log2bpp-3);
   int bytesPerRow;
   int offsetToSpriteData = 60;

   if (format->log2bpp == 3)
   {
      offsetToSpriteData += 2048; /* Add in size of palette */
   }

   bytesPerRow = bytesPerPixel * width;

   if ((bytesPerRow & 3) != 0)
   {
      bytesPerRow += 4 - (bytesPerRow & 3);
   }
   size = bytesPerRow * height;

   buffer = SDL_malloc( (size_t) size + offsetToSpriteData );
   if (!buffer) {
      SDL_OutOfMemory();
      return NULL;
   }

   /* Initialise a sprite area */

   *(unsigned int *)buffer         = size + offsetToSpriteData;
   *(unsigned int *)(buffer + 8)   = 16;

   regs.r[0] = 256+9;
   regs.r[1] = (unsigned int)buffer;
   _kernel_swi(OS_SpriteOp, &regs, &regs);

   regs.r[0] = 256+15;
   regs.r[1] = (unsigned int)buffer;
   regs.r[2] = (unsigned int)&sprite_name;
   regs.r[3] = 0; /* Palette flag: 0 = no palette */
   regs.r[4] = width;
   regs.r[5] = height;
   regs.r[6] = format->sprite_mode_word;
   error = _kernel_swi(OS_SpriteOp, &regs, &regs);
   if (error == NULL)
   {
       if (format->log2bpp == 3)
       {
          /* Modify sprite to take into account 256 colour palette */
          int *sprite = (int *)(buffer + 16);
          /* Adjust sprite offsets */
          sprite[0] += 2048;
          sprite[8] += 2048;
          sprite[9] += 2048;
          /* Adjust sprite area next free pointer */
          (*(int *)(buffer+12)) += 2048;

          /* Don't need to set up palette as SDL sets up the default
             256 colour palette */
/*          {
             int *pal = sprite + 11;
             unsigned int j;
             unsigned int entry;
             for (j = 0; j < 255; j++)
             {
                entry = (j << 24) | (j << 16) | (j << 8);
                *pal++ = entry;
                *pal++ = entry;
             }
          }
*/
       }
   } else
   {
      SDL_SetError("Unable to create sprite: %s (%i)", error->errmess, error->errnum);
      SDL_free(buffer);
      buffer = NULL;
   }

   return buffer;
}


/* Setup translation buffers for the sprite plotting */

void WIMP_SetupPlotInfo(_THIS)
{
   _kernel_swi_regs regs;
   int *sprite = ((int *)this->hidden->bank[1])+4;

   regs.r[0] = (unsigned int)this->hidden->bank[1];
   regs.r[1] = (unsigned int)sprite;
   regs.r[2] = -1; /* Current mode */
   regs.r[3] = -1; /* Current palette */
   regs.r[4] = 0; /* Get size of buffer */
   regs.r[5] = 1|2|16; /* R1 - pointer to sprite and can use full palette words */
   regs.r[6] = 0;
   regs.r[7] = 0;

   if (this->hidden->pixtrans) SDL_free(this->hidden->pixtrans);
   this->hidden->pixtrans = 0;

   /* Get the size required for the buffer */
   _kernel_swi(ColourTrans_GenerateTable, &regs, &regs);
   if (regs.r[4])
   {
      this->hidden->pixtrans = SDL_malloc(regs.r[4]);
    
      regs.r[4] = (unsigned int)this->hidden->pixtrans;
      /* Actually read the buffer */
      _kernel_swi(ColourTrans_GenerateTable, &regs, &regs);
   }

   if (this->hidden->scale) SDL_free(this->hidden->scale);
   this->hidden->scale = 0; /* No scale factors i.e. 1:1 */

   if (this->hidden->xeig != 1 || this->hidden->yeig != 1) {
      int eig_mul[4] = { 2, 1, 1, 1 };
      int eig_div[4] = { 1, 1, 2, 4 };

      this->hidden->scale = SDL_malloc(16);
      this->hidden->scale[0] = eig_mul[this->hidden->xeig];
      this->hidden->scale[1] = eig_mul[this->hidden->yeig];
      this->hidden->scale[2] = eig_div[this->hidden->xeig];
      this->hidden->scale[3] = eig_div[this->hidden->yeig];
   }
}

/* Plot the sprite in the given context */
void WIMP_PlotSprite(_THIS, int x, int y)
{
   _kernel_swi_regs regs;
   _kernel_oserror *err;

   regs.r[0] =  52 + 512;
   regs.r[1] = (unsigned int)this->hidden->bank[1];
   regs.r[2] = (unsigned int)this->hidden->bank[1]+16;
   regs.r[3] = x;
   regs.r[4] = y;
   regs.r[5] = 0|32; /* Overwrite screen and pixtrans contains wide colour entries */
   regs.r[6] = (int)this->hidden->scale;
   regs.r[7] = (int)this->hidden->pixtrans;

   if ((err = _kernel_swi(OS_SpriteOp, &regs, &regs)) != 0)
   {
      int *p = (int *)this->hidden->pixtrans;
      printf("OS_SpriteOp failed \n%s\n",err->errmess);
      printf("pixtrans %d\n", (int)this->hidden->pixtrans);
      printf("%x %x %x\n", p[0], p[1], p[2]);
   }
}

