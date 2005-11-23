#=============================================================================
#          This is a Watcom makefile to build SDL.DLL for OS/2
#
# Makefile for endianness
#=============================================================================

object_files=SDL_endian.obj
ExtraCFlags=

#
#==============================================================================
#
!include ..\..\Watcom.mif

.before
    set include=$(%os2tk)\h;$(%include);../../include

all : $(object_files)

clean : .SYMBOLIC
        @if exist *.obj del *.obj
        @if exist *.map del *.map
        @if exist *.res del *.res
        @if exist *.lst del *.lst
