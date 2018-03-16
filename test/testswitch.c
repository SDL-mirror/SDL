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
#include "SDL2/SDL.h"

#define WINDOW_WIDTH    1280
#define WINDOW_HEIGHT   720

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

    // switch only support software renderer for now
    if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer) < 0) {
        printf("SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // open CONTROLLER_P1_AUTO
    // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L45
    if (SDL_JoystickOpen(0) == NULL) {
        printf("SDL_JoystickOpen: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    while (!done) {

        while (SDL_PollEvent(&event)) {
            // seek for (B) button press
            // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L51
            if (event.type == SDL_JOYBUTTONDOWN && event.jbutton.button == KEY_B) {
                done = 1;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

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

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();

    return 0;
}
