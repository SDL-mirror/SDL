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

static SDL_bool
OS4_GetDisplayMode(_THIS, ULONG id, SDL_DisplayMode * mode)
{
	SDL_DisplayModeData *data;
	APTR handle;
	struct DimensionInfo diminfo;
	struct DisplayInfo dispinfo;

	//dprintf("Called\n");

	handle = IGraphics->FindDisplayInfo(id);
	if (!handle) {
		return SDL_FALSE;
	}

	if (!IGraphics->GetDisplayInfoData(handle, (UBYTE *)&diminfo, sizeof(diminfo), DTAG_DIMS, 0)) {
		dprintf("Failed to get dim info\n");
		return SDL_FALSE;
	}

	if (!IGraphics->GetDisplayInfoData(handle, (UBYTE*)&dispinfo, sizeof(dispinfo), DTAG_DISP, 0)) {
		dprintf("Failed to get disp info\n");
		return SDL_FALSE;
	}

	data = (SDL_DisplayModeData *) SDL_malloc(sizeof(*data));
	if (!data) {
		return SDL_FALSE;
	}

	SDL_zero(*mode);
	data->modeid = id;
	data->x = diminfo.Nominal.MinX;
	data->y = diminfo.Nominal.MinY;
	mode->w = diminfo.Nominal.MaxX - diminfo.Nominal.MinX + 1;
	mode->h = diminfo.Nominal.MaxY - diminfo.Nominal.MinY + 1;
	mode->refresh_rate = 60; // grab DTAG_MNTR?
	mode->format = SDL_PIXELFORMAT_UNKNOWN;

	// We are only interested in RTG modes
	if (dispinfo.PropertyFlags & DIPF_IS_RTG) {

		dprintf("RTG mode %d: w=%d, h=%d, bits=%d\n", id, mode->w, mode->h, diminfo.MaxDepth);

		switch (diminfo.MaxDepth) {
		case 32:
			mode->format = SDL_PIXELFORMAT_RGBA8888; //SDL_PIXELFORMAT_RGB888;
			break;
		case 24:
			mode->format = SDL_PIXELFORMAT_RGB888; // SDL_PIXELFORMAT_RGB24;
			break;
		case 16:
			mode->format = SDL_PIXELFORMAT_RGB565;
			break;
		case 15:
			mode->format = SDL_PIXELFORMAT_RGB555;
			break;
		case 8:
			mode->format = SDL_PIXELFORMAT_INDEX8;
			break;
		}
	}

	mode->driverdata = data;

	return SDL_TRUE;
}

int
OS4_InitModes(_THIS)
{
	SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
	SDL_VideoDisplay display;
	SDL_DisplayMode current_mode;
	SDL_DisplayData *displaydata;
	ULONG modeid;

	dprintf("Called\n");

	displaydata = (SDL_DisplayData *) SDL_malloc(sizeof(*displaydata));
	if (!displaydata) {
		return SDL_OutOfMemory();
	}

	data->publicScreen = IIntuition->LockPubScreen(NULL);
	if (!data->publicScreen) {
		SDL_free(displaydata);
		return SDL_SetError("No displays available");
	}

	IIntuition->GetScreenAttrs(data->publicScreen, SA_DisplayID, &modeid, TAG_DONE);
	if (!OS4_GetDisplayMode(_this, modeid, &current_mode)) {
		dprintf("Failed to get display mode for %d\n", modeid);
		SDL_free(displaydata);
		return SDL_SetError("Couldn't get display mode\n");
	}

	/* OS4 has no multi-monitor support */
	SDL_zero(display);
	display.desktop_mode = current_mode;
	display.current_mode = current_mode;
	display.driverdata = displaydata;
	displaydata->screen = NULL;

	SDL_AddVideoDisplay(&display);

	return 0;
}

int
OS4_GetDisplayBounds(_THIS, SDL_VideoDisplay * display, SDL_Rect * rect)
{
	SDL_DisplayModeData *data = (SDL_DisplayModeData *) display->current_mode.driverdata;

	dprintf("Called\n");

	rect->x = data->x;
	rect->y = data->y;
	rect->w = display->current_mode.w;
	rect->h = display->current_mode.h;

	return 0;
}

void
OS4_GetDisplayModes(_THIS, SDL_VideoDisplay * display)
{
	//SDL_DisplayData *displaydata = (SDL_DisplayData *) display->driverdata;
	SDL_DisplayMode mode;
	ULONG id = INVALID_ID;

	dprintf("Called\n");

	while ((id = IGraphics->NextDisplayInfo(id)) != INVALID_ID) {
		
		if (OS4_GetDisplayMode(_this, id, &mode)) {
			if (mode.format != SDL_PIXELFORMAT_UNKNOWN) {
				if (!SDL_AddDisplayMode(display, &mode)) {
					SDL_free(mode.driverdata);
				}
			} else {
				SDL_free(mode.driverdata);
			}
		} else {
			dprintf("Failed to get display mode for %d\n", id);
		}
	}
}

void
OS4_CloseScreenInternal(_THIS, struct Screen *screen)
{
	if (screen) {
		dprintf("Close screen %p\n", screen);

		if (IIntuition->CloseScreen(screen) == FALSE) {
    		dprintf("Screen has open window(s)\n");
		}

	} else {
		dprintf("NULL pointer\n");
	}
}

int
OS4_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
	SDL_DisplayData *displaydata = (SDL_DisplayData *) display->driverdata;
	SDL_DisplayModeData *data = (SDL_DisplayModeData *) mode->driverdata;
	ULONG openError = 0;
	int bpp = SDL_BITSPERPIXEL(mode->format);

	if (displaydata->screen) {
		OS4_CloseScreenInternal(_this, displaydata->screen);
		displaydata->screen = NULL;
	}

	if (SDL_memcmp(mode, &display->desktop_mode, sizeof(SDL_DisplayMode)) == 0)
	{
		// Don't create another "Workbench"
		dprintf("Desktop mode passed\n");
		
		//TODO: should we check the current display ID and reopen the screen when needed?
		return 0;
	}

	dprintf("Opening screen id %d: %d*%d*%d\n", data->modeid, mode->w, mode->h, bpp);

	displaydata->screen = IIntuition->OpenScreenTags(NULL,
		SA_Width, 		mode->w,
		SA_Height,		mode->h,
		SA_Depth,		bpp,
		SA_DisplayID,	data->modeid,
		SA_Quiet,		TRUE,
		SA_ShowTitle,	FALSE,
		SA_ErrorCode,	&openError,
		TAG_DONE);

	if (!displaydata->screen) {
		switch (openError) {
			case OSERR_NOMONITOR:
				SDL_SetError("Monitor for display mode not available");
				break;
			case OSERR_NOCHIPS:
				SDL_SetError("Newer custom chips required (yeah, sure!)");
				break;
			case OSERR_NOMEM:
			case OSERR_NOCHIPMEM:
				SDL_OutOfMemory();
				break;
			case OSERR_PUBNOTUNIQUE:
				SDL_SetError("Public screen name not unique");
				break;
			case OSERR_UNKNOWNMODE:
			case OSERR_TOODEEP:
				SDL_SetError("Unknown display mode");
				break;
			case OSERR_ATTACHFAIL:
				SDL_SetError("Attachment failed");
				break;
			default:
				SDL_SetError("OpenScreen failed");
		}
		return -1;
	}

	return 0;
}

void
OS4_QuitModes(_THIS)
{
	SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

	dprintf("Called\n");
	
	IIntuition->UnlockPubScreen(NULL, data->publicScreen);
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
