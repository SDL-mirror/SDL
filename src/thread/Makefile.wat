#=============================================================================
#          This is a Watcom makefile to build SDL.DLL for OS/2
#
# Makefile for threading
#=============================================================================

object_files=SDL_thread.obj SDL_sysmutex.obj SDL_syssem.obj SDL_systhread.obj SDL_syscond.obj
ExtraCFlags=

#
#==============================================================================
#
!include ..\..\Watcom.mif

.before
    set include=$(%os2tk)\h;$(%include);../../include;./os2;../;

all : $(object_files)

SDL_sysmutex.obj: .AUTODEPEND
    wcc386 os2\SDL_sysmutex.c $(cflags)

SDL_syssem.obj: .AUTODEPEND
    wcc386 os2\SDL_syssem.c $(cflags)

SDL_systhread.obj: .AUTODEPEND
    wcc386 os2\SDL_systhread.c $(cflags)

SDL_syscond.obj: .AUTODEPEND
    wcc386 os2\SDL_syscond.c $(cflags)

clean : .SYMBOLIC
        @if exist *.obj del *.obj
        @if exist *.map del *.map
        @if exist *.res del *.res
        @if exist *.lst del *.lst
