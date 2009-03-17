/*
 *	touch.c
 *	written by Holmes Futrell
 *	use however you want
 */

#include "SDL.h"
#include "math.h"
#include "common.h"

#define BRUSH_SIZE 32           /* width and height of the brush */
#define PIXELS_PER_ITERATION 5  /* number of pixels between brush blots when forming a line */

static SDL_TextureID brushID = 0;       /* texture for the brush */

/*
	draws a line from (startx, starty) to (startx + dx, starty + dy)
	this is accomplished by drawing several blots spaced PIXELS_PER_ITERATION apart
*/
void
drawLine(float startx, float starty, float dx, float dy)
{

    float distance = sqrt(dx * dx + dy * dy);   /* length of line segment (pythagoras) */
    int iterations = distance / PIXELS_PER_ITERATION + 1;       /* number of brush sprites to draw for the line */
    float dx_prime = dx / iterations;   /* x-shift per iteration */
    float dy_prime = dy / iterations;   /* y-shift per iteration */
    SDL_Rect dstRect;           /* rect to draw brush sprite into */

    dstRect.w = BRUSH_SIZE;
    dstRect.h = BRUSH_SIZE;

    /* setup x and y for the location of the first sprite */
    float x = startx - BRUSH_SIZE / 2.0f;
    float y = starty - BRUSH_SIZE / 2.0f;

    int i;
    /* draw a series of blots to form the line */
    for (i = 0; i < iterations; i++) {
        dstRect.x = x;
        dstRect.y = y;
        /* shift x and y for next sprite location */
        x += dx_prime;
        y += dy_prime;
        /* draw brush blot */
        SDL_RenderCopy(brushID, NULL, &dstRect);
    }
}

/*
	loads the brush texture
*/
void
initializeTexture()
{
    SDL_Surface *bmp_surface;
    bmp_surface = SDL_LoadBMP("stroke.bmp");
    if (bmp_surface == NULL) {
        fatalError("could not load stroke.bmp");
    }
    brushID =
        SDL_CreateTextureFromSurface(SDL_PIXELFORMAT_ABGR8888, bmp_surface);
    SDL_FreeSurface(bmp_surface);
    if (brushID == 0) {
        fatalError("could not create brush texture");
    }
    /* additive blending -- laying strokes on top of eachother makes them brighter */
    SDL_SetTextureBlendMode(brushID, SDL_BLENDMODE_ADD);
    /* set brush color (red) */
    SDL_SetTextureColorMod(brushID, 255, 100, 100);
}

int
main(int argc, char *argv[])
{

    int x, y, dx, dy;           /* mouse location          */
    Uint8 state;                /* mouse (touch) state */
    SDL_Event event;
    SDL_WindowID windowID;      /* main window */
    int done;                   /* does user want to quit? */

    /* initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fatalError("Could not initialize SDL");
    }

    /* create main window and renderer */
    windowID = SDL_CreateWindow(NULL, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
                                SDL_WINDOW_BORDERLESS);
    SDL_CreateRenderer(windowID, 0, 0);

    /*load brush texture */
    initializeTexture();

    /* fill canvass initially with all black */
    SDL_SetRenderDrawColor(0, 0, 0, 255);
    SDL_RenderFill(NULL);
    SDL_RenderPresent();

    done = 0;
    while (!done && SDL_WaitEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            done = 1;
            break;
        case SDL_MOUSEMOTION:
            SDL_SelectMouse(event.motion.which);        /* select 'mouse' (touch) that moved */
            state = SDL_GetMouseState(&x, &y);  /* get its location */
            SDL_GetRelativeMouseState(&dx, &dy);        /* find how much the mouse moved */
            if (state & SDL_BUTTON_LMASK) {     /* is the mouse (touch) down? */
                drawLine(x - dx, y - dy, dx, dy);       /* draw line segment */
                SDL_RenderPresent();
            }
            break;
        }
    }

    /* cleanup */
    SDL_DestroyTexture(brushID);
    SDL_Quit();

    return 0;
}
