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

#ifdef SDL_LOADSO_AMIGAOS4

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */

#include <proto/elf.h>
#include <proto/dos.h>

static struct Library *ElfBase;
struct Library *DOSBase;

struct ElfIFace *IElf;
struct DOSIFace *IDOS;

#include "SDL_loadso.h"
#include "../../video/amigaos4/SDL_os4library.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

typedef struct {
    Elf32_Handle elf_handle;
    APTR shared_object;
} OS4_ObjectHandle;

static void
OS4_CloseLibs()
{
    OS4_DropInterface((void *)&IElf);
    OS4_DropInterface((void *)&IDOS);

    OS4_CloseLibrary(&ElfBase);
    OS4_CloseLibrary(&DOSBase);
}

static BOOL
OS4_OpenLibs()
{
    BOOL result = FALSE;

    if (!ElfBase) {
        ElfBase = OS4_OpenLibrary("elf.library", 52);

        if (ElfBase) {
            IElf = (struct ElfIFace *) OS4_GetInterface(ElfBase);
        }
    }

    if (!DOSBase) {
        DOSBase = OS4_OpenLibrary("dos.library", 51);

        if (DOSBase) {
            IDOS = (struct DOSIFace *) OS4_GetInterface(DOSBase);
        }
    }

    if (IElf && IDOS)
    {
        result = TRUE;
    } else {
        OS4_CloseLibs();
    }

    return result;
}

void *
SDL_LoadObject(const char *sofile)
{        
    if (OS4_OpenLibs()) {
        OS4_ObjectHandle *handle = SDL_malloc(sizeof(OS4_ObjectHandle));

        if (handle) {
            Elf32_Handle eh = NULL;
            BPTR seglist = IDOS->GetProcSegList(NULL, GPSLF_RUN);

            if (seglist) {
                IDOS->GetSegListInfoTags(seglist, GSLI_ElfHandle, &eh, TAG_DONE);

                dprintf("Elf handle %p\n", eh);

                if (eh) {
                    APTR so = IElf->DLOpen(eh, sofile, 0);
                        
                    if (so) {
                        dprintf("'%s' loaded\n", sofile);

                        handle->elf_handle = eh;
                        handle->shared_object = so;
                            
                        return handle;
                    } else {
                        dprintf("DLOpen failed for '%s'\n", sofile);
                        SDL_SetError("DLOpen failed for '%s'", sofile);
                    }
                } else {
                    dprintf("Failed to get elf handle of running task\n");
                    SDL_SetError("Failed to get elf handle");
                }
            } else {
                dprintf("Failed to get seglist\n");
                SDL_SetError("Failed to get seglist");
            }
                
            SDL_free(handle);
        }
    }

    return NULL;
}

void *
SDL_LoadFunction(void *handle, const char *name)
{
    void *symbol = NULL;

    if (OS4_OpenLibs() && handle) {
        APTR address;
        OS4_ObjectHandle *oh = handle;

        Elf32_Error result = IElf->DLSym(oh->elf_handle, oh->shared_object, name, &address);

        if (result == ELF32_NO_ERROR) {
            symbol = address;
            dprintf("Symbol '%s' found at %p\n", name, address);
        } else {
            dprintf("Symbol '%s' not found\n", name);
            SDL_SetError("Symbol '%s' not found", name);
        }
    }

    return (symbol);
}

void
SDL_UnloadObject(void *handle)
{
    if (OS4_OpenLibs() && handle) {

        Elf32_Error result;
        OS4_ObjectHandle *oh = handle;

        result = IElf->DLClose(oh->elf_handle, oh->shared_object);

        dprintf("DLClose %s\n", (result == ELF32_NO_ERROR) ? "OK" : "failed" );

/* BUG: testloadso crashes on Final Edition...removed this block for now, until a solution is found.
        IElf->CloseElfTags(
            oh->elf_handle,
            TAG_DONE);
*/
        SDL_free(handle);
    }

    OS4_CloseLibs();
}

#endif /* SDL_LOADSO_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
