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

/* iPod SDL framebuffer driver
 * Joshua Oreman
 * Main header file
 */

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#ifndef _SDL_ipodvideo_h
#define _SDL_ipodvideo_h

struct SDL_PrivateVideoData {
    char *buffer;
    int w, h;
};


#endif
