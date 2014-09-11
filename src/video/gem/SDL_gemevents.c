/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/*
 * GEM SDL video driver implementation
 * inspired from the Dummy SDL driver
 * 
 * Patrice Mandin
 * and work from
 * Olivier Landemarre, Johan Klockars, Xavier Joubert, Claude Attard
 */

#include <gem.h>

#include "SDL_timer.h"
#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"
#include "SDL_gemvideo.h"
#include "SDL_gemevents_c.h"
#include "SDL_gemmouse_c.h"
#include "../ataricommon/SDL_atarikeys.h"	/* for keyboard scancodes */
#include "../ataricommon/SDL_atarievents_c.h"
#include "../ataricommon/SDL_xbiosevents_c.h"
#include "../ataricommon/SDL_ataridevmouse_c.h"

/* Duration after which we consider key released */

#define KEY_PRESS_DURATION 100

#define MSG_SDL_ID	(('S'<<8)|'D')

/* Variables */

static unsigned char gem_currentkeyboard[ATARIBIOS_MAXKEYS];
static unsigned char gem_previouskeyboard[ATARIBIOS_MAXKEYS];
static Uint32 keyboard_ticks[ATARIBIOS_MAXKEYS];

static short prevmx=0,prevmy=0,prevmb=0;
static short dummy_msgbuf[8] = {MSG_SDL_ID,0,0,0, 0,0,0,0};

/* Functions prototypes */

static int do_messages(_THIS, short *message, short latest_msg_id);
static void do_keyboard(short kc, Uint32 tick);
static void do_keyboard_special(short ks, Uint32 tick);
static void do_mouse_motion(_THIS, short mx, short my);
static void do_mouse_buttons(_THIS, short mb);
static int mouse_in_work_area(int winhandle, short mx, short my);
static void clearKeyboardState(Uint32 tick);

/* Functions */

void GEM_InitOSKeymap(_THIS)
{
	SDL_memset(gem_currentkeyboard, 0, sizeof(gem_currentkeyboard));
	SDL_memset(gem_previouskeyboard, 0, sizeof(gem_previouskeyboard));
	SDL_memset(keyboard_ticks, 0, sizeof(keyboard_ticks));

	/* Mouse init */
	GEM_mouse_relative = SDL_FALSE;

	SDL_Atari_InitInternalKeymap(this);
}

void GEM_PumpEvents(_THIS)
{
	short prevkc=0, mousex, mousey, mouseb, kstate;
	int i, quit = 0;
	SDL_keysym keysym;
	Uint32 cur_tick;
	static Uint32 prev_now = 0, prev_msg = 0;
	static short latest_msg_id = 0;

	cur_tick = SDL_GetTicks();
	if (prev_now == cur_tick) {
		return;
	}
	prev_now = cur_tick;

	SDL_AtariMint_BackgroundTasks();
	clearKeyboardState(cur_tick);

	if (prev_msg) {
		/* Wait at least 20ms before each event processing loop */
		if (cur_tick-prev_msg < 20) {
			return;
		}
	}
	prev_msg = cur_tick;

	dummy_msgbuf[1] = ++latest_msg_id;
	if (appl_write(GEM_ap_id, sizeof(dummy_msgbuf), dummy_msgbuf) == 0) {
		/* If it fails, wait for previous id */
		--latest_msg_id;
	}

	while (!quit) {
		int resultat;
		short buffer[8], kc, dummy;

		resultat = evnt_multi(
			MU_MESAG|MU_TIMER|MU_KEYBD,
			0,0,0,
			0,0,0,0,0,
			0,0,0,0,0,
			buffer,
			1000,
			&dummy,&dummy,&dummy,&kstate,&kc,&dummy
		);

		/* Message event ? */
		if (resultat & MU_MESAG)
			quit = do_messages(this, buffer, latest_msg_id);

		/* Keyboard event ? */
		if (resultat & MU_KEYBD) {
			do_keyboard_special(kstate, cur_tick);
			if (prevkc != kc) {
				do_keyboard(kc, cur_tick);
				prevkc = kc;
			}
		}

		/* Timer event ? Just used as a safeguard */
		if (resultat & MU_TIMER) {
			quit = 1;
		}
	}

	GEM_CheckMouseMode(this);

	/* Update mouse state */
	graf_mkstate(&mousex, &mousey, &mouseb, &kstate);
	do_keyboard_special(kstate, cur_tick);
	do_mouse_motion(this, mousex, mousey);
	do_mouse_buttons(this, mouseb);

	/* Now generate keyboard events */
	for (i=0; i<ATARIBIOS_MAXKEYS; i++) {
		/* Key pressed ? */
		if (gem_currentkeyboard[i] && !gem_previouskeyboard[i])
			SDL_PrivateKeyboard(SDL_PRESSED,
				SDL_Atari_TranslateKey(i, &keysym, SDL_TRUE));
			
		/* Key unpressed ? */
		if (gem_previouskeyboard[i] && !gem_currentkeyboard[i])
			SDL_PrivateKeyboard(SDL_RELEASED,
				SDL_Atari_TranslateKey(i, &keysym, SDL_FALSE));
	}

	SDL_memcpy(gem_previouskeyboard,gem_currentkeyboard,sizeof(gem_previouskeyboard));

	/* Refresh window name ? */
	if (GEM_refresh_name) {
		const char *window_name =
			(SDL_GetAppState() & SDL_APPACTIVE)
			? GEM_title_name : GEM_icon_name;
		if (window_name) {
			wind_set(GEM_handle,WF_NAME,
				(short)(((unsigned long)window_name)>>16),
				(short)(((unsigned long)window_name) & 0xffff),
				0,0);
		}
		GEM_refresh_name = SDL_FALSE;
	}
}

static int do_messages(_THIS, short *message, short latest_msg_id)
{
	int quit;
	short x2,y2,w2,h2;

	quit = 0;
	switch (message[0]) {
		case MSG_SDL_ID:
			quit=(message[1] == latest_msg_id);
			break;
		case WM_CLOSED:
		case AP_TERM:    
			SDL_PrivateQuit();
			quit=1;
			break;
		case WM_MOVED:
			wind_set(message[3],WF_CURRXYWH,message[4],message[5],message[6],message[7]);
			break;
		case WM_TOPPED:
			wind_set(message[3],WF_TOP,message[4],0,0,0);
			/* Continue with TOP event processing */
		case WM_ONTOP:
			SDL_PrivateAppActive(1, SDL_APPINPUTFOCUS);
			if (VDI_setpalette) {
				VDI_setpalette(this, VDI_curpalette);
			}
			break;
		case WM_REDRAW:
			if (!GEM_lock_redraw) {
				GEM_wind_redraw(this, message[3],&message[4]);
			}
			break;
		case WM_ICONIFY:
		case WM_ALLICONIFY:
			wind_set(message[3],WF_ICONIFY,message[4],message[5],message[6],message[7]);
			/* If we're active, make ourselves inactive */
			if ( SDL_GetAppState() & SDL_APPACTIVE ) {
				/* Send an internal deactivate event */
				SDL_PrivateAppActive(0, SDL_APPACTIVE);
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
			wind_get (message[3], WF_WORKXYWH, &x2, &y2, &w2, &h2);
			GEM_win_fulled = SDL_FALSE;		/* Cancel maximized flag */
			GEM_lock_redraw = SDL_TRUE;		/* Prevent redraw till buffers resized */
			SDL_PrivateResize(w2, h2);
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
				wind_get (message[3], WF_WORKXYWH, &x2, &y2, &w2, &h2);
				GEM_lock_redraw = SDL_TRUE;		/* Prevent redraw till buffers resized */
				SDL_PrivateResize(w2, h2);
			}
			break;
		case WM_BOTTOMED:
			wind_set(message[3],WF_BOTTOM,0,0,0,0);
			/* Continue with BOTTOM event processing */
		case WM_UNTOPPED:
			SDL_PrivateAppActive(0, SDL_APPINPUTFOCUS);
			if (VDI_setpalette) {
				VDI_setpalette(this, VDI_oldpalette);
			}
			break;
	}
	
	return quit;
}

static void do_keyboard(short kc, Uint32 tick)
{
	int scancode;

	if (kc) {
		scancode=(kc>>8) & (ATARIBIOS_MAXKEYS-1);
		gem_currentkeyboard[scancode]=0xFF;
		keyboard_ticks[scancode]=tick;
	}
}

static void do_keyboard_special(short ks, Uint32 tick)
{
	int scancode=0;

	/* Read special keys */
	if (ks & K_RSHIFT)
		scancode=SCANCODE_RIGHTSHIFT;
	if (ks & K_LSHIFT)
		scancode=SCANCODE_LEFTSHIFT;
	if (ks & K_CTRL)
		scancode=SCANCODE_LEFTCONTROL;
	if (ks & K_ALT)
		scancode=SCANCODE_LEFTALT;

	if (scancode) {
		gem_currentkeyboard[scancode]=0xFF;
		keyboard_ticks[scancode]=tick;
	}
}

static void do_mouse_motion(_THIS, short mx, short my)
{
	short x2, y2, w2, h2;

	if (this->input_grab == SDL_GRAB_OFF) {
		/* Switch mouse focus state */
		if (!GEM_fullscreen && (GEM_handle>=0)) {
			SDL_PrivateAppActive(
				mouse_in_work_area(GEM_handle, mx,my),
				SDL_APPMOUSEFOCUS);
		}
	}
	GEM_CheckMouseMode(this);

	/* Don't return mouse events if out of window */
	if ((SDL_GetAppState() & SDL_APPMOUSEFOCUS)==0) {
		return;
	}

	/* Relative mouse motion ? */
	if (GEM_mouse_relative) {
		if (GEM_usedevmouse) {
			SDL_AtariDevMouse_PostMouseEvents(this, SDL_FALSE);
		} else {
			SDL_AtariXbios_PostMouseEvents(this, SDL_FALSE);
		}
		return;
	}

	/* Retrieve window coords, and generate mouse events accordingly */
	x2 = y2 = 0;
	w2 = VDI_w;
	h2 = VDI_h;
	if ((!GEM_fullscreen) && (GEM_handle>=0)) {
		wind_get (GEM_handle, WF_WORKXYWH, &x2, &y2, &w2, &h2);
	}

	if ((prevmx!=mx) || (prevmy!=my)) {
		int posx, posy;

		/* Give mouse position relative to window position */
		posx = mx - x2;
		if (posx<0) posx = 0;
		if (posx>w2) posx = w2-1;
		posy = my - y2;
		if (posy<0) posy = 0;
		if (posy>h2) posy = h2-1;

		SDL_PrivateMouseMotion(0, 0, posx, posy);
	}

	prevmx = mx;
	prevmy = my;
}

static void do_mouse_buttons(_THIS, short mb)
{
	int i;

	/* Don't return mouse events if out of window */
	if ((SDL_GetAppState() & SDL_APPMOUSEFOCUS)==0)
		return;

	if (prevmb==mb)
		return;

	for (i=0;i<3;i++) {
		int curbutton, prevbutton;
		
		curbutton = mb & (1<<i);
		prevbutton = prevmb & (1<<i);
	
		if (curbutton && !prevbutton) {
			SDL_PrivateMouseButton(SDL_PRESSED, i+1, 0, 0);
		}
		if (!curbutton && prevbutton) {
			SDL_PrivateMouseButton(SDL_RELEASED, i+1, 0, 0);
		}
	}

	prevmb = mb;
}

/* Check if mouse in visible area of the window */
static int mouse_in_work_area(int winhandle, short mx, short my)
{
	short todo[4];
	short inside[4] = {mx,my,1,1};

	/* Browse the rectangle list */
	if (wind_get(winhandle, WF_FIRSTXYWH, &todo[0], &todo[1], &todo[2], &todo[3])!=0) {
		while (todo[2] && todo[3]) {
			if (rc_intersect((GRECT *)inside,(GRECT *)todo)) {
				return 1;
			}

			if (wind_get(winhandle, WF_NEXTXYWH, &todo[0], &todo[1], &todo[2], &todo[3])==0) {
				break;
			}
		}

	}

	return 0;
}

/* Clear key state for which we did not receive events for a while */

static void clearKeyboardState(Uint32 tick)
{
	int i;

	for (i=0; i<ATARIBIOS_MAXKEYS; i++) {
		if (keyboard_ticks[i]) {
			if (tick-keyboard_ticks[i] > KEY_PRESS_DURATION) {
				gem_currentkeyboard[i]=0;
				keyboard_ticks[i]=0;
			}
		}
	}
}
