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

#include "SDL_x11video.h"

#include "../../events/SDL_keyboard_c.h"

#include <X11/keysym.h>

#include "imKStoUCS.h"

/* 
   KeyCode-to-SDLKey translation tables for various X servers. Which
   one to use is decided in X11_InitKeyboard().
*/

static SDLKey macKeyCodeToSDLK[];
static SDLKey xorgLinuxKeyCodeToSDLK[];

static SDLKey *keyCodeToSDLKeyTables[] = {
    xorgLinuxKeyCodeToSDLK,
    macKeyCodeToSDLK,
    NULL
};

/* *INDENT-OFF* */

/* These are just Mac virtual key codes + 8 (see SDL/src/video/cocoa/
   SDL_cocoakeys.h for more info). Observed to work with Apple X11 on
   Mac OS X 10.4. May also work on older Linux distributions on Mac
   hardware.
*/

#define KeyCodeTableSize (256)
static SDLKey macKeyCodeToSDLK[KeyCodeTableSize] = 
{
    /*   0 */   SDLK_UNKNOWN,
    /*   1 */   SDLK_UNKNOWN,
    /*   2 */   SDLK_UNKNOWN,
    /*   3 */   SDLK_UNKNOWN,
    /*   4 */   SDLK_UNKNOWN,
    /*   5 */   SDLK_UNKNOWN,
    /*   6 */   SDLK_UNKNOWN,
    /*   7 */   SDLK_UNKNOWN,
    /*   8 */   SDLK_A,
    /*   9 */   SDLK_S,
    /*  10 */   SDLK_D,
    /*  11 */   SDLK_F,
    /*  12 */   SDLK_H,
    /*  13 */   SDLK_G,
    /*  14 */   SDLK_Z,
    /*  15 */   SDLK_X,
    /*  16 */   SDLK_C,
    /*  17 */   SDLK_V,
    /*  18 */   SDLK_GRAVE,
    /*  19 */   SDLK_B,
    /*  20 */   SDLK_Q,
    /*  21 */   SDLK_W,
    /*  22 */   SDLK_E,
    /*  23 */   SDLK_R,
    /*  24 */   SDLK_Y,
    /*  25 */   SDLK_T,
    /*  26 */   SDLK_1,
    /*  27 */   SDLK_2,
    /*  28 */   SDLK_3,
    /*  29 */   SDLK_4,
    /*  30 */   SDLK_6,
    /*  31 */   SDLK_5,
    /*  32 */   SDLK_EQUALS,
    /*  33 */   SDLK_9,
    /*  34 */   SDLK_7,
    /*  35 */   SDLK_HYPHENMINUS,
    /*  36 */   SDLK_8,
    /*  37 */   SDLK_0,
    /*  38 */   SDLK_RIGHTBRACKET,
    /*  39 */   SDLK_O,
    /*  40 */   SDLK_U,
    /*  41 */   SDLK_LEFTBRACKET,
    /*  42 */   SDLK_I,
    /*  43 */   SDLK_P,
    /*  44 */   SDLK_RETURN,
    /*  45 */   SDLK_L,
    /*  46 */   SDLK_J,
    /*  47 */   SDLK_APOSTROPHE,
    /*  48 */   SDLK_K,
    /*  49 */   SDLK_SEMICOLON,
    /*  50 */   SDLK_BACKSLASH,
    /*  51 */   SDLK_COMMA,
    /*  52 */   SDLK_SLASH,
    /*  53 */   SDLK_N,
    /*  54 */   SDLK_M,
    /*  55 */   SDLK_PERIOD,
    /*  56 */   SDLK_TAB,
    /*  57 */   SDLK_SPACE,
    /*  58 */   SDLK_NONUSBACKSLASH,
    /*  59 */   SDLK_BACKSPACE,
    /*  60 */   SDLK_KP_ENTER,
    /*  61 */   SDLK_ESCAPE,
    /*  62 */   SDLK_RMETA,
    /*  63 */   SDLK_LMETA,
    /*  64 */   SDLK_LSHIFT,
    /*  65 */   SDLK_CAPSLOCK,
    /*  66 */   SDLK_LALT,
    /*  67 */   SDLK_LCTRL,
    /*  68 */   SDLK_RSHIFT,
    /*  69 */   SDLK_RALT,
    /*  70 */   SDLK_RCTRL,
    /*  71 */   SDLK_NONE,
    /*  72 */   SDLK_UNKNOWN,
    /*  73 */   SDLK_KP_PERIOD,
    /*  74 */   SDLK_UNKNOWN,
    /*  75 */   SDLK_KP_MULTIPLY,
    /*  76 */   SDLK_UNKNOWN,
    /*  77 */   SDLK_KP_PLUS,
    /*  78 */   SDLK_UNKNOWN,
    /*  79 */   SDLK_KP_NUMLOCKCLEAR,
    /*  80 */   SDLK_VOLUMEUP,
    /*  81 */   SDLK_VOLUMEDOWN,
    /*  82 */   SDLK_MUTE,
    /*  83 */   SDLK_KP_DIVIDE,
    /*  84 */   SDLK_KP_ENTER,
    /*  85 */   SDLK_UNKNOWN,
    /*  86 */   SDLK_KP_MINUS,
    /*  87 */   SDLK_UNKNOWN,
    /*  88 */   SDLK_UNKNOWN,
    /*  89 */   SDLK_KP_EQUALS,
    /*  90 */   SDLK_KP_0,
    /*  91 */   SDLK_KP_1,
    /*  92 */   SDLK_KP_2,
    /*  93 */   SDLK_KP_3,
    /*  94 */   SDLK_KP_4,
    /*  95 */   SDLK_KP_5,
    /*  96 */   SDLK_KP_6,
    /*  97 */   SDLK_KP_7,
    /*  98 */   SDLK_UNKNOWN,
    /*  99 */   SDLK_KP_8,
    /* 100 */   SDLK_KP_9,
    /* 101 */   SDLK_INTERNATIONAL3,
    /* 102 */   SDLK_INTERNATIONAL1,
    /* 103 */   SDLK_KP_COMMA,
    /* 104 */   SDLK_F5,
    /* 105 */   SDLK_F6,
    /* 106 */   SDLK_F7,
    /* 107 */   SDLK_F3,
    /* 108 */   SDLK_F8,
    /* 109 */   SDLK_F9,
    /* 110 */   SDLK_LANG2,
    /* 111 */   SDLK_F11,
    /* 112 */   SDLK_LANG1,
    /* 113 */   SDLK_PRINTSCREEN,
    /* 114 */   SDLK_F16,
    /* 115 */   SDLK_SCROLLLOCK,
    /* 116 */   SDLK_UNKNOWN,
    /* 117 */   SDLK_F10,
    /* 118 */   SDLK_APPLICATION,
    /* 119 */   SDLK_F12,
    /* 120 */   SDLK_UNKNOWN,
    /* 121 */   SDLK_PAUSE,
    /* 122 */   SDLK_INSERT,
    /* 123 */   SDLK_HOME,
    /* 124 */   SDLK_PAGEUP,
    /* 125 */   SDLK_DELETE,
    /* 126 */   SDLK_F4,
    /* 127 */   SDLK_END,
    /* 128 */   SDLK_F2,
    /* 129 */   SDLK_PAGEDOWN,
    /* 130 */   SDLK_F1,
    /* 131 */   SDLK_LEFT,
    /* 132 */   SDLK_RIGHT,
    /* 133 */   SDLK_DOWN,
    /* 134 */   SDLK_UP,
    /* 135 */   SDLK_POWER,
    /* 136 */   SDLK_UNKNOWN, /* codes higher than 135 shouldn't occur as Mac virtual keycodes only go to 127 */
    /* 137 */   SDLK_UNKNOWN,
    /* 138 */   SDLK_UNKNOWN,
    /* 139 */   SDLK_UNKNOWN,
    /* 140 */   SDLK_UNKNOWN,
    /* 141 */   SDLK_UNKNOWN,
    /* 142 */   SDLK_UNKNOWN,
    /* 143 */   SDLK_UNKNOWN,
    /* 144 */   SDLK_UNKNOWN,
    /* 145 */   SDLK_UNKNOWN,
    /* 146 */   SDLK_UNKNOWN,
    /* 147 */   SDLK_UNKNOWN,
    /* 148 */   SDLK_UNKNOWN,
    /* 149 */   SDLK_UNKNOWN,
    /* 150 */   SDLK_UNKNOWN,
    /* 151 */   SDLK_UNKNOWN,
    /* 152 */   SDLK_UNKNOWN,
    /* 153 */   SDLK_UNKNOWN,
    /* 154 */   SDLK_UNKNOWN,
    /* 155 */   SDLK_UNKNOWN,
    /* 156 */   SDLK_UNKNOWN,
    /* 157 */   SDLK_UNKNOWN,
    /* 158 */   SDLK_UNKNOWN,
    /* 159 */   SDLK_UNKNOWN,
    /* 160 */   SDLK_UNKNOWN,
    /* 161 */   SDLK_UNKNOWN,
    /* 162 */   SDLK_UNKNOWN,
    /* 163 */   SDLK_UNKNOWN,
    /* 164 */   SDLK_UNKNOWN,
    /* 165 */   SDLK_UNKNOWN,
    /* 166 */   SDLK_UNKNOWN,
    /* 167 */   SDLK_UNKNOWN,
    /* 168 */   SDLK_UNKNOWN,
    /* 169 */   SDLK_UNKNOWN,
    /* 170 */   SDLK_UNKNOWN,
    /* 171 */   SDLK_UNKNOWN,
    /* 172 */   SDLK_UNKNOWN,
    /* 173 */   SDLK_UNKNOWN,
    /* 174 */   SDLK_UNKNOWN,
    /* 175 */   SDLK_UNKNOWN,
    /* 176 */   SDLK_UNKNOWN,
    /* 177 */   SDLK_UNKNOWN,
    /* 178 */   SDLK_UNKNOWN,
    /* 179 */   SDLK_UNKNOWN,
    /* 180 */   SDLK_UNKNOWN,
    /* 181 */   SDLK_UNKNOWN,
    /* 182 */   SDLK_UNKNOWN,
    /* 183 */   SDLK_UNKNOWN,
    /* 184 */   SDLK_UNKNOWN,
    /* 185 */   SDLK_UNKNOWN,
    /* 186 */   SDLK_UNKNOWN,
    /* 187 */   SDLK_UNKNOWN,
    /* 188 */   SDLK_UNKNOWN,
    /* 189 */   SDLK_UNKNOWN,
    /* 190 */   SDLK_UNKNOWN,
    /* 191 */   SDLK_UNKNOWN,
    /* 192 */   SDLK_UNKNOWN,
    /* 193 */   SDLK_UNKNOWN,
    /* 194 */   SDLK_UNKNOWN,
    /* 195 */   SDLK_UNKNOWN,
    /* 196 */   SDLK_UNKNOWN,
    /* 197 */   SDLK_UNKNOWN,
    /* 198 */   SDLK_UNKNOWN,
    /* 199 */   SDLK_UNKNOWN,
    /* 200 */   SDLK_UNKNOWN,
    /* 201 */   SDLK_UNKNOWN,
    /* 202 */   SDLK_UNKNOWN,
    /* 203 */   SDLK_UNKNOWN,
    /* 204 */   SDLK_UNKNOWN,
    /* 205 */   SDLK_UNKNOWN,
    /* 206 */   SDLK_UNKNOWN,
    /* 207 */   SDLK_UNKNOWN,
    /* 208 */   SDLK_UNKNOWN,
    /* 209 */   SDLK_UNKNOWN,
    /* 210 */   SDLK_UNKNOWN,
    /* 211 */   SDLK_UNKNOWN,
    /* 212 */   SDLK_UNKNOWN,
    /* 213 */   SDLK_UNKNOWN,
    /* 214 */   SDLK_UNKNOWN,
    /* 215 */   SDLK_UNKNOWN,
    /* 216 */   SDLK_UNKNOWN,
    /* 217 */   SDLK_UNKNOWN,
    /* 218 */   SDLK_UNKNOWN,
    /* 219 */   SDLK_UNKNOWN,
    /* 220 */   SDLK_UNKNOWN,
    /* 221 */   SDLK_UNKNOWN,
    /* 222 */   SDLK_UNKNOWN,
    /* 223 */   SDLK_UNKNOWN,
    /* 224 */   SDLK_UNKNOWN,
    /* 225 */   SDLK_UNKNOWN,
    /* 226 */   SDLK_UNKNOWN,
    /* 227 */   SDLK_UNKNOWN,
    /* 228 */   SDLK_UNKNOWN,
    /* 229 */   SDLK_UNKNOWN,
    /* 230 */   SDLK_UNKNOWN,
    /* 231 */   SDLK_UNKNOWN,
    /* 232 */   SDLK_UNKNOWN,
    /* 233 */   SDLK_UNKNOWN,
    /* 234 */   SDLK_UNKNOWN,
    /* 235 */   SDLK_UNKNOWN,
    /* 236 */   SDLK_UNKNOWN,
    /* 237 */   SDLK_UNKNOWN,
    /* 238 */   SDLK_UNKNOWN,
    /* 239 */   SDLK_UNKNOWN,
    /* 240 */   SDLK_UNKNOWN,
    /* 241 */   SDLK_UNKNOWN,
    /* 242 */   SDLK_UNKNOWN,
    /* 243 */   SDLK_UNKNOWN,
    /* 244 */   SDLK_UNKNOWN,
    /* 245 */   SDLK_UNKNOWN,
    /* 246 */   SDLK_UNKNOWN,
    /* 247 */   SDLK_UNKNOWN,
    /* 248 */   SDLK_UNKNOWN,
    /* 249 */   SDLK_UNKNOWN,
    /* 250 */   SDLK_UNKNOWN,
    /* 251 */   SDLK_UNKNOWN,
    /* 252 */   SDLK_UNKNOWN,
    /* 253 */   SDLK_UNKNOWN,
    /* 254 */   SDLK_UNKNOWN,
    /* 255 */   SDLK_UNKNOWN
};

/* Found mostly by experimentation with X.org on Linux (Fedora Core 4 and
   Ubuntu Dapper) on PC and PPC Mac hardware, some parts (especially about
   the "multimedia"/"internet" keys) from various sources on the web.
*/
static SDLKey xorgLinuxKeyCodeToSDLK[KeyCodeTableSize] = 
{
    /*   0 */   SDLK_UNKNOWN,
    /*   1 */   SDLK_UNKNOWN,
    /*   2 */   SDLK_UNKNOWN,
    /*   3 */   SDLK_UNKNOWN,
    /*   4 */   SDLK_UNKNOWN,
    /*   5 */   SDLK_UNKNOWN,
    /*   6 */   SDLK_UNKNOWN,
    /*   7 */   SDLK_UNKNOWN,
    /*   8 */   SDLK_UNKNOWN,
    /*   9 */   SDLK_ESCAPE,
    /*  10 */   SDLK_1,
    /*  11 */   SDLK_2,
    /*  12 */   SDLK_3,
    /*  13 */   SDLK_4,
    /*  14 */   SDLK_5,
    /*  15 */   SDLK_6,
    /*  16 */   SDLK_7,
    /*  17 */   SDLK_8,
    /*  18 */   SDLK_9,
    /*  19 */   SDLK_0,
    /*  20 */   SDLK_HYPHENMINUS,
    /*  21 */   SDLK_EQUALS,
    /*  22 */   SDLK_BACKSPACE,
    /*  23 */   SDLK_TAB,
    /*  24 */   SDLK_Q,
    /*  25 */   SDLK_W,
    /*  26 */   SDLK_E,
    /*  27 */   SDLK_R,
    /*  28 */   SDLK_T,
    /*  29 */   SDLK_Y,
    /*  30 */   SDLK_U,
    /*  31 */   SDLK_I,
    /*  32 */   SDLK_O,
    /*  33 */   SDLK_P,
    /*  34 */   SDLK_LEFTBRACKET,
    /*  35 */   SDLK_RIGHTBRACKET,
    /*  36 */   SDLK_RETURN,
    /*  37 */   SDLK_LCTRL,
    /*  38 */   SDLK_A,
    /*  39 */   SDLK_S,
    /*  40 */   SDLK_D,
    /*  41 */   SDLK_F,
    /*  42 */   SDLK_G,
    /*  43 */   SDLK_H,
    /*  44 */   SDLK_J,
    /*  45 */   SDLK_K,
    /*  46 */   SDLK_L,
    /*  47 */   SDLK_SEMICOLON,
    /*  48 */   SDLK_APOSTROPHE,
    /*  49 */   SDLK_GRAVE,
    /*  50 */   SDLK_LSHIFT,
    /*  51 */   SDLK_BACKSLASH,
    /*  52 */   SDLK_Z,
    /*  53 */   SDLK_X,
    /*  54 */   SDLK_C,
    /*  55 */   SDLK_V,
    /*  56 */   SDLK_B,
    /*  57 */   SDLK_N,
    /*  58 */   SDLK_M,
    /*  59 */   SDLK_COMMA,
    /*  60 */   SDLK_PERIOD,
    /*  61 */   SDLK_SLASH,
    /*  62 */   SDLK_RSHIFT,
    /*  63 */   SDLK_KP_MULTIPLY,
    /*  64 */   SDLK_LALT,
    /*  65 */   SDLK_SPACE,
    /*  66 */   SDLK_CAPSLOCK,
    /*  67 */   SDLK_F1,
    /*  68 */   SDLK_F2,
    /*  69 */   SDLK_F3,
    /*  70 */   SDLK_F4,
    /*  71 */   SDLK_F5,
    /*  72 */   SDLK_F6,
    /*  73 */   SDLK_F7,
    /*  74 */   SDLK_F8,
    /*  75 */   SDLK_F9,
    /*  76 */   SDLK_F10,
    /*  77 */   SDLK_KP_NUMLOCKCLEAR,
    /*  78 */   SDLK_SCROLLLOCK,
    /*  79 */   SDLK_KP_7,
    /*  80 */   SDLK_KP_8,
    /*  81 */   SDLK_KP_9,
    /*  82 */   SDLK_KP_MINUS,
    /*  83 */   SDLK_KP_4,
    /*  84 */   SDLK_KP_5,
    /*  85 */   SDLK_KP_6,
    /*  86 */   SDLK_KP_PLUS,
    /*  87 */   SDLK_KP_1,
    /*  88 */   SDLK_KP_2,
    /*  89 */   SDLK_KP_3,
    /*  90 */   SDLK_KP_0,
    /*  91 */   SDLK_KP_PERIOD,
    /*  92 */   SDLK_SYSREQ,
    /*  93 */   SDLK_MODE, /* is translated to XK_Mode_switch by my X server, but I have no keyboard that generates this code, so I'm not sure if this is correct */
    /*  94 */   SDLK_NONUSBACKSLASH,
    /*  95 */   SDLK_F11,
    /*  96 */   SDLK_F12,
    /*  97 */   SDLK_HOME,
    /*  98 */   SDLK_UP,
    /*  99 */   SDLK_PAGEUP,
    /* 100 */   SDLK_LEFT,
    /* 101 */   SDLK_BRIGHTNESSDOWN, /* on PowerBook G4 */
    /* 102 */   SDLK_RIGHT,
    /* 103 */   SDLK_END,
    /* 104 */   SDLK_DOWN,
    /* 105 */   SDLK_PAGEDOWN,
    /* 106 */   SDLK_INSERT,
    /* 107 */   SDLK_DELETE,
    /* 108 */   SDLK_KP_ENTER,
    /* 109 */   SDLK_RCTRL,
    /* 110 */   SDLK_PAUSE,
    /* 111 */   SDLK_PRINTSCREEN,
    /* 112 */   SDLK_KP_DIVIDE,
    /* 113 */   SDLK_RALT,
    /* 114 */   SDLK_UNKNOWN,
    /* 115 */   SDLK_LMETA,
    /* 116 */   SDLK_RMETA,
    /* 117 */   SDLK_APPLICATION,
    /* 118 */   SDLK_F13,
    /* 119 */   SDLK_F14,
    /* 120 */   SDLK_F15,
    /* 121 */   SDLK_F16,
    /* 122 */   SDLK_F17,
    /* 123 */   SDLK_UNKNOWN,
    /* 124 */   SDLK_UNKNOWN, /* is translated to XK_ISO_Level3_Shift by my X server, but I have no keyboard that generates this code, so I don't know what the correct SDLK_* for it is */
    /* 125 */   SDLK_UNKNOWN,
    /* 126 */   SDLK_KP_EQUALS,
    /* 127 */   SDLK_UNKNOWN,
    /* 128 */   SDLK_UNKNOWN,
    /* 129 */   SDLK_UNKNOWN,
    /* 130 */   SDLK_UNKNOWN,
    /* 131 */   SDLK_UNKNOWN,
    /* 132 */   SDLK_UNKNOWN,
    /* 133 */   SDLK_INTERNATIONAL3, /* Yen */
    /* 134 */   SDLK_UNKNOWN,
    /* 135 */   SDLK_AGAIN,
    /* 136 */   SDLK_UNDO,
    /* 137 */   SDLK_UNKNOWN,
    /* 138 */   SDLK_UNKNOWN,
    /* 139 */   SDLK_UNKNOWN,
    /* 140 */   SDLK_UNKNOWN,
    /* 141 */   SDLK_UNKNOWN,
    /* 142 */   SDLK_UNKNOWN,
    /* 143 */   SDLK_UNKNOWN,
    /* 144 */   SDLK_AUDIOPREV,
    /* 145 */   SDLK_UNKNOWN,
    /* 146 */   SDLK_UNKNOWN,
    /* 147 */   SDLK_UNKNOWN,
    /* 148 */   SDLK_UNKNOWN,
    /* 149 */   SDLK_UNKNOWN,
    /* 150 */   SDLK_UNKNOWN,
    /* 151 */   SDLK_UNKNOWN,
    /* 152 */   SDLK_UNKNOWN,
    /* 153 */   SDLK_AUDIONEXT,
    /* 154 */   SDLK_UNKNOWN,
    /* 155 */   SDLK_UNKNOWN,
    /* 156 */   SDLK_UNKNOWN,
    /* 157 */   SDLK_KP_EQUALS, /* on PowerBook G4 */
    /* 158 */   SDLK_UNKNOWN,
    /* 159 */   SDLK_UNKNOWN,
    /* 160 */   SDLK_MUTE,
    /* 161 */   SDLK_CALC,
    /* 162 */   SDLK_AUDIOPLAY,
    /* 163 */   SDLK_UNKNOWN,
    /* 164 */   SDLK_AUDIOSTOP,
    /* 165 */   SDLK_UNKNOWN,
    /* 166 */   SDLK_UNKNOWN,
    /* 167 */   SDLK_UNKNOWN,
    /* 168 */   SDLK_UNKNOWN,
    /* 169 */   SDLK_UNKNOWN,
    /* 170 */   SDLK_UNKNOWN,
    /* 171 */   SDLK_UNKNOWN,
    /* 172 */   SDLK_UNKNOWN,
    /* 173 */   SDLK_UNKNOWN,
    /* 174 */   SDLK_VOLUMEDOWN,
    /* 175 */   SDLK_UNKNOWN,
    /* 176 */   SDLK_VOLUMEUP,
    /* 177 */   SDLK_UNKNOWN,
    /* 178 */   SDLK_WWW,
    /* 179 */   SDLK_UNKNOWN,
    /* 180 */   SDLK_UNKNOWN,
    /* 181 */   SDLK_UNKNOWN,
    /* 182 */   SDLK_UNKNOWN,
    /* 183 */   SDLK_UNKNOWN,
    /* 184 */   SDLK_UNKNOWN,
    /* 185 */   SDLK_UNKNOWN,
    /* 186 */   SDLK_UNKNOWN,
    /* 187 */   SDLK_HELP,
    /* 188 */   SDLK_UNKNOWN,
    /* 189 */   SDLK_UNKNOWN,
    /* 190 */   SDLK_UNKNOWN,
    /* 191 */   SDLK_UNKNOWN,
    /* 192 */   SDLK_UNKNOWN,
    /* 193 */   SDLK_UNKNOWN,
    /* 194 */   SDLK_UNKNOWN,
    /* 195 */   SDLK_UNKNOWN,
    /* 196 */   SDLK_UNKNOWN,
    /* 197 */   SDLK_UNKNOWN,
    /* 198 */   SDLK_UNKNOWN,
    /* 199 */   SDLK_UNKNOWN,
    /* 200 */   SDLK_UNKNOWN,
    /* 201 */   SDLK_UNKNOWN,
    /* 202 */   SDLK_UNKNOWN,
    /* 203 */   SDLK_UNKNOWN,
    /* 204 */   SDLK_EJECT, /* on PowerBook G4 */
    /* 205 */   SDLK_UNKNOWN,
    /* 206 */   SDLK_UNKNOWN,
    /* 207 */   SDLK_UNKNOWN,
    /* 208 */   SDLK_UNKNOWN,
    /* 209 */   SDLK_UNKNOWN,
    /* 210 */   SDLK_UNKNOWN,
    /* 211 */   SDLK_UNKNOWN,
    /* 212 */   SDLK_BRIGHTNESSUP, /* on PowerBook G4 */
    /* 213 */   SDLK_UNKNOWN,
    /* 214 */   SDLK_DISPLAYSWITCH, /* on PowerBook G4 */
    /* 215 */   SDLK_KBDILLUMTOGGLE,
    /* 216 */   SDLK_KBDILLUMDOWN,
    /* 217 */   SDLK_KBDILLUMUP,
    /* 218 */   SDLK_UNKNOWN,
    /* 219 */   SDLK_UNKNOWN,
    /* 220 */   SDLK_UNKNOWN,
    /* 221 */   SDLK_UNKNOWN,
    /* 222 */   SDLK_POWER,
    /* 223 */   SDLK_SLEEP,
    /* 224 */   SDLK_UNKNOWN,
    /* 225 */   SDLK_UNKNOWN,
    /* 226 */   SDLK_UNKNOWN,
    /* 227 */   SDLK_UNKNOWN,
    /* 228 */   SDLK_UNKNOWN,
    /* 229 */   SDLK_SEARCH,
    /* 230 */   SDLK_BOOKMARKS,
    /* 231 */   SDLK_BROWSERRELOAD,
    /* 232 */   SDLK_BROWSERSTOP,
    /* 233 */   SDLK_BROWSERFORWARD,
    /* 234 */   SDLK_BROWSERBACK,
    /* 235 */   SDLK_COMPUTER,
    /* 236 */   SDLK_EMAIL,
    /* 237 */   SDLK_MEDIA,
    /* 238 */   SDLK_UNKNOWN,
    /* 239 */   SDLK_UNKNOWN,
    /* 240 */   SDLK_UNKNOWN,
    /* 241 */   SDLK_UNKNOWN,
    /* 242 */   SDLK_UNKNOWN,
    /* 243 */   SDLK_UNKNOWN,
    /* 244 */   SDLK_UNKNOWN,
    /* 245 */   SDLK_UNKNOWN,
    /* 246 */   SDLK_UNKNOWN,
    /* 247 */   SDLK_UNKNOWN,
    /* 248 */   SDLK_UNKNOWN,
    /* 249 */   SDLK_UNKNOWN,
    /* 250 */   SDLK_UNKNOWN,
    /* 251 */   SDLK_UNKNOWN,
    /* 252 */   SDLK_UNKNOWN,
    /* 253 */   SDLK_UNKNOWN,
    /* 254 */   SDLK_UNKNOWN,
    /* 255 */   SDLK_UNKNOWN
};

/*---------------------------------------------------------------------------*/

/* Used by X11_KeySymToSDLKey(). This is a hybrid of a KeySym-to-layout-key
    mapping (needed in X11_GetLayoutKey()) and a fallback KeySym-to-physical-key
    mapping under the assumption of a US keyboard layout (needed in
    X11_InitKeyboard()). If for a given KeySym...
    - the layout and physical codes are the same (must be an SDLK_ constant):
      there is one entry,
    - the layout and physical codes differ: there are two entries, with the
      layout one first,
    - there is only a physical code in the table (must be an SDLK_ constant):
      it's marked by X11_KEY_PHYSICAL_ONLY_BIT (this is the case when the layout
      key code is handled by KeySymToUcs4()),
    - there is only a layout code in the table (can't be an SDLK_ constant):
      recognizable by the absence of SDL_KEY_CAN_BE_PHYSICAL_BIT.
    This list is sorted by KeySym to allow a binary search.
*/
#define X11_KEY_PHYSICAL_ONLY_BIT SDL_KEY_LAYOUT_SPECIAL_BIT
/* SDL_KEY_LAYOUT_SPECIAL_BIT cannot possibly occur in an SDLK_ constant, so we may repurpose that bit for our own use. */
static struct
{
    KeySym sym;
    SDLKey key;
} keySymToSDLKey[KeyCodeTableSize] = 
{
    /* 0x00xx */
    {XK_space, SDLK_SPACE},
    {XK_apostrophe, SDLK_APOSTROPHE | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_comma, SDLK_COMMA | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_minus, SDLK_HYPHENMINUS | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_period, SDLK_PERIOD | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_slash, SDLK_SLASH | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_0, SDLK_0 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_1, SDLK_1 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_2, SDLK_2 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_3, SDLK_3 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_4, SDLK_4 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_5, SDLK_5 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_6, SDLK_6 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_7, SDLK_7 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_8, SDLK_8 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_9, SDLK_9 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_semicolon, SDLK_SEMICOLON | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_less, SDLK_NONUSBACKSLASH | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_equal, SDLK_EQUALS | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_bracketleft, SDLK_LEFTBRACKET | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_backslash, SDLK_BACKSLASH | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_bracketright, SDLK_RIGHTBRACKET | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_grave, SDLK_GRAVE | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_a, SDLK_A | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_b, SDLK_B | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_c, SDLK_C | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_d, SDLK_D | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_e, SDLK_E | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_f, SDLK_F | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_g, SDLK_G | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_h, SDLK_H | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_i, SDLK_I | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_j, SDLK_J | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_k, SDLK_K | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_l, SDLK_L | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_m, SDLK_M | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_n, SDLK_N | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_o, SDLK_O | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_p, SDLK_P | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_q, SDLK_Q | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_r, SDLK_R | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_s, SDLK_S | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_t, SDLK_T | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_u, SDLK_U | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_v, SDLK_V | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_w, SDLK_W | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_x, SDLK_X | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_y, SDLK_Y | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_z, SDLK_Z | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_section, SDLK_NONUSBACKSLASH | X11_KEY_PHYSICAL_ONLY_BIT},
        /* 0xFExx */
    {XK_ISO_Level3_Shift, SDLK_RALT},
    {XK_dead_grave, '`'},
    {XK_dead_acute, 0xB4},
    {XK_dead_circumflex, '^'},
    {XK_dead_tilde, '~'},
    {XK_dead_macron, 0xAF},
    {XK_dead_breve, 0x2D8},
    {XK_dead_abovedot, 0x2D9},
    {XK_dead_diaeresis, 0xA8},
    {XK_dead_abovering, 0x2DA},
    {XK_dead_doubleacute, 0x2DD},
    {XK_dead_caron, 0x2C7},
    {XK_dead_cedilla, 0xB8},
    {XK_dead_ogonek, 0x2DB},
    {XK_dead_iota, 0x3B9},
    {XK_dead_voiced_sound, 0x309B},
    {XK_dead_semivoiced_sound, 0x309C},
    {XK_dead_belowdot, 0xB7},        /* that's actually MIDDLE DOT,
                                        but I haven't found a
                                        non-combining DOT BELOW
                                        XK_dead_hook, XK_dead_horn: I
                                        haven't found non-combining
                                        HOOK and HORN characters */
    /* 0xFFxx */
    {XK_BackSpace, SDLK_BACKSPACE},
    {XK_Tab, SDLK_TAB},
    {XK_Return, SDLK_RETURN},
    {XK_Pause, SDLK_PAUSE},
    {XK_Scroll_Lock, SDLK_SCROLLLOCK},
    {XK_Escape, SDLK_ESCAPE},
    {XK_Home, SDLK_HOME},
    {XK_Left, SDLK_LEFT},
    {XK_Up, SDLK_UP},
    {XK_Right, SDLK_RIGHT},
    {XK_Down, SDLK_DOWN},
    {XK_Page_Up, SDLK_PAGEUP},
    {XK_Page_Down, SDLK_PAGEDOWN},
    {XK_End, SDLK_END},
    {XK_Print, SDLK_PRINTSCREEN},
    {XK_Insert, SDLK_INSERT},
    {XK_Menu, SDLK_APPLICATION},
    {XK_Break, SDLK_PAUSE},
    {XK_Mode_switch, SDLK_MODE},
    {XK_Num_Lock, SDLK_KP_NUMLOCKCLEAR},
    {XK_KP_Enter, SDLK_KP_ENTER},
    {XK_KP_Home, SDLK_KP_7 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_Left, SDLK_KP_4 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_Up, SDLK_KP_8 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_Right, SDLK_KP_6 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_Down, SDLK_KP_2 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_Page_Up, SDLK_KP_9 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_Page_Down, SDLK_KP_3 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_End, SDLK_KP_1 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_Begin, SDLK_KP_5 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_Insert, SDLK_KP_0 | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_Delete, SDLK_KP_PERIOD | X11_KEY_PHYSICAL_ONLY_BIT},
    {XK_KP_Multiply, '*'},
    {XK_KP_Multiply, SDLK_KP_MULTIPLY},
    {XK_KP_Add, '+'},
    {XK_KP_Add, SDLK_KP_PLUS},
    {XK_KP_Separator, '.'},
    {XK_KP_Separator, SDLK_KP_PERIOD},
    {XK_KP_Subtract, '-'},
    {XK_KP_Subtract, SDLK_KP_MINUS},
    {XK_KP_Decimal, '.'},
    {XK_KP_Decimal, SDLK_KP_PERIOD},
    {XK_KP_Divide, '/'},
    {XK_KP_Divide, SDLK_KP_DIVIDE},
    {XK_KP_0, '0'},
    {XK_KP_0, SDLK_KP_0},
    {XK_KP_1, '1'},
    {XK_KP_1, SDLK_KP_1},
    {XK_KP_2, '2'},
    {XK_KP_2, SDLK_KP_2},
    {XK_KP_3, '3'},
    {XK_KP_3, SDLK_KP_3},
    {XK_KP_4, '4'},
    {XK_KP_4, SDLK_KP_4},
    {XK_KP_5, '5'},
    {XK_KP_5, SDLK_KP_5},
    {XK_KP_6, '6'},
    {XK_KP_6, SDLK_KP_6},
    {XK_KP_7, '7'},
    {XK_KP_7, SDLK_KP_7},
    {XK_KP_8, '8'},
    {XK_KP_8, SDLK_KP_8},
    {XK_KP_9, '9'},
    {XK_KP_9, SDLK_KP_9},
    {XK_KP_Equal, '='},
    {XK_KP_Equal, SDLK_KP_EQUALS},
    {XK_F1, SDLK_F1},
    {XK_F2, SDLK_F2},
    {XK_F3, SDLK_F3},
    {XK_F4, SDLK_F4},
    {XK_F5, SDLK_F5},
    {XK_F6, SDLK_F6},
    {XK_F7, SDLK_F7},
    {XK_F8, SDLK_F8},
    {XK_F9, SDLK_F9},
    {XK_F10, SDLK_F10},
    {XK_F11, SDLK_F11},
    {XK_F12, SDLK_F12},
    {XK_F13, SDLK_F13},
    {XK_F14, SDLK_F14},
    {XK_F15, SDLK_F15},
    {XK_F16, SDLK_F16},
    {XK_F17, SDLK_F17},
    {XK_F18, SDLK_F18},
    {XK_F19, SDLK_F19},
    {XK_F20, SDLK_F20},
    {XK_F21, SDLK_F21},
    {XK_F22, SDLK_F22},
    {XK_F23, SDLK_F23},
    {XK_F24, SDLK_F24},
    {XK_Shift_L, SDLK_LSHIFT},
    {XK_Shift_R, SDLK_RSHIFT},
    {XK_Control_L, SDLK_LCTRL},
    {XK_Control_R, SDLK_RCTRL},
    {XK_Caps_Lock, SDLK_CAPSLOCK},
    {XK_Shift_Lock, SDLK_CAPSLOCK},
    {XK_Meta_L, SDLK_LMETA},
    {XK_Meta_R, SDLK_RMETA},
    {XK_Alt_L, SDLK_LALT},
    {XK_Alt_R, SDLK_RALT},
    {XK_Super_L, SDLK_LMETA},
    {XK_Super_R, SDLK_RMETA},
    {XK_Hyper_L, SDLK_LMETA},
    {XK_Hyper_R, SDLK_RMETA},
    {XK_Delete, SDLK_DELETE},
    {0x1000003, SDLK_KP_ENTER}   /* keyboard enter on Mac OS X */
};

/* *INDENT-ON* */

/* 
   Used for two purposes: - by X11_GetLayoutKey(), with physical =
   false, to convert a KeySym to the corresponding layout key code
   (SDLK_ ones and some character ones - most character KeySyms are
   handled by X11_KeySymToUcs4() after this function returns
   SDLK_UNKNOWN for them).  - by X11_InitKeyboard(), with physical =
   true, to build a makeshift translation table based on the KeySyms
   when none of the predefined KeyCode- to-SDLKey tables fits. This
   is *not* correct anywhere but on a US layout, since the
   translation table deals with physical key codes, while the X11
   KeySym corresponds to our concept of a layout key code, but it's
   better than nothing.
*/

static SDLKey
X11_KeySymToSDLKey(KeySym sym, SDL_bool physical)
{
    /* Do a binary search for sym in the keySymToSDLKey table */
    SDLKey key = SDLK_UNKNOWN;
    int start = -1;
    int end = SDL_arraysize(keySymToSDLKey);
    int i;
    /* Invariant: keySymToSDLKey[start].sym < sym <
       keySymToSDLKey[end].sym (imagine ...[-1] = -inf and
       ...[arraysize] = inf, these entries needn't be there in reality
       because they're never accessed) */
    while (end > start + 1) {
        i = (start + end) / 2;
        if (keySymToSDLKey[i].sym == sym) {
            /* found an entry, if it's the second of two back up to the first */
            if (keySymToSDLKey[i - 1].sym == sym)
                i--;
            /* if there are two, the physical one is the second */
            if (physical && keySymToSDLKey[i + 1].sym == sym)
                i++;
            key = keySymToSDLKey[i].key;
            break;
        } else if (keySymToSDLKey[i].sym < sym)
            start = i;
        else
            end = i;
    }

    /* if we're being asked for a layout key code, but the table only
       has a physical one, or vice versa, return SDLK_UNKNOWN) */

    if (!physical && ((key & X11_KEY_PHYSICAL_ONLY_BIT) != 0)
        || physical && ((key & SDL_KEY_CAN_BE_PHYSICAL_BIT) == 0))
        key = SDLK_UNKNOWN;
    key &= ~X11_KEY_PHYSICAL_ONLY_BIT;
    return key;
}

int
X11_InitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_Keyboard keyboard;
    SDLKey **table;
    SDLKey *foundTable;
    int i;
    int code;
    SDLKey sdlkey;

    XAutoRepeatOn(data->display);

    /* A random collection of KeySym/SDLKey pairs that should be valid
       in any keyboard layout (if this isn't the case on yours,
       please adjust). Using XKeysymToKeycode on these KeySyms
       creates a "fingerprint" of the X server's key-to-KeyCode
       mapping which is then matched against all our predefined
       KeyCodeToSDLK tables to find the right one (if any).
     */

/* *INDENT-OFF* */
    struct
    {
        KeySym sym;
        SDLKey key;
    } fingerprint[] = 
      {
        {XK_Tab, SDLK_TAB}, 
        {XK_Return, SDLK_RETURN}, 
        {XK_Escape, SDLK_ESCAPE}, 
        {XK_space, SDLK_SPACE}
    };
/* *INDENT-ON* */

    SDL_zero(keyboard);
    data->keyboard = SDL_AddKeyboard(&keyboard, -1);

    /* Determine which X11 KeyCode to SDL physical key code table to use */

    foundTable = NULL;
    table = keyCodeToSDLKeyTables;
    while ((NULL == foundTable) && (NULL != (*table))) {
        foundTable = *table;
        for (i = 0; i < SDL_arraysize(fingerprint); i++) {
            code = XKeysymToKeycode(data->display, fingerprint[i].sym);
            if ((code != 0) && ((*table)[code] != fingerprint[i].key)) {
                foundTable = NULL;
                break;
            }
        }
        table++;
    }

    if (NULL != foundTable) {
        /* Found a suitable one among the predefined tables */
        data->keyCodeToSDLKTable = foundTable;
    } else {
        /* No suitable table found - build a makeshift table based on
           the KeySyms, assuming a US keyboard layout */

#if 1
        fprintf(stderr,
                "The key codes of your X server are unknown to SDL. Keys may not be recognized properly. To help get this fixed, report this to the SDL mailing list <sdl@libsdl.org> or to Christian Walther <cwalther@gmx.ch>.\n");
#endif
        data->keyCodeToSDLKTable =
            SDL_malloc(KeyCodeTableSize * sizeof(SDLKey));
        if (data->keyCodeToSDLKTable == NULL) {
            SDL_OutOfMemory();
            return -1;
        }
        for (code = KeyCodeTableSize; code >= 0; code--) {
            data->keyCodeToSDLKTable[code] =
                X11_KeySymToSDLKey(XKeycodeToKeysym(data->display, code, 0),
                                   SDL_TRUE);
        }
    }

    /* Set some non-default key names */

    for (code = 0; code < KeyCodeTableSize; code++) {
        sdlkey = data->keyCodeToSDLKTable[code];
        switch (sdlkey) {
            /* The SDLK_*META keys are used as XK_Meta_* by some X
               servers, as XK_Super_* by others */
        case SDLK_LMETA:
        case SDLK_RMETA:
            switch (XKeycodeToKeysym(data->display, code, 0)) {
                /* nothing to do for XK_Meta_* because that's already the default name */
            case XK_Super_L:
                SDL_SetKeyName(sdlkey, "left super");
                break;
            case XK_Super_R:
                SDL_SetKeyName(sdlkey, "right super");
                break;
            }
            break;
        }
    }
    SDL_SetKeyName(SDLK_APPLICATION, "menu");

    return 0;
}

SDLKey
X11_GetLayoutKey(_THIS, SDLKey physicalKey)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int code = 0;
    KeySym sym;
    SDLKey layoutKey;

    switch (physicalKey) {
    case SDLK_UNKNOWN:
        return physicalKey;

        /* Shortcut handling of keypad numbers because on my PC their
           primary KeySyms are not the numbers that I want but
           XK_KP_Home, XK_KP_Up etc. The downside is that this gets us
           latin numerals even on e.g. an Arabic layout. */
    case SDLK_KP_1:
        return '1';
    case SDLK_KP_2:
        return '2';
    case SDLK_KP_3:
        return '3';
    case SDLK_KP_4:
        return '4';
    case SDLK_KP_5:
        return '5';
    case SDLK_KP_6:
        return '6';
    case SDLK_KP_7:
        return '7';
    case SDLK_KP_8:
        return '8';
    case SDLK_KP_9:
        return '9';
    case SDLK_KP_0:
        return '0';
    case SDLK_KP_PERIOD:
        return '.';
    default:
        break;                  /* just to avoid a compiler warning */
    }

    /* Look up physicalKey to get an X11 KeyCode - linear search isn't
       terribly efficient, this might have to be optimized. */
    while ((code < KeyCodeTableSize) &&
           (physicalKey != data->keyCodeToSDLKTable[code])) {
        code++;
    }

    if (code == KeyCodeTableSize) {
        return physicalKey;
    }
    /* Get the corresponding KeySym - this is where the keyboard
       layout comes into play */
    sym = XKeycodeToKeysym(data->display, code, 0);

    /* Try our own KeySym-to-layout-key-code table: it handles all
       keys whose layout code is an SDLK_ one, including a few where
       X11_KeySymToUcs4() would yield a character, but not a suitable
       one as a key name (e.g. space), and some that are character
       keys for our purposes, but aren't handled by X11_KeySymToUcs4()
       (dead keys, keypad operations). */

    layoutKey = X11_KeySymToSDLKey(sym, SDL_FALSE);

    /* If it wasn't handled by X11_KeySymToSDLKey(), it's most
       probably a plain character KeySym that X11_KeySymToUcs4()
       (ripped from X.org) knows. */

    if (layoutKey == SDLK_UNKNOWN) {
        unsigned int ucs = X11_KeySymToUcs4(sym);
        if (ucs != 0)
            layoutKey = (SDLKey) ucs;
    }

    /* Still no success? Give up. */
    if (layoutKey == SDLK_UNKNOWN) {
        return physicalKey;
    }

    return layoutKey;
}

void
X11_QuitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    if (data->keyCodeToSDLKTable != NULL) {
        /* If it's not one of the predefined tables, it was malloced
           and must be freed */
        SDLKey **table = keyCodeToSDLKeyTables;
        while (*table != NULL && *table != data->keyCodeToSDLKTable) {
            table++;
        }
        if (*table == NULL)
            SDL_free(data->keyCodeToSDLKTable);
        data->keyCodeToSDLKTable = NULL;
    }

    SDL_DelKeyboard(data->keyboard);
}

/* vi: set ts=4 sw=4 expandtab: */
