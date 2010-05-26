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

/* General touch handling code for SDL */

#include "SDL_events.h"
#include "SDL_events_c.h"
#include "../video/SDL_sysvideo.h"


static int SDL_num_touch = 0;
static int SDL_current_touch = -1;
static SDL_Touch **SDL_touch = NULL;


/* Public functions */
int
SDL_TouchInit(void)
{
    return (0);
}

SDL_Touch *
SDL_GetTouch(int index)
{
    if (index < 0 || index >= SDL_num_touch) {
        return NULL;
    }
    return SDL_touch[index];
}

static int
SDL_GetTouchIndexId(int id)
{
    int index;
    SDL_Touch *touch;

    for (index = 0; index < SDL_num_touch; ++index) {
        touch = SDL_GetTouch(index);
        if (touch->id == id) {
            return index;
        }
    }
    return -1;
}

int
SDL_AddTouch(const SDL_Touch * touch, char *name, int pressure_max,
             int pressure_min, int ends)
{
    SDL_Touch **touch;
    int selected_touch;
    int index;
    size_t length;

    if (SDL_GetTouchIndexId(touch->id) != -1) {
        SDL_SetError("Touch ID already in use");
    }

    /* Add the touch to the list of touch */
    touch = (SDL_Touch **) SDL_realloc(SDL_touch,
                                      (SDL_num_touch + 1) * sizeof(*touch));
    if (!touch) {
        SDL_OutOfMemory();
        return -1;
    }

    SDL_touch = touch;
    index = SDL_num_touch++;

    SDL_touch[index] = (SDL_Touch *) SDL_malloc(sizeof(*SDL_touch[index]));
    if (!SDL_touch[index]) {
        SDL_OutOfMemory();
        return -1;
    }
    *SDL_touch[index] = *touch;

    /* we're setting the touch properties */
    length = 0;
    length = SDL_strlen(name);
    SDL_touch[index]->focus = 0;
    SDL_touch[index]->name = SDL_malloc((length + 2) * sizeof(char));
    SDL_strlcpy(SDL_touch[index]->name, name, length + 1);
    SDL_touch[index]->pressure_max = pressure_max;
    SDL_touch[index]->pressure_min = pressure_min;
    SDL_touch[index]->cursor_shown = SDL_TRUE;
    selected_touch = SDL_SelectTouch(index);
    SDL_touch[index]->cur_cursor = NULL;
    SDL_touch[index]->def_cursor =
    /* we're assuming that all touch are in the computer sensing zone */
    SDL_touch[index]->proximity = SDL_TRUE;
    /* we're assuming that all touch are working in the absolute position mode
       thanx to that, the users that don't want to use many touch don't have to
       worry about anything */
    SDL_touch[index]->relative_mode = SDL_FALSE;
    SDL_touch[index]->current_end = 0;
    SDL_touch[index]->total_ends = ends;
    SDL_SelectTouch(selected_touch);

    return index;
}

void
SDL_DelTouch(int index)
{
    SDL_Touch *touch = SDL_GetTouch(index);

    if (!touch) {
        return;
    }

    touch->def_cursor = NULL;
    SDL_free(touch->name);
 
    if (touch->FreeTouch) {
        touch->FreeTouch(touch);
    }
    SDL_free(touch);

    SDL_touch[index] = NULL;
}

void
SDL_ResetTouch(int index)
{
    SDL_Touch *touch = SDL_GetTouch(index);

    if (!touch) {
        return;
    }

    /* FIXME */
}

void
SDL_TouchQuit(void)
{
    int i;

    for (i = 0; i < SDL_num_touch; ++i) {
        SDL_DelTouch(i);
    }
    SDL_num_touch = 0;
    SDL_current_touch = -1;

    if (SDL_touch) {
        SDL_free(SDL_touch);
        SDL_touch = NULL;
    }
}

int
SDL_GetNumTouch(void)
{
    return SDL_num_touch;
}

int
SDL_SelectTouch(int index)
{
    if (index >= 0 && index < SDL_num_touch) {
        SDL_current_touch = index;
    }
    return SDL_current_touch;
}

SDL_Window *
SDL_GetTouchFocusWindow(int index)
{
    SDL_Touch *touch = SDL_GetTouch(index);

    if (!touch) {
        return 0;
    }
    return touch->focus;
}

static int SDLCALL
FlushTouchMotion(void *param, SDL_Event * event)
{
    if (event->type == SDL_TOUCHMOTION
        && event->motion.which == (Uint8) SDL_current_touch) {
        return 0;
    } else {
        return 1;
    }
}

int
SDL_SetRelativeTouchMode(int index, SDL_bool enabled)
{
    SDL_Touch *touch = SDL_GetTouch(index);

    if (!touch) {
        return -1;
    }

    /* Flush pending touch motion */
    touch->flush_motion = SDL_TRUE;
    SDL_PumpEvents();
    touch->flush_motion = SDL_FALSE;
    SDL_FilterEvents(FlushTouchMotion, touch);

    /* Set the relative mode */
    touch->relative_mode = enabled;



    if (!enabled) {
        /* Restore the expected touch position */
        SDL_WarpTouchInWindow(touch->focus, touch->x, touch->y);
    }
    return 0;
}

SDL_bool
SDL_GetRelativeTouchMode(int index)
{
    SDL_Touch *touch = SDL_GetTouch(index);

    if (!touch) {
        return SDL_FALSE;
    }
    return touch->relative_mode;
}

Uint8
SDL_GetTouchState(int *x, int *y)
{
    SDL_Touch *touch = SDL_GetTouch(SDL_current_touch);

    if (!touch) {
        if (x) {
            *x = 0;
        }
        if (y) {
            *y = 0;
        }
        return 0;
    }

    if (x) {
        *x = touch->x;
    }
    if (y) {
        *y = touch->y;
    }
    return touch->buttonstate;
}

Uint8
SDL_GetRelativeTouchState(int index, int *x, int *y)
{
    SDL_Touch *touch = SDL_GetTouch(index);

    if (!touch) {
        if (x) {
            *x = 0;
        }
        if (y) {
            *y = 0;
        }
        return 0;
    }

    if (x) {
        *x = touch->xdelta;
    }
    if (y) {
        *y = touch->ydelta;
    }
    touch->xdelta = 0;
    touch->ydelta = 0;
    return touch->buttonstate;
}

void
SDL_SetTouchFocus(int id, SDL_Window * window)
{
    int index = SDL_GetTouchIndexId(id);
    SDL_Touch *touch = SDL_GetTouch(index);
    int i;
    SDL_bool focus;

    if (!touch || (touch->focus == window)) {
        return;
    }

    /* See if the current window has lost focus */
    if (touch->focus) {
        focus = SDL_FALSE;
        for (i = 0; i < SDL_num_touch; ++i) {
            SDL_Touch *check;
            if (i != index) {
                check = SDL_GetTouch(i);
                if (check && check->focus == touch->focus) {
                    focus = SDL_TRUE;
                    break;
                }
            }
        }
        if (!focus) {
            SDL_SendWindowEvent(touch->focus, SDL_WINDOWEVENT_LEAVE, 0, 0);
        }
    }

    touch->focus = window;

    if (touch->focus) {
        focus = SDL_FALSE;
        for (i = 0; i < SDL_num_touch; ++i) {
            SDL_Touch *check;
            if (i != index) {
                check = SDL_GetTouch(i);
                if (check && check->focus == touch->focus) {
                    focus = SDL_TRUE;
                    break;
                }
            }
        }
        if (!focus) {
            SDL_SendWindowEvent(touch->focus, SDL_WINDOWEVENT_ENTER, 0, 0);
        }
    }
}

int
SDL_SendProximity(int id, int x, int y, int type)
{
    int index = SDL_GetTouchIndexId(id);
    SDL_Touch *touch = SDL_GetTouch(index);
    int posted = 0;

    if (!touch) {
        return 0;
    }

    touch->last_x = x;
    touch->last_y = y;
    if (SDL_GetEventState(type) == SDL_ENABLE) {
        SDL_Event event;
        event.proximity.which = (Uint8) index;
        event.proximity.x = x;
        event.proximity.y = y;
        event.proximity.cursor = touch->current_end;
        event.proximity.type = type;
        /* FIXME: is this right? */
        event.proximity.windowID = touch->focus ? touch->focus->id : 0;
        posted = (SDL_PushEvent(&event) > 0);
        if (type == SDL_PROXIMITYIN) {
            touch->proximity = SDL_TRUE;
        } else {
            touch->proximity = SDL_FALSE;
        }
    }
    return posted;
}

int
SDL_SendTouchMotion(int id, int relative, int x, int y, int pressure)
{
    int index = SDL_GetTouchIndexId(id);
    SDL_Touch *touch = SDL_GetTouch(index);
    int posted;
    int xrel;
    int yrel;
    int x_max = 0, y_max = 0;

    if (!touch || touch->flush_motion) {
        return 0;
    }

    /* if the touch is out of proximity we don't to want to have any motion from it */
    if (touch->proximity == SDL_FALSE) {
        touch->last_x = x;
        touch->last_y = y;
        return 0;
    }

    /* the relative motion is calculated regarding the system cursor last position */
    if (relative) {
        xrel = x;
        yrel = y;
        x = (touch->last_x + x);
        y = (touch->last_y + y);
    } else {
        xrel = x - touch->last_x;
        yrel = y - touch->last_y;
    }

    /* Drop events that don't change state */
    if (!xrel && !yrel) {
#if 0
        printf("Touch event didn't change state - dropped!\n");
#endif
        return 0;
    }

    /* Update internal touch coordinates */
    if (touch->relative_mode == SDL_FALSE) {
        touch->x = x;
        touch->y = y;
    } else {
        touch->x += xrel;
        touch->y += yrel;
    }

    SDL_GetWindowSize(touch->focus, &x_max, &y_max);

    /* make sure that the pointers find themselves inside the windows */
    /* only check if touch->xmax is set ! */
    if (x_max && touch->x > x_max) {
        touch->x = x_max;
    } else if (touch->x < 0) {
        touch->x = 0;
    }

    if (y_max && touch->y > y_max) {
        touch->y = y_max;
    } else if (touch->y < 0) {
        touch->y = 0;
    }

    touch->xdelta += xrel;
    touch->ydelta += yrel;
    touch->pressure = pressure;



    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(SDL_TOUCHMOTION) == SDL_ENABLE &&
        touch->proximity == SDL_TRUE) {
        SDL_Event event;
        event.motion.type = SDL_TOUCHMOTION;
        event.motion.which = (Uint8) index;
        event.motion.state = touch->buttonstate;
        event.motion.x = touch->x;
        event.motion.y = touch->y;
        event.motion.z = touch->z;
        event.motion.pressure = touch->pressure;
        event.motion.pressure_max = touch->pressure_max;
        event.motion.pressure_min = touch->pressure_min;
        event.motion.rotation = 0;
        event.motion.tilt_x = 0;
        event.motion.tilt_y = 0;
        event.motion.cursor = touch->current_end;
        event.motion.xrel = xrel;
        event.motion.yrel = yrel;
        event.motion.windowID = touch->focus ? touch->focus->id : 0;
        posted = (SDL_PushEvent(&event) > 0);
    }
    touch->last_x = touch->x;
    touch->last_y = touch->y;
    return posted;
}

int
SDL_SendTouchButton(int id, Uint8 state, Uint8 button)
{
    int index = SDL_GetTouchIndexId(id);
    SDL_Touch *touch = SDL_GetTouch(index);
    int posted;
    Uint32 type;

    if (!touch) {
        return 0;
    }

    /* Figure out which event to perform */
    switch (state) {
    case SDL_PRESSED:
        if (touch->buttonstate & SDL_BUTTON(button)) {
            /* Ignore this event, no state change */
            return 0;
        }
        type = SDL_TOUCHBUTTONDOWN;
        touch->buttonstate |= SDL_BUTTON(button);
        break;
    case SDL_RELEASED:
        if (!(touch->buttonstate & SDL_BUTTON(button))) {
            /* Ignore this event, no state change */
            return 0;
        }
        type = SDL_TOUCHBUTTONUP;
        touch->buttonstate &= ~SDL_BUTTON(button);
        break;
    default:
        /* Invalid state -- bail */
        return 0;
    }

    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(type) == SDL_ENABLE) {
        SDL_Event event;
        event.type = type;
        event.button.which = (Uint8) index;
        event.button.state = state;
        event.button.button = button;
        event.button.x = touch->x;
        event.button.y = touch->y;
        event.button.windowID = touch->focus ? touch->focus->id : 0;
        posted = (SDL_PushEvent(&event) > 0);
    }
    return posted;
}

int
SDL_SendTouchWheel(int index, int x, int y)
{
    SDL_Touch *touch = SDL_GetTouch(index);
    int posted;

    if (!touch || (!x && !y)) {
        return 0;
    }

    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(SDL_TOUCHWHEEL) == SDL_ENABLE) {
        SDL_Event event;
        event.type = SDL_TOUCHWHEEL;
        event.wheel.which = (Uint8) index;
        event.wheel.x = x;
        event.wheel.y = y;
        event.wheel.windowID = touch->focus ? touch->focus->id : 0;
        posted = (SDL_PushEvent(&event) > 0);
    }
    return posted;
}


char *
SDL_GetTouchName(int index)
{
    SDL_Touch *touch = SDL_GetTouch(index);
    if (!touch) {
        return NULL;
    }
    return touch->name;
}

void
SDL_ChangeEnd(int id, int end)
{
    int index = SDL_GetTouchIndexId(id);
    SDL_Touch *touch = SDL_GetTouch(index);

    if (touch) {
        touch->current_end = end;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
