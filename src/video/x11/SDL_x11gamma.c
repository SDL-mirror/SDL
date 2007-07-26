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
    Colormap colormap;
    XStandardColormap cmap;
    Visual visual;
} cmapTableEntry;

cmapTableEntry *cmapTable = NULL;

/* To reduce the overhead as much as possible lets do as little as
   possible. When we do have to create a colormap keep track of it and
   reuse it. We're going to do this for both DirectColor and
   PseudoColor colormaps. */

Colormap
X11_LookupColormap(Display * display, int scrNum, VisualID vid)
{
    int i;

    for (i = 0; i < numCmaps; i++) {
        if (cmapTable[i].display == display &&
            cmapTable[i].scrNum == scrNum &&
            cmapTable[i].cmap.visualid == vid) {
            return cmapTable[i].cmap.colormap;
        }
    }

    return 0;
}


void
X11_TrackColormap(Display * display, int scrNum, Colormap colormap,
                  XStandardColormap * cmap, Visual * visual)
{
    int i;

    /* search the table to find out if we already have this one. We
       only want one entry for each display, screen number, visualid,
       and colormap combination */
    for (i = 0; i < numCmaps; i++) {
        if (cmapTable[i].display == display &&
            cmapTable[i].scrNum == scrNum &&
            cmapTable[i].cmap.visualid == cmap->visualid &&
            cmapTable[i].cmap.colormap == colormap) {
            return;
        }
    }

    /* increase the table by one entry. If the table is NULL create the
       first entrty */
    cmapTable = SDL_realloc(cmapTable, (numCmaps + 1) * sizeof(cmapTableEntry));
    if (NULL == cmapTable) {
        SDL_SetError("Out of memory in X11_TrackColormap()");
        return;
    }

    cmapTable[numCmaps].display = display;
    cmapTable[numCmaps].scrNum = scrNum;
    cmapTable[numCmaps].colormap = colormap;
    SDL_memcpy(&cmapTable[numCmaps].cmap, cmap, sizeof(XStandardColormap));
    SDL_memcpy(&cmapTable[numCmaps].visual, visual, sizeof(Visual));

    numCmaps++;
}

/* The problem is that you have to have at least one DirectColor
   colormap before you can set the gamma ramps or read the gamma
   ramps. If the application has created a DirectColor window then the
   cmapTable will have at least one colormap in it and everything is
   cool. If not, then we just fail  */

int
X11_SetDisplayGammaRamp(_THIS, Uint16 * ramp)
{
    Display *display;
    Colormap colormap;
    XColor *colorcells;
    int ncolors;
    int i;
    int j;

    int rmax, gmax, bmax;
    int rmul, gmul, bmul;

    for (j = 0; j < numCmaps; j++) {
        if (cmapTable[j].visual.class == DirectColor) {
            display = cmapTable[j].display;
            colormap = cmapTable[j].colormap;
            ncolors = cmapTable[j].visual.map_entries;

            colorcells = SDL_malloc(ncolors * sizeof(XColor));
            if (NULL == colorcells) {
                SDL_SetError("out of memory in X11_SetDisplayGammaRamp");
                return -1;
            }

            rmax = cmapTable[j].cmap.red_max + 1;
            gmax = cmapTable[j].cmap.blue_max + 1;
            bmax = cmapTable[j].cmap.green_max + 1;

            rmul = cmapTable[j].cmap.red_mult;
            gmul = cmapTable[j].cmap.blue_mult;
            bmul = cmapTable[j].cmap.green_mult;

            /* build the color table pixel values */
            for (i = 0; i < ncolors; i++) {
                Uint32 red = (rmax * i) / ncolors;
                Uint32 green = (gmax * i) / ncolors;
                Uint32 blue = (bmax * i) / ncolors;

                colorcells[i].pixel =
                    (red * rmul) | (green * gmul) | (blue * bmul);
                colorcells[i].flags = DoRed | DoGreen | DoBlue;

                colorcells[i].red = ramp[(0 * 256) + i];
                colorcells[i].green = ramp[(1 * 256) + i];
                colorcells[i].blue = ramp[(2 * 256) + i];
            }

            XStoreColors(display, colormap, colorcells, ncolors);
            XFlush(display);
            SDL_free(colorcells);
        }
    }

    return 0;
}

int
X11_GetDisplayGammaRamp(_THIS, Uint16 * ramp)
{
    Display *display;
    Colormap colormap;
    XColor *colorcells;
    int ncolors;
    int dc;
    int i;

    int rmax, gmax, bmax;
    int rmul, gmul, bmul;

    /* find the first DirectColor colormap and use it to get the gamma
       ramp */

    dc = -1;
    for (i = 0; i < numCmaps; i++) {
        if (cmapTable[i].visual.class == DirectColor) {
            dc = i;
            break;
        }
    }

    if (dc < 0) {
        return -1;
    }

    /* there is at least one DirectColor colormap in the cmapTable,
       let's just get the entries from that colormap */

    display = cmapTable[dc].display;
    colormap = cmapTable[dc].colormap;
    ncolors = cmapTable[dc].visual.map_entries;
    colorcells = SDL_malloc(ncolors * sizeof(XColor));
    if (NULL == colorcells) {
        SDL_SetError("out of memory in X11_GetDisplayGammaRamp");
        return -1;
    }

    rmax = cmapTable[dc].cmap.red_max + 1;
    gmax = cmapTable[dc].cmap.blue_max + 1;
    bmax = cmapTable[dc].cmap.green_max + 1;

    rmul = cmapTable[dc].cmap.red_mult;
    gmul = cmapTable[dc].cmap.blue_mult;
    bmul = cmapTable[dc].cmap.green_mult;

    /* build the color table pixel values */
    for (i = 0; i < ncolors; i++) {
        Uint32 red = (rmax * i) / ncolors;
        Uint32 green = (gmax * i) / ncolors;
        Uint32 blue = (bmax * i) / ncolors;

        colorcells[i].pixel = (red * rmul) | (green * gmul) | (blue * bmul);
    }

    XQueryColors(display, colormap, colorcells, ncolors);

    /* prepare the values to be returned. Note that SDL assumes that
       gamma ramps are always 3 * 256 entries long with the red entries
       in the first 256 elements, the green in the second 256 elements
       and the blue in the last 256 elements */

    for (i = 0; i < ncolors; i++) {
        ramp[(0 * 256) + i] = colorcells[i].red;
        ramp[(1 * 256) + i] = colorcells[i].green;
        ramp[(2 * 256) + i] = colorcells[i].blue;
    }

    SDL_free(colorcells);
    return 0;
}
