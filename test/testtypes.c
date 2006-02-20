
#include <stdio.h>
#include "SDL_main.h"
#include "SDL_stdinc.h"

/*
 * Watcom C flags these as Warning 201: "Unreachable code" if you just
 *  compare them directly, so we push it through a function to keep the
 *  compiler quiet.  --ryan.
 */
static int badsize(size_t sizeoftype, size_t hardcodetype)
{
    return sizeoftype != hardcodetype;
}

int main(int argc, char *argv[])
{
	int error = 0;
	int verbose = 1;

	if ( argv[1] && (strcmp(argv[1], "-q") == 0) )
		verbose = 0;

	if ( badsize(sizeof(Uint8), 1) ) {
		if ( verbose )
			printf("sizeof(Uint8) != 1, instead = %d\n",
								sizeof(Uint8));
		++error;
	}
	if ( badsize(sizeof(Uint16), 2) ) {
		if ( verbose )
			printf("sizeof(Uint16) != 2, instead = %d\n",
								sizeof(Uint16));
		++error;
	}
	if ( badsize(sizeof(Uint32), 4) ) {
		if ( verbose )
			printf("sizeof(Uint32) != 4, instead = %d\n",
								sizeof(Uint32));
		++error;
	}
#ifdef SDL_HAS_64BIT_TYPE
	if ( badsize(sizeof(Uint64), 8) ) {
		if ( verbose )
			printf("sizeof(Uint64) != 8, instead = %d\n",
								sizeof(Uint64));
		++error;
	}
#else
	if ( verbose ) {
		printf("WARNING: No 64-bit datatype on this platform\n");
	}
#endif
	if ( verbose && ! error )
		printf("All data types are the expected size.\n");

	return( error ? 1 : 0 );
}
