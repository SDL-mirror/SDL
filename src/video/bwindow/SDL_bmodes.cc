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

#if SDL_VIDEO_OPENGL
#include "SDL_bopengl.h"
#endif

#include "../../main/beos/SDL_BApp.h"

#ifdef __cplusplus
extern "C" {
#endif


#define WRAP_BMODE 1

#if WRAP_BMODE
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
#if WRAP_BMODE
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














static int get_combine_mode(display_mode &mode) {
	if ((mode.flags & B_SCROLL) == 0)
		return 0;

	if (mode.virtual_width == mode.timing.h_display * 2)
		return 1;

	if (mode.virtual_height == mode.timing.v_display * 2)
		return 2;
     
		return 0;
}
#if 0
bool _GetDisplayMode(const screen_mode& mode, display_mode& displayMode)
{
 	uint16 virtualWidth, virtualHeight;
 	int32 bestIndex = -1;
 	float bestDiff = 999;
 
 	virtualWidth = mode.combine == kCombineHorizontally
 		? mode.width * 2 : mode.width;
 	virtualHeight = mode.combine == kCombineVertically
 		? mode.height * 2 : mode.height;
 
 	// try to find mode in list provided by driver
 	for (uint32 i = 0; i < fModeCount; i++) {
 		if (fModeList[i].virtual_width != virtualWidth
 			|| fModeList[i].virtual_height != virtualHeight
 			|| (color_space)fModeList[i].space != mode.space)
 			continue;
 
 		// Accept the mode if the computed refresh rate of the mode is within
 		// 0.6 percent of the refresh rate specified by the caller.  Note that
 		// refresh rates computed from mode parameters is not exact; especially
 		// some of the older modes such as 640x480, 800x600, and 1024x768.
 		// The tolerance of 0.6% was obtained by examining the various possible
 		// modes.

 		float refreshDiff = fabs(get_refresh_rate(fModeList[i]) - mode.refresh);
 		if (refreshDiff < 0.006 * mode.refresh) {
 			// Accept this mode.
 			displayMode = fModeList[i];
 			displayMode.h_display_start = 0;
 			displayMode.v_display_start = 0;
 
			// Since the computed refresh rate of the selected mode might differ
 			// from selected refresh rate by a few tenths (e.g. 60.2 instead of
 			// 60.0), tweak the pixel clock so the the refresh rate of the mode
 			// matches the selected refresh rate.
 
 			displayMode.timing.pixel_clock = uint32(((displayMode.timing.h_total
 				* displayMode.timing.v_total * mode.refresh) / 1000.0) + 0.5);
 			return true;
 		}
 
		// Mode not acceptable.
 
 		if (refreshDiff < bestDiff) {
 			bestDiff = refreshDiff;
 			bestIndex = i;
		}
	}
 
 	// we didn't find the exact mode, but something very similar?
 	if (bestIndex == -1)
 		return false;
 
 	displayMode = fModeList[bestIndex];
 	displayMode.h_display_start = 0;
 	displayMode.v_display_start = 0;
 
 	// For the mode selected by the width, height, and refresh rate, compute
 	// the video timing parameters for the mode by using the VESA Generalized
 	// Timing Formula (GTF).
 
	ComputeGTFVideoTiming(displayMode.timing.h_display,
 		displayMode.timing.v_display, mode.refresh, displayMode.timing);

	return true;
}
#endif

void _SpoutModeData(display_mode *bmode) {
	printf("BMode:\n");
	printf("\tw,h = (%i,%i)\n", bmode->virtual_width, bmode->virtual_height);
	printf("\th,v = (%i,%i)\n", bmode->h_display_start, 
			bmode->v_display_start);
	printf("\tcombine mode = %i\n", get_combine_mode(*bmode));
	if(bmode->flags) {
		printf("\tFlags:\n");
		if(bmode->flags & B_SCROLL) {
			printf("\t\tB_SCROLL\n");
		}
		if(bmode->flags & B_8_BIT_DAC) {
			printf("\t\tB_8_BIT_DAC\n");
		}
		if(bmode->flags & B_HARDWARE_CURSOR) {
			printf("\t\tB_HARDWARE_CURSOR\n");
		}
		if(bmode->flags & B_PARALLEL_ACCESS) {
			printf("\t\tB_PARALLEL_ACCESS\n");
		}
		if(bmode->flags & B_DPMS) {
			printf("\t\tB_DPMS\n");
		}
		if(bmode->flags & B_IO_FB_NA) {
			printf("\t\tB_IO_FB_NA\n");
		}
	}
	printf("\tTiming:\n");
	printf("\t\tpx clock: %i\n", bmode->timing.pixel_clock);
	printf("\t\th - display: %i sync start: %i sync end: %i total: %i\n",
		bmode->timing.h_display, bmode->timing.h_sync_start,
		bmode->timing.h_sync_end, bmode->timing.h_total);
	printf("\t\tv - display: %i sync start: %i sync end: %i total: %i\n",
		bmode->timing.v_display, bmode->timing.v_sync_start,
		bmode->timing.v_sync_end, bmode->timing.v_total);
	if(bmode->timing.flags) {
		printf("\t\tFlags:\n");
		if(bmode->timing.flags & B_BLANK_PEDESTAL) {
			printf("\t\t\tB_BLANK_PEDESTAL\n");
		}
		if(bmode->timing.flags & B_TIMING_INTERLACED) {
			printf("\t\t\tB_TIMING_INTERLACED\n");
		}
		if(bmode->timing.flags & B_POSITIVE_HSYNC) {
			printf("\t\t\tB_POSITIVE_HSYNC\n");
		}
		if(bmode->timing.flags & B_POSITIVE_VSYNC) {
			printf("\t\t\tB_POSITIVE_VSYNC\n");
		}
		if(bmode->timing.flags & B_SYNC_ON_GREEN) {
			printf("\t\t\tB_SYNC_ON_GREEN\n");
		}
	}
}

int BE_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode){
	/* Get the current screen */
printf(__FILE__": %d\n", __LINE__);
	BScreen bscreen;
	if(!bscreen.IsValid()) {
		printf(__FILE__": %d - ERROR: BAD SCREEN\n", __LINE__);
	}

	/* Set the mode using the driver data */
	display_mode *bmode = _ExtractBMode(mode);


_SpoutModeData(bmode);
printf("\n");
uint32 c = 0, i, x = 0;
#define RES 0
display_mode *bmode_list;
bscreen.GetModeList(&bmode_list, &c);
for(i = 0; i < c; ++i) {
	if(	bmode_list[i].space == bmode->space &&
		bmode_list[i].virtual_width == bmode->virtual_width &&
		bmode_list[i].virtual_height == bmode->virtual_height ) {
			if(x == RES)
				bmode = &bmode_list[i];
			
			++x;
			_SpoutModeData(&bmode_list[i]);
//			break;
	}
}

	if(bscreen.SetMode(bmode) != B_OK) {
		SDL_SetError("Bad video mode\n");
		return -1;
	}
free(bmode_list);
	
#if SDL_VIDEO_OPENGL
	/* The OpenGL context may need to be rebooted */
//	BE_GL_RebootContexts(_this);
#endif

printf(__FILE__": %d\n", __LINE__);
	return 0;
}

#ifdef __cplusplus
}
#endif
