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
 *	Atari keyboard events manager, using BIOS
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

static unsigned char bios_currentkeyboard[ATARIBIOS_MAXKEYS];
static unsigned char bios_previouskeyboard[ATARIBIOS_MAXKEYS];
static unsigned char bios_currentascii[ATARIBIOS_MAXKEYS];

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

/* The translation tables from a console scancode to a SDL keysym */
static SDLKey keymap[ATARIBIOS_MAXKEYS];

static SDL_keysym *TranslateKey(int scancode, int asciicode, SDL_keysym *keysym);
static void UpdateSpecialKeys(int special_keys_state);

void AtariBios_InitOSKeymap(_THIS)
{
	int i;

	memset(bios_currentkeyboard, 0, sizeof(bios_currentkeyboard));
	memset(bios_previouskeyboard, 0, sizeof(bios_previouskeyboard));

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

void AtariBios_PumpEvents(_THIS)
{
	int i;
	SDL_keysym keysym;

	/* Update pressed keys */
	memset(bios_currentkeyboard, 0, ATARIBIOS_MAXKEYS);

	while (Bconstat(_CON)) {
		unsigned long key_pressed;
		unsigned char asciicode, scancode;

		key_pressed=Bconin(_CON);

		asciicode = key_pressed;
		scancode = key_pressed >> 16;

		bios_currentkeyboard[scancode]=0xFF;
		bios_currentascii[scancode]=asciicode;
	}

	/* Read special keys */
	UpdateSpecialKeys(Kbshift(-1));

	/* Now generate events */
	for (i=0; i<ATARIBIOS_MAXKEYS; i++) {
		/* Key pressed ? */
		if (bios_currentkeyboard[i] && !bios_previouskeyboard[i])
			SDL_PrivateKeyboard(SDL_PRESSED, TranslateKey(i, bios_currentascii[i], &keysym));
			
		/* Key unpressed ? */
		if (bios_previouskeyboard[i] && !bios_currentkeyboard[i])
			SDL_PrivateKeyboard(SDL_RELEASED, TranslateKey(i, bios_currentascii[i], &keysym));
	}

	SDL_AtariXbios_PostMouseEvents(this);

	/* Will be previous table */
	memcpy(bios_previouskeyboard, bios_currentkeyboard, ATARIBIOS_MAXKEYS);
}

static void UpdateSpecialKeys(int special_keys_state)
{
#define UPDATE_SPECIAL_KEYS(numbit,scancode) \
	{	\
		if (special_keys_state & (1<<(numbit))) { \
			bios_currentkeyboard[scancode]=0xFF; \
			bios_currentascii[scancode]=0; \
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

void AtariBios_ShutdownEvents(void)
{
	SDL_AtariXbios_RestoreVectors();
}
