/*
 *	rectangles.c
 *	written by Holmes Futrell
 *	use however you want
 */

#include "SDL.h"
#include <time.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

int
randomInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}

void
render(void)
{

    Uint8 r, g, b;
    /*  Come up with a random rectangle */
    SDL_Rect rect;
    rect.w = randomInt(64, 128);
    rect.h = randomInt(64, 128);
    rect.x = randomInt(0, SCREEN_WIDTH);
    rect.y = randomInt(0, SCREEN_HEIGHT);

    /* Come up with a random color */
    r = randomInt(50, 255);
    g = randomInt(50, 255);
    b = randomInt(50, 255);

    /*  Fill the rectangle in the color */
    SDL_RenderFill(r, g, b, 255, &rect);

    /* update screen */
    SDL_RenderPresent();

}

int
main(int argc, char *argv[])
{

    SDL_WindowID windowID;
    int done;
    SDL_Event event;

    /* initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Could not initialize SDL\n");
    }

    /* seed random number generator */
    srand(time(NULL));

    /* create window and renderer */
    windowID =
        SDL_CreateWindow(NULL, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (windowID == 0) {
        printf("Could not initialize Window\n");
    }
    if (SDL_CreateRenderer(windowID, -1, 0) != 0) {
        printf("Could not create renderer\n");
    }

    /* Fill screen with black */
    SDL_RenderFill(0, 0, 0, 0, NULL);

    /* Enter render loop, waiting for user to quit */
    done = 0;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = 1;
            }
        }
        render();
        SDL_Delay(1);
    }

    /* shutdown SDL */
    SDL_Quit();

    return 0;
}
