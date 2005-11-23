#=============================================================================
#          This is a Watcom makefile to build SDL.DLL for OS/2
#
# Makefile for timers
#=============================================================================

object_files=SDL_timer.obj SDL_systimer.obj
ExtraCFlags=

#
#==============================================================================
#
!include ..\..\Watcom.mif

.before
    set include=$(%os2tk)\h;$(%include);../../include;./os2;../;

all : $(object_files)

SDL_systimer.obj: .AUTODEPEND
    wcc386 os2\SDL_systimer.c $(cflags)

clean : .SYMBOLIC
        @if exist *.obj del *.obj
        @if exist *.map del *.map
        @if exist *.res del *.res
        @if exist *.lst del *.lst
