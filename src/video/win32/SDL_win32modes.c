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

#include "SDL_win32video.h"


/* FIXME: Each call to EnumDisplaySettings() takes about 6 ms on my laptop.
          With 500 or so modes, this takes almost 3 seconds to run!
*/

static SDL_bool
WIN_GetDisplayMode(LPCTSTR deviceName, DWORD index, SDL_DisplayMode * mode)
{
    SDL_DisplayModeData *data;
    DEVMODE devmode;
    HDC hdc;

    devmode.dmSize = sizeof(devmode);
    devmode.dmDriverExtra = 0;
    if (!EnumDisplaySettings(deviceName, index, &devmode)) {
        return SDL_FALSE;
    }

    data = (SDL_DisplayModeData *) SDL_malloc(sizeof(*data));
    if (!data) {
        return SDL_FALSE;
    }
    SDL_memcpy(data->DeviceName, deviceName, sizeof(data->DeviceName));
    data->DeviceMode = devmode;
    data->DeviceMode.dmFields =
        (DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY |
         DM_DISPLAYFLAGS);

    /* Fill in the mode information */
    mode->format = SDL_PixelFormat_Unknown;
    mode->w = devmode.dmPelsWidth;
    mode->h = devmode.dmPelsHeight;
    mode->refresh_rate = devmode.dmDisplayFrequency;
    mode->driverdata = data;

    hdc = CreateDC(deviceName, NULL, NULL, &devmode);
    if (hdc) {
        char bmi_data[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
        LPBITMAPINFO bmi;
        HBITMAP hbm;

        SDL_zero(bmi_data);
        bmi = (LPBITMAPINFO) bmi_data;
        bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        hbm = CreateCompatibleBitmap(hdc, 1, 1);
        GetDIBits(hdc, hbm, 0, 1, NULL, bmi, DIB_RGB_COLORS);
        GetDIBits(hdc, hbm, 0, 1, NULL, bmi, DIB_RGB_COLORS);
        DeleteObject(hbm);
        DeleteDC(hdc);
        if (bmi->bmiHeader.biCompression == BI_BITFIELDS) {
            switch (*(Uint32 *) bmi->bmiColors) {
            case 0x00FF0000:
                mode->format = SDL_PixelFormat_RGB888;
                break;
            case 0x000000FF:
                mode->format = SDL_PixelFormat_BGR888;
                break;
            case 0xF800:
                mode->format = SDL_PixelFormat_RGB565;
                break;
            case 0x7C00:
                mode->format = SDL_PixelFormat_RGB555;
                break;
            }
        } else if (bmi->bmiHeader.biBitCount == 8) {
            mode->format = SDL_PixelFormat_Index8;
        }
    } else {
        switch (devmode.dmBitsPerPel) {
        case 32:
            mode->format = SDL_PixelFormat_RGB888;
            break;
        case 24:
            mode->format = SDL_PixelFormat_RGB24;
            break;
        case 16:
            mode->format = SDL_PixelFormat_RGB565;
            break;
        case 15:
            mode->format = SDL_PixelFormat_RGB555;
            break;
        case 8:
            mode->format = SDL_PixelFormat_Index8;
            break;
        }
    }
    return SDL_TRUE;
}

void
WIN_InitModes(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    DWORD i, j;
    DISPLAY_DEVICE device;

    device.cb = sizeof(device);
    for (i = 0;; ++i) {
        TCHAR DeviceName[32];

        if (!EnumDisplayDevices(NULL, i, &device, 0)) {
            break;
        }
        if (!(device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
            continue;
        }
        SDL_memcpy(DeviceName, device.DeviceName, sizeof(DeviceName));
#ifdef DEBUG_MODES
        printf("Device: %s\n", WIN_StringToUTF8(DeviceName));
#endif
        for (j = 0;; ++j) {
            SDL_VideoDisplay display;
            SDL_DisplayData *displaydata;
            SDL_DisplayMode mode;

            if (!EnumDisplayDevices(DeviceName, j, &device, 0)) {
                break;
            }
            if (!(device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
                continue;
            }
#ifdef DEBUG_MODES
            printf("Monitor: %s\n", WIN_StringToUTF8(device.DeviceName));
#endif
            if (!WIN_GetDisplayMode(DeviceName, ENUM_CURRENT_SETTINGS, &mode)) {
                break;
            }

            displaydata =
                (SDL_DisplayData *) SDL_malloc(sizeof(*displaydata));
            if (!displaydata) {
                continue;
            }
            SDL_memcpy(displaydata->DeviceName, DeviceName,
                       sizeof(DeviceName));

            SDL_zero(display);
            display.desktop_mode = mode;
            display.current_mode = mode;
            display.driverdata = displaydata;
            SDL_AddVideoDisplay(&display);
        }
    }
}

void
WIN_GetDisplayModes(_THIS)
{
    SDL_DisplayData *data = (SDL_DisplayData *) SDL_CurrentDisplay.driverdata;
    DWORD i;
    SDL_DisplayMode mode;

    for (i = 0;; ++i) {
        if (!WIN_GetDisplayMode(data->DeviceName, i, &mode)) {
            break;
        }
        if (!SDL_AddDisplayMode(_this->current_display, &mode)) {
            SDL_free(mode.driverdata);
        }
    }
}

int
WIN_SetDisplayMode(_THIS, SDL_DisplayMode * mode)
{
    SDL_DisplayModeData *data = (SDL_DisplayModeData *) mode->driverdata;
    LONG status;

    status =
        ChangeDisplaySettingsEx(data->DeviceName, &data->DeviceMode, NULL,
                                CDS_FULLSCREEN, NULL);
    if (status == DISP_CHANGE_SUCCESSFUL) {
        return 0;
    } else {
        const char *reason = "Unknown reason";
        switch (status) {
        case DISP_CHANGE_BADFLAGS:
            reason = "DISP_CHANGE_BADFLAGS";
            break;
        case DISP_CHANGE_BADMODE:
            reason = "DISP_CHANGE_BADMODE";
            break;
        case DISP_CHANGE_BADPARAM:
            reason = "DISP_CHANGE_BADPARAM";
            break;
        case DISP_CHANGE_FAILED:
            reason = "DISP_CHANGE_FAILED";
            break;
        }
        SDL_SetError("ChangeDisplaySettingsEx() failed: %s", reason);
        return -1;
    }
}

void
WIN_QuitModes(_THIS)
{
    ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
}

/* vi: set ts=4 sw=4 expandtab: */
