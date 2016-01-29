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
#include "SDL_os4window.h"

#include "../../events/SDL_mouse_c.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

#include <devices/input.h>

static Uint32
OS4_GetDoubleClickTimeInMillis(_THIS)
{
	struct Preferences preferences;
	Uint32 interval;

	IIntuition->GetPrefs(&preferences, sizeof(preferences));
	
	interval =  preferences.DoubleClick.Seconds * 1000 +
				preferences.DoubleClick.Microseconds / 1000;

	dprintf("Doubleclick time %d ms\n", interval);

	return interval;
}

static SDL_Cursor*
OS4_CreateDefaultCursor()
{
	return NULL;
}

static SDL_Cursor*
OS4_CreateCursor(SDL_Surface * surface, int hot_x, int hot_y)
{
	return NULL;
}

static SDL_Cursor*
OS4_CreateSystemCursor(SDL_SystemCursor id)
{
	return NULL;
}

static int
OS4_ShowCursor(SDL_Cursor * cursor)
{
	return 0;
}

static void
OS4_FreeCursor(SDL_Cursor * cursor)
{
}

static void
OS4_WarpMouse(SDL_Window * window, int x, int y)
{
	SDL_VideoDevice * device    = SDL_GetVideoDevice();
	SDL_VideoData * videoData   = device->driverdata;

	SDL_WindowData * winData    = window->driverdata;
	struct Window * syswin      = winData->syswin;

	BOOL warpHostPointer;

	dprintf("Warping mouse to %d, %d\n", x, y);

	/* If the host mouse pointer is outside of the SDL window or the SDL
	 * window is inactive then we just need to warp SDL's notion of where
	 * the pointer position is - we don't need to move the Workbench
	 * pointer. In the former case, we don't pass mass movements on to
	 * to the app anyway and in the latter case we don't receive mouse
	 * movements events anyway.
	 */
	//warpHostPointer  = (hidden->pointerState == pointer_inside_window) && !hidden->isMouseRelative
	//				  &&  hidden->windowActive;

	// TODO: "inside window" logic needed/wanted?
	warpHostPointer = !SDL_GetRelativeMouseMode() && (window == SDL_GetMouseFocus());

	if (warpHostPointer && (videoData->inputReq != NULL)) {
		struct InputEvent     *FakeEvent;
		struct IEPointerPixel *NeoPix;

		/* We move the Workbench pointer by stuffing mouse
		 * movement events in input.device's queue.
		 *
		 * This in turn will cause mouse movement events to be
		 * sent back to us
		 */

		/* Allocate the event from our pool */
		FakeEvent = (struct InputEvent *)OS4_SaveAllocPooled(
			device, sizeof(struct InputEvent) + sizeof(struct IEPointerPixel));

		if (FakeEvent) {
			dprintf("Building event structure\n");

			NeoPix = (struct IEPointerPixel *) (FakeEvent + 1);

			if (window->flags & SDL_WINDOW_FULLSCREEN)
				NeoPix->iepp_Screen = syswin->WScreen;
			else
				NeoPix->iepp_Screen = videoData->publicScreen;

			NeoPix->iepp_Position.Y = y + syswin->BorderTop  + syswin->TopEdge;
			NeoPix->iepp_Position.X = x + syswin->BorderLeft + syswin->LeftEdge;

			FakeEvent->ie_EventAddress = (APTR)NeoPix;
			FakeEvent->ie_NextEvent    = NULL;
			FakeEvent->ie_Class        = IECLASS_NEWPOINTERPOS;
			FakeEvent->ie_SubClass     = IESUBCLASS_PIXEL;
			FakeEvent->ie_Code         = IECODE_NOBUTTON;
			FakeEvent->ie_Qualifier    = 0;

			videoData->inputReq->io_Data    = (APTR)FakeEvent;
			videoData->inputReq->io_Length  = sizeof(struct InputEvent);
			videoData->inputReq->io_Command = IND_WRITEEVENT;

			dprintf("Fire!\n");

			IExec->DoIO((struct IORequest *)videoData->inputReq);

			OS4_SaveFreePooled(device, (void *)FakeEvent,
				sizeof(struct InputEvent) + sizeof(struct IEPointerPixel));
		}
	} else {
		/* Just warp SDL's notion of the pointer position */
		SDL_SendMouseMotion(0, 0, SDL_GetRelativeMouseMode(), x, y);
	}

	dprintf("Done\n");
}

static int
OS4_SetRelativeMouseMode(SDL_bool enabled)
{
	return 0;
}

void
OS4_InitMouse(_THIS)
{
	//SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

	SDL_Mouse *mouse = SDL_GetMouse();

	mouse->CreateCursor = OS4_CreateCursor;
	mouse->CreateSystemCursor = OS4_CreateSystemCursor;
	mouse->ShowCursor = OS4_ShowCursor;
	mouse->FreeCursor = OS4_FreeCursor;
	mouse->WarpMouse = OS4_WarpMouse;
	mouse->SetRelativeMouseMode = OS4_SetRelativeMouseMode;

	SDL_SetDefaultCursor( OS4_CreateDefaultCursor() );

	SDL_SetDoubleClickTime( OS4_GetDoubleClickTimeInMillis(_this) );
}

void
OS4_QuitMouse(_THIS)
{
    SDL_Mouse *mouse = SDL_GetMouse();

    if ( mouse->def_cursor ) {
        SDL_free(mouse->def_cursor);
        mouse->def_cursor = NULL;
        mouse->cur_cursor = NULL;
    }
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
