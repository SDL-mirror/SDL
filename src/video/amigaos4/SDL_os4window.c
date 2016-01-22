/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

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

#if SDL_VIDEO_DRIVER_AMIGAOS4

#include "SDL_os4video.h"
#include "SDL_syswm.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

static int
SetupWindowData(_THIS, SDL_Window * sdlwin, struct Window * syswin, SDL_bool created)
{
	SDL_WindowData *data;
	
	data = (SDL_WindowData *) SDL_calloc(1, sizeof(*data));
	if (!data) {
		return SDL_OutOfMemory();
	}

	data->sdlwin = sdlwin;
	data->syswin = syswin;
	data->created = created;

	sdlwin->driverdata = data;

	return 0;
}

int
OS4_CreateWindow(_THIS, SDL_Window * window)
{
	SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
	
	struct Window   * syswin;

	uint32 windowFlags = WFLG_REPORTMOUSE | WFLG_RMBTRAP;

	uint32 IDCMPFlags  = IDCMP_NEWSIZE | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE
					   | IDCMP_DELTAMOVE | IDCMP_RAWKEY | IDCMP_ACTIVEWINDOW
					   | IDCMP_INACTIVEWINDOW | IDCMP_INTUITICKS
					   | IDCMP_EXTENDEDMOUSE;
	
	uint32 windowX;
	uint32 windowY;

	dprintf("Called\n");

	if (window->flags & SDL_WINDOW_FULLSCREEN) {
	    // TODO: fullscreen
		windowFlags |= WFLG_BORDERLESS | WFLG_SIMPLE_REFRESH | WFLG_BACKDROP;
		
		windowX = 0;
		windowY = 0;

	} else {

		windowFlags |= WFLG_SMART_REFRESH | WFLG_NOCAREREFRESH | WFLG_NEWLOOKMENUS;

		windowX = window->x;
		windowY = window->y;

		if (window->flags & SDL_WINDOW_BORDERLESS) {
			windowFlags |= WFLG_BORDERLESS;
		} else {
			windowFlags |= WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET;

			IDCMPFlags  |= IDCMP_CLOSEWINDOW;
		}

		if (window->flags & SDL_WINDOW_RESIZABLE) {
			windowFlags |= WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM;
			
			IDCMPFlags  |= IDCMP_SIZEVERIFY;
		}
	}

	dprintf("Trying to open window '%s' at (%d,%d) of size (%dx%d)\n",
		window->title, windowX, windowY, window->w, window->h);

	syswin = IIntuition->OpenWindowTags(
		NULL,
		WA_PubScreen, videodata->publicScreen,
		WA_Title, window->title,
		WA_ScreenTitle, window->title,
		WA_Left, windowX,
		WA_Top, windowY,
		WA_InnerWidth, window->w,
		WA_InnerHeight, window->h,
		WA_Flags, windowFlags,
		WA_IDCMP, IDCMPFlags,
		WA_Hidden, (window->flags & SDL_WINDOW_SHOWN) ? FALSE : TRUE,
		//WA_GrabFocus, (window->flags & SDL_WINDOW_INPUT_GRABBED) ? 100 : 0,
		WA_UserPort, videodata->userport,
		TAG_DONE);

	if (!syswin) {		  
		return SDL_SetError("Couldn't create window");
	}

	if (window->flags & SDL_WINDOW_RESIZABLE) {
		
		/* If this window is resizable, reset window size limits
	 	 * so that the user can actually resize it.
		 *
		 * What's a useful minimum size, anyway?
		 */
		
		IIntuition->WindowLimits(syswin,
			syswin->BorderLeft + syswin->BorderRight + 100,
			syswin->BorderTop + syswin->BorderBottom + 100,
			-1,
			-1);
	}

	if (SetupWindowData(_this, window, syswin, SDL_TRUE) < 0) {
		IIntuition->CloseWindow(syswin);

		return SDL_SetError("Failed to setup window data");
	}

	// TODO: OpenGL context
	if (window->flags & SDL_WINDOW_OPENGL) {
		// ...
	}

	if (window->flags & SDL_WINDOW_FULLSCREEN) {
		//IIntuition->ScreenToFront(screen);
	}

	IIntuition->ActivateWindow(syswin);

	return 0;
}


int
OS4_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
	struct Window *syswin = (struct Window *) data;

	dprintf("Called\n");

	if (syswin->Title && SDL_strlen(syswin->Title)) {
		window->title = SDL_strdup(syswin->Title);
	}

	if (SetupWindowData(_this, window, syswin, SDL_FALSE) < 0) {
		return -1;
	}

	// TODO: OpenGL, (fullscreen may not be applicable here?)

	return 0;
}

void
OS4_SetWindowTitle(_THIS, SDL_Window * window)
{
	SDL_WindowData *data = window->driverdata;

	if (data && data->syswin) {
		STRPTR title = window->title ? window->title : "";

		IIntuition->SetWindowTitles(data->syswin, title, title);
	}
}

void
OS4_SetWindowPosition(_THIS, SDL_Window * window)
{
	SDL_WindowData *data = window->driverdata;

	if (data && data->syswin) {
	
		IIntuition->SetWindowAttrs(data->syswin,
			WA_Left, window->x,
			WA_Top, window->y,
			TAG_DONE);
	}
}

void
OS4_SetWindowSize(_THIS, SDL_Window * window)
{
	SDL_WindowData *data = window->driverdata;

	if (data && data->syswin) {

		IIntuition->SetWindowAttrs(data->syswin,
			WA_InnerWidth, window->w,
			WA_InnerHeight, window->h,
			TAG_DONE);
	}
}


void
OS4_ShowWindow(_THIS, SDL_Window * window)
{
	SDL_WindowData *data = window->driverdata;

	if (data && data->syswin) {
	
		// TODO: could use ShowWindow but what we pass for the Other?
		IIntuition->SetWindowAttrs(data->syswin,
			WA_Hidden, FALSE,
			TAG_DONE);
	}
}

void
OS4_HideWindow(_THIS, SDL_Window * window)
{
	SDL_WindowData *data = window->driverdata;

	if (data && data->syswin) {

		IIntuition->HideWindow(data->syswin);
	}
}

void
OS4_RaiseWindow(_THIS, SDL_Window * window)
{
	SDL_WindowData *data = window->driverdata;

	if (data && data->syswin) {
		IIntuition->WindowToFront(data->syswin);
	}
}

void
OS4_DestroyWindow(_THIS, SDL_Window * window)
{
	SDL_WindowData *data = window->driverdata;

	dprintf("Called\n");

	if (data) {
		if (data->created && data->syswin) {
			IIntuition->CloseWindow(data->syswin);
		}

		SDL_free(data);
	}

	if (window->flags & SDL_WINDOW_FULLSCREEN) {
		// TODO:fullscreen
	}

	if (window->flags & SDL_WINDOW_OPENGL) {
		// TODO: OpenGL context removal
	}
}

SDL_bool
OS4_GetWindowWMInfo(_THIS, SDL_Window * window,
									struct SDL_SysWMinfo *info)
{
	struct Window * syswin = ((SDL_WindowData *) window->driverdata)->syswin;

	dprintf("Called\n");

	if (info->version.major <= SDL_MAJOR_VERSION) {
		info->subsystem = SDL_SYSWM_OS4;
		info->info.os4.window = syswin;
		return SDL_TRUE;
	} else {
		SDL_SetError("Application not compiled with SDL %d.%d\n",
					SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
		return SDL_FALSE;
	}
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
