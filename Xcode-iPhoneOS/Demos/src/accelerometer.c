/*
 *	accelerometer.c
 *	written by Holmes Futrell
 *	use however you want
 */

#include "SDL.h"
#include "math.h"
#include "common.h"

#define MILLESECONDS_PER_FRAME 16       /* about 60 frames per second */
#define DAMPING 0.5f;           /* after bouncing off a wall, damping coefficient determines final speed */
#define FRICTION 0.0008f        /* coefficient of acceleration that opposes direction of motion */
#define GRAVITY_CONSTANT 0.004f /* how sensitive the ship is to the accelerometer */

/*	If we aren't on an iPhone, then this definition ought to yield reasonable behavior */
#ifndef SDL_IPHONE_MAX_GFORCE
#define SDL_IPHONE_MAX_GFORCE 5.0f
#endif

static SDL_Joystick *accelerometer;     /* used for controlling the ship */

static struct
{
    float x, y;                 /* position of ship */
    float vx, vy;               /* velocity of ship (in pixels per millesecond) */
    SDL_Rect rect;              /* (drawn) position and size of ship */
} ship;

static SDL_TextureID shipID = 0;        /* texture for spaceship */
static SDL_TextureID spaceID = 0;       /* texture for space (background */

void
render(void)
{


    /* get joystick (accelerometer) axis values and normalize them */
    float ax = SDL_JoystickGetAxis(accelerometer, 0);
    float ay = -SDL_JoystickGetAxis(accelerometer, 1);

    /* ship screen constraints */
    Uint32 minx = 0.0f;
    Uint32 maxx = SCREEN_WIDTH - ship.rect.w;
    Uint32 miny = 0.0f;
    Uint32 maxy = SCREEN_HEIGHT - ship.rect.h;

#define SINT16_MAX ((float)(0x7FFF))

    /* update velocity from accelerometer
       the factor SDL_IPHONE_MAX_G_FORCE / SINT16_MAX converts between 
       SDL's units reported from the joytick, and units of g-force, as reported by the accelerometer
     */
    ship.vx +=
        ax * SDL_IPHONE_MAX_GFORCE / SINT16_MAX * GRAVITY_CONSTANT *
        MILLESECONDS_PER_FRAME;
    ship.vy +=
        ay * SDL_IPHONE_MAX_GFORCE / SINT16_MAX * GRAVITY_CONSTANT *
        MILLESECONDS_PER_FRAME;

    float speed = sqrt(ship.vx * ship.vx + ship.vy * ship.vy);

    if (speed > 0) {
        /* compensate for friction */
        float dirx = ship.vx / speed;   /* normalized x velocity */
        float diry = ship.vy / speed;   /* normalized y velocity */

        /* update velocity due to friction */
        if (speed - FRICTION * MILLESECONDS_PER_FRAME > 0) {
            /* apply friction */
            ship.vx -= dirx * FRICTION * MILLESECONDS_PER_FRAME;
            ship.vy -= diry * FRICTION * MILLESECONDS_PER_FRAME;
        } else {
            /* applying friction would MORE than stop the ship, so just stop the ship */
            ship.vx = 0.0f;
            ship.vy = 0.0f;
        }
    }

    /* update ship location */
    ship.x += ship.vx * MILLESECONDS_PER_FRAME;
    ship.y += ship.vy * MILLESECONDS_PER_FRAME;

    if (ship.x > maxx) {
        ship.x = maxx;
        ship.vx = -ship.vx * DAMPING;
    } else if (ship.x < minx) {
        ship.x = minx;
        ship.vx = -ship.vx * DAMPING;
    }
    if (ship.y > maxy) {
        ship.y = maxy;
        ship.vy = -ship.vy * DAMPING;
    } else if (ship.y < miny) {
        ship.y = miny;
        ship.vy = -ship.vy * DAMPING;
    }

    /* draw the background */
    SDL_RenderCopy(spaceID, NULL, NULL);

    /* draw the ship */
    ship.rect.x = ship.x;
    ship.rect.y = ship.y;

    SDL_RenderCopy(shipID, NULL, &ship.rect);

    /* update screen */
    SDL_RenderPresent();

}

void
initializeTextures()
{

    SDL_Surface *bmp_surface;
    SDL_Surface *bmp_surface_rgba;
    int format = SDL_PIXELFORMAT_ABGR8888;      /* desired texture format */
    Uint32 Rmask, Gmask, Bmask, Amask;  /* masks for desired format */
    int bpp;                    /* bits per pixel for desired format */

    /* load the ship */
    bmp_surface = SDL_LoadBMP("ship.bmp");
    if (bmp_surface == NULL) {
        fatalError("could not ship.bmp");
    }
    /* set blue to transparent on the ship */
    SDL_SetColorKey(bmp_surface, 1,
                    SDL_MapRGB(bmp_surface->format, 0, 0, 255));
    SDL_PixelFormatEnumToMasks(format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
    /*
       create a new RGBA surface and blit the bmp to it
       this is an extra step, but it seems to be necessary for the color key to work

       does the fact that this is necessary indicate a bug in SDL?
     */
    bmp_surface_rgba =
        SDL_CreateRGBSurface(0, bmp_surface->w, bmp_surface->h, bpp, Rmask,
                             Gmask, Bmask, Amask);
    SDL_BlitSurface(bmp_surface, NULL, bmp_surface_rgba, NULL);

    /* create ship texture from surface */
    shipID = SDL_CreateTextureFromSurface(format, bmp_surface_rgba);
    if (shipID == 0) {
        fatalError("could not create ship texture");
    }
    SDL_SetTextureBlendMode(shipID, SDL_BLENDMODE_BLEND);

    /* set the width and height of the ship from the surface dimensions */
    ship.rect.w = bmp_surface->w;
    ship.rect.h = bmp_surface->h;

    SDL_FreeSurface(bmp_surface_rgba);
    SDL_FreeSurface(bmp_surface);

    /* load the space background */
    bmp_surface = SDL_LoadBMP("space.bmp");
    if (bmp_surface == NULL) {
        fatalError("could not load space.bmp");
    }
    /* create space texture from surface */
    spaceID = SDL_CreateTextureFromSurface(format, bmp_surface);
    if (spaceID == 0) {
        fatalError("could not create space texture");
    }
    SDL_FreeSurface(bmp_surface);

}



int
main(int argc, char *argv[])
{

    SDL_WindowID windowID;      /* ID of main window */
    Uint32 startFrame;          /* time frame began to process */
    Uint32 endFrame;            /* time frame ended processing */
    Uint32 delay;               /* time to pause waiting to draw next frame */
    int done;                   /* should we clean up and exit? */

    /* initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        fatalError("Could not initialize SDL");
    }

    /* create main window and renderer */
    windowID = SDL_CreateWindow(NULL, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
                                SDL_WINDOW_BORDERLESS);
    SDL_CreateRenderer(windowID, 0, 0);

    /* print out some info about joysticks and try to open accelerometer for use */
    printf("There are %d joysticks available\n", SDL_NumJoysticks());
    printf("Default joystick (index 0) is %s\n", SDL_JoystickName(0));
    accelerometer = SDL_JoystickOpen(0);
    if (accelerometer == NULL) {
        fatalError("Could not open joystick (accelerometer)");
    }
    printf("joystick number of axis = %d\n",
           SDL_JoystickNumAxes(accelerometer));
    printf("joystick number of hats = %d\n",
           SDL_JoystickNumHats(accelerometer));
    printf("joystick number of balls = %d\n",
           SDL_JoystickNumBalls(accelerometer));
    printf("joystick number of buttons = %d\n",
           SDL_JoystickNumButtons(accelerometer));

    /* load graphics */
    initializeTextures();

    /* setup ship */
    ship.x = (SCREEN_WIDTH - ship.rect.w) / 2;
    ship.y = (SCREEN_HEIGHT - ship.rect.h) / 2;
    ship.vx = 0.0f;
    ship.vy = 0.0f;

    done = 0;
    /* enter main loop */
    while (!done) {
        startFrame = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = 1;
            }
        }
        render();
        endFrame = SDL_GetTicks();

        /* figure out how much time we have left, and then sleep */
        delay = MILLESECONDS_PER_FRAME - (endFrame - startFrame);
        if (delay < 0) {
            delay = 0;
        } else if (delay > MILLESECONDS_PER_FRAME) {
            delay = MILLESECONDS_PER_FRAME;
        }
        SDL_Delay(delay);
    }

    /* delete textures */
    SDL_DestroyTexture(shipID);
    SDL_DestroyTexture(spaceID);

    /* shutdown SDL */
    SDL_Quit();

    return 0;

}
