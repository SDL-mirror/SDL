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

#include <Carbon/Carbon.h>


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

@interface SDLTranslatorResponder : NSTextView
{
}
- (void) doCommandBySelector:(SEL)myselector;
@end

@implementation SDLTranslatorResponder
- (void) doCommandBySelector:(SEL) myselector {}
@end

/* This is the original behavior, before support was added for 
 * differentiating between left and right versions of the keys.
 */
static void
DoUnsidedModifiers(int keyboard, unsigned short scancode,
                   unsigned int oldMods, unsigned int newMods)
{
    const int mapping[] = {
        SDL_SCANCODE_CAPSLOCK,
        SDL_SCANCODE_LSHIFT,
        SDL_SCANCODE_LCTRL,
        SDL_SCANCODE_LALT,
        SDL_SCANCODE_LGUI
    };
    unsigned int i, bit;

    /* Iterate through the bits, testing each against the current modifiers */
    for (i = 0, bit = NSAlphaShiftKeyMask; bit <= NSCommandKeyMask; bit <<= 1, ++i) {
        unsigned int oldMask, newMask;

        oldMask = oldMods & bit;
        newMask = newMods & bit;

        if (oldMask && oldMask != newMask) {        /* modifier up event */
            /* If this was Caps Lock, we need some additional voodoo to make SDL happy */
            if (bit == NSAlphaShiftKeyMask) {
                SDL_SendKeyboardKey(keyboard, SDL_PRESSED, mapping[i]);
            }
            SDL_SendKeyboardKey(keyboard, SDL_RELEASED, mapping[i]);
        } else if (newMask && oldMask != newMask) { /* modifier down event */
            SDL_SendKeyboardKey(keyboard, SDL_PRESSED, mapping[i]);
            /* If this was Caps Lock, we need some additional voodoo to make SDL happy */
            if (bit == NSAlphaShiftKeyMask) {
                SDL_SendKeyboardKey(keyboard, SDL_RELEASED, mapping[i]);
            }
        }
    }
}

/* This is a helper function for HandleModifierSide. This 
 * function reverts back to behavior before the distinction between
 * sides was made.
 */
static void
HandleNonDeviceModifier(int keyboard,
                        unsigned int device_independent_mask,
                        unsigned int oldMods,
                        unsigned int newMods,
                        SDL_scancode scancode)
{
    unsigned int oldMask, newMask;
    
    /* Isolate just the bits we care about in the depedent bits so we can 
     * figure out what changed
     */ 
    oldMask = oldMods & device_independent_mask;
    newMask = newMods & device_independent_mask;
    
    if (oldMask && oldMask != newMask) {
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, scancode);
    } else if (newMask && oldMask != newMask) {
        SDL_SendKeyboardKey(keyboard, SDL_PRESSED, scancode);
    }
}

/* This is a helper function for HandleModifierSide. 
 * This function sets the actual SDL_PrivateKeyboard event.
 */
static void
HandleModifierOneSide(int keyboard,
                      unsigned int oldMods, unsigned int newMods,
                      SDL_scancode scancode, 
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
        SDL_SendKeyboardKey(keyboard, SDL_PRESSED, scancode);
    } else {
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, scancode);
    }
}

/* This is a helper function for DoSidedModifiers.
 * This function will figure out if the modifier key is the left or right side, 
 * e.g. left-shift vs right-shift. 
 */
static void
HandleModifierSide(int keyboard,
                   int device_independent_mask, 
                   unsigned int oldMods, unsigned int newMods, 
                   SDL_scancode left_scancode, 
                   SDL_scancode right_scancode,
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
        HandleNonDeviceModifier(keyboard, device_independent_mask, oldMods, newMods, left_scancode);
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
            HandleModifierOneSide(keyboard, oldMods, newMods, left_scancode, left_device_dependent_mask);
        }
        if (right_device_dependent_mask & diff_mod) {
            HandleModifierOneSide(keyboard, oldMods, newMods, right_scancode, right_device_dependent_mask);
        }
    }
}
   
/* This is a helper function for DoSidedModifiers.
 * This function will release a key press in the case that 
 * it is clear that the modifier has been released (i.e. one side 
 * can't still be down).
 */
static void
ReleaseModifierSide(int keyboard,
                    unsigned int device_independent_mask, 
                    unsigned int oldMods, unsigned int newMods,
                    SDL_scancode left_scancode, 
                    SDL_scancode right_scancode,
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
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, left_scancode);
        return;
    }

    /* 
     * This could have been done in an if-else case because at this point,
     * we know that all keys have been released when calling this function. 
     * But I'm being paranoid so I want to handle each separately,
     * so I hope this doesn't cause other problems.
     */
    if ( left_device_dependent_mask & oldMods ) {
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, left_scancode);
    }
    if ( right_device_dependent_mask & oldMods ) {
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, right_scancode);
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
        SDL_SendKeyboardKey(keyboard, SDL_PRESSED, SDL_SCANCODE_CAPSLOCK);
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, SDL_SCANCODE_CAPSLOCK);
    }

    oldMask = oldMods & NSNumericPadKeyMask;
    newMask = newMods & NSNumericPadKeyMask;

    if (oldMask != newMask) {
        SDL_SendKeyboardKey(keyboard, SDL_PRESSED, SDL_SCANCODE_NUMLOCKCLEAR);
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, SDL_SCANCODE_NUMLOCKCLEAR);
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
    const SDL_scancode left_mapping[]  = {
        SDL_SCANCODE_LSHIFT,
        SDL_SCANCODE_LCTRL,
        SDL_SCANCODE_LALT,
        SDL_SCANCODE_LGUI
    };
    const SDL_scancode right_mapping[] = {
        SDL_SCANCODE_RSHIFT,
        SDL_SCANCODE_RCTRL,
        SDL_SCANCODE_RALT,
        SDL_SCANCODE_RGUI
    };
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
            HandleModifierSide(keyboard, bit, oldMods, newMods,
                               left_mapping[i], right_mapping[i],
                               left_device_mapping[i], right_device_mapping[i]);
        }
        /* If the state changed from pressed to unpressed, we must examine
            * the device dependent bits to release the correct keys.
            */
        else if (oldMask && oldMask != newMask) {
            ReleaseModifierSide(keyboard, bit, oldMods, newMods,
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

static void
UpdateKeymap(SDL_VideoData *data)
{
    KeyboardLayoutRef key_layout;
    const void *chr_data;
    int i;
    SDL_scancode scancode;
    SDLKey keymap[SDL_NUM_SCANCODES];

    /* See if the keymap needs to be updated */
    KLGetCurrentKeyboardLayout(&key_layout);
    if (key_layout == data->key_layout) {
        return;
    }
    data->key_layout = key_layout;

    SDL_GetDefaultKeymap(keymap);

    /* Try Unicode data first (preferred as of Mac OS X 10.5) */
    KLGetKeyboardLayoutProperty(key_layout, kKLuchrData, &chr_data);
    if (chr_data) {
        UInt32 keyboard_type = LMGetKbdType();
        OSStatus err;

        for (i = 0; i < SDL_arraysize(scancode_table); i++) {
            UniChar s[8];
            UniCharCount len;
            UInt32 dead_key_state;

            /* Make sure this scancode is a valid character scancode */
            scancode = scancode_table[i];
            if (scancode == SDL_SCANCODE_UNKNOWN ||
                (keymap[scancode] & SDLK_SCANCODE_MASK)) {
                continue;
            }

            dead_key_state = 0;
            err = UCKeyTranslate (chr_data, i, kUCKeyActionDown,
                                  0, keyboard_type,
                                  kUCKeyTranslateNoDeadKeysMask,
                                  &dead_key_state, 8, &len, s);
            if (err != noErr)
                continue;

            if (len > 0 && s[0] != 0x10) {
                keymap[scancode] = s[0];
            }
        }
        SDL_SetKeymap(data->keyboard, 0, keymap, SDL_NUM_SCANCODES);
        return;
    }

    /* Fall back to older style key map data */
    KLGetKeyboardLayoutProperty(key_layout, kKLKCHRData, &chr_data);
    if (chr_data) {
        for (i = 0; i < 128; i++) {
            UInt32 c, state = 0;

            /* Make sure this scancode is a valid character scancode */
            scancode = scancode_table[i];
            if (scancode == SDL_SCANCODE_UNKNOWN ||
                (keymap[scancode] & SDLK_SCANCODE_MASK)) {
                continue;
            }

            c = KeyTranslate (chr_data, i, &state) & 255;
            if (state) {
                /* Dead key, process key up */
                c = KeyTranslate (chr_data, i | 128, &state) & 255;
            }

            if (c != 0 && c != 0x10) {
                /* MacRoman to Unicode table, taken from X.org sources */
                static const unsigned short macroman_table[128] = {
                    0xc4, 0xc5, 0xc7, 0xc9, 0xd1, 0xd6, 0xdc, 0xe1,
                    0xe0, 0xe2, 0xe4, 0xe3, 0xe5, 0xe7, 0xe9, 0xe8,
                    0xea, 0xeb, 0xed, 0xec, 0xee, 0xef, 0xf1, 0xf3,
                    0xf2, 0xf4, 0xf6, 0xf5, 0xfa, 0xf9, 0xfb, 0xfc,
                    0x2020, 0xb0, 0xa2, 0xa3, 0xa7, 0x2022, 0xb6, 0xdf,
                    0xae, 0xa9, 0x2122, 0xb4, 0xa8, 0x2260, 0xc6, 0xd8,
                    0x221e, 0xb1, 0x2264, 0x2265, 0xa5, 0xb5, 0x2202, 0x2211,
                    0x220f, 0x3c0, 0x222b, 0xaa, 0xba, 0x3a9, 0xe6, 0xf8,
                    0xbf, 0xa1, 0xac, 0x221a, 0x192, 0x2248, 0x2206, 0xab,
                    0xbb, 0x2026, 0xa0, 0xc0, 0xc3, 0xd5, 0x152, 0x153,
                    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0xf7, 0x25ca,
                    0xff, 0x178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02,
                    0x2021, 0xb7, 0x201a, 0x201e, 0x2030, 0xc2, 0xca, 0xc1,
                    0xcb, 0xc8, 0xcd, 0xce, 0xcf, 0xcc, 0xd3, 0xd4,
                    0xf8ff, 0xd2, 0xda, 0xdb, 0xd9, 0x131, 0x2c6, 0x2dc,
                    0xaf, 0x2d8, 0x2d9, 0x2da, 0xb8, 0x2dd, 0x2db, 0x2c7,
                };

                if (c >= 128) {
                    c = macroman_table[c - 128];
                }
                keymap[scancode] = c;
            }
        }
        SDL_SetKeymap(data->keyboard, 0, keymap, SDL_NUM_SCANCODES);
        return;
    }
}

void
Cocoa_InitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_Keyboard keyboard;
    NSAutoreleasePool *pool;

    pool = [[NSAutoreleasePool alloc] init];
    data->fieldEdit = [[SDLTranslatorResponder alloc] initWithFrame:NSMakeRect(0.0, 0.0, 0.0, 0.0)];
    [pool release];
    
    SDL_zero(keyboard);
    data->keyboard = SDL_AddKeyboard(&keyboard, -1);
    UpdateKeymap(data);
    
    /* Set our own names for the platform-dependent but layout-independent keys */
    /* This key is NumLock on the MacBook keyboard. :) */
    /*SDL_SetScancodeName(SDL_SCANCODE_NUMLOCKCLEAR, "Clear");*/
    SDL_SetScancodeName(SDL_SCANCODE_LALT, "Left Option");
    SDL_SetScancodeName(SDL_SCANCODE_LGUI, "Left Command");
    SDL_SetScancodeName(SDL_SCANCODE_RALT, "Right Option");
    SDL_SetScancodeName(SDL_SCANCODE_RGUI, "Right Command");
}

void
Cocoa_HandleKeyEvent(_THIS, NSEvent *event)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    unsigned short scancode = [event keyCode];
    SDL_scancode code;
    const char *text;

    if ((scancode == 10 || scancode == 50) && KBGetLayoutType(LMGetKbdType()) == kKeyboardISO) {
        /* see comments in SDL_cocoakeys.h */
        scancode = 60 - scancode;
    }
    if (scancode < SDL_arraysize(scancode_table)) {
        code = scancode_table[scancode];
    }
    else {
        /* Hmm, does this ever happen?  If so, need to extend the keymap... */
        code = SDL_SCANCODE_UNKNOWN;
    }

    switch ([event type]) {
    case NSKeyDown:
        if (![event isARepeat]) {
            /* See if we need to rebuild the keyboard layout */
            UpdateKeymap(data);

            SDL_SendKeyboardKey(data->keyboard, SDL_PRESSED, code);
#if 1
            if (code == SDL_SCANCODE_UNKNOWN) {
                fprintf(stderr, "The key you just pressed is not recognized by SDL. To help get this fixed, report this to the SDL mailing list <sdl@libsdl.org> or to Christian Walther <cwalther@gmx.ch>. Mac virtual key code is %d.\n", scancode);
            }
#endif
        }
        if (SDL_EventState(SDL_TEXTINPUT, SDL_QUERY)) {
            /* FIXME CW 2007-08-16: only send those events to the field editor for which we actually want text events, not e.g. esc or function keys. Arrow keys in particular seem to produce crashes sometimes. */
            [data->fieldEdit interpretKeyEvents:[NSArray arrayWithObject:event]];
            text = [[event characters] UTF8String];
            if(text && *text) {
                SDL_SendKeyboardText(data->keyboard, text);
                [data->fieldEdit setString:@""];
            }
        }
        break;
    case NSKeyUp:
        SDL_SendKeyboardKey(data->keyboard, SDL_RELEASED, code);
        break;
    case NSFlagsChanged:
        /* FIXME CW 2007-08-14: check if this whole mess that takes up half of this file is really necessary */
        HandleModifiers(_this, scancode, [event modifierFlags]);
        break;
    default: /* just to avoid compiler warnings */
        break;
    }
}

void
Cocoa_QuitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    NSAutoreleasePool *pool;

    SDL_DelKeyboard(data->keyboard);

    pool = [[NSAutoreleasePool alloc] init];
    [data->fieldEdit release];
    [pool release];
}

/* vi: set ts=4 sw=4 expandtab: */
