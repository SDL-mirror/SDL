/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

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

#include "SDL_cocoavideo.h"
#include "SDL_cocoakeys.h"

#include "../../events/SDL_keyboard_c.h"


#ifndef NX_DEVICERCTLKEYMASK
    #define NX_DEVICELCTLKEYMASK    0x00000001
#endif
#ifndef NX_DEVICELSHIFTKEYMASK
    #define NX_DEVICELSHIFTKEYMASK  0x00000002
#endif
#ifndef NX_DEVICERSHIFTKEYMASK
    #define NX_DEVICERSHIFTKEYMASK  0x00000004
#endif
#ifndef NX_DEVICELCMDKEYMASK
    #define NX_DEVICELCMDKEYMASK    0x00000008
#endif
#ifndef NX_DEVICERCMDKEYMASK
    #define NX_DEVICERCMDKEYMASK    0x00000010
#endif
#ifndef NX_DEVICELALTKEYMASK
    #define NX_DEVICELALTKEYMASK    0x00000020
#endif
#ifndef NX_DEVICERALTKEYMASK
    #define NX_DEVICERALTKEYMASK    0x00000040
#endif
#ifndef NX_DEVICERCTLKEYMASK
    #define NX_DEVICERCTLKEYMASK    0x00002000
#endif

static void
InitKeymap (SDLKey *keymap)
{
    const void *KCHRPtr;
    UInt32 state;
    UInt32 value;
    int i;

    for ( i=0; i<256; ++i )
        keymap[i] = SDLK_UNKNOWN;

    /* This keymap is almost exactly the same as the OS 9 one */
    keymap[KEY_ESCAPE] = SDLK_ESCAPE;
    keymap[KEY_F1] = SDLK_F1;
    keymap[KEY_F2] = SDLK_F2;
    keymap[KEY_F3] = SDLK_F3;
    keymap[KEY_F4] = SDLK_F4;
    keymap[KEY_F5] = SDLK_F5;
    keymap[KEY_F6] = SDLK_F6;
    keymap[KEY_F7] = SDLK_F7;
    keymap[KEY_F8] = SDLK_F8;
    keymap[KEY_F9] = SDLK_F9;
    keymap[KEY_F10] = SDLK_F10;
    keymap[KEY_F11] = SDLK_F11;
    keymap[KEY_F12] = SDLK_F12;
    keymap[KEY_F13] = SDLK_F13;
    keymap[KEY_F14] = SDLK_F14;
    keymap[KEY_F15] = SDLK_F15;
/*
    keymap[KEY_PRINT] = SDLK_PRINT;
    keymap[KEY_SCROLLOCK] = SDLK_SCROLLOCK;
    keymap[KEY_PAUSE] = SDLK_PAUSE;
*/
    keymap[KEY_POWER] = SDLK_POWER;
    keymap[KEY_BACKQUOTE] = SDLK_BACKQUOTE;
    keymap[KEY_1] = SDLK_1;
    keymap[KEY_2] = SDLK_2;
    keymap[KEY_3] = SDLK_3;
    keymap[KEY_4] = SDLK_4;
    keymap[KEY_5] = SDLK_5;
    keymap[KEY_6] = SDLK_6;
    keymap[KEY_7] = SDLK_7;
    keymap[KEY_8] = SDLK_8;
    keymap[KEY_9] = SDLK_9;
    keymap[KEY_0] = SDLK_0;
    keymap[KEY_MINUS] = SDLK_MINUS;
    keymap[KEY_EQUALS] = SDLK_EQUALS;
    keymap[KEY_BACKSPACE] = SDLK_BACKSPACE;
    keymap[KEY_INSERT] = SDLK_INSERT;
    keymap[KEY_HOME] = SDLK_HOME;
    keymap[KEY_PAGEUP] = SDLK_PAGEUP;
    keymap[KEY_NUMLOCK] = SDLK_NUMLOCK;
    keymap[KEY_KP_EQUALS] = SDLK_KP_EQUALS;
    keymap[KEY_KP_DIVIDE] = SDLK_KP_DIVIDE;
    keymap[KEY_KP_MULTIPLY] = SDLK_KP_MULTIPLY;
    keymap[KEY_TAB] = SDLK_TAB;
    keymap[KEY_q] = SDLK_q;
    keymap[KEY_w] = SDLK_w;
    keymap[KEY_e] = SDLK_e;
    keymap[KEY_r] = SDLK_r;
    keymap[KEY_t] = SDLK_t;
    keymap[KEY_y] = SDLK_y;
    keymap[KEY_u] = SDLK_u;
    keymap[KEY_i] = SDLK_i;
    keymap[KEY_o] = SDLK_o;
    keymap[KEY_p] = SDLK_p;
    keymap[KEY_LEFTBRACKET] = SDLK_LEFTBRACKET;
    keymap[KEY_RIGHTBRACKET] = SDLK_RIGHTBRACKET;
    keymap[KEY_BACKSLASH] = SDLK_BACKSLASH;
    keymap[KEY_DELETE] = SDLK_DELETE;
    keymap[KEY_END] = SDLK_END;
    keymap[KEY_PAGEDOWN] = SDLK_PAGEDOWN;
    keymap[KEY_KP7] = SDLK_KP7;
    keymap[KEY_KP8] = SDLK_KP8;
    keymap[KEY_KP9] = SDLK_KP9;
    keymap[KEY_KP_MINUS] = SDLK_KP_MINUS;
    keymap[KEY_CAPSLOCK] = SDLK_CAPSLOCK;
    keymap[KEY_a] = SDLK_a;
    keymap[KEY_s] = SDLK_s;
    keymap[KEY_d] = SDLK_d;
    keymap[KEY_f] = SDLK_f;
    keymap[KEY_g] = SDLK_g;
    keymap[KEY_h] = SDLK_h;
    keymap[KEY_j] = SDLK_j;
    keymap[KEY_k] = SDLK_k;
    keymap[KEY_l] = SDLK_l;
    keymap[KEY_SEMICOLON] = SDLK_SEMICOLON;
    keymap[KEY_QUOTE] = SDLK_QUOTE;
    keymap[KEY_RETURN] = SDLK_RETURN;
    keymap[KEY_KP4] = SDLK_KP4;
    keymap[KEY_KP5] = SDLK_KP5;
    keymap[KEY_KP6] = SDLK_KP6;
    keymap[KEY_KP_PLUS] = SDLK_KP_PLUS;
    keymap[KEY_LSHIFT] = SDLK_LSHIFT;
    keymap[KEY_RSHIFT] = SDLK_RSHIFT;
    keymap[KEY_z] = SDLK_z;
    keymap[KEY_x] = SDLK_x;
    keymap[KEY_c] = SDLK_c;
    keymap[KEY_v] = SDLK_v;
    keymap[KEY_b] = SDLK_b;
    keymap[KEY_n] = SDLK_n;
    keymap[KEY_m] = SDLK_m;
    keymap[KEY_COMMA] = SDLK_COMMA;
    keymap[KEY_PERIOD] = SDLK_PERIOD;
    keymap[KEY_SLASH] = SDLK_SLASH;
    keymap[KEY_UP] = SDLK_UP;
    keymap[KEY_KP1] = SDLK_KP1;
    keymap[KEY_KP2] = SDLK_KP2;
    keymap[KEY_KP3] = SDLK_KP3;
    keymap[KEY_KP_ENTER] = SDLK_KP_ENTER;
    keymap[KEY_LCTRL] = SDLK_LCTRL;
    keymap[KEY_LALT] = SDLK_LALT;
    keymap[KEY_LMETA] = SDLK_LMETA;
    keymap[KEY_RCTRL] = SDLK_RCTRL;
    keymap[KEY_RALT] = SDLK_RALT;
    keymap[KEY_RMETA] = SDLK_RMETA;
    keymap[KEY_SPACE] = SDLK_SPACE;
    keymap[KEY_LEFT] = SDLK_LEFT;
    keymap[KEY_DOWN] = SDLK_DOWN;
    keymap[KEY_RIGHT] = SDLK_RIGHT;
    keymap[KEY_KP0] = SDLK_KP0;
    keymap[KEY_KP_PERIOD] = SDLK_KP_PERIOD;
    keymap[KEY_IBOOK_ENTER] = SDLK_KP_ENTER;
    keymap[KEY_IBOOK_RIGHT] = SDLK_RIGHT;
    keymap[KEY_IBOOK_DOWN] = SDLK_DOWN;
    keymap[KEY_IBOOK_UP]      = SDLK_UP;
    keymap[KEY_IBOOK_LEFT] = SDLK_LEFT;

    /* 
        Up there we setup a static scancode->keysym map. However, it will not
        work very well on international keyboard. Hence we now query Mac OS X
        for its own keymap to adjust our own mapping table. However, this is
        basically only useful for ascii char keys. This is also the reason
        why we keep the static table, too.
     */

    /* Get a pointer to the systems cached KCHR */
    KCHRPtr = (void *)GetScriptManagerVariable(smKCHRCache);
    if (KCHRPtr) {
        /* Loop over all 127 possible scan codes */
        for (i = 0; i < 0x7F; i++) {
            /* We pretend a clean start to begin with (i.e. no dead keys active */
            state = 0;

            /* Now translate the key code to a key value */
            value = KeyTranslate(KCHRPtr, i, &state) & 0xff;

            /* If the state become 0, it was a dead key. We need to translate again,
                passing in the new state, to get the actual key value */
            if (state != 0)
                value = KeyTranslate(KCHRPtr, i, &state) & 0xff;

            /* Now we should have a latin1 value, which are SDL keysyms */
            if (value >= 32 && value <= 255) {
                keymap[i] = value;
            }
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
    keymap[KEY_KP0] = SDLK_KP0;
    keymap[KEY_KP1] = SDLK_KP1;
    keymap[KEY_KP2] = SDLK_KP2;
    keymap[KEY_KP3] = SDLK_KP3;
    keymap[KEY_KP4] = SDLK_KP4;
    keymap[KEY_KP5] = SDLK_KP5;
    keymap[KEY_KP6] = SDLK_KP6;
    keymap[KEY_KP7] = SDLK_KP7;
    keymap[KEY_KP8] = SDLK_KP8;
    keymap[KEY_KP9] = SDLK_KP9;
    keymap[KEY_KP_MINUS] = SDLK_KP_MINUS;
    keymap[KEY_KP_PLUS] = SDLK_KP_PLUS;
    keymap[KEY_KP_PERIOD] = SDLK_KP_PERIOD;
    keymap[KEY_KP_EQUALS] = SDLK_KP_EQUALS;
    keymap[KEY_KP_DIVIDE] = SDLK_KP_DIVIDE;
    keymap[KEY_KP_MULTIPLY] = SDLK_KP_MULTIPLY;
    keymap[KEY_KP_ENTER] = SDLK_KP_ENTER;
}

/* This is the original behavior, before support was added for 
 * differentiating between left and right versions of the keys.
 */
static void
DoUnsidedModifiers(int keyboard, unsigned short scancode,
                   unsigned int oldMods, unsigned int newMods)
{
    const int mapping[] = { SDLK_CAPSLOCK, SDLK_LSHIFT, SDLK_LCTRL, SDLK_LALT, SDLK_LMETA };
    unsigned int i, bit;

    /* Iterate through the bits, testing each against the current modifiers */
    for (i = 0, bit = NSAlphaShiftKeyMask; bit <= NSCommandKeyMask; bit <<= 1, ++i) {
        unsigned int oldMask, newMask;

        oldMask = oldMods & bit;
        newMask = newMods & bit;

        if (oldMask && oldMask != newMask) {        /* modifier up event */
            /* If this was Caps Lock, we need some additional voodoo to make SDL happy */
            if (bit == NSAlphaShiftKeyMask) {
                SDL_SendKeyboardKey(keyboard, SDL_PRESSED, (Uint8)scancode, mapping[i]);
            }
            SDL_SendKeyboardKey(keyboard, SDL_RELEASED, (Uint8)scancode, mapping[i]);
        } else if (newMask && oldMask != newMask) { /* modifier down event */
            SDL_SendKeyboardKey(keyboard, SDL_PRESSED, (Uint8)scancode, mapping[i]);
            /* If this was Caps Lock, we need some additional voodoo to make SDL happy */
            if (bit == NSAlphaShiftKeyMask) {
                SDL_SendKeyboardKey(keyboard, SDL_RELEASED, (Uint8)scancode, mapping[i]);
            }
        }
    }
}

/* This is a helper function for HandleModifierSide. This 
 * function reverts back to behavior before the distinction between
 * sides was made.
 */
static void
HandleNonDeviceModifier(int keyboard, unsigned short scancode,
                        unsigned int device_independent_mask,
                        unsigned int oldMods,
                        unsigned int newMods,
                        SDLKey key_sym)
{
    unsigned int oldMask, newMask;
    
    /* Isolate just the bits we care about in the depedent bits so we can 
     * figure out what changed
     */ 
    oldMask = oldMods & device_independent_mask;
    newMask = newMods & device_independent_mask;
    
    if (oldMask && oldMask != newMask) {
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, (Uint8)scancode, key_sym);
    } else if (newMask && oldMask != newMask) {
        SDL_SendKeyboardKey(keyboard, SDL_PRESSED, (Uint8)scancode, key_sym);
    }
}

/* This is a helper function for HandleModifierSide. 
 * This function sets the actual SDL_PrivateKeyboard event.
 */
static void
HandleModifierOneSide(int keyboard, unsigned short scancode,
                      unsigned int oldMods, unsigned int newMods,
                      SDLKey key_sym, 
                      unsigned int sided_device_dependent_mask)
{
    unsigned int old_dep_mask, new_dep_mask;

    /* Isolate just the bits we care about in the depedent bits so we can 
     * figure out what changed
     */ 
    old_dep_mask = oldMods & sided_device_dependent_mask;
    new_dep_mask = newMods & sided_device_dependent_mask;

    /* We now know that this side bit flipped. But we don't know if
     * it went pressed to released or released to pressed, so we must 
     * find out which it is.
     */
    if (new_dep_mask && old_dep_mask != new_dep_mask) {
        SDL_SendKeyboardKey(keyboard, SDL_PRESSED, (Uint8)scancode, key_sym);
    } else {
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, (Uint8)scancode, key_sym);
    }
}

/* This is a helper function for DoSidedModifiers.
 * This function will figure out if the modifier key is the left or right side, 
 * e.g. left-shift vs right-shift. 
 */
static void
HandleModifierSide(int keyboard, unsigned short scancode,
                   int device_independent_mask, 
                   unsigned int oldMods, unsigned int newMods, 
                   SDLKey left_key_sym, 
                   SDLKey right_key_sym,
                   unsigned int left_device_dependent_mask, 
                   unsigned int right_device_dependent_mask)
{
    unsigned int device_dependent_mask = (left_device_dependent_mask |
                                         right_device_dependent_mask);
    unsigned int diff_mod;
    
    /* On the basis that the device independent mask is set, but there are 
     * no device dependent flags set, we'll assume that we can't detect this 
     * keyboard and revert to the unsided behavior.
     */
    if ((device_dependent_mask & newMods) == 0) {
        /* Revert to the old behavior */
        HandleNonDeviceModifier(keyboard, scancode, device_independent_mask, oldMods, newMods, left_key_sym);
        return;
    }

    /* XOR the previous state against the new state to see if there's a change */
    diff_mod = (device_dependent_mask & oldMods) ^
               (device_dependent_mask & newMods);
    if (diff_mod) {
        /* A change in state was found. Isolate the left and right bits 
         * to handle them separately just in case the values can simulataneously
         * change or if the bits don't both exist.
         */
        if (left_device_dependent_mask & diff_mod) {
            HandleModifierOneSide(keyboard, scancode, oldMods, newMods, left_key_sym, left_device_dependent_mask);
        }
        if (right_device_dependent_mask & diff_mod) {
            HandleModifierOneSide(keyboard, scancode, oldMods, newMods, right_key_sym, right_device_dependent_mask);
        }
    }
}
   
/* This is a helper function for DoSidedModifiers.
 * This function will release a key press in the case that 
 * it is clear that the modifier has been released (i.e. one side 
 * can't still be down).
 */
static void
ReleaseModifierSide(int keyboard, unsigned short scancode,
                    unsigned int device_independent_mask, 
                    unsigned int oldMods, unsigned int newMods,
                    SDLKey left_key_sym, 
                    SDLKey right_key_sym,
                    unsigned int left_device_dependent_mask, 
                    unsigned int right_device_dependent_mask)
{
    unsigned int device_dependent_mask = (left_device_dependent_mask |
                                          right_device_dependent_mask);

    /* On the basis that the device independent mask is set, but there are 
     * no device dependent flags set, we'll assume that we can't detect this 
     * keyboard and revert to the unsided behavior.
     */
    if ((device_dependent_mask & oldMods) == 0) {
        /* In this case, we can't detect the keyboard, so use the left side 
         * to represent both, and release it. 
         */
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, (Uint8)scancode, left_key_sym);
        return;
    }

    /* 
     * This could have been done in an if-else case because at this point,
     * we know that all keys have been released when calling this function. 
     * But I'm being paranoid so I want to handle each separately,
     * so I hope this doesn't cause other problems.
     */
    if ( left_device_dependent_mask & oldMods ) {
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, (Uint8)scancode, left_key_sym);
    }
    if ( right_device_dependent_mask & oldMods ) {
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, (Uint8)scancode, right_key_sym);
    }
}

/* This is a helper function for DoSidedModifiers.
 * This function handles the CapsLock case.
 */
static void
HandleCapsLock(int keyboard, unsigned short scancode,
               unsigned int oldMods, unsigned int newMods)
{
    unsigned int oldMask, newMask;
    
    oldMask = oldMods & NSAlphaShiftKeyMask;
    newMask = newMods & NSAlphaShiftKeyMask;

    if (oldMask != newMask) {
        SDL_SendKeyboardKey(keyboard, SDL_PRESSED, (Uint8)scancode, SDLK_CAPSLOCK);
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, (Uint8)scancode, SDLK_CAPSLOCK);
    }

    oldMask = oldMods & NSNumericPadKeyMask;
    newMask = newMods & NSNumericPadKeyMask;

    if (oldMask != newMask) {
        SDL_SendKeyboardKey(keyboard, SDL_PRESSED, (Uint8)scancode, SDLK_NUMLOCK);
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, (Uint8)scancode, SDLK_NUMLOCK);
    }
}

/* This function will handle the modifier keys and also determine the 
 * correct side of the key.
 */
static void
DoSidedModifiers(int keyboard, unsigned short scancode,
                 unsigned int oldMods, unsigned int newMods)
{
	/* Set up arrays for the key syms for the left and right side. */
    const SDLKey left_mapping[]  = { SDLK_LSHIFT, SDLK_LCTRL, SDLK_LALT, SDLK_LMETA };
    const SDLKey right_mapping[] = { SDLK_RSHIFT, SDLK_RCTRL, SDLK_RALT, SDLK_RMETA };
	/* Set up arrays for the device dependent masks with indices that 
     * correspond to the _mapping arrays 
     */
    const unsigned int left_device_mapping[]  = { NX_DEVICELSHIFTKEYMASK, NX_DEVICELCTLKEYMASK, NX_DEVICELALTKEYMASK, NX_DEVICELCMDKEYMASK };
    const unsigned int right_device_mapping[] = { NX_DEVICERSHIFTKEYMASK, NX_DEVICERCTLKEYMASK, NX_DEVICERALTKEYMASK, NX_DEVICERCMDKEYMASK };

    unsigned int i, bit;

    /* Handle CAPSLOCK separately because it doesn't have a left/right side */
    HandleCapsLock(keyboard, scancode, oldMods, newMods);

    /* Iterate through the bits, testing each against the old modifiers */
    for (i = 0, bit = NSShiftKeyMask; bit <= NSCommandKeyMask; bit <<= 1, ++i) {
        unsigned int oldMask, newMask;
		
        oldMask = oldMods & bit;
        newMask = newMods & bit;
		
        /* If the bit is set, we must always examine it because the left
         * and right side keys may alternate or both may be pressed.
         */
        if (newMask) {
            HandleModifierSide(keyboard, scancode, bit, oldMods, newMods,
                               left_mapping[i], right_mapping[i],
                               left_device_mapping[i], right_device_mapping[i]);
        }
        /* If the state changed from pressed to unpressed, we must examine
            * the device dependent bits to release the correct keys.
            */
        else if (oldMask && oldMask != newMask) {
            ReleaseModifierSide(keyboard, scancode, bit, oldMods, newMods,
                              left_mapping[i], right_mapping[i],
                              left_device_mapping[i], right_device_mapping[i]);
        }
    }
}

static void
HandleModifiers(_THIS, unsigned short scancode, unsigned int modifierFlags)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    if (modifierFlags == data->modifierFlags) {
    	return;
    }

    /* 
     * Starting with Panther (10.3.0), the ability to distinguish between 
     * left side and right side modifiers is available.
     */
    if (data->osversion >= 0x1030) {
        DoSidedModifiers(data->keyboard, scancode, data->modifierFlags, modifierFlags);
    } else {
        DoUnsidedModifiers(data->keyboard, scancode, data->modifierFlags, modifierFlags);
    }
    data->modifierFlags = modifierFlags;
}

void
Cocoa_InitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_Keyboard keyboard;

    InitKeymap(data->keymap);

    data->fieldEdit = [[NSTextView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 0.0, 0.0)];
    
    SDL_zero(keyboard);
    data->keyboard = SDL_AddKeyboard(&keyboard, -1);
}

void
Cocoa_HandleKeyEvent(_THIS, NSEvent *event)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    unsigned short scancode = [event keyCode];
    const char *text;

    if (scancode >= 256) {
        /* Hmm, does this ever happen?  If so, need to extend the keymap... */
        return;
    }

    switch ([event type]) {
    case NSKeyDown:
        if (![event isARepeat]) {
            SDL_SendKeyboardKey(data->keyboard, SDL_PRESSED, (Uint8)scancode,
                                data->keymap[scancode]);
        }
        if (SDL_EventState(SDL_TEXTINPUT, SDL_QUERY)) {
            [data->fieldEdit interpretKeyEvents:[NSArray arrayWithObject:event]];
            text = [[event characters] UTF8String];
            if(text && *text) {
                SDL_SendKeyboardText(data->keyboard, text);
            }
        }
        break;
    case NSKeyUp:
        SDL_SendKeyboardKey(data->keyboard, SDL_RELEASED, (Uint8)scancode,
                            data->keymap[scancode]);
        break;
    case NSFlagsChanged:
        HandleModifiers(_this, scancode, [event modifierFlags]);
        break;
    }
}

void
Cocoa_QuitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    SDL_DelKeyboard(data->keyboard);

    [data->fieldEdit release];
}

/* vi: set ts=4 sw=4 expandtab: */
