/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"
#include "../SDL_sysvideo.h"
#include "SDL_x11video.h"

static int numCmaps = 0;

typedef struct
{
    Display *display;
    int scrNum;
    XStandardColormap cmap;
    Visual visual;
} cmapTableEntry;

cmapTableEntry *cmapTable = NULL;

void
X11_TrackColormap(Display * display, int scrNum,
                  XStandardColormap * cmap, Visual * visual)
{
    int i;
    cmapTableEntry *newTable = NULL;

    /* only tracking DirectColor colormaps because they're the only ones
       with gamma ramps */
    if (DirectColor != visual->class) {
        return;
    }

    /* search the table to find out if we already have this one. We only
       want one entry for each display, screen number, visualid
       combination */
    for (i = 0; i < numCmaps; i++) {
        if (cmapTable[i].display == display &&
            cmapTable[i].scrNum == scrNum &&
            cmapTable[i].cmap.visualid == cmap->visualid) {
            return;
        }
    }

    /* increase the table by one entry. If the table is NULL create the
       first entrty */
    newTable = SDL_malloc((numCmaps + 1) * sizeof(cmapTableEntry));
    if (NULL == newTable) {
        SDL_SetError("Out of memory in X11_TrackColormap()");
        return;
    }

    if (NULL != cmapTable) {
        SDL_memcpy(newTable, cmapTable, numCmaps * sizeof(cmapTableEntry));
        SDL_free(cmapTable);
    }
    cmapTable = newTable;

    cmapTable[numCmaps].display = display;
    cmapTable[numCmaps].scrNum = scrNum;
    SDL_memcpy(&cmapTable[numCmaps].cmap, cmap, sizeof(XStandardColormap));
    SDL_memcpy(&cmapTable[numCmaps].visual, visual, sizeof(Visual));

    numCmaps++;
}

int
X11_SetDisplayGammaRamp(_THIS, Uint16 * ramp)
{
    return -1;
}

int
X11_GetDisplayGammaRamp(_THIS, Uint16 * ramp)
{
    return -1;
}
