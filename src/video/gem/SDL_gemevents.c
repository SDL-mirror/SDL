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
 * GEM SDL video driver implementation
 * inspired from the Dummy SDL driver
 * 
 * Patrice Mandin
 * and work from
 * Olivier Landemarre, Johan Klockars, Xavier Joubert, Claude Attard
 */

#include <string.h>

#include <gem.h>

#include "SDL.h"
#include "SDL_sysevents.h"
#include "SDL_events_c.h"
#include "SDL_gemvideo.h"
#include "SDL_gemevents_c.h"
#include "SDL_atarikeys.h"	/* for keyboard scancodes */
#include "SDL_xbiosinterrupt_s.h"

/* Defines */

#define ATARIBIOS_MAXKEYS 128

/* Variables */

static unsigned char gem_currentkeyboard[ATARIBIOS_MAXKEYS];
static unsigned char gem_previouskeyboard[ATARIBIOS_MAXKEYS];
static unsigned char gem_currentascii[ATARIBIOS_MAXKEYS];

/* The translation tables from a console scancode to a SDL keysym */
static SDLKey keymap[ATARIBIOS_MAXKEYS];

/* Functions prototypes */

static SDL_keysym *TranslateKey(int scancode, int asciicode, SDL_keysym *keysym);
static int do_messages(_THIS, short *message);
static void do_keyboard(short kc, short ks);
static void do_mouse(_THIS, short mx, short my, short mb, short ks);

/* Functions */

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

void GEM_InitOSKeymap(_THIS)
{
	int i;

	memset(gem_currentkeyboard, 0, sizeof(gem_currentkeyboard));
	memset(gem_previouskeyboard, 0, sizeof(gem_previouskeyboard));
	memset(gem_currentascii, 0, sizeof(gem_currentascii));

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

	/* Mouse init */
	GEM_mouse_relative = SDL_FALSE;
}

void GEM_PumpEvents(_THIS)
{
	short mousex, mousey, mouseb, dummy;
	short kstate, prevkc, prevks;
	int i;
	SDL_keysym	keysym;

	memset(gem_currentkeyboard,0,sizeof(gem_currentkeyboard));
	prevkc = prevks = 0;
	
	for (;;)
	{
		int quit, resultat;
		short buffer[8], kc;

		quit = 0;

		resultat = evnt_multi(
			MU_MESAG|MU_TIMER|MU_KEYBD,
			0,0,0,
			0,0,0,0,0,
			0,0,0,0,0,
			buffer,
			10,
			&dummy,&dummy,&dummy,&kstate,&kc,&dummy
		);

		/* Message event ? */
		if (resultat & MU_MESAG)
			quit = do_messages(this, buffer);

		/* Keyboard event ? */
		if (resultat & MU_KEYBD) {
			if ((prevkc != kc) || (prevks != kstate)) {
				do_keyboard(kc,kstate);
			} else {
				/* Avoid looping, if repeating same key */
				break;
			}
		}

		/* Timer event ? */
		if ((resultat & MU_TIMER) || quit)
			break;
	}

	/* Update mouse */
	graf_mkstate(&mousex, &mousey, &mouseb, &kstate);
	do_mouse(this, mousex, mousey, mouseb, kstate);

	/* Now generate keyboard events */
	for (i=0; i<ATARIBIOS_MAXKEYS; i++) {
		/* Key pressed ? */
		if (gem_currentkeyboard[i] && !gem_previouskeyboard[i])
			SDL_PrivateKeyboard(SDL_PRESSED, TranslateKey(i, gem_currentascii[i], &keysym));
			
		/* Key unpressed ? */
		if (gem_previouskeyboard[i] && !gem_currentkeyboard[i])
			SDL_PrivateKeyboard(SDL_RELEASED, TranslateKey(i, gem_currentascii[i], &keysym));
	}

	memcpy(gem_previouskeyboard,gem_currentkeyboard,sizeof(gem_previouskeyboard));
}

static int do_messages(_THIS, short *message)
{
	int quit, posted;

	quit=0;
	switch (message[0]) {
		case WM_CLOSED:
		case AP_TERM:    
			posted = SDL_PrivateQuit();
			quit=1;
			break;
		case WM_MOVED:
			wind_set(message[3],WF_CURRXYWH,message[4],message[5],message[6],message[7]);
			break;
		case WM_TOPPED:
			wind_set(message[3],WF_TOP,message[4],0,0,0);
			SDL_PrivateAppActive(1, SDL_APPINPUTFOCUS);
			break;
		case WM_REDRAW:
			GEM_wind_redraw(this, message[3],&message[4]);
			break;
		case WM_ICONIFY:
		case WM_ALLICONIFY:
			wind_set(message[3],WF_ICONIFY,message[4],message[5],message[6],message[7]);
			/* If we're active, make ourselves inactive */
			if ( SDL_GetAppState() & SDL_APPACTIVE ) {
				/* Send an internal deactivate event */
				SDL_PrivateAppActive(0, SDL_APPACTIVE|SDL_APPINPUTFOCUS);
			}
			/* Update window title */
			if (GEM_refresh_name && GEM_icon_name) {
				wind_set(GEM_handle,WF_NAME,(short)(((unsigned long)GEM_icon_name)>>16),(short)(((unsigned long)GEM_icon_name) & 0xffff),0,0);
				GEM_refresh_name = SDL_FALSE;
			}
			break;
		case WM_UNICONIFY:
			wind_set(message[3],WF_UNICONIFY,message[4],message[5],message[6],message[7]);
			/* If we're not active, make ourselves active */
			if ( !(SDL_GetAppState() & SDL_APPACTIVE) ) {
				/* Send an internal activate event */
				SDL_PrivateAppActive(1, SDL_APPACTIVE);
			}
			if (GEM_refresh_name && GEM_title_name) {
				wind_set(GEM_handle,WF_NAME,(short)(((unsigned long)GEM_title_name)>>16),(short)(((unsigned long)GEM_title_name) & 0xffff),0,0);
				GEM_refresh_name = SDL_FALSE;
			}
			break;
		case WM_SIZED:
			wind_set (message[3], WF_CURRXYWH, message[4], message[5], message[6], message[7]);
			GEM_win_fulled = SDL_FALSE;		/* Cancel maximized flag */
			SDL_PrivateResize(message[6], message[7]);
			break;
		case WM_FULLED:
			{
				short x,y,w,h;

				if (GEM_win_fulled) {
					wind_get (message[3], WF_PREVXYWH, &x, &y, &w, &h);
					GEM_win_fulled = SDL_FALSE;
				} else {
					x = GEM_desk_x;
					y = GEM_desk_y;
					w = GEM_desk_w;
					h = GEM_desk_h;
					GEM_win_fulled = SDL_TRUE;
				}
				wind_set (message[3], WF_CURRXYWH, x, y, w, h);
				SDL_PrivateResize(w, h);
			}
			break;
		case WM_BOTTOMED:
		case WM_UNTOPPED:
			SDL_PrivateAppActive(0, SDL_APPINPUTFOCUS);
			break;
	}
	
	return quit;
}

static void do_keyboard(short kc, short ks)
{
	int			scancode, asciicode;

	if (kc) {
		scancode=(kc>>8) & 127;
		asciicode=kc & 255;

		gem_currentkeyboard[scancode]=0xFF;
		gem_currentascii[scancode]=asciicode;
	}

	/* Read special keys */
	if (ks & K_RSHIFT)
		gem_currentkeyboard[SCANCODE_RIGHTSHIFT]=0xFF;
	if (ks & K_LSHIFT)
		gem_currentkeyboard[SCANCODE_LEFTSHIFT]=0xFF;
	if (ks & K_CTRL)
		gem_currentkeyboard[SCANCODE_LEFTCONTROL]=0xFF;
	if (ks & K_ALT)
		gem_currentkeyboard[SCANCODE_LEFTALT]=0xFF;
}

static void do_mouse(_THIS, short mx, short my, short mb, short ks)
{
	static short prevmousex=0, prevmousey=0, prevmouseb=0;

	/* Mouse motion ? */
	if ((prevmousex!=mx) || (prevmousey!=my)) {
		if (GEM_mouse_relative) {
			SDL_PrivateMouseMotion(0, 1, SDL_AtariXbios_mousex, SDL_AtariXbios_mousey);
			SDL_AtariXbios_mousex = SDL_AtariXbios_mousey = 0;
		} else {
			SDL_PrivateMouseMotion(0, 1, mx, my);
		}
		prevmousex = mx;
		prevmousey = my;
	}

	/* Mouse button ? */
	if (prevmouseb!=mb) {
		int i;

		for (i=0;i<2;i++) {
			int curbutton, prevbutton;
		
			curbutton = mb & (1<<i);
			prevbutton = prevmouseb & (1<<i);
		
			if (curbutton && !prevbutton) {
				SDL_PrivateMouseButton(SDL_PRESSED, i+1, 0, 0);
			}
			if (!curbutton && prevbutton) {
				SDL_PrivateMouseButton(SDL_RELEASED, i+1, 0, 0);
			}
		}
		prevmouseb = mb;
	}

	/* Read special keys */
	if (ks & K_RSHIFT)
		gem_currentkeyboard[SCANCODE_RIGHTSHIFT]=0xFF;
	if (ks & K_LSHIFT)
		gem_currentkeyboard[SCANCODE_LEFTSHIFT]=0xFF;
	if (ks & K_CTRL)
		gem_currentkeyboard[SCANCODE_LEFTCONTROL]=0xFF;
	if (ks & K_ALT)
		gem_currentkeyboard[SCANCODE_LEFTALT]=0xFF;
}
