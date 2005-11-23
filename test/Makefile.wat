#=============================================================================
#          This is a Watcom makefile to build SDL.DLL for OS/2
#
# Makefile for test applications
#=============================================================================

# Create debug build or not?
debug_build=defined

#-----------------------------------------------------------------------------
# The next part is somewhat general, for creation of EXE files.
#-----------------------------------------------------------------------------

cflags = $(debugflags) -bm -bt=OS2 -5 -fpi -sg -otexan -wx -ei

.before
    set include=$(%os2tk)\h;$(%include);../include

.extensions:
.extensions: .exe .obj .c

all :   testalpha.exe     &
        testbitmap.exe    &
        testcdrom.exe     &
        testcpuinfo.exe   &
        testjoystick.exe  &
        testkeys.exe      &
        testlock.exe      &
        testsem.exe       &
        testsprite.exe    &
        testtimer.exe     &
        testtypes.exe     &
        testver.exe       &
        testvidinfo.exe   &
        testwin.exe       &
        testwm.exe        &
        threadwin.exe     &
        torturethread.exe &
        checkkeys.exe

.c.obj : .AUTODEPEND
    wcc386 -zq -bm -5s -ei -oteaxan -wx $[* $(cflags)

.obj.exe : .AUTODEPEND
    wlink system os2v2 F $* L ..\src\sdl.lib name $@ op quiet

clean : .SYMBOLIC
        @if exist *.exe del *.exe
        @if exist *.obj del *.obj
        @if exist *.map del *.map
        @if exist *.res del *.res
        @if exist *.lst del *.lst
