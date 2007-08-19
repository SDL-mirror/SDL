/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2007 Sam Lantinga

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

    Christian Walther
    cwalther@gmx.ch
*/

/* Names for the physical SDLKey constants, returned by SDL_GetKeyName().
   The strings are in UTF-8 encoding.
   This table can (and should) be modified by a video driver in its VideoInit()
   function using SDL_SetKeyName() to account for platform-dependent (but
   layout-independent) key names.
   
   The physical SDLKey codes can be divided into two groups:
   - codes that occur both as physical and as layout keys. These have their
     real, "user-readable" display name here.
   - codes that only occur as physical keys, i.e. are never returned by
     SDL_GetLayoutKey() if the backend implements it properly. These names are
     therefore never returned by SDL_GetKeyName(SDL_GetLayoutKey(
     event.key.keysym.sym)), the proper way of displaying a key name to the
     user, but only by SDL_GetKeyName(event.key.keysym.sym), which is only
     useful for debugging purposes. To emphasize this, these codes are named by
     their "programmer-readable" SDLK_ constants here rather than by a
     "user-readable" display name.
 */

/* *INDENT-OFF* */
static const char *SDL_keynames[SDLK_LAST] = {
    /*   0 */   "", /* SDL_PK_NONE */
    /*   1 */   "unknown key",
    /*   2 */   NULL, /* unused */
    /*   3 */   NULL, /* unused */
    /*   4 */   "SDLK_A",
    /*   5 */   "SDLK_B",
    /*   6 */   "SDLK_C",
    /*   7 */   "SDLK_D",
    /*   8 */   "SDLK_E",
    /*   9 */   "SDLK_F",
    /*  10 */   "SDLK_G",
    /*  11 */   "SDLK_H",
    /*  12 */   "SDLK_I",
    /*  13 */   "SDLK_J",
    /*  14 */   "SDLK_K",
    /*  15 */   "SDLK_L",
    /*  16 */   "SDLK_M",
    /*  17 */   "SDLK_N",
    /*  18 */   "SDLK_O",
    /*  19 */   "SDLK_P",
    /*  20 */   "SDLK_Q",
    /*  21 */   "SDLK_R",
    /*  22 */   "SDLK_S",
    /*  23 */   "SDLK_T",
    /*  24 */   "SDLK_U",
    /*  25 */   "SDLK_V",
    /*  26 */   "SDLK_W",
    /*  27 */   "SDLK_X",
    /*  28 */   "SDLK_Y",
    /*  29 */   "SDLK_Z",
    /*  30 */   "SDLK_1",
    /*  31 */   "SDLK_2",
    /*  32 */   "SDLK_3",
    /*  33 */   "SDLK_4",
    /*  34 */   "SDLK_5",
    /*  35 */   "SDLK_6",
    /*  36 */   "SDLK_7",
    /*  37 */   "SDLK_8",
    /*  38 */   "SDLK_9",
    /*  39 */   "SDLK_0",
    /*  40 */   "return",
    /*  41 */   "escape",
    /*  42 */   "backspace",
    /*  43 */   "tab",
    /*  44 */   "space",
    /*  45 */   "SDLK_HYPHENMINUS",
    /*  46 */   "SDLK_EQUALS",
    /*  47 */   "SDLK_LEFTBRACKET",
    /*  48 */   "SDLK_RIGHTBRACKET",
    /*  49 */   "SDLK_BACKSLASH",
    /*  50 */   "SDLK_NONUSHASH",
    /*  51 */   "SDLK_SEMICOLON",
    /*  52 */   "SDLK_APOSTROPHE",
    /*  53 */   "SDLK_GRAVE",
    /*  54 */   "SDLK_COMMA",
    /*  55 */   "SDLK_PERIOD",
    /*  56 */   "SDLK_SLASH",
    /*  57 */   "caps lock",
    /*  58 */   "F1",
    /*  59 */   "F2",
    /*  60 */   "F3",
    /*  61 */   "F4",
    /*  62 */   "F5",
    /*  63 */   "F6",
    /*  64 */   "F7",
    /*  65 */   "F8",
    /*  66 */   "F9",
    /*  67 */   "F10",
    /*  68 */   "F11",
    /*  69 */   "F12",
    /*  70 */   "print screen",
    /*  71 */   "scroll lock",
    /*  72 */   "pause",
    /*  73 */   "insert",
    /*  74 */   "home",
    /*  75 */   "page up",
    /*  76 */   "delete",
    /*  77 */   "end",
    /*  78 */   "page down",
    /*  79 */   "right",
    /*  80 */   "left",
    /*  81 */   "down",
    /*  82 */   "up",
    /*  83 */   "num lock",
    /*  84 */   "SDLK_KP_DIVIDE",
    /*  85 */   "SDLK_KP_MULTIPLY",
    /*  86 */   "SDLK_KP_MINUS",
    /*  87 */   "SDLK_KP_PLUS",
    /*  88 */   "enter",
    /*  89 */   "SDLK_KP_1",
    /*  90 */   "SDLK_KP_2",
    /*  91 */   "SDLK_KP_3",
    /*  92 */   "SDLK_KP_4",
    /*  93 */   "SDLK_KP_5",
    /*  94 */   "SDLK_KP_6",
    /*  95 */   "SDLK_KP_7",
    /*  96 */   "SDLK_KP_8",
    /*  97 */   "SDLK_KP_9",
    /*  98 */   "SDLK_KP_0",
    /*  99 */   "SDLK_KP_PERIOD",
    /* 100 */   "SDLK_NONUSBACKSLASH",
    /* 101 */   "application",
    /* 102 */   "power",
    /* 103 */   "SDLK_KP_EQUALS",
    /* 104 */   "F13",
    /* 105 */   "F14",
    /* 106 */   "F15",
    /* 107 */   "F16",
    /* 108 */   "F17",
    /* 109 */   "F18",
    /* 110 */   "F19",
    /* 111 */   "F20",
    /* 112 */   "F21",
    /* 113 */   "F22",
    /* 114 */   "F23",
    /* 115 */   "F24",
    /* 116 */   "execute",
    /* 117 */   "help",
    /* 118 */   "menu",
    /* 119 */   "select",
    /* 120 */   "stop",
    /* 121 */   "again",
    /* 122 */   "undo",
    /* 123 */   "cut",
    /* 124 */   "copy",
    /* 125 */   "paste",
    /* 126 */   "find",
    /* 127 */   "mute",
    /* 128 */   "volume up",
    /* 129 */   "volume down",
    /* 130 */   "caps lock", /* unused */
    /* 131 */   "num lock", /* unused */
    /* 132 */   "scroll lock", /* unused */
    /* 133 */   "SDLK_KP_COMMA",
    /* 134 */   "SDLK_KP_EQUALSAS400",
    /* 135 */   "international 1",
    /* 136 */   "international 2",
    /* 137 */   "international 3",
    /* 138 */   "international 4",
    /* 139 */   "international 5",
    /* 140 */   "international 6",
    /* 141 */   "international 7",
    /* 142 */   "international 8",
    /* 143 */   "international 9",
    /* 144 */   "lang 1",
    /* 145 */   "lang 2",
    /* 146 */   "lang 3",
    /* 147 */   "lang 4",
    /* 148 */   "lang 5",
    /* 149 */   "lang 6",
    /* 150 */   "lang 7",
    /* 151 */   "lang 8",
    /* 152 */   "lang 9",
    /* 153 */   "alt erase",
    /* 154 */   "sys req",
    /* 155 */   "cancel",
    /* 156 */   "clear",
    /* 157 */   "prior",
    /* 158 */   "return",
    /* 159 */   "separator",
    /* 160 */   "out",
    /* 161 */   "oper",
    /* 162 */   "clear/again",
    /* 163 */   "crsel/props",
    /* 164 */   "exsel",
    /* 165 */   NULL, /* unused */
    /* 166 */   NULL, /* unused */
    /* 167 */   NULL, /* unused */
    /* 168 */   NULL, /* unused */
    /* 169 */   NULL, /* unused */
    /* 170 */   NULL, /* unused */
    /* 171 */   NULL, /* unused */
    /* 172 */   NULL, /* unused */
    /* 173 */   NULL, /* unused */
    /* 174 */   NULL, /* unused */
    /* 175 */   NULL, /* unused */
    /* 176 */   "[00]",
    /* 177 */   "[000]",
    /* 178 */   "thousands separator",
    /* 179 */   "decimal separator",
    /* 180 */   "currency unit",
    /* 181 */   "currency sub-unit",
    /* 182 */   "[(]",
    /* 183 */   "[)]",
    /* 184 */   "[{]",
    /* 185 */   "[}]",
    /* 186 */   "[tab]",
    /* 187 */   "[backspace]",
    /* 188 */   "[A]",
    /* 189 */   "[B]",
    /* 190 */   "[C]",
    /* 191 */   "[D]",
    /* 192 */   "[E]",
    /* 193 */   "[F]",
    /* 194 */   "[XOR]",
    /* 195 */   "[^]",
    /* 196 */   "[%]",
    /* 197 */   "[<]",
    /* 198 */   "[>]",
    /* 199 */   "[&]",
    /* 200 */   "[&&]",
    /* 201 */   "[|]",
    /* 202 */   "[||]",
    /* 203 */   "[:]",
    /* 204 */   "[#]",
    /* 205 */   "[space]",
    /* 206 */   "[@]",
    /* 207 */   "[!]",
    /* 208 */   "[mem store]",
    /* 209 */   "[mem recall]",
    /* 210 */   "[mem clear]",
    /* 211 */   "[mem +]",
    /* 212 */   "[mem -]",
    /* 213 */   "[mem *]",
    /* 214 */   "[mem /]",
    /* 215 */   "[+/-]",
    /* 216 */   "[clear]",
    /* 217 */   "[clear entry]",
    /* 218 */   "[binary]",
    /* 219 */   "[octal]",
    /* 220 */   "[decimal]",
    /* 221 */   "[hexadecimal]",
    /* 222 */   NULL, /* unused */
    /* 223 */   NULL, /* unused */
    /* 224 */   "left ctrl",
    /* 225 */   "left shift",
    /* 226 */   "left alt",
    /* 227 */   "left meta",
    /* 228 */   "right ctrl",
    /* 229 */   "right shift",
    /* 230 */   "right alt",
    /* 231 */   "right meta",
    /* 232 */   "mode",
    /* 233 */   NULL, /* unused */
    /* 234 */   NULL, /* unused */
    /* 235 */   NULL, /* unused */
    /* 236 */   "brightness down",
    /* 237 */   "brightness up",
    /* 238 */   "display switch",
    /* 239 */   "kbd illum toggle",
    /* 240 */   "kbd illum down",
    /* 241 */   "kbd illum up",
    /* 242 */   "eject",
    /* 243 */   "sleep",
    /* 244 */   "play",
    /* 245 */   "stop",
    /* 246 */   "previous",
    /* 247 */   "next",
    /* 248 */   "calc",
    /* 249 */   "www",
    /* 250 */   "e-mail",
    /* 251 */   "media",
    /* 252 */   "computer",
    /* 253 */   "search",
    /* 254 */   "bookmarks",
    /* 255 */   "back",
    /* 256 */   "forward",
    /* 257 */   "reload",
    /* 258 */   "stop"
};
/* *INDENT-ON* */
