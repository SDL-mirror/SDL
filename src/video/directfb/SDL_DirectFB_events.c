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

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* Handle the event stream, converting DirectFB input events into SDL events */

#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <directfb.h>

#include "SDL.h"
#include "SDL_sysevents.h"
#include "SDL_sysvideo.h"
#include "SDL_events_c.h"
#include "SDL_DirectFB_video.h"
#include "SDL_DirectFB_events.h"

/* The translation tables from a DirectFB keycode to a SDL keysym */
static SDLKey keymap[256];
static SDL_keysym *DirectFB_TranslateKey (DFBInputEvent *ev, SDL_keysym *keysym);
static int DirectFB_TranslateButton (DFBInputEvent *ev);

static int posted = 0;


void DirectFB_PumpEvents (_THIS)
{
  DFBInputEvent evt;

  while (HIDDEN->eventbuffer->GetEvent (HIDDEN->eventbuffer,
                                        DFB_EVENT (&evt)) == DFB_OK)
    {
      SDL_keysym keysym;

      switch (evt.type)
        {
        case DIET_BUTTONPRESS:
          posted += SDL_PrivateMouseButton(SDL_PRESSED,
                                           DirectFB_TranslateButton (&evt), 0, 0);
          break;
        case DIET_BUTTONRELEASE:
          posted += SDL_PrivateMouseButton(SDL_RELEASED,
                                           DirectFB_TranslateButton (&evt), 0, 0);
          break;
        case DIET_KEYPRESS:
          posted += SDL_PrivateKeyboard(SDL_PRESSED, DirectFB_TranslateKey(&evt, &keysym));
          break;
        case DIET_KEYRELEASE:
          posted += SDL_PrivateKeyboard(SDL_RELEASED, DirectFB_TranslateKey(&evt, &keysym));
          break;
        case DIET_AXISMOTION:
          if (evt.flags & DIEF_AXISREL)
            {
              if (evt.axis == DIAI_X)
                posted += SDL_PrivateMouseMotion(0, 1, evt.axisrel, 0);
              else if (evt.axis == DIAI_Y)
                posted += SDL_PrivateMouseMotion(0, 1, 0, evt.axisrel);
            }
          break;
        default:
          ;
        }
    }
}

void DirectFB_InitOSKeymap (_THIS)
{
  int i;
	
  /* Initialize the DirectFB key translation table */
  for (i=0; i<SDL_TABLESIZE(keymap); ++i)
    keymap[i] = SDLK_UNKNOWN;

  keymap[DIKC_A] = SDLK_a;
  keymap[DIKC_B] = SDLK_b;
  keymap[DIKC_C] = SDLK_c;
  keymap[DIKC_D] = SDLK_d;
  keymap[DIKC_E] = SDLK_e;
  keymap[DIKC_F] = SDLK_f;
  keymap[DIKC_G] = SDLK_g;
  keymap[DIKC_H] = SDLK_h;
  keymap[DIKC_I] = SDLK_i;
  keymap[DIKC_J] = SDLK_j;
  keymap[DIKC_K] = SDLK_k;
  keymap[DIKC_L] = SDLK_l;
  keymap[DIKC_M] = SDLK_m;
  keymap[DIKC_N] = SDLK_n;
  keymap[DIKC_O] = SDLK_o;
  keymap[DIKC_P] = SDLK_p;
  keymap[DIKC_Q] = SDLK_q;
  keymap[DIKC_R] = SDLK_r;
  keymap[DIKC_S] = SDLK_s;
  keymap[DIKC_T] = SDLK_t;
  keymap[DIKC_U] = SDLK_u;
  keymap[DIKC_V] = SDLK_v;
  keymap[DIKC_W] = SDLK_w;
  keymap[DIKC_X] = SDLK_x;
  keymap[DIKC_Y] = SDLK_y;
  keymap[DIKC_Z] = SDLK_z;
  
  keymap[DIKC_0] = SDLK_0;
  keymap[DIKC_1] = SDLK_1;
  keymap[DIKC_2] = SDLK_2;
  keymap[DIKC_3] = SDLK_3;
  keymap[DIKC_4] = SDLK_4;
  keymap[DIKC_5] = SDLK_5;
  keymap[DIKC_6] = SDLK_6;
  keymap[DIKC_7] = SDLK_7;
  keymap[DIKC_8] = SDLK_8;
  keymap[DIKC_9] = SDLK_9;
  
  keymap[DIKC_F1] = SDLK_F1;
  keymap[DIKC_F2] = SDLK_F2;
  keymap[DIKC_F3] = SDLK_F3;
  keymap[DIKC_F4] = SDLK_F4;
  keymap[DIKC_F5] = SDLK_F5;
  keymap[DIKC_F6] = SDLK_F6;
  keymap[DIKC_F7] = SDLK_F7;
  keymap[DIKC_F8] = SDLK_F8;
  keymap[DIKC_F9] = SDLK_F9;
  keymap[DIKC_F10] = SDLK_F10;
  keymap[DIKC_F11] = SDLK_F11;
  keymap[DIKC_F12] = SDLK_F12;
  
  keymap[DIKC_ESCAPE] = SDLK_ESCAPE;
  keymap[DIKC_LEFT] = SDLK_LEFT;
  keymap[DIKC_RIGHT] = SDLK_RIGHT;
  keymap[DIKC_UP] = SDLK_UP;
  keymap[DIKC_DOWN] = SDLK_DOWN;
  keymap[DIKC_CTRL] = SDLK_LCTRL;
  keymap[DIKC_SHIFT] = SDLK_LSHIFT;
  keymap[DIKC_ALT] = SDLK_LALT;
  keymap[DIKC_ALTGR] = SDLK_RALT;
  keymap[DIKC_TAB] = SDLK_TAB;
  keymap[DIKC_ENTER] = SDLK_RETURN;
  keymap[DIKC_SPACE] = SDLK_SPACE;
  keymap[DIKC_BACKSPACE] = SDLK_BACKSPACE;
  keymap[DIKC_INSERT] = SDLK_INSERT;
  keymap[DIKC_DELETE] = SDLK_DELETE;
  keymap[DIKC_HOME] = SDLK_HOME;
  keymap[DIKC_END] = SDLK_END;
  keymap[DIKC_PAGEUP] = SDLK_PAGEUP;
  keymap[DIKC_PAGEDOWN] = SDLK_PAGEDOWN;
  keymap[DIKC_CAPSLOCK] = SDLK_CAPSLOCK;
  keymap[DIKC_NUMLOCK] = SDLK_NUMLOCK;
  keymap[DIKC_SCRLOCK] = SDLK_SCROLLOCK;
  keymap[DIKC_PRINT] = SDLK_PRINT;
  keymap[DIKC_PAUSE] = SDLK_PAUSE;
  keymap[DIKC_KP_DIV] = SDLK_KP_DIVIDE;
  keymap[DIKC_KP_MULT] = SDLK_KP_MULTIPLY;
  keymap[DIKC_KP_MINUS] = SDLK_KP_MINUS;
  keymap[DIKC_KP_PLUS] = SDLK_KP_PLUS;
  keymap[DIKC_KP_ENTER] = SDLK_KP_ENTER;
  
  keymap[DIKC_OK] = SDLK_RETURN;
  keymap[DIKC_CANCEL] = SDLK_BREAK;
  keymap[DIKC_CLEAR] = SDLK_DELETE;
  keymap[DIKC_POWER] = SDLK_POWER;
  keymap[DIKC_POWER2] = SDLK_POWER;
  keymap[DIKC_MENU] = SDLK_MENU;
  keymap[DIKC_HELP] = SDLK_HELP;
  keymap[DIKC_BACK] = SDLK_ESCAPE;
}


static SDL_keysym *DirectFB_TranslateKey (DFBInputEvent *ev, SDL_keysym *keysym)
{
  /* Set the keysym information */
  keysym->scancode = ev->keycode;
  keysym->mod = KMOD_NONE;
  keysym->unicode = 0;

  if (ev->key_ascii > 0 && ev->key_ascii < 128)
    keysym->sym = ev->key_ascii;
  else
    keysym->sym = keymap[ev->keycode];

  return keysym;
}

static int DirectFB_TranslateButton (DFBInputEvent *ev)
{
  switch (ev->button)
    {
    case DIBI_LEFT:
      return 1;
    case DIBI_MIDDLE:
      return 2;
    case DIBI_RIGHT:
      return 3;
    default:
      return 0;
    }
}
