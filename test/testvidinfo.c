
/* Simple program -- figure out what kind of video display we have */

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"

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
			printf("\t%dx%d\n", modes[i]->w, modes[i]->h);
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
	SDL_Quit();
	return(0);
}
