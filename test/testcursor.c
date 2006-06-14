#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"

/* This is an example 16x16 cursor
	top left :	black
	top right : inverted color or black
	bottom left: white
	bottom right: transparent
	(swap left and right for different endianness)
*/

Uint16 cursor_data[16]={
	0xffff,
	0xffff,
	0xffff,
	0xffff,	

	0xffff,
	0xffff,
	0xffff,
	0xffff,	

	0x0000,
	0x0000,
	0x0000,
	0x0000,

	0x0000,
	0x0000,
	0x0000,
	0x0000
};

Uint16 cursor_mask[16]={
	0xff00,
	0xff00,
	0xff00,
	0xff00,

	0xff00,
	0xff00,
	0xff00,
	0xff00,

	0xff00,
	0xff00,
	0xff00,
	0xff00,

	0xff00,
	0xff00,
	0xff00,
	0xff00
};

int main(int argc, char *argv[])
{
	SDL_Surface *screen;
	SDL_bool quit = SDL_FALSE, first_time = SDL_TRUE;
	SDL_Cursor *cursor;
	SDL_Rect update_area;

	/* Load the SDL library */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		return(1);
	}

	screen = SDL_SetVideoMode(320,200,8,SDL_ANYFORMAT);
	if (screen==NULL) {
		fprintf(stderr, "Couldn't initialize video mode: %s\n",SDL_GetError());
		return(1);
	}

	update_area.x = update_area.y = 0;
	update_area.w = screen->w;
	update_area.h = screen->h;

	SDL_FillRect(screen, NULL, 0x664422);

	cursor = SDL_CreateCursor((Uint8 *)cursor_data, (Uint8 *)cursor_mask,
		16, 16, 8, 8);
	if (cursor==NULL) {
		fprintf(stderr, "Couldn't initialize cursor: %s\n",SDL_GetError());
		return(1);
	}

	SDL_SetCursor(cursor);

	while (!quit) {
		SDL_Event	event;
		while (SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						quit = SDL_TRUE;
					}
					break;
				case SDL_QUIT:
					quit = SDL_TRUE;
					break;
			}
		}	
		if (screen->flags & SDL_DOUBLEBUF) {
			SDL_Flip(screen);
		} else {
			if (first_time) {
				SDL_UpdateRects(screen, 1, &update_area);
				first_time = SDL_FALSE;
			}
		}	
		SDL_Delay(1);
	}

	SDL_FreeCursor(cursor);

	SDL_Quit();
	return(0);
}
