
## Makefile.am for the SDL cdrom library

noinst_LTLIBRARIES = libcdrom.la

ARCH_SUBDIRS = dummy beos freebsd linux macos openbsd qnx win32

# Include the architecture-independent sources
COMMON_SRCS = SDL_cdrom.c SDL_syscdrom.h

# Include the architecture-specific sources
ARCH_SRCS = SDL_syscdrom.c

libcdrom_la_SOURCES = $(COMMON_SRCS) $(ARCH_SRCS)

## Let automake know that it shouldn't distribute linked sources
BUILT_SOURCES = $(ARCH_SRCS)

## Let automake know that it should remove these for distribution
DISTCLEANFILES = $(ARCH_SRCS)

# The architecture specific directories need to be copied into place
# when building a distribution.
dist-hook:
	(cd $(distdir) && rm -f $(BUILT_SOURCES))
	cp -rp $(ARCH_SUBDIRS) $(distdir)
	(cd $(distdir) && rm -rf `find . -name CVS`)
