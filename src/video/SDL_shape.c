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

#include "SDL_shape.h"

struct SDL_Shaped_Window {
	SDL_Window *window;
	SDL_Surface *shape_mask;
}

SDL_Shaped_Window * SDLCALL SDL_CreateShapedWindow(const char *title,
						   unsigned int x,unsigned int y,
						   unsigned int w,unsigned int h,
						   Uint32 flags) {
	//TODO: Fill in stub of SDL_CreatedShapedWindow
	return NULL;
}

int SDLCALL SDL_PresentShape(const SDL_Shaped_Window *window) {
	//TODO: Fill in stub of SDL_PresentShape
	return -1;
}

void SDLCALL SDL_DestroyShapedWindow(const SDL_Shaped_Window *window) {
	//TODO: Fill in stub of SDL_DestroyShapedWindow
	return;
}
