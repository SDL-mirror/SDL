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

/**
 * \file SDL_keysym.h
 */

#ifndef _SDL_keysym_h
#define _SDL_keysym_h

#include "SDL_stdinc.h"

/**
 * \typedef SDLKey
 *
 * \brief The SDL virtual key representation.
 *
 * Values of this type are used to represent keyboard keys, among other places
 * in the \link SDL_keysym::sym key.keysym.sym \endlink field of the SDL_Event
 * structure.
 *
 * There are two fundamental ways of referring to a key: First, a certain code
 * can stand for a key at a specific physical location on the keyboard,
 * independent of its label or what character it generates. These are the \e
 * physical key codes, comparable to the raw hardware scancodes that a keyboard
 * generates. Second, a code can refer to a key with a specific label,
 * generating a specific character or performing a specific function, which may
 * be located at different places on the keyboard, or not exist at all,
 * depending on what keyboard layout is used. These are \e layout key codes.
 *
 * There is a certain overlap between the sets of physical key codes and layout
 * key codes: \e return, \e tab, \e ctrl etc. are typically independent of the
 * keyboard layout and can be thought of as either a physical or a layout key.
 * Therefore, rather than having separate types with separate sets of constants
 * for physical keys and layout keys, a single type ::SDLKey is used for both
 * sets. The physical key codes (forming a well-known set of a few hundred
 * elements) are enumerated in enum ::SDLPhysicalKey. The set of layout key
 * codes is more diverse: For keys that don't generate characters, the layout
 * key code is equal to the physical key code, i.e. the same SDLK_* constants
 * from enum ::SDLPhysicalKey are used. For character keys, the layout key code
 * is equal to the Unicode code point of the character that is generated when
 * the key is pressed without shift or any other modifiers (for ASCII
 * characters, this can be directly written as a character literal like
 * <tt>'x'</tt>).
 *
 * The \link SDL_keysym::sym key.keysym.sym \endlink field of the SDL_Event
 * structure is always a physical key code. To get the layout key code for the
 * event, run that physical key code through SDL_GetLayoutKey(), which converts
 * it to a layout key code according to the current keyboard layout settings of
 * the OS. In particular, this is what should be done when displaying the name
 * of a key to the user: use
 * <tt>SDL_GetKeyName(SDL_GetLayoutKey(myPhysicalKeyCode))</tt>. Do not use
 * SDL_GetKeyName() directly on a physical key code (except for debugging
 * purposes), as the name returned by that will not correspond to how the key
 * is labeled on the user's keyboard.
 *
 * \par Example:
 * To implement WASD directional keys, it makes sense to use physical key
 * codes, so that the "forward" key will be above the "backward" key even
 * though, for example, it's labeled "Z", not "W", on a French keyboard:
 * \code
 * print("To go forward, press the %s key.", SDL_GetKeyName(SDL_GetLayoutKey(SDLK_W)));
 * ...
 * switch (event.key.keysym.sym) {
 *     case SDLK_W:
 *         forward();
 *         break;
 *     case SDLK_A:
 *         left();
 *         break;
 *     ...
 * }
 * \endcode
 * For keys based on mnemonics like "I for inventory" or "Z for zoom", use
 * layout key codes, so that the key labeled "Z" will zoom, whether it's at the
 * bottom left of the keyboard like on a US layout, or in the upper center like
 * on a German layout (but keep in mind that this forces your users to use a
 * keyboard layout where there \e is an I or Z key):
 * \code
 * print("To open the inventory, press the %s key.", SDL_GetKeyName('i'));
 * ...
 * switch (SDL_GetLayoutKey(event.key.keysym.sym)) {
 *     case 'i':
 *         inventory();
 *         break;
 *     case 'z':
 *         zoom();
 *         break;
 *     ...
 * }
 * \endcode
 * Of course, in a real application, you should not hardcode your key
 * assignments like this, but make them user-configurable.
 */
typedef Uint32 SDLKey;

#define SDL_KEY_CAN_BE_PHYSICAL_BIT (1<<24)     /* marks SDLKeys from the "physical" set (some of these are also in the "layout" set) */
#define SDL_KEY_KEYPAD_BIT (1<<25)      /* marks keypad keys that need [] around their name to distinguish them from the corresponding keyboard keys */
#define SDL_KEY_LAYOUT_SPECIAL_BIT (1<<26)      /* marks non-physical layout keys that cannot be described by a single character */

/** Converts an \link ::SDLPhysicalKey SDLK_* \endlink constant to an index into the array obtained from SDL_GetKeyState(). */
#define SDLK_INDEX(k) ((k) & 0x00FFFFFF)

#define SDL_PHYSICAL_KEY(n) ((n) | SDL_KEY_CAN_BE_PHYSICAL_BIT)

/**
 * \brief SDL physical key codes.
 *
 * This is the set of physical key codes, i.e. the values of SDL_keysym::sym.
 * Some of them (those for non-character keys) also appear as layout key codes.
 * The constants are typically named after how the key would be labeled on a US
 * keyboard, e.g. SDLK_A or SDLK_LEFTBRACKET refer to the keys used as A and [
 * on a US layout, but used as Q and ^ on a French layout.
 *
 * <em>enum SDLPhysicalKey</em> is not a useful type in its own right - the
 * constants defined here are intended as values of the ::SDLKey type. The only
 * reason for the enum to have a name at all is that otherwise it would be
 * impossible to refer to it in the documentation.
 *
 * \sa SDLKey
 *
 * \par Notes for driver implementors:
 * These constants and their numerical values are based on the USB HID usage
 * tables, version 1.12
 * <http://www.usb.org/developers/devclass_docs/Hut1_12.pdf>, section "10
 * Keyboard/Keypad Page (0x07)". When deciding what code to generate for what
 * key, the following rules can be used as guidelines:
 * - A given key on a given keyboard should produce the same SDLK_* code, no
 * matter what computer it is connected to, what OS runs on that computer, and
 * what the keyboard layout settings in the OS are. For USB keyboards, that
 * should be the code numerically corresponding to the key's USB usage code
 * (with exceptions, see comments for specific constants).
 * - Two keys on two different keyboards are considered "the same key" and
 * should generate the same SDLK_* code if, when connected to the same
 * computer, they are treated equally by the OS. For USB keyboards, that's
 * generally the case when they generate the same USB usage code. Non-USB
 * keyboards can probably be treated like USB keyboards of the same layout, if
 * such exist. If not, and there's no possibility to determine the equivalence
 * relation by transitivity from the above - in particular, on devices like
 * phones or game consoles that don't have PC-style alphabetic keyboards -
 * apply common sense. If none of the predefined codes fit, insert new ones at
 * the end.
 */
enum SDLPhysicalKey
{
    SDLK_FIRST_PHYSICAL = 0, /**< (not a key, just marks the lowest used value in this enum) */
    SDLK_NONE = SDL_PHYSICAL_KEY(0),
    SDLK_UNKNOWN = SDL_PHYSICAL_KEY(1), /* Not from the USB spec, but this is a convenient place for it */

    SDLK_A = SDL_PHYSICAL_KEY(4),
    SDLK_B = SDL_PHYSICAL_KEY(5),
    SDLK_C = SDL_PHYSICAL_KEY(6),
    SDLK_D = SDL_PHYSICAL_KEY(7),
    SDLK_E = SDL_PHYSICAL_KEY(8),
    SDLK_F = SDL_PHYSICAL_KEY(9),
    SDLK_G = SDL_PHYSICAL_KEY(10),
    SDLK_H = SDL_PHYSICAL_KEY(11),
    SDLK_I = SDL_PHYSICAL_KEY(12),
    SDLK_J = SDL_PHYSICAL_KEY(13),
    SDLK_K = SDL_PHYSICAL_KEY(14),
    SDLK_L = SDL_PHYSICAL_KEY(15),
    SDLK_M = SDL_PHYSICAL_KEY(16),
    SDLK_N = SDL_PHYSICAL_KEY(17),
    SDLK_O = SDL_PHYSICAL_KEY(18),
    SDLK_P = SDL_PHYSICAL_KEY(19),
    SDLK_Q = SDL_PHYSICAL_KEY(20),
    SDLK_R = SDL_PHYSICAL_KEY(21),
    SDLK_S = SDL_PHYSICAL_KEY(22),
    SDLK_T = SDL_PHYSICAL_KEY(23),
    SDLK_U = SDL_PHYSICAL_KEY(24),
    SDLK_V = SDL_PHYSICAL_KEY(25),
    SDLK_W = SDL_PHYSICAL_KEY(26),
    SDLK_X = SDL_PHYSICAL_KEY(27),
    SDLK_Y = SDL_PHYSICAL_KEY(28),
    SDLK_Z = SDL_PHYSICAL_KEY(29),

    SDLK_1 = SDL_PHYSICAL_KEY(30),
    SDLK_2 = SDL_PHYSICAL_KEY(31),
    SDLK_3 = SDL_PHYSICAL_KEY(32),
    SDLK_4 = SDL_PHYSICAL_KEY(33),
    SDLK_5 = SDL_PHYSICAL_KEY(34),
    SDLK_6 = SDL_PHYSICAL_KEY(35),
    SDLK_7 = SDL_PHYSICAL_KEY(36),
    SDLK_8 = SDL_PHYSICAL_KEY(37),
    SDLK_9 = SDL_PHYSICAL_KEY(38),
    SDLK_0 = SDL_PHYSICAL_KEY(39),

    SDLK_RETURN = SDL_PHYSICAL_KEY(40),
    SDLK_ESCAPE = SDL_PHYSICAL_KEY(41),
    SDLK_BACKSPACE = SDL_PHYSICAL_KEY(42),
    SDLK_TAB = SDL_PHYSICAL_KEY(43),
    SDLK_SPACE = SDL_PHYSICAL_KEY(44),

    SDLK_HYPHENMINUS = SDL_PHYSICAL_KEY(45),
    SDLK_EQUALS = SDL_PHYSICAL_KEY(46),
    SDLK_LEFTBRACKET = SDL_PHYSICAL_KEY(47),
    SDLK_RIGHTBRACKET = SDL_PHYSICAL_KEY(48),
    SDLK_BACKSLASH = SDL_PHYSICAL_KEY(49), /**< Located at the lower left of the return key on ISO keyboards and at the right end of the QWERTY row on ANSI keyboards. Produces REVERSE SOLIDUS (backslash) and VERTICAL LINE in a US layout, REVERSE SOLIDUS and VERTICAL LINE in a UK Mac layout, NUMBER SIGN and TILDE in a UK Windows layout, DOLLAR SIGN and POUND SIGN in a Swiss German layout, NUMBER SIGN and APOSTROPHE in a German layout, GRAVE ACCENT and POUND SIGN in a French Mac layout, and ASTERISK and MICRO SIGN in a French Windows layout. */
    SDLK_NONUSHASH = SDL_PHYSICAL_KEY(50), /**< ISO USB keyboards actually use this code instead of 49 for the same key, but all OSes I've seen treat the two codes identically. So, as an implementor, unless your keyboard generates both of those codes and your OS treats them differently, you should generate SDLK_BACKSLASH instead of this code. As a user, you should not rely on this code because SDL will never generate it with most (all?) keyboards. */
    SDLK_SEMICOLON = SDL_PHYSICAL_KEY(51),
    SDLK_APOSTROPHE = SDL_PHYSICAL_KEY(52),
    SDLK_GRAVE = SDL_PHYSICAL_KEY(53), /**< Located in the top left corner (on both ANSI and ISO keyboards). Produces GRAVE ACCENT and TILDE in a US Windows layout and in US and UK Mac layouts on ANSI keyboards, GRAVE ACCENT and NOT SIGN in a UK Windows layout, SECTION SIGN and PLUS-MINUS SIGN in US and UK Mac layouts on ISO keyboards, SECTION SIGN and DEGREE SIGN in a Swiss German layout (Mac: only on ISO keyboards), CIRCUMFLEX ACCENT and DEGREE SIGN in a German layout (Mac: only on ISO keyboards), SUPERSCRIPT TWO and TILDE in a French Windows layout, COMMERCIAL AT and NUMBER SIGN in a French Mac layout on ISO keyboards, and LESS-THAN SIGN and GREATER-THAN SIGN in a Swiss German, German, or French Mac layout on ANSI keyboards. */
    SDLK_COMMA = SDL_PHYSICAL_KEY(54),
    SDLK_PERIOD = SDL_PHYSICAL_KEY(55),
    SDLK_SLASH = SDL_PHYSICAL_KEY(56),

    SDLK_CAPSLOCK = SDL_PHYSICAL_KEY(57),

    SDLK_F1 = SDL_PHYSICAL_KEY(58),
    SDLK_F2 = SDL_PHYSICAL_KEY(59),
    SDLK_F3 = SDL_PHYSICAL_KEY(60),
    SDLK_F4 = SDL_PHYSICAL_KEY(61),
    SDLK_F5 = SDL_PHYSICAL_KEY(62),
    SDLK_F6 = SDL_PHYSICAL_KEY(63),
    SDLK_F7 = SDL_PHYSICAL_KEY(64),
    SDLK_F8 = SDL_PHYSICAL_KEY(65),
    SDLK_F9 = SDL_PHYSICAL_KEY(66),
    SDLK_F10 = SDL_PHYSICAL_KEY(67),
    SDLK_F11 = SDL_PHYSICAL_KEY(68),
    SDLK_F12 = SDL_PHYSICAL_KEY(69),

    SDLK_PRINTSCREEN = SDL_PHYSICAL_KEY(70),
    SDLK_SCROLLLOCK = SDL_PHYSICAL_KEY(71),
    SDLK_PAUSE = SDL_PHYSICAL_KEY(72),
    SDLK_INSERT = SDL_PHYSICAL_KEY(73), /**< insert on PC, help on some Mac keyboards (but does send code 73, not 117) */
    SDLK_HOME = SDL_PHYSICAL_KEY(74),
    SDLK_PAGEUP = SDL_PHYSICAL_KEY(75),
    SDLK_DELETE = SDL_PHYSICAL_KEY(76),
    SDLK_END = SDL_PHYSICAL_KEY(77),
    SDLK_PAGEDOWN = SDL_PHYSICAL_KEY(78),
    SDLK_RIGHT = SDL_PHYSICAL_KEY(79),
    SDLK_LEFT = SDL_PHYSICAL_KEY(80),
    SDLK_DOWN = SDL_PHYSICAL_KEY(81),
    SDLK_UP = SDL_PHYSICAL_KEY(82),

    SDLK_KP_NUMLOCKCLEAR = SDL_PHYSICAL_KEY(83), /**< num lock on PC, clear on Mac keyboards */
    SDLK_KP_DIVIDE = SDL_PHYSICAL_KEY(84) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_MULTIPLY = SDL_PHYSICAL_KEY(85) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_MINUS = SDL_PHYSICAL_KEY(86) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_PLUS = SDL_PHYSICAL_KEY(87) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_ENTER = SDL_PHYSICAL_KEY(88),
    SDLK_KP_1 = SDL_PHYSICAL_KEY(89) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_2 = SDL_PHYSICAL_KEY(90) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_3 = SDL_PHYSICAL_KEY(91) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_4 = SDL_PHYSICAL_KEY(92) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_5 = SDL_PHYSICAL_KEY(93) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_6 = SDL_PHYSICAL_KEY(94) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_7 = SDL_PHYSICAL_KEY(95) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_8 = SDL_PHYSICAL_KEY(96) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_9 = SDL_PHYSICAL_KEY(97) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_0 = SDL_PHYSICAL_KEY(98) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_PERIOD = SDL_PHYSICAL_KEY(99) | SDL_KEY_KEYPAD_BIT,

    SDLK_NONUSBACKSLASH = SDL_PHYSICAL_KEY(100), /**< This is the additional key that ISO keyboards have over ANSI ones, located between left shift and Y. Produces GRAVE ACCENT and TILDE in a US or UK Mac layout, REVERSE SOLIDUS (backslash) and VERTICAL LINE in a US or UK Windows layout, and LESS-THAN SIGN and GREATER-THAN SIGN in a Swiss German, German, or French layout. */
    SDLK_APPLICATION = SDL_PHYSICAL_KEY(101), /**< windows contextual menu, compose */
    SDLK_POWER = SDL_PHYSICAL_KEY(102), /**< The USB document says this is a status flag, not a physical key - but some Mac keyboards do have a power key. */
    SDLK_KP_EQUALS = SDL_PHYSICAL_KEY(103) | SDL_KEY_KEYPAD_BIT,
    SDLK_F13 = SDL_PHYSICAL_KEY(104),
    SDLK_F14 = SDL_PHYSICAL_KEY(105),
    SDLK_F15 = SDL_PHYSICAL_KEY(106),
    SDLK_F16 = SDL_PHYSICAL_KEY(107),
    SDLK_F17 = SDL_PHYSICAL_KEY(108),
    SDLK_F18 = SDL_PHYSICAL_KEY(109),
    SDLK_F19 = SDL_PHYSICAL_KEY(110),
    SDLK_F20 = SDL_PHYSICAL_KEY(111),
    SDLK_F21 = SDL_PHYSICAL_KEY(112),
    SDLK_F22 = SDL_PHYSICAL_KEY(113),
    SDLK_F23 = SDL_PHYSICAL_KEY(114),
    SDLK_F24 = SDL_PHYSICAL_KEY(115),
    SDLK_EXECUTE = SDL_PHYSICAL_KEY(116),
    SDLK_HELP = SDL_PHYSICAL_KEY(117),
    SDLK_MENU = SDL_PHYSICAL_KEY(118),
    SDLK_SELECT = SDL_PHYSICAL_KEY(119),
    SDLK_STOP = SDL_PHYSICAL_KEY(120),
    SDLK_AGAIN = SDL_PHYSICAL_KEY(121), /*!< redo */
    SDLK_UNDO = SDL_PHYSICAL_KEY(122),
    SDLK_CUT = SDL_PHYSICAL_KEY(123),
    SDLK_COPY = SDL_PHYSICAL_KEY(124),
    SDLK_PASTE = SDL_PHYSICAL_KEY(125),
    SDLK_FIND = SDL_PHYSICAL_KEY(126),
    SDLK_MUTE = SDL_PHYSICAL_KEY(127),
    SDLK_VOLUMEUP = SDL_PHYSICAL_KEY(128),
    SDLK_VOLUMEDOWN = SDL_PHYSICAL_KEY(129),
/* not sure whether there's a reason to enable these */
/*     SDLK_LOCKINGCAPSLOCK = SDL_PHYSICAL_KEY(130),  */
/*     SDLK_LOCKINGNUMLOCK = SDL_PHYSICAL_KEY(131), */
/*     SDLK_LOCKINGSCROLLLOCK = SDL_PHYSICAL_KEY(132), */
    SDLK_KP_COMMA = SDL_PHYSICAL_KEY(133) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_EQUALSAS400 = SDL_PHYSICAL_KEY(134) | SDL_KEY_KEYPAD_BIT,

    SDLK_INTERNATIONAL1 = SDL_PHYSICAL_KEY(135), /**< used on Asian keyboards, see footnotes in USB doc */
    SDLK_INTERNATIONAL2 = SDL_PHYSICAL_KEY(136),
    SDLK_INTERNATIONAL3 = SDL_PHYSICAL_KEY(137), /**< Yen */
    SDLK_INTERNATIONAL4 = SDL_PHYSICAL_KEY(138),
    SDLK_INTERNATIONAL5 = SDL_PHYSICAL_KEY(139),
    SDLK_INTERNATIONAL6 = SDL_PHYSICAL_KEY(140),
    SDLK_INTERNATIONAL7 = SDL_PHYSICAL_KEY(141),
    SDLK_INTERNATIONAL8 = SDL_PHYSICAL_KEY(142),
    SDLK_INTERNATIONAL9 = SDL_PHYSICAL_KEY(143),
    SDLK_LANG1 = SDL_PHYSICAL_KEY(144), /**< Hangul/English toggle */
    SDLK_LANG2 = SDL_PHYSICAL_KEY(145), /**< Hanja conversion */
    SDLK_LANG3 = SDL_PHYSICAL_KEY(146), /**< Katakana */
    SDLK_LANG4 = SDL_PHYSICAL_KEY(147), /**< Hiragana */
    SDLK_LANG5 = SDL_PHYSICAL_KEY(148), /**< Zenkaku/Hankaku */
    SDLK_LANG6 = SDL_PHYSICAL_KEY(149), /**< reserved */
    SDLK_LANG7 = SDL_PHYSICAL_KEY(150), /**< reserved */
    SDLK_LANG8 = SDL_PHYSICAL_KEY(151), /**< reserved */
    SDLK_LANG9 = SDL_PHYSICAL_KEY(152), /**< reserved */

    SDLK_ALTERASE = SDL_PHYSICAL_KEY(153), /**< Erase-Eaze */
    SDLK_SYSREQ = SDL_PHYSICAL_KEY(154),
    SDLK_CANCEL = SDL_PHYSICAL_KEY(155),
    SDLK_CLEAR = SDL_PHYSICAL_KEY(156),
    SDLK_PRIOR = SDL_PHYSICAL_KEY(157),
    SDLK_RETURN2 = SDL_PHYSICAL_KEY(158),
    SDLK_SEPARATOR = SDL_PHYSICAL_KEY(159),
    SDLK_OUT = SDL_PHYSICAL_KEY(160),
    SDLK_OPER = SDL_PHYSICAL_KEY(161),
    SDLK_CLEARAGAIN = SDL_PHYSICAL_KEY(162),
    SDLK_CRSELPROPS = SDL_PHYSICAL_KEY(163),
    SDLK_EXSEL = SDL_PHYSICAL_KEY(164),

    SDLK_KP_00 = SDL_PHYSICAL_KEY(176) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_000 = SDL_PHYSICAL_KEY(177) | SDL_KEY_KEYPAD_BIT,
    SDLK_THOUSANDSSEPARATOR = SDL_PHYSICAL_KEY(178),
    SDLK_DECIMALSEPARATOR = SDL_PHYSICAL_KEY(179),
    SDLK_CURRENCYUNIT = SDL_PHYSICAL_KEY(180),
    SDLK_CURRENCYSUBUNIT = SDL_PHYSICAL_KEY(181),
    SDLK_KP_LEFTPAREN = SDL_PHYSICAL_KEY(182) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_RIGHTPAREN = SDL_PHYSICAL_KEY(183) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_LEFTBRACE = SDL_PHYSICAL_KEY(184) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_RIGHTBRACE = SDL_PHYSICAL_KEY(185) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_TAB = SDL_PHYSICAL_KEY(186) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_BACKSPACE = SDL_PHYSICAL_KEY(187) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_A = SDL_PHYSICAL_KEY(188) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_B = SDL_PHYSICAL_KEY(189) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_C = SDL_PHYSICAL_KEY(190) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_D = SDL_PHYSICAL_KEY(191) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_E = SDL_PHYSICAL_KEY(192) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_F = SDL_PHYSICAL_KEY(193) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_XOR = SDL_PHYSICAL_KEY(194) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_POWER = SDL_PHYSICAL_KEY(195) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_PERCENT = SDL_PHYSICAL_KEY(196) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_LESS = SDL_PHYSICAL_KEY(197) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_GREATER = SDL_PHYSICAL_KEY(198) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_AMPERSAND = SDL_PHYSICAL_KEY(199) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_DBLAMPERSAND = SDL_PHYSICAL_KEY(200) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_VERTICALBAR = SDL_PHYSICAL_KEY(201) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_DBLVERTICALBAR = SDL_PHYSICAL_KEY(202) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_COLON = SDL_PHYSICAL_KEY(203) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_HASH = SDL_PHYSICAL_KEY(204) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_SPACE = SDL_PHYSICAL_KEY(205) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_AT = SDL_PHYSICAL_KEY(206) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_EXCLAM = SDL_PHYSICAL_KEY(207) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_MEMSTORE = SDL_PHYSICAL_KEY(208) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_MEMRECALL = SDL_PHYSICAL_KEY(209) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_MEMCLEAR = SDL_PHYSICAL_KEY(210) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_MEMADD = SDL_PHYSICAL_KEY(211) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_MEMSUBTRACT = SDL_PHYSICAL_KEY(212) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_MEMMULTIPLY = SDL_PHYSICAL_KEY(213) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_MEMDIVIDE = SDL_PHYSICAL_KEY(214) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_PLUSMINUS = SDL_PHYSICAL_KEY(215) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_CLEAR = SDL_PHYSICAL_KEY(216) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_CLEARENTRY = SDL_PHYSICAL_KEY(217) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_BINARY = SDL_PHYSICAL_KEY(218) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_OCTAL = SDL_PHYSICAL_KEY(219) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_DECIMAL = SDL_PHYSICAL_KEY(220) | SDL_KEY_KEYPAD_BIT,
    SDLK_KP_HEXADECIMAL = SDL_PHYSICAL_KEY(221) | SDL_KEY_KEYPAD_BIT,

    SDLK_LCTRL = SDL_PHYSICAL_KEY(224),
    SDLK_LSHIFT = SDL_PHYSICAL_KEY(225),
    SDLK_LALT = SDL_PHYSICAL_KEY(226), /**< alt, option */
    SDLK_LMETA = SDL_PHYSICAL_KEY(227), /**< windows, command (apple), meta */
    SDLK_RCTRL = SDL_PHYSICAL_KEY(228),
    SDLK_RSHIFT = SDL_PHYSICAL_KEY(229),
    SDLK_RALT = SDL_PHYSICAL_KEY(230), /**< alt gr, option */
    SDLK_RMETA = SDL_PHYSICAL_KEY(231), /**< windows, command (apple), meta */

    /* Everything below here is not from the USB spec */

    SDLK_MODE = SDL_PHYSICAL_KEY(232),  /* I'm not sure if this is really not covered by any of the above, but since there's a special KMOD_MODE for it I'm adding it here */

    SDLK_BRIGHTNESSDOWN = SDL_PHYSICAL_KEY(236),
    SDLK_BRIGHTNESSUP = SDL_PHYSICAL_KEY(237),
    SDLK_DISPLAYSWITCH = SDL_PHYSICAL_KEY(238), /**< display mirroring/dual display switch, video mode switch */
    SDLK_KBDILLUMTOGGLE = SDL_PHYSICAL_KEY(239),
    SDLK_KBDILLUMDOWN = SDL_PHYSICAL_KEY(240),
    SDLK_KBDILLUMUP = SDL_PHYSICAL_KEY(241),
    SDLK_EJECT = SDL_PHYSICAL_KEY(242),
    SDLK_SLEEP = SDL_PHYSICAL_KEY(243),

    /* Some of the more common and more standardized "multimedia"/"internet" keyboard keys */
    SDLK_AUDIOPLAY = SDL_PHYSICAL_KEY(244),
    SDLK_AUDIOSTOP = SDL_PHYSICAL_KEY(245),
    SDLK_AUDIOPREV = SDL_PHYSICAL_KEY(246),
    SDLK_AUDIONEXT = SDL_PHYSICAL_KEY(247),
    SDLK_CALC = SDL_PHYSICAL_KEY(248),
    SDLK_WWW = SDL_PHYSICAL_KEY(249),
    SDLK_EMAIL = SDL_PHYSICAL_KEY(250),
    SDLK_MEDIA = SDL_PHYSICAL_KEY(251),
    SDLK_COMPUTER = SDL_PHYSICAL_KEY(252),
    SDLK_SEARCH = SDL_PHYSICAL_KEY(253),
    SDLK_BOOKMARKS = SDL_PHYSICAL_KEY(254),
    SDLK_BROWSERBACK = SDL_PHYSICAL_KEY(255),
    SDLK_BROWSERFORWARD = SDL_PHYSICAL_KEY(256),
    SDLK_BROWSERRELOAD = SDL_PHYSICAL_KEY(257),
    SDLK_BROWSERSTOP = SDL_PHYSICAL_KEY(258),

    /* Add any other keys here */

    SDLK_LAST_PHYSICAL /**< (not a key, just marks the highest used value in this enum) */
};

#define SDLK_FIRST SDLK_INDEX(SDLK_FIRST_PHYSICAL)
#define SDLK_LAST SDLK_INDEX(SDLK_LAST_PHYSICAL)



/**
 * \enum SDLMod
 *
 * \brief Enumeration of valid key mods (possibly OR'd together)
 */
typedef enum
{
    KMOD_NONE = 0x0000,
    KMOD_LSHIFT = 0x0001,
    KMOD_RSHIFT = 0x0002,
    KMOD_LCTRL = 0x0040,
    KMOD_RCTRL = 0x0080,
    KMOD_LALT = 0x0100,
    KMOD_RALT = 0x0200,
    KMOD_LMETA = 0x0400,
    KMOD_RMETA = 0x0800,
    KMOD_NUM = 0x1000,
    KMOD_CAPS = 0x2000,
    KMOD_MODE = 0x4000,
    KMOD_RESERVED = 0x8000
} SDLMod;

#define KMOD_CTRL	(KMOD_LCTRL|KMOD_RCTRL)
#define KMOD_SHIFT	(KMOD_LSHIFT|KMOD_RSHIFT)
#define KMOD_ALT	(KMOD_LALT|KMOD_RALT)
#define KMOD_META	(KMOD_LMETA|KMOD_RMETA)

#endif /* _SDL_keysym_h */
