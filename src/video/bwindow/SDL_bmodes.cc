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

#include "../SDL_sysvideo.h"

int BE_InitModes(_THIS) {
#if 0
	display_mode *modes;
	uint32 i, nmodes;
	int bpp;

	/* It is important that this be created after SDL_InitBeApp() */
	BScreen bscreen;

	/* Save the current display mode */
	bscreen.GetMode(&saved_mode);
	_this->info.current_w = saved_mode.virtual_width;
	_this->info.current_h = saved_mode.virtual_height;
        
	/* Get the video modes we can switch to in fullscreen mode */
	bscreen.GetModeList(&modes, &nmodes);
	SDL_qsort(modes, nmodes, sizeof *modes, CompareModes);
	for (i = 0; i < nmodes; ++i) {
		bpp = ColorSpaceToBitsPerPixel(modes[i].space);
		//if ( bpp != 0 ) { // There are bugs in changing colorspace
		if (modes[i].space == saved_mode.space) {
			BE_AddMode(_this, ((bpp + 7) / 8) - 1,
				modes[i].virtual_width, modes[i].virtual_height);
		}
	}
#else
return -1;
#endif
}

int BE_QuitModes(_THIS) {
#if 0
    int i, j;
	for (i = 0; i < NUM_MODELISTS; ++i) {
		if (SDL_modelist[i]) {
			for (j = 0; SDL_modelist[i][j]; ++j) {
				SDL_free(SDL_modelist[i][j]);
			}
			SDL_free(SDL_modelist[i]);
			SDL_modelist[i] = NULL;
		}
	}

	/* Restore the original video mode */
	if (_this->screen) {
		if ((_this->screen->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN) {
			BScreen bscreen;
			bscreen.SetMode(&saved_mode);
		}
		_this->screen->pixels = NULL;
	}
#else
return -1;
#endif
}
