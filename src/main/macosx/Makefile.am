# On OS X, install the SDLMain.nib.
# We use an ugly hack to force the creation of the
# destination dir, maybe somebody with more automake
# experience knows how to do this properly?

if TARGET_MACOSX
hackhacknibdatadir = $(nibdatadir)/SDLMain.nib
hackhacknibdata_DATA = info.nib

nibdatadir = $(datadir)/sdl
nibdata_DATA =			\
	SDLMain.nib/classes.nib	\
	SDLMain.nib/info.nib	\
	SDLMain.nib/objects.nib	\
	Info.plist
endif

EXTRA_DIST = SDLMain.h SDLMain.m info.nib

# The nib and exports directories need to be copied into place
# when building a distribution.
dist-hook:
	cp -rp SDLMain.nib $(distdir)
	cp -rp exports $(distdir)
	(cd $(distdir) && rm -rf `find . -name CVS`)
