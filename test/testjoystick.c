/*
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

/* Simple program to test the SDL joystick routines */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#ifdef __IPHONEOS__
#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	480
#else
#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#endif

#define MAX_NUM_AXES 6
#define MAX_NUM_HATS 2

void
WatchJoystick(SDL_Joystick * joystick)
{
    SDL_Window *window;
    SDL_Renderer *screen;
    const char *name;
    int i, done;
    SDL_Event event;
    int x, y;
    SDL_Rect axis_area[MAX_NUM_AXES][2];
    int axis_draw[MAX_NUM_AXES];
    SDL_Rect hat_area[MAX_NUM_HATS][2];
    int hat_draw[MAX_NUM_HATS];
    Uint8 hat_pos;

    /* Create a window to display joystick axis position */
    window = SDL_CreateWindow("Joystick Test", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
                              SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
        return;
    }

    screen = SDL_CreateRenderer(window, -1, 0);
    if (screen == NULL) {
        fprintf(stderr, "Couldn't create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return;
    }

    SDL_SetRenderDrawColor(screen, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(screen);

    /* Print info about the joystick we are watching */
    name = SDL_JoystickName(SDL_JoystickIndex(joystick));
    printf("Watching joystick %d: (%s)\n", SDL_JoystickIndex(joystick),
           name ? name : "Unknown Joystick");
    printf("Joystick has %d axes, %d hats, %d balls, and %d buttons\n",
           SDL_JoystickNumAxes(joystick), SDL_JoystickNumHats(joystick),
           SDL_JoystickNumBalls(joystick), SDL_JoystickNumButtons(joystick));

    /* Initialize drawing rectangles */
    memset(axis_area, 0, (sizeof axis_area));
    memset(axis_draw, 0, (sizeof axis_draw));
    memset(hat_area, 0, (sizeof hat_area));
    memset(hat_draw, 0, (sizeof hat_draw));

    /* Loop, getting joystick events! */
    done = 0;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_JOYAXISMOTION:
                printf("Joystick %d axis %d value: %d\n",
                       event.jaxis.which,
                       event.jaxis.axis, event.jaxis.value);
                break;
            case SDL_JOYHATMOTION:
                printf("Joystick %d hat %d value:",
                       event.jhat.which, event.jhat.hat);
                if (event.jhat.value == SDL_HAT_CENTERED)
                    printf(" centered");
                if (event.jhat.value & SDL_HAT_UP)
                    printf(" up");
                if (event.jhat.value & SDL_HAT_RIGHT)
                    printf(" right");
                if (event.jhat.value & SDL_HAT_DOWN)
                    printf(" down");
                if (event.jhat.value & SDL_HAT_LEFT)
                    printf(" left");
                printf("\n");
                break;
            case SDL_JOYBALLMOTION:
                printf("Joystick %d ball %d delta: (%d,%d)\n",
                       event.jball.which,
                       event.jball.ball, event.jball.xrel, event.jball.yrel);
                break;
            case SDL_JOYBUTTONDOWN:
                printf("Joystick %d button %d down\n",
                       event.jbutton.which, event.jbutton.button);
                break;
            case SDL_JOYBUTTONUP:
                printf("Joystick %d button %d up\n",
                       event.jbutton.which, event.jbutton.button);
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym != SDLK_ESCAPE) {
                    break;
                }
                /* Fall through to signal quit */
            case SDL_QUIT:
                done = 1;
                break;
            default:
                break;
            }
        }
        /* Update visual joystick state */
        for (i = 0; i < SDL_JoystickNumButtons(joystick); ++i) {
            SDL_Rect area;

            area.x = i * 34;
            area.y = SCREEN_HEIGHT - 34;
            area.w = 32;
            area.h = 32;
            if (SDL_JoystickGetButton(joystick, i) == SDL_PRESSED) {
                SDL_SetRenderDrawColor(screen, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
            } else {
                SDL_SetRenderDrawColor(screen, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
            }
            SDL_RenderFillRect(screen, &area);
            SDL_RenderPresent(screen);
        }

        for (i = 0;
             i < SDL_JoystickNumAxes(joystick) / 2
             && i < SDL_arraysize(axis_area); ++i) {

            /* Erase previous axes */
            SDL_SetRenderDrawColor(screen, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(screen, &axis_area[i][axis_draw[i]]);

            /* Draw the X/Y axis */
            axis_draw[i] = !axis_draw[i];
            x = (((int) SDL_JoystickGetAxis(joystick, i * 2 + 0)) + 32768);
            x *= SCREEN_WIDTH;
            x /= 65535;
            if (x < 0) {
                x = 0;
            } else if (x > (SCREEN_WIDTH - 16)) {
                x = SCREEN_WIDTH - 16;
            }
            y = (((int) SDL_JoystickGetAxis(joystick, i * 2 + 1)) + 32768);
            y *= SCREEN_HEIGHT;
            y /= 65535;
            if (y < 0) {
                y = 0;
            } else if (y > (SCREEN_HEIGHT - 16)) {
                y = SCREEN_HEIGHT - 16;
            }

            axis_area[i][axis_draw[i]].x = (Sint16) x;
            axis_area[i][axis_draw[i]].y = (Sint16) y;
            axis_area[i][axis_draw[i]].w = 16;
            axis_area[i][axis_draw[i]].h = 16;

            SDL_SetRenderDrawColor(screen, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(screen, &axis_area[i][axis_draw[i]]);
            SDL_RenderPresent(screen);
        }

        for (i = 0;
             i < SDL_JoystickNumHats(joystick)
             && i < SDL_arraysize(hat_area); ++i) {

            /* Erase previous hat position */
            SDL_SetRenderDrawColor(screen, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(screen, &hat_area[i][hat_draw[i]]);

            hat_draw[i] = !hat_draw[i];

            /* Derive the new position */
            hat_pos = SDL_JoystickGetHat(joystick, i);

            hat_area[i][hat_draw[i]].x = SCREEN_WIDTH/2;
            hat_area[i][hat_draw[i]].y = SCREEN_HEIGHT/2;
            hat_area[i][hat_draw[i]].w = 8;
            hat_area[i][hat_draw[i]].h = 8;

            if (hat_pos & SDL_HAT_UP) {
                hat_area[i][hat_draw[i]].y = 0;
            } else if (hat_pos & SDL_HAT_DOWN) {
                hat_area[i][hat_draw[i]].y = SCREEN_HEIGHT-8;
            }

            if (hat_pos & SDL_HAT_LEFT) {
                hat_area[i][hat_draw[i]].x = 0;
            } else if (hat_pos & SDL_HAT_RIGHT) {
                hat_area[i][hat_draw[i]].x = SCREEN_WIDTH-8;
            }

            /* Draw it */
            SDL_SetRenderDrawColor(screen, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(screen, &hat_area[i][hat_draw[i]]);
            SDL_RenderPresent(screen);
        }
    }

    SDL_DestroyRenderer(screen);
    SDL_DestroyWindow(window);
}

int
main(int argc, char *argv[])
{
    const char *name;
    int i;
    SDL_Joystick *joystick;

    /* Initialize SDL (Note: video is required to start event loop) */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    /* Print information about the joysticks */
    printf("There are %d joysticks attached\n", SDL_NumJoysticks());
    for (i = 0; i < SDL_NumJoysticks(); ++i) {
        name = SDL_JoystickName(i);
        printf("Joystick %d: %s\n", i, name ? name : "Unknown Joystick");
        joystick = SDL_JoystickOpen(i);
        if (joystick == NULL) {
            fprintf(stderr, "SDL_JoystickOpen(%d) failed: %s\n", i,
                    SDL_GetError());
        } else {
            printf("       axes: %d\n", SDL_JoystickNumAxes(joystick));
            printf("      balls: %d\n", SDL_JoystickNumBalls(joystick));
            printf("       hats: %d\n", SDL_JoystickNumHats(joystick));
            printf("    buttons: %d\n", SDL_JoystickNumButtons(joystick));
            SDL_JoystickClose(joystick);
        }
    }

    if (argv[1]) {
        joystick = SDL_JoystickOpen(atoi(argv[1]));
        if (joystick == NULL) {
            printf("Couldn't open joystick %d: %s\n", atoi(argv[1]),
                   SDL_GetError());
        } else {
            WatchJoystick(joystick);
            SDL_JoystickClose(joystick);
        }
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

    return (0);
}
