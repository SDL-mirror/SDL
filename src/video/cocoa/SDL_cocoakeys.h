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

/* These are the Macintosh key scancode constants -- from Inside Macintosh */

#define KEY_ESCAPE		0x35
#define KEY_F1			0x7A
#define KEY_F2			0x78
#define KEY_F3			0x63
#define KEY_F4			0x76
#define KEY_F5			0x60
#define KEY_F6			0x61
#define KEY_F7			0x62
#define KEY_F8			0x64
#define KEY_F9			0x65
#define KEY_F10			0x6D
#define KEY_F11			0x67
#define KEY_F12			0x6F
#define KEY_PRINT		0x69
#define KEY_SCROLLOCK    0x6B
#define KEY_PAUSE		0x71
#define KEY_POWER		0x7F
#define KEY_BACKQUOTE	0x32
#define KEY_1			0x12
#define KEY_2			0x13
#define KEY_3			0x14
#define KEY_4			0x15
#define KEY_5			0x17
#define KEY_6			0x16
#define KEY_7			0x1A
#define KEY_8			0x1C
#define KEY_9			0x19
#define KEY_0			0x1D
#define KEY_MINUS		0x1B
#define KEY_EQUALS		0x18
#define KEY_BACKSPACE	0x33
#define KEY_INSERT		0x72
#define KEY_HOME			0x73
#define KEY_PAGEUP		0x74
#define KEY_NUMLOCK		0x47
#define KEY_KP_EQUALS	0x51
#define KEY_KP_DIVIDE	0x4B
#define KEY_KP_MULTIPLY	0x43
#define KEY_TAB			0x30
#define KEY_q			0x0C
#define KEY_w			0x0D
#define KEY_e			0x0E
#define KEY_r			0x0F
#define KEY_t			0x11
#define KEY_y			0x10
#define KEY_u			0x20
#define KEY_i			0x22
#define KEY_o			0x1F
#define KEY_p			0x23
#define KEY_LEFTBRACKET	0x21
#define KEY_RIGHTBRACKET	0x1E
#define KEY_BACKSLASH	0x2A
#define KEY_DELETE		0x75
#define KEY_END			0x77
#define KEY_PAGEDOWN		0x79
#define KEY_KP7			0x59
#define KEY_KP8			0x5B
#define KEY_KP9			0x5C
#define KEY_KP_MINUS		0x4E
#define KEY_CAPSLOCK		0x39
#define KEY_a			0x00
#define KEY_s			0x01
#define KEY_d			0x02
#define KEY_f			0x03
#define KEY_g			0x05
#define KEY_h			0x04
#define KEY_j			0x26
#define KEY_k			0x28
#define KEY_l			0x25
#define KEY_SEMICOLON	0x29
#define KEY_QUOTE		0x27
#define KEY_RETURN		0x24
#define KEY_KP4			0x56
#define KEY_KP5			0x57
#define KEY_KP6			0x58
#define KEY_KP_PLUS		0x45
#define KEY_LSHIFT		0x38
#define KEY_z			0x06
#define KEY_x			0x07
#define KEY_c			0x08
#define KEY_v			0x09
#define KEY_b			0x0B
#define KEY_n			0x2D
#define KEY_m			0x2E
#define KEY_COMMA		0x2B
#define KEY_PERIOD		0x2F
#define KEY_SLASH		0x2C
#if 1                           /* Panther now defines right side keys */
#define KEY_RSHIFT		0x3C
#endif
#define KEY_UP			0x7E
#define KEY_KP1			0x53
#define KEY_KP2			0x54
#define KEY_KP3			0x55
#define KEY_KP_ENTER		0x4C
#define KEY_LCTRL		0x3B
#define KEY_LALT			0x3A
#define KEY_LMETA		0x37
#define KEY_SPACE		0x31
#if 1                           /* Panther now defines right side keys */
#define KEY_RMETA		0x36
#define KEY_RALT			0x3D
#define KEY_RCTRL		0x3E
#endif
#define KEY_LEFT			0x7B
#define KEY_DOWN			0x7D
#define KEY_RIGHT		0x7C
#define KEY_KP0			0x52
#define KEY_KP_PERIOD	0x41

/* Wierd, these keys are on my iBook under Mac OS X */
#define KEY_IBOOK_ENTER		0x34
#define KEY_IBOOK_LEFT		0x3B
#define KEY_IBOOK_RIGHT		0x3C
#define KEY_IBOOK_DOWN		0x3D
#define KEY_IBOOK_UP			0x3E
