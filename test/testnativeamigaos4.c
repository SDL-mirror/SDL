/*
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#include "testnative.h"

#ifdef TEST_NATIVE_AMIGAOS4

#include "../src/video/SDL_sysvideo.h"
#include "../src/video/amigaos4/SDL_os4video.h"

static void *CreateWindowAmigaOS4(int w, int h);
static void DestroyWindowAmigaOS4(void *window);

NativeWindowFactory AmigaOS4WindowFactory = {
	"os4",
	CreateWindowAmigaOS4,
	DestroyWindowAmigaOS4
};

static void *
CreateWindowAmigaOS4(int w, int h)
{
	struct Window * window = NULL;

	SDL_VideoDevice * vd = SDL_GetVideoDevice();

	if (vd) {

		SDL_VideoData * data = vd->driverdata;

		if (data && data->iIntuition) {

			window = data->iIntuition->OpenWindowTags(
				NULL,
				WA_Title, "Native window",
				WA_InnerWidth, w,
				WA_InnerHeight, h,
				WA_Flags, WFLG_CLOSEGADGET,
				WA_IDCMP, IDCMP_CLOSEWINDOW,
				WA_UserPort, data->userport,
				TAG_DONE);
		}
	}

    return (void *) window;
}

static void
DestroyWindowAmigaOS4(void *window)
{
	_THIS = SDL_GetVideoDevice();
	
	IIntuition->CloseWindow(window);
}

#endif
