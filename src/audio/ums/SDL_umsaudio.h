/*
    AIX support for the SDL - Simple DirectMedia Layer
    Copyright (C) 2000  Carsten Griwodz

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

    Carsten Griwodz
    griff@kom.tu-darmstadt.de

    based on linux/SDL_dspaudio.h by Sam Lantinga
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#ifndef _SDL_UMSaudio_h
#define _SDL_UMSaudio_h

#include "SDL_sysaudio.h"

#include <UMS/UMSAudioDevice.h>

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData
{
    /* Pointer to the (open) UMS audio device */
    Environment*   ev;
    UMSAudioDevice umsdev;

    /* Raw mixing buffer */
    UMSAudioTypes_Buffer playbuf;
    UMSAudioTypes_Buffer fillbuf;

    long bytesPerSample;
};

#endif /* _SDL_UMSaudio_h */

