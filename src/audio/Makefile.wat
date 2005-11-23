#=============================================================================
#          This is a Watcom makefile to build SDL.DLL for OS/2
#
# Makefile for DART (audio support)
#=============================================================================

object_files= SDL_audio.obj SDL_audiocvt.obj SDL_audiomem.obj SDL_mixer.obj SDL_mixer_MMX_VC.obj SDL_wave.obj SDL_dart.obj
ExtraCFlags=-dUSE_ASM_MIXER_VC -dUSE_DOSSETPRIORITY

#
#==============================================================================
#
!include ..\..\Watcom.mif

.before
    set include=$(%os2tk)\h;$(%include);../../include;./dart

all : $(object_files)

SDL_dart.obj: .AUTODEPEND
    wcc386 dart\SDL_dart.c $(cflags)

clean : .SYMBOLIC
        @if exist *.obj del *.obj
        @if exist *.map del *.map
        @if exist *.res del *.res
        @if exist *.lst del *.lst
