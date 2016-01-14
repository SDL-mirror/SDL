/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#ifdef SDL_FILESYSTEM_AMIGAOS4

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent filesystem routines                                */

#include "SDL_error.h"
#include "SDL_filesystem.h"

#define OS4_BASE_PATH "PROGDIR:"

char *
SDL_GetBasePath(void)
{
	size_t len = 0;
	char *retval = NULL;

	len = SDL_strlen(OS4_BASE_PATH) + 1;

	retval = (char *) SDL_malloc(len);
	if (!retval) {
		SDL_OutOfMemory();
		return NULL;
	}

	SDL_memset(retval, 0, len);
	SDL_snprintf(retval, len, "%s", OS4_BASE_PATH);

	return retval;
}

char *
SDL_GetPrefPath(const char *org, const char *app)
{
    size_t len = 0;
    char *retval = NULL;

    len = 4 + SDL_strlen(org) + SDL_strlen(app) + 3;
    retval = (char *) SDL_malloc(len);
    if (!retval) {
        SDL_OutOfMemory();
        return NULL;
    }
	SDL_memset(retval, 0, len);
    SDL_snprintf(retval, len, "%s%s/%s/", "ENV:", org, app);

	return retval;
}

#endif /* SDL_FILESYSTEM_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
