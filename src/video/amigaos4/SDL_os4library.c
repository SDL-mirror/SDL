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

#if SDL_VIDEO_DRIVER_AMIGAOS4

#include "SDL_os4library.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

#include <proto/exec.h>

struct ExecIFace* IExec; // Need the symbol for .so linkage

void _OS4_INIT(void) __attribute__((constructor));

void _OS4_INIT(void)
{
    IExec = ((struct ExecIFace *)((*(struct ExecBase **)4)->MainInterface));
    dprintf("IExec %p\n", IExec);
}

struct Library *
OS4_OpenLibrary(STRPTR name, ULONG version)
{
    struct Library* lib = IExec->OpenLibrary(name, version);

    dprintf("Opening '%s' version %u %s (address %p)\n",
        name, version, lib ? "succeeded" : "FAILED", lib);

    return lib;
}

struct Interface *
OS4_GetInterface(struct Library * lib)
{
    struct Interface* interface = IExec->GetInterface(lib, "main", 1, NULL);

    dprintf("Getting interface for libbase %p %s (address %p)\n",
        lib, interface ? "succeeded" : "FAILED", interface);

    return interface;
}

void
OS4_DropInterface(struct Interface ** interface)
{
    if (interface && *interface) {
        dprintf("Dropping interface %p\n", *interface);
        IExec->DropInterface(*interface);
        *interface = NULL;
    }
}

void
OS4_CloseLibrary(struct Library ** library)
{
    if (library && *library) {
        dprintf("Closing library %p\n", *library);
        IExec->CloseLibrary(*library);
        *library = NULL;
    }
}

#endif
