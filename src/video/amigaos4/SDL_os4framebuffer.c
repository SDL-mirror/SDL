/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_AMIGAOS4

#include "SDL_os4video.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

int
OS4_CreateWindowFramebuffer(_THIS, SDL_Window * window, Uint32 * format, void ** pixels, int *pitch)
{
	SDL_WindowData * data = window->driverdata;

	if (data) {
		APTR lock;
		APTR base_address;
		uint32 bytes_per_row;

		if (data->bitmap) {

			dprintf("Freeing old bitmap 0x%p\n", data->bitmap);

			IGraphics->FreeBitMap(data->bitmap);
		}

		data->bitmap = IGraphics->AllocBitMapTags(
			window->w,
			window->h,
			32, // TODO: what we use for the depth and format?
			BMATags_Clear, TRUE,
			BMATags_UserPrivate, TRUE,
			//BMATags_Friend,,
			BMATags_PixelFormat, PIXF_R8G8B8,
			TAG_DONE);

		if (!data->bitmap) {
			return -1;
		}

		*format = SDL_PIXELFORMAT_RGB888;

		/* Lock the bitmap to get details. Since it's user private,
		it should be safe to cache address and pitch. */
		lock = IGraphics->LockBitMapTags(data->bitmap,
			LBM_BaseAddress, &base_address,
			LBM_BytesPerRow, &bytes_per_row,
			TAG_DONE);

		if (lock) {
			*pixels = base_address;
			*pitch = bytes_per_row;

			IGraphics->UnlockBitMap(lock);
		} else {
			dprintf("Failed to lock bitmap\n");

			IGraphics->FreeBitMap(data->bitmap);
			data->bitmap = NULL;
			return -1;
		}
	}
	
	return 0;
}

#ifndef MIN
#   define MIN(x,y) ((x)<(y)?(x):(y))
#endif

int
OS4_UpdateWindowFramebuffer(_THIS, SDL_Window * window, const SDL_Rect * rects, int numrects)
{
	SDL_WindowData * data = window->driverdata;

	if (data && data->bitmap) {
		if (data->syswin) {

			int i;

			const struct IBox windowBox = {
				data->syswin->BorderLeft,
				data->syswin->BorderTop,
				data->syswin->Width, // TODO: is this total width or inner width?
				data->syswin->Height };

			ILayers->LockLayer(0, data->syswin->WLayer);

			for (i = 0; i < numrects; ++i) {
				const SDL_Rect * r = &rects[i];

				int32 ret = IGraphics->BltBitMapTags(
					BLITA_Source, data->bitmap,
					BLITA_Dest, data->syswin->RPort,
					BLITA_DestType, BLITT_RASTPORT,
					BLITA_SrcX, r->x,
					BLITA_SrcY, r->y,
					BLITA_DestX, r->x + windowBox.Left,
					BLITA_DestY, r->y + windowBox.Top,
					BLITA_Width, MIN(r->w, windowBox.Width),
					BLITA_Height, MIN(r->h, windowBox.Height),
					TAG_DONE);
			
				if (ret != -1) {
					dprintf("BltBitMapTags() returned %d\n", ret);
				}
			}

			ILayers->UnlockLayer(data->syswin->WLayer);
		}
	}

	dprintf("called\n");

	return 0;
}

void
OS4_DestroyWindowFramebuffer(_THIS, SDL_Window * window)
{
	SDL_WindowData * data = window->driverdata;

	if (data && data->bitmap) {

		dprintf("Freeing bitmap 0x%p\n", data->bitmap);

		IGraphics->FreeBitMap(data->bitmap);
		data->bitmap = NULL;
	}
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
