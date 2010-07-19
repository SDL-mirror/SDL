#include <stdlib.h>
#include <math.h>
#include <SDL_events.h>
#include <SDL_rect.h>
#include <SDL_pixels.h>
#include <SDL_video.h>
#include <SDL_shape.h>

#define SHAPED_WINDOW_X 150
#define SHAPED_WINDOW_Y 150
#define SHAPED_WINDOW_DIMENSION 640

int main(int argc,char** argv) {
        if(argc < 2) {
        	printf("SDL_Shape requires at least one bitmap file as argument.\n");
        	exit(-1);
        }
	
	if(SDL_VideoInit(NULL,0) == -1) {
		printf("Could not initialize SDL video.\n");
		exit(-2);
	}
	
	Uint8 num_pictures = argc - 1;
	SDL_Surface **pictures = malloc(sizeof(SDL_Surface*)*num_pictures);
	int i = 0;
	for(i=0;i<num_pictures;i++)
		pictures[i] = NULL;
	for(i=0;i<num_pictures;i++) {
		SDL_Surface *original = SDL_LoadBMP(argv[i+1]);
		if(original == NULL) {
			int j = 0;
			for(j=0;j<num_pictures;j++)
				if(pictures[j] != NULL)
					SDL_FreeSurface(pictures[j]);
			free(pictures);
			SDL_VideoQuit();
			printf("Could not load surface from named bitmap file.\n");
			exit(-3);
		}
		//THIS CONVERSION ROUTINE IS FRAGILE!  It relies in the fact that only certain portions of the format structure must be filled in to use it.
		SDL_PixelFormat format = {NULL,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		int bpp = 0;
		SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_RGBA8888,&bpp,&format.Rmask,&format.Gmask,&format.Bmask,&format.Amask);
		format.BitsPerPixel = bpp;
		format.BytesPerPixel = format.BitsPerPixel / 8 + (format.BitsPerPixel % 8 > 0 ? 1 : 0);
		pictures[i] = SDL_ConvertSurface(original,&format,0);
		//We have no more need of the original now that we have our desired format.
		SDL_FreeSurface(original);
		if(pictures[i] == NULL) {
			int j = 0;
			for(j=0;j<num_pictures;j++)
				if(pictures[j] != NULL)
					SDL_FreeSurface(pictures[j]);
			free(pictures);
			SDL_VideoQuit();
			printf("Could not convert bitmap surface to desired format.\n");
			exit(-3);
		}
		
		if(SDL_MUSTLOCK(pictures[i]))
			SDL_LockSurface(pictures[i]);
			
		void* pixels = pictures[i]->pixels;
		unsigned int pitch = pictures[i]->pitch;
		int y =0,x = 0;
		for(y=0;y<pictures[i]->h;y++)
			for(x=0;x<pictures[i]->w;x++) {
				Uint32* pixel = pixels + y * pitch + x * pictures[i]->format->BytesPerPixel;
				Uint8 r = 0,g = 0,b = 0;
				SDL_GetRGB(*pixel,pictures[i]->format,&r,&g,&b);
				//if(r == g == b == 0xff)
				//	*pixel = SDL_MapRGBA(pictures[i]->format,r,g,b,0);
			}
			
		if(SDL_MUSTLOCK(pictures[i]))
			SDL_UnlockSurface(pictures[i]);
	}
	
	SDL_Window *window = SDL_CreateShapedWindow("SDL_Shape test",SHAPED_WINDOW_X,SHAPED_WINDOW_Y,SHAPED_WINDOW_DIMENSION,SHAPED_WINDOW_DIMENSION,SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
	if(window == NULL) {
		for(i=0;i<num_pictures;i++)
			SDL_FreeSurface(pictures[i]);
		free(pictures);
		SDL_VideoQuit();
		printf("Could not create shaped window for SDL_Shape.\n");
		exit(-4);
	}
	if(SDL_CreateRenderer(window,-1,SDL_RENDERER_PRESENTFLIP2) == -1) {
		SDL_DestroyWindow(window);
		for(i=0;i<num_pictures;i++)
			SDL_FreeSurface(pictures[i]);
		free(pictures);
		SDL_VideoQuit();
		printf("Could not create rendering context for SDL_Shape window.\n");
		exit(-5);
	}
	
	SDL_Texture **textures = malloc(sizeof(SDL_Texture*)*num_pictures);
	for(i=0;i<num_pictures;i++)
		textures[i] = NULL;
	for(i=0;i<num_pictures;i++) {
		textures[i] = SDL_CreateTextureFromSurface(0,pictures[i]);
		if(textures[i] == NULL) {
			int j = 0;
			for(j=0;j<num_pictures;i++)
				if(textures[i] != NULL)
					SDL_DestroyTexture(textures[i]);
			free(textures);
			for(i=0;i<num_pictures;i++)
				SDL_FreeSurface(pictures[i]);
			free(pictures);
			SDL_DestroyRenderer(window);
			SDL_DestroyWindow(window);
			SDL_VideoQuit();
			printf("Could not create texture for SDL_shape.\n");
			exit(-6);
		}
	}
	
	SDL_Event event;
	int event_pending = 0,should_exit = 0;
	event_pending = SDL_PollEvent(&event);
	unsigned int current_picture = 0;
	SDL_WindowShapeMode mode = {ShapeModeDefault,1};
	SDL_SetWindowShape(window,pictures[current_picture],&mode);
	int mouse_down = 0;
	Uint32 format,access;
	SDL_Rect texture_dimensions = {0,0,0,0};
	SDL_QueryTexture(textures[current_picture],&format,&access,&texture_dimensions.w,&texture_dimensions.h);
	SDL_SetWindowSize(window,texture_dimensions.w,texture_dimensions.h);
	while(should_exit == 0) {
		event_pending = SDL_PollEvent(&event);
		if(event_pending == 1) {
			if(event.type == SDL_MOUSEBUTTONDOWN)
				mouse_down = 1;
			if(mouse_down && event.type == SDL_MOUSEBUTTONUP) {
				mouse_down = 0;
				current_picture += 1;
				if(current_picture >= num_pictures)
					current_picture = 0;
				SDL_QueryTexture(textures[current_picture],&format,&access,&texture_dimensions.w,&texture_dimensions.h);
				SDL_SetWindowSize(window,texture_dimensions.w,texture_dimensions.h);
				SDL_SetWindowShape(window,pictures[current_picture],&mode);
			}
			if(event.type == SDL_QUIT)
				should_exit = 1;
			event_pending = 0;
		}
		
		SDL_SelectRenderer(window);
	
		//Clear render-target to blue.
		SDL_SetRenderDrawColor(0x00,0x00,0xff,0xff);
		SDL_RenderClear();
		
		//Render the texture.
		SDL_RenderCopy(textures[current_picture],&texture_dimensions,&texture_dimensions);
		
		SDL_RenderPresent();
	}
	
	//Free the textures.
	for(i=0;i<num_pictures;i++)
		SDL_DestroyTexture(textures[i]);
	free(textures);
	//Destroy the window.
	SDL_DestroyWindow(window);
	//Free the original surfaces backing the textures.
	for(i=0;i<num_pictures;i++)
		SDL_FreeSurface(pictures[i]);
	free(pictures);
	//Call SDL_VideoQuit() before quitting.
	SDL_VideoQuit();
}
