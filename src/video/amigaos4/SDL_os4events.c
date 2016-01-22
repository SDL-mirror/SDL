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

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

static void
OS4_EventHandler(_THIS)
{
	SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
	struct IntuiMessage *imsg;

	while ((imsg = (struct IntuiMessage *)IExec->GetMsg(data->userport))) {
		
		dprintf("Message class %d, code %d\n", imsg->Class, imsg->Code);

		IExec->ReplyMsg((struct Message *) imsg);
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
