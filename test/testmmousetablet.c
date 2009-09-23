#include <stdio.h>
#include "SDL.h"

SDL_Surface *screen;
int quit = 0;

int
main(int argc, char *argv[])
{
    SDL_Event event;
    int mice;
    int i;
    printf("Initing...\n");
    if (SDL_Init(0) != 0) {
        return 1;
    }
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        return 1;
    } else {
        screen = SDL_SetVideoMode(640, 480, 32, SDL_DOUBLEBUF);
    }
    mice = SDL_GetNumMice();
    printf("%d pointing devices found\n", mice);
    for (i = 0; i < mice; ++i) {
        printf("device index: %d name:%s\n", i, SDL_GetMouseName(i));
    }
    while (quit != 1) {
        if (SDL_PollEvent(&event) == 0) {
        } else {
            switch (event.type) {
            case SDL_MOUSEMOTION:
                printf
                    ("Device id: %d x: %d y: %d relx: %d rely: %d pressure: %d\n \
					pressure_max: %d pressure_min: %d current cursor:%d\n",
                     event.motion.which, event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel,
                     event.motion.pressure, event.motion.pressure_max, event.motion.pressure_min,
                     event.motion.cursor);
                break;
            case SDL_PROXIMITYIN:
                printf("proximity in id: %d x: %d y: %d\n",
                       (int) event.proximity.which, event.proximity.x,
                       event.proximity.y);
                break;
            case SDL_PROXIMITYOUT:
                printf("proximity out id: %d x: %d y: %d\n",
                       (int) event.proximity.which, event.proximity.x,
                       event.proximity.y);
                break;
            case SDL_MOUSEBUTTONDOWN:
                printf("mouse button down id: %d button:%d\n",
                       event.button.which, event.button.button);
                break;
            case SDL_MOUSEBUTTONUP:
                printf("mouse button up id: %d button: %d\n",
                       event.button.which, event.button.button);
                break;
            case SDL_QUIT:
                printf("Quitting\n");
                SDL_QuitSubSystem(SDL_INIT_VIDEO);
                SDL_Quit();
                quit = 1;
                break;
            }
        }
    }
    return 0;
}
