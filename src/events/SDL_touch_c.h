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

#ifndef _SDL_touch_c_h
#define _SDL_touch_c_h

typedef struct SDL_Touch SDL_Touch;


struct SDL_Touch
{
    /* Warp the touch to (x,y) */
    void (*WarpTouch) (SDL_Touch * touch, SDL_Window * window, int x,
                       int y);

    /* Free the touch when it's time */
    void (*FreeTouch) (SDL_Touch * touch);

    /* data common for tablets */
    int pressure;
    int pressure_max;
    int pressure_min;
    int tilt;                   /* for future use */
    int rotation;               /* for future use */
    int total_ends;
    int current_end;

    /* Data common to all touch */
    int id;
    SDL_Window *focus;
    int which;
    int x;
    int y;
    int z;                      /* for future use */
    int xdelta;
    int ydelta;
    int last_x, last_y;         /* the last reported x and y coordinates */
    char *name;
    Uint8 buttonstate;
    SDL_bool relative_mode;
    SDL_bool proximity;
    SDL_bool flush_motion;

    SDL_Cursor *cursors;
    SDL_Cursor *def_cursor;
    SDL_Cursor *cur_cursor;
    SDL_bool cursor_shown;

    void *driverdata;
};

/* Initialize the touch subsystem */
extern int SDL_TouchInit(void);

/* Get the touch at an index */
extern SDL_Touch *SDL_GetTouch(int index);

/* Add a touch, possibly reattaching at a particular index (or -1),
   returning the index of the touch, or -1 if there was an error.
 */
extern int SDL_AddTouch(const SDL_Touch * touch, char *name,
                        int pressure_max, int pressure_min, int ends);

/* Remove a touch at an index, clearing the slot for later */
extern void SDL_DelTouch(int index);

/* Clear the button state of a touch at an index */
extern void SDL_ResetTouch(int index);

/* Set the touch focus window */
extern void SDL_SetTouchFocus(int id, SDL_Window * window);

/* Send a touch motion event for a touch */
extern int SDL_SendTouchMotion(int id, int relative, int x, int y, int z);

/* Send a touch button event for a touch */
extern int SDL_SendTouchButton(int id, Uint8 state, Uint8 button);

/* Send a touch wheel event for a touch */
extern int SDL_SendTouchWheel(int id, int x, int y);

/* Send a proximity event for a touch */
extern int SDL_SendProximity(int id, int x, int y, int type);

/* Shutdown the touch subsystem */
extern void SDL_TouchQuit(void);

/* FIXME: Where do these functions go in this header? */
extern void SDL_ChangeEnd(int id, int end);

#endif /* _SDL_touch_c_h */

/* vi: set ts=4 sw=4 expandtab: */
