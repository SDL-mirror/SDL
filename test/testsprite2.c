/* Simple program:  Move N sprites around on the screen as fast as possible */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "common.h"

#define NUM_SPRITES	100
#define MAX_SPEED 	1
#define BACKGROUND  0x00FFFFFF

static CommonState *state;
static int num_sprites;
static SDL_TextureID *sprites;
static SDL_Rect *positions;
static SDL_Rect *velocities;
static int sprite_w, sprite_h;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    if (sprites) {
        SDL_free(sprites);
    }
    if (positions) {
        SDL_free(positions);
    }
    if (velocities) {
        SDL_free(velocities);
    }
    CommonQuit(state);
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
    for (i = 0; i < state->num_windows; ++i) {
        SDL_SelectRenderer(state->windows[i]);
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
    SDL_RenderFill(NULL, BACKGROUND);
    /*
       for (i = 0; i < num_sprites; ++i) {
       position = &positions[i];
       SDL_RenderFill(position, BACKGROUND);
       }
     */
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
    int i, done;
    SDL_Event event;
    Uint32 then, now, frames;

    /* Initialize parameters */
    num_sprites = NUM_SPRITES;

    /* Initialize test framework */
    state = CommonCreateState(argv, SDL_INIT_VIDEO);
    if (!state) {
        return 1;
    }
    for (i = 1; i < argc;) {
        int consumed;

        consumed = CommonArg(state, i);
        if (consumed == 0) {
            num_sprites = SDL_atoi(argv[i]);
            consumed = 1;
        }
        if (consumed < 0) {
            fprintf(stderr, "Usage: %s %s", argv[0], CommonUsage(state));
            quit(1);
        }
        i += consumed;
    }
    if (!CommonInit(state)) {
        quit(2);
    }

    /* Create the windows, initialize the renderers, and load the textures */
    sprites =
        (SDL_TextureID *) SDL_malloc(state->num_windows * sizeof(*sprites));
    if (!sprites) {
        fprintf(stderr, "Out of memory!\n");
        quit(2);
    }
    for (i = 0; i < state->num_windows; ++i) {
        SDL_SelectRenderer(state->windows[i]);
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
        positions[i].x = rand() % (state->window_w - sprite_w);
        positions[i].y = rand() % (state->window_h - sprite_h);
        positions[i].w = sprite_w;
        positions[i].h = sprite_h;
        velocities[i].x = 0;
        velocities[i].y = 0;
        while (!velocities[i].x && !velocities[i].y) {
            velocities[i].x = (rand() % (MAX_SPEED * 2 + 1)) - MAX_SPEED;
            velocities[i].y = (rand() % (MAX_SPEED * 2 + 1)) - MAX_SPEED;
        }
    }

    /* Main render loop */
    frames = 0;
    then = SDL_GetTicks();
    done = 0;
    while (!done) {
        /* Check for events */
        ++frames;
        while (SDL_PollEvent(&event)) {
            CommonEvent(state, &event, &done);
            switch (event.type) {
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_EXPOSED:
                    SDL_SelectRenderer(event.window.windowID);
                    SDL_RenderFill(NULL, BACKGROUND);
                    break;
                }
                break;
            default:
                break;
            }
        }
        for (i = 0; i < state->num_windows; ++i) {
            MoveSprites(state->windows[i], sprites[i]);
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
