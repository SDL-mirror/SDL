
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

Keyboard (GEMDOS, BIOS, Ikbd)
Mouse (XBIOS, GEM, Ikbd)
Video (XBIOS (Fullscreen), GEM (Windowed and Fullscreen))
Timer (VBL vector)

- What is missing:
Audio support (TOS)
CDROM support (Metados, /dev/cdrom)
Joystick and joypad support (Hardware)
Threads support (TOS)

- Driver combinations:
Video	Kbd	Mouse	Timer
xbios	ikbd	ikbd	vbl
xbios	gemdos	xbios	vbl
xbios	bios	xbios	vbl
gem	gem	gem	vbl

==============================================================================
V.  Environment variables:

SDL_ATARI_EVENTSDRIVER
	Set to 'ikbd' to force IKBD 6301 keyboard driver
	Set to 'gemdos' to force gemdos keyboard driver
	Set to 'bios' to force bios keyboard driver

SDL_VIDEODRIVER:
	Set to 'xbios' to force xbios video driver
	Set to 'gem' to force gem video driver

-- 
Patrice Mandin <pmandin@caramail.com>
http://www.multimania.com/pmandin
