/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2003  Sam Lantinga

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

#include "SDL_QuartzVideo.h"

#include <stdlib.h> // For getenv()
#include <IOKit/IOMessage.h> // For wake from sleep detection
#include <IOKit/pwr_mgt/IOPMLib.h> // For wake from sleep detection
#include "SDL_QuartzKeys.h"

void     QZ_InitOSKeymap (_THIS) {
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
    keymap[QZ_IBOOK_UP]      = SDLK_UP;
    keymap[QZ_IBOOK_LEFT] = SDLK_LEFT;

    /* 
        Up there we setup a static scancode->keysym map. However, it will not
        work very well on international keyboard. Hence we now query MacOS
        for its own keymap to adjust our own mapping table. However, this is
        basically only useful for ascii char keys. This is also the reason
        why we keep the static table, too.
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
            if (value >= 128)     /* Some non-ASCII char, map it to SDLK_WORLD_* */
                keymap[i] = world++;
            else if (value >= 32)     /* non-control ASCII char */
                keymap[i] = value;
        }
    }

    /* 
        The keypad codes are re-setup here, because the loop above cannot
        distinguish between a key on the keypad and a regular key. We maybe
        could get around this problem in another fashion: NSEvent's flags
        include a "NSNumericPadKeyMask" bit; we could check that and modify
        the symbol we return on the fly. However, this flag seems to exhibit
        some weird behaviour related to the num lock key
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

static void QZ_DoKey (_THIS, int state, NSEvent *event) {

    NSString *chars;
    unsigned int numChars;
    SDL_keysym key;
    
    /* 
        A key event can contain multiple characters,
        or no characters at all. In most cases, it
        will contain a single character. If it contains
        0 characters, we'll use 0 as the unicode. If it
        contains multiple characters, we'll use 0 as
        the scancode/keysym.
    */
    chars = [ event characters ];
    numChars = [ chars length ];

    if (numChars == 1) {

        key.scancode = [ event keyCode ];
        key.sym      = keymap [ key.scancode ];
        key.unicode  = [ chars characterAtIndex:0 ];
        key.mod      = KMOD_NONE;

        SDL_PrivateKeyboard (state, &key);
    }
    else if (numChars == 0) {
      
        key.scancode = [ event keyCode ];
        key.sym      = keymap [ key.scancode ];
        key.unicode  = 0;
        key.mod      = KMOD_NONE;

        SDL_PrivateKeyboard (state, &key);
    }
    else /* (numChars > 1) */ {
      
        int i;
        for (i = 0; i < numChars; i++) {

            key.scancode = 0;
            key.sym      = 0;
            key.unicode  = [ chars characterAtIndex:i];
            key.mod      = KMOD_NONE;

            SDL_PrivateKeyboard (state, &key);
        }
    }
    
    if (getenv ("SDL_ENABLEAPPEVENTS"))
        [ NSApp sendEvent:event ];
}

static void QZ_DoModifiers (_THIS, unsigned int newMods) {

    const int mapping[] = { SDLK_CAPSLOCK, SDLK_LSHIFT, SDLK_LCTRL, SDLK_LALT, SDLK_LMETA };

    int i;
    int bit;
    SDL_keysym key;
    
    if (current_mods == newMods)
    	return;

    key.scancode    = 0;
    key.sym         = SDLK_UNKNOWN;
    key.unicode     = 0;
    key.mod         = KMOD_NONE;

    /* Iterate through the bits, testing each against the current modifiers */
    for (i = 0, bit = NSAlphaShiftKeyMask; bit <= NSCommandKeyMask; bit <<= 1, ++i) {

        unsigned int currentMask, newMask;

        currentMask = current_mods & bit;
        newMask     = newMods & bit;

        if ( currentMask &&
             currentMask != newMask ) {     /* modifier up event */

             key.sym = mapping[i];
             /* If this was Caps Lock, we need some additional voodoo to make SDL happy */
             if (bit == NSAlphaShiftKeyMask)
                  SDL_PrivateKeyboard (SDL_PRESSED, &key);
             SDL_PrivateKeyboard (SDL_RELEASED, &key);
        }
        else if ( newMask &&
                  currentMask != newMask ) {     /* modifier down event */
        
             key.sym = mapping[i];
             SDL_PrivateKeyboard (SDL_PRESSED, &key);
             /* If this was Caps Lock, we need some additional voodoo to make SDL happy */
             if (bit == NSAlphaShiftKeyMask)
                  SDL_PrivateKeyboard (SDL_RELEASED, &key);
        }
    }

    current_mods = newMods;
}

static void QZ_GetMouseLocation (_THIS, NSPoint *p) {
    *p = [ NSEvent mouseLocation ]; /* global coordinates */
    if (qz_window)
        QZ_PrivateGlobalToLocal (this, p);
    QZ_PrivateCocoaToSDL (this, p);
}

static void QZ_DoActivate (_THIS)
{
    /* Hide the cursor if it was hidden by SDL_ShowCursor() */
    if (!cursor_should_be_visible)
        QZ_HideMouse (this);

    /* Regrab input, only if it was previously grabbed */
    if ( current_grab_mode == SDL_GRAB_ON ) {
        
        /* Restore cursor location if input was grabbed */
        QZ_PrivateWarpCursor (this, cursor_loc.x, cursor_loc.y);
        QZ_ChangeGrabState (this, QZ_ENABLE_GRAB);
    }
}

static void QZ_DoDeactivate (_THIS) {

    /* Get the current cursor location, for restore on activate */
    QZ_GetMouseLocation (this, &cursor_loc);
    
    /* Reassociate mouse and cursor */
    CGAssociateMouseAndMouseCursorPosition (1);

    /* Show the cursor if it was hidden by SDL_ShowCursor() */
    if (!cursor_should_be_visible)
        QZ_ShowMouse (this);
}

void QZ_SleepNotificationHandler (void * refcon,
                                  io_service_t service,
                                  natural_t messageType,
                                  void * messageArgument )
{
     SDL_VideoDevice *this = (SDL_VideoDevice*)refcon;
     
     switch(messageType)
     {
         case kIOMessageSystemWillSleep:
             IOAllowPowerChange(power_connection, (long) messageArgument);
             break;
         case kIOMessageCanSystemSleep:
             IOAllowPowerChange(power_connection, (long) messageArgument);
             break;
         case kIOMessageSystemHasPoweredOn:
            /* awake */
            SDL_PrivateExpose();
            break;
     }
}

void QZ_RegisterForSleepNotifications (_THIS)
{
     CFRunLoopSourceRef rls;
     IONotificationPortRef thePortRef;
     io_object_t notifier;

     power_connection = IORegisterForSystemPower (this, &thePortRef, QZ_SleepNotificationHandler, &notifier);

     if (power_connection == 0)
         NSLog(@"SDL: QZ_SleepNotificationHandler() IORegisterForSystemPower failed.");

     rls = IONotificationPortGetRunLoopSource (thePortRef);
     CFRunLoopAddSource (CFRunLoopGetCurrent(), rls, kCFRunLoopDefaultMode);
     CFRelease (rls);
}


// Try to map Quartz mouse buttons to SDL's lingo...
static int QZ_OtherMouseButtonToSDL(int button)
{
    switch (button)
    {
        case 0:
            return(SDL_BUTTON_LEFT);   // 1
        case 1:
            return(SDL_BUTTON_RIGHT);  // 3
        case 2:
            return(SDL_BUTTON_MIDDLE); // 2
    }

    // >= 3: skip 4 & 5, since those are the SDL mousewheel buttons.
    return(button + 3);
}


void QZ_PumpEvents (_THIS)
{
    int firstMouseEvent;
    CGMouseDelta dx, dy;

    NSDate *distantPast;
    NSEvent *event;
    NSRect winRect;
    NSAutoreleasePool *pool;

    /* Update activity every five seconds to prevent screensaver. --ryan. */
    static Uint32 screensaverTicks = 0;
    Uint32 nowTicks = SDL_GetTicks();
    if ((nowTicks - screensaverTicks) > 5000)
    {
        UpdateSystemActivity(UsrActivity);
        screensaverTicks = nowTicks;
    }

    pool = [ [ NSAutoreleasePool alloc ] init ];
    distantPast = [ NSDate distantPast ];

    winRect = NSMakeRect (0, 0, SDL_VideoSurface->w, SDL_VideoSurface->h);
    
    /* send the first mouse event in absolute coordinates */
    firstMouseEvent = 1;
    
    /* accumulate any additional mouse moved events into one SDL mouse event */
    dx = 0;
    dy = 0;
    
    do {
    
        /* Poll for an event. This will not block */
        event = [ NSApp nextEventMatchingMask:NSAnyEventMask
                                    untilDate:distantPast
                                    inMode: NSDefaultRunLoopMode dequeue:YES ];
        if (event != nil) {

            int button;
            unsigned int type;
            BOOL isInGameWin;
            
            #define DO_MOUSE_DOWN(button) do {                                               \
                            if ( [ NSApp isActive ] ) {                                      \
                                if ( isInGameWin ) {                                         \
                                    SDL_PrivateMouseButton (SDL_PRESSED, button, 0, 0);      \
                                    expect_mouse_up |= 1<<button;                            \
                                }                                                            \
                            }                                                                \
                            else {                                                           \
                                QZ_DoActivate (this);                                        \
                            }                                                                \
                            [ NSApp sendEvent:event ];                                       \
            } while(0)
            
            #define DO_MOUSE_UP(button) do {                                            \
                            if ( expect_mouse_up & (1<<button) ) {                      \
                                SDL_PrivateMouseButton (SDL_RELEASED, button, 0, 0);    \
                                expect_mouse_up &= ~(1<<button);                        \
                            }                                                           \
                            [ NSApp sendEvent:event ];                                  \
            } while(0)
            
            type = [ event type ];
            isInGameWin = QZ_IsMouseInWindow (this);

            QZ_DoModifiers(this, [ event modifierFlags ] );

            switch (type) {
                case NSLeftMouseDown:
                    if ( getenv("SDL_HAS3BUTTONMOUSE") ) {
                        DO_MOUSE_DOWN (SDL_BUTTON_LEFT);
                    } else {
                        if ( NSCommandKeyMask & current_mods ) {
                            last_virtual_button = SDL_BUTTON_RIGHT;
                            DO_MOUSE_DOWN (SDL_BUTTON_RIGHT);
                        }
                        else if ( NSAlternateKeyMask & current_mods ) {
                            last_virtual_button = SDL_BUTTON_MIDDLE;
                            DO_MOUSE_DOWN (SDL_BUTTON_MIDDLE);
                        }
                        else {
                            DO_MOUSE_DOWN (SDL_BUTTON_LEFT);
                        }
                    }
                    break;

                case NSLeftMouseUp:
                    if ( last_virtual_button != 0 ) {
                        DO_MOUSE_UP (last_virtual_button);
                        last_virtual_button = 0;
                    }
                    else {
                        DO_MOUSE_UP (SDL_BUTTON_LEFT);
                    }
                    break;

                case NSOtherMouseDown:
                case NSRightMouseDown:
                    button = QZ_OtherMouseButtonToSDL([ event buttonNumber ]);
                    DO_MOUSE_DOWN (button);
                    break;

                case NSOtherMouseUp:
                case NSRightMouseUp:
                    button = QZ_OtherMouseButtonToSDL([ event buttonNumber ]);
                    DO_MOUSE_UP (button);
                    break;

                case NSSystemDefined:
                    /*
                        Future: up to 32 "mouse" buttons can be handled.
                        if ([event subtype] == 7) {
                            unsigned int buttons;
                            buttons = [ event data2 ];
                    */
                    break;
                case NSLeftMouseDragged:
                case NSRightMouseDragged:
                case NSOtherMouseDragged: /* usually middle mouse dragged */
                case NSMouseMoved:
                    if ( grab_state == QZ_INVISIBLE_GRAB ) {
                
                        /*
                            If input is grabbed+hidden, the cursor doesn't move,
                            so we have to call the lowlevel window server
                            function. This is less accurate but works OK.                         
                        */
                        CGMouseDelta dx1, dy1;
                        CGGetLastMouseDelta (&dx1, &dy1);
                        dx += dx1;
                        dy += dy1;
                    }
                    else if (firstMouseEvent) {
                        
                        /*
                            Get the first mouse event in a possible
                            sequence of mouse moved events. Since we
                            use absolute coordinates, this serves to
                            compensate any inaccuracy in deltas, and
                            provides the first known mouse position,
                            since everything after this uses deltas
                        */
                        NSPoint p;
                        QZ_GetMouseLocation (this, &p);
                        SDL_PrivateMouseMotion (0, 0, p.x, p.y);
                        firstMouseEvent = 0;
                   }
                    else {
                    
                        /*
                            Get the amount moved since the last drag or move event,
                            add it on for one big move event at the end.
                        */
                        dx += [ event deltaX ];
                        dy += [ event deltaY ];
                    }
                    
                    /* 
                        Handle grab input+cursor visible by warping the cursor back
                        into the game window. This still generates a mouse moved event,
                        but not as a result of the warp (so it's in the right direction).
                    */
                    if ( grab_state == QZ_VISIBLE_GRAB &&
                         !isInGameWin ) {
                       
                        NSPoint p;
                        QZ_GetMouseLocation (this, &p);

                        if ( p.x < 0.0 ) 
                            p.x = 0.0;
                        
                        if ( p.y < 0.0 ) 
                            p.y = 0.0;
                        
                        if ( p.x >= winRect.size.width ) 
                            p.x = winRect.size.width-1;
                        
                        if ( p.y >= winRect.size.height ) 
                            p.y = winRect.size.height-1;
                        
                        QZ_PrivateWarpCursor (this, p.x, p.y);
                    }
                    else
                    if ( !isInGameWin && (SDL_GetAppState() & SDL_APPMOUSEFOCUS) ) {
                    
                        SDL_PrivateAppActive (0, SDL_APPMOUSEFOCUS);
                        if (!cursor_should_be_visible)
                            QZ_ShowMouse (this);
                    }
                    else
                    if ( isInGameWin && !(SDL_GetAppState() & SDL_APPMOUSEFOCUS) ) {
                    
                        SDL_PrivateAppActive (1, SDL_APPMOUSEFOCUS);
                        if (!cursor_should_be_visible)
                            QZ_HideMouse (this);
                    }
                    break;
                case NSScrollWheel:
                    if ( isInGameWin ) {
                        float dy;
                        Uint8 button;
                        dy = [ event deltaY ];
                        if ( dy > 0.0 ) /* Scroll up */
                            button = SDL_BUTTON_WHEELUP;
                        else /* Scroll down */
                            button = SDL_BUTTON_WHEELDOWN;
                        /* For now, wheel is sent as a quick down+up */
                        SDL_PrivateMouseButton (SDL_PRESSED, button, 0, 0);
                        SDL_PrivateMouseButton (SDL_RELEASED, button, 0, 0);
                    }
                    break;
                case NSKeyUp:
                    QZ_DoKey (this, SDL_RELEASED, event);
                    break;
                case NSKeyDown:
                    QZ_DoKey (this, SDL_PRESSED, event);
                    break;
                case NSFlagsChanged:
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
    
    /* handle accumulated mouse moved events */
    if (dx != 0 || dy != 0)
        SDL_PrivateMouseMotion (0, 1, dx, dy);
    
    [ pool release ];
}
