/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

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

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#include <stdlib.h>
#include <string.h>


#include "SDL_error.h"
#include "SDL_QWin.h"

extern "C" {

#include "SDL_sysmouse_c.h"

/* The implementation dependent data for the window manager cursor */
struct WMcursor {
	char *bits;
};
WMcursor *QT_CreateWMCursor(_THIS,
		Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y)
{
  static WMcursor dummy;
  dummy.bits = 0;
  return &dummy;
}

int QT_ShowWMCursor(_THIS, WMcursor *cursor)
{
  return 1;
}

void QT_FreeWMCursor(_THIS, WMcursor *cursor)
{
}

void QT_WarpWMCursor(_THIS, Uint16 x, Uint16 y)
{
  SDL_Win->setMousePos(QPoint(x, y));
}

}; /* Extern C */
