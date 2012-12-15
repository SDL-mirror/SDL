/*
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "SDL.h"
#include "SDL_test.h"

#include "tests/testsuites.h"

static SDLTest_CommonState *state;


/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDLTest_CommonQuit(state);
    exit(rc);
}

int
main(int argc, char *argv[])
{
    int result;
    int i;

    /* Initialize test framework */
    state = SDLTest_CommonCreateState(argv, SDL_INIT_VIDEO);
    if (!state) {
        return 1;
    }

    // Needed?
    // state->window_flags |= SDL_WINDOW_RESIZABLE;

    /* Parse commandline */
    for (i = 1; i < argc;) {
        int consumed;

        consumed = SDLTest_CommonArg(state, i);
        if (consumed == 0) {
            consumed = -1;
/* Parse additional parameters

            if (SDL_strcasecmp(argv[i], "--BLAH") == 0) {
                if (argv[i + 1]) {
                    if (SDL_strcasecmp(argv[i + 1], "BLUB") == 0) {
                        blah = blub;
                        consumed = 2;
                    }
                }
            } else if (SDL_strcasecmp(argv[i], "--BINGO") == 0) {
                bingo = SDL_TRUE;
                consumed = 1;
            }
*/
        }
        if (consumed < 0) {
            fprintf(stderr,
                    "Usage: %s %s [--BLAH BLUB --BINGO]\n",
                    argv[0], SDLTest_CommonUsage(state));
            quit(1);
        }
        i += consumed;
    }

    /* Initialize common state */    
    if (!SDLTest_CommonInit(state)) {
        quit(2);
    }

    /* Create the windows, initialize the renderers */
    for (i = 0; i < state->num_windows; ++i) {
        SDL_Renderer *renderer = state->renderers[i];
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);
    }

    /* Call Harness */
    // TODO: pass custom parameters
    result = SDLTest_RunSuites(testSuites, NULL, 0, 1);
//int SDLTest_RunSuites(SDLTest_TestSuiteReference *testSuites, char *userRunSeed, Uint64 userExecKey, int testIterations);
    
    /* Shutdown everything */
    quit(result);        
    return(result);
}

/* vi: set ts=4 sw=4 expandtab: */
