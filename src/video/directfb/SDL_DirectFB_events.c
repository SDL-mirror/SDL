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

  keymap[DIKI_A] = SDLK_a;
  keymap[DIKI_B] = SDLK_b;
  keymap[DIKI_C] = SDLK_c;
  keymap[DIKI_D] = SDLK_d;
  keymap[DIKI_E] = SDLK_e;
  keymap[DIKI_F] = SDLK_f;
  keymap[DIKI_G] = SDLK_g;
  keymap[DIKI_H] = SDLK_h;
  keymap[DIKI_I] = SDLK_i;
  keymap[DIKI_J] = SDLK_j;
  keymap[DIKI_K] = SDLK_k;
  keymap[DIKI_L] = SDLK_l;
  keymap[DIKI_M] = SDLK_m;
  keymap[DIKI_N] = SDLK_n;
  keymap[DIKI_O] = SDLK_o;
  keymap[DIKI_P] = SDLK_p;
  keymap[DIKI_Q] = SDLK_q;
  keymap[DIKI_R] = SDLK_r;
  keymap[DIKI_S] = SDLK_s;
  keymap[DIKI_T] = SDLK_t;
  keymap[DIKI_U] = SDLK_u;
  keymap[DIKI_V] = SDLK_v;
  keymap[DIKI_W] = SDLK_w;
  keymap[DIKI_X] = SDLK_x;
  keymap[DIKI_Y] = SDLK_y;
  keymap[DIKI_Z] = SDLK_z;
  
  keymap[DIKI_0] = SDLK_0;
  keymap[DIKI_1] = SDLK_1;
  keymap[DIKI_2] = SDLK_2;
  keymap[DIKI_3] = SDLK_3;
  keymap[DIKI_4] = SDLK_4;
  keymap[DIKI_5] = SDLK_5;
  keymap[DIKI_6] = SDLK_6;
  keymap[DIKI_7] = SDLK_7;
  keymap[DIKI_8] = SDLK_8;
  keymap[DIKI_9] = SDLK_9;
  
  keymap[DIKI_F1] = SDLK_F1;
  keymap[DIKI_F2] = SDLK_F2;
  keymap[DIKI_F3] = SDLK_F3;
  keymap[DIKI_F4] = SDLK_F4;
  keymap[DIKI_F5] = SDLK_F5;
  keymap[DIKI_F6] = SDLK_F6;
  keymap[DIKI_F7] = SDLK_F7;
  keymap[DIKI_F8] = SDLK_F8;
  keymap[DIKI_F9] = SDLK_F9;
  keymap[DIKI_F10] = SDLK_F10;
  keymap[DIKI_F11] = SDLK_F11;
  keymap[DIKI_F12] = SDLK_F12;
  
  keymap[DIKI_ESCAPE] = SDLK_ESCAPE;
  keymap[DIKI_LEFT] = SDLK_LEFT;
  keymap[DIKI_RIGHT] = SDLK_RIGHT;
  keymap[DIKI_UP] = SDLK_UP;
  keymap[DIKI_DOWN] = SDLK_DOWN;
  keymap[DIKI_CONTROL_L] = SDLK_LCTRL;
  keymap[DIKI_CONTROL_R] = SDLK_RCTRL;
  keymap[DIKI_SHIFT_L] = SDLK_LSHIFT;
  keymap[DIKI_SHIFT_R] = SDLK_RSHIFT;
  keymap[DIKI_ALT_L] = SDLK_LALT;
  keymap[DIKI_ALTGR] = SDLK_RALT;
  keymap[DIKI_TAB] = SDLK_TAB;
  keymap[DIKI_ENTER] = SDLK_RETURN;
  keymap[DIKI_SPACE] = SDLK_SPACE;
  keymap[DIKI_BACKSPACE] = SDLK_BACKSPACE;
  keymap[DIKI_INSERT] = SDLK_INSERT;
  keymap[DIKI_DELETE] = SDLK_DELETE;
  keymap[DIKI_HOME] = SDLK_HOME;
  keymap[DIKI_END] = SDLK_END;
  keymap[DIKI_PAGE_UP] = SDLK_PAGEUP;
  keymap[DIKI_PAGE_DOWN] = SDLK_PAGEDOWN;
  keymap[DIKI_CAPS_LOCK] = SDLK_CAPSLOCK;
  keymap[DIKI_NUM_LOCK] = SDLK_NUMLOCK;
  keymap[DIKI_SCROLL_LOCK] = SDLK_SCROLLOCK;
  keymap[DIKI_PRINT] = SDLK_PRINT;
  keymap[DIKI_PAUSE] = SDLK_PAUSE;
  keymap[DIKI_KP_DIV] = SDLK_KP_DIVIDE;
  keymap[DIKI_KP_MULT] = SDLK_KP_MULTIPLY;
  keymap[DIKI_KP_MINUS] = SDLK_KP_MINUS;
  keymap[DIKI_KP_PLUS] = SDLK_KP_PLUS;
  keymap[DIKI_KP_ENTER] = SDLK_KP_ENTER;
}


static SDL_keysym *DirectFB_TranslateKey (DFBInputEvent *ev, SDL_keysym *keysym)
{
  /* Set the keysym information */
  keysym->scancode = ev->key_id;
  keysym->mod = KMOD_NONE; /* FIXME */
  keysym->unicode = (DFB_KEY_TYPE (ev->key_symbol) == DIKT_UNICODE) ? ev->key_symbol : 0;

  if (ev->key_symbol > 0 && ev->key_symbol < 128)
    keysym->sym = ev->key_symbol;
  else
    keysym->sym = keymap[ev->key_id];

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
