#=============================================================================
#          This is a Watcom makefile to build SDL.DLL for OS/2
#
# Makefile for threading
#=============================================================================

object_files=SDL_blit.obj SDL_blit_0.obj SDL_blit_1.obj SDL_blit_A.obj SDL_blit_N.obj SDL_bmp.obj SDL_cursor.obj SDL_gamma.obj SDL_pixels.obj SDL_RLEaccel.obj SDL_stretch.obj SDL_surface.obj SDL_video.obj SDL_yuv.obj SDL_yuv_mmx.obj SDL_yuv_sw.obj SDL_os2fslib.obj
ExtraCFlags=-dUSE_DOSSETPRIORITY

#
#==============================================================================
#
!include ..\..\Watcom.mif

.before
    set include=$(%os2tk)\h;$(%include);../../include;../;./os2;../events;../hermes;$(%FSLIB);

all : $(object_files)

SDL_os2fslib.obj : .AUTODEPEND
    wcc386 os2fslib\SDL_os2fslib.c $(cflags)

clean : .SYMBOLIC
        @if exist *.obj del *.obj
        @if exist *.map del *.map
        @if exist *.res del *.res
        @if exist *.lst del *.lst
