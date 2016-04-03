/*

This is a simple SDL2 renderer benchmark tool. It test every active
renderer with different blend modes.

On AmigaOS, result may be affected by Workbench screen depth in case
pixel conversion takes place. For example compositing renderer works
best with 32-bit screen mode.

Some blend modes may not be supported for all renderers. These tests will give failure.

TODO:
- command line arguments for things like window size, iterations...
- texture color + alpha modulation
- take out randomness of primitive sizes

gcc -Wall -O3 sdl2benchmark.c -lSDL2 -lpthread -use-dynld -ldl

*/

#include "SDL2/SDL.h"

#define WIDTH 800
#define HEIGHT 600

#define RECTSIZE 100

#define ITERATIONS 100
#define OBJECTS 100

typedef struct {
	SDL_Renderer *renderer;
	SDL_Window *window;
	SDL_Texture *texture;
	SDL_Surface *surface;
	SDL_BlendMode mode;
	Uint32 width;
	Uint32 height;
	Uint32 rectsize;
	Uint32 texturewidth;
	Uint32 textureheight;
	Uint64 frequency;
	int iterations;
	int objects;
} Context;

typedef struct {
	const char *name;
	SDL_bool (*testfp)(Context *);
} Test;

typedef struct {
	const char *name;
	SDL_BlendMode mode;
} BlendMode;

/* Test function prototypes */
static SDL_bool testPoints(Context *);
static SDL_bool testLines(Context *);
static SDL_bool testFillRects(Context *);
static SDL_bool testRenderCopy(Context *);
static SDL_bool testRenderCopyEx(Context *);

/* Insert here new tests */
static Test tests[] = {
	{ "Points", testPoints },
	{ "Lines", testLines },
	{ "FillRects", testFillRects },
	{ "RenderCopy", testRenderCopy },
	{ "RenderCopyEx", testRenderCopyEx },
};

static BlendMode modes[] = {
	{ "None", SDL_BLENDMODE_NONE },
	{ "Blend", SDL_BLENDMODE_BLEND },
	{ "Add", SDL_BLENDMODE_ADD },
	{ "Mod", SDL_BLENDMODE_MOD }
};

static const char *getModeName(SDL_BlendMode mode)
{
	int i;
	
	static const char *unknown = "Unknown";

	for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
	{
		if (modes[i].mode == mode) {
			return modes[i].name;
		}
	}

	return unknown;
}

static void printInfo(Context *ctx)
{
	SDL_RendererInfo ri;
	int result;

	result = SDL_GetRendererInfo(ctx->renderer, &ri);

	if (result) {
		SDL_Log("Failed to get renderer info\n");
	} else {

		SDL_Log("Starting to test renderer called [%s], flags 0x%X\n", ri.name, ri.flags);
	}
}

static void updateWindowTitle(Context *ctx)
{
	SDL_RendererInfo ri;
	int result;

	result = SDL_GetRendererInfo(ctx->renderer, &ri);

	if (result) {
		SDL_Log("Failed to get renderer info\n");
	} else {
		char title[128];

		snprintf(title, sizeof(title), "Testing renderer: %s (blend mode %s)",
			ri.name, getModeName(ctx->mode));

		SDL_SetWindowTitle(ctx->window, title);
	}
}

static SDL_bool clearDisplay(Context *ctx)
{
	int result;

	result = SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

	if (result) {
		SDL_Log("[%s]Failed to set draw color\n", __FUNCTION__);
		return SDL_FALSE;
	}

	result = SDL_RenderClear(ctx->renderer);

	if (result) {
		SDL_Log("[%s]Failed to clear\n", __FUNCTION__);
		return SDL_FALSE;
	}

	SDL_RenderPresent(ctx->renderer);

	return SDL_TRUE;
}

static SDL_bool runTest(Context *ctx, Test *test)
{
	Uint64 start, finish;
	double duration;

	updateWindowTitle(ctx);

	if (!clearDisplay(ctx)) {
		return SDL_FALSE;
	}

	start = SDL_GetPerformanceCounter();

	if (!test->testfp(ctx)) {
		SDL_Log("[%s]...failed!\n", __FUNCTION__);
		return SDL_FALSE;
	}

	finish = SDL_GetPerformanceCounter();

	duration = (finish - start) / (double)ctx->frequency;

	SDL_Log("...%f seconds, %.1f frames per second\n", duration, ctx->iterations / duration);

	return SDL_TRUE;
}

static Uint32 getRand(Uint32 max)
{
	return rand() % max;
}

static SDL_bool setRandomColor(Context *ctx)
{
	int result;

	result = SDL_SetRenderDrawColor(ctx->renderer,
		getRand(256), getRand(256), getRand(256), getRand(256));

	if (result) {
		SDL_Log("[%s]Failed to set color\n", __FUNCTION__);
		return SDL_FALSE;
	}

	return SDL_TRUE;
}

static SDL_bool testPoints(Context *ctx)
{
	int iteration, result;

	result = SDL_SetRenderDrawBlendMode(ctx->renderer, ctx->mode);
	
	if (result) {
		SDL_Log("[%s]Failed to set blend mode\n", __FUNCTION__);
    	return SDL_FALSE;
	}

	for (iteration = 0; iteration < ctx->iterations; iteration++) {
		
		SDL_Point points[ctx->objects];
		int object;

		if (!setRandomColor(ctx)) {
			return SDL_FALSE;
		}

		for (object = 0; object < ctx->objects; object++) {
			points[object].x = getRand(ctx->width);
			points[object].y = getRand(ctx->height);
    	}

		result = SDL_RenderDrawPoints(ctx->renderer, points, ctx->objects);
		
		if (result) {
			SDL_Log("[%s]Failed to draw points\n", __FUNCTION__);
			return SDL_FALSE;
		}


		SDL_RenderPresent(ctx->renderer);
	}

	return SDL_TRUE;
}

static SDL_bool testLines(Context *ctx)
{
	int iteration, result;

	result = SDL_SetRenderDrawBlendMode(ctx->renderer, ctx->mode);

	if (result) {
		SDL_Log("[%s]Failed to set blend mode\n", __FUNCTION__);
    	return SDL_FALSE;
	}

	for (iteration = 0; iteration < ctx->iterations; iteration++) {

		SDL_Point points[ctx->objects];
		int object;

		if (!setRandomColor(ctx)) {
			return SDL_FALSE;
		}

		for (object = 0; object < ctx->objects; object++) {
			points[object].x = getRand(ctx->width);
			points[object].y = getRand(ctx->height);
    	}

		result = SDL_RenderDrawLines(ctx->renderer, points, ctx->objects);

		if (result) {
			SDL_Log("[%s]Failed to draw lines\n", __FUNCTION__);
			return SDL_FALSE;
		}


		SDL_RenderPresent(ctx->renderer);
	}

	return SDL_TRUE;
}

static SDL_bool testFillRects(Context *ctx)
{
	int iteration, result;

	result = SDL_SetRenderDrawBlendMode(ctx->renderer, ctx->mode);

	if (result) {
		SDL_Log("[%s]Failed to set blend mode\n", __FUNCTION__);
    	return SDL_FALSE;
	}

	for (iteration = 0; iteration < ctx->iterations; iteration++) {

		SDL_Rect rects[ctx->objects];
		int object;

		if (!setRandomColor(ctx)) {
			return SDL_FALSE;
		}

		for (object = 0; object < ctx->objects; object++) {
			rects[object].x = getRand(ctx->width - ctx->rectsize);
			rects[object].y = getRand(ctx->height - ctx->rectsize);
			rects[object].w = getRand(ctx->rectsize);
			rects[object].h = getRand(ctx->rectsize);
    	}

		result = SDL_RenderFillRects(ctx->renderer, rects, ctx->objects);

		if (result) {
			SDL_Log("[%s]Failed to draw lines\n", __FUNCTION__);
			return SDL_FALSE;
		}


		SDL_RenderPresent(ctx->renderer);
	}

	return SDL_TRUE;
}

static SDL_bool prepareTexture(Context *ctx)
{
	int result;
	
	ctx->texture = SDL_CreateTextureFromSurface(ctx->renderer, ctx->surface);

	if (!ctx->texture) {
		SDL_Log("[%s]Failed to create texture\n");
		return SDL_FALSE;
	}

	result = SDL_SetTextureBlendMode(ctx->texture, ctx->mode);

	if (result) {
		SDL_Log("[%s]Failed to set texture blend mode\n", __FUNCTION__);
    	return SDL_FALSE;
	}

	ctx->texturewidth = ctx->surface->w;
	ctx->textureheight = ctx->surface->h;

	return SDL_TRUE;
}

static SDL_bool testRenderCopy(Context *ctx)
{
	int iteration, result;

	if (!prepareTexture(ctx)) {
		return SDL_FALSE;
	}

	for (iteration = 0; iteration < ctx->iterations; iteration++) {

		//int object;

		//for (object = 0; object < ctx->objects; object++) {
    		SDL_Rect rect;

			/* Scale and place texture randomly */
			rect.x = getRand(ctx->width - 2 * ctx->texturewidth);
			rect.y = getRand(ctx->height - 2 * ctx->textureheight);
			rect.w = getRand(2 * ctx->texturewidth);
			rect.h = getRand(2 * ctx->textureheight);

			result = SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, &rect);

			if (result) {
				SDL_Log("[%s]Failed to draw texture\n", __FUNCTION__);
				return SDL_FALSE;
			}
		//}

		SDL_RenderPresent(ctx->renderer);
	}

	return SDL_TRUE;
}

static SDL_bool testRenderCopyEx(Context *ctx)
{
	int iteration, result;

	if (!prepareTexture(ctx)) {
		return SDL_FALSE;
	}

	for (iteration = 0; iteration < ctx->iterations; iteration++) {

		//int object;

		//for (object = 0; object < ctx->objects; object++) {
    		SDL_Rect rect;

			/* Scale, rotate and place randomly */
			rect.x = getRand(ctx->width);
			rect.y = getRand(ctx->height);
			rect.w = getRand(2 * ctx->texturewidth);
			rect.h = getRand(2 * ctx->textureheight);

			result = SDL_RenderCopyEx(
				ctx->renderer,
				ctx->texture,
				NULL,
				&rect,
				getRand(360),
				NULL,
				SDL_FLIP_NONE);

			if (result) {
				SDL_Log("[%s]Failed to draw texture\n", __FUNCTION__);
				return SDL_FALSE;
			}
		//}

		SDL_RenderPresent(ctx->renderer);
	}

	return SDL_TRUE;
}

static void runTestSuite(Context *ctx)
{
	int m, t;

	for (t = 0; t < sizeof(tests) / sizeof(tests[0]); t++) {
		for (m = 0; m < sizeof(modes) / sizeof(modes[0]); m++) {
			ctx->mode = modes[m].mode;

			SDL_Log("%s, blend mode: %s...\n", tests[t].name, getModeName(ctx->mode));
			
			runTest(ctx, &tests[t]);
		}
	}
}

static void initContext(Context *ctx)
{
	SDL_memset(ctx, 0, sizeof(ctx));

	ctx->frequency = SDL_GetPerformanceFrequency();

	ctx->width = WIDTH;
	ctx->height = HEIGHT;
	ctx->rectsize = RECTSIZE;
	ctx->iterations = ITERATIONS;
	ctx->objects = OBJECTS;

	SDL_Log("Parameters: width %d, height %d, iterations %d, objects %d\n",
		ctx->width, ctx->height, ctx->iterations, ctx->objects);
}

int main(int argc, char **argv)
{
	Context ctx;

	if (SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Init failed\n");
		return -1;
	}

	SDL_Log("SDL2 renderer benchmark v. 0.1\n");

	initContext(&ctx);

	ctx.surface = SDL_LoadBMP("sample.bmp");
	
	if (ctx.surface) {

		SDL_Log("Image size %d*%d\n", ctx.surface->w, ctx.surface->h);

		ctx.window = SDL_CreateWindow(
			"SDL2 benchmark",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			ctx.width,
			ctx.height,
			0);

		if (ctx.window) {

			int r;

			SDL_Log("Pixel format 0x%X\n", SDL_GetWindowPixelFormat(ctx.window));

			for (r = 0; r < SDL_GetNumRenderDrivers(); r++) {

				ctx.renderer = SDL_CreateRenderer(ctx.window, r, 0);

				if (ctx.renderer) {

				    printInfo(&ctx);

					runTestSuite(&ctx);

					if (ctx.texture) {
				    	SDL_DestroyTexture(ctx.texture);
					}

					SDL_DestroyRenderer(ctx.renderer);

				} else {
					SDL_Log("Failed to create renderer\n");
				}
			}

			SDL_DestroyWindow(ctx.window);
		} else {
			SDL_Log("Failed to create window\n");
		}

		SDL_FreeSurface(ctx.surface);
	} else {
		SDL_Log("Failed do load image\n");
	}

	SDL_Log("Bye bye\n");

	SDL_Quit();

	return 0;
}
