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

static inline SDL_BWin *_ToBeWin(SDL_Window *window) {
	return ((SDL_BWin*)(window->driverdata));
}

static inline SDL_BApp *_GetBeApp() {
	return ((SDL_BApp*)be_app);
}


/* Copied from haiku/trunk/src/preferences/screen/ScreenMode.cpp */
static float get_refresh_rate(display_mode &mode) {
	return rint(10 * float(mode.timing.pixel_clock * 1000)
		/ float(mode.timing.h_total * mode.timing.v_total)) / 10.0;
}

static inline int ColorSpaceToBitsPerPixel(uint32 colorspace)
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

static inline int32 BppToSDLPxFormat(int32 bpp) {
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
	mode->driverdata = bmode;	/* This makes setting display
									   modes easier */

	/* Set the format */
	int32 bpp = ColorSpaceToBitsPerPixel(bmode->space);
	mode->format = BppToSDLPxFormat(bpp);
}

/* Later, there may be more than one monitor available */
void BE_AddDisplay(BScreen *screen) {
	SDL_VideoDisplay display;
	SDL_DisplayMode mode;
	display_mode bmode;
	screen->GetMode(&bmode);

	BE_BDisplayModeToSdlDisplayMode(&bmode, &mode);
	
	SDL_zero(display);
	display.desktop_mode = mode;
	display.current_mode = mode;
	SDL_AddVideoDisplay(&display);
}

int BE_InitModes(_THIS) {
	printf("Init Modes\n");
	BScreen screen;
	
	/* Save the current display mode */
	display_mode *prevMode;
	screen.GetMode(prevMode);
	_GetBeApp()->SetPrevMode(prevMode);

	/* Only one possible video display right now */
	BE_AddDisplay(&screen);
}

int BE_QuitModes(_THIS) {
	/* Restore the previous video mode */
	printf("Quit Modes\n");
	
	BScreen screen;
	display_mode *savedMode = _GetBeApp()->GetPrevMode();
	screen.SetMode(savedMode);
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
	printf("Get Display Modes\n");
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
	printf("Set Display Modes\n");
	/* Get the current screen */
	BScreen bscreen;
	
	/* Set the mode using the driver data */
	display_mode *bmode = (display_mode*)mode->driverdata;
	if(bscreen.SetMode(bmode) == B_OK) {
		return 0;	/* No error */
	}
	
	return -1;
}



int BE_CreateWindowFramebuffer(_THIS, SDL_Window * window,
                                       Uint32 * format,
                                       void ** pixels, int *pitch) {
	SDL_BWin *bwin = _ToBeWin(window);
	BScreen bscreen;
	if(!bscreen.IsValid()) {
		return -1;
	}
	
	while(!bwin->Connected()) { snooze(1600); }

	/* Make sure we have exclusive access to frame buffer data */
	bwin->LockBuffer();

	/* format */
	display_mode bmode;
	bscreen.GetMode(&bmode);
	int32 bpp = ColorSpaceToBitsPerPixel(bmode.space);
	*format = BppToSDLPxFormat(bpp);

	/* pitch = width of screen, in bytes */
	*pitch = bpp * bwin->GetFbWidth() / 8;

	/* Create a copy of the pixel buffer */
	printf("SDL_bmodes.cc: 230; fbh: %i, pitch: %i; (x,y) = (%i, %i)\n", bwin->GetFbHeight(), (*pitch), bwin->GetFbX(), bwin->GetFbY());
	*pixels = SDL_calloc((*pitch) * bwin->GetFbHeight() * bpp / 8, sizeof(uint8));

	bwin->UnlockBuffer();
	return 0;
}



int BE_UpdateWindowFramebuffer(_THIS, SDL_Window * window,
                                      SDL_Rect * rects, int numrects) {
	SDL_BWin *bwin = _ToBeWin(window);
	BScreen bscreen;
	if(!bscreen.IsValid()) {
		return -1;
	}

	if(bwin->ConnectionEnabled() && bwin->Connected()) {
		bwin->LockBuffer();
		int32 windowPitch = window->surface->pitch;
		int32 bufferPitch = bwin->GetRowBytes();
		uint8 *windowpx;
		uint8 *bufferpx;

		int32 bpp = window->surface->format->BitsPerPixel;
		uint8 *windowBaseAddress = (uint8*)window->surface->pixels;
		int32 windowSub = bwin->GetFbX() * bpp / 8 +
						  bwin->GetFbY() * windowPitch;
		clipping_rect *clips = bwin->GetClips();
		int32 numClips = bwin->GetNumClips();
		int i, y;

		/* Blit each clipping rectangle */
		bscreen.WaitForRetrace();
		for(i = 0; i < numClips; ++i) {
			/* Get addresses of the start of each clipping rectangle */
			int32 width = clips[i].right - clips[i].left + 1;
			int32 height = clips[i].bottom - clips[i].top + 1;
			bufferpx = bwin->GetBufferPx() + 
				clips[i].top * bufferPitch + clips[i].left * bpp / 8;
			windowpx = windowBaseAddress + 
				clips[i].top * windowPitch + clips[i].left * bpp / 8
				- windowSub;

			/* Copy each row of pixels from the window buffer into the frame
			   buffer */
			for(y = 0; y < height; ++y)
			{
				memcpy(bufferpx, windowpx, width * bpp / 8);
				bufferpx += bufferPitch;
				windowpx += windowPitch;
			}
		}
		bwin->UnlockBuffer();
	}
	return 0;
}

void BE_DestroyWindowFramebuffer(_THIS, SDL_Window * window) {
	/* FIXME: FINISH! */
	printf("ERROR: Attempted to destroy the window frame buffer\n");
}

#ifdef __cplusplus
}
#endif
