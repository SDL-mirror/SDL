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
#if SDL_VIDEO_DRIVER_AMIGAOS4

#include "../../events/SDL_events_c.h"
#include "../../events/SDL_sysevents.h"
#include "SDL_os4video.h"

#include <libraries/Picasso96.h>
#include <intuition/intuition.h>
#include <devices/inputevent.h>
#include <workbench/workbench.h>

#include <proto/exec.h>
#include <proto/Picasso96API.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/intuition.h>

//#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

extern struct LayersIFace    *SDL_ILayers;
extern struct IntuitionIFace *SDL_IIntuition;
extern struct KeymapIFace    *SDL_IKeymap;

#define POINTER_GRAB_TIMEOUT		20	/* Number of ticks before pointer grab needs to be reactivated */

SDL_Scancode rawkey_table[128];

#define map(x,y) rawkey_table[x] = SDL_GetScancodeFromKey(y)

extern void SetMouseColors(SDL_VideoDevice *_this);
extern void ResetMouseColors(SDL_VideoDevice *_this);
extern void os4video_UniconifyWindow(_THIS);
extern void os4video_ResetCursor(struct SDL_PrivateVideoData *hidden);

struct MyIntuiMessage
{
	uint32 Class;
	uint16 Code;
	uint16 Qualifier;
	struct Gadget *Gadget;

	int16  MouseDX;		/* Relative mouse movement */
	int16  MouseDY;

	int16  PointerX;	/* Absolute pointer position, relative to */
	int16  PointerY;	/* top-left corner of inner window */

	int16  Width;		/* Inner window dimensions */
	int16  Height;

	int8   wantDelta;	/* Do we want to report delta movements or absolute position */
};

void
os4video_InitOSKeymap(_THIS)
{
	long i;
	for (i=0; i<128; i++)
		rawkey_table[i] = SDLK_UNKNOWN;

	map(0x45, SDLK_ESCAPE);
	map(0x50, SDLK_F1);
	map(0x51, SDLK_F2);
	map(0x52, SDLK_F3);
	map(0x53, SDLK_F4);
	map(0x54, SDLK_F5);
	map(0x55, SDLK_F6);
	map(0x56, SDLK_F7);
	map(0x57, SDLK_F8);
	map(0x58, SDLK_F9);
	map(0x59, SDLK_F10);
	map(0x4B, SDLK_F11);
	map(0x6F, SDLK_F12);
	map(0x01, SDLK_1);
	map(0x02, SDLK_2);
	map(0x03, SDLK_3);
	map(0x04, SDLK_4);
	map(0x05, SDLK_5);
	map(0x06, SDLK_6);
	map(0x07, SDLK_7);
	map(0x08, SDLK_8);
	map(0x09, SDLK_9);
	map(0x0A, SDLK_0);
	map(0x41, SDLK_BACKSPACE);
	map(0x46, SDLK_DELETE);
	map(0x5F, SDLK_HELP);
	map(0x5A, 0);
	map(0x5B, 0);
	map(0x5C, SDLK_KP_DIVIDE);
	map(0x5D, SDLK_KP_MULTIPLY);
	map(0x42, SDLK_TAB);
	map(0x10, SDLK_q);
	map(0x11, SDLK_w);
	map(0x12, SDLK_e);
	map(0x13, SDLK_r);
	map(0x14, SDLK_t);
	map(0x15, SDLK_y);
	map(0x16, SDLK_u);
	map(0x17, SDLK_i);
	map(0x18, SDLK_o);
	map(0x19, SDLK_p);
	map(0x44, SDLK_RETURN);
	map(0x3D, SDLK_KP_7);
	map(0x3E, SDLK_KP_8);
	map(0x3F, SDLK_KP_9);
	map(0x4A, SDLK_KP_MINUS);
	map(0x63, SDLK_LCTRL);
	map(0x62, SDLK_CAPSLOCK);
	map(0x20, SDLK_a);
	map(0x21, SDLK_s);
	map(0x22, SDLK_d);
	map(0x23, SDLK_f);
	map(0x24, SDLK_g);
	map(0x25, SDLK_h);
	map(0x26, SDLK_j);
	map(0x27, SDLK_k);
	map(0x28, SDLK_l);
	map(0x4C, SDLK_UP);
	map(0x2D, SDLK_KP_4);
	map(0x2E, SDLK_KP_5);
	map(0x2F, SDLK_KP_6);
	map(0x5E, SDLK_KP_PLUS);
	map(0x60, SDLK_LSHIFT);
	map(0x31, SDLK_z);
	map(0x32, SDLK_x);
	map(0x33, SDLK_c);
	map(0x34, SDLK_v);
	map(0x35, SDLK_b);
	map(0x36, SDLK_n);
	map(0x37, SDLK_m);
	map(0x61, SDLK_RSHIFT);
	map(0x4F, SDLK_LEFT);
	map(0x4D, SDLK_DOWN);
	map(0x4E, SDLK_RIGHT);
	map(0x1D, SDLK_KP_1);
	map(0x1E, SDLK_KP_2);
	map(0x1F, SDLK_KP_3);
	map(0x43, SDLK_KP_ENTER);
	map(0x64, SDLK_LALT);
	map(0x66, SDLK_LGUI);
	map(0x40, SDLK_SPACE);
	map(0x67, SDLK_RGUI);
	map(0x65, SDLK_RALT);
	map(0x0f, SDLK_KP_0);
	map(0x3C, SDLK_KP_PERIOD);
	// Only on PS/2 or USB PC keyboards
	map(0x6D, SDLK_PRINTSCREEN);
	map(0x6E, SDLK_PAUSE);
	map(0x47, SDLK_INSERT);
	map(0x70, SDLK_HOME);
	map(0x48, SDLK_PAGEUP);
	map(0x71, SDLK_END);
	map(0x49, SDLK_PAGEDOWN);
	map(0x6B, SDLK_MODE);
}

#undef map


static uint32
os4video_TranslateUnicode(uint16 Code, uint32 Qualifier)
{
	struct InputEvent ie;
	uint16 res;
	char buffer[10];

	ie.ie_Class = IECLASS_RAWKEY;
    ie.ie_SubClass = 0;
    ie.ie_Code  = Code & ~(IECODE_UP_PREFIX);
    ie.ie_Qualifier = Qualifier;
    ie.ie_EventAddress = NULL;

	res = SDL_IKeymap->MapRawKey(&ie, buffer, 10, 0);
	if (res != 1) return 0;
	else return buffer[0];
}

static SDL_Keymod
os4video_KMod(uint16 Qualifier)
{
	SDL_Keymod x = KMOD_NONE;
	if (Qualifier & IEQUALIFIER_LSHIFT) 	x |= KMOD_LSHIFT;
	if (Qualifier & IEQUALIFIER_RSHIFT) 	x |= KMOD_RSHIFT;
	if (Qualifier & IEQUALIFIER_CAPSLOCK)	x |= KMOD_CAPS;
	if (Qualifier & IEQUALIFIER_CONTROL)	x |= KMOD_LCTRL;
	if (Qualifier & IEQUALIFIER_LALT)		x |= KMOD_LALT;
	if (Qualifier & IEQUALIFIER_RALT)		x |= KMOD_RALT;
	if (Qualifier & IEQUALIFIER_LCOMMAND)	x |= KMOD_LGUI;
	if (Qualifier & IEQUALIFIER_RCOMMAND)	x |= KMOD_RGUI;

	return x;
}

static void
os4video_HandleKeyboard(struct MyIntuiMessage *imsg)
{
	if ((imsg->Qualifier & IEQUALIFIER_REPEAT) == 0)
	{
		SDL_Scancode s;
		extern int SDL_TranslateUNICODE;

		s.scancode = (uint8)imsg->Code;
		s.sym      = rawkey_table[imsg->Code & 0x7F];
		if (s.sym == SDLK_UNKNOWN)
			s.sym = os4video_TranslateUnicode(imsg->Code, imsg->Qualifier);

		s.unicode  = SDL_TranslateUNICODE ? os4video_TranslateUnicode(imsg->Code, imsg->Qualifier)
										  : 0;

		if (imsg->Code <= 127)
			SDL_SendKeyboardKey(SDL_PRESSED, &s);
		else
			SDL_SendKeyboardKey(SDL_RELEASED, &s);
	}
}

static void
os4video_HandleMouseMotion(SDL_Window windows, struct MyIntuiMessage *imsg)
{
	uint8 buttonstate = 0;
    SDL_Mouse *mouse = SDL_GetMouse();

	if (imsg->Qualifier & IEQUALIFIER_LEFTBUTTON)
		buttonstate |= SDL_BUTTON(SDL_BUTTON_LEFT);
	if (imsg->Qualifier & IEQUALIFIER_RBUTTON)
		buttonstate |= SDL_BUTTON(SDL_BUTTON_RIGHT);
	if (imsg->Qualifier & IEQUALIFIER_MIDBUTTON)
		buttonstate |= SDL_BUTTON(SDL_BUTTON_MIDDLE);

	if (imsg->wantDelta)
	{
		if (imsg->MouseDX != 0 || imsg->MouseDY != 0)
		{
			dprintf("dX:%d dY:%d buttonstate:%d\n", imsg->MouseDX, imsg->MouseDY, buttonstate);

			SDL_SendMouseMotion(windows, mouse->mouseID, 1, imsg->MouseDX, imsg->MouseDY);
		}
	}
	else
	{
		dprintf("X:%d Y:%d buttonstate:%d\n", imsg->PointerX, imsg->PointerY, buttonstate);

		SDL_SendMouseMotion(windows, mouse->mouseID, 0, imsg->PointerX, imsg->PointerY);
	}
}

static void
os4video_HandleMouseButton(struct MyIntuiMessage *imsg)
{
	uint8 button = 0;
	uint8 state = SDL_PRESSED;

	if ((imsg->Code & ~IECODE_UP_PREFIX) == IECODE_LBUTTON)
		button = SDL_BUTTON_LEFT;
	else if ((imsg->Code & ~IECODE_UP_PREFIX) == IECODE_RBUTTON)
		button = SDL_BUTTON_RIGHT;
	else if ((imsg->Code & ~IECODE_UP_PREFIX) == IECODE_MBUTTON)
		button = SDL_BUTTON_MIDDLE;
	/*
	else if ((imsg->Code & ~IECODE_UP_PREFIX) == IECODE_WHEELUP)
		button = SDL_BUTTON_WHEELUP;
	else if ((imsg->Code & ~IECODE_UP_PREFIX) == IECODE_WHEELDOWN)
		button = SDL_BUTTON_WHEELDOWN;
	*/

	if (imsg->Code & IECODE_UP_PREFIX)
		state = SDL_RELEASED;

	dprintf("X:%d Y:%d button:%d state:%d\n", imsg->PointerX, imsg->PointerY, button, state);

	SDL_PrivateMouseButton(state, button, 0, 0);
}

static void
os4video_HandleMouseWheel(struct MyIntuiMessage *imsg)
{
	struct IntuiWheelData *data = (struct IntuiWheelData *)imsg->Gadget;

	if (data->WheelY < 0) {
		SDL_PrivateMouseButton(SDL_PRESSED,  SDL_BUTTON_WHEELUP, 0, 0);
		SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_WHEELUP, 0 ,0);
	} else if (data->WheelY > 0) {
		SDL_PrivateMouseButton(SDL_PRESSED,  SDL_BUTTON_WHEELDOWN, 0, 0);
		SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_WHEELDOWN, 0 ,0);
	}
}

static void
os4video_HandleNewSize(struct MyIntuiMessage *imsg)
{
	SDL_PrivateResize(imsg->Width, imsg->Height);
}

static pointer_state
os4video_CheckPointerInWindow (struct Window *win, struct MyIntuiMessage *imsg)
{
	pointer_state result = pointer_outside_window;

	/* Is pointer within bounds of inner window? */
	if (imsg->PointerX >= 0  && imsg->PointerX < imsg->Width
		&& imsg->PointerY >= 0 && imsg->PointerY < imsg->Height)
	{
		/* Yes. Now check whether the window is obscured by another
		 * window at the pointer position */
		struct Screen *scr = win->WScreen;
		struct Layer *layer;

		/* Find which layer the pointer is in */
		SDL_ILayers->LockLayerInfo(&scr->LayerInfo);
		layer = SDL_ILayers->WhichLayer(&scr->LayerInfo, scr->MouseX, scr->MouseY);
		SDL_ILayers->UnlockLayerInfo(&scr->LayerInfo);

		/* Is this layer our window's layer? */
		if (layer == win->WLayer)
		{
			/* Yes */
			dprintf("Pointer inside window\n");
			result = pointer_inside_window;
		}
		else
		{
			/* No */
			dprintf("Pointer inside window but window obscured\n");
		}
	}
	else
	{
		/* No */
		dprintf("Pointer outside of window\n");
	}

	return result;
}

static void
os4video_HandleEnterLeaveWindow (SDL_VideoDevice *_this, struct MyIntuiMessage *imsg)
{
	struct SDL_PrivateVideoData *hidden = (struct SDL_PrivateVideoData *)_this->driverdata;
	SDL_Window *windows = hidden->windows;
	int new_state;

	if (hidden->scr)
		/* If in full-screen mode, then pointer can't leave the window */
		new_state = pointer_inside_window;
	else
		/* Otherwise, we're in windowed mode. Check pointer position */
		new_state = os4video_CheckPointerInWindow (hidden->win, imsg);

	if (new_state != hidden->pointerState)
	{
		hidden->pointerState = new_state;

		if (new_state == pointer_inside_window)
		{
			dprintf ("Pointer entered window\n");

			/* Reset SDL pointer image */
			SetMouseColors(_this);
			os4video_ResetCursor(hidden);

			/* Tell SDL where the pointer re-entered the window.
			 * This a rather hacky way of doing this. */
			imsg->wantDelta = FALSE;
			os4video_HandleMouseMotion(windows, imsg);

			/* Tell SDL we have gained mouse focus */
     		SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
		}
		else
		{
			dprintf ("Pointer left window\n");

			/* Install WB default pointer image */
			ResetMouseColors(_this);
			if (hidden->mouse)
				SDL_IIntuition->SetWindowPointer(hidden->win, WA_Pointer, NULL, TAG_DONE);

			/* Tell SDL we have lost mouse focus */
			SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);

		}
	}
}

void
os4video_activatePointerGrab(struct Window *w, BOOL activate)
{
	struct IBox grabBox = {
		w->BorderLeft,
		w->BorderTop,
		w->Width  - w->BorderLeft - w->BorderRight,
		w->Height - w->BorderTop  - w->BorderBottom
	};

	SDL_IIntuition->SetWindowAttrs(w, WA_MouseLimits, &grabBox, sizeof grabBox);
	SDL_IIntuition->SetWindowAttrs(w, WA_GrabFocus, activate ? POINTER_GRAB_TIMEOUT : 0, sizeof (ULONG));
}

static void
os4video_EventHandler(SDL_VideoDevice *_this)
{
	struct IntuiMessage *imsg;
	struct MyIntuiMessage msg;
	struct SDL_PrivateVideoData *hidden = (struct SDL_PrivateVideoData *)_this->driverdata;
	SDL_Window *windows = hidden->windows;
	struct AppMessage *amsg;
	BOOL bUniconify = FALSE;

	while (NULL != (amsg = (struct AppMessage *)IExec->GetMsg(hidden->appPort)))
	{
		if (amsg->am_NumArgs == 0)
			bUniconify = TRUE;

		IExec->ReplyMsg((struct Message *)amsg);
	}

	if (bUniconify)
	{
		os4video_UniconifyWindow(_this);
	}

        BOOL bailOut = FALSE;

	while (!bailOut && (NULL != (imsg = (struct IntuiMessage *)IExec->GetMsg(hidden->userPort))))
	{
		/* Copy relevant fields. This makes it safer if the window goes away during
		   this loop (re-open due to keystroke) */
		msg.Class 			= imsg->Class;
		msg.Code 			= imsg->Code;
		msg.Qualifier 		= imsg->Qualifier;

		msg.Gadget			= (struct Gadget *) imsg->IAddress;

		/* We've got IDCMP_DELTAMOVE set on our window, so the intuimsg will report
		 * relative mouse movements in MouseX/Y not absolute pointer position. */
		msg.MouseDX 		= imsg->MouseX;
		msg.MouseDY 		= imsg->MouseY;

		/* The window's MouseX/Y fields, however, always contain the absolute pointer
		 * position (relative to the window's upper-left corner). */
		msg.PointerX		= imsg->IDCMPWindow->MouseX - imsg->IDCMPWindow->BorderLeft;
		msg.PointerY		= imsg->IDCMPWindow->MouseY - imsg->IDCMPWindow->BorderTop;

		msg.Width 			= imsg->IDCMPWindow->Width  - imsg->IDCMPWindow->BorderLeft - imsg->IDCMPWindow->BorderRight;
		msg.Height 			= imsg->IDCMPWindow->Height - imsg->IDCMPWindow->BorderTop  - imsg->IDCMPWindow->BorderBottom;

		/* Report delta movements when pointer when in
		 * full-screen mode or when the input is grabbed */
		msg.wantDelta		= hidden->isMouseRelative;

		IExec->ReplyMsg((struct Message *)imsg);

		switch(msg.Class)
		{
		case IDCMP_MOUSEMOVE:
			if (hidden->pointerState == pointer_inside_window)
				os4video_HandleMouseMotion(windows, &msg);
			os4video_HandleEnterLeaveWindow(_this, &msg);
			break;

		case IDCMP_RAWKEY:
			os4video_HandleKeyboard(&msg);
			break;

		case IDCMP_MOUSEBUTTONS:
			os4video_HandleMouseButton(&msg);
			break;

		case IDCMP_EXTENDEDMOUSE:
			os4video_HandleMouseWheel(&msg);
			break;

		case IDCMP_NEWSIZE:
			os4video_HandleNewSize(&msg);
			break;

		case IDCMP_ACTIVEWINDOW:
			dprintf("Window active\n");
			SDL_SetModState(os4video_KMod(msg.Qualifier));
			SDL_PrivateAppActive(1, SDL_APPINPUTFOCUS);
			hidden->windowActive = TRUE;
			os4video_HandleEnterLeaveWindow(_this, &msg);
			break;

		case IDCMP_INACTIVEWINDOW:
			dprintf("Window inactive\n");
			hidden->windowActive = FALSE;
			hidden->pointerState = pointer_dont_know;
			SDL_PrivateAppActive(0, SDL_APPINPUTFOCUS);
			ResetMouseColors(_this);
			break;

		case IDCMP_CLOSEWINDOW:
			SDL_PrivateQuit();
			break;

		case IDCMP_INTUITICKS:
			if (!hidden->scr && hidden->windowActive && (SDL_GetRelativeMouseMode() == SDL_TRUE))
			{
				hidden->pointerGrabTicks--;
				if (hidden->pointerGrabTicks > POINTER_GRAB_TIMEOUT)
					os4video_activatePointerGrab (hidden->win, TRUE);
			}
			break;
		}
	}
}

void
os4video_PumpEvents(SDL_VideoDevice *_this)
{
	os4video_EventHandler(_this);
}

SDL_GrabMode
os4video_GrabInput(SDL_VideoDevice *_this, SDL_bool mode)
{
	struct SDL_PrivateVideoData *hidden = (struct SDL_PrivateVideoData *)_this->driverdata;

	if (hidden->win) {
		SDL_Lock_EventThread();

		dprintf("Setting GrabMode: %d\n", mode);

		if (!hidden->scr && hidden->windowActive && (mode == SDL_TRUE))
			os4video_activatePointerGrab(hidden->win, TRUE);
		else if (mode == SDL_FALSE)
			os4video_activatePointerGrab(hidden->win, FALSE);

		SDL_Unlock_EventThread();
	}

	return mode;
}

/*
 * Should we report relative or absolute mouse movements?
 * This function is called by SDL whenever there's a change of video mode
 * or when the grab-input status changes.
 */
void os4video_CheckMouseMode(SDL_VideoDevice *_this)
{
	struct SDL_PrivateVideoData *hidden = (struct SDL_PrivateVideoData *)_this->driverdata;
    SDL_Mouse *mouse = SDL_GetMouse();

	SDL_Lock_EventThread();

	/* Report relative mouse movements when the mouse is pointer is hidden
	 * and the pointer is grabbed
	 */
	hidden->isMouseRelative = (!(mouse->cursor_shown) && (SDL_GetRelativeMouseMode() != SDL_FALSE)); //test with mouse->relative_mode //afxgroup

	dprintf("isMouseRelative=%d\n", hidden->isMouseRelative);

	SDL_Unlock_EventThread();
}

VideoBootStrap AMIGAOS4_bootstrap = {
	"amigaos4", "AmigaOS4 graphics",
	OS4_Available, OS4_CreateDevice
};

#endif