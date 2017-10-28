/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

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
    char *buffer = NULL;

    len = SDL_strlen(OS4_BASE_PATH) + 1;

    buffer = (char *) SDL_malloc(len);
    if (!buffer) {
        SDL_OutOfMemory();
        return NULL;
    }

    SDL_memset(buffer, 0, len);
    SDL_snprintf(buffer, len, "%s", OS4_BASE_PATH);

    return buffer;
}

char *
SDL_GetPrefPath(const char *org, const char *app)
{
    size_t len = 5; // "ENV:" + NUL
    char *buffer = NULL;

    if (org) {
        len += SDL_strlen(org) + 1;
    }

    if (app) {
        len += SDL_strlen(app) + 1;
    }

    buffer = (char *) SDL_malloc(len);
    if (!buffer) {
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_memset(buffer, 0, len);

    SDL_snprintf(buffer, len, "ENV:");

    if (org) {
        SDL_snprintf(buffer + SDL_strlen(buffer), len - SDL_strlen(buffer), "%s/", org);
    }

    if (app) {
        SDL_snprintf(buffer + SDL_strlen(buffer), len - SDL_strlen(buffer), "%s/", app);
    }

    return buffer;
}

#endif /* SDL_FILESYSTEM_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
