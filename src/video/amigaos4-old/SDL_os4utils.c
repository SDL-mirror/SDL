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

#include "SDL_config.h"

#include "SDL_os4video.h"

//#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

//#define DEBUG_FINDMODE		/* Define me to get verbose output when searching for a screenmode */


#include <libraries/Picasso96.h>
#include <proto/Picasso96API.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include <limits.h>

extern struct GraphicsIFace *SDL_IGraphics;
extern struct P96IFace      *SDL_IP96;

uint32
os4video_PFtoPPF(const SDL_PixelFormat *vf)
{
	if (vf->BitsPerPixel == 8)
		return RGBFB_CLUT;
	if (vf->BitsPerPixel == 24)
	{
		if (vf->Rmask == 0x00FF0000 && vf->Gmask == 0x0000FF00 && vf->Bmask == 0x000000FF)
			return RGBFB_R8G8B8;
		if (vf->Rmask == 0x000000FF && vf->Gmask == 0x0000FF00 && vf->Bmask == 0x00FF0000)
			return RGBFB_B8G8R8;
	}
	else if (vf->BitsPerPixel == 32)
	{
		if (vf->Rmask == 0xFF000000 && vf->Gmask == 0x00FF0000 && vf->Bmask == 0x0000FF00)
			return RGBFB_R8G8B8A8;
		if (vf->Rmask == 0x00FF0000 && vf->Gmask == 0x0000FF00 && vf->Bmask == 0x000000FF)
			return RGBFB_A8R8G8B8;
		if (vf->Bmask == 0x00FF0000 && vf->Gmask == 0x0000FF00 && vf->Rmask == 0x000000FF)
			return RGBFB_A8B8G8R8;
		if (vf->Bmask == 0xFF000000 && vf->Gmask == 0x00FF0000 && vf->Rmask == 0x0000FF00)
			return RGBFB_B8G8R8A8;
	}
	else if (vf->BitsPerPixel == 16)
	{
		if (vf->Rmask == 0xf800 && vf->Gmask == 0x07e0 && vf->Bmask == 0x001f)
			return RGBFB_R5G6B5;
		if (vf->Rmask == 0x7C00 && vf->Gmask == 0x03e0 && vf->Bmask == 0x001f)
			return RGBFB_R5G5B5;
	}
	else if (vf->BitsPerPixel == 15)
	{
		if (vf->Rmask == 0x7C00 && vf->Gmask == 0x03e0 && vf->Bmask == 0x001f)
			return RGBFB_R5G5B5;
	}

	return RGBFB_NONE;
}

BOOL
os4video_PPFtoPF(SDL_PixelFormat *vformat, uint32 p96Format)
{
	vformat->Rmask = vformat->Gmask = vformat->Bmask = vformat->Amask = 0;
	vformat->Rshift = vformat->Gshift = vformat->Bshift = vformat->Ashift = 0;
	vformat->Rloss = vformat->Gloss = vformat->Bloss = vformat->Aloss = 8;

	switch(p96Format)
	{
	case RGBFB_CLUT:
		vformat->BitsPerPixel = 8;
		vformat->BytesPerPixel = 1;
		break;

    case RGBFB_R8G8B8:
		vformat->BitsPerPixel = 24;
		vformat->BytesPerPixel = 3;

		vformat->Rmask = 0x00FF0000;
		vformat->Rshift = 16;
		vformat->Rloss = 0;

		vformat->Gmask = 0x0000FF00;
		vformat->Gshift = 8;
		vformat->Gloss = 0;

		vformat->Bmask = 0x000000FF;
		vformat->Bshift = 0;
		vformat->Bloss = 0;

		break;

    case RGBFB_B8G8R8:
		vformat->BitsPerPixel = 24;
		vformat->BytesPerPixel = 3;

		vformat->Rmask = 0x000000FF;
		vformat->Rshift = 0;
		vformat->Rloss = 0;

		vformat->Gmask = 0x0000FF00;
		vformat->Gshift = 8;
		vformat->Gloss = 0;

		vformat->Bmask = 0x00FF0000;
		vformat->Bshift = 16;
		vformat->Bloss = 0;
		break;

    case RGBFB_R5G6B5PC:
    case RGBFB_R5G6B5:
		// We handle these equivalent and do swapping elsewhere.
		// PC format cannot be expressed by mask/shift alone
		vformat->BitsPerPixel = 16;
		vformat->BytesPerPixel = 2;

		vformat->Rmask = 0x0000F800;
		vformat->Rshift = 11;
		vformat->Rloss = 3;

		vformat->Gmask = 0x000007E0;
		vformat->Gshift = 5;
		vformat->Gloss = 2;

		vformat->Bmask = 0x0000001F;
		vformat->Bshift = 0;
		vformat->Bloss = 3;
		break;

    case RGBFB_R5G5B5PC:
    case RGBFB_R5G5B5:
		vformat->BitsPerPixel = 15;
		vformat->BytesPerPixel = 2;

		vformat->Rmask = 0x00007C00;
		vformat->Rshift = 10;
		vformat->Rloss = 3;

		vformat->Gmask = 0x000003E0;
		vformat->Gshift = 5;
		vformat->Gloss = 3;

		vformat->Bmask = 0x0000001F;
		vformat->Bshift = 0;
		vformat->Bloss = 3;
		break;

    case RGBFB_B5G6R5PC:
    case RGBFB_B5G5R5PC:
		return FALSE; // L8r

    case RGBFB_A8R8G8B8:
		vformat->BitsPerPixel = 32;
		vformat->BytesPerPixel = 4;

		vformat->Rmask = 0x00FF0000;
		vformat->Rshift = 16;
		vformat->Rloss = 0;

		vformat->Gmask = 0x0000FF00;
		vformat->Gshift = 8;
		vformat->Gloss = 0;

		vformat->Bmask = 0x000000FF;
		vformat->Bshift = 0;
		vformat->Bloss = 0;

#if 0
/* Don't report alpha mask, since P96 can't usefully support it */

		vformat->Amask = 0xFF000000;
		vformat->Ashift = 24;
		vformat->Aloss = 0;
#endif
		break;

    case RGBFB_A8B8G8R8:
		vformat->BitsPerPixel = 32;
		vformat->BytesPerPixel = 4;

		vformat->Rmask = 0x000000FF;
		vformat->Rshift = 0;
		vformat->Rloss = 0;

		vformat->Gmask = 0x0000FF00;
		vformat->Gshift = 8;
		vformat->Gloss = 0;

		vformat->Bmask = 0x00FF0000;
		vformat->Bshift = 16;
		vformat->Bloss = 0;
#if 0
/* Don't report alpha mask, since P96 can't usefully support it */

		vformat->Amask = 0xFF000000;
		vformat->Ashift = 24;
		vformat->Aloss = 0;
#endif
		break;

    case RGBFB_R8G8B8A8:
		vformat->BitsPerPixel = 32;
		vformat->BytesPerPixel = 4;

#if 0
/* Don't report alpha mask, since P96 can't usefully support it */

		vformat->Amask = 0x000000FF;
		vformat->Ashift = 0;
		vformat->Aloss = 0;
#endif

		vformat->Bmask = 0x0000FF00;
		vformat->Bshift = 8;
		vformat->Bloss = 0;

		vformat->Gmask = 0x00FF0000;
		vformat->Gshift = 16;
		vformat->Gloss = 0;

		vformat->Rmask = 0xFF000000;
		vformat->Rshift = 24;
		vformat->Rloss = 0;
		break;

    case RGBFB_B8G8R8A8:
		vformat->BitsPerPixel = 32;
		vformat->BytesPerPixel = 4;

#if 0
/* Don't report alpha mask, since P96 can't usefully support it */

		vformat->Amask = 0x000000FF;
		vformat->Ashift = 0;
		vformat->Aloss = 0;
#endif

		vformat->Rmask = 0x0000FF00;
		vformat->Rshift = 8;
		vformat->Rloss = 0;

		vformat->Gmask = 0x00FF0000;
		vformat->Gshift = 16;
		vformat->Gloss = 0;

		vformat->Bmask = 0xFF000000;
		vformat->Bshift = 24;
		vformat->Bloss = 0;
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

BOOL
os4video_PixelFormatFromModeID(SDL_PixelFormat *vformat, uint32 displayID)
{
	if (SDL_IP96->p96GetModeIDAttr(displayID, P96IDA_ISP96) == FALSE)
		return FALSE;

	return os4video_PPFtoPF(vformat, SDL_IP96->p96GetModeIDAttr(displayID, P96IDA_RGBFORMAT));
}

BOOL
os4video_CmpPixelFormat(const SDL_PixelFormat *f, const SDL_PixelFormat *g)
{
	if (f->BitsPerPixel != g->BitsPerPixel)
		return FALSE;
	if (f->BytesPerPixel != g->BytesPerPixel)
		return FALSE;
	if (f->Amask != g->Amask || f->Rmask != g->Rmask || f->Gmask != g->Gmask || f->Bmask != g->Bmask)
		return FALSE;
	if (f->Ashift != g->Ashift || f->Rshift != g->Rshift || f->Gshift != g->Gshift || f->Bshift != g->Bshift)
		return FALSE;
	if (f->Aloss != g->Aloss || f->Rloss != g->Rloss || f->Gloss != g->Gloss || f->Bloss != g->Bloss)
		return FALSE;

	return TRUE;
}

uint32
os4video_CountModes(const SDL_PixelFormat *format)
{
	uint32
		mode = INVALID_ID,
		mode_count = 0;

	/* TODO: We need to handle systems with muliple monitors -
	 * this should probably count modes for a specific monitor
	 */
    while ((mode = SDL_IGraphics->NextDisplayInfo(mode)) != INVALID_ID)
	{
		if ((mode & 0xFF000000) == 0xFF000000)
		{
			/* I think this means the mode is disabled... */
			continue;
		}

		SDL_PixelFormat g;

		if (FALSE == os4video_PixelFormatFromModeID(&g, mode))
			continue;

		if (FALSE == os4video_CmpPixelFormat(format, &g))
			continue;

		mode_count++;
    }
    return mode_count;
}

uint32
os4video_RTGFB2Bits(uint32 rgbfmt)
{
	switch(rgbfmt)
	{
		case RGBFB_CLUT:
			return 8;

		case RGBFB_R8G8B8:
		case RGBFB_B8G8R8:
			return 24;

		case RGBFB_R5G6B5PC:
		case RGBFB_R5G6B5:
		case RGBFB_B5G6R5PC:
			return 16;

		case RGBFB_B5G5R5PC:
		case RGBFB_R5G5B5PC:
		case RGBFB_R5G5B5:
			return 15;

		case RGBFB_A8R8G8B8:
		case RGBFB_A8B8G8R8:
		case RGBFB_R8G8B8A8:
		case RGBFB_B8G8R8A8:
			return 32;
	}

	return 0;
}


static void
logMode (uint32 mode, uint32 width, uint32 height, uint32 format, const char *message)
{
#ifdef DEBUG_FINDMODE
   IExec->DebugPrintF("  Mode:%08lx (%4d x %4d x %2d : format = %2d) - %s\n",
		      mode, width, height, os4video_RTGFB2Bits(format), format, message);
#endif
}

/*
 * Search display database for a screenmode with desired pixel format
 * and a default size of least <width> x <height> pixels.
 *
 * Todo: better handle cases when there are multiple matching modes.
 * For now we simply pick the last mode in the display database which matches.
 */
static uint32
findModeWithPixelFormat (uint32 width, uint32 height, uint32 pixelFormat, uint32 *outWidth, uint32 *outHeight)
{
	uint32 bestMode   = INVALID_ID;
	uint32 bestWidth  = UINT_MAX;
	uint32 bestHeight = UINT_MAX;

	uint32 mode = INVALID_ID;

	dprintf("Looking for mode %dx%d with pixel format %d\n", width, height, pixelFormat);

	while ((mode = SDL_IGraphics->NextDisplayInfo(mode)) != INVALID_ID)
	{
		uint32 modeFormat;
		uint32 modeWidth;
		uint32 modeHeight;

		if (SDL_IP96->p96GetModeIDAttr(mode,  P96IDA_ISP96) == FALSE)
		{
			dprintf("Skipping non-P96 mode: %08lx\n", mode);
			continue;
		}

		if ((mode & 0xFF000000) == 0xFF000000)
			continue; /* Mode is unavailable? */

		modeWidth  = SDL_IP96->p96GetModeIDAttr(mode, P96IDA_WIDTH);
		modeHeight = SDL_IP96->p96GetModeIDAttr(mode, P96IDA_HEIGHT);
		modeFormat = SDL_IP96->p96GetModeIDAttr(mode, P96IDA_RGBFORMAT);

		if (pixelFormat != modeFormat)
		{
			logMode (mode, modeWidth, modeHeight, modeFormat, "wrong pixel format!");
			continue;
		}

		if (width > modeWidth || height > modeHeight)
		{
			logMode (mode, modeWidth, modeHeight, modeFormat, "too small!");
			continue;
		}

		if (modeWidth > bestWidth || modeHeight > bestHeight)
		{
			logMode (mode, modeWidth, modeHeight, modeFormat, "too big!");
			continue;
		}

		logMode (mode, modeWidth, modeHeight, modeFormat, "okay");

		bestWidth  = modeWidth;
		bestHeight = modeHeight;
		bestMode   = mode;
	}

	if (bestMode != INVALID_ID)
	{
		dprintf("Found mode:%08lx\n", bestMode);

		*outWidth  = bestWidth;
		*outHeight = bestHeight;
	}

	return bestMode;
}

/*
 * Get a list of suitable pixel formats for this bpp sorted by preference.
 * For example, we prefer big-endian modes over little-endian modes
 */
const RGBFTYPE *os4video_GetP96FormatsForBpp (uint32 bpp)
{
	static const RGBFTYPE preferredFmts_8bit[]  = { RGBFB_CLUT, RGBFB_NONE};
	static const RGBFTYPE preferredFmts_15bit[] = { RGBFB_R5G5B5, RGBFB_R5G5B5PC, RGBFB_NONE};
	static const RGBFTYPE preferredFmts_16bit[] = { RGBFB_R5G6B5, RGBFB_R5G6B5PC, RGBFB_NONE};
	static const RGBFTYPE preferredFmts_24bit[] = { RGBFB_R8G8B8, RGBFB_B8G8R8, RGBFB_NONE};
	static const RGBFTYPE preferredFmts_32bit[] = { RGBFB_A8R8G8B8, RGBFB_R8G8B8A8, RGBFB_B8G8R8A8, RGBFB_A8R8G8B8, RGBFB_NONE };
	static const RGBFTYPE unsupportedFmt[]      = { RGBFB_NONE };

	switch (bpp)
	{
		case 8:  return preferredFmts_8bit;
		case 15: return preferredFmts_15bit;
		case 16: return preferredFmts_16bit;
		case 24: return preferredFmts_24bit;
		case 32: return preferredFmts_32bit;
		default: return unsupportedFmt;
	}
}

/*
 * Find a suitable AmigaOS screenmode to use for an SDL
 * screen of the specified width, height, bits-per-pixel
 * and SDL flags.
 *
 * Actually we ignore the flags for now, but it may come
 * in handy later.
 */
uint32
os4video_FindMode(uint32 width, uint32 height, uint32 bpp, uint32 flags)
{
	/* Get a list of p96 formats for this bpp */
	const RGBFTYPE *p96Format = os4video_GetP96FormatsForBpp(bpp);

	uint32 foundWidth = 0;
	uint32 foundHeight = 0;
	uint32 foundMode = INVALID_ID;

	/* Try each p96 format in the list */
	while (*p96Format != RGBFB_NONE)
	{
		/* And see if there is a mode of sufficient size for the p96 format */
		foundMode = findModeWithPixelFormat (width, height, *p96Format, &foundWidth, &foundHeight);

		if (foundMode != INVALID_ID)
		{
			dprintf("Found mode %08lx with height:%d width:%d\n", foundMode, foundWidth, foundHeight);

			return foundMode;
		}

		/* Otherwise try the next format */
		p96Format++;
	}

	return INVALID_ID;
}

/*
 * Insert mode <rect> in <rectArray> queued by reverse order of size
 */
static void queueMode(const SDL_Rect **rectArray, const SDL_Rect *rect, uint32 count)
{
	uint32 i = 0;

	while (i<count)
	{
		if ((rectArray[i]->w <= rect->w) && (rectArray[i]->h <= rect->h))
			break;
		i++;
	}

	if (i < count)
	{
		int j;

		for (j = count - 1; j >= (int)i; j--)
			rectArray[j + 1] = rectArray[j];
	}

	rectArray[i] = (SDL_Rect *) rect;
}

static void
fillModeArray(const SDL_PixelFormat *format, const SDL_Rect **rectArray, SDL_Rect *rects, uint32 maxCount)
{
	uint32
		mode = INVALID_ID,
		width = 0,
		height = 0,
		count = 0;

	/*
	 * SDL wants the mode list in reverse order by mode size, so we queue
	 * the modes in rectArray accordingly since we cannot rely on the ordering
	 * of modes in graphic.lib's database.
	 *
	 * We should really pay attention to other criteria, not just mode size.
	 * In particular, we need a way to predictably handle systems with
	 * multiple montors. The current implementation can't do that.
	 */
	while (maxCount && (mode = SDL_IGraphics->NextDisplayInfo(mode)) != INVALID_ID)
	{
		if ((mode & 0xFF000000) == 0xFF000000)
		{
			/* I think this means the mode is disabled... */
			continue;
		}

		SDL_PixelFormat g;

		if (FALSE == os4video_PixelFormatFromModeID(&g, mode))
			continue;
		if (FALSE == os4video_CmpPixelFormat(format, &g))
			continue;

		width =  SDL_IP96->p96GetModeIDAttr(mode, P96IDA_WIDTH);
		height = SDL_IP96->p96GetModeIDAttr(mode, P96IDA_HEIGHT);

		(*rects).x = 0;
		(*rects).y = 0;
		(*rects).w = (Uint16)width;
		(*rects).h = (Uint16)height;

		queueMode (rectArray, rects, count++);

		dprintf("Added mode %dx%d\n", width, height);
		maxCount--;
		rects++;
	}
}

SDL_Rect **
os4video_MakeResArray(const SDL_PixelFormat *f)
{
	uint32
		allocSize,
		modeCnt;

	SDL_Rect
		*rectBase = NULL,
		**rectPtr = NULL;

	modeCnt = os4video_CountModes(f);

	if(modeCnt) {
		allocSize = (modeCnt+1) * sizeof(SDL_Rect **) + modeCnt * sizeof(SDL_Rect);
		dprintf("%d video modes (allocating a %d array)\n", modeCnt, allocSize);

		rectPtr = (SDL_Rect **)IExec->AllocVecTags(allocSize, AVT_Type, MEMF_SHARED, TAG_DONE);
		if (rectPtr)
		{
			rectBase = (SDL_Rect *)((uint8*)rectPtr + (modeCnt+1)*sizeof(SDL_Rect **));
			fillModeArray(f, (const SDL_Rect **)rectPtr, rectBase, modeCnt);
			rectPtr[modeCnt] = NULL;
		}
	}
	return rectPtr;
}

void *
SaveAllocPooled(struct SDL_PrivateVideoData *hidden, uint32 size)
{
	void *res;

	IExec->ObtainSemaphore(hidden->poolSemaphore);
	res = IExec->AllocPooled(hidden->pool, size);
	IExec->ReleaseSemaphore(hidden->poolSemaphore);

	return res;
}


void *
SaveAllocVecPooled(struct SDL_PrivateVideoData *hidden, uint32 size)
{
	void *res;

	IExec->ObtainSemaphore(hidden->poolSemaphore);
	res = IExec->AllocVecPooled(hidden->pool, size);
	IExec->ReleaseSemaphore(hidden->poolSemaphore);

	return res;
}

void
SaveFreePooled(struct SDL_PrivateVideoData *hidden, void *mem, uint32 size)
{
	IExec->ObtainSemaphore(hidden->poolSemaphore);
	IExec->FreePooled(hidden->pool, mem, size);
	IExec->ReleaseSemaphore(hidden->poolSemaphore);
}

void
SaveFreeVecPooled(struct SDL_PrivateVideoData *hidden, void *mem)
{
	IExec->ObtainSemaphore(hidden->poolSemaphore);
	IExec->FreeVecPooled(hidden->pool, mem);
	IExec->ReleaseSemaphore(hidden->poolSemaphore);
}
