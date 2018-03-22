/*
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#include <stdlib.h>
#include <stdio.h>

#include <switch.h>
#include <video/SDL_sysvideo.h>
#include "SDL2/SDL.h"

int main(int argc, char *argv[])
{
    SDL_Event event;
    SDL_Window *window;
    SDL_Renderer *renderer;
    int done = 0;

    // redirect stdout to emulators
    consoleDebugInit(debugDevice_SVC);
    stdout = stderr;

    // mandatory at least on switch, else gfx is not properly closed
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        printf("SDL_Init: %s\n", SDL_GetError());
        return -1;
    }

    // create a 800x600 centered window for demonstration.
    // if SDL_WINDOW_FULLSCREEN flag is passed, the window will be hardware scaled to fit switch screen.
    // maximum window dimension is currently limited to 1280x720
    window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    if (!window) {
        printf("SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // switch only support software renderer for now
    renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // open CONTROLLER_PLAYER_1 and CONTROLLER_PLAYER_2
    // when connected, both joycons are mapped to joystick #0,
    // else joycons are individually mapped to joystick #0, joystick #1, ...
    // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L45
    for (int i = 0; i < 2; i++) {
        if (SDL_JoystickOpen(i) == NULL) {
            printf("SDL_JoystickOpen: %s\n", SDL_GetError());
            SDL_Quit();
            return -1;
        }
    }

    while (!done) {

        while (SDL_PollEvent(&event)) {

            switch (event.type) {

            case SDL_JOYAXISMOTION:
                printf("Joystick %d axis %d value: %d\n",
                       event.jaxis.which,
                       event.jaxis.axis, event.jaxis.value);
                break;

            case SDL_JOYBUTTONDOWN:
                printf("Joystick %d button %d down\n",
                       event.jbutton.which, event.jbutton.button);
                // seek for joystick #0 down (B)
                // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L51
                if (event.jbutton.which == 0 && event.jbutton.button == 1) {
                    done = 1;
                }
                break;

            default:
                break;
            }
        }

        for (int i = 0; i < 100; i++) {

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_Rect bg = {0, 0, window->w, window->h};
            SDL_RenderFillRect(renderer, &bg);

            // R
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_Rect r = {0, 0, 64, 64};
            SDL_RenderFillRect(renderer, &r);

            // G
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_Rect g = {64, 0, 64, 64};
            SDL_RenderFillRect(renderer, &g);

            // B
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            SDL_Rect b = {128, 0, 64, 64};
            SDL_RenderFillRect(renderer, &b);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();

    return 0;
}
