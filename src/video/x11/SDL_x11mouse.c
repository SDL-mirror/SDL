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
#include "SDL_config.h"
#include "SDL_x11video.h"
#include "../../events/SDL_mouse_c.h"

void
X11_InitMouse(_THIS)
{
#if SDL_VIDEO_DRIVER_X11_XINPUT
    XDevice **newDevices;
    int i, j, index = 0, numOfDevices;
    XDeviceInfo *DevList;
    XAnyClassPtr deviceClass;
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    SDL_XDevices = NULL;
    SDL_NumOfXDevices = 0;

    if (!SDL_X11_HAVE_XINPUT) {
        /* should have dynamically loaded, but wasn't available. */
        return;
    }

    /* we're getting the list of input devices */
    DevList = XListInputDevices(data->display, &numOfDevices);
    SDL_XDevices = (XDevice **) SDL_malloc(sizeof(XDevice));

    /* we're aquiring valuators:mices, tablets, etc. */
    for (i = 0; i < numOfDevices; ++i) {
        /* if it's the core pointer or core keyborard we don't want it */
        if ((DevList[i].use != IsXPointer && DevList[i].use != IsXKeyboard)) {
            /* we have to check all of the device classes */
            deviceClass = DevList[i].inputclassinfo;
            for (j = 0; j < DevList[i].num_classes; ++j) {
                if (deviceClass->class == ValuatorClass) {      /* bingo ;) */
                    XValuatorInfo *valInfo;
                    SDL_Mouse mouse;

                    newDevices =
                        (XDevice **) SDL_realloc(SDL_XDevices,
                                                 (index +
                                                  1) * sizeof(*newDevices));
                    if (!newDevices) {
                        SDL_OutOfMemory();
                        return;
                    }
                    SDL_XDevices = newDevices;
                    SDL_XDevices[index] =
                        XOpenDevice(data->display, DevList[i].id);
                    SDL_zero(mouse);

                    /* the id of the device differs from its index
                     * so we're assigning the index of a device to it's id */
                    SDL_SetMouseIndexId(DevList[i].id, index);
                    /* lets get the device parameters */
                    valInfo = (XValuatorInfo *) deviceClass;
                    /* if the device reports pressure, lets check it parameteres */
                    if (valInfo->num_axes > 2) {
                        data->mouse =
                            SDL_AddMouse(&mouse, index++, DevList[i].name,
                                         valInfo->axes[2].max_value,
                                         valInfo->axes[2].min_value, 1);
                    } else {
                        data->mouse =
                            SDL_AddMouse(&mouse, index++, DevList[i].name, 0,
                                         0, 1);
                    }
                    break;
                }
                /* if it's not class we're interested in, lets go further */
                deviceClass =
                    (XAnyClassPtr) ((char *) deviceClass +
                                    deviceClass->length);
            }
        }
    }
    XFreeDeviceList(DevList);

    SDL_NumOfXDevices = index;
#endif
}

void
X11_QuitMouse(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    /* !!! FIXME: use XCloseDevice()? Or maybe handle under SDL_MouseQuit()? */

    /* let's delete all of the mice */
    SDL_MouseQuit();
}

/* vi: set ts=4 sw=4 expandtab: */
