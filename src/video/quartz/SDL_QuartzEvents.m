/*
	SDL - Simple DirectMedia Layer
	Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002	Sam Lantinga

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
#include <sys/time.h>

#include "SDL_QuartzKeys.h"

static SDLKey keymap[256];
static unsigned int currentMods = 0; /* Current keyboard modifiers, to track modifier state */
static int last_virtual_button = 0; /* Last virtual mouse button pressed */

static void	 QZ_InitOSKeymap (_THIS) {
	const void *KCHRPtr;
	UInt32 state;
	UInt32 value;
	int i;
	int world = SDLK_WORLD_0;
	
	for ( i=0; i<SDL_TABLESIZE(keymap); ++i )
		keymap[i] = SDLK_UNKNOWN;

	/* This keymap is almost exactly the same as the OS 9 one */
	keymap[QZ_ESCAPE] = SDLK_ESCAPE;
	keymap[QZ_F1] = SDLK_F1;
	keymap[QZ_F2] = SDLK_F2;
	keymap[QZ_F3] = SDLK_F3;
	keymap[QZ_F4] = SDLK_F4;
	keymap[QZ_F5] = SDLK_F5;
	keymap[QZ_F6] = SDLK_F6;
	keymap[QZ_F7] = SDLK_F7;
	keymap[QZ_F8] = SDLK_F8;
	keymap[QZ_F9] = SDLK_F9;
	keymap[QZ_F10] = SDLK_F10;
	keymap[QZ_F11] = SDLK_F11;
	keymap[QZ_F12] = SDLK_F12;
	keymap[QZ_PRINT] = SDLK_PRINT;
	keymap[QZ_SCROLLOCK] = SDLK_SCROLLOCK;
	keymap[QZ_PAUSE] = SDLK_PAUSE;
	keymap[QZ_POWER] = SDLK_POWER;
	keymap[QZ_BACKQUOTE] = SDLK_BACKQUOTE;
	keymap[QZ_1] = SDLK_1;
	keymap[QZ_2] = SDLK_2;
	keymap[QZ_3] = SDLK_3;
	keymap[QZ_4] = SDLK_4;
	keymap[QZ_5] = SDLK_5;
	keymap[QZ_6] = SDLK_6;
	keymap[QZ_7] = SDLK_7;
	keymap[QZ_8] = SDLK_8;
	keymap[QZ_9] = SDLK_9;
	keymap[QZ_0] = SDLK_0;
	keymap[QZ_MINUS] = SDLK_MINUS;
	keymap[QZ_EQUALS] = SDLK_EQUALS;
	keymap[QZ_BACKSPACE] = SDLK_BACKSPACE;
	keymap[QZ_INSERT] = SDLK_INSERT;
	keymap[QZ_HOME] = SDLK_HOME;
	keymap[QZ_PAGEUP] = SDLK_PAGEUP;
	keymap[QZ_NUMLOCK] = SDLK_NUMLOCK;
	keymap[QZ_KP_EQUALS] = SDLK_KP_EQUALS;
	keymap[QZ_KP_DIVIDE] = SDLK_KP_DIVIDE;
	keymap[QZ_KP_MULTIPLY] = SDLK_KP_MULTIPLY;
	keymap[QZ_TAB] = SDLK_TAB;
	keymap[QZ_q] = SDLK_q;
	keymap[QZ_w] = SDLK_w;
	keymap[QZ_e] = SDLK_e;
	keymap[QZ_r] = SDLK_r;
	keymap[QZ_t] = SDLK_t;
	keymap[QZ_y] = SDLK_y;
	keymap[QZ_u] = SDLK_u;
	keymap[QZ_i] = SDLK_i;
	keymap[QZ_o] = SDLK_o;
	keymap[QZ_p] = SDLK_p;
	keymap[QZ_LEFTBRACKET] = SDLK_LEFTBRACKET;
	keymap[QZ_RIGHTBRACKET] = SDLK_RIGHTBRACKET;
	keymap[QZ_BACKSLASH] = SDLK_BACKSLASH;
	keymap[QZ_DELETE] = SDLK_DELETE;
	keymap[QZ_END] = SDLK_END;
	keymap[QZ_PAGEDOWN] = SDLK_PAGEDOWN;
	keymap[QZ_KP7] = SDLK_KP7;
	keymap[QZ_KP8] = SDLK_KP8;
	keymap[QZ_KP9] = SDLK_KP9;
	keymap[QZ_KP_MINUS] = SDLK_KP_MINUS;
	keymap[QZ_CAPSLOCK] = SDLK_CAPSLOCK;
	keymap[QZ_a] = SDLK_a;
	keymap[QZ_s] = SDLK_s;
	keymap[QZ_d] = SDLK_d;
	keymap[QZ_f] = SDLK_f;
	keymap[QZ_g] = SDLK_g;
	keymap[QZ_h] = SDLK_h;
	keymap[QZ_j] = SDLK_j;
	keymap[QZ_k] = SDLK_k;
	keymap[QZ_l] = SDLK_l;
	keymap[QZ_SEMICOLON] = SDLK_SEMICOLON;
	keymap[QZ_QUOTE] = SDLK_QUOTE;
	keymap[QZ_RETURN] = SDLK_RETURN;
	keymap[QZ_KP4] = SDLK_KP4;
	keymap[QZ_KP5] = SDLK_KP5;
	keymap[QZ_KP6] = SDLK_KP6;
	keymap[QZ_KP_PLUS] = SDLK_KP_PLUS;
	keymap[QZ_LSHIFT] = SDLK_LSHIFT;
	keymap[QZ_z] = SDLK_z;
	keymap[QZ_x] = SDLK_x;
	keymap[QZ_c] = SDLK_c;
	keymap[QZ_v] = SDLK_v;
	keymap[QZ_b] = SDLK_b;
	keymap[QZ_n] = SDLK_n;
	keymap[QZ_m] = SDLK_m;
	keymap[QZ_COMMA] = SDLK_COMMA;
	keymap[QZ_PERIOD] = SDLK_PERIOD;
	keymap[QZ_SLASH] = SDLK_SLASH;
	keymap[QZ_UP] = SDLK_UP;
	keymap[QZ_KP1] = SDLK_KP1;
	keymap[QZ_KP2] = SDLK_KP2;
	keymap[QZ_KP3] = SDLK_KP3;
	keymap[QZ_KP_ENTER] = SDLK_KP_ENTER;
	keymap[QZ_LCTRL] = SDLK_LCTRL;
	keymap[QZ_LALT] = SDLK_LALT;
	keymap[QZ_LMETA] = SDLK_LMETA;
	keymap[QZ_SPACE] = SDLK_SPACE;
	keymap[QZ_LEFT] = SDLK_LEFT;
	keymap[QZ_DOWN] = SDLK_DOWN;
	keymap[QZ_RIGHT] = SDLK_RIGHT;
	keymap[QZ_KP0] = SDLK_KP0;
	keymap[QZ_KP_PERIOD] = SDLK_KP_PERIOD;
	keymap[QZ_IBOOK_ENTER] = SDLK_KP_ENTER;
	keymap[QZ_IBOOK_RIGHT] = SDLK_RIGHT;
	keymap[QZ_IBOOK_DOWN] = SDLK_DOWN;
	keymap[QZ_IBOOK_UP]	  = SDLK_UP;
	keymap[QZ_IBOOK_LEFT] = SDLK_LEFT;
	
	/* Up there we setup a static scancode->keysym map. However, it will not
	 * work very well on international keyboard. Hence we now query MacOS
	 * for its own keymap to adjust our own mapping table. However, this is
	 * bascially only useful for ascii char keys. This is also the reason
	 * why we keep the static table, too.
	 */
	
	/* Get a pointer to the systems cached KCHR */
	KCHRPtr = (void *)GetScriptManagerVariable(smKCHRCache);
	if (KCHRPtr)
	{
		/* Loop over all 127 possible scan codes */
		for (i = 0; i < 0x7F; i++)
		{
			/* We pretend a clean start to begin with (i.e. no dead keys active */
			state = 0;
			
			/* Now translate the key code to a key value */
			value = KeyTranslate(KCHRPtr, i, &state) & 0xff;
			
			/* If the state become 0, it was a dead key. We need to translate again,
			passing in the new state, to get the actual key value */
			if (state != 0)
				value = KeyTranslate(KCHRPtr, i, &state) & 0xff;
			
			/* Now we should have an ascii value, or 0. Try to figure out to which SDL symbol it maps */
			if (value >= 128)	 /* Some non-ASCII char, map it to SDLK_WORLD_* */
				keymap[i] = world++;
			else if (value >= 32)	 /* non-control ASCII char */
				keymap[i] = value;
		}
	}
	
	/* The keypad codes are re-setup here, because the loop above cannot
	 * distinguish between a key on the keypad and a regular key. We maybe
	 * could get around this problem in another fashion: NSEvent's flags
	 * include a "NSNumericPadKeyMask" bit; we could check that and modify
	 * the symbol we return on the fly. However, this flag seems to exhibit
	 * some weird behaviour related to the num lock key
	 */
	keymap[QZ_KP0] = SDLK_KP0;
	keymap[QZ_KP1] = SDLK_KP1;
	keymap[QZ_KP2] = SDLK_KP2;
	keymap[QZ_KP3] = SDLK_KP3;
	keymap[QZ_KP4] = SDLK_KP4;
	keymap[QZ_KP5] = SDLK_KP5;
	keymap[QZ_KP6] = SDLK_KP6;
	keymap[QZ_KP7] = SDLK_KP7;
	keymap[QZ_KP8] = SDLK_KP8;
	keymap[QZ_KP9] = SDLK_KP9;
	keymap[QZ_KP_MINUS] = SDLK_KP_MINUS;
	keymap[QZ_KP_PLUS] = SDLK_KP_PLUS;
	keymap[QZ_KP_PERIOD] = SDLK_KP_PERIOD;
	keymap[QZ_KP_EQUALS] = SDLK_KP_EQUALS;
	keymap[QZ_KP_DIVIDE] = SDLK_KP_DIVIDE;
	keymap[QZ_KP_MULTIPLY] = SDLK_KP_MULTIPLY;
	keymap[QZ_KP_ENTER] = SDLK_KP_ENTER;
}

static void QZ_DoKey (int state, NSEvent *event) {

	NSString *chars;
	int i;
	SDL_keysym key;
	
	/* An event can contain multiple characters */
	/* I'll ignore this fact for now, since there is only one virtual key code per event */
	chars = [ event characters ];
	for (i =0; i < 1 /*[ chars length ] */; i++) {
				
		key.scancode = [ event keyCode ];
		key.sym		 = keymap [ key.scancode ];
		key.unicode	 = [ chars characterAtIndex:i];
		key.mod		 = KMOD_NONE;
					
		SDL_PrivateKeyboard (state, &key);
	}
}

static void QZ_DoModifiers (unsigned int newMods) {

	const int mapping[] = { SDLK_CAPSLOCK, SDLK_LSHIFT, SDLK_LCTRL, SDLK_LALT, SDLK_LMETA } ;

	int i;
	int bit;
	SDL_keysym key;

	key.scancode = 0;
	key.sym		 = SDLK_UNKNOWN;
	key.unicode	 = 0;
	key.mod		 = KMOD_NONE;
	
	/* Iterate through the bits, testing each against the current modifiers */
	for (i = 0, bit = NSAlphaShiftKeyMask; bit <= NSCommandKeyMask; bit <<= 1, ++i) {
	
		unsigned int currentMask, newMask;
		
		currentMask = currentMods & bit;
		newMask		= newMods & bit;
		
		if ( currentMask &&
			 currentMask != newMask ) {	 /* modifier up event */

			key.sym = mapping[i];
			/* If this was Caps Lock, we need some additional voodoo to make SDL happy */
			if (bit == NSAlphaShiftKeyMask)
				SDL_PrivateKeyboard (SDL_PRESSED, &key);
			SDL_PrivateKeyboard (SDL_RELEASED, &key);
		}
		else
		if ( newMask &&
			 currentMask != newMask ) {	 /* modifier down event */
		
			key.sym = mapping[i];
			SDL_PrivateKeyboard (SDL_PRESSED, &key);
			/* If this was Caps Lock, we need some additional voodoo to make SDL happy */
			if (bit == NSAlphaShiftKeyMask)
				SDL_PrivateKeyboard (SDL_RELEASED, &key);
		}
	}
	
	currentMods = newMods;
}

static void QZ_DoActivate (_THIS)
{
	inForeground = YES;

	/* Regrab the mouse */
	if (currentGrabMode == SDL_GRAB_ON) {
		QZ_WarpWMCursor (this, SDL_VideoSurface->w / 2, SDL_VideoSurface->h / 2);
		CGAssociateMouseAndMouseCursorPosition (0);
	}

	/* Hide the mouse cursor if inside the app window */
	if (!QZ_cursor_visible) {
		HideCursor ();
	}
	
	SDL_PrivateAppActive (1, SDL_APPINPUTFOCUS);
}

static void QZ_DoDeactivate (_THIS) {
	
	inForeground = NO;

	/* Ungrab mouse if it is grabbed */
	if (currentGrabMode == SDL_GRAB_ON) {
		CGAssociateMouseAndMouseCursorPosition (1);
	}

	/* Show the mouse cursor */
	if (!QZ_cursor_visible) {
		ShowCursor ();
	}
	
	SDL_PrivateAppActive (0, SDL_APPINPUTFOCUS);
}

static void QZ_PumpEvents (_THIS)
{
	
        static NSPoint lastMouse;
        NSPoint mouse, saveMouse;
        Point qdMouse;
        CGMouseDelta dx, dy;
        
        NSDate *distantPast;
	NSEvent *event;
	NSRect winRect;
	NSRect titleBarRect;
	NSAutoreleasePool *pool;
	
	pool = [ [ NSAutoreleasePool alloc ] init ];
	distantPast = [ NSDate distantPast ];
	
	winRect = NSMakeRect (0, 0, SDL_VideoSurface->w, SDL_VideoSurface->h);
	titleBarRect = NSMakeRect ( 0, SDL_VideoSurface->h, SDL_VideoSurface->w,
		SDL_VideoSurface->h + 22 );
	
        if (currentGrabMode != SDL_GRAB_ON) { /* if grabbed, the cursor can't move! (see fallback below) */
        
            /* 1/2 second after a warp, the mouse cannot move (don't ask me why) */
            /* So, approximate motion with CGGetLastMouseDelta, which still works, somehow */
            if (! warp_flag) {
            
                GetGlobalMouse (&qdMouse);  /* use Carbon since [ NSEvent mouseLocation ] is broken */
                mouse = NSMakePoint (qdMouse.h, qdMouse.v);
                saveMouse = mouse;
                
                if (mouse.x != lastMouse.x || mouse.y != lastMouse.y) {
                
                    QZ_PrivateCGToSDL (this, &mouse);
                    if (inForeground && NSPointInRect (mouse, winRect)) {
                        //printf ("Mouse Loc: (%f, %f)\n", mouse.x, mouse.y);
                        SDL_PrivateMouseMotion (0, 0, mouse.x, mouse.y);
                    }
                }
                lastMouse = saveMouse;
            }
        }
        
        /* accumulate any mouse events into one SDL mouse event */
        dx = 0;
        dy = 0;
        
	do {
	
		/* Poll for an event. This will not block */
		event = [ NSApp nextEventMatchingMask:NSAnyEventMask
					untilDate:distantPast
					inMode: NSDefaultRunLoopMode dequeue:YES ];
	
		if (event != nil) {
			unsigned int type;
			BOOL isForGameWin;

			#define DO_MOUSE_DOWN(button, sendToWindow) do {				 \
				if ( inForeground ) {			                                 \
					if ( (SDL_VideoSurface->flags & SDL_FULLSCREEN) ||		 \
						 NSPointInRect([event locationInWindow], winRect) )	 \
						SDL_PrivateMouseButton (SDL_PRESSED, button, 0, 0);	 \
				}                                                                        \
				else {									 \
					QZ_DoActivate (this);                                            \
				}									 \
				[ NSApp sendEvent:event ];			                         \
				} while(0)
				
			#define DO_MOUSE_UP(button, sendToWindow) do {					 \
				if ( (SDL_VideoSurface->flags & SDL_FULLSCREEN) ||			 \
					 !NSPointInRect([event locationInWindow], titleBarRect) )        \
					SDL_PrivateMouseButton (SDL_RELEASED, button, 0, 0);	         \
				[ NSApp sendEvent:event ];						 \
				} while(0)

			type = [ event type ];
			isForGameWin = (qz_window == [ event window ]);
			switch (type) {
			
			case NSLeftMouseDown:
				if ( NSCommandKeyMask & currentMods ) {
					last_virtual_button = 3;
					DO_MOUSE_DOWN (3, 0);
				}
				else if ( NSAlternateKeyMask & currentMods ) {
					last_virtual_button = 2;
					DO_MOUSE_DOWN (2, 0);
				}
				else {
					DO_MOUSE_DOWN (1, 1);
				}
				break;
			case NSOtherMouseDown: DO_MOUSE_DOWN (2, 0); break;
			case NSRightMouseDown: DO_MOUSE_DOWN (3, 0); break;	
			case NSLeftMouseUp:
			
				if ( last_virtual_button != 0 ) {
					DO_MOUSE_UP (last_virtual_button, 0);
					last_virtual_button = 0;
				}
				else {
					DO_MOUSE_UP (1, 1);
				}
				break;
			case NSOtherMouseUp:   DO_MOUSE_UP (2, 0);	 break;
			case NSRightMouseUp:   DO_MOUSE_UP (3, 0);	 break;
			case NSSystemDefined:
				//if ([event subtype] == 7) {
				//	  unsigned int buttons;	  // up to 32 mouse button states!
				//	  buttons = [ event data2 ];
				//}
				break;
			case NSLeftMouseDragged:
			case NSRightMouseDragged:
			case 27:
			case NSMouseMoved:
                            
                                if (currentGrabMode == SDL_GRAB_ON) { 
                                    
                                    /** 
                                     *  If input is grabbed, we'll wing it and try to send some mouse
                                     *  moved events with CGGetLastMouseDelta(). Not optimal, but better
                                     *  than nothing.
                                     **/ 
                                     CGMouseDelta dx1, dy1;
                                     CGGetLastMouseDelta (&dx1, &dy1);
                                     dx += dx1;
                                     dy += dy1;
                                }
                                else if (warp_flag) {
                                
                                    Uint32 ticks;
                
                                    ticks = SDL_GetTicks();
                                    if (ticks - warp_ticks < 150) {
                                    
                                        CGMouseDelta dx1, dy1;
                                        CGGetLastMouseDelta (&dx1, &dy1);
                                        dx += dx1;
                                        dy += dy1;
                                    }
                                    else {
                                        
                                        warp_flag = 0;
                                    }
                                }
                                
				break;
			case NSScrollWheel:
				{
					if (NSPointInRect([ event locationInWindow ], winRect)) {
						float dy;
						dy = [ event deltaY ];
						if ( dy > 0.0 ) /* Scroll up */
							SDL_PrivateMouseButton (SDL_PRESSED, 4, 0, 0);
						else /* Scroll down */
							SDL_PrivateMouseButton (SDL_PRESSED, 5, 0, 0);
					}
				}
				break;
			case NSKeyUp:
				QZ_DoKey (SDL_RELEASED, event);
				break;
			case NSKeyDown:
				QZ_DoKey (SDL_PRESSED, event);
				break;
			case NSFlagsChanged:
				QZ_DoModifiers( [ event modifierFlags ] );
				break;
			case NSAppKitDefined:
				switch ( [ event subtype ] ) {
				case NSApplicationActivatedEventType:
					QZ_DoActivate (this);
					break;
				case NSApplicationDeactivatedEventType:
					QZ_DoDeactivate (this);
					break;
				}
				[ NSApp sendEvent:event ];
				break;
			/* case NSApplicationDefined: break; */
			/* case NSPeriodic: break; */
			/* case NSCursorUpdate: break; */
                        
                        default:
				[ NSApp sendEvent:event ];
			}
		}
	  } while (event != nil);
	
          /* check for accumulated mouse events */
          if (dx != 0 || dy != 0)
            SDL_PrivateMouseMotion (0, 1, dx, dy);
        
	  [ pool release ];
}

