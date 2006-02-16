
## Makefile.am for the main() function in the SDL library
#
# This is necessary because some platforms have special program
# entry points, which require special application initialization.

# Build a separate library containing the main() entry point.
lib_LIBRARIES = libSDLmain.a

if TARGET_WIN32
MAINLIB_ARCH_SRCS = win32/SDL_win32_main.c
else
if TARGET_MACOSX
if USE_COCOA
MAINLIB_ARCH_SRCS = macosx/SDLMain.m macosx/SDLMain.h
else
if USE_CARBON
MAINLIB_ARCH_SRCS = macos/SDL_main.c
endif # USE_CARBON
endif # USE_COCOA
else
if TARGET_QTOPIA
MAINLIB_ARCH_SRCS = qtopia/SDL_qtopia_main.cc
else
MAINLIB_ARCH_SRCS = dummy/SDL_dummy_main.c
endif # !TARGET_QTOPIA
endif # !TARGET_MACOSX
endif # !TARGET_WIN32
libSDLmain_a_SOURCES = $(MAINLIB_ARCH_SRCS)

# Build an internal library of any special app setup functions
noinst_LTLIBRARIES = libarch.la

if TARGET_BEOS
ARCH_SRCS = beos/SDL_BeApp.cc beos/SDL_BeApp.h
else
ARCH_SRCS = arch.c
endif

libarch_la_SOURCES = $(ARCH_SRCS)

exports:
	for i in $(srcdir)/*; do if [ -d $$i/exports ]; then (cd $$i/exports; make); fi; done

EXTRA_DIST = beos dummy epoc linux macos macosx qtopia win32
