/*
	SDL - Simple DirectMedia Layer
	Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#include "SDL_types.h"
#include "SDL_video.h"
#include "SDL_blit.h"
#include "SDL_fbmatrox.h"
#include "matrox_mmio.h"


static int LockHWSurface(_THIS, SDL_Surface *surface)
{
	if ( surface == SDL_VideoSurface ) {
		mga_waitidle();
	}
	return(0);
}
static void UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

/* Wait for vertical retrace - taken from the XFree86 Matrox driver */
static void WaitVBL(_THIS)
{
	int count;

	/* find start of retrace */
	mga_waitidle();
	while (  (mga_in8(0x1FDA) & 0x08) )
		;
	while ( !(mga_in8(0x1FDA) & 0x08) )
		; 
	/* wait until we're past the start */
	count = mga_in32(0x1E20) + 2;
	while ( mga_in32(0x1E20) < count )
		;
}

/* Sets video mem colorkey and accelerated blit function */
static int SetHWColorKey(_THIS, SDL_Surface *surface, Uint32 key)
{
	return(0);
}

/* Sets per surface hardware alpha value */
#if 0
static int SetHWAlpha(_THIS, SDL_Surface *surface, Uint8 value)
{
	return(0);
}
#endif

static int FillHWRect(_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color)
{
	int dstX, dstY;
	Uint32 fxbndry;
	Uint32 ydstlen;
	Uint32 fillop;

	switch (dst->format->BytesPerPixel) {
	    case 1:
		color |= (color<<8);
	    case 2:
		color |= (color<<16);
		break;
	}

	/* Set up the X/Y base coordinates */
	dstX = 0;
	dstY = ((char *)dst->pixels - mapped_mem) / SDL_VideoSurface->pitch;

	/* Adjust for the current rectangle */
	dstX += rect->x;
	dstY += rect->y;

	/* Set up the X boundaries */
	fxbndry = (dstX | ((dstX+rect->w) << 16));

	/* Set up the Y boundaries */
	ydstlen = (rect->h | (dstY << 16));

#if 0	/* This old way doesn't work on the Matrox G450 */
	/* Set up for color fill operation */
	fillop = MGADWG_TRAP | MGADWG_SOLID |
	         MGADWG_ARZERO | MGADWG_SGNZERO | MGADWG_SHIFTZERO |
	         MGADWG_BFCOL | MGADWG_BLK;

	/* Execute the operations! */
	mga_wait(4);
	mga_out32(MGAREG_FCOL, color);
	mga_out32(MGAREG_FXBNDRY, fxbndry);
	mga_out32(MGAREG_YDSTLEN, ydstlen);
	mga_out32(MGAREG_DWGCTL + MGAREG_EXEC, fillop);
#else
	/* Set up for color fill operation */
	fillop = MGADWG_TRAP | MGADWG_SOLID |
	         MGADWG_ARZERO | MGADWG_SGNZERO | MGADWG_SHIFTZERO;

	/* Execute the operations! */
	mga_wait(5);
	mga_out32(MGAREG_DWGCTL, fillop | MGADWG_REPLACE);
	mga_out32(MGAREG_FCOL, color);
	mga_out32(MGAREG_FXBNDRY, fxbndry);
	mga_out32(MGAREG_YDSTLEN + MGAREG_EXEC, ydstlen);
#endif

	return(0);
}

static int HWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect,
                       SDL_Surface *dst, SDL_Rect *dstrect)
{
	SDL_VideoDevice *this;
	int bpp;
	int srcX, srcY;
	int dstX, dstY;
	Uint32 sign;
	Uint32 sstart, sstop;
	int sskip;
	Uint32 blitop;

	/* FIXME: For now, only blit to display surface */
	if ( dst->pitch != SDL_VideoSurface->pitch ) {
		return(src->map->sw_blit(src, srcrect, dst, dstrect));
	}

	/* Calculate source and destination base coordinates (in pixels) */
	this = current_video;
	srcX= 0;	/* FIXME: Calculate this from memory offset */
	srcY = ((char *)src->pixels - mapped_mem) / SDL_VideoSurface->pitch;
	dstX = 0;	/* FIXME: Calculate this from memory offset */
	dstY = ((char *)dst->pixels - mapped_mem) / SDL_VideoSurface->pitch;

	/* Adjust for the current blit rectangles */
	srcX += srcrect->x;
	srcY += srcrect->y;
	dstX += dstrect->x;
	dstY += dstrect->y;

	/* Set up the blit direction (sign) flags */
	sign = 0;
	if ( srcX < dstX ) {
		sign |= 1;
	}
	if ( srcY < dstY ) {
		sign |= 4;
	}

	/* Set up the blit source row start, end, and skip (in pixels) */
	bpp = src->format->BytesPerPixel;
	sstop = sstart = ((srcY * SDL_VideoSurface->pitch)/bpp) + srcX;
	if ( srcX < dstX ) {
		sstart += (dstrect->w - 1);
	} else {
		sstop += (dstrect->w - 1);
	}
	sskip = src->pitch/bpp;
	if ( srcY < dstY ) {
		sskip = -sskip;
	}

	/* Set up the blit operation */
	if ( (src->flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY ) {
		Uint32 colorkey;

		blitop = MGADWG_BFCOL | MGADWG_BITBLT |
		         MGADWG_SHIFTZERO | MGADWG_RSTR | (0x0C << 16) |
		         MGADWG_TRANSC;

		colorkey = src->format->colorkey;
		switch (dst->format->BytesPerPixel) {
		    case 1:
			colorkey |= (colorkey<<8);
		    case 2:
			colorkey |= (colorkey<<16);
			break;
		}
		mga_wait(2);
		mga_out32(MGAREG_FCOL, colorkey);
		mga_out32(MGAREG_BCOL, 0xFFFFFFFF);
	} else {
		blitop = MGADWG_BFCOL | MGADWG_BITBLT |
		         MGADWG_SHIFTZERO | MGADWG_RSTR | (0x0C << 16);
	}
	mga_wait(7);
	mga_out32(MGAREG_SGN, sign);
	mga_out32(MGAREG_AR3, sstart);
	mga_out32(MGAREG_AR0, sstop);
	mga_out32(MGAREG_AR5, sskip);
	mga_out32(MGAREG_FXBNDRY, (dstX | ((dstX + dstrect->w-1) << 16)));
	mga_out32(MGAREG_YDSTLEN, (dstY << 16) | dstrect->h);
	mga_out32(MGAREG_DWGCTL + MGAREG_EXEC, blitop);

	return(0);
}

static int CheckHWBlit(_THIS, SDL_Surface *src, SDL_Surface *dst)
{
	int accelerated;

	/* Set initial acceleration on */
	src->flags |= SDL_HWACCEL;

	/* Set the surface attributes */
	if ( (src->flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
		if ( ! this->info.blit_hw_A ) {
			src->flags &= ~SDL_HWACCEL;
		}
	}
	if ( (src->flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY ) {
		if ( ! this->info.blit_hw_CC ) {
			src->flags &= ~SDL_HWACCEL;
		}
	}

	/* Check to see if final surface blit is accelerated */
	accelerated = !!(src->flags & SDL_HWACCEL);
	if ( accelerated ) {
		src->map->hw_blit = HWAccelBlit;
	}
	return(accelerated);
}

void FB_MatroxAccel(_THIS, __u32 card)
{
	/* We have hardware accelerated surface functions */
	this->CheckHWBlit = CheckHWBlit;
	this->LockHWSurface = LockHWSurface;
	this->UnlockHWSurface = UnlockHWSurface;
	wait_vbl = WaitVBL;

	/* The Matrox has an accelerated color fill */
	this->info.blit_fill = 1;
	this->FillHWRect = FillHWRect;

	/* The Matrox has accelerated normal and colorkey blits. */
	this->info.blit_hw = 1;
	/* The Millenium I appears to do the colorkey test a word
	   at a time, and the transparency is intverted. (?)
	 */
	if ( card != FB_ACCEL_MATROX_MGA2064W ) {
		this->info.blit_hw_CC = 1;
		this->SetHWColorKey = SetHWColorKey;
	}

#if 0 /* Not yet implemented? */
	/* The Matrox G200/G400 has an accelerated alpha blit */
	if ( (card == FB_ACCEL_MATROX_MGAG200)
	  || (card == FB_ACCEL_MATROX_MGAG400)
	) {
		this->info.blit_hw_A = 1;
		this->SetHWAlpha = SetHWAlpha;
	}
#endif
}
