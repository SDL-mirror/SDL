#include <stdlib.h>
#include <math.h>
#include <SDL_events.h>
#include <SDL_rect.h>
#include <SDL_pixels.h>
#include <SDL_video.h>
#include <SDL_shape.h>
#include "testeyes_bitmap.h"
#include "testeyes_mask_bitmap.h"

/* The following code for the calculation of pupil positions has been taken and rewritten from the original xeyes.  The
   copyright notice is included as follows, and the code is now under the same license as the rest of this file. */
/*

Copyright (c) 1991  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/
# define NUM_EYES	2
# define EYE_X(n)	((n) * 2.0)
# define EYE_Y(n)	(0.0)
# define EYE_THICK	(0.175)	/* thickness of eye rim */
# define EYE_OFFSET	(0.1)	/* padding between eyes */
# define EYE_WIDTH	(2.0 - (EYE_THICK + EYE_OFFSET) * 2)
# define EYE_HEIGHT	EYE_WIDTH
# define EYE_HWIDTH	(EYE_WIDTH / 2.0)
# define EYE_HHEIGHT	(EYE_HEIGHT / 2.0)
# define BALL_WIDTH	(0.3)
# define BALL_HEIGHT	BALL_WIDTH
# define BALL_PAD	(0.05)
# define BALL_DIST	((EYE_WIDTH - BALL_WIDTH) / 2.0 - BALL_PAD)

SDL_Point computePupil (int num,SDL_Point mouse) {
	double dx = mouse.x - EYE_X(num);
	double dy = mouse.y - EYE_Y(num);
	double cx = 0.0,cy = 0.0;
	if(dx == 0 && dy == 0) {
		cx = EYE_X(num);
		cy = EYE_Y(num);
	}
	else {
		double angle = atan2((double)dy,(double)dx);
		double cosa = cos(angle);
		double sina = sin(angle);
		double hypotenuse = hypot(EYE_HHEIGHT * cosa, EYE_HWIDTH * sina);
		double dist = BALL_DIST * hypot((EYE_HWIDTH * EYE_HHEIGHT) * cosa / hypotenuse,
					        (EYE_HWIDTH * EYE_HHEIGHT) * sina / hypotenuse);
		if(dist > hypot((double)dx,(double)dy)) {
			cx = dx + EYE_X(num);
			cy = dy + EYE_Y(num);
		}
		else {
			cx = dist * cosa + EYE_X(num);
			cy = dist * sina + EYE_Y(num);
		}
	}
	SDL_Point ret = {cx,cy};
	return ret;
}

/* Here begins the code exclusively and entirely written by Eli Gottlieb in May 2010. */
typedef struct {
	SDL_Rect left;
	SDL_Rect right;
} Pupil_Pair;
typedef struct {
	SDL_Point left;
	SDL_Point right;
} Pupil_Points;

Pupil_Pair compute_pupil_positions(SDL_Point target) {
	Pupil_Pair result;
	Pupil_Points points;
	points.left = computePupil(0,target);
	points.right = computePupil(1,target);
	result.left.x = points.left.x - BALL_WIDTH / 2.0;
	result.left.y = points.left.y - BALL_HEIGHT / 2.0;
	result.right.x = points.right.x - BALL_WIDTH / 2.0;
	result.right.y = points.right.y - BALL_HEIGHT / 2.0;
	result.left.w = result.right.w = BALL_WIDTH;
	result.left.h = result.left.w = BALL_HEIGHT;
	return result;
}

void render_eyes(SDL_Window *window,SDL_Texture *eyes_texture,Pupil_Pair pupils) {
	SDL_SelectRenderer(window);
	
	//Clear render-target to blue.
	SDL_SetRenderDrawColor(0x00,0x00,0xff,0xff);
	SDL_RenderClear();
	
	//Render the whites of the eyes.
	SDL_Rect srcdestrect = {0,0,eyes_width,eyes_height};
	SDL_RenderCopy(eyes_texture,&srcdestrect,&srcdestrect);
	
	//Render the pupils.
	SDL_SetRenderDrawColor(0x00,0x00,0x00,0xff);
	const SDL_Rect eyes[2] = {pupils.left,pupils.right};
	SDL_RenderFillRects((const SDL_Rect**)&eyes,2);
	
	SDL_RenderPresent();
}

int main(int argc,char** argv) {
	if(SDL_VideoInit(NULL,0) == -1) {
		printf("Could not initialize SDL video.\n");
		exit(-1);
	}
	
	SDL_Window *window = SDL_CreateShapedWindow("Big Brother is watching you.",eyes_x_hot,eyes_y_hot,eyes_width,eyes_height,SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
	if(window == NULL) {
		SDL_VideoQuit();
		printf("Could not create shaped window for eyes.\n");
		exit(-2);
	}
	if(SDL_CreateRenderer(window,-1,SDL_RENDERER_PRESENTFLIP2) == -1) {
		SDL_DestroyWindow(window);
		SDL_VideoQuit();
		printf("Could not create rendering context for SDL_Eyes window.\n");
		exit(-3);
	}
	
	int bpp = 0;
	Uint32 r = 0,g = 0,b = 0,a = 0;
	SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_ARGB4444,&bpp,&r,&g,&b,&a);
	SDL_Surface *eyes = SDL_CreateRGBSurface(0,eyes_width,eyes_height,bpp,r,g,b,a);
	if(eyes == NULL) {
		SDL_DestroyRenderer(window);
		SDL_DestroyWindow(window);
		SDL_VideoQuit();
		printf("Could not create eyes surface.\n");
		exit(-4);
	}
	
	void *pixels = NULL;
	int pitch = 0;
	SDL_Rect rect = {0,0,eyes_width,eyes_height};
	if(SDL_MUSTLOCK(eyes))
		SDL_LockSurface(eyes);
	pixels = eyes->pixels;
	pitch = eyes->pitch;
	for(int y=0;y<eyes_height;y++)
		for(int x=0;x<eyes_width;x++) {
			Uint8 brightness = *(Uint8*)(eyes_bits+(eyes_width/8)*y+(x/8)) & (1 << (7 - x % 8)) ? 255 : 0;
			*(Uint16*)(pixels+pitch*y+x*16/8) = SDL_MapRGBA(eyes->format,brightness,brightness,brightness,255);
		}
	if(SDL_MUSTLOCK(eyes))
		SDL_UnlockSurface(eyes);
	SDL_Texture *eyes_texture = SDL_CreateTextureFromSurface(0,eyes);
	if(eyes_texture == NULL) {
		SDL_FreeSurface(eyes);
		SDL_DestroyRenderer(window);
		SDL_DestroyWindow(window);
		SDL_VideoQuit();
		printf("Could not create eyes texture.\n");
		exit(-5);
	}

	SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_ARGB4444,&bpp,&r,&g,&b,&a);
	SDL_Surface *mask = SDL_CreateRGBSurface(0,eyesmask_width,eyesmask_height,bpp,r,g,b,a);
	if(mask == NULL) {
		SDL_DestroyTexture(eyes_texture);
		SDL_FreeSurface(eyes);
		SDL_DestroyRenderer(window);
		SDL_DestroyWindow(window);
		SDL_VideoQuit();
		printf("Could not create shape mask texture.\n");
		exit(-5);
	}
	
	if(SDL_MUSTLOCK(mask))
		SDL_LockSurface(mask);
	pixels = mask->pixels;
	pitch = mask->pitch;
	for(int y=0;y<eyesmask_height;y++)
		for(int x=0;x<eyesmask_width;x++) {
			Uint8 alpha = *(Uint8*)(eyesmask_bits+(eyesmask_width/8)*y+(x/8)) & (1 << (7 - x % 8)) ? 1 : 0;
			*(Uint16*)(pixels+pitch*y+x*bpp/8) = SDL_MapRGBA(mask->format,0,0,0,alpha);
		}
	if(SDL_MUSTLOCK(mask))
		SDL_UnlockSurface(mask);
	
	SDL_WindowShapeMode mode = {ShapeModeDefault,1};
	SDL_SetWindowShape(window,mask,&mode);
	
	SDL_Event event;
	int event_pending = 0;
	event_pending = SDL_PollEvent(&event);
	SDL_Point mouse_position;
	Pupil_Pair pupil_positions;
	SDL_SelectMouse(0);
	SDL_GetMouseState(&mouse_position.x,&mouse_position.y);
	pupil_positions = compute_pupil_positions(mouse_position);
	while(event.type != SDL_QUIT) {
		if(event.type == SDL_MOUSEMOTION) {
			mouse_position.x = event.motion.x;
			mouse_position.y = event.motion.y;
			pupil_positions = compute_pupil_positions(mouse_position);
		}
		render_eyes(window,eyes_texture,pupil_positions);
		event_pending = SDL_PollEvent(&event);
	}
	
	SDL_FreeSurface(mask);
	SDL_DestroyTexture(eyes_texture);
	SDL_DestroyWindow(window);
	//Call SDL_VideoQuit() before quitting.
	SDL_VideoQuit();
}
