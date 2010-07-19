#include <stdlib.h>
#include <math.h>
#include <SDL_events.h>
#include <SDL_rect.h>
#include <SDL_pixels.h>
#include <SDL_video.h>
#include <SDL_shape.h>
#include <SDL_keysym.h>
#include <SDL_timer.h>

#define SHAPED_WINDOW_X 150
#define SHAPED_WINDOW_Y 150
#define SHAPED_WINDOW_DIMENSION 640

#define TICK_INTERVAL 18

void render(SDL_Window* window,SDL_Texture *texture,SDL_Rect texture_dimensions) {
	SDL_SelectRenderer(window);
	
	//Clear render-target to blue.
	SDL_SetRenderDrawColor(0x00,0x00,0xff,0xff);
	SDL_RenderClear();
	
	//Render the texture.
	SDL_RenderCopy(texture,&texture_dimensions,&texture_dimensions);
	
	SDL_RenderPresent();
}

static Uint32 next_time;

Uint32 time_left() {
	Uint32 now = SDL_GetTicks();
	if(next_time <= now)
		return 0;
	else
		return next_time - now;
}

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
		pictures[i] = SDL_LoadBMP(argv[i+1]);
		if(pictures[i] == NULL) {
			int j = 0;
			for(j=0;j<num_pictures;j++)
				if(pictures[j] != NULL)
					SDL_FreeSurface(pictures[j]);
			free(pictures);
			SDL_VideoQuit();
			printf("Could not load surface from named bitmap file.\n");
			exit(-3);
		}
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
	int button_down = 0;
	Uint32 format = 0,access = 0;
	SDL_Rect texture_dimensions = {0,0,0,0};
	SDL_QueryTexture(textures[current_picture],&format,&access,&texture_dimensions.w,&texture_dimensions.h);
	SDL_SetWindowSize(window,texture_dimensions.w,texture_dimensions.h);
	SDL_SetWindowShape(window,pictures[current_picture],&mode);
	next_time = SDL_GetTicks() + TICK_INTERVAL;
	while(should_exit == 0) {
		event_pending = SDL_PollEvent(&event);
		if(event_pending == 1) {
			if(event.type == SDL_KEYDOWN) {
				button_down = 1;
				if(event.key.keysym.sym == SDLK_ESCAPE)
					should_exit = 1;
			}
			if(button_down && event.type == SDL_KEYUP) {
				button_down = 0;
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
		render(window,textures[current_picture],texture_dimensions);
		SDL_Delay(time_left());
		next_time += TICK_INTERVAL;
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
