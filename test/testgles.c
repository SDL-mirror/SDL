#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "common.h"

#ifdef __IPHONEOS__
#define HAVE_OPENGLES
#endif

#ifdef HAVE_OPENGLES

#include "SDL_opengles.h"

static CommonState *state;
static SDL_GLContext context;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    if (context) {
        /* SDL_GL_MakeCurrent(0, NULL); *//* doesn't do anything */
        SDL_GL_DeleteContext(context);
    }
    CommonQuit(state);
    exit(rc);
}

static void
Render()
{
    static GLubyte color[8][4] = { {255, 0, 0, 0},
    {255, 0, 0, 255},
    {0, 255, 0, 255},
    {0, 255, 0, 255},
    {0, 255, 0, 255},
    {255, 255, 255, 255},
    {255, 0, 255, 255},
    {0, 0, 255, 255}
    };
    static GLfloat cube[8][3] = { {0.5, 0.5, -0.5},
    {0.5f, -0.5f, -0.5f},
    {-0.5f, -0.5f, -0.5f},
    {-0.5f, 0.5f, -0.5f},
    {-0.5f, 0.5f, 0.5f},
    {0.5f, 0.5f, 0.5f},
    {0.5f, -0.5f, 0.5f},
    {-0.5f, -0.5f, 0.5f}
    };
    static GLubyte indices[36] = { 0, 3, 4,
        4, 5, 0,
        0, 5, 6,
        6, 1, 0,
        6, 7, 2,
        2, 1, 6,
        7, 4, 3,
        3, 2, 7,
        5, 4, 7,
        7, 6, 5,
        2, 3, 1,
        3, 0, 1
    };


    /* Do our drawing, too. */
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Draw the cube */
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, color);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, cube);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);

    glMatrixMode(GL_MODELVIEW);
    glRotatef(5.0, 1.0, 1.0, 1.0);
}

int
main(int argc, char *argv[])
{
    int fsaa, accel;
    int value;
    int i, done;
    SDL_DisplayMode mode;
    SDL_Event event;
    Uint32 then, now, frames;

    /* Initialize parameters */
    fsaa = 0;
    accel = 0;

    /* Initialize test framework */
    state = CommonCreateState(argv, SDL_INIT_VIDEO);
    if (!state) {
        return 1;
    }
    for (i = 1; i < argc;) {
        int consumed;

        consumed = CommonArg(state, i);
        if (consumed == 0) {
            if (SDL_strcasecmp(argv[i], "--fsaa") == 0) {
                ++fsaa;
                consumed = 1;
            } else if (SDL_strcasecmp(argv[i], "--accel") == 0) {
                ++accel;
                consumed = 1;
            } else {
                consumed = -1;
            }
        }
        if (consumed < 0) {
            fprintf(stderr, "Usage: %s %s [--fsaa] [--accel]\n", argv[0],
                    CommonUsage(state));
            quit(1);
        }
        i += consumed;
    }

    /* Set OpenGL parameters */
    state->window_flags |= SDL_WINDOW_OPENGL;
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    if (fsaa) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, fsaa);
    }
    if (accel) {
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    }
    if (!CommonInit(state)) {
        quit(2);
    }

    /* Create OpenGL context */
    context = SDL_GL_CreateContext(state->windows[0]);
    if (!context) {
        fprintf(stderr, "SDL_GL_CreateContext(): %s\n", SDL_GetError());
        quit(2);
    }

    if (state->render_flags & SDL_RENDERER_PRESENTVSYNC) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }

    SDL_GetCurrentDisplayMode(&mode);
    printf("Screen BPP: %d\n", SDL_BITSPERPIXEL(mode.format));
    printf("\n");
    printf("Vendor     : %s\n", glGetString(GL_VENDOR));
    printf("Renderer   : %s\n", glGetString(GL_RENDERER));
    printf("Version    : %s\n", glGetString(GL_VERSION));
    printf("Extensions : %s\n", glGetString(GL_EXTENSIONS));
    printf("\n");

    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &value);
    printf("SDL_GL_RED_SIZE: requested %d, got %d\n", 5, value);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &value);
    printf("SDL_GL_GREEN_SIZE: requested %d, got %d\n", 5, value);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &value);
    printf("SDL_GL_BLUE_SIZE: requested %d, got %d\n", 5, value);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &value);
    printf("SDL_GL_DEPTH_SIZE: requested %d, got %d\n", 16, value);
    if (fsaa) {
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &value);
        printf("SDL_GL_MULTISAMPLEBUFFERS: requested 1, got %d\n", value);
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &value);
        printf("SDL_GL_MULTISAMPLESAMPLES: requested %d, got %d\n", fsaa,
               value);
    }
    if (accel) {
        SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &value);
        printf("SDL_GL_ACCELERATED_VISUAL: requested 1, got %d\n", value);
    }

    /* Set rendering settings */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(-2.0, 2.0, -2.0, 2.0, -20.0, 20.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glShadeModel(GL_SMOOTH);

    /* Main render loop */
    frames = 0;
    then = SDL_GetTicks();
    done = 0;
    while (!done) {
        /* Check for events */
        ++frames;
        while (SDL_PollEvent(&event)) {
            CommonEvent(state, &event, &done);
        }
        for (i = 0; i < state->num_windows; ++i) {
            int w, h;
            SDL_GL_MakeCurrent(state->windows[i], context);
            SDL_GetWindowSize(state->windows[i], &w, &h);
            glViewport(0, 0, w, h);
            Render();
            SDL_GL_SwapWindow(state->windows[i]);
        }
    }

    /* Print out some timing information */
    now = SDL_GetTicks();
    if (now > then) {
        printf("%2.2f frames per second\n",
               ((double) frames * 1000) / (now - then));
    }
    quit(0);
    return 0;
}

#else /* HAVE_OPENGLES */

int
main(int argc, char *argv[])
{
    printf("No OpenGL ES support on this system\n");
    return 1;
}

#endif /* HAVE_OPENGLES */
