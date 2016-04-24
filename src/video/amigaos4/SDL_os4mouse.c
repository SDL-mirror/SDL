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

#include <intuition/pointerclass.h>

#include "SDL_os4video.h"
#include "SDL_os4window.h"

#include "../../events/SDL_mouse_c.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

#include <devices/input.h>

typedef struct SDL_CursorData {
	int hot_x;
	int hot_y;
	ULONG type;
	Object *object;
} SDL_CursorData;

static UWORD fallbackPointerData[2 * 16] = { 0 };

static struct BitMap fallbackPointerBitMap = { 2, 16, 0, 2, 0,
	{ (PLANEPTR)fallbackPointerData, (PLANEPTR)(fallbackPointerData + 16) } };


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
OS4_CreateCursorInternal()
{
	SDL_Cursor *cursor = SDL_calloc(1, sizeof(SDL_Cursor));

	//dprintf("Called\n");

	if (cursor) {

		SDL_CursorData *data = SDL_calloc(1, sizeof(SDL_CursorData));

		if (data) {
			cursor->driverdata = data;
		} else {
			dprintf("Failed to create cursor data\n");
	    }

	} else {
		dprintf("Failed to create cursor\n");
    }

	return cursor;

}

static SDL_Cursor*
OS4_CreateDefaultCursor()
{
	SDL_Cursor *cursor = OS4_CreateCursorInternal();

	dprintf("Called\n");

	if (cursor && cursor->driverdata) {
		SDL_CursorData *data = cursor->driverdata;

		data->type = POINTERTYPE_NORMAL;
	}
	
	return cursor;
}

static SDL_Cursor*
OS4_CreateCursor(SDL_Surface * surface, int hot_x, int hot_y)
{
	SDL_Cursor *cursor = OS4_CreateCursorInternal();

	dprintf("Called %p %d %d\n", surface, hot_x, hot_y);

	if (cursor && cursor->driverdata) {

		if (surface->w > 64) {		  
			dprintf("Invalid width %d\n", surface->w);
		} else if (surface->h > 64) {
			dprintf("Invalid height %d\n", surface->h);
		} else {
			_THIS = SDL_GetVideoDevice();

			/* We need to pass some compatibility parameters
			even though we are going to use just ARGB pointer */

			Object *object = IIntuition->NewObject(
				NULL,
				POINTERCLASS,
				POINTERA_BitMap, &fallbackPointerBitMap,
				POINTERA_XOffset, hot_x,
				POINTERA_YOffset, hot_y,
				//POINTERA_WordWidth, 1,
				//POINTERA_XResolution, POINTERXRESN_SCREENRES,
				//POINTERA_YResolution, POINTERYRESN_SCREENRES,
				POINTERA_ImageData, surface->pixels,
				POINTERA_Width, surface->w,
				POINTERA_Height, surface->h,
				TAG_DONE);

			if (object) {
				SDL_CursorData *data = cursor->driverdata;
				data->object = object;
			} else {
				dprintf("Failed to create pointer object\n");
			}
		}
	}

	return cursor;
}

static ULONG
OS4_MapCursorIdToNative(SDL_SystemCursor id)
{
	switch (id) {
		case SDL_SYSTEM_CURSOR_ARROW: return POINTERTYPE_NORMAL;
		case SDL_SYSTEM_CURSOR_IBEAM: return POINTERTYPE_SELECT;
		case SDL_SYSTEM_CURSOR_WAITARROW:
		case SDL_SYSTEM_CURSOR_WAIT: return POINTERTYPE_BUSY;
		case SDL_SYSTEM_CURSOR_CROSSHAIR: return POINTERTYPE_CROSS;
		case SDL_SYSTEM_CURSOR_SIZENWSE: return POINTERTYPE_NORTHWESTSOUTHEASTRESIZE;
		case SDL_SYSTEM_CURSOR_SIZENESW: return POINTERTYPE_NORTHEASTSOUTHWESTRESIZE;
		case SDL_SYSTEM_CURSOR_SIZEWE: return POINTERTYPE_EASTWESTRESIZE;
		case SDL_SYSTEM_CURSOR_SIZENS: return POINTERTYPE_NORTHSOUTHRESIZE;
		case SDL_SYSTEM_CURSOR_NO: return POINTERTYPE_NOTALLOWED;
		case SDL_SYSTEM_CURSOR_HAND: return POINTERTYPE_HAND;
		//
		case SDL_SYSTEM_CURSOR_SIZEALL:
		default:
			dprintf("Unknown mapping from type %d\n", id);
		    return POINTERTYPE_NORMAL;
	}
}

static SDL_Cursor*
OS4_CreateSystemCursor(SDL_SystemCursor id)
{
	SDL_Cursor *cursor = OS4_CreateCursorInternal();

	//dprintf("Called %d\n", id);

	if (cursor && cursor->driverdata) {
		SDL_CursorData *data = cursor->driverdata;

		data->type = OS4_MapCursorIdToNative(id);
	}

	return cursor;
}

static void
OS4_SetPointerForEachWindow(ULONG type, Object *object)
{
	SDL_Window *sdlwin;

	_THIS = SDL_GetVideoDevice();

	for (sdlwin = _this->windows; sdlwin; sdlwin = sdlwin->next) {

		SDL_WindowData *data = sdlwin->driverdata;

		if (data->syswin) {

			dprintf("Setting pointer object/type %p/%d for window %p\n", object, type, data->syswin);

			if (object) {
				IIntuition->SetWindowPointer(
					data->syswin,
					WA_Pointer, object,
					TAG_DONE);
			} else {
				IIntuition->SetWindowPointer(
					data->syswin,
					WA_PointerType, type,
					TAG_DONE);
			}
		}
	}
}


static int
OS4_ShowCursor(SDL_Cursor * cursor)
{
	ULONG type = POINTERTYPE_NORMAL;
	Object *object = NULL;

	if (cursor) {
		SDL_CursorData *data = cursor->driverdata;

		dprintf("Called %p %p\n", cursor, data);

		if (data) {
			type = data->type;
			object = data->object;
		} else {
			dprintf("No cursor data\n");
			return -1;
		}
	} else {
		dprintf("Hiding cursor\n");
		type = POINTERTYPE_NONE;
	}

	OS4_SetPointerForEachWindow(type, object);

	return 0;
}

static void
OS4_FreeCursor(SDL_Cursor * cursor)
{
	SDL_CursorData *data = cursor->driverdata;
	
	dprintf("Called %p\n", cursor);

	if (data) {

		if (data->object) {
			_THIS = SDL_GetVideoDevice();

			IIntuition->DisposeObject(data->object);
			data->object = NULL;
		}

		SDL_free(data);
		cursor->driverdata = NULL;
	}
}

static void
OS4_WarpMouse(SDL_Window * window, int x, int y)
{
	SDL_VideoDevice *device    = SDL_GetVideoDevice();
	SDL_VideoData *videoData   = device->driverdata;

	SDL_WindowData *winData    = window->driverdata;
	struct Window *syswin      = winData->syswin;

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
		OS4_FreeCursor(mouse->def_cursor);
        mouse->def_cursor = NULL;
        mouse->cur_cursor = NULL;
    }
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
