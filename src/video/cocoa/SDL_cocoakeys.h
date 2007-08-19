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

/* Mac virtual key code to SDLKey mapping table
   Sources:
   - Inside Macintosh: Text <http://developer.apple.com/documentation/mac/Text/Text-571.html>
   - Apple USB keyboard driver source <http://darwinsource.opendarwin.org/10.4.6.ppc/IOHIDFamily-172.8/IOHIDFamily/Cosmo_USB2ADB.c>
   - experimentation on various ADB and USB ISO keyboards and one ADB ANSI keyboard
*/
/* *INDENT-OFF* */
static SDLKey macToSDLKey[128] = {
    /*   0 */   SDLK_A,
    /*   1 */   SDLK_S,
    /*   2 */   SDLK_D,
    /*   3 */   SDLK_F,
    /*   4 */   SDLK_H,
    /*   5 */   SDLK_G,
    /*   6 */   SDLK_Z,
    /*   7 */   SDLK_X,
    /*   8 */   SDLK_C,
    /*   9 */   SDLK_V,
    /*  10 */   SDLK_NONUSBACKSLASH, /* SDLK_NONUSBACKSLASH on ANSI and JIS keyboards (if this key would exist there), SDLK_GRAVE on ISO. (The USB keyboard driver actually translates these usage codes to different virtual key codes depending on whether the keyboard is ISO/ANSI/JIS. That's why you have to help it identify the keyboard type when you plug in a PC USB keyboard. It's a historical thing - ADB keyboards are wired this way.) */
    /*  11 */   SDLK_B,
    /*  12 */   SDLK_Q,
    /*  13 */   SDLK_W,
    /*  14 */   SDLK_E,
    /*  15 */   SDLK_R,
    /*  16 */   SDLK_Y,
    /*  17 */   SDLK_T,
    /*  18 */   SDLK_1,
    /*  19 */   SDLK_2,
    /*  20 */   SDLK_3,
    /*  21 */   SDLK_4,
    /*  22 */   SDLK_6,
    /*  23 */   SDLK_5,
    /*  24 */   SDLK_EQUALS,
    /*  25 */   SDLK_9,
    /*  26 */   SDLK_7,
    /*  27 */   SDLK_HYPHENMINUS,
    /*  28 */   SDLK_8,
    /*  29 */   SDLK_0,
    /*  30 */   SDLK_RIGHTBRACKET,
    /*  31 */   SDLK_O,
    /*  32 */   SDLK_U,
    /*  33 */   SDLK_LEFTBRACKET,
    /*  34 */   SDLK_I,
    /*  35 */   SDLK_P,
    /*  36 */   SDLK_RETURN,
    /*  37 */   SDLK_L,
    /*  38 */   SDLK_J,
    /*  39 */   SDLK_APOSTROPHE,
    /*  40 */   SDLK_K,
    /*  41 */   SDLK_SEMICOLON,
    /*  42 */   SDLK_BACKSLASH,
    /*  43 */   SDLK_COMMA,
    /*  44 */   SDLK_SLASH,
    /*  45 */   SDLK_N,
    /*  46 */   SDLK_M,
    /*  47 */   SDLK_PERIOD,
    /*  48 */   SDLK_TAB,
    /*  49 */   SDLK_SPACE,
    /*  50 */   SDLK_GRAVE, /* SDLK_GRAVE on ANSI and JIS keyboards, SDLK_NONUSBACKSLASH on ISO (see comment about virtual key code 10 above) */
    /*  51 */   SDLK_BACKSPACE,
    /*  52 */   SDLK_KP_ENTER, /* keyboard enter on portables */
    /*  53 */   SDLK_ESCAPE,
    /*  54 */   SDLK_RMETA,
    /*  55 */   SDLK_LMETA,
    /*  56 */   SDLK_LSHIFT,
    /*  57 */   SDLK_CAPSLOCK,
    /*  58 */   SDLK_LALT,
    /*  59 */   SDLK_LCTRL,
    /*  60 */   SDLK_RSHIFT,
    /*  61 */   SDLK_RALT,
    /*  62 */   SDLK_RCTRL,
    /*  63 */   SDLK_NONE, /* fn on portables, acts as a hardware-level modifier already, so we don't generate events for it */
    /*  64 */   SDLK_UNKNOWN, /* unknown (unused?) */
    /*  65 */   SDLK_KP_PERIOD,
    /*  66 */   SDLK_UNKNOWN, /* unknown (unused?) */
    /*  67 */   SDLK_KP_MULTIPLY,
    /*  68 */   SDLK_UNKNOWN, /* unknown (unused?) */
    /*  69 */   SDLK_KP_PLUS,
    /*  70 */   SDLK_UNKNOWN, /* unknown (unused?) */
    /*  71 */   SDLK_KP_NUMLOCKCLEAR,
    /*  72 */   SDLK_VOLUMEUP,
    /*  73 */   SDLK_VOLUMEDOWN,
    /*  74 */   SDLK_MUTE,
    /*  75 */   SDLK_KP_DIVIDE,
    /*  76 */   SDLK_KP_ENTER, /* keypad enter on external keyboards, fn-return on portables */
    /*  77 */   SDLK_UNKNOWN, /* unknown (unused?) */
    /*  78 */   SDLK_KP_MINUS,
    /*  79 */   SDLK_UNKNOWN, /* unknown (unused?) */
    /*  80 */   SDLK_UNKNOWN, /* unknown (unused?) */
    /*  81 */   SDLK_KP_EQUALS,
    /*  82 */   SDLK_KP_0,
    /*  83 */   SDLK_KP_1,
    /*  84 */   SDLK_KP_2,
    /*  85 */   SDLK_KP_3,
    /*  86 */   SDLK_KP_4,
    /*  87 */   SDLK_KP_5,
    /*  88 */   SDLK_KP_6,
    /*  89 */   SDLK_KP_7,
    /*  90 */   SDLK_UNKNOWN, /* unknown (unused?) */
    /*  91 */   SDLK_KP_8,
    /*  92 */   SDLK_KP_9,
    /*  93 */   SDLK_INTERNATIONAL3, /* Cosmo_USB2ADB.c says "Yen (JIS)" */
    /*  94 */   SDLK_INTERNATIONAL1, /* Cosmo_USB2ADB.c says "Ro (JIS)" */
    /*  95 */   SDLK_KP_COMMA, /* Cosmo_USB2ADB.c says ", JIS only" */
    /*  96 */   SDLK_F5,
    /*  97 */   SDLK_F6,
    /*  98 */   SDLK_F7,
    /*  99 */   SDLK_F3,
    /* 100 */   SDLK_F8,
    /* 101 */   SDLK_F9,
    /* 102 */   SDLK_LANG2, /* Cosmo_USB2ADB.c says "Eisu" */
    /* 103 */   SDLK_F11,
    /* 104 */   SDLK_LANG1, /* Cosmo_USB2ADB.c says "Kana" */
    /* 105 */   SDLK_PRINTSCREEN, /* On ADB keyboards, this key is labeled "F13/print screen". Problem: USB has different usage codes for these two functions. On Apple USB keyboards, the key is labeled "F13" and sends the F13 usage code (SDLK_F13). I decided to use SDLK_PRINTSCREEN here nevertheless since SDL applications are more likely to assume the presence of a print screen key than an F13 key. */
    /* 106 */   SDLK_F16,
    /* 107 */   SDLK_SCROLLLOCK, /* F14/scroll lock, see comment about F13/print screen above */
    /* 108 */   SDLK_UNKNOWN, /* unknown (unused?) */
    /* 109 */   SDLK_F10,
    /* 110 */   SDLK_APPLICATION, /* windows contextual menu key, fn-enter on portables */
    /* 111 */   SDLK_F12,
    /* 112 */   SDLK_UNKNOWN, /* unknown (unused?) */
    /* 113 */   SDLK_PAUSE, /* F15/pause, see comment about F13/print screen above */
    /* 114 */   SDLK_INSERT, /* the key is actually labeled "help" on Apple keyboards, and works as such in Mac OS, but it sends the "insert" usage code even on Apple USB keyboards */
    /* 115 */   SDLK_HOME,
    /* 116 */   SDLK_PAGEUP,
    /* 117 */   SDLK_DELETE,
    /* 118 */   SDLK_F4,
    /* 119 */   SDLK_END,
    /* 120 */   SDLK_F2,
    /* 121 */   SDLK_PAGEDOWN,
    /* 122 */   SDLK_F1,
    /* 123 */   SDLK_LEFT,
    /* 124 */   SDLK_RIGHT,
    /* 125 */   SDLK_DOWN,
    /* 126 */   SDLK_UP,
    /* 127 */   SDLK_POWER
};
/* *INDENT-ON* */
