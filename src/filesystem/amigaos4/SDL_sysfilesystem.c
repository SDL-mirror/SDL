/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

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
#include "../../video/amigaos4/SDL_os4library.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

#include <proto/dos.h>

static struct DOSIFace* iDos;
static struct DOSBase* dosBase;

static BOOL
OS4_OpenDosLibrary()
{
    dosBase = (struct DOSBase *)OS4_OpenLibrary("dos.library", 50);
    iDos = (struct DOSIFace *)OS4_GetInterface((struct Library *)dosBase);
    return iDos != NULL;
}

static void
OS4_CloseDosLibrary()
{
    OS4_DropInterface((struct Interface **)&iDos);
    OS4_CloseLibrary((struct Library **)&dosBase);
}

char *
SDL_GetBasePath(void)
{
    char* buffer = NULL;
    const char* const basePath = "PROGDIR:";

    size_t len = SDL_strlen(basePath) + 1;

    buffer = (char *) SDL_malloc(len);
    if (!buffer) {
        SDL_OutOfMemory();
        return NULL;
    }

    SDL_memset(buffer, 0, len);
    SDL_snprintf(buffer, len, "%s", basePath);

    return buffer;
}

static BOOL
OS4_CreateDirTree(const char* path)
{
    BOOL success = FALSE;

    char* temp = SDL_strdup(path);

    if (!temp) {
        dprintf("Failed to create temporary path\n");
        return FALSE;
    }

    const size_t len = SDL_strlen(temp);

    if (len < 1) {
        dprintf("Empty string\n");
        return FALSE;
    }

    if (temp[len - 1] == '/') {
        temp[len - 1] = '\0';
    }

    if (OS4_OpenDosLibrary()) {
        BPTR lock = iDos->CreateDirTree(temp);
        if (lock) {
            success = TRUE;
            iDos->UnLock(lock);
        } else {
            const int32 err = iDos->IoErr();
            dprintf("Failed to create dir tree '%s' (err %d)\n", temp, err);
            if (err == ERROR_OBJECT_EXISTS) {
                dprintf("Object already exists -> success\n");
                success = TRUE;
            }
        }

        OS4_CloseDosLibrary();
    }

    SDL_free(temp);

    return success;
}

char *
SDL_GetPrefPath(const char *org, const char *app)
{
    const char* const envPath = "ENVARC:";
    size_t len = SDL_strlen(envPath) + 1;
    char* buffer = NULL;

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
    SDL_snprintf(buffer, len, envPath);

    if (org) {
        SDL_snprintf(buffer + SDL_strlen(buffer), len - SDL_strlen(buffer), "%s/", org);
    }

    if (app) {
        SDL_snprintf(buffer + SDL_strlen(buffer), len - SDL_strlen(buffer), "%s/", app);
    }

    if (OS4_CreateDirTree(buffer)) {
        return buffer;
    }

    SDL_free(buffer);
    return NULL;
}

#endif /* SDL_FILESYSTEM_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
