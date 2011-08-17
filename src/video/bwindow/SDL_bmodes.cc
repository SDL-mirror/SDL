/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

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



#include <AppKit.h>
#include <InterfaceKit.h>
#include "SDL_bmodes.h"
#include "SDL_BWin.h"

#include "../../main/beos/SDL_BApp.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 1
/* This wrapper is here so that the driverdata can be freed */
typedef struct SDL_DisplayModeData {
	display_mode *bmode;
};
#endif

static inline SDL_BWin *_ToBeWin(SDL_Window *window) {
	return ((SDL_BWin*)(window->driverdata));
}

static inline SDL_BApp *_GetBeApp() {
	return ((SDL_BApp*)be_app);
}

static inline display_mode * _ExtractBMode(SDL_DisplayMode *mode) {
#if 0
	return (display_mode*)(mode->driverdata);
#else
	return ((SDL_DisplayModeData*)mode->driverdata)->bmode;
#endif
}

/* Copied from haiku/trunk/src/preferences/screen/ScreenMode.cpp */
static float get_refresh_rate(display_mode &mode) {
	return float(mode.timing.pixel_clock * 1000)
		/ float(mode.timing.h_total * mode.timing.v_total);
}

static inline int32 ColorSpaceToBitsPerPixel(uint32 colorspace)
{
	int bitsperpixel;

	bitsperpixel = 0;
	switch (colorspace) {
	    case B_CMAP8:
		bitsperpixel = 8;
		break;
	    case B_RGB15:
	    case B_RGBA15:
	    case B_RGB15_BIG:
	    case B_RGBA15_BIG:
		bitsperpixel = 15;
		break;
	    case B_RGB16:
	    case B_RGB16_BIG:
		bitsperpixel = 16;
		break;
	    case B_RGB32:
	    case B_RGBA32:
	    case B_RGB32_BIG:
	    case B_RGBA32_BIG:
		bitsperpixel = 32;
		break;
	    default:
		break;
	}
	return(bitsperpixel);
}

static inline int32 BPPToSDLPxFormat(int32 bpp) {
	/* Translation taken from SDL_windowsmodes.c */
	switch (bpp) {
	case 32:
		return SDL_PIXELFORMAT_RGB888;
		break;
	case 24:	/* May not be supported by Haiku */
		return SDL_PIXELFORMAT_RGB24;
		break;
	case 16:
		return SDL_PIXELFORMAT_RGB565;
		break;
	case 15:
		return SDL_PIXELFORMAT_RGB555;
		break;
	case 8:
		return SDL_PIXELFORMAT_INDEX8;
		break;
	case 4:		/* May not be supported by Haiku */
		return SDL_PIXELFORMAT_INDEX4LSB;
		break;
	}
}

static inline void BE_BDisplayModeToSdlDisplayMode(display_mode *bmode,
		SDL_DisplayMode *mode) {
	mode->w = bmode->virtual_width;
	mode->h = bmode->virtual_height;
	mode->refresh_rate = (int)get_refresh_rate(*bmode);
#if 1
	SDL_DisplayModeData *data = (SDL_DisplayModeData*)SDL_calloc(1, sizeof(SDL_DisplayModeData));
	data->bmode = bmode;
	
	mode->driverdata = data;
#else
	mode->driverdata = bmode;
#endif

	/* Set the format */
	int32 bpp = ColorSpaceToBitsPerPixel(bmode->space);
	mode->format = BPPToSDLPxFormat(bpp);
}

/* Later, there may be more than one monitor available */
void BE_AddDisplay(BScreen *screen) {
	SDL_VideoDisplay display;
	SDL_DisplayMode *mode = (SDL_DisplayMode*)SDL_calloc(1, sizeof(SDL_DisplayMode));
	display_mode *bmode = (display_mode*)SDL_calloc(1, sizeof(display_mode));
	screen->GetMode(bmode);

	BE_BDisplayModeToSdlDisplayMode(bmode, mode);
	
	SDL_zero(display);
	display.desktop_mode = *mode;
	display.current_mode = *mode;
	
	SDL_AddVideoDisplay(&display);
}

int BE_InitModes(_THIS) {
	BScreen screen;
	
	/* Save the current display mode */
//	display_mode *prevMode;
//	screen.GetMode(prevMode);
//	_GetBeApp()->SetPrevMode(prevMode);

	/* Only one possible video display right now */
	BE_AddDisplay(&screen);
}

int BE_QuitModes(_THIS) {
	/* Restore the previous video mode */
	BScreen screen;
//	display_mode *savedMode = _GetBeApp()->GetPrevMode();
//	screen.SetMode(savedMode);
	return 0;
}


int BE_GetDisplayBounds(_THIS, SDL_VideoDisplay *display, SDL_Rect *rect) {
	BScreen bscreen;
	BRect rc = bscreen.Frame();
	rect->x = (int)rc.left;
	rect->y = (int)rc.top;
	rect->w = (int)rc.Width() + 1;
	rect->h = (int)rc.Height() + 1;
	return 0;
}

void BE_GetDisplayModes(_THIS, SDL_VideoDisplay *display) {
	/* Get the current screen */
	BScreen bscreen;

	/* Iterate through all of the modes */
	SDL_DisplayMode mode;
	display_mode this_bmode;
	display_mode *bmodes;
	uint32 count, i;
	
	/* Get graphics-hardware supported modes */
	bscreen.GetModeList(&bmodes, &count);
	bscreen.GetMode(&this_bmode);
	
	for(i = 0; i < count; ++i) {
		//FIXME: Apparently there are errors with colorspace changes
		if (bmodes[i].space == this_bmode.space) {
			BE_BDisplayModeToSdlDisplayMode(&bmodes[i], &mode);
			SDL_AddDisplayMode(display, &mode);
		}
	}
	free(bmodes);
}

int BE_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode){
	/* Get the current screen */
	BScreen bscreen;
	if(!bscreen.IsValid()) {
		printf(__FILE__": %d - ERROR: BAD SCREEN\n", __LINE__);
	}

	/* Set the mode using the driver data */
	display_mode *bmode = _ExtractBMode(mode);

	status_t s;
	if((s = bscreen.SetMode(bmode)) == B_OK) {
		return 0;	/* No error */
	}
printf(__FILE__": %d - ERROR: FAILED TO CHANGE VIDEO MODE; s = %i, status = B_BAD_VALUE? %i\n", __LINE__, s, s == B_BAD_VALUE);
	display_mode *bmode_list;
	uint32 count;
	bscreen.GetModeList(&bmode_list, &count);
	s = bscreen.ProposeMode(bmode, &bmode_list[count - 1], &bmode_list[0]);
	switch(s) {
	case B_OK:
		printf(__FILE__": %d - B_OK\n", __LINE__);
		break;
	case B_BAD_VALUE:
		printf(__FILE__": %d - B_BAD_VALUE\n", __LINE__);
		break;
	case B_ERROR:
		printf(__FILE__": %d - B_ERROR\n", __LINE__);
		break;
	default:
		printf(__FILE__": %d - (unknown error code)\n", __LINE__);
		break;
	}
	free(bmode_list);
	return -1;
}

#ifdef __cplusplus
}
#endif
