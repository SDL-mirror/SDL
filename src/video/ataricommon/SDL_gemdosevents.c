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
 *	Atari keyboard events manager, using Gemdos
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
#include "SDL_xbiosevents_c.h"

/* To save state of keyboard */
#define ATARIBIOS_MAXKEYS 128

static unsigned char gemdos_currentkeyboard[ATARIBIOS_MAXKEYS];
static unsigned char gemdos_previouskeyboard[ATARIBIOS_MAXKEYS];
static unsigned char gemdos_currentascii[ATARIBIOS_MAXKEYS];

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

enum {
	DEV_BUSY=0,
	DEV_READY
};

/* The translation tables from a console scancode to a SDL keysym */
static SDLKey keymap[ATARIBIOS_MAXKEYS];

static SDL_keysym *TranslateKey(int scancode, int asciicode, SDL_keysym *keysym);
static void UpdateSpecialKeys(int special_keys_state);

void AtariGemdos_InitOSKeymap(_THIS)
{
	int i;

	memset(gemdos_currentkeyboard, 0, sizeof(gemdos_currentkeyboard));
	memset(gemdos_previouskeyboard, 0, sizeof(gemdos_previouskeyboard));

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

	SDL_AtariXbios_InstallVectors(ATARI_XBIOS_MOUSEEVENTS|ATARI_XBIOS_JOYSTICKEVENTS);
}

void AtariGemdos_PumpEvents(_THIS)
{
	int i;
	SDL_keysym keysym;

	/* Update pressed keys */
	memset(gemdos_currentkeyboard, 0, ATARIBIOS_MAXKEYS);

	while (Cconis()!=DEV_BUSY) {
		unsigned long key_pressed;
		unsigned char scancode, asciicode;

		key_pressed=Cnecin();

		asciicode = key_pressed;
		scancode = key_pressed >> 16;

		gemdos_currentkeyboard[scancode]=0xFF;
		gemdos_currentascii[scancode]=asciicode;
	}

	/* Read special keys */
	UpdateSpecialKeys(Kbshift(-1));

	/* Now generate events */
	for (i=0; i<ATARIBIOS_MAXKEYS; i++) {
		/* Key pressed ? */
		if (gemdos_currentkeyboard[i] && !gemdos_previouskeyboard[i])
			SDL_PrivateKeyboard(SDL_PRESSED, TranslateKey(i, gemdos_currentascii[i], &keysym));
			
		/* Key unpressed ? */
		if (gemdos_previouskeyboard[i] && !gemdos_currentkeyboard[i])
			SDL_PrivateKeyboard(SDL_RELEASED, TranslateKey(i, gemdos_currentascii[i], &keysym));
	}

	SDL_AtariXbios_PostMouseEvents(this);

	/* Will be previous table */
	memcpy(gemdos_previouskeyboard, gemdos_currentkeyboard, ATARIBIOS_MAXKEYS);
}

static void UpdateSpecialKeys(int special_keys_state)
{
#define UPDATE_SPECIAL_KEYS(numbit,scancode) \
	{	\
		if (special_keys_state & (1<<(numbit))) { \
			gemdos_currentkeyboard[scancode]=0xFF; \
			gemdos_currentascii[scancode]=0; \
		}	\
	}

	UPDATE_SPECIAL_KEYS(K_RSHIFT, SCANCODE_RIGHTSHIFT);
	UPDATE_SPECIAL_KEYS(K_LSHIFT, SCANCODE_LEFTSHIFT);
	UPDATE_SPECIAL_KEYS(K_CTRL, SCANCODE_LEFTCONTROL);
	UPDATE_SPECIAL_KEYS(K_ALT, SCANCODE_LEFTALT);
	UPDATE_SPECIAL_KEYS(K_CAPSLOCK, SCANCODE_CAPSLOCK);
}

static SDL_keysym *TranslateKey(int scancode, int asciicode, SDL_keysym *keysym)
{
	/* Set the keysym information */
	keysym->scancode = scancode;

	if (asciicode)
		keysym->sym = asciicode;		
	else
		keysym->sym = keymap[scancode];

	keysym->mod = KMOD_NONE;
	keysym->unicode = 0;

	return(keysym);
}

void AtariGemdos_ShutdownEvents(void)
{
	SDL_AtariXbios_RestoreVectors();
}
