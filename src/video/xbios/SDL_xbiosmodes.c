/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

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

#include <mint/osbind.h>
#include <mint/falcon.h>

#include "SDL_config.h"
#include "SDL_xbios.h"
#include "SDL_xbiosmodes.h"


typedef struct
{
    int width, height, bpp;
    int modecode;
    int doubleline;
} xbios_mode_t;

static xbios_mode_t falcon_rgb_modes[] = {
    {768, 480, 16, BPS16 | COL80 | OVERSCAN | VERTFLAG},
    {768, 240, 16, BPS16 | COL80 | OVERSCAN},
    {640, 400, 16, BPS16 | COL80 | VERTFLAG},
    {640, 200, 16, BPS16 | COL80},
    {384, 480, 16, BPS16 | OVERSCAN | VERTFLAG},
    {384, 240, 16, BPS16 | OVERSCAN},
    {320, 400, 16, BPS16 | VERTFLAG},
    {320, 200, 16, BPS16},
    {768, 480, 8, BPS8 | COL80 | OVERSCAN | VERTFLAG},
    {768, 240, 8, BPS8 | COL80 | OVERSCAN},
    {640, 400, 8, BPS8 | COL80 | VERTFLAG},
    {640, 200, 8, BPS8 | COL80},
    {384, 480, 8, BPS8 | OVERSCAN | VERTFLAG},
    {384, 240, 8, BPS8 | OVERSCAN},
    {320, 400, 8, BPS8 | VERTFLAG},
    {320, 200, 8, BPS8}
};

static xbios_mode_t falcon_vga_modes[] = {
    {320, 480, 16, BPS16},
    {320, 240, 16, BPS16 | VERTFLAG},
    {640, 480, 8, BPS8 | COL80},
    {640, 240, 8, BPS8 | COL80 | VERTFLAG},
    {320, 480, 8, BPS8},
    {320, 240, 8, BPS8 | VERTFLAG}
};

static void
SDL_XBIOS_AddMode(_THIS, int width, int height, int bpp, Uint16 modecode,
                  SDL_bool doubleline)
{
    SDL_VideoDisplay display;
    SDL_DisplayData *displaydata;
    SDL_DisplayMode mode;
    Uint32 Rmask, Gmask, Bmask, Amask;
    int orig_bpp;

    Rmask = Gmask = Bmask = Amask = 0;
    if (bpp == 16) {
        Rmask = 31 << 11;
        Gmask = 63 << 5;
        Bmask = 31;
    }
    /* Memorize for c2p4 operation */
    orig_bpp = bpp;
    if (bpp == 4) {
        bpp = 8;
    }

    mode.format = SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
    mode.w = width;
    mode.h = height;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;

    displaydata = (SDL_DisplayData *) SDL_malloc(sizeof(*displaydata));
    if (!displaydata) {
        return;
    }
    displaydata->modecode = modecode;
    displaydata->doubleline = doubleline;
    displaydata->c2p4 = (orig_bpp == 4);

    SDL_zero(display);
    display.desktop_mode = mode;
    display.current_mode = mode;
    display.driverdata = displaydata;
    SDL_AddVideoDisplay(&display);
}

/* Current video mode save/restore */

static void
SDL_XBIOS_ModeSave(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    data->old_vbase = Physbase();

    switch (data->cookie_vdo >> 16) {
    case VDO_ST:
    case VDO_STE:
        data->old_modecode = Getrez();
        break;
    case VDO_TT:
        data->old_modecode = EgetShift();
        break;
    case VDO_F30:
        data->old_modecode = VsetMode(-1);
        break;
    }
}

static void
SDL_XBIOS_ModeRestore(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    switch (data->cookie_vdo >> 16) {
    case VDO_ST:
    case VDO_STE:
        Setscreen(-1, data->old_vbase, data->old_modecode);
        break;
    case VDO_TT:
        Setscreen(-1, data->old_vbase, -1);
        EsetShift(data->old_modecode);
        break;
    case VDO_F30:
        Setscreen(-1, data->old_vbase, -1);
        VsetMode(data->old_modecode);
        break;
    }
}

/* Current palette save/restore */

static void
SDL_XBIOS_PaletteSave(_THIS)
{
    int i;
    Uint16 *palette;
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    data->old_numcol = 0;

    switch (data->cookie_vdo >> 16) {
    case VDO_ST:
    case VDO_STE:
        switch (data->old_modecode << 8) {
        case ST_LOW:
            data->old_numcol = 16;
            break;
        case ST_MED:
            data->old_numcol = 4;
            break;
        case ST_HIGH:
            data->old_numcol = 2;
            break;
        }

        palette = (Uint16 *) data->old_palette;
        for (i = 0; i < data->old_numcol; i++) {
            *palette++ = Setcolor(i, -1);
        }
        break;
    case VDO_TT:
        switch (data->old_modecode & ES_MODE) {
        case TT_LOW:
            data->old_numcol = 256;
            break;
        case ST_LOW:
        case TT_MED:
            data->old_numcol = 16;
            break;
        case ST_MED:
            data->old_numcol = 4;
            break;
        case ST_HIGH:
        case TT_HIGH:
            data->old_numcol = 2;
            break;
        }
        if (data->old_numcol) {
            EgetPalette(0, data->old_numcol, data->old_palette);
        }
        break;
    case VDO_F30:
        data->old_numcol = 1 << (1 << (data->old_modecode & NUMCOLS));
        if (data->old_numcol > 256) {
            data->old_numcol = 0;
        } else {
            VgetRGB(0, data->old_numcol, data->old_palette);
        }
        break;
    }
}

static void
SDL_XBIOS_PaletteRestore(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    if (data->old_numcol == 0) {
        return;
    }

    switch (data->cookie_vdo >> 16) {
    case VDO_ST:
    case VDO_STE:
        Setpalette(data->old_palette);
        break;
    case VDO_TT:
        EsetPalette(0, data->old_numcol, data->old_palette);
        break;
    case VDO_F30:
        VsetRGB(0, data->old_numcol, data->old_palette);
        break;
    }
}

/* Public functions for use by the driver */

void
SDL_XBIOS_InitModes(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    SDL_XBIOS_PaletteSave(_this);
    SDL_XBIOS_ModeSave(_this);

    switch (data->cookie_vdo >> 16) {
    case VDO_ST:
    case VDO_STE:
        {
            SDL_XBIOS_AddMode(_this, 320, 200, 4, ST_LOW >> 8, SDL_FALSE);
        }
        break;
    case VDO_TT:
        {
            SDL_XBIOS_AddMode(_this, 320, 480, 8, TT_LOW, SDL_FALSE);
            /* Software double-lined mode */
            SDL_XBIOS_AddMode(_this, 320, 240, 8, TT_LOW, SDL_TRUE);
        }
        break;
    case VDO_F30:
        {
            Uint16 modecodemask = data->old_modecode & (VGA | PAL);
            int i;

            switch (VgetMonitor()) {
            case MONITOR_MONO:
                /* Not usable */
                break;
            case MONITOR_RGB:
            case MONITOR_TV:
                for (i = 0;
                     i < sizeof(falcon_rgb_modes) / sizeof(xbios_mode_t);
                     i++) {
                    SDL_XBIOS_AddMode(_this, falcon_rgb_modes[i].width,
                                      falcon_rgb_modes[i].height,
                                      falcon_rgb_modes[i].bpp,
                                      falcon_rgb_modes[i].modecode &
                                      modecodemask, SDL_FALSE);
                }
                break;
            case MONITOR_VGA:
                for (i = 0;
                     i < sizeof(falcon_vga_modes) / sizeof(xbios_mode_t);
                     i++) {
                    SDL_XBIOS_AddMode(_this, falcon_vga_modes[i].width,
                                      falcon_vga_modes[i].height,
                                      falcon_vga_modes[i].bpp,
                                      falcon_vga_modes[i].modecode &
                                      modecodemask, SDL_FALSE);
                }
                break;
            }
        }
        break;
    }
}

void
SDL_XBIOS_GetDisplayModes(_THIS)
{
    SDL_DisplayData *data = (SDL_DisplayData *) SDL_CurrentDisplay.driverdata;
    SDL_DisplayMode mode;
    //SDL_AddDisplayMode(_this->current_display, &mode);
}

int
SDL_XBIOS_SetDisplayMode(_THIS, SDL_DisplayMode * mode)
{
    //SDL_DisplayModeData *data = (SDL_DisplayModeData *) mode->driverdata;
    return -1;
}

void
SDL_XBIOS_QuitModes(_THIS)
{
    SDL_XBIOS_ModeRestore(_this);
    SDL_XBIOS_PaletteRestore(_this);
    Vsync();
}

/* vi: set ts=4 sw=4 expandtab: */
