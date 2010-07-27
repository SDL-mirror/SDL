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

#include "SDL_cocoavideo.h"
#include "SDL_shape.h"
#include "SDL_cocoashape.h"

SDL_WindowShaper* Cocoa_CreateShaper(SDL_Window* window) {
	SDL_WindowData* data = (SDL_WindowData*)window->driverdata;
	[data->nswindow setAlpha:1.0];
	[data->nswindow setOpaque:YES];
	[data->nswindow setStyleMask:NSBorderlessWindowMask];
	SDL_Shaper* result = result = malloc(sizeof(SDL_WindowShaper));
	result->window = window;
	result->mode.mode = ShapeModeDefault;
	result->mode.parameters.binarizationCutoff = 1;
	result->usershownflag = 0;
	window->shaper = result;
	
	SDL_ShapeData* data = malloc(sizeof(SDL_ShapeData));
	result->driverdata = data;
	data->context = [data->nswindow graphicsContext];
	data->saved = SDL_False;
	data->rects = NULL;
	data->count = 0;
	
	int resized_properly = Cocoa_ResizeWindowShape(window);
	assert(resized_properly == 0);
	return result;
}

int Cocoa_SetWindowShape(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shapeMode) {
	SDL_WindowData* data = (SDL_WindowData*)shaper->window->driverdata;
	if(data->saved == SDL_True) {
		[data->context restoreGraphicsState];
		data->saved = SDL_False;
	}
		
	[data->context saveGraphicsState];
	data->saved = SDL_True;
	
	[[NSColor clearColor] set];
	NSRectFill([[data->nswindow contentView] frame]);
	/* TODO: It looks like Cocoa can set a clipping path based on a list of rectangles.  That's what we get from the
           Windoze shape-calculation code: a list of rectangles.  This will work... I think. */
}

int Cocoa_ResizeWindowShape(SDL_Window *window) {
	SDL_ShapeData* data = window->shaper->driverdata;
	assert(data != NULL);
	return 0;
}
