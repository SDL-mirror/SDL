
/* Test program to check SDL's CPU endian detection and byte swapping routines */

#include <stdio.h>

#include "SDL.h"
#include "SDL_endian.h"

int main(int argc, char *argv[])
{
	Uint16 value16 = 0xCDAB;
	Uint32 value32 = 0xEFBEADDE;
#if defined(__GNUC__) && defined(SDL_HAS_64BIT_TYPE)
	Uint64 value64 = 0xEFBEADDECDAB3412LL;
#endif

	printf("This is a %s endian machine.\n",
		(SDL_BYTEORDER == SDL_LIL_ENDIAN) ? "little" : "big");
	printf("Value 16 = 0x%X, swapped = 0x%X\n", value16, SDL_Swap16(value16));
	printf("Value 32 = 0x%X, swapped = 0x%X\n", value32, SDL_Swap32(value32));
#if defined(__GNUC__) && defined(SDL_HAS_64BIT_TYPE)
	printf("Value 64 = 0x%llX, swapped = 0x%llX\n", value64, SDL_Swap64(value64));
#endif
	return(0);
}
