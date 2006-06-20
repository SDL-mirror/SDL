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


/* XPM */
static const char *arrow[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "X                               ",
  "XX                              ",
  "X.X                             ",
  "X..X                            ",
  "X...X                           ",
  "X....X                          ",
  "X.....X                         ",
  "X......X                        ",
  "X.......X                       ",
  "X........X                      ",
  "X.....XXXXX                     ",
  "X..X..X                         ",
  "X.X X..X                        ",
  "XX  X..X                        ",
  "X    X..X                       ",
  "     X..X                       ",
  "      X..X                      ",
  "      X..X                      ",
  "       XX                       ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "0,0"
};

static SDL_Cursor *create_arrow_cursor()
{
  int i, row, col;
  Uint8 data[4*32];
  Uint8 mask[4*32];
  int hot_x, hot_y;

  i = -1;
  for ( row=0; row<32; ++row ) {
    for ( col=0; col<32; ++col ) {
      if ( col % 8 ) {
        data[i] <<= 1;
        mask[i] <<= 1;
      } else {
        ++i;
        data[i] = mask[i] = 0;
      }
      switch (arrow[4+row][col]) {
        case 'X':
          data[i] |= 0x01;
          mask[i] |= 0x01;
          break;
        case '.':
          mask[i] |= 0x01;
          break;
        case ' ':
          break;
      }
    }
  }
  sscanf(arrow[4+row], "%d,%d", &hot_x, &hot_y);
  return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
}


int main(int argc, char *argv[])
{
	SDL_Surface *screen;
	SDL_bool quit = SDL_FALSE, first_time = SDL_TRUE;
	SDL_Cursor *cursor[2];
	int current;

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

	SDL_FillRect(screen, NULL, 0x664422);

	cursor[0] = SDL_CreateCursor((Uint8 *)cursor_data, (Uint8 *)cursor_mask,
		16, 16, 8, 8);
	if (cursor[0]==NULL) {
		fprintf(stderr, "Couldn't initialize test cursor: %s\n",SDL_GetError());
		SDL_Quit();
		return(1);
	}
	cursor[1] = create_arrow_cursor();
	if (cursor[1]==NULL) {
		fprintf(stderr, "Couldn't initialize arrow cursor: %s\n",SDL_GetError());
		SDL_FreeCursor(cursor[0]);
		SDL_Quit();
		return(1);
	}

	current = 0;
	SDL_SetCursor(cursor[current]);

	while (!quit) {
		SDL_Event	event;
		while (SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_MOUSEBUTTONDOWN:
					current = !current;
					SDL_SetCursor(cursor[current]);
					break;
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
		SDL_Flip(screen);
		SDL_Delay(1);
	}

	SDL_FreeCursor(cursor[0]);
	SDL_FreeCursor(cursor[1]);

	SDL_Quit();
	return(0);
}
