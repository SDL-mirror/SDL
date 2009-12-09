/* Simple program:  Move N sprites around on the screen as fast as possible */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "common.h"

#define NUM_SPRITES	100
#define MAX_SPEED 	1

static CommonState *state;
static int num_sprites;
static SDL_TextureID *sprites;
static SDL_bool cycle_color;
static SDL_bool cycle_alpha;
static int cycle_direction = 1;
static int current_alpha = 0;
static int current_color = 0;
static SDL_Rect *positions;
static SDL_Rect *velocities;
static int sprite_w, sprite_h;
static SDL_BlendMode blendMode = SDL_BLENDMODE_MASK;
static SDL_TextureScaleMode scaleMode = SDL_TEXTURESCALEMODE_NONE;

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
    } else {
        switch (temp->format->BitsPerPixel) {
        case 15:
            SDL_SetColorKey(temp, SDL_SRCCOLORKEY,
                            (*(Uint16 *) temp->pixels) & 0x00007FFF);
            break;
        case 16:
            SDL_SetColorKey(temp, SDL_SRCCOLORKEY, *(Uint16 *) temp->pixels);
            break;
        case 24:
            SDL_SetColorKey(temp, SDL_SRCCOLORKEY,
                            (*(Uint32 *) temp->pixels) & 0x00FFFFFF);
            break;
        case 32:
            SDL_SetColorKey(temp, SDL_SRCCOLORKEY, *(Uint32 *) temp->pixels);
            break;
        }
    }

    /* Create textures from the image */
    for (i = 0; i < state->num_windows; ++i) {
        SDL_SelectRenderer(state->windows[i]);
        sprites[i] = SDL_CreateTextureFromSurface(0, temp);
        if (!sprites[i]) {
		SDL_SetColorKey(temp, 0, 0);
		sprites[i] = SDL_CreateTextureFromSurface(0, temp);
	}
        if (!sprites[i]) {
            fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
            SDL_FreeSurface(temp);
            return (-1);
        }
        SDL_SetTextureBlendMode(sprites[i], blendMode);
        SDL_SetTextureScaleMode(sprites[i], scaleMode);
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
    SDL_Rect temp;
    SDL_Rect *position, *velocity;

    SDL_SelectRenderer(window);

    /* Query the sizes */
    SDL_GetWindowSize(window, &window_w, &window_h);

    /* Cycle the color and alpha, if desired */
    if (cycle_color) {
        current_color += cycle_direction;
        if (current_color < 0) {
            current_color = 0;
            cycle_direction = -cycle_direction;
        }
        if (current_color > 255) {
            current_color = 255;
            cycle_direction = -cycle_direction;
        }
        SDL_SetTextureColorMod(sprite, 255, (Uint8) current_color,
                               (Uint8) current_color);
    }
    if (cycle_alpha) {
        current_alpha += cycle_direction;
        if (current_alpha < 0) {
            current_alpha = 0;
            cycle_direction = -cycle_direction;
        }
        if (current_alpha > 255) {
            current_alpha = 255;
            cycle_direction = -cycle_direction;
        }
        SDL_SetTextureAlphaMod(sprite, (Uint8) current_alpha);
    }

    /* Draw a gray background */
    SDL_SetRenderDrawColor(0xA0, 0xA0, 0xA0, 0xFF);
    SDL_RenderRect(NULL);

    /* Test points */
    SDL_SetRenderDrawColor(0xFF, 0x00, 0x00, 0xFF);
    SDL_RenderPoint(0, 0);
    SDL_RenderPoint(window_w-1, 0);
    SDL_RenderPoint(0, window_h-1);
    SDL_RenderPoint(window_w-1, window_h-1);

    /* Test horizontal and vertical lines */
    SDL_SetRenderDrawColor(0x00, 0xFF, 0x00, 0xFF);
    SDL_RenderLine(1, 0, window_w-2, 0);
    SDL_RenderLine(1, window_h-1, window_w-2, window_h-1);
    SDL_RenderLine(0, 1, 0, window_h-2);
    SDL_RenderLine(window_w-1, 1, window_w-1, window_h-2);

    /* Test fill and copy */
    SDL_SetRenderDrawColor(0xFF, 0xFF, 0xFF, 0xFF);
    temp.x = 1;
    temp.y = 1;
    temp.w = sprite_w;
    temp.h = sprite_h;
    SDL_RenderRect(&temp);
    SDL_RenderCopy(sprite, NULL, &temp);
    temp.x = window_w-sprite_w-1;
    temp.y = 1;
    temp.w = sprite_w;
    temp.h = sprite_h;
    SDL_RenderRect(&temp);
    SDL_RenderCopy(sprite, NULL, &temp);
    temp.x = 1;
    temp.y = window_h-sprite_h-1;
    temp.w = sprite_w;
    temp.h = sprite_h;
    SDL_RenderRect(&temp);
    SDL_RenderCopy(sprite, NULL, &temp);
    temp.x = window_w-sprite_w-1;
    temp.y = window_h-sprite_h-1;
    temp.w = sprite_w;
    temp.h = sprite_h;
    SDL_RenderRect(&temp);
    SDL_RenderCopy(sprite, NULL, &temp);

    /* Test diagonal lines */
    SDL_SetRenderDrawColor(0x00, 0xFF, 0x00, 0xFF);
    SDL_RenderLine(sprite_w, sprite_h,
                   window_w-sprite_w-2, window_h-sprite_h-2);
    SDL_RenderLine(window_w-sprite_w-2, sprite_h,
                   sprite_w, window_h-sprite_h-2);

    /* Move the sprite, bounce at the wall, and draw */
    n = 0;
    for (i = 0; i < num_sprites; ++i) {
        position = &positions[i];
        velocity = &velocities[i];
        position->x += velocity->x;
        if ((position->x < 0) || (position->x >= (window_w - sprite_w))) {
            velocity->x = -velocity->x;
            position->x += velocity->x;
        }
        position->y += velocity->y;
        if ((position->y < 0) || (position->y >= (window_h - sprite_h))) {
            velocity->y = -velocity->y;
            position->y += velocity->y;
        }

        /* Blit the sprite onto the screen */
        SDL_RenderCopy(sprite, NULL, position);
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
            consumed = -1;
            if (SDL_strcasecmp(argv[i], "--blend") == 0) {
                if (argv[i + 1]) {
                    if (SDL_strcasecmp(argv[i + 1], "none") == 0) {
                        blendMode = SDL_BLENDMODE_NONE;
                        consumed = 2;
                    } else if (SDL_strcasecmp(argv[i + 1], "mask") == 0) {
                        blendMode = SDL_BLENDMODE_MASK;
                        consumed = 2;
                    } else if (SDL_strcasecmp(argv[i + 1], "blend") == 0) {
                        blendMode = SDL_BLENDMODE_BLEND;
                        consumed = 2;
                    } else if (SDL_strcasecmp(argv[i + 1], "add") == 0) {
                        blendMode = SDL_BLENDMODE_ADD;
                        consumed = 2;
                    } else if (SDL_strcasecmp(argv[i + 1], "mod") == 0) {
                        blendMode = SDL_BLENDMODE_MOD;
                        consumed = 2;
                    }
                }
            } else if (SDL_strcasecmp(argv[i], "--scale") == 0) {
                if (argv[i + 1]) {
                    if (SDL_strcasecmp(argv[i + 1], "none") == 0) {
                        scaleMode = SDL_TEXTURESCALEMODE_NONE;
                        consumed = 2;
                    } else if (SDL_strcasecmp(argv[i + 1], "fast") == 0) {
                        scaleMode = SDL_TEXTURESCALEMODE_FAST;
                        consumed = 2;
                    } else if (SDL_strcasecmp(argv[i + 1], "slow") == 0) {
                        scaleMode = SDL_TEXTURESCALEMODE_SLOW;
                        consumed = 2;
                    } else if (SDL_strcasecmp(argv[i + 1], "best") == 0) {
                        scaleMode = SDL_TEXTURESCALEMODE_BEST;
                        consumed = 2;
                    }
                }
            } else if (SDL_strcasecmp(argv[i], "--cyclecolor") == 0) {
                cycle_color = SDL_TRUE;
                consumed = 1;
            } else if (SDL_strcasecmp(argv[i], "--cyclealpha") == 0) {
                cycle_alpha = SDL_TRUE;
                consumed = 1;
            } else if (SDL_isdigit(*argv[i])) {
                num_sprites = SDL_atoi(argv[i]);
                consumed = 1;
            }
        }
        if (consumed < 0) {
            fprintf(stderr,
                    "Usage: %s %s [--blend none|mask|blend|add|mod] [--scale none|fast|slow|best] [--cyclecolor] [--cyclealpha]\n",
                    argv[0], CommonUsage(state));
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
        SDL_SetRenderDrawColor(0xA0, 0xA0, 0xA0, 0xFF);
        SDL_RenderRect(NULL);
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
    if (scaleMode != SDL_TEXTURESCALEMODE_NONE) {
        sprite_w += sprite_w / 2;
        sprite_h += sprite_h / 2;
    }
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
                    SDL_SetRenderDrawColor(0xA0, 0xA0, 0xA0, 0xFF);
                    SDL_RenderRect(NULL);
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
        double fps = ((double) frames * 1000) / (now - then);
        printf("%2.2f frames per second\n", fps);
    }
    quit(0);
}

/* vi: set ts=4 sw=4 expandtab: */
