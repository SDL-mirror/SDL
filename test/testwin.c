#include <windows.h>
#include "SDL/SDL.h"


#define WIDTH 480
#define HEIGHT 480
#define DEPTH 32


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR
lpCmdLine, int nCmdShow)
{
        if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != -1)
        {
                SDL_Surface* w;
                Uint32* r;
                int x;

                w = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_SWSURFACE |
SDL_FULLSCREEN);

                SDL_LockSurface(w);
                r = w->pixels + w->pitch * HEIGHT / 2;
                for (x = 0; x < WIDTH; ++x) r[x] = 0xFFFFFFFF;
                SDL_UnlockSurface(w);
                SDL_UpdateRect(w, 0, 0, 0, 0);

                SDL_Delay(1000);
                SDL_Quit();
        }

        return 0;
}
