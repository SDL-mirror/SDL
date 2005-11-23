#=============================================================================
#          This is a Watcom makefile to build SDL.DLL for OS/2
#
#
#=============================================================================

dllname=SDL

audioobjs = audio\SDL_audio.obj audio\SDL_audiocvt.obj audio\SDL_audiomem.obj &
            audio\SDL_mixer.obj audio\SDL_mixer_MMX_VC.obj audio\SDL_wave.obj &
            audio\SDL_dart.obj
cdromobjs = cdrom\SDL_cdrom.obj cdrom\SDL_syscdrom.obj
cpuinfoobjs = cpuinfo\SDL_cpuinfo.obj
endianobjs = endian\SDL_endian.obj
eventsobjs = events\SDL_active.obj events\SDL_events.obj events\SDL_expose.obj &
             events\SDL_keyboard.obj events\SDL_mouse.obj events\SDL_quit.obj &
             events\SDL_resize.obj
fileobjs = file\SDL_rwops.obj
hermesobjs = hermes\mmx_main.obj hermes\mmxp2_32.obj hermes\x86_main.obj &
             hermes\x86p_16.obj hermes\x86p_32.obj
joystickobjs = joystick\SDL_joystick.obj joystick\SDL_sysjoystick.obj
threadobjs = thread\SDL_thread.obj thread\SDL_sysmutex.obj &
             thread\SDL_syssem.obj thread\SDL_systhread.obj &
             thread\SDL_syscond.obj
timerobjs = timer\SDL_timer.obj timer\SDL_systimer.obj
videoobjs = video\SDL_blit.obj video\SDL_blit_0.obj video\SDL_blit_1.obj &
            video\SDL_blit_A.obj video\SDL_blit_N.obj video\SDL_bmp.obj &
            video\SDL_cursor.obj video\SDL_gamma.obj video\SDL_pixels.obj &
            video\SDL_RLEaccel.obj video\SDL_stretch.obj video\SDL_surface.obj &
            video\SDL_video.obj video\SDL_yuv.obj video\SDL_yuv_mmx.obj &
            video\SDL_yuv_sw.obj video\SDL_os2fslib.obj

object_files= SDL.obj SDL_error.obj SDL_fatal.obj SDL_getenv.obj &
              SDL_loadso.obj $(audioobjs) $(cpuinfoobjs) $(endianobjs) &
              $(eventsobjs) $(fileobjs) $(joystickobjs) &
              $(threadobjs) $(timerobjs) $(videoobjs) $(cdromobjs)
	      

# Extra stuffs to pass to C compiler:
ExtraCFlags=

#
#==============================================================================
#
!include ..\Watcom.mif

.before
    @set include=$(%os2tk)\h;$(%include);../include;./thread;./thread/os2;./video;./cdrom;./cdrom/os2;./joystick;./joystick/os2;

all : check_subdir_objects $(dllname).dll $(dllname).lib

$(dllname).dll : $(dllname).lnk $(object_files)
    wlink @$(dllname)

check_subdir_objects: .always .symbolic
    @cd audio
    @wmake -h -f Makefile.wat
    @cd ..\cdrom
    @wmake -h -f Makefile.wat
    @cd ..\cpuinfo
    @wmake -h -f Makefile.wat
    @cd ..\endian
    @wmake -h -f Makefile.wat
    @cd ..\events
    @wmake -h -f Makefile.wat
    @cd ..\file
    @wmake -h -f Makefile.wat
    @cd ..\joystick
    @wmake -h -f Makefile.wat
    @cd ..\thread
    @wmake -h -f Makefile.wat
    @cd ..\timer
    @wmake -h -f Makefile.wat
    @cd ..\video
    @wmake -h -f Makefile.wat
    @cd ..

$(dllname).lnk :
    @echo Creating linker file ($(dllname).lnk)...
    @echo $#============================================================================= >$^@
    @echo $#              This is a linker file to build SDL.DLL for OS/2 >>$^@
    @echo $# >>$^@
    @echo $# Generated automatically by Makefile.wat >>$^@
    @echo $#============================================================================= >>$^@
    @echo SYSTEM 386 LX DLL INITINSTANCE TERMINSTANCE >>$^@
    @echo NAME $^& >>$^@
    @for %i in ($(object_files)) do @echo FILE %i >>$^@
    @echo LIBPATH %os2tk%\lib >>$^@
    @echo LIBPATH %fslib% >>$^@
    @echo LIB mmpm2.lib >>$^@
    @echo LIB fslib.lib >>$^@
    @echo OPTION QUIET >>$^@
    @echo OPTION MAP=$^&.map >>$^@
    @echo OPTION DESCRIPTION 'Simple DirectMedia Layer v1.2.7' >>$^@
    @echo OPTION ELIMINATE >>$^@
    @echo OPTION MANYAUTODATA >>$^@
    @echo OPTION OSNAME='OS/2 and eComStation' >>$^@
    @echo OPTION SHOWDEAD >>$^@
    @echo Linker file created!

$(dllname).lib : $(dllname).dll
    implib $(dllname).lib $(dllname).dll

clean : .SYMBOLIC
    @if exist *.dll del *.dll
    @if exist *.lib del *.lib
    @if exist *.obj del *.obj
    @if exist *.lnk del *.lnk
    @if exist *.map del *.map
    @if exist *.res del *.res
    @if exist *.lst del *.lst
    @cd audio
    @wmake -h -f Makefile.wat clean
    @cd ..\cdrom
    @wmake -h -f Makefile.wat clean
    @cd ..\cpuinfo
    @wmake -h -f Makefile.wat clean
    @cd ..\endian
    @wmake -h -f Makefile.wat clean
    @cd ..\events
    @wmake -h -f Makefile.wat clean
    @cd ..\file
    @wmake -h -f Makefile.wat clean
    @cd ..\joystick
    @wmake -h -f Makefile.wat clean
    @cd ..\thread
    @wmake -h -f Makefile.wat clean
    @cd ..\timer
    @wmake -h -f Makefile.wat clean
    @cd ..\video
    @wmake -h -f Makefile.wat clean
    @cd ..
