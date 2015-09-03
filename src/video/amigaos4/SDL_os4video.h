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

#ifndef _SDL_os4video_h
#define _SDL_os4video_h

#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/Picasso96API.h>
#include <proto/icon.h>

#include "../SDL_sysvideo.h"

#include "SDL_os4keyboard.h"
#include "SDL_os4modes.h"

/* Private display data */

typedef struct
{
	struct Screen *			publicScreen;

	struct Library			*gfxbase;
	struct Library			*layersbase;
	struct Library			*p96base;
	struct Library			*intuitionbase;
	struct Library			*iconbase;
	struct Library			*workbenchbase;
	struct Library			*keymapbase;

	struct GraphicsIFace	*iGraphics;
	struct LayersIFace		*iLayers;
	struct P96IFace			*iP96;
	struct IntuitionIFace	*iIntuition;
	struct IconIFace		*iIcon;
	struct WorkbenchIFace	*iWorkbench;
	struct KeymapIFace		*iKeymap;
} SDL_VideoData;

#define GfxBase ((SDL_VideoData *) _this->driverdata)->gfxbase
#define LayersBase ((SDL_VideoData *) _this->driverdata)->layersbase
#define P96Base ((SDL_VideoData *) _this->driverdata)->p96base
#define IntuitionBase ((SDL_VideoData *) _this->driverdata)->intuitionbase
#define IconBase ((SDL_VideoData *) _this->driverdata)->iconbase
#define WorkbenchBase ((SDL_VideoData *) _this->driverdata)->workbenchbase
#define KeymapBase ((SDL_VideoData *) _this->driverdata)->keymapbase

#define IGraphics ((SDL_VideoData *) _this->driverdata)->iGraphics
#define ILayers ((SDL_VideoData *) _this->driverdata)->iLayers
#define IP96 ((SDL_VideoData *) _this->driverdata)->iP96
#define IIntuition ((SDL_VideoData *) _this->driverdata)->iIntuition
#define IIcon ((SDL_VideoData *) _this->driverdata)->iIcon
#define IWorkbench ((SDL_VideoData *) _this->driverdata)->iWorkbench
#define IKeymap ((SDL_VideoData *) _this->driverdata)->iKeymap

#endif /* _SDL_os4video_h */

/* vi: set ts=4 sw=4 expandtab: */
