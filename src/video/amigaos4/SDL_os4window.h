/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

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

#ifndef _SDL_os4window_h
#define _SDL_os4window_h

#include "../SDL_sysvideo.h"

#define POINTER_GRAB_TIMEOUT        20  /* Number of ticks before pointer grab needs to be reactivated */

#define GID_ICONIFY 123

typedef struct HitTestInfo
{
    SDL_HitTestResult htr;
    SDL_Point point;
} HitTestInfo;

typedef struct
{
    SDL_Window      * sdlwin;
    struct Window   * syswin;
    struct BitMap   * bitmap;
    struct AppWindow * appWin;
    struct AppIcon  * appIcon;

    Uint32            pointerGrabTicks;

    void*           * glContext;
    struct BitMap   * glFrontBuffer;
    struct BitMap   * glBackBuffer;

    HitTestInfo       hti;

    struct Gadget   * gadget;
    struct Image    * image;

} SDL_WindowData;

extern void OS4_GetWindowSize(_THIS, struct Window * window, int * width, int * height);
extern void OS4_GetWindowActiveSize(SDL_Window * window, int * width, int * height);

extern int OS4_CreateWindow(_THIS, SDL_Window * window);
extern int OS4_CreateWindowFrom(_THIS, SDL_Window * window, const void *data);
extern void OS4_SetWindowTitle(_THIS, SDL_Window * window);
//extern void OS4_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon);
extern void OS4_SetWindowBox(_THIS, SDL_Window * window);
extern void OS4_SetWindowPosition(_THIS, SDL_Window * window);
extern void OS4_SetWindowSize(_THIS, SDL_Window * window);
extern void OS4_ShowWindow(_THIS, SDL_Window * window);
extern void OS4_HideWindow(_THIS, SDL_Window * window);
extern void OS4_RaiseWindow(_THIS, SDL_Window * window);

extern void OS4_SetWindowMinMaxSize(_THIS, SDL_Window * window);

extern void OS4_MaximizeWindow(_THIS, SDL_Window * window);
extern void OS4_MinimizeWindow(_THIS, SDL_Window * window);
extern void OS4_RestoreWindow(_THIS, SDL_Window * window);

extern void OS4_SetWindowResizable (_THIS, SDL_Window * window, SDL_bool resizable);

//extern void OS4_SetWindowBordered(_THIS, SDL_Window * window, SDL_bool bordered);
extern void OS4_SetWindowFullscreen(_THIS, SDL_Window * window, SDL_VideoDisplay * display, SDL_bool fullscreen);
//extern int OS4_SetWindowGammaRamp(_THIS, SDL_Window * window, const Uint16 * ramp);
//extern int OS4_GetWindowGammaRamp(_THIS, SDL_Window * window, Uint16 * ramp);

extern void OS4_SetWindowGrabPrivate(_THIS, struct Window * w, SDL_bool activate);
extern void OS4_SetWindowGrab(_THIS, SDL_Window * window, SDL_bool grabbed);
extern void OS4_DestroyWindow(_THIS, SDL_Window * window);
extern SDL_bool OS4_GetWindowWMInfo(_THIS, SDL_Window * window, struct SDL_SysWMinfo *info);

//extern void OS4_OnWindowEnter(_THIS, SDL_Window * window);
//extern void OS4_UpdateClipCursor(SDL_Window *window);

extern int OS4_SetWindowHitTest(SDL_Window * window, SDL_bool enabled);

extern int OS4_SetWindowOpacity(_THIS, SDL_Window * window, float opacity);
extern int OS4_GetWindowBordersSize(_THIS, SDL_Window * window, int * top, int * left, int * bottom, int * right);

extern void OS4_IconifyWindow(_THIS, SDL_Window * window);
extern void OS4_UniconifyWindow(_THIS, SDL_Window * window);

#endif /* _SDL_os4window_h */

/* vi: set ts=4 sw=4 expandtab: */
