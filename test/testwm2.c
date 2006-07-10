/* Simple program:  Move N sprites around on the screen as fast as possible */

#include "SDL.h"

#define NUM_WINDOWS 2
#define WINDOW_W    640
#define WINDOW_H    480

static int num_windows;
static SDL_WindowID *windows;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    if (windows) {
        SDL_free(windows);
    }
    SDL_Quit();
    exit(rc);
}

int
main(int argc, char *argv[])
{
    int window_w, window_h;
    int i, done;
    SDL_Event event;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    num_windows = NUM_WINDOWS;
    window_w = WINDOW_W;
    window_h = WINDOW_H;
    while (argc > 1) {
        if (strcmp(argv[argc - 1], "-width") == 0) {
            window_w = atoi(argv[argc]);
            --argc;
        } else if (strcmp(argv[argc - 1], "-height") == 0) {
            window_h = atoi(argv[argc]);
            --argc;
        } else {
            fprintf(stderr, "Usage: %s [-width] [-height]\n", argv[0]);
            quit(1);
        }
    }

    /* Create the windows */
    windows = (SDL_WindowID *) SDL_malloc(num_windows * sizeof(*windows));
    if (!windows) {
        fprintf(stderr, "Out of memory!\n");
        quit(2);
    }
    for (i = 0; i < num_windows; ++i) {
        char title[32];
        int x, y;

        SDL_snprintf(title, sizeof(title), "testwm %d", i + 1);
        if (i == 0) {
            x = SDL_WINDOWPOS_CENTERED;
            y = SDL_WINDOWPOS_CENTERED;
        } else {
            x = SDL_WINDOWPOS_UNDEFINED;
            y = SDL_WINDOWPOS_UNDEFINED;
        }
        windows[i] =
            SDL_CreateWindow(title, x, y, window_w, window_h,
                             SDL_WINDOW_SHOWN);
        if (!windows[i]) {
            fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
            quit(2);
        }
    }

    /* Loop, blitting sprites and waiting for a keystroke */
    done = 0;
    while (!done) {
        /* Check for events */
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
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

    quit(0);
}

/* vi: set ts=4 sw=4 expandtab: */
