/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

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

#include "SDL_events.h"
#include "SDL_cocoavideo.h"

#include "../../events/SDL_mouse_c.h"

void
Cocoa_InitMouse(_THIS)
{
}

static int
ConvertMouseButtonToSDL(int button)
{
    switch (button)
    {
        case 0:
            return(SDL_BUTTON_LEFT);   /* 1 */
        case 1:
            return(SDL_BUTTON_RIGHT);  /* 3 */
        case 2:
            return(SDL_BUTTON_MIDDLE); /* 2 */
    }
    return button;
}

void
Cocoa_HandleMouseEvent(_THIS, NSEvent *event)
{
    int i;
    NSPoint point = { 0, 0 };
    SDL_Window *window;
    SDL_Window *focus = SDL_GetMouseFocus();

    /* See if there are any fullscreen windows that might handle this event */
    window = NULL;
    for (i = 0; i < _this->num_displays; ++i) {
        SDL_VideoDisplay *display = &_this->displays[i];
        SDL_Window *candidate = display->fullscreen_window;

        if (candidate) {
            SDL_Rect bounds;

            Cocoa_GetDisplayBounds(_this, display, &bounds);
            point = [NSEvent mouseLocation];
            point.x = point.x - bounds.x;
            point.y = CGDisplayPixelsHigh(kCGDirectMainDisplay) - point.y - bounds.y;
            if ((point.x >= 0 && point.x < candidate->w) &&
                (point.y >= 0 && point.y < candidate->h)) {
                /* This is it! */
                window = candidate;
                break;
            } else if (candidate == focus) {
                SDL_SetMouseFocus(NULL);
            }
        }
    }

    if (!window) {
        return;
    }

    switch ([event type]) {
    case NSLeftMouseDown:
    case NSOtherMouseDown:
    case NSRightMouseDown:
        SDL_SendMouseButton(window, SDL_PRESSED, ConvertMouseButtonToSDL([event buttonNumber]));
        break;
    case NSLeftMouseUp:
    case NSOtherMouseUp:
    case NSRightMouseUp:
        SDL_SendMouseButton(window, SDL_RELEASED, ConvertMouseButtonToSDL([event buttonNumber]));
        break;
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    case NSOtherMouseDragged: /* usually middle mouse dragged */
    case NSMouseMoved:
        SDL_SendMouseMotion(window, 0, (int)point.x, (int)point.y);
        break;
    default: /* just to avoid compiler warnings */
        break;
    }
}

void
Cocoa_QuitMouse(_THIS)
{
}

/* vi: set ts=4 sw=4 expandtab: */
