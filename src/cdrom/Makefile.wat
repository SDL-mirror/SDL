#=============================================================================
#          This is a Watcom makefile to build SDL.DLL for OS/2
#
# Makefile for OS/2 System CDROM support
#=============================================================================

object_files=SDL_syscdrom.obj SDL_cdrom.obj
ExtraCFlags=

#
#==============================================================================
#
!include ..\..\Watcom.mif

.before
    set include=$(%os2tk)\h;$(%include);../../include;./os2;../;

all : $(object_files)

SDL_syscdrom.obj : .AUTODEPEND
    wcc386 os2\SDL_syscdrom.c $(cflags)

SDL_cdrom.obj : .AUTODEPEND
    wcc386 SDL_cdrom.c $(cflags)

clean : .SYMBOLIC
        @if exist *.obj del *.obj
        @if exist *.map del *.map
        @if exist *.res del *.res
        @if exist *.lst del *.lst
