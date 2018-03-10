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

SDL_Renderer *renderer = NULL;

int done = 0;

int main(int argc, char *argv[])
{
    consoleDebugInit(debugDevice_SVC);
    stdout = stderr;

    SDL_Window *window;

    if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer) < 0) {
        printf("SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Event event;

    printf("entering main loop\n");

    while (!done) {

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN) {
                done = 1;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect r = {0, 0, 64, 64};
        SDL_RenderFillRect(renderer, &r);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_Rect g = {64, 0, 64, 64};
        SDL_RenderFillRect(renderer, &g);

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Rect b = {128, 0, 64, 64};
        SDL_RenderFillRect(renderer, &b);

        SDL_RenderPresent(renderer);
    }

    return 0;
}
