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

#include <windows.h>
#include "SDL_win32shape.h"

SDL_WindowShaper* Win32_CreateShaper(SDL_Window * window) {
	SDL_WindowShaper* result = malloc(sizeof(SDL_WindowShaper));
	result->window = window;
	result->alphacutoff = 0;
	result->usershownflag = 0;
	//Put some driver-data here.
	window->shaper = result;
	int resized_properly = X11ResizeWindowShape(window);
	assert(resized_properly == 0);
	return result;
}

int Win32_SetWindowShape(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shapeMode) {
	assert(shaper != NULL && shape != NULL);
	if(!SDL_ISPIXELFORMAT_ALPHA(SDL_MasksToPixelFormatEnum(shape->format->BitsPerPixel,shape->format->Rmask,shape->format->Gmask,shape->format->Bmask,shape->format->Amask)))
		return -2;
	if(shape->w != shaper->window->w || shape->h != shaper->window->h)
		return -3;
	
	/* Assume that shaper->alphacutoff already has a value, because SDL_SetWindowShape() should have given it one. */
	/*
	 * Start with empty region 
	 */
	HRGN MaskRegion = CreateRectRgn(0, 0, 0, 0);
	
	unsigned int pitch = shape->pitch;
	unsigned int width = shape->width;
	unsigned int height = shape->height;
	unsigned int dy = pitch - width;
	
	SDL_ShapeData *data = (SDL_ShapeData*)shaper->driverdata;
	/*
	 * Transfer binarized mask image into workbuffer 
	 */
	SDL_CalculateShapeBitmap(shaper->alphacutoff,shape,data->shapebuffer,1,0xff);
	//Move code over to here from AW_windowShape.c
	
}

int Win32_ResizeWindowShape(SDL_Window *window) {
	SDL_ShapeData* data = window->shaper->driverdata;
	assert(data != NULL);
	
	unsigned int buffersize = window->w * window->h;
	if(data->buffersize != buffersize || data->shapebuffer == NULL) {
		data->buffersize = buffersize;
		if(data->shapebuffer != NULL)
			free(data->shapebuffer);
		data->shapebuffer = malloc(data->buffersize);
		if(data->shapebuffer == NULL) {
			SDL_SetError("Could not allocate memory for shaped-window bitmap.");
			return -1;
		}
	}
	
	window->shaper->usershownflag = window->flags & SDL_WINDOW_SHOWN;
	
	return 0;
}
