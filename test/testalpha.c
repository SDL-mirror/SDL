
/* Simple program:  Fill a colormap with gray and stripe it down the screen,
		    Then move an alpha valued sprite around the screen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "SDL.h"

#define FRAME_TICKS	(1000/30)		/* 30 frames/second */

/* Create a "light" -- a yellowish surface with variable alpha */
SDL_Surface *CreateLight(SDL_Surface *screen, int radius)
{
	Uint8  trans, alphamask;
	int    range, addition;
	int    xdist, ydist;
	Uint16 x, y;
	Uint16 skip;
	Uint32 pixel;
	SDL_Surface *light;

#ifdef LIGHT_16BIT
	Uint16 *buf;

	/* Create a 16 (4/4/4/4) bpp square with a full 4-bit alpha channel */
	/* Note: this isn't any faster than a 32 bit alpha surface */
	alphamask = 0x0000000F;
	light = SDL_CreateRGBSurface(SDL_SWSURFACE, 2*radius, 2*radius, 16,
			0x0000F000, 0x00000F00, 0x000000F0, alphamask);
#else
	Uint32 *buf;

	/* Create a 32 (8/8/8/8) bpp square with a full 8-bit alpha channel */
	alphamask = 0x000000FF;
	light = SDL_CreateRGBSurface(SDL_SWSURFACE, 2*radius, 2*radius, 32,
			0xFF000000, 0x00FF0000, 0x0000FF00, alphamask);
	if ( light == NULL ) {
		fprintf(stderr, "Couldn't create light: %s\n", SDL_GetError());
		return(NULL);
	}
#endif

	/* Fill with a light yellow-orange color */
	skip = light->pitch-(light->w*light->format->BytesPerPixel);
#ifdef LIGHT_16BIT
	buf = (Uint16 *)light->pixels;
#else
	buf = (Uint32 *)light->pixels;
#endif
        /* Get a tranparent pixel value - we'll add alpha later */
	pixel = SDL_MapRGBA(light->format, 0xFF, 0xDD, 0x88, 0);
	for ( y=0; y<light->h; ++y ) {
		for ( x=0; x<light->w; ++x ) {
			*buf++ = pixel;
		}
		buf += skip;	/* Almost always 0, but just in case... */
	}

	/* Calculate alpha values for the surface. */
#ifdef LIGHT_16BIT
	buf = (Uint16 *)light->pixels;
#else
	buf = (Uint32 *)light->pixels;
#endif
	for ( y=0; y<light->h; ++y ) {
		for ( x=0; x<light->w; ++x ) {
			/* Slow distance formula (from center of light) */
			xdist = x-(light->w/2);
			ydist = y-(light->h/2);
			range = (int)sqrt(xdist*xdist+ydist*ydist);

			/* Scale distance to range of transparency (0-255) */
			if ( range > radius ) {
				trans = alphamask;
			} else {
				/* Increasing transparency with distance */
				trans = (Uint8)((range*alphamask)/radius);

				/* Lights are very transparent */
				addition = (alphamask+1)/8;
				if ( (int)trans+addition > alphamask ) {
					trans = alphamask;
				} else {
					trans += addition;
				}
			}
			/* We set the alpha component as the right N bits */
			*buf++ |= (255-trans);
		}
		buf += skip;	/* Almost always 0, but just in case... */
	}
	/* Enable RLE acceleration of this alpha surface */
	SDL_SetAlpha(light, SDL_SRCALPHA|SDL_RLEACCEL, 0);

	/* We're done! */
	return(light);
}

static Uint32 flashes = 0;
static Uint32 flashtime = 0;

void FlashLight(SDL_Surface *screen, SDL_Surface *light, int x, int y)
{
	SDL_Rect position;
	Uint32   ticks1;
	Uint32   ticks2;

	/* Easy, center light */
	position.x = x-(light->w/2);
	position.y = y-(light->h/2);
	position.w = light->w;
	position.h = light->h;
	ticks1 = SDL_GetTicks();
	SDL_BlitSurface(light, NULL, screen, &position);
	ticks2 = SDL_GetTicks();
	SDL_UpdateRects(screen, 1, &position);
	++flashes;

	/* Update time spend doing alpha blitting */
	flashtime += (ticks2-ticks1);
}

static int sprite_visible = 0;
static SDL_Surface *sprite;
static SDL_Surface *backing;
static SDL_Rect    position;
static int         x_vel, y_vel;
static int	   alpha_vel;

int LoadSprite(SDL_Surface *screen, char *file)
{
	SDL_Surface *converted;

	/* Load the sprite image */
	sprite = SDL_LoadBMP(file);
	if ( sprite == NULL ) {
		fprintf(stderr, "Couldn't load %s: %s", file, SDL_GetError());
		return(-1);
	}

	/* Set transparent pixel as the pixel at (0,0) */
	if ( sprite->format->palette ) {
		SDL_SetColorKey(sprite, SDL_SRCCOLORKEY,
						*(Uint8 *)sprite->pixels);
	}

	/* Convert sprite to video format */
	converted = SDL_DisplayFormat(sprite);
	SDL_FreeSurface(sprite);
	if ( converted == NULL ) {
		fprintf(stderr, "Couldn't convert background: %s\n",
							SDL_GetError());
		return(-1);
	}
	sprite = converted;

	/* Create the background */
	backing = SDL_CreateRGBSurface(SDL_SWSURFACE, sprite->w, sprite->h, 8,
								0, 0, 0, 0);
	if ( backing == NULL ) {
		fprintf(stderr, "Couldn't create background: %s\n",
							SDL_GetError());
		SDL_FreeSurface(sprite);
		return(-1);
	}

	/* Convert background to video format */
	converted = SDL_DisplayFormat(backing);
	SDL_FreeSurface(backing);
	if ( converted == NULL ) {
		fprintf(stderr, "Couldn't convert background: %s\n",
							SDL_GetError());
		SDL_FreeSurface(sprite);
		return(-1);
	}
	backing = converted;

	/* Set the initial position of the sprite */
	position.x = (screen->w-sprite->w)/2;
	position.y = (screen->h-sprite->h)/2;
	position.w = sprite->w;
	position.h = sprite->h;
	x_vel = 0; y_vel = 0;
	alpha_vel = 1;

	/* We're ready to roll. :) */
	return(0);
}

void AttractSprite(Uint16 x, Uint16 y)
{
	x_vel = ((int)x-position.x)/10;
	y_vel = ((int)y-position.y)/10;
}

void MoveSprite(SDL_Surface *screen, SDL_Surface *light)
{
	SDL_Rect updates[2];
	int alpha;

	/* Erase the sprite if it was visible */
	if ( sprite_visible ) {
		updates[0] = position;
		SDL_BlitSurface(backing, NULL, screen, &updates[0]);
	} else {
		updates[0].x = 0; updates[0].y = 0;
		updates[0].w = 0; updates[0].h = 0;
		sprite_visible = 1;
	}

	/* Since the sprite is off the screen, we can do other drawing
	   without being overwritten by the saved area behind the sprite.
	 */
	if ( light != NULL ) {
		int x, y;

		SDL_GetMouseState(&x, &y);
		FlashLight(screen, light, x, y);
	}
	   
	/* Move the sprite, bounce at the wall */
	position.x += x_vel;
	if ( (position.x < 0) || (position.x >= screen->w) ) {
		x_vel = -x_vel;
		position.x += x_vel;
	}
	position.y += y_vel;
	if ( (position.y < 0) || (position.y >= screen->h) ) {
		y_vel = -y_vel;
		position.y += y_vel;
	}

	/* Update transparency (fade in and out) */
	alpha = sprite->format->alpha;
	if ( (alpha+alpha_vel) < 0 ) {
		alpha_vel = -alpha_vel;
	} else
	if ( (alpha+alpha_vel) > 255 ) {
		alpha_vel = -alpha_vel;
	}
	SDL_SetAlpha(sprite, SDL_SRCALPHA, (Uint8)(alpha+alpha_vel));

	/* Save the area behind the sprite */
	updates[1] = position;
	SDL_BlitSurface(screen, &updates[1], backing, NULL);
	
	/* Blit the sprite onto the screen */
	updates[1] = position;
	SDL_BlitSurface(sprite, NULL, screen, &updates[1]);

	/* Make it so! */
	SDL_UpdateRects(screen, 2, updates);
}

void WarpSprite(SDL_Surface *screen, int x, int y)
{
	SDL_Rect updates[2];

	/* Erase, move, Draw, update */
	updates[0] = position;
	SDL_BlitSurface(backing, NULL, screen, &updates[0]);
	position.x = x-sprite->w/2;	/* Center about X */
	position.y = y-sprite->h/2;	/* Center about Y */
	updates[1] = position;
	SDL_BlitSurface(screen, &updates[1], backing, NULL);
	updates[1] = position;
	SDL_BlitSurface(sprite, NULL, screen, &updates[1]);
	SDL_UpdateRects(screen, 2, updates);
}

int main(int argc, char *argv[])
{
	const SDL_VideoInfo *info;
	SDL_Surface *screen;
	Uint8  video_bpp;
	Uint32 videoflags;
	Uint8 *buffer;
	int    i, k, done;
	SDL_Event event;
	SDL_Surface *light;
	int mouse_pressed;
	Uint32 ticks, lastticks;
	Uint16 *buffer16;
        Uint16 color;
        Uint8  gradient;


	/* Initialize SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	/* Alpha blending doesn't work well at 8-bit color */
	info = SDL_GetVideoInfo();
	if ( info->vfmt->BitsPerPixel > 8 ) {
		video_bpp = info->vfmt->BitsPerPixel;
	} else {
		video_bpp = 16;
                fprintf(stderr, "forced 16 bpp mode\n");
	}
	videoflags = SDL_SWSURFACE;
	while ( argc > 1 ) {
		--argc;
		if ( strcmp(argv[argc-1], "-bpp") == 0 ) {
			video_bpp = atoi(argv[argc]);
                        if (video_bpp<=8) {
                            video_bpp=16;
                            fprintf(stderr, "forced 16 bpp mode\n");
                        }
			--argc;
		} else
		if ( strcmp(argv[argc], "-hw") == 0 ) {
			videoflags |= SDL_HWSURFACE;
		} else
		if ( strcmp(argv[argc], "-warp") == 0 ) {
			videoflags |= SDL_HWPALETTE;
		} else
		if ( strcmp(argv[argc], "-fullscreen") == 0 ) {
			videoflags |= SDL_FULLSCREEN;
		} else {
			fprintf(stderr, 
			"Usage: %s [-bpp N] [-warp] [-hw] [-fullscreen]\n",
								argv[0]);
			exit(1);
		}
	}

	/* Set 640x480 video mode */
	if ( (screen=SDL_SetVideoMode(640,480,video_bpp,videoflags)) == NULL ) {
		fprintf(stderr, "Couldn't set 640x480x%d video mode: %s\n",
						video_bpp, SDL_GetError());
		exit(2);
	}

	/* Set the surface pixels and refresh! */
	if ( SDL_LockSurface(screen) < 0 ) {
		fprintf(stderr, "Couldn't lock the display surface: %s\n",
							SDL_GetError());
		exit(2);
	}
	buffer=(Uint8 *)screen->pixels;
	if (screen->format->BytesPerPixel!=2) {
		for ( i=0; i<screen->h; ++i ) {
			memset(buffer,(i*255)/screen->h, screen->pitch);
			buffer += screen->pitch;
		}
	}
        else
        {
		for ( i=0; i<screen->h; ++i ) {
			gradient=((i*255)/screen->h);
                        color = SDL_MapRGB(screen->format, gradient, gradient, gradient);
                        buffer16=(Uint16*)buffer;
                        for (k=0; k<screen->w; k++)
                        {
                            *(buffer16+k)=color;
                        }
			buffer += screen->pitch;
		}
        }

	SDL_UnlockSurface(screen);
	SDL_UpdateRect(screen, 0, 0, 0, 0);

	/* Create the light */
	light = CreateLight(screen, 82);
	if ( light == NULL ) {
		exit(1);
	}

	/* Load the sprite */
	if ( LoadSprite(screen, "icon.bmp") < 0 ) {
		SDL_FreeSurface(light);
		exit(1);
	}

	/* Set a clipping rectangle to clip the outside edge of the screen */
	{ SDL_Rect clip;
		clip.x = 32;
		clip.y = 32;
		clip.w = screen->w-(2*32);
		clip.h = screen->h-(2*32);
		SDL_SetClipRect(screen, &clip);
	}

	/* Wait for a keystroke */
	lastticks = SDL_GetTicks();
	done = 0;
	mouse_pressed = 0;
	while ( !done ) {
		/* Update the frame -- move the sprite */
		if ( mouse_pressed ) {
			MoveSprite(screen, light);
			mouse_pressed = 0;
		} else {
			MoveSprite(screen, NULL);
		}

		/* Slow down the loop to 30 frames/second */
		ticks = SDL_GetTicks();
		if ( (ticks-lastticks) < FRAME_TICKS ) {
#ifdef CHECK_SLEEP_GRANULARITY
fprintf(stderr, "Sleeping %d ticks\n", FRAME_TICKS-(ticks-lastticks));
#endif
			SDL_Delay(FRAME_TICKS-(ticks-lastticks));
#ifdef CHECK_SLEEP_GRANULARITY
fprintf(stderr, "Slept %d ticks\n", (SDL_GetTicks()-ticks));
#endif
		}
		lastticks = ticks;

		/* Check for events */
		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {
				/* Attract sprite while mouse is held down */
				case SDL_MOUSEMOTION:
					if (event.motion.state != 0) {
						AttractSprite(event.motion.x,
								event.motion.y);
						mouse_pressed = 1;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if ( event.button.button == 1 ) {
						AttractSprite(event.button.x,
						              event.button.y);
						mouse_pressed = 1;
					} else {
						SDL_Rect area;

						area.x = event.button.x-16;
						area.y = event.button.y-16;
						area.w = 32;
						area.h = 32;
						SDL_FillRect(screen, &area, 0);
						SDL_UpdateRects(screen,1,&area);
					}
					break;
				case SDL_KEYDOWN:
					/* Any keypress quits the app... */
				case SDL_QUIT:
					done = 1;
					break;
				default:
					break;
			}
		}
	}
	SDL_FreeSurface(light);
	SDL_FreeSurface(sprite);
	SDL_FreeSurface(backing);

	/* Print out some timing information */
	if ( flashes > 0 ) {
		printf("%d alpha blits, ~%4.4f ms per blit\n", 
			flashes, (float)flashtime/flashes);
	}
	return(0);
}
