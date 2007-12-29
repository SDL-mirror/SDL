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
        SDL_SendKeyboardKey(keyboard, SDL_PRESSED, (Uint8)scancode, SDLK_KP_NUMLOCKCLEAR);
        SDL_SendKeyboardKey(keyboard, SDL_RELEASED, (Uint8)scancode, SDLK_KP_NUMLOCKCLEAR);
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
    NSAutoreleasePool *pool;

    pool = [[NSAutoreleasePool alloc] init];
    data->fieldEdit = [[SDLTranslatorResponder alloc] initWithFrame:NSMakeRect(0.0, 0.0, 0.0, 0.0)];
    [pool release];
    
    SDL_zero(keyboard);
    data->keyboard = SDL_AddKeyboard(&keyboard, -1);
    
    /* Set our own names for the platform-dependent but layout-independent keys */
    SDL_SetKeyName(SDLK_KP_NUMLOCKCLEAR, "clear");
    SDL_SetKeyName(SDLK_LALT, "left option");
    SDL_SetKeyName(SDLK_LMETA, "left command");
    SDL_SetKeyName(SDLK_RALT, "right option");
    SDL_SetKeyName(SDLK_RMETA, "right command");
}

void
Cocoa_HandleKeyEvent(_THIS, NSEvent *event)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    unsigned short scancode = [event keyCode];
    SDLKey physicalKey;
    const char *text;

    if ((scancode == 10 || scancode == 50) && KBGetLayoutType(LMGetKbdType()) == kKeyboardISO) {
        /* see comments in SDL_cocoakeys.h */
        scancode = 60 - scancode;
    }
    if (scancode < SDL_arraysize(macToSDLKey)) {
        physicalKey = macToSDLKey[scancode];
    }
    else {
        /* Hmm, does this ever happen?  If so, need to extend the keymap... */
        physicalKey = SDLK_UNKNOWN;
    }

    switch ([event type]) {
    case NSKeyDown:
        if (![event isARepeat]) {
            SDL_SendKeyboardKey(data->keyboard, SDL_PRESSED, (Uint8)scancode, physicalKey);
#if 1
            if (physicalKey == SDLK_UNKNOWN) {
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
        SDL_SendKeyboardKey(data->keyboard, SDL_RELEASED, (Uint8)scancode, physicalKey);
        break;
    case NSFlagsChanged:
        /* FIXME CW 2007-08-14: check if this whole mess that takes up half of this file is really necessary */
        HandleModifiers(_this, scancode, [event modifierFlags]);
        break;
    default: /* just to avoid compiler warnings */
        break;
    }
}

SDLKey
Cocoa_GetLayoutKey(_THIS, SDLKey physicalKey)
{
    switch (physicalKey) {
        /* Many of these keys would generate a character in the translation by keyboard layout, but an inappropriate one, so we catch them before. */
        case SDLK_UNKNOWN:
        case SDLK_RETURN:
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE:
        case SDLK_TAB:
        case SDLK_SPACE:
        case SDLK_CAPSLOCK:
        case SDLK_F1:
        case SDLK_F2:
        case SDLK_F3:
        case SDLK_F4:
        case SDLK_F5:
        case SDLK_F6:
        case SDLK_F7:
        case SDLK_F8:
        case SDLK_F9:
        case SDLK_F10:
        case SDLK_F11:
        case SDLK_F12:
        case SDLK_PRINTSCREEN:
        case SDLK_SCROLLLOCK:
        case SDLK_PAUSE:
        case SDLK_INSERT:
        case SDLK_HOME:
        case SDLK_PAGEUP:
        case SDLK_DELETE:
        case SDLK_END:
        case SDLK_PAGEDOWN:
        case SDLK_RIGHT:
        case SDLK_LEFT:
        case SDLK_DOWN:
        case SDLK_UP:
        case SDLK_KP_NUMLOCKCLEAR:
        case SDLK_KP_ENTER:
        case SDLK_APPLICATION:
        case SDLK_POWER:
        case SDLK_F13:
        case SDLK_F14:
        case SDLK_F15:
        case SDLK_F16:
        case SDLK_LCTRL:
        case SDLK_LSHIFT:
        case SDLK_LALT:
        case SDLK_LMETA:
        case SDLK_RCTRL:
        case SDLK_RSHIFT:
        case SDLK_RALT:
        case SDLK_RMETA:
            return physicalKey;
        
        /* For the rest, we try the translation first. */
        default: {
            UInt16 vkey = 0;
            KeyboardLayoutRef layout;
            KeyboardLayoutKind kind;
            UInt32 keyboardType = LMGetKbdType();
            
            /* Look up pkey to get a Mac virtual key code - linear search isn't terribly efficient, this might have to be optimized. */
            while (vkey < 128 && physicalKey != macToSDLKey[vkey]) vkey++;
            if (vkey == 128) return physicalKey;
            if ((vkey == 10 || vkey == 50) && KBGetLayoutType(keyboardType) == kKeyboardISO) vkey = 60 - vkey; /* see comments in SDL_cocoakeys.h */
            
            if (KLGetCurrentKeyboardLayout(&layout) != noErr) return physicalKey;
            if (KLGetKeyboardLayoutProperty(layout, kKLKind, (const void **)&kind) != noErr) return physicalKey;
            if (kind == kKLKCHRuchrKind || kind == kKLuchrKind) {
                UniChar utf16String[4];
                UInt32 deadKeyState = 0;
                UniCharCount actualStringLength;
                const UCKeyboardLayout *uchrData;
                
                if (KLGetKeyboardLayoutProperty(layout, kKLuchrData, (const void **)&uchrData) != noErr) return physicalKey;
                if (UCKeyTranslate(uchrData, vkey, kUCKeyActionDisplay, 0, keyboardType, 0, &deadKeyState, 4, &actualStringLength, utf16String) != noErr) return physicalKey;
                /* kUCKeyActionDisplay (instead of kUCKeyActionDown) seems to take care of dead keys, so no need to check for that case and simulate a second key press */
                
                if (actualStringLength == 0) return physicalKey;
                
                /* Decode the first character from UTF-16. I'm not sure if this is appropriate for keyboard layouts that generate more than 1 character, or if we would have to use SDL_KEY_LAYOUT_SPECIAL_BIT in that case. */
                if (utf16String[0] < 0xD800 || utf16String[0] > 0xDFFF) {
                    return utf16String[0];
                }
                else if (utf16String[0] > 0xDBFF || utf16String[1] < 0xDC00 || utf16String[1] > 0xDFFF) {
                    /* invalid UTF-16 */
                    return physicalKey;
                }
                else {
                    return (((utf16String[0] & 0x3FF) << 10) | (utf16String[1] & 0x3FF)) + 0x10000;
                }
            }
            else { /* kind == kKLKCHRKind */
                const void *kchrData;
                UInt32 state = 0;
                UInt8 charCode;
                SInt32 scriptCode;
                TextEncoding keyboardEncoding;
                CFStringRef conversionString;
                UniChar codepoint;
                
                if (KLGetKeyboardLayoutProperty(layout, kKLKCHRData, &kchrData) != noErr) return physicalKey;
                charCode = KeyTranslate(kchrData, vkey, &state) & 0xFF; /* Actually returns a UInt32 containing two character codes (and two 'reserved' bytes), but we're only interested in the second (or only) one */
                if (charCode == 0) {
                    /* It's a dead key, so simulate a second key press */
                    charCode = KeyTranslate(kchrData, vkey, &state) & 0xFF;
                    /* Still zero? Give up. */
                    if (charCode == 0) return physicalKey;
                }
                if (KLGetKeyboardLayoutProperty(layout, kKLGroupIdentifier, (const void **)&scriptCode) != noErr) return physicalKey; /* That the group identifier is actually a script code is not documented, but confirmed here: <http://lists.apple.com/archives/carbon-dev/2005/Jan/msg00533.html> */
                if (UpgradeScriptInfoToTextEncoding(scriptCode, kTextLanguageDontCare, kTextRegionDontCare, NULL, &keyboardEncoding) != noErr) return physicalKey;
                
                conversionString = CFStringCreateWithBytes(kCFAllocatorDefault, &charCode, 1, keyboardEncoding, FALSE);
                codepoint = CFStringGetCharacterAtIndex(conversionString, 0);
                CFRelease(conversionString);
                return codepoint;
            }
        }
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
