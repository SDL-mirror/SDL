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

/*
 *	Atari keyboard events manager, using hardware IKBD
 *
 *	Patrice Mandin
 */

#include <string.h>

/* Mint includes */
#include <mint/osbind.h>

#include "SDL.h"
#include "SDL_sysevents.h"
#include "SDL_events_c.h"

#include "SDL_atarikeys.h"
#include "SDL_ikbdinterrupt_s.h"

/* Special keys state */
enum {
	K_RSHIFT=0,
	K_LSHIFT,
	K_CTRL,
	K_ALT,
	K_CAPSLOCK,
	K_CLRHOME,
	K_INSERT
};

/* To save state of keyboard */
#define ATARIBIOS_MAXKEYS 128

static unsigned char ikbd_previouskeyboard[ATARIBIOS_MAXKEYS];
static Uint16 atari_prevmouseb;	/* buttons */

/* The translation tables from a console scancode to a SDL keysym */
#define KT_NOCHANGE -1

enum {
	KT_UNSHIFT=0,
	KT_SHIFT=1,
	KT_CAPS=2
};

static int caps_state;
_KEYTAB *curtables;
static unsigned char *tab_unshift, *tab_shift, *tab_caps;
static SDLKey keymap[ATARIBIOS_MAXKEYS];

static SDL_keysym *TranslateKey(int scancode, int numkeytable, SDL_keysym *keysym);

void AtariIkbd_InitOSKeymap(_THIS)
{
	int i;

	memset(SDL_AtariIkbd_keyboard, 0, ATARIBIOS_MAXKEYS);
	memset(ikbd_previouskeyboard, 0, ATARIBIOS_MAXKEYS);

	/* Initialize keymap */
	for ( i=0; i<sizeof(keymap); i++ )
		keymap[i] = SDLK_UNKNOWN;

	/* Functions keys */
	for ( i = 0; i<10; i++ )
		keymap[SCANCODE_F1 + i] = SDLK_F1+i;

	/* Cursor keypad */
	keymap[SCANCODE_HELP] = SDLK_HELP;
	keymap[SCANCODE_UNDO] = SDLK_UNDO;
	keymap[SCANCODE_INSERT] = SDLK_INSERT;
	keymap[SCANCODE_CLRHOME] = SDLK_HOME;
	keymap[SCANCODE_UP] = SDLK_UP;
	keymap[SCANCODE_DOWN] = SDLK_DOWN;
	keymap[SCANCODE_RIGHT] = SDLK_RIGHT;
	keymap[SCANCODE_LEFT] = SDLK_LEFT;

	/* Special keys */
	keymap[SCANCODE_ESCAPE] = SDLK_ESCAPE;
	keymap[SCANCODE_BACKSPACE] = SDLK_BACKSPACE;
	keymap[SCANCODE_TAB] = SDLK_TAB;
	keymap[SCANCODE_ENTER] = SDLK_RETURN;
	keymap[SCANCODE_DELETE] = SDLK_DELETE;
	keymap[SCANCODE_LEFTCONTROL] = SDLK_LCTRL;
	keymap[SCANCODE_LEFTSHIFT] = SDLK_LSHIFT;
	keymap[SCANCODE_RIGHTSHIFT] = SDLK_RSHIFT;
	keymap[SCANCODE_LEFTALT] = SDLK_LALT;
	keymap[SCANCODE_CAPSLOCK] = SDLK_CAPSLOCK;

	/* Read XBIOS tables for scancode -> ascii translation */
	curtables=Keytbl(KT_NOCHANGE, KT_NOCHANGE, KT_NOCHANGE);
	tab_unshift=curtables->unshift;
	tab_shift=curtables->shift;
	tab_caps=curtables->caps;

	/* Set Caps lock initial state */
	caps_state=(Kbshift(-1) & (1<<K_CAPSLOCK));

	/* Now install our handler */
	SDL_AtariIkbd_mouseb = SDL_AtariIkbd_mousex = SDL_AtariIkbd_mousey = 0;
	atari_prevmouseb = 0;

	Supexec(SDL_AtariIkbdInstall);
}

static int atari_GetButton(int button)
{
	switch(button)
	{
		case 0:
			return SDL_BUTTON_RIGHT;
			break;
		case 1:
		default:
			return SDL_BUTTON_LEFT;
			break;
	}
}

void AtariIkbd_PumpEvents(_THIS)
{
	int i;
	SDL_keysym keysym;
	int specialkeys;

	/*--- Send keyboard events ---*/

	/* Update caps lock state */
	if (SDL_AtariIkbd_keyboard[SCANCODE_CAPSLOCK] && !ikbd_previouskeyboard[SCANCODE_CAPSLOCK])
		caps_state ^= 1;

	/* Choose the translation table */
	specialkeys=KT_UNSHIFT;
	if (SDL_AtariIkbd_keyboard[SCANCODE_LEFTSHIFT] || SDL_AtariIkbd_keyboard[SCANCODE_RIGHTSHIFT])
		specialkeys = KT_SHIFT;
	if (caps_state)
		specialkeys = KT_CAPS;

	/* Now generate events */
	for (i=0; i<ATARIBIOS_MAXKEYS; i++) {
		/* Key pressed ? */
		if (SDL_AtariIkbd_keyboard[i] && !ikbd_previouskeyboard[i])
			SDL_PrivateKeyboard(SDL_PRESSED, TranslateKey(i, specialkeys, &keysym));
			
		/* Key unpressed ? */
		if (ikbd_previouskeyboard[i] && !SDL_AtariIkbd_keyboard[i])
			SDL_PrivateKeyboard(SDL_RELEASED, TranslateKey(i, specialkeys, &keysym));
	}

	/* Will be previous table */
	memcpy(ikbd_previouskeyboard, SDL_AtariIkbd_keyboard, ATARIBIOS_MAXKEYS);

	/*--- Send mouse events ---*/

	/* Mouse motion ? */
	if (SDL_AtariIkbd_mousex || SDL_AtariIkbd_mousey) {
		SDL_PrivateMouseMotion(0, 1, SDL_AtariIkbd_mousex, SDL_AtariIkbd_mousey);
		SDL_AtariIkbd_mousex = SDL_AtariIkbd_mousey = 0;
	}

	/* Mouse button ? */
	if (SDL_AtariIkbd_mouseb != atari_prevmouseb) {
		for (i=0;i<2;i++) {
			int curbutton, prevbutton;

			curbutton = SDL_AtariIkbd_mouseb & (1<<i);
			prevbutton = atari_prevmouseb & (1<<i);

			if (curbutton && !prevbutton) {
				SDL_PrivateMouseButton(SDL_PRESSED, atari_GetButton(i), 0, 0);
			}
			if (!curbutton && prevbutton) {
				SDL_PrivateMouseButton(SDL_RELEASED, atari_GetButton(i), 0, 0);
			}
		}
		atari_prevmouseb = SDL_AtariIkbd_mouseb;
	}
}

static SDL_keysym *TranslateKey(int scancode, int numkeytable, SDL_keysym *keysym)
{
	unsigned char asciicode;

	/* Set the keysym information */
	keysym->scancode = scancode;

	asciicode=0;
	switch(numkeytable) {
		case KT_UNSHIFT:
			asciicode=tab_unshift[scancode];
			break;
		case KT_SHIFT:
			asciicode=tab_shift[scancode];
			break;
		case KT_CAPS:
			asciicode=tab_caps[scancode];
			break;
	}

	if (asciicode)
		keysym->sym = asciicode;		
	else
		keysym->sym = keymap[scancode];

	keysym->mod = KMOD_NONE;
	keysym->unicode = 0;

	return(keysym);
}

void AtariIkbd_ShutdownEvents(void)
{
	Supexec(SDL_AtariIkbdUninstall);
}

