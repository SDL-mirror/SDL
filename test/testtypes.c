
#include <stdio.h>
#include "SDL_main.h"
#include "SDL_types.h"

int main(int argc, char *argv[])
{
	int error = 0;
	int verbose = 1;

	if ( argv[1] && (strcmp(argv[1], "-q") == 0) )
		verbose = 0;

	if ( sizeof(Uint8) != 1 ) {
		if ( verbose )
			printf("sizeof(Uint8) != 1, instead = %d\n",
								sizeof(Uint8));
		++error;
	}
	if ( sizeof(Uint16) != 2 ) {
		if ( verbose )
			printf("sizeof(Uint16) != 2, instead = %d\n",
								sizeof(Uint16));
		++error;
	}
	if ( sizeof(Uint32) != 4 ) {
		if ( verbose )
			printf("sizeof(Uint32) != 4, instead = %d\n",
								sizeof(Uint32));
		++error;
	}
#ifdef SDL_HAS_64BIT_TYPE
	if ( sizeof(Uint64) != 8 ) {
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
