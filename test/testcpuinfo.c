
/* Test program to check SDL's CPU feature detection */

#include <stdio.h>

#include "SDL.h"
#include "SDL_cpuinfo.h"

int main(int argc, char *argv[])
{
	printf("RDTSC %s\n", SDL_HasRDTSC() ? "detected" : "not detected");
	printf("MMX %s\n", SDL_HasMMX() ? "detected" : "not detected");
	printf("3DNow %s\n", SDL_Has3DNow() ? "detected" : "not detected");
	printf("SSE %s\n", SDL_HasSSE() ? "detected" : "not detected");
	printf("AltiVec %s\n", SDL_HasAltiVec() ? "detected" : "not detected");
	return(0);
}
