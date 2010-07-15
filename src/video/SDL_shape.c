/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 2010 Eli Gottlieb

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Eli Gottlieb
    eligottlieb@gmail.com
*/
#include "SDL_config.h"

#include "SDL.h"
#include "SDL_video.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels.h"
#include "SDL_surface.h"
#include "SDL_shape.h"

SDL_Window* SDL_CreateShapedWindow(const char *title,unsigned int x,unsigned int y,unsigned int w,unsigned int h,Uint32 flags) {
	SDL_Window *result = SDL_CreateWindow(title,x,y,w,h,SDL_WINDOW_BORDERLESS | flags & !SDL_WINDOW_FULLSCREEN & !SDL_WINDOW_SHOWN);
	if(result != NULL) {
		result->shaper = result->display->device->shape_driver.CreateShaper(result);
		if(result->shaper != NULL) {
			result->shaper->usershownflag = flags & SDL_WINDOW_SHOWN;
			result->shaper->alphacutoff = 1;
			result->shaper->hasshape = SDL_FALSE;
			return result;
		}
		else {
			SDL_DestroyWindow(result);
			return NULL;
		}
	}
	else
		return NULL;
}

SDL_bool SDL_IsShapedWindow(const SDL_Window *window) {
	if(window == NULL)
		return SDL_FALSE;
	else
		return (SDL_bool)(window->shaper != NULL);
}

/* REQUIRES that bitmap point to a w-by-h bitmap with 1bpp. */
void SDL_CalculateShapeBitmap(Uint8 alphacutoff,SDL_Surface *shape,Uint8* bitmap,Uint8 ppb,Uint8 value) {
	int x = 0;
	int y = 0;
	Uint8 r = 0,g = 0,b = 0,alpha = 0;
	Uint8* pixel;
	Uint32 bitmap_pixel;
	if(SDL_MUSTLOCK(shape))
		SDL_LockSurface(shape);
	for(x = 0;x<shape->w;x++)
		for(y = 0;y<shape->h;y++) {
			pixel = (Uint8 *)(shape->pixels) + (y*shape->pitch) + (x*shape->format->BytesPerPixel);
			alpha = 0;
			SDL_GetRGBA(*(Uint32*)pixel,shape->format,&r,&g,&b,&alpha);
			bitmap_pixel = y*shape->w + x;
			bitmap[bitmap_pixel / ppb] |= (alpha >= alphacutoff ? value : 0) << ((ppb - 1) - (bitmap_pixel % ppb));
		}
	if(SDL_MUSTLOCK(shape))
		SDL_UnlockSurface(shape);
}

int SDL_SetWindowShape(SDL_Window *window,SDL_Surface *shape,SDL_WindowShapeMode *shapeMode) {
	int result;
	if(window == NULL || !SDL_IsShapedWindow(window))
		//The window given was not a shapeable window.
		return -2;
	if(shape == NULL)
		//Invalid shape argument.
		return -1;
	
	if(shapeMode != NULL) {
		switch(shapeMode->mode) {
			case ShapeModeDefault: {
				window->shaper->alphacutoff = 1;
				break;
			}
			case ShapeModeBinarizeAlpha: {
				window->shaper->alphacutoff = shapeMode->parameters.binarizationCutoff;
				break;
			}
		}
	}
	//TODO: Platform-specific implementations of SetWindowShape.  X11 is finished.  Win32 is in progress.
	result = window->display->device->shape_driver.SetWindowShape(window->shaper,shape,shapeMode);
	window->shaper->hasshape = SDL_TRUE;
	if((window->shaper->usershownflag & SDL_WINDOW_SHOWN) == SDL_WINDOW_SHOWN) {
		SDL_ShowWindow(window);
		window->shaper->usershownflag &= !SDL_WINDOW_SHOWN;
	}
	return result;
}

SDL_bool SDL_WindowHasAShape(SDL_Window *window) {
	if (window == NULL && !SDL_IsShapedWindow(window))
		return SDL_FALSE;
	return window->shaper->hasshape;
}

int SDL_GetShapedWindowMode(SDL_Window *window,SDL_WindowShapeMode *shapeMode) {
	if(window != NULL && SDL_IsShapedWindow(window)) {
		if(shapeMode == NULL) {
			if(SDL_WindowHasAShape(window))
				//The window given has a shape.
				return 0;
			else
				//The window given is shapeable but lacks a shape.
				return -2;
		}
		else {
			if(window->shaper->alphacutoff != 1) {
				shapeMode->mode = ShapeModeBinarizeAlpha;
				shapeMode->parameters.binarizationCutoff = window->shaper->alphacutoff;
			}
			else
				shapeMode->mode = ShapeModeDefault;
			return 0;
		}
	}
	else
		//The window given is not a valid shapeable window.
		return -1;
}
