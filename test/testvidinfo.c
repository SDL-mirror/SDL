
/* Simple program -- figure out what kind of video display we have */

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"

#define FLAG_MASK	(SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF)

void PrintFlags(Uint32 flags)
{
	printf("0x%8.8x", (flags & FLAG_MASK));
	if ( flags & SDL_HWSURFACE ) {
		printf(" SDL_HWSURFACE");
	} else {
		printf(" SDL_SWSURFACE");
	}
	if ( flags & SDL_FULLSCREEN ) {
		printf(" | SDL_FULLSCREEN");
	}
	if ( flags & SDL_DOUBLEBUF ) {
		printf(" | SDL_DOUBLEBUF");
	}
}

int RunBlitTests(SDL_Surface *screen, SDL_Surface *bmp, int blitcount)
{
	int i, j;
	int maxx;
	int maxy;
	SDL_Rect *rects;

	rects = (SDL_Rect *)malloc(blitcount * sizeof(*rects));
	if ( ! rects ) {
		return 0;
	}
	maxx = (int)screen->w - bmp->w;
	maxy = (int)screen->h - bmp->h;
	for ( i = 0; i < 100; ++i ) {
		for ( j = 0; j < blitcount; ++j ) {
			if ( maxx ) {
				rects[j].x = rand() % maxx;
			} else {
				rects[j].x = 0;
			}
			if ( maxy ) {
				rects[j].y = rand() % maxy;
			} else {
				rects[j].y = 0;
			}
			rects[j].w = bmp->w;
			rects[j].h = bmp->h;
			SDL_BlitSurface(bmp, NULL, screen, &rects[j]);
		}
		if ( screen->flags & SDL_DOUBLEBUF ) {
			SDL_Flip(screen);
		} else {
			SDL_UpdateRects(screen, blitcount, rects);
		}
	}
	free(rects);

	return i;
}

void RunModeTests(SDL_Surface *screen)
{
	Uint32 then, now;
	Uint32 frames;
	int i;
	Uint8 r, g, b;
	Uint32 pixel;
	SDL_Surface *bmp, *tmp;

	/* First test fills and screen update speed */
	printf("Running color fill and fullscreen update test\n");
	then = SDL_GetTicks();
	frames = 0;
	for ( i = 0; i < 256; ++i ) {
		r = i;
		g = 0;
		b = 0;
		SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, r, g, b));
		SDL_Flip(screen);
		++frames;
	}
	for ( i = 0; i < 256; ++i ) {
		r = 0;
		g = i;
		b = 0;
		SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, r, g, b));
		SDL_Flip(screen);
		++frames;
	}
	for ( i = 0; i < 256; ++i ) {
		r = 0;
		g = 0;
		b = i;
		SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, r, g, b));
		SDL_Flip(screen);
		++frames;
	}
	now = SDL_GetTicks();
	printf("%d fills and flips, %f FPS\n", frames, (float)(now - then) / frames);

	bmp = SDL_LoadBMP("sample.bmp");
	if ( ! bmp ) {
		printf("Couldn't load sample.bmp: %s\n", SDL_GetError());
		return;
	}
	printf("Running freshly loaded blit test: %dx%d at %d bpp, flags: ",
		bmp->w, bmp->h, bmp->format->BitsPerPixel);
	PrintFlags(bmp->flags);
	printf("\n");
	then = SDL_GetTicks();
	frames = RunBlitTests(screen, bmp, 10);
	now = SDL_GetTicks();
	if ( frames ) {
		printf("%d blits, %d updates, %f FPS\n", 10*frames, frames, (float)(now - then) / frames);
	}

	tmp = bmp;
	bmp = SDL_DisplayFormat(bmp);
	SDL_FreeSurface(tmp);
	if ( ! bmp ) {
		printf("Couldn't convert sample.bmp: %s\n", SDL_GetError());
		return;
	}
	printf("Running display format blit test: %dx%d at %d bpp, flags: ",
		bmp->w, bmp->h, bmp->format->BitsPerPixel);
	PrintFlags(bmp->flags);
	printf("\n");
	then = SDL_GetTicks();
	frames = RunBlitTests(screen, bmp, 10);
	now = SDL_GetTicks();
	if ( frames ) {
		printf("%d blits, %d updates, %f FPS\n", 10*frames, frames, (float)(now - then) / frames);
	}
	SDL_FreeSurface(bmp);
}

void RunVideoTests()
{
	static const struct {
		int w, h, bpp;
	} mode_list[] = {
		{ 640, 480, 8 }, { 640, 480, 16 }, { 640, 480, 32 },
		{ 800, 600, 8 }, { 800, 600, 16 }, { 800, 600, 32 },
		{ 1024, 768, 8 }, { 1024, 768, 16 }, { 1024, 768, 32 }
	};
	static const Uint32 flags[] = {
		(SDL_SWSURFACE),
		(SDL_SWSURFACE | SDL_FULLSCREEN),
		(SDL_HWSURFACE | SDL_FULLSCREEN),
		(SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF)
	};
	int i, j;
	SDL_Surface *screen;

	/* Test out several different video mode combinations */
	for ( i = 0; i < SDL_TABLESIZE(mode_list); ++i ) {
		for ( j = 0; j < SDL_TABLESIZE(flags); ++j ) {
			printf("===================================\n");
			printf("Setting video mode: %dx%d at %d bpp, flags: ",
			                          mode_list[i].w,
			                          mode_list[i].h,
			                          mode_list[i].bpp);
			PrintFlags(flags[j]);
			printf("\n");
			screen = SDL_SetVideoMode(mode_list[i].w,
			                          mode_list[i].h,
			                          mode_list[i].bpp,
			                          flags[j]);
			if ( ! screen ) {
				printf("Setting video mode failed: %s\n", SDL_GetError());
				continue;
			}
			if ( (screen->flags & FLAG_MASK) != flags[j] ) {
				printf("Flags didn't match: ");
				PrintFlags(screen->flags);
				printf("\n");
				continue;
			}
			RunModeTests(screen);
		}
	}
}

int main(int argc, char *argv[])
{
	const SDL_VideoInfo *info;
	int i;
	SDL_Rect **modes;

	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr,
			"Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	info = SDL_GetVideoInfo();
	printf(
"Current display: %d bits-per-pixel\n",info->vfmt->BitsPerPixel);
	if ( info->vfmt->palette == NULL ) {
		printf("	Red Mask = 0x%.8x\n", info->vfmt->Rmask);
		printf("	Green Mask = 0x%.8x\n", info->vfmt->Gmask);
		printf("	Blue Mask = 0x%.8x\n", info->vfmt->Bmask);
	}
	/* Print available fullscreen video modes */
	modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
	if ( modes == (SDL_Rect **)0 ) {
		printf("No available fullscreen video modes\n");
	} else
	if ( modes == (SDL_Rect **)-1 ) {
		printf("No special fullscreen video modes\n");
	} else {
		printf("Fullscreen video modes:\n");
		for ( i=0; modes[i]; ++i ) {
			printf("\t%dx%dx%d\n", modes[i]->w, modes[i]->h, info->vfmt->BitsPerPixel);
		}
	}
	if ( info->wm_available ) {
		printf("A window manager is available\n");
	}
	if ( info->hw_available ) {
		printf("Hardware surfaces are available (%dK video memory)\n",
			info->video_mem);
	}
	if ( info->blit_hw ) {
		printf(
"Copy blits between hardware surfaces are accelerated\n");
	}
	if ( info->blit_hw_CC ) {
		printf(
"Colorkey blits between hardware surfaces are accelerated\n");
	}
	if ( info->blit_hw_A ) {
		printf(
"Alpha blits between hardware surfaces are accelerated\n");
	}
	if ( info->blit_sw ) {
		printf(
"Copy blits from software surfaces to hardware surfaces are accelerated\n");
	}
	if ( info->blit_sw_CC ) {
		printf(
"Colorkey blits from software surfaces to hardware surfaces are accelerated\n");
	}
	if ( info->blit_sw_A ) {
		printf(
"Alpha blits from software surfaces to hardware surfaces are accelerated\n");
	}
	if ( info->blit_fill ) {
		printf(
"Color fills on hardware surfaces are accelerated\n");
	}

	if ( argv[1] && (strcmp(argv[1], "-benchmark") == 0) ) {
		RunVideoTests();
	}

	SDL_Quit();
	return(0);
}
