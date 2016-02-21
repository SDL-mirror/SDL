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

#define BUTTON_BUF_SIZE 1024

static char *
OS4_MakeButtonString(const SDL_MessageBoxData * messageboxdata)
{
	char *buttonBuffer = SDL_malloc(BUTTON_BUF_SIZE);

	if (buttonBuffer) {
		int b;

		SDL_memset(buttonBuffer, 0, BUTTON_BUF_SIZE);

		/* Generate "Button 1|Button2... "*/
		for (b = 0; b < messageboxdata->numbuttons; b++) {
			strncat(buttonBuffer, messageboxdata->buttons[b].text, BUTTON_BUF_SIZE - sizeof(buttonBuffer) - 1);

			if (b != (messageboxdata->numbuttons - 1)) {
				strncat(buttonBuffer, "|", BUTTON_BUF_SIZE - sizeof(buttonBuffer) - 1);
			}
		}

		dprintf("String '%s'\n", buttonBuffer);
	}

	return buttonBuffer;
}

int
OS4_ShowMessageBox(_THIS, const SDL_MessageBoxData * messageboxdata, int * buttonid)
{
	char *buttonString;

	if ((buttonString = OS4_MakeButtonString(messageboxdata))) {
		
		struct EasyStruct es = {
			sizeof(struct EasyStruct),
			0, // Flags
			messageboxdata->title,
			messageboxdata->message,
			buttonString,
			NULL, // Screen
			NULL  // TagList
		};

		const int LAST_BUTTON = messageboxdata->numbuttons;

		/* Amiga button order is 1, 2, ..., N, 0! */

		int amigaButton = IIntuition->EasyRequest(NULL, &es, 0, NULL);
		
		dprintf("Button %d chosen\n", amigaButton);

		if (amigaButton >= 0 && amigaButton < LAST_BUTTON) {
			if (amigaButton == 0) {
				/* Last */
				*buttonid = messageboxdata->buttons[LAST_BUTTON - 1].buttonid;
			} else {
				*buttonid = messageboxdata->buttons[amigaButton - 1].buttonid;
			}

			dprintf("Mapped button %d\n", *buttonid);
		}

		SDL_free(buttonString);
		return 0;
	}

	return -1;
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */

/* vi: set ts=4 sw=4 expandtab: */
