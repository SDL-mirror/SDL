/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../include/SDL_scancode.h"

/* Amiga virtual key code to SDL_Keycode mapping table
   Sources:
   - AmigaOS wiki
*/
/* *INDENT-OFF* */
static SDL_Scancode const amiga_scancode_table[] = {
    /*  0 */    SDL_SCANCODE_GRAVE,
    /*  1 */    SDL_SCANCODE_1,
    /*  2 */    SDL_SCANCODE_2,
    /*  3 */    SDL_SCANCODE_3,
    /*  4 */    SDL_SCANCODE_4,
    /*  5 */    SDL_SCANCODE_5,
    /*  6 */    SDL_SCANCODE_6,
    /*  7 */    SDL_SCANCODE_7,
    /*  8 */    SDL_SCANCODE_8,
    /*  9 */     SDL_SCANCODE_9,
    /*  10 */    SDL_SCANCODE_0,
    /*  11 */    SDL_SCANCODE_MINUS,
    /*  12 */    SDL_SCANCODE_EQUALS,
    /*  13 */    SDL_SCANCODE_BACKSLASH,
    /*  14 */    SDL_SCANCODE_INTERNATIONAL3,
    /*  15 */    SDL_SCANCODE_KP_0,
    /*  16 */    SDL_SCANCODE_Q,
    /*  17 */    SDL_SCANCODE_W,
    /*  18 */    SDL_SCANCODE_E,
    /*  19 */    SDL_SCANCODE_R,
    /*  20 */    SDL_SCANCODE_T,
    /*  21 */    SDL_SCANCODE_Y,
    /*  22 */    SDL_SCANCODE_U,
    /*  23 */    SDL_SCANCODE_I,
    /*  24 */    SDL_SCANCODE_O,
    /*  25 */    SDL_SCANCODE_P,
    /*  26 */    SDL_SCANCODE_LEFTBRACKET,
    /*  27 */    SDL_SCANCODE_RIGHTBRACKET,
    /*  28 */    SDL_SCANCODE_UNKNOWN,
    /*  29 */    SDL_SCANCODE_KP_1,
    /*  30 */    SDL_SCANCODE_KP_2,
    /*  31 */    SDL_SCANCODE_KP_3,
    /*  32 */    SDL_SCANCODE_A,
    /*  33 */    SDL_SCANCODE_S,
    /*  34 */    SDL_SCANCODE_D,
    /*  35 */    SDL_SCANCODE_F,
    /*  36 */    SDL_SCANCODE_G,
    /*  37 */    SDL_SCANCODE_H,
    /*  38 */    SDL_SCANCODE_J,
    /*  39 */    SDL_SCANCODE_K,
    /*  40 */    SDL_SCANCODE_L,
    /*  41 */    SDL_SCANCODE_SEMICOLON,
    /*  42 */    SDL_SCANCODE_APOSTROPHE,
    /*  43 */    SDL_SCANCODE_INTERNATIONAL1,
    /*  44 */    SDL_SCANCODE_UNKNOWN,
    /*  45 */    SDL_SCANCODE_KP_4,
    /*  46 */    SDL_SCANCODE_KP_5,
    /*  47 */    SDL_SCANCODE_KP_6,
    /*  48 */    SDL_SCANCODE_INTERNATIONAL2,
    /*  49 */    SDL_SCANCODE_Z,
    /*  50 */    SDL_SCANCODE_X,
    /*  51 */    SDL_SCANCODE_C,
    /*  52 */    SDL_SCANCODE_V,
    /*  53 */    SDL_SCANCODE_B,
    /*  54 */    SDL_SCANCODE_N,
    /*  55 */    SDL_SCANCODE_M,
    /*  56 */    SDL_SCANCODE_COMMA,
    /*  57 */    SDL_SCANCODE_PERIOD,
    /*  58 */    SDL_SCANCODE_SLASH,
    /*  59 */    SDL_SCANCODE_UNKNOWN, // or SDL_SCANCODE_INTERNATIONAL1,
    /*  60 */    SDL_SCANCODE_KP_PERIOD,
    /*  61 */    SDL_SCANCODE_KP_7,
    /*  62 */    SDL_SCANCODE_KP_8,
    /*  63 */    SDL_SCANCODE_KP_9,
    /*  64 */    SDL_SCANCODE_SPACE,
    /*  65 */    SDL_SCANCODE_BACKSPACE,
    /*  66 */    SDL_SCANCODE_TAB,
    /*  67 */    SDL_SCANCODE_KP_ENTER,
    /*  68 */    SDL_SCANCODE_RETURN,
    /*  69 */    SDL_SCANCODE_ESCAPE,
    /*  70 */    SDL_SCANCODE_DELETE,
    /*  71 */    SDL_SCANCODE_INSERT,
    /*  72 */    SDL_SCANCODE_PAGEUP,
    /*  73 */    SDL_SCANCODE_PAGEDOWN,
    /*  74 */    SDL_SCANCODE_KP_MINUS,
    /*  75 */    SDL_SCANCODE_F11,
    /*  76 */    SDL_SCANCODE_UP,
    /*  77 */    SDL_SCANCODE_DOWN,
    /*  78 */    SDL_SCANCODE_RIGHT,
    /*  79 */    SDL_SCANCODE_LEFT,
    /*  80 */    SDL_SCANCODE_F1,
    /*  81 */    SDL_SCANCODE_F2,
    /*  82 */    SDL_SCANCODE_F3,
    /*  83 */    SDL_SCANCODE_F4,
    /*  84 */    SDL_SCANCODE_F5,
    /*  85 */    SDL_SCANCODE_F6,
    /*  86 */    SDL_SCANCODE_F7,
    /*  87 */    SDL_SCANCODE_F8,
    /*  88 */    SDL_SCANCODE_F9,
    /*  89 */    SDL_SCANCODE_F10,
    /*  90 */    SDL_SCANCODE_KP_LEFTPAREN,
    /*  91 */    SDL_SCANCODE_KP_RIGHTPAREN,
    /*  92 */    SDL_SCANCODE_KP_DIVIDE,
    /*  93 */    SDL_SCANCODE_KP_MULTIPLY,
    /*  94 */    SDL_SCANCODE_KP_PLUS,
    /*  95 */    SDL_SCANCODE_SCROLLLOCK, // or SDL_SCANCODE_HELP,
    /*  96 */    SDL_SCANCODE_LSHIFT,
    /*  97 */    SDL_SCANCODE_RSHIFT,
    /*  98 */    SDL_SCANCODE_CAPSLOCK,
    /*  99 */    SDL_SCANCODE_LCTRL,
    /*  100 */    SDL_SCANCODE_LALT,
    /*  101 */    SDL_SCANCODE_RALT,
    /*  102 */    SDL_SCANCODE_LGUI,
    /*  103 */    SDL_SCANCODE_RGUI,
    /*  104 */    SDL_SCANCODE_UNKNOWN,
    /*  105 */    SDL_SCANCODE_UNKNOWN,
    /*  106 */    SDL_SCANCODE_UNKNOWN,
    /*  107 */    SDL_SCANCODE_MENU,
    /*  108 */    SDL_SCANCODE_KP_PERIOD,
    /*  109 */    SDL_SCANCODE_SYSREQ,
    /*  110 */    SDL_SCANCODE_PAUSE,
    /*  111 */    SDL_SCANCODE_F12,
    /*  112 */    SDL_SCANCODE_HOME,
    /*  113 */    SDL_SCANCODE_END,
};
/* *INDENT-ON* */
