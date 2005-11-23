#=============================================================================
#          This is a Watcom makefile to build SDL.DLL for OS/2
#
# Makefile for joystick (using the dummy joystick driver)
#=============================================================================

object_files=SDL_joystick.obj SDL_sysjoystick.obj
ExtraCFlags=

#
#==============================================================================
#
!include ..\..\Watcom.mif

.before
    set include=$(%os2tk)\h;$(%include);../../include;./os2;../;../events;

all : $(object_files)

SDL_sysjoystick.obj: .AUTODEPEND
    wcc386 os2\SDL_sysjoystick.c $(cflags)

clean : .SYMBOLIC
        @if exist *.obj del *.obj
        @if exist *.map del *.map
        @if exist *.res del *.res
        @if exist *.lst del *.lst
