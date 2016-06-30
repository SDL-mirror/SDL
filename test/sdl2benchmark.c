/*

This is a simple SDL2 renderer benchmark tool. It test every active
renderer with different blend modes.

On AmigaOS, result may be affected by Workbench screen depth in case
pixel conversion takes place. For example compositing renderer works
best with 32-bit screen mode.

Some blend modes may not be supported for all renderers. These tests will give failure.

TODO:
- command line arguments for things like window size, iterations...

gcc -Wall -O3 sdl2benchmark.c -lSDL2 -lpthread -use-dynld

*/

#include "SDL2/SDL.h"

#define WIDTH 800
#define HEIGHT 600

#define RECTSIZE 100

#define ITERATIONS 100
#define OBJECTS 100

#define SLEEP 0

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
    Uint32 iterations;
    Uint32 objects;
    Uint32 sleep;
    Uint32 frames;
    Uint32 operations;
    Uint32 *buffer;
} Context;

typedef struct {
    const char *name;
    SDL_bool (*testfp)(Context *);
    SDL_bool usetexture;
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
static SDL_bool testColorModulation(Context *);
static SDL_bool testAlphaModulation(Context *);
static SDL_bool testUpdateTexture(Context *);
static SDL_bool testReadPixels(Context *);

/* Insert here new tests */
static Test tests[] = {
    { "Points", testPoints, SDL_FALSE },
    { "Lines", testLines, SDL_FALSE },
    { "FillRects", testFillRects, SDL_FALSE },
    { "RenderCopy", testRenderCopy, SDL_TRUE },
    { "RenderCopyEx", testRenderCopyEx, SDL_TRUE },
    { "Color modulation", testColorModulation, SDL_TRUE },
    { "Alpha modulation", testAlphaModulation, SDL_TRUE },
    { "UpdateTexture", testUpdateTexture, SDL_TRUE },
    { "ReadPixels", testReadPixels, SDL_TRUE }
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

static void updateWindowTitle(Context *ctx, Test *test)
{
    SDL_RendererInfo ri;
    int result;

    result = SDL_GetRendererInfo(ctx->renderer, &ri);

    if (result) {
        SDL_Log("Failed to get renderer info\n");
    } else {
        char title[128];

        snprintf(title, sizeof(title), "Testing '%s' renderer - %s, blend mode %s",
            ri.name, test->name, getModeName(ctx->mode));

        SDL_SetWindowTitle(ctx->window, title);
    }
}

static void render(Context *ctx)
{
    SDL_RenderPresent(ctx->renderer);
    ctx->frames++;
}

static SDL_bool clearDisplay(Context *ctx)
{
    int result;

    if (ctx->mode != SDL_BLENDMODE_MOD) {
        result = SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    } else {
        result = SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    }

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

static float interpolate(float min, float max, float percentage)
{
    return min + percentage * (max - min);
}

static Uint32 getRand(Uint32 max)
{
    return rand() % max;
}

static void makeRandomTexture(Context *ctx)
{
    int i;

    SDL_memset(ctx->buffer, 0, ctx->texturewidth * ctx->textureheight * sizeof(Uint32));

    for (i = 0; i < 100; i++) {
        ctx->buffer[
            getRand(ctx->texturewidth) +
            getRand(ctx->textureheight) * ctx->texturewidth] = 0xFFFFFFFF;
    }
}

static SDL_bool prepareTexture(Context *ctx)
{
    int result;

    result = SDL_SetColorKey(ctx->surface, 1, 0);

    if (result) {
        SDL_Log("[%s]Failed to set color key\n", __FUNCTION__);
        return SDL_FALSE;
    }

    if (ctx->texture) {
        SDL_Log("Old texture!\n");
    }

    ctx->texture = SDL_CreateTextureFromSurface(ctx->renderer, ctx->surface);

    if (!ctx->texture) {
        SDL_Log("[%s]Failed to create texture\n", __FUNCTION__);
        return SDL_FALSE;
    }

    result = SDL_SetTextureBlendMode(ctx->texture, ctx->mode);

    if (result) {
        SDL_Log("[%s]Failed to set texture blend mode\n", __FUNCTION__);
        
        SDL_DestroyTexture(ctx->texture);
        ctx->texture = NULL;

        return SDL_FALSE;
    }

    ctx->texturewidth = ctx->surface->w;
    ctx->textureheight = ctx->surface->h;

    ctx->buffer = SDL_malloc(ctx->texturewidth * ctx->textureheight * sizeof(Uint32));

    if (!ctx->buffer) {
        SDL_Log("[%s]Failed to alloc buffer\n", __FUNCTION__);
        SDL_DestroyTexture(ctx->texture);
        ctx->texture = NULL;
        return SDL_FALSE;
    }

    makeRandomTexture(ctx);

    return SDL_TRUE;
}

static SDL_bool prepareTest(Context *ctx, Test *test)
{
    updateWindowTitle(ctx, test);

    if (!clearDisplay(ctx)) {
        return SDL_FALSE;
    }

    if (test->usetexture) {
        if (!prepareTexture(ctx)) {
            return SDL_FALSE;
        }
    }

    ctx->frames = 0;
    ctx->operations = 0;

    return SDL_TRUE;
}

static void afterTest(Context *ctx)
{
    if (ctx->texture) {
        SDL_DestroyTexture(ctx->texture);
        ctx->texture = NULL;
    }

    if (ctx->buffer) {
        SDL_free(ctx->buffer);
        ctx->buffer = NULL;
    }

    if (ctx->sleep) {
        SDL_Delay(ctx->sleep);
    }
}

static SDL_bool runTest(Context *ctx, Test *test)
{
    Uint64 start, finish;
    double duration;
    float fps, ops;

    if (!prepareTest(ctx, test)) {
        return SDL_FALSE;
    }

    start = SDL_GetPerformanceCounter();

    if (!test->testfp(ctx)) {
        afterTest(ctx);
        return SDL_FALSE;
    }

    finish = SDL_GetPerformanceCounter();

    duration = (finish - start) / (double)ctx->frequency;

    if (duration == 0.0) {
        SDL_Log("Division by zero!\n");
        afterTest(ctx);
        return SDL_FALSE;
    }

    fps = ctx->frames / duration;
    ops = ctx->operations / duration;

    if (fps == ops) {
        SDL_Log("%s [mode: %s]...%d frames drawn in %.3f seconds => %.1f frames per second\n",
            test->name, getModeName(ctx->mode), ctx->frames, duration, fps);
    } else {
        SDL_Log("%s [mode: %s]...%d frames drawn in %.3f seconds => %.1f frames per second, %.1f operations per second\n",
            test->name, getModeName(ctx->mode), ctx->frames, duration, fps, ops);
    }

    afterTest(ctx);

    return SDL_TRUE;
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

static SDL_bool testPointsInner(Context *ctx, SDL_bool linemode)
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

        if (linemode) {
            result = SDL_RenderDrawLines(ctx->renderer, points, ctx->objects);
        } else {
            result = SDL_RenderDrawPoints(ctx->renderer, points, ctx->objects);
        }
        
        ctx->operations++;

        if (result) {
            SDL_Log("[%s]Failed to draw points\n", __FUNCTION__);
            return SDL_FALSE;
        }


        render(ctx);
    }

    return SDL_TRUE;
}

static SDL_bool testPoints(Context *ctx)
{
    return testPointsInner(ctx, SDL_FALSE);
}

static SDL_bool testLines(Context *ctx)
{
    return testPointsInner(ctx, SDL_TRUE);
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
        int rectsize;

        if (!setRandomColor(ctx)) {
            return SDL_FALSE;
        }

        rectsize = ctx->rectsize + iteration;

        for (object = 0; object < ctx->objects; object++) {
            rects[object].x = getRand(ctx->width - rectsize);
            rects[object].y = getRand(ctx->height - rectsize);
            rects[object].w = rectsize;
            rects[object].h = rectsize;
        }

        result = SDL_RenderFillRects(ctx->renderer, rects, ctx->objects);

        if (result) {
            SDL_Log("[%s]Failed to draw lines\n", __FUNCTION__);
            return SDL_FALSE;
        }

        ctx->operations++;

        render(ctx);
    }

    return SDL_TRUE;
}

static SDL_bool testRenderCopyInner(Context *ctx, SDL_bool ex) {
    int iteration;

    for (iteration = 0; iteration < ctx->iterations; iteration++) {

        int result;
        int w, h;
        float scale;

        //int object;
        scale = interpolate(0.5f, 2.0f, (float)iteration / ctx->iterations);

        w = ctx->texturewidth * scale;
        h = ctx->textureheight * scale;

        //for (object = 0; object < ctx->objects; object++) {
            SDL_Rect rect;

            rect.x = getRand(ctx->width - w);
            rect.y = getRand(ctx->height - h);
            rect.w = w;
            rect.h = h;

            if (!ex) {
                result = SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, &rect);
            } else {
                result = SDL_RenderCopyEx(
                    ctx->renderer,
                    ctx->texture,
                    NULL,
                    &rect,
                    getRand(360),
                    NULL,
                    SDL_FLIP_NONE);            
            }

            if (result) {
                SDL_Log("[%s]Failed to draw texture\n", __FUNCTION__);
                return SDL_FALSE;
            }

            ctx->operations++;
        //}

        render(ctx);
    }

    return SDL_TRUE;
}

static SDL_bool testRenderCopy(Context *ctx)
{
    return testRenderCopyInner(ctx, SDL_FALSE);
}

static SDL_bool testRenderCopyEx(Context *ctx)
{
    return testRenderCopyInner(ctx, SDL_TRUE);
}

static SDL_bool testColorModulation(Context *ctx)
{
    SDL_bool result = SDL_TRUE;

    SDL_Color colors[] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {127, 127, 127, 255}
    };

    size_t count = sizeof(colors) / sizeof(colors[0]);

    int iterations = ctx->iterations;
    int i;

    ctx->iterations = 1;

    for (i = 0; i < iterations; i++) {
        int c = i % count;

        if (SDL_SetTextureColorMod(ctx->texture, colors[c].r, colors[c].g, colors[c].b)) {
            SDL_Log("[%s]Failed to set color modulation\n", __FUNCTION__);
            result = SDL_FALSE;
            break;
        }    

        result = testRenderCopyInner(ctx, SDL_FALSE);
    }

    ctx->iterations = iterations;

    return result;
}

static SDL_bool testAlphaModulation(Context *ctx)
{
    if (SDL_SetTextureAlphaMod(ctx->texture, 127)) {
        SDL_Log("[%s]Failed to set alpha modulation\n", __FUNCTION__);
        return SDL_FALSE;
    }

    return testRenderCopyInner(ctx, SDL_FALSE);
}

static SDL_bool testUpdateTexture(Context *ctx)
{
    SDL_bool result = SDL_TRUE;

    SDL_Color colors[] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {127, 127, 127, 255}
    };

    size_t count = sizeof(colors) / sizeof(colors[0]);

    int iterations = ctx->iterations;
    int i;

    /* Divide our test so that total execution time doesn't take much longer than other tests */
    ctx->iterations /= count;

    for (i = 0; i < count; i++) {

        if (SDL_SetTextureColorMod(ctx->texture, colors[i].r, colors[i].g, colors[i].b)) {
            SDL_Log("[%s]Failed to set color modulation\n", __FUNCTION__);
            result = SDL_FALSE;
            break;
        }

        if (SDL_UpdateTexture(ctx->texture, NULL, ctx->buffer, ctx->texturewidth * sizeof(Uint32))) {
            SDL_Log("[%s]Failed to update texture\n", __FUNCTION__);
            result = SDL_FALSE;
            break;
        }

        ctx->operations++;

        result = testRenderCopyInner(ctx, SDL_FALSE);
    }

    ctx->iterations = iterations;

    return result;
}

static SDL_bool testReadPixels(Context *ctx)
{
    SDL_bool result = SDL_TRUE;

    int i;

    for (i = 0; i < ctx->iterations; i++) {

        SDL_Rect rect;
        rect.x = getRand(ctx->width - ctx->texturewidth);
        rect.y = getRand(ctx->height - ctx->textureheight);
        rect.w = ctx->texturewidth;
        rect.h = ctx->textureheight;

        if (SDL_RenderReadPixels(
            ctx->renderer,
            &rect,
            SDL_PIXELFORMAT_ARGB8888,
            ctx->buffer,
            ctx->texturewidth * sizeof(Uint32)) != 0) {

            SDL_Log("[%s]Failed to read pixels\n", __FUNCTION__);

            result = SDL_FALSE;
            break;
        }

        ctx->operations++;
    }

    return result;
}

static void runTestSuite(Context *ctx)
{
    int m, t;

    for (t = 0; t < sizeof(tests) / sizeof(tests[0]); t++) {
        for (m = 0; m < sizeof(modes) / sizeof(modes[0]); m++) {
            ctx->mode = modes[m].mode;
            
            runTest(ctx, &tests[t]);
        }
    }
}

static void checkParameters(Context *ctx, int argc, char **argv)
{
    if (argc > 2) {
        ctx->sleep = atoi(argv[2]);
    }

    if (argc > 1) {
        ctx->iterations = atoi(argv[1]);
    }
}

static void initContext(Context *ctx, int argc, char **argv)
{
    SDL_memset(ctx, 0, sizeof(*ctx));

    ctx->frequency = SDL_GetPerformanceFrequency();

    ctx->width = WIDTH;
    ctx->height = HEIGHT;
    ctx->rectsize = RECTSIZE;
    ctx->iterations = ITERATIONS;
    ctx->objects = OBJECTS;
    ctx->sleep = SLEEP;

    checkParameters(ctx, argc, argv);

    SDL_Log("Parameters: width %d, height %d, iterations %d, objects %d, sleep %d\n",
        ctx->width, ctx->height, ctx->iterations, ctx->objects, ctx->sleep);
}

static void checkPixelFormat(Context *ctx)
{
    Uint32 pf;
    
    pf = SDL_GetWindowPixelFormat(ctx->window);

    SDL_Log("Pixel format 0x%X (%s)\n", pf, SDL_GetPixelFormatName(pf));

    if (pf != SDL_PIXELFORMAT_ARGB8888 && pf != SDL_PIXELFORMAT_RGB888) {
        SDL_Log("NOTE: window's pixel format not ARGB8888 - possible bitmap conversion can slow down\n");
    }
}

int main(int argc, char **argv)
{
    Context ctx;

    if (SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Init failed\n");
        return -1;
    }

    SDL_Log("SDL2 renderer benchmark v. 0.2\n");

    initContext(&ctx, argc, argv);

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

            checkPixelFormat(&ctx);

            for (r = 0; r < SDL_GetNumRenderDrivers(); r++) {

                ctx.renderer = SDL_CreateRenderer(ctx.window, r, 0);

                if (ctx.renderer) {

                    printInfo(&ctx);

                    runTestSuite(&ctx);

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
