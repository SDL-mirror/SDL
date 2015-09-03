/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#include "../../events/SDL_events_c.h"
#include "SDL_os4video.h"
#include "SDL_os4utils.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/Picasso96API.h>
#include <proto/input.h>
#include <proto/wb.h>
#include <proto/icon.h>

#include <devices/input.h>
#include <devices/inputevent.h>

//#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

extern struct GraphicsIFace  *SDL_IGraphics;
extern struct IntuitionIFace *SDL_IIntuition;
extern struct WorkbenchIFace *SDL_IWorkbench;
extern struct IconIFace      *SDL_IIcon;

struct WMcursor
{
	uint16 *Image;
	int16 Width;
	int16 Height;
	int16 XOffset;
	int16 YOffset;
};

struct IOStdReq  *inputReq = 0;
struct MsgPort   *inputPort = 0;


void _INIT_CreateInput(void) __attribute__((constructor));
void _EXIT_DeleteInput(void) __attribute__((destructor));

void _INIT_CreateInput(void)
{
	inputPort = IExec->AllocSysObjectTags(ASOT_PORT, TAG_DONE);
	if (inputPort)
	{
		inputReq = IExec->AllocSysObjectTags(ASOT_IOREQUEST,
											 ASOIOR_Size, 		sizeof(struct IOStdReq),
											 ASOIOR_ReplyPort,	inputPort,
											 TAG_DONE);
		if (inputReq)
		{
			if (!IExec->OpenDevice("input.device", 0, (struct IORequest *)inputReq, 0))
			{
				dprintf("Device opened\n");
				return;
			}
		}

		IExec->FreeSysObject(ASOT_IOREQUEST, inputReq);
	}

	IExec->FreeSysObject(ASOT_PORT, inputPort);
}

void _EXIT_DeleteInput(void)
{
	if (inputReq)
	{
		dprintf("Deleting input device\n");
		//IExec->AbortIO((struct IORequest *)inputReq);
		//IExec->WaitIO((struct IORequest *)inputReq);
		IExec->CloseDevice((struct IORequest *)inputReq);
		dprintf("Deleting IORequest\n");
		IExec->FreeSysObject(ASOT_IOREQUEST, (void *)inputReq);
		dprintf("Deleting MsgPort\n");
		IExec->FreeSysObject(ASOT_PORT, (void *)inputPort);
		dprintf("Done\n");
	}
}

void os4video_SetCaption(_THIS, const char *title, const char *icon)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;

	if (title) {
		dprintf("Setting title to %s\n", title);
		SDL_strlcpy(hidden->currentCaption, title, 127);
		if (hidden->win)
			SDL_IIntuition->SetWindowTitles(hidden->win, hidden->currentCaption, hidden->currentCaption);
	}
        if (icon) {
		dprintf("Setting icon caption to %s\n", title);
		SDL_strlcpy(hidden->currentIconCaption, icon, 127);
	}
}

WMcursor *
os4video_CreateWMCursor(_THIS, Uint8 *data, Uint8 *mask,
					int w, int h, int hot_x, int hot_y)
{
	WMcursor *cursor;
	struct SDL_PrivateVideoData *hidden = _this->hidden;
	uint16 *pCurImage;
	int y;

	dprintf("Creating cursor %dx%d\n", w, h);

	/* Verify size */
	if (w > 16)
	{
		dprintf("Cursor too wide\n");
		SDL_SetError("Mouse pointer must be <= 16 pixels wide");
		return 0;
	}

	/* Create control structure */
	cursor = SaveAllocPooled(hidden, sizeof(struct WMcursor));
	if (!cursor)
	{
		dprintf("No memory for cursor\n");
		SDL_SetError("Not enough memory to allocate cursor control structure");
		return 0;
	}

	cursor->Width = w;
	cursor->Height = h;
	cursor->XOffset = -hot_x;
	cursor->YOffset = -hot_y;

	/* Create image data */
	cursor->Image = IExec->AllocVecTags(4*h+4, AVT_ClearWithValue, 0, AVT_Type, MEMF_SHARED, TAG_DONE );
	if (!cursor->Image)
	{
		dprintf("No memory for image\n");
		SaveFreePooled(hidden, cursor, sizeof(struct WMcursor));
		SDL_SetError("Not enough memory to allocate cursor image data");
		return 0;
	}

	/* Convert the image */
	pCurImage = cursor->Image + 2;

	for (y = 0; y < h; y++)
	{
		/* Case one: The image is <= 8: Use the data as the high byte of the
		   first plane, and the mask as the high byte of the second plane */
		if (w <= 8)
		{
			*pCurImage++ = (*mask++)<<8;
			*pCurImage++ = (*data++)<<8;
		}
		/* Case two: The image is 8 < w <= 16: Use two bytes of data in one word for
		   first plane, and two bytes of mask for the second plane */
		else
		{
			*pCurImage++ = mask[0] << 8 | mask[1];
			*pCurImage++ = data[0] << 8 | data[1];
			data += 2;
			mask += 2;
		}
	}

	dprintf("cursor = %p\n", cursor);

	return cursor;
}

void os4video_FreeWMCursor(_THIS, WMcursor *cursor)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;

	dprintf("Freeing %p\n", cursor);

	if (cursor && cursor->Image)
		IExec->FreeVec(cursor->Image);

	if (cursor)
		SaveFreePooled(hidden, cursor, sizeof(struct WMcursor));
}

void os4video_ResetCursor(struct SDL_PrivateVideoData *hidden)
{
	WMcursor *cursor = hidden->currentCursor;

	if (cursor)
	{
		SDL_IIntuition->SetPointer(hidden->win, cursor->Image, cursor->Width, cursor->Height,
							   cursor->XOffset, cursor->YOffset);

		dprintf("Cursor image set\n");
	}
	else
	{
		if (hidden->mouse)
		{
			SDL_IIntuition->SetPointer(hidden->win, hidden->mouse, 0, 0, 0, 0);

			dprintf("Cursor image blanked\n");
		}
	}
}

int	os4video_ShowWMCursor(_THIS, WMcursor *cursor)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;

	dprintf("Setting cursor %p\n", cursor);

	SDL_Lock_EventThread();

	hidden->currentCursor = (void *)cursor;

	if (hidden->win && (hidden->pointerState != pointer_outside_window))
		os4video_ResetCursor(hidden);

	SDL_Unlock_EventThread();

	return 1;
}

void ResetMouseState(_THIS)
{
	if (_this->ShowWMCursor)
		_this->ShowWMCursor(_this, _this->hidden->currentCursor);
}

void os4video_WarpWMCursor(_THIS, Uint16 x, Uint16 y)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;
	BOOL warpHostPointer;

	dprintf("Warping mouse to %d, %d\n", x, y);

	SDL_Lock_EventThread();

	/* If the host mouse pointer is outside of the SDL window or the SDL
	 * window is inactive then we just need to warp SDL's notion of where
	 * the pointer position is - we don't need to move the Workbench
	 * pointer. In the former case, we don't pass mass movements on to
	 * to the app anyway and in the latter case we don't receive mouse
	 * movements events anyway.
	 */
	warpHostPointer  = (hidden->pointerState == pointer_inside_window) && !hidden->isMouseRelative
					&&  hidden->windowActive;

	if (warpHostPointer && (inputReq != NULL))
	{
		struct InputEvent     *FakeEvent;
		struct IEPointerPixel *NeoPix;

		/* We move the Workbench pointer by stuffing mouse
		 * movement events in input.device's queue.
		 *
		 * This in turn will cause mouse movement events to be
		 * sent back to us
		 */

		/* Allocate the event from our pool */
		FakeEvent = (struct InputEvent *)SaveAllocPooled(hidden, sizeof(struct InputEvent)+sizeof(struct IEPointerPixel));

		if (FakeEvent)
		{
			dprintf("Building event structure\n");

			NeoPix = (struct IEPointerPixel *) (FakeEvent + 1);

			if (hidden->scr)
				NeoPix->iepp_Screen = hidden->scr;
			else
				NeoPix->iepp_Screen = hidden->publicScreen;

			NeoPix->iepp_Position.Y = y + hidden->win->BorderTop  + hidden->win->TopEdge;
			NeoPix->iepp_Position.X = x + hidden->win->BorderLeft + hidden->win->LeftEdge;

			FakeEvent->ie_EventAddress = (APTR)NeoPix;
			FakeEvent->ie_NextEvent    = NULL;
			FakeEvent->ie_Class        = IECLASS_NEWPOINTERPOS;
			FakeEvent->ie_SubClass     = IESUBCLASS_PIXEL;
			FakeEvent->ie_Code         = IECODE_NOBUTTON;
			FakeEvent->ie_Qualifier    = 0;

			inputReq->io_Data    = (APTR)FakeEvent;
			inputReq->io_Length  = sizeof(struct InputEvent);
			inputReq->io_Command = IND_WRITEEVENT;

			dprintf("Fire!\n");

			IExec->DoIO((struct IORequest *)inputReq);

			SaveFreePooled(hidden, (void *)FakeEvent,  sizeof(struct InputEvent)+sizeof(struct IEPointerPixel));
		}
	}
	else
	{
		/* Just warp SDL's notion of the pointer position */
		SDL_PrivateMouseMotion(0, 0, x, y);
	}
	SDL_Unlock_EventThread();

	dprintf("Done\n");
}


void SetMouseColors(_THIS)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;
	struct ViewPort *vp;
	uint32 colors[15];

	if (hidden->scr)
		vp = &hidden->scr->ViewPort;
	else
		vp = &hidden->publicScreen->ViewPort;

	if (!hidden->mouseColorsValid)
	{
		/* Store old colors */
		SDL_IGraphics->GetRGB32(vp->ColorMap, 16, 4, &hidden->mouseColors[1]);
		hidden->mouseColorsValid = 1;
	}

	/* Build array for new colors */
	colors[0] = 4 << 16 | 16;

	/* Transparent */
	colors[1] = 0;
	colors[2] = 0;
	colors[3] = 0;

	/* White */
	colors[4] = 0xff000000;
	colors[5] = 0xff000000;
	colors[6] = 0xff000000;

	/* Invert */
	colors[7] = 0;
	colors[8] = 0;
	colors[9] = 0;

	/* Black */
	colors[10] = 0;
	colors[11] = 0;
	colors[12] = 0;

	colors[13] = 0;

	SDL_IGraphics->LoadRGB32(vp, colors);
}

void ResetMouseColors(_THIS)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;
	struct ViewPort *vp;

	if (!hidden->mouseColorsValid)
		return;

	if (hidden->scr)
		vp = &hidden->scr->ViewPort;
	else
		vp = &hidden->publicScreen->ViewPort;

	/* Finish the color array */
	hidden->mouseColors[0] = 4 << 16 | 16;
	hidden->mouseColors[13] = 0;

	/* Just load old colors */
	SDL_IGraphics->LoadRGB32(vp, hidden->mouseColors);
	hidden->mouseColorsValid = 0;
}


void os4video_UpdateMouse(_THIS)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;
	int16 MouseX, MouseY;

	SDL_Lock_EventThread();

	if (hidden->scr)
	{
		MouseX = hidden->scr->MouseX;
		MouseY = hidden->scr->MouseY;
	}
	else
	{
		/* Need to subtract window position and border */
		MouseX = hidden->publicScreen->MouseX - hidden->win->LeftEdge - hidden->win->BorderLeft;
		MouseY = hidden->publicScreen->MouseY - hidden->win->TopEdge - hidden->win->BorderTop;
	}


	/* Check if the mouse is inside the window */
	if (MouseX >= 0 && MouseX < SDL_VideoSurface->w && MouseY >= 0 && MouseY < SDL_VideoSurface->h)
		SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
	else
		SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);

	SDL_PrivateMouseMotion(0, 0, MouseX, MouseY);

	SDL_Unlock_EventThread();
}

BOOL CreateAppIcon(_THIS)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;

	if (!hidden->currentIcon)
	{
		hidden->currentIcon = SDL_IIcon->GetDefDiskObject(WBTOOL);
	}

	hidden->currentAppIcon = SDL_IWorkbench->AddAppIcon(0, (uint32)_this, hidden->currentIconCaption,
								hidden->appPort, 0, hidden->currentIcon,
								TAG_DONE);

	if (hidden->currentAppIcon)
		return TRUE;
	else
		return FALSE;
}

void DeleteAppIcon(_THIS)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;

	if (hidden->currentAppIcon)
		SDL_IWorkbench->RemoveAppIcon(hidden->currentAppIcon);

	hidden->currentAppIcon = 0;
}

int os4video_IconifyWindow(_THIS)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;

	/* If we have an app icon, we're already iconified */
	if (hidden->currentAppIcon)
		return 1;

	if (!CreateAppIcon(_this))
		return 0;

	if (SDL_VideoSurface->flags & SDL_FULLSCREEN)
	{
		/* Just bring the Workbench to front */
		SDL_IIntuition->WBenchToFront();
	}
	else
	{
		/* Hide the window */
		SDL_IIntuition->HideWindow(hidden->win);
		hidden->pointerState = pointer_outside_window;
		SDL_PrivateAppActive(0, SDL_APPACTIVE | SDL_APPINPUTFOCUS);
	}

	return 1;
}

/* NOTE: not called through video device */
void os4video_UniconifyWindow(_THIS)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;

	DeleteAppIcon(_this);

	if (SDL_VideoSurface->flags & SDL_FULLSCREEN)
	{
		/* Just bring the Workbench to front */
		SDL_IIntuition->ScreenToFront(hidden->scr);
	}
	else
	{
		/* Show the window */
		SDL_PrivateAppActive(1, SDL_APPACTIVE);
		SDL_IIntuition->ShowWindow(hidden->win, WINDOW_FRONTMOST);
	}
}


void os4video_SetIcon(_THIS, SDL_Surface *icon, Uint8 *mask)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;

	if (hidden->currentIcon)
	{
		/* If no app icon exists, we can just delete the old icon */
		if (hidden->currentAppIcon)
		{
			SDL_IIcon->FreeDiskObject(hidden->currentIcon);
			hidden->currentIcon = 0;
		}
		else
		{
			struct AppMessage *amsg;
			while (NULL != (amsg = (struct AppMessage *)IExec->GetMsg(hidden->appPort)))
				IExec->ReplyMsg((struct Message *)amsg);

			DeleteAppIcon(_this);
			SDL_IIcon->FreeDiskObject(hidden->currentIcon);
			CreateAppIcon(_this);
		}
	}


	hidden->currentIcon = SDL_IIcon->GetDefDiskObject(WBTOOL);
	//hidden->currentIcon = SDL_IIcon->NewDiskObject(WBTOOL);
	hidden->currentIcon->do_Type = 0;

}
