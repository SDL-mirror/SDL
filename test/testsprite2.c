/* Simple program:  Move N sprites around on the screen as fast as possible */

#include <stdlib.h>
#include <time.h>

#include "SDL.h"

#define NUM_WINDOWS 4
#define WINDOW_W    640
#define WINDOW_H    480
#define NUM_SPRITES	100
#define MAX_SPEED 	1
#define BACKGROUND  0x00FFFFFF

static int num_windows;
static int num_sprites;
static SDL_WindowID *windows;
static SDL_TextureID *sprites;
static SDL_Rect *positions;
static SDL_Rect *velocities;
static int sprite_w, sprite_h;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    if (windows) {
        SDL_free(windows);
    }
    if (sprites) {
        SDL_free(sprites);
    }
    if (positions) {
        SDL_free(positions);
    }
    if (velocities) {
        SDL_free(velocities);
    }
    SDL_Quit();
    exit(rc);
}

int
LoadSprite(char *file)
{
    int i;
    SDL_Surface *temp;

    /* Load the sprite image */
    temp = SDL_LoadBMP(file);
    if (temp == NULL) {
        fprintf(stderr, "Couldn't load %s: %s", file, SDL_GetError());
        return (-1);
    }
    sprite_w = temp->w;
    sprite_h = temp->h;

    /* Set transparent pixel as the pixel at (0,0) */
    if (temp->format->palette) {
        SDL_SetColorKey(temp, SDL_SRCCOLORKEY, *(Uint8 *) temp->pixels);
    }

    /* Create textures from the image */
    for (i = 0; i < num_windows; ++i) {
        SDL_SelectRenderer(windows[i]);
        sprites[i] =
            SDL_CreateTextureFromSurface(0, SDL_TextureAccess_Remote, temp);
        if (!sprites[i]) {
            fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
            SDL_FreeSurface(temp);
            return (-1);
        }
    }
    SDL_FreeSurface(temp);

    /* We're ready to roll. :) */
    return (0);
}

void
MoveSprites(SDL_WindowID window, SDL_TextureID sprite)
{
    int i, n;
    int window_w, window_h;
    SDL_Rect area, *position, *velocity;

    SDL_SelectRenderer(window);

    /* Query the sizes */
    SDL_GetWindowSize(window, &window_w, &window_h);

    /* Move the sprite, bounce at the wall, and draw */
    n = 0;
    for (i = 0; i < num_sprites; ++i) {
        position = &positions[i];
        SDL_RenderFill(position, BACKGROUND);
    }
    for (i = 0; i < num_sprites; ++i) {
        position = &positions[i];
        velocity = &velocities[i];
        position->x += velocity->x;
        if ((position->x < 0) || (position->x >= (window_w - sprite_w))) {
            velocity->x = -velocity->x;
            position->x += velocity->x;
        }
        position->y += velocity->y;
        if ((position->y < 0) || (position->y >= (window_h - sprite_w))) {
            velocity->y = -velocity->y;
            position->y += velocity->y;
        }

        /* Blit the sprite onto the screen */
        SDL_RenderCopy(sprite, NULL, position, SDL_TextureBlendMode_Mask,
                       SDL_TextureScaleMode_None);
    }

    /* Update the screen! */
    SDL_RenderPresent();
}

int
main(int argc, char *argv[])
{
    int window_w, window_h;
    Uint32 window_flags = SDL_WINDOW_SHOWN;
    SDL_DisplayMode *mode, fullscreen_mode;
    int i, done;
    SDL_Event event;
    Uint32 then, now, frames;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    num_windows = NUM_WINDOWS;
    num_sprites = NUM_SPRITES;
    window_w = WINDOW_W;
    window_h = WINDOW_H;
    while (argc > 1) {
        if (strcmp(argv[argc - 1], "-width") == 0) {
            window_w = atoi(argv[argc]);
            --argc;
        } else if (strcmp(argv[argc - 1], "-height") == 0) {
            window_h = atoi(argv[argc]);
            --argc;
        } else if (strcmp(argv[argc - 1], "-fullscreen") == 0) {
            num_windows = 1;
            window_flags |= SDL_WINDOW_FULLSCREEN;
            --argc;
        } else if (isdigit(argv[argc][0])) {
            num_sprites = atoi(argv[argc]);
        } else {
            fprintf(stderr,
                    "Usage: %s [-width] [-height] [numsprites]\n", argv[0]);
            quit(1);
        }
    }

    if (window_flags & SDL_WINDOW_FULLSCREEN) {
        SDL_zero(fullscreen_mode);
        fullscreen_mode.w = window_w;
        fullscreen_mode.h = window_h;
        SDL_SetFullscreenDisplayMode(&fullscreen_mode);
    }

    /* Create the windows, initialize the renderers, and load the textures */
    windows = (SDL_WindowID *) SDL_malloc(num_windows * sizeof(*windows));
    sprites = (SDL_TextureID *) SDL_malloc(num_windows * sizeof(*sprites));
    if (!windows || !sprites) {
        fprintf(stderr, "Out of memory!\n");
        quit(2);
    }
    for (i = 0; i < num_windows; ++i) {
        char title[32];

        SDL_snprintf(title, sizeof(title), "testsprite %d", i + 1);
        windows[i] =
            SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, window_w, window_h,
                             window_flags);
        if (!windows[i]) {
            fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
            quit(2);
        }

        if (SDL_CreateRenderer(windows[i], -1, 0) < 0) {
            fprintf(stderr, "Couldn't create renderer: %s\n", SDL_GetError());
            quit(2);
        }
        SDL_RenderFill(NULL, BACKGROUND);
    }
    if (LoadSprite("icon.bmp") < 0) {
        quit(2);
    }

    /* Allocate memory for the sprite info */
    positions = (SDL_Rect *) SDL_malloc(num_sprites * sizeof(SDL_Rect));
    velocities = (SDL_Rect *) SDL_malloc(num_sprites * sizeof(SDL_Rect));
    if (!positions || !velocities) {
        fprintf(stderr, "Out of memory!\n");
        quit(2);
    }
    srand(time(NULL));
    for (i = 0; i < num_sprites; ++i) {
        positions[i].x = rand() % (window_w - sprite_w);
        positions[i].y = rand() % (window_h - sprite_h);
        positions[i].w = sprite_w;
        positions[i].h = sprite_h;
        velocities[i].x = 0;
        velocities[i].y = 0;
        while (!velocities[i].x && !velocities[i].y) {
            velocities[i].x = (rand() % (MAX_SPEED * 2 + 1)) - MAX_SPEED;
            velocities[i].y = (rand() % (MAX_SPEED * 2 + 1)) - MAX_SPEED;
        }
    }

    /* Loop, blitting sprites and waiting for a keystroke */
    frames = 0;
    then = SDL_GetTicks();
    done = 0;
    while (!done) {
        /* Check for events */
        ++frames;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_EXPOSED:
                    SDL_SelectRenderer(event.window.windowID);
                    SDL_RenderFill(NULL, BACKGROUND);
                    break;
                case SDL_WINDOWEVENT_CLOSE:
                    done = 1;
                    break;
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
        for (i = 0; i < num_windows; ++i) {
            MoveSprites(windows[i], sprites[i]);
        }
    }

    /* Print out some timing information */
    now = SDL_GetTicks();
    if (now > then) {
        printf("%2.2f frames per second\n",
               ((double) frames * 1000) / (now - then));
    }
    quit(0);
}

/* vi: set ts=4 sw=4 expandtab: */
