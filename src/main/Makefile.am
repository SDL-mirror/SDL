
## Makefile.am for the main() function in the SDL library
#
# This is necessary because some platforms have special program
# entry points, which require special application initialization.

ARCH_SUBDIRS = $(srcdir)/beos \
               $(srcdir)/epoc \
               $(srcdir)/linux \
               $(srcdir)/macos \
               $(srcdir)/macosx \
               $(srcdir)/win32

# Build a separate library containing the main() entry point.
lib_LIBRARIES = libSDLmain.a

if TARGET_MACOSX
MAINLIB_ARCH_SRCS = SDLMain.m SDLMain.h
else
if TARGET_QTOPIA
MAINLIB_ARCH_SRCS = SDL_Qtopia_main.cc
else
MAINLIB_ARCH_SRCS = SDL_main.c
endif
endif
libSDLmain_a_SOURCES = $(MAINLIB_ARCH_SRCS)

# Build an internal library of any special app setup functions
noinst_LTLIBRARIES = libarch.la

if TARGET_BEOS
ARCH_SRCS = SDL_BeApp.cc SDL_BeApp.h
else
ARCH_SRCS = 
endif

# Include the architecture-independent sources
COMMON_SRCS = dummy.c

libarch_la_SOURCES = $(COMMON_SRCS) $(ARCH_SRCS)

## Let automake know that it shouldn't distribute linked sources
BUILT_SOURCES = $(MAINLIB_ARCH_SRCS) $(ARCH_SRCS)

## Let automake know that it should remove these for distribution
DISTCLEANFILES = $(MAINLIB_ARCH_SRCS) $(ARCH_SRCS)

# The architecture specific directories need to be copied into place
# when building a distribution.
dist-hook:
	(cd $(distdir) && rm -f $(BUILT_SOURCES))
	cp -rp $(ARCH_SUBDIRS) $(distdir)
	(cd $(distdir) && rm -rf `find . -name CVS`)
