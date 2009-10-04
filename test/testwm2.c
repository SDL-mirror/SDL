
#include <stdlib.h>
#include <stdio.h>

#include "common.h"

static CommonState *state;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    CommonQuit(state);
    exit(rc);
}

int
main(int argc, char *argv[])
{
    int i, done;
    SDL_Event event;

    /* Initialize test framework */
    state = CommonCreateState(argv, SDL_INIT_VIDEO);
    if (!state) {
        return 1;
    }
    state->skip_renderer = SDL_TRUE;
    for (i = 1; i < argc;) {
        int consumed;

        consumed = CommonArg(state, i);
        if (consumed == 0) {
            consumed = -1;
        }
        if (consumed < 0) {
            fprintf(stderr, "Usage: %s %s\n", argv[0], CommonUsage(state));
            quit(1);
        }
        i += consumed;
    }
    if (!CommonInit(state)) {
        quit(2);
    }

    /* Main render loop */
    done = 0;
    while (!done) {
        /* Check for events */
        while (SDL_PollEvent(&event)) {
            CommonEvent(state, &event, &done);
        }
    }
    quit(0);
	// keep the compiler happy ...
	return(0);
}

/* vi: set ts=4 sw=4 expandtab: */
