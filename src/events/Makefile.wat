#=============================================================================
#          This is a Watcom makefile to build SDL.DLL for OS/2
#
# Makefile for events
#=============================================================================

object_files=SDL_active.obj SDL_events.obj SDL_expose.obj SDL_keyboard.obj SDL_mouse.obj SDL_quit.obj SDL_resize.obj
ExtraCFlags=-dUSE_DOSSETPRIORITY

#
#==============================================================================
#
!include ..\..\Watcom.mif

.before
    set include=$(%os2tk)\h;$(%include);../../include;../timer;../joystick;../video;

all : $(object_files)

clean : .SYMBOLIC
        @if exist *.obj del *.obj
        @if exist *.map del *.map
        @if exist *.res del *.res
        @if exist *.lst del *.lst
