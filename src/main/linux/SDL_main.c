
/* Include the SDL main definition header */
#include "SDL_main.h"
#ifdef main
#undef main
#endif

extern int SDL_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	return(SDL_main(argc, argv));
}
