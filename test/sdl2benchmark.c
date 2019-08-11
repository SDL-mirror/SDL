/*

This is a simple SDL2 renderer benchmark tool. It test every active
renderer with available blend modes.

On AmigaOS, result may be affected by Workbench screen depth in case
pixel conversion takes place. For example compositing renderer works
best with 32-bit screen mode.

Some blend modes may not be supported for all renderers. These tests
will give failure.

TODO:
- command line arguments for things like window size, iterations...

*/

#include "SDL2/SDL.h"

#define BENCHMARK_VERSION "0.6"

#define WIDTH 800
#define HEIGHT 600

#define RECTSIZE 100

#define DURATION 1.0
#define OBJECTS 100

#define SLEEP 0

#ifdef __amigaos4__
static const char stackCookie[] __attribute__((used)) = "$STACK:60000";
#endif

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
    double duration;
    Uint32 objects;
    Uint32 sleep;
    Uint32 frames;
    Uint32 operations;
    Uint32 *buffer;
    SDL_bool running;
    const char* rendname;
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
static const Test tests[] = {
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

static const BlendMode modes[] = {
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
        SDL_Log("Failed to get renderer info: %s\n", SDL_GetError());
    } else {

        SDL_Log("Starting to test renderer called [%s], flags 0x%X\n", ri.name, ri.flags);
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
        SDL_Log("[%s]Failed to set draw color: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    result = SDL_RenderClear(ctx->renderer);

    if (result) {
        SDL_Log("[%s]Failed to clear: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    SDL_RenderPresent(ctx->renderer);

    return SDL_TRUE;
}

/*
static float interpolate(float min, float max, float percentage)
{
    return min + percentage * (max - min);
}
*/

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
        SDL_Log("[%s]Failed to set color key: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    if (ctx->texture) {
        SDL_Log("Old texture!\n");
    }

    ctx->texture = SDL_CreateTextureFromSurface(ctx->renderer, ctx->surface);

    if (!ctx->texture) {
        SDL_Log("[%s]Failed to create texture: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    result = SDL_SetTextureBlendMode(ctx->texture, ctx->mode);

    if (result) {
        SDL_Log("[%s]Failed to set texture blend mode: %s\n", __FUNCTION__, SDL_GetError());

        SDL_DestroyTexture(ctx->texture);
        ctx->texture = NULL;

        return SDL_FALSE;
    }

    ctx->texturewidth = ctx->surface->w;
    ctx->textureheight = ctx->surface->h;

    ctx->buffer = SDL_malloc(ctx->texturewidth * ctx->textureheight * sizeof(Uint32));

    if (!ctx->buffer) {
        SDL_Log("[%s]Failed to allocate texture buffer: %s\n", __FUNCTION__, SDL_GetError());
        SDL_DestroyTexture(ctx->texture);
        ctx->texture = NULL;
        return SDL_FALSE;
    }

    makeRandomTexture(ctx);

    return SDL_TRUE;
}

static SDL_bool prepareTest(Context *ctx, const Test *test)
{
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

static SDL_bool runTest(Context *ctx, const Test *test)
{
    Uint64 start, finish;
    double duration;
    float fps, ops;

    if (!prepareTest(ctx, test)) {
        return SDL_FALSE;
    }

    start = SDL_GetPerformanceCounter();

    do {
        if (!test->testfp(ctx)) {
            afterTest(ctx);
            return SDL_FALSE;
        }

        finish = SDL_GetPerformanceCounter();

        duration = (finish - start) / (double)ctx->frequency;
    } while (duration < ctx->duration);

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
        SDL_Log("[%s]Failed to set color: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    return SDL_TRUE;
}

static SDL_bool testPointsInner(Context *ctx, SDL_bool linemode)
{
    int result = SDL_SetRenderDrawBlendMode(ctx->renderer, ctx->mode);

    if (result) {
        SDL_Log("[%s]Failed to set blend mode: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    SDL_Point points[ctx->objects];
    int object;

    if (!setRandomColor(ctx)) {
        return SDL_FALSE;
    }

    for (object = 0; object < ctx->objects; object++) {
        points[object].x = getRand(ctx->width);
        points[object].y = getRand(ctx->height);
    }

    result = linemode ?
        SDL_RenderDrawLines(ctx->renderer, points, ctx->objects) :
        SDL_RenderDrawPoints(ctx->renderer, points, ctx->objects);

    ctx->operations++;

    if (result) {
        SDL_Log("[%s]Failed to draw lines/points: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    render(ctx);

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
    int result = SDL_SetRenderDrawBlendMode(ctx->renderer, ctx->mode);

    if (result) {
        SDL_Log("[%s]Failed to set blend mode: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    SDL_Rect rects[ctx->objects];
    int object;

    if (!setRandomColor(ctx)) {
        return SDL_FALSE;
    }

    const int rectsize = ctx->rectsize + getRand(100); // + iteration

    for (object = 0; object < ctx->objects; object++) {
        rects[object].x = getRand(ctx->width - rectsize);
        rects[object].y = getRand(ctx->height - rectsize);
        rects[object].w = rectsize;
        rects[object].h = rectsize;
    }

    result = SDL_RenderFillRects(ctx->renderer, rects, ctx->objects);

    if (result) {
        SDL_Log("[%s]Failed to draw filled rectangles: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    ctx->operations++;

    render(ctx);

    return SDL_TRUE;
}

static SDL_bool testRenderCopyInner(Context *ctx, SDL_bool ex)
{
    int result;

    //int object;
    //const float scale = interpolate(0.5f, 2.0f, (float)ctx->iteration / ctx->iterations);
    const float scale = (getRand(4) + 1) / 2.0f;

    int w = ctx->texturewidth * scale;
    int h = ctx->textureheight * scale;

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
            SDL_Log("[%s]Failed to draw texture: %s\n", __FUNCTION__, SDL_GetError());
            return SDL_FALSE;
        }

        ctx->operations++;
    //}

    render(ctx);

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

static const SDL_Color colors[] = {
    {255, 0, 0, 255},
    {0, 255, 0, 255},
    {0, 0, 255, 255},
    {127, 127, 127, 255}
};

static const size_t count = sizeof(colors) / sizeof(colors[0]);

static SDL_bool testColorModulation(Context *ctx)
{
    static int i = 0;

    const int c = i++ % count;

    if (SDL_SetTextureColorMod(ctx->texture, colors[c].r, colors[c].g, colors[c].b)) {
        SDL_Log("[%s]Failed to set color modulation: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    return testRenderCopyInner(ctx, SDL_FALSE);
}

static SDL_bool testAlphaModulation(Context *ctx)
{
    if (SDL_SetTextureAlphaMod(ctx->texture, 127)) {
        SDL_Log("[%s]Failed to set alpha modulation: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    return testRenderCopyInner(ctx, SDL_FALSE);
}

static SDL_bool testUpdateTexture(Context *ctx)
{
    static int i = 0;

    const int c = i++ % count;

    if (SDL_SetTextureColorMod(ctx->texture, colors[c].r, colors[c].g, colors[c].b)) {
        SDL_Log("[%s]Failed to set color modulation: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    if (SDL_UpdateTexture(ctx->texture, NULL, ctx->buffer, ctx->texturewidth * sizeof(Uint32))) {
        SDL_Log("[%s]Failed to update texture: %s\n", __FUNCTION__, SDL_GetError());
        return SDL_FALSE;
    }

    ctx->operations++;

    return testRenderCopyInner(ctx, SDL_FALSE);
}

static SDL_bool testReadPixels(Context *ctx)
{
    SDL_bool result = SDL_TRUE;

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

        SDL_Log("[%s]Failed to read pixels: %s\n", __FUNCTION__, SDL_GetError());

        result = SDL_FALSE;
    }

    ctx->operations++;

    return result;
}

static void checkEvents(Context *ctx)
{
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_KEYDOWN) {
            SDL_KeyboardEvent *ke = (SDL_KeyboardEvent *)&e;

            if (ke->keysym.sym == SDLK_ESCAPE) {
                SDL_Log("Quitting...\n");
                ctx->running = SDL_FALSE;
            }
        }
    }
}

static void runTestSuite(Context *ctx)
{
    int m, t;

    for (t = 0; t < sizeof(tests) / sizeof(tests[0]); t++) {
        for (m = 0; m < sizeof(modes) / sizeof(modes[0]); m++) {
            ctx->mode = modes[m].mode;

            runTest(ctx, &tests[t]);

            checkEvents(ctx);

            if (!ctx->running) {
                return;
            }
        }
    }
}

/* TODO: need proper handling */
static void checkParameters(Context *ctx, int argc, char **argv)
{
    if (argc > 3) {
        ctx->sleep = atoi(argv[3]);
    }

    if (argc > 2) {
        ctx->duration = atof(argv[2]);
    }

    if (argc > 1) {
        ctx->rendname = argv[1];
    }
}

static void initContext(Context *ctx, int argc, char **argv)
{
    SDL_memset(ctx, 0, sizeof(Context));

    ctx->frequency = SDL_GetPerformanceFrequency();

    ctx->width = WIDTH;
    ctx->height = HEIGHT;
    ctx->rectsize = RECTSIZE;
    ctx->duration = DURATION;
    ctx->objects = OBJECTS;
    ctx->sleep = SLEEP;
    ctx->running = SDL_TRUE;

    checkParameters(ctx, argc, argv);

    SDL_Log("Parameters: width %d, height %d, renderer name '%s', duration %.3f s, objects %u, sleep %u\n",
        ctx->width, ctx->height, ctx->rendname, ctx->duration, ctx->objects, ctx->sleep);
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

static void testRenderer(Context *ctx)
{
    if (ctx->renderer) {
        printInfo(ctx);

        runTestSuite(ctx);

        SDL_DestroyRenderer(ctx->renderer);
    } else {
        SDL_Log("Failed to create renderer: %s\n", SDL_GetError());
    }
}

static void testSpecificRenderer(Context *ctx)
{
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, ctx->rendname);

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, 0);

    testRenderer(ctx);
}

static void testAllRenderers(Context *ctx)
{
    int r;

    for (r = 0; r < SDL_GetNumRenderDrivers(); r++) {

        ctx->renderer = SDL_CreateRenderer(ctx->window, r, 0);

        testRenderer(ctx);

        if (!ctx->running) {
            break;
        }
    }
}

int main(int argc, char **argv)
{
    Context ctx;
    SDL_version linked;

    if (SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Init failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GetVersion(&linked);

    SDL_Log("SDL2 renderer benchmark v. " BENCHMARK_VERSION " (SDL version %d.%d.%d)\n",
        linked.major, linked.minor, linked.patch);

    SDL_Log("This tool measures the speed of various 2D drawing features\n");
    SDL_Log("Press ESC key to quit\n");

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
            SDL_WINDOW_FULLSCREEN);

        if (ctx.window) {

            checkPixelFormat(&ctx);

            if (ctx.rendname) {
                testSpecificRenderer(&ctx);
            } else {
                testAllRenderers(&ctx);
            }

            SDL_DestroyWindow(ctx.window);
        } else {
            SDL_Log("Failed to create window: %s\n", SDL_GetError());
        }

        SDL_FreeSurface(ctx.surface);

    } else {
        SDL_Log("Failed do load image: %s\n", SDL_GetError());
    }

    SDL_Log("Bye bye\n");

    SDL_Quit();

    return 0;
}
