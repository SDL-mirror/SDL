/* Generic implementation for file opening routines.
* Customizations for specific platforms should go in alternative files.
*/

// quiet windows compiler warnings
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <SDL/SDL.h>

#include "TestSupportRWops.h"

FILE* TestSupportRWops_OpenFPFromReadDir(const char *file, const char *mode)
{
	return fopen(file, mode);
}

FILE* TestSupportRWops_OpenFPFromWriteDir(const char *file, const char *mode)
{
	return fopen(file, mode);
}

SDL_RWops* TestSupportRWops_OpenRWopsFromReadDir(const char *file, const char *mode)
{
	return SDL_RWFromFile(file, mode);
}

SDL_RWops* TestSupportRWops_OpenRWopsFromWriteDir(const char *file, const char *mode)
{
	return SDL_RWFromFile(file, mode);
}
