# The top-level input Makefile for SDL

# require automake 1.4
AUTOMAKE_OPTIONS = 1.4

## Any directories that you want built and installed should go here.
SUBDIRS = src include docs

## Any directories you want a part of the distribution should be listed
## here, as well as have a Makefile generated at the end of configure.in
##
## This only works for subdirectories one level deep.
DIST_SUBDIRS = $(SUBDIRS) docs

# SDL runtime configuration script
bin_SCRIPTS = sdl-config

# All the rest of the distributed files
EXTRA_DIST =		\
	BUGS		\
	TODO		\
        COPYING		\
        CREDITS		\
        INSTALL		\
        README		\
        README-SDL.txt	\
        README.Win32	\
        README.WinCE	\
        README.MacOS	\
        README.MacOSX	\
        VisualC.html	\
        VisualC.zip	\
        MPWmake.sea.bin	\
	CWprojects.sea.bin \
        WhatsNew	\
        docs.html	\
        sdl.m4		\
        autogen.sh	\
        strip_fPIC.sh

# M4 macro file for inclusion with autoconf
m4datadir = $(datadir)/aclocal
m4data_DATA = sdl.m4

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba SDL.spec

# Rule to rebuild the export lists for BeOS, MacOS and Win32.
exports:
	(cd src/main/beos/exports; $(MAKE))
	(cd src/main/macos/exports; $(MAKE))
	(cd src/main/win32/exports; $(MAKE))

# Rule to force automake to rebuild the library
changed:
	@echo "This build target is no longer necessary"

# Rule to install the libraries only - prevent rebuilding apps
install-lib:
	cd src && $(MAKE) install-libLTLIBRARIES

# Run ldconfig after installing the library:
install-hook:
	-ldconfig

# Grab the test programs for the distribution:
dist-hook:
	if test -f test/Makefile; then (cd test; make distclean); fi
	cp -rp test $(distdir)
	rm -rf `find $(distdir) -type d -name CVS -print`

# Create a CVS snapshot that people can run update -d on
CVSROOT = :pserver:guest@cvs.lokigames.com:/cvs
snapshot:
	cvs -d $(CVSROOT) login
	cvs -d $(CVSROOT) checkout -r devel_1_1_0 SDL
	(cd SDL && ./autogen.sh)
	mv SDL SDL-1.2
	tar zcvf $(HOME)/SDL-1.2.tar.gz SDL-1.2
	rm -rf SDL-1.2
