
==============================================================================
Using the Simple DirectMedia Layer on Atari
==============================================================================

==============================================================================
I.  Building the Simple DirectMedia Layer libraries:
    (This step isn't necessary if you have the SDL binary distribution)

  Do the classic configure, with --disable-shared --enable-static and:

    Tos version (should run everywhere):
      --disable-audio --disable-threads
    Tos does not support threads, so can not support audio, maybe in a future
    version audio support will be added via interrupts.

    MiNT version (maybe Magic, only for multitasking OS):
      --disable-pthreads --enable-pth
    Mint and Magic may supports threads, so audio can be used with current
    devices, like Sun audio, or disk-writing support. Like Tos, interrupt
    audio without threads is more suited for Atari machines.

  Then you can make ; make install it.

==============================================================================
II. Building the Simple DirectMedia Layer test programs:

  Do the classic configure, then make.

  Run them !

==============================================================================
III.  Enjoy! :)

  If you have a project you'd like me to know about, or want to ask questions,
  go ahead and join the SDL developer's mailing list by sending e-mail to:

	sdl-request@libsdl.org

  and put "subscribe" into the subject of the message. Or alternatively you
  can use the web interface:

	http://www.libsdl.org/mailman/listinfo/sdl
  
==============================================================================
IV.  What is supported:

Keyboard (GEMDOS, BIOS, GEM, Ikbd)
Mouse (XBIOS, GEM, Ikbd)
Video (XBIOS (Fullscreen), GEM (Windowed and Fullscreen))
Timer (VBL vector)
Joystick and joypad support (Ikbd, Hardware)

- What is missing:
Audio support (TOS)
CDROM support (Metados, /dev/cdrom)
Threads support (TOS)

- Driver combinations:
Video	Kbd	Mouse	Timer	Jstick	Joypads
xbios	ikbd	ikbd	vbl	ikbd	hardware
xbios	gemdos	xbios	vbl	xbios	hardware
xbios	bios	xbios	vbl	xbios	hardware
gem	gem	gem(*)	vbl	xbios	hardware

(*) GEM does not report relative mouse motion, so xbios mouse driver is used
to report this type event.

==============================================================================
V.  Environment variables:

SDL_ATARI_EVENTSDRIVER
	Set to 'ikbd' to force IKBD 6301 keyboard driver
	Set to 'gemdos' to force gemdos keyboard driver
	Set to 'bios' to force bios keyboard driver

SDL_VIDEODRIVER:
	Set to 'xbios' to force xbios video driver
	Set to 'gem' to force gem video driver

SDL_JOYSTICK_ATARI:
	Use any of these strings in the environment variable to enable or
	disable a joystick:

	'ikbd-joy1-[on|off]' for IKBD joystick on port 1 (hardware access)
	'xbios-joy1-[on|off]' for IKBD joystick on port 1 (xbios access)
	'porta-pad-[on|off]' for joypad on port A
	'porta-joy0-[on|off]' for joystick 0 on port A
	'porta-joy1-[on|off]' for joystick 1 on port A
	'porta-lp-[on|off]' for lightpen on port A
	'porta-anpad-[on|off]' for analog paddle on port A
	'portb-pad-[on|off]' for joypad on port B
	'portb-joy0-[on|off]' for joystick 0 on port B
	'portb-joy1-[on|off]' for joystick 1 on port B
	'portb-anpad-[on|off]' for analog paddle on port B

	Default configuration is:
		'ikbd-joy1-on' (if IKBD events driver enabled)
		'xbios-joy1-on' (if gemdos/bios/gem events driver enabled)
		'porta-pad-on portb-pad-on' (if available on the machine)

	port[a|b]-[pad|joy?|lp|anpad]-* strings are mutually exclusives.
	On such a port, you can only use a joypad OR 1 or 2 joysticks OR
	a lightpen OR an analog paddle. You must disable joypad before
	setting another controller.

	The second joystick port on IKBD is used by the mouse, so not usable.

	Joypads are multibuttons controller (Atari Jaguar console-like).
	Joysticks are 1 button, 2 axis controllers.
	Lightpen and analog paddle are 2 buttons, 2 axis controllers. The 2
	buttons are those affected to 1 button joysticks on the same port.

==============================================================================
VI.  More informations about drivers:

Xbios video:
	Video chip is detected using the _VDO cookie.
	Screen enhancers are not supported, but could be if you know how to
	use them.

	ST, STE, Mega ST, Mega STE:
		320x200x4 bits, shades of grey, available only for the purpose
		of testing SDL.
	TT:
		320x480x8 and 320x240x8 (software double-lined mode).
	Falcon:
		All modes supported by the current monitor (RVB or VGA).
	Clones and any machine with monochrome monitor:
		Not supported.

Gem video:
	Automatically used if xbios not available.

	All machines:
		Only the current resolution, if 8 bits or higher depth.

IKBD keyboard, mouse and joystick driver:
	Available if _MCH cookie is ST, Mega ST, STE, Mega STE, TT or Falcon.

	Hades has an IKBD, but xbios is not available for video, so IKBD
	driver is disabled.

Gemdos and bios keyboard driver:
	Available on all machines.

Mouse and joystick xbios driver:
	Available on all machines (I think).

Joypad driver:
	Available if _MCH cookie is STE or Falcon.

VBL timer driver:
	Available all machines (I think).

-- 
Patrice Mandin <pmandin@caramail.com>
http://membres.lycos.fr/pmandin/
