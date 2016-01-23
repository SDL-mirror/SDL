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

#include "../../events/SDL_keyboard_c.h"

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

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

/*
static uint32
OS4_TranslateUnicode(uint16 Code, uint32 Qualifier)
{
	struct InputEvent ie;
	uint16 res;
	char buffer[10];

	ie.ie_Class = IECLASS_RAWKEY;
    ie.ie_SubClass = 0;
    ie.ie_Code  = Code & ~(IECODE_UP_PREFIX);
    ie.ie_Qualifier = Qualifier;
    ie.ie_EventAddress = NULL;

	res = IKeymap->MapRawKey(&ie, buffer, sizeof(buffer), 0);
	if (res != 1) return 0;
	else return buffer[0];
}
*/
static SDL_Keymod
OS4_KMod(uint16 Qualifier)
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
OS4_HandleKeyboard(struct MyIntuiMessage *imsg)
{
	if ((imsg->Qualifier & IEQUALIFIER_REPEAT) == 0) {

		SDL_Scancode s = imsg->Code;

		if (s <= 127)
			SDL_SendKeyboardKey(SDL_PRESSED, s);
		else
			SDL_SendKeyboardKey(SDL_RELEASED, s);
	}
}


static void
OS4_CopyRelevantFields(struct IntuiMessage * src, struct MyIntuiMessage * dst)
{
	/* Copy relevant fields. This makes it safer if the window goes away during
	  this loop (re-open due to keystroke) */
	dst->Class 			 = src->Class;
	dst->Code 			 = src->Code;
	dst->Qualifier 		 = src->Qualifier;

	dst->Gadget			 = (struct Gadget *) src->IAddress;

	/* We've got IDCMP_DELTAMOVE set on our window, so the intuimsg will report
	* relative mouse movements in MouseX/Y not absolute pointer position. */
	dst->MouseDX 		 = src->MouseX;
	dst->MouseDY 		 = src->MouseY;

	/* The window's MouseX/Y fields, however, always contain the absolute pointer
	* position (relative to the window's upper-left corner). */
	dst->PointerX		 = src->IDCMPWindow->MouseX - src->IDCMPWindow->BorderLeft;
	dst->PointerY		 = src->IDCMPWindow->MouseY - src->IDCMPWindow->BorderTop;

	dst->Width 			 = src->IDCMPWindow->Width  - src->IDCMPWindow->BorderLeft - src->IDCMPWindow->BorderRight;
	dst->Height 		 = src->IDCMPWindow->Height - src->IDCMPWindow->BorderTop  - src->IDCMPWindow->BorderBottom;

	/* Report delta movements when pointer when in
	* full-screen mode or when the input is grabbed */
	// TODO: dst->wantDelta		  = hidden->isMouseRelative;
}


static void
OS4_EventHandler(_THIS)
{
	SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
	
	struct IntuiMessage *imsg;
	struct MyIntuiMessage msg;

	while ((imsg = (struct IntuiMessage *)IExec->GetMsg(data->userport))) {

		OS4_CopyRelevantFields(imsg, &msg);

		IExec->ReplyMsg((struct Message *) imsg);

		dprintf("Message class %d, code %d\n", msg.Class, msg.Code);

		switch (msg.Class) {
			case IDCMP_MOUSEMOVE:
				break;

			case IDCMP_RAWKEY:
				OS4_HandleKeyboard(&msg);
				break;

			case IDCMP_MOUSEBUTTONS:
				break;

			case IDCMP_EXTENDEDMOUSE:
				break;

			case IDCMP_NEWSIZE:
				break;

			case IDCMP_ACTIVEWINDOW:
				break;

			case IDCMP_INACTIVEWINDOW:
				break;

			case IDCMP_CLOSEWINDOW:
				break;

			case IDCMP_INTUITICKS:
				break;

			default:
				dprintf("Unknown event received\n");
				break;
		}

	}
}

void
OS4_PumpEvents(_THIS)
{
	//SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
	OS4_EventHandler(_this);
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
