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

#include <support/UTF8.h>
#include <stdio.h>
#include <string.h>
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_BWin.h"
#include "SDL_lowvideo.h"

extern "C" {

#include "SDL_events_c.h"
#include "SDL_sysevents.h"
#include "SDL_sysevents_c.h"

/* A note on why the polling loops are necessary.
   The BeOS Preview 2 implementation of BView->MouseMoved() only notifies
   the view when the mouse enters or leaves the view.  The documentation 
   says that you should loop and use GetMouse() to detect mouse motion.
   The BeOS Preview 2 implementation of BView->KeyDown() and BView->KeyUp()
   are only called for keys that generate ASCII characters.  Since we want
   to act like a low level raw keyboard, we need to poll the state of the
   keys instead of catching the keys in callbacks.
   These are documented portions of the BeBook for Preview Release 2
*/
 
/* Table to convert scancodes to SDL virtual keys */
static SDLKey keymap[128];

/* Function to convert from a key scancode to a UNICODE character */
static key_map *be_keymap;
static int32 *option_caps_map[2], *option_map[2], *caps_map[2], *normal_map[2];
static char *unicode_map;
static Uint16 TranslateScancode(int scancode)
{
	SDLMod modstate;
	int shifted;
	int32 index;		/* Index into system unicode map */
	Uint16 unicode;

	/* Set the default character -- no character */
	unicode = 0;

	/* See whether or not the shift state is set */
	modstate = SDL_GetModState();
	if ( modstate & KMOD_SHIFT ) {
		shifted = 1;
	} else {
		shifted = 0;
	}

	if ( modstate & KMOD_NUM ) {
	/* If numlock is on, numeric keypad keys have shift state inverted */
		switch (keymap[scancode]) {
		    case SDLK_KP0:
		    case SDLK_KP1:
		    case SDLK_KP2:
		    case SDLK_KP3:
		    case SDLK_KP4:
		    case SDLK_KP5:
		    case SDLK_KP6:
		    case SDLK_KP7:
		    case SDLK_KP8:
		    case SDLK_KP9:
		    case SDLK_KP_PERIOD:
		    case SDLK_KP_DIVIDE:
		    case SDLK_KP_MULTIPLY:
		    case SDLK_KP_MINUS:
		    case SDLK_KP_PLUS:
		    case SDLK_KP_ENTER:
		    case SDLK_KP_EQUALS:
			shifted = !shifted;
			break;
		    default:
			break;
		}
	}

	/* Get the index based on modifier state */
	if ( modstate & KMOD_CTRL )
		index = be_keymap->control_map[scancode];
	else
	if ( (modstate & KMOD_META) && (modstate & KMOD_CAPS) )
		index = option_caps_map[shifted][scancode];
	else
	if ( modstate & KMOD_META )
		index = option_map[shifted][scancode];
	else
	if ( modstate & KMOD_CAPS )
		index = caps_map[shifted][scancode];
	else
		index = normal_map[shifted][scancode];

	/* If there is a mapping, convert character from UTF-8 to UNICODE */
	if ( unicode_map[index] ) {
		int32 state, srclen, dstlen;
		unsigned char destbuf[2];

		state = 0;
		srclen = unicode_map[index++];
		dstlen = sizeof(destbuf);
		convert_from_utf8(B_UNICODE_CONVERSION,
			&unicode_map[index], &srclen, (char *)destbuf, &dstlen,
									&state);
		unicode = destbuf[0];
		unicode <<= 8;
		unicode |= destbuf[1];

		/* Keyboard input maps newline to carriage return */
		if ( unicode == '\n' ) {
			unicode = '\r';
		}

		/* For some reason function keys map to control characters */
# define CTRL(X)	((X)-'@')
		switch (unicode) {
		    case CTRL('A'):
		    case CTRL('B'):
		    case CTRL('C'):
		    case CTRL('D'):
		    case CTRL('E'):
		    case CTRL('K'):
		    case CTRL('L'):
		    case CTRL('P'):
			if ( ! (modstate & KMOD_CTRL) )
				unicode = 0;
			break;
		    default:
			break;
		}
	}
	return(unicode);
}

/* Function to translate a keyboard transition and queue the key event */
static void QueueKey(int scancode, int pressed)
{
	SDL_keysym keysym;

	/* Set the keysym information */
	keysym.scancode = scancode;
	keysym.sym = keymap[scancode];
	keysym.mod = KMOD_NONE;
	if ( SDL_TranslateUNICODE ) {
		keysym.unicode = TranslateScancode(scancode);
	} else {
		keysym.unicode = 0;
	}

	/* NUMLOCK and CAPSLOCK are implemented as double-presses in reality */
	if ( (keysym.sym == SDLK_NUMLOCK) || (keysym.sym == SDLK_CAPSLOCK) ) {
		pressed = 1;
	}

	/* Queue the key event */
	if ( pressed ) {
		SDL_PrivateKeyboard(SDL_PRESSED, &keysym);
	} else {
		SDL_PrivateKeyboard(SDL_RELEASED, &keysym);
	}
}

/* This is special because we know it will be run in a loop in a separate
   thread.  Normally this function should loop as long as there are input
   states changing, i.e. new events arriving.
*/
void BE_PumpEvents(_THIS)
{
	BView *view;
	BRect bounds;
	BPoint point;
	uint32 buttons;
	const uint32 button_masks[3] = {
		B_PRIMARY_MOUSE_BUTTON,
		B_TERTIARY_MOUSE_BUTTON, 
		B_SECONDARY_MOUSE_BUTTON,
	};
	unsigned int    i, j;

	/* Check out the mouse buttons and position (slight race condition) */
	if ( SDL_Win->Lock() ) {
		/* Don't do anything if we have no view */
		view = SDL_Win->View();
		if ( ! view ) {
			SDL_Win->Unlock();
			return;
		}
		bounds = view->Bounds();
		/* Get new input state, if still active */
		if ( SDL_Win->IsActive() ) {
			key_flip = !key_flip;
			get_key_info(&keyinfo[key_flip]);
			view->GetMouse(&point, &buttons, true);
		} else {
			key_flip = key_flip;
			point = last_point;
			buttons = last_buttons;
		}
		SDL_Win->Unlock();
	} else {
		return;
	}

	/* If our view is active, we'll find key changes here */
	if ( memcmp(keyinfo[0].key_states, keyinfo[1].key_states, 16) != 0 ) {
		for ( i=0; i<16; ++i ) {
			Uint8 new_state, transition;

			new_state = keyinfo[key_flip].key_states[i];
			transition = keyinfo[!key_flip].key_states[i] ^
					keyinfo[ key_flip].key_states[i];
			for ( j=0; j<8; ++j ) {
				if ( transition&0x80 )
					QueueKey(i*8+j, new_state&0x80);
				transition <<= 1;
				new_state <<= 1;
			}
		}
	}

	/* We check keyboard, but not mouse if mouse isn't in window */
	if ( ! bounds.Contains(point) ) {
		/* Mouse moved outside our view? */
		if ( SDL_GetAppState() & SDL_APPMOUSEFOCUS ) {
			SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);
			be_app->SetCursor(B_HAND_CURSOR);
		}
		return;
	}
	/* Has the mouse moved back into our view? */
	if ( ! (SDL_GetAppState() & SDL_APPMOUSEFOCUS) ) {
		/* Reset the B_HAND_CURSOR to our own */
		SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
		SDL_SetCursor(NULL);
	}

	/* Check for mouse motion */
	if ( point != last_point ) {
		int x, y;

		SDL_Win->GetXYOffset(x, y);
		x = (int)point.x - x;
		y = (int)point.y - y;
		SDL_PrivateMouseMotion(0, 0, x, y);
	}
	last_point = point;

	/* Add any mouse button events */
	for ( i=0; i<SDL_TABLESIZE(button_masks); ++i ) {
		if ( (buttons ^ last_buttons) & button_masks[i] ) {
			if ( buttons & button_masks[i] ) {
				SDL_PrivateMouseButton(SDL_PRESSED, 1+i, 0, 0);
			} else {
				SDL_PrivateMouseButton(SDL_RELEASED, 1+i, 0, 0);
			}
		}
	}
	last_buttons = buttons;
}

void BE_InitOSKeymap(_THIS)
{
	unsigned int i;

	/* Initialize the keyboard state */
	key_flip = 0;
	get_key_info(&keyinfo[key_flip]);
	memcpy(keyinfo[!key_flip].key_states,
	       keyinfo[key_flip].key_states,
	       SDL_TABLESIZE(keyinfo[key_flip].key_states));

	/* Initialize the BeOS key translation table */
	/* Source: <be/interface/InterfaceDefs.h> and BeOS keyboard info */
	for ( i=0; i<SDL_TABLESIZE(keymap); ++i )
		keymap[i] = SDLK_UNKNOWN;

	keymap[0x01]		= SDLK_ESCAPE;
	keymap[B_F1_KEY]	= SDLK_F1;
	keymap[B_F2_KEY]	= SDLK_F2;
	keymap[B_F3_KEY]	= SDLK_F3;
	keymap[B_F4_KEY]	= SDLK_F4;
	keymap[B_F5_KEY]	= SDLK_F5;
	keymap[B_F6_KEY]	= SDLK_F6;
	keymap[B_F7_KEY]	= SDLK_F7;
	keymap[B_F8_KEY]	= SDLK_F8;
	keymap[B_F9_KEY]	= SDLK_F9;
	keymap[B_F10_KEY]	= SDLK_F10;
	keymap[B_F11_KEY]	= SDLK_F11;
	keymap[B_F12_KEY]	= SDLK_F12;
	keymap[B_PRINT_KEY]	= SDLK_PRINT;
	//keymap[B_SCROLL_KEY]	= SDLK_SCROLLOCK;
	keymap[B_PAUSE_KEY]	= SDLK_PAUSE;
	keymap[0x11]		= SDLK_BACKQUOTE;
	keymap[0x12]		= SDLK_1;
	keymap[0x13]		= SDLK_2;
	keymap[0x14]		= SDLK_3;
	keymap[0x15]		= SDLK_4;
	keymap[0x16]		= SDLK_5;
	keymap[0x17]		= SDLK_6;
	keymap[0x18]		= SDLK_7;
	keymap[0x19]		= SDLK_8;
	keymap[0x1a]		= SDLK_9;
	keymap[0x1b]		= SDLK_0;
	keymap[0x1c]		= SDLK_MINUS;
	keymap[0x1d]		= SDLK_EQUALS;
	keymap[0x1e]		= SDLK_BACKSPACE;
	keymap[0x1f]		= SDLK_INSERT;
	keymap[0x20]		= SDLK_HOME;
	keymap[0x21]		= SDLK_PAGEUP;
	//keymap[0x22]		= SDLK_NUMLOCK;
	keymap[0x23]		= SDLK_KP_DIVIDE;
	keymap[0x24]		= SDLK_KP_MULTIPLY;
	keymap[0x25]		= SDLK_KP_MINUS;
	keymap[0x26]		= SDLK_TAB;
	keymap[0x27]		= SDLK_q;
	keymap[0x28]		= SDLK_w;
	keymap[0x29]		= SDLK_e;
	keymap[0x2a]		= SDLK_r;
	keymap[0x2b]		= SDLK_t;
	keymap[0x2c]		= SDLK_y;
	keymap[0x2d]		= SDLK_u;
	keymap[0x2e]		= SDLK_i;
	keymap[0x2f]		= SDLK_o;
	keymap[0x30]		= SDLK_p;
	keymap[0x31]		= SDLK_LEFTBRACKET;
	keymap[0x32]		= SDLK_RIGHTBRACKET;
	keymap[0x33]		= SDLK_BACKSLASH;
	keymap[0x34]		= SDLK_DELETE;
	keymap[0x35]		= SDLK_END;
	keymap[0x36]		= SDLK_PAGEDOWN;
	keymap[0x37]		= SDLK_KP7;
	keymap[0x38]		= SDLK_KP8;
	keymap[0x39]		= SDLK_KP9;
	keymap[0x3a]		= SDLK_KP_PLUS;
	//keymap[0x3b]		= SDLK_CAPSLOCK;
	keymap[0x3c]		= SDLK_a;
	keymap[0x3d]		= SDLK_s;
	keymap[0x3e]		= SDLK_d;
	keymap[0x3f]		= SDLK_f;
	keymap[0x40]		= SDLK_g;
	keymap[0x41]		= SDLK_h;
	keymap[0x42]		= SDLK_j;
	keymap[0x43]		= SDLK_k;
	keymap[0x44]		= SDLK_l;
	keymap[0x45]		= SDLK_SEMICOLON;
	keymap[0x46]		= SDLK_QUOTE;
	keymap[0x47]		= SDLK_RETURN;
	keymap[0x48]		= SDLK_KP4;
	keymap[0x49]		= SDLK_KP5;
	keymap[0x4a]		= SDLK_KP6;
	keymap[0x4b]		= SDLK_LSHIFT;
	keymap[0x4c]		= SDLK_z;
	keymap[0x4d]		= SDLK_x;
	keymap[0x4e]		= SDLK_c;
	keymap[0x4f]		= SDLK_v;
	keymap[0x50]		= SDLK_b;
	keymap[0x51]		= SDLK_n;
	keymap[0x52]		= SDLK_m;
	keymap[0x53]		= SDLK_COMMA;
	keymap[0x54]		= SDLK_PERIOD;
	keymap[0x55]		= SDLK_SLASH;
	keymap[0x56]		= SDLK_RSHIFT;
	keymap[0x57]		= SDLK_UP;
	keymap[0x58]		= SDLK_KP1;
	keymap[0x59]		= SDLK_KP2;
	keymap[0x5a]		= SDLK_KP3;
	keymap[0x5b]		= SDLK_KP_ENTER;
	//keymap[0x5c]		= SDLK_LCTRL;
	//keymap[0x5d]		= SDLK_LALT;
	keymap[0x5e]		= SDLK_SPACE;
	//keymap[0x5f]		= SDLK_RALT;
	//keymap[0x60]		= SDLK_RCTRL;
	keymap[0x61]		= SDLK_LEFT;
	keymap[0x62]		= SDLK_DOWN;
	keymap[0x63]		= SDLK_RIGHT;
	keymap[0x64]		= SDLK_KP0;
	keymap[0x65]		= SDLK_KP_PERIOD;
	//keymap[0x66]		= SDLK_LMETA;
	//keymap[0x67]		= SDLK_RMETA;
	//keymap[0x68]		= SDLK_MENU;
	keymap[0x69]		= SDLK_EURO;
	keymap[0x6a]		= SDLK_KP_EQUALS;
	keymap[0x6b]		= SDLK_POWER;

	/* Get the system keymap and UNICODE table.
	   Note that this leaks memory since the maps are never freed.
	 */
	get_key_map(&be_keymap, &unicode_map);

	/* Set the modifier keys from the system keymap */
	keymap[be_keymap->caps_key] = SDLK_CAPSLOCK;
	keymap[be_keymap->scroll_key] = SDLK_SCROLLOCK;
	keymap[be_keymap->num_key] = SDLK_NUMLOCK;
	keymap[be_keymap->left_shift_key] = SDLK_LSHIFT;
	keymap[be_keymap->right_shift_key] = SDLK_RSHIFT;
	keymap[be_keymap->left_command_key] = SDLK_LALT;
	keymap[be_keymap->right_command_key] = SDLK_RALT;
	keymap[be_keymap->left_control_key] = SDLK_LCTRL;
	keymap[be_keymap->right_control_key] = SDLK_RCTRL;
	keymap[be_keymap->left_option_key] = SDLK_LMETA;
	keymap[be_keymap->right_option_key] = SDLK_RMETA;
	keymap[be_keymap->menu_key] = SDLK_MENU;

	/* Set the modifier map pointers */
	option_caps_map[0] = be_keymap->option_caps_map;
	option_caps_map[1] = be_keymap->option_caps_shift_map;
	option_map[0] = be_keymap->option_map;
	option_map[1] = be_keymap->option_shift_map;
	caps_map[0] = be_keymap->caps_map;
	caps_map[1] = be_keymap->caps_shift_map;
	normal_map[0] = be_keymap->normal_map;
	normal_map[1] = be_keymap->shift_map;
}

}; /* Extern C */
