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

/* Functions and variables exported from SDL_audio.c for SDL_sysaudio.c */

/* Functions to get a list of "close" audio formats */
extern Uint16 SDL_FirstAudioFormat(Uint16 format);
extern Uint16 SDL_NextAudioFormat(void);

/* Function to calculate the size and silence for a SDL_AudioSpec */
extern void SDL_CalculateAudioSpec(SDL_AudioSpec *spec);

/* The actual mixing thread function */
extern int SDL_RunAudio(void *audiop);
