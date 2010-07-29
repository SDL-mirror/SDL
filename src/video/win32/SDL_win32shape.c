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
	result->mode.mode = ShapeModeDefault;
	result->mode.parameters.binarizationCutoff = 1;
	result->usershownflag = 0;
	//Put some driver-data here.
	window->shaper = result;
	int resized_properly = Win32_ResizeWindowShape(window);
	assert(resized_properly == 0);
	return result;
}

void CombineRectRegions(SDL_ShapeTree* node,HRGN* mask_region) {
	if(node->kind == OpaqueShape) {
		HRGN temp_region = CreateRectRgn(node->data.shape.x,node->data.shape.y,node->data.shape.w,node->data.shape.h);
		CombineRgn(*mask_region,*mask_region,temp_region, RGN_OR);
		DeleteObject(temp_region);
	}
}

int Win32_SetWindowShape(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shapeMode) {
	assert(shaper != NULL && shape != NULL);
	if(!SDL_ISPIXELFORMAT_ALPHA(SDL_MasksToPixelFormatEnum(shape->format->BitsPerPixel,shape->format->Rmask,shape->format->Gmask,shape->format->Bmask,shape->format->Amask)) && shapeMode->mode != ShapeModeColorKey || shape->w != shaper->window->w || shape->h != shaper->window->h)
		return SDL_INVALID_SHAPE_ARGUMENT;
	
	SDL_ShapeData *data = (SDL_ShapeData*)shaper->driverdata;
	data->mask_tree = SDL_CalculateShapeTree(shapeMode,shape,SDL_FALSE);
	
	/*
	 * Start with empty region 
	 */
	HRGN mask_region = CreateRectRgn(0, 0, 0, 0);
	
	SDL_TraverseShapeTree(data->mask_tree,&CombineRectRegions,&mask_region);
	
	/*
	 * Set the new region mask for the window 
	 */
	SetWindowRgn((SDL_WindowData*)(shaper->window->driverdata)->hwnd, mask_region, TRUE);
	
	return 0;
}

int Win32_ResizeWindowShape(SDL_Window *window) {
	SDL_ShapeData* data = window->shaper->driverdata;
	assert(data != NULL);
	
	if(data->mask_tree != NULL)
		SDL_FreeShapeTree(&data->mask_tree);
	
	window->shaper->usershownflag |= window->flags & SDL_WINDOW_SHOWN;
	
	return 0;
}
