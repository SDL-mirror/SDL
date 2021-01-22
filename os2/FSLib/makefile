# This is a Watcom Makefile to create the FSLib.dll

# Settings
#
dllname=FSLib
object_files=FSLib.obj

# Create debug build?
#debug_build=defined

# Common stuff to build a DLL
DLLFLAGS = -bd
!ifdef debug_build
DEBUGFLAGS = -d2 -DDEBUG_BUILD
!else
DEBUGFLAGS =
!endif
CFLAGS = $(DEBUGFLAGS) -zq -bm -bt=OS2 -5 -fpi -sg -otexan -wx -ei

.before
    @SET INCLUDE=$(%OS2TK)\h;$(%INCLUDE)

.extensions:
.extensions: .dll .obj .c

all : $(dllname).dll

.c.obj: .AUTODEPEND
  wcc386 $(DLLFLAGS) $(CFLAGS) -fo=$^@ $<

$(dllname).dll : $(object_files)
    wlink @$(dllname).lnk
    wlib -q -b -n -c -pa -s -t -zld -ii -io $(dllname).lib $(dllname).dll

clean : .SYMBOLIC
        @rm -f *.dll
        @rm -f $(dllname).lib
        @rm -f *.obj
        @rm -f *.map
        @rm -f *.res
        @rm -f *.lst

