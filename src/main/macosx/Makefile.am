# On OS X, install the SDL_main.nib.
# We use an ugly hack to force the creation of the
# destination dir, maybe somebody with more automake
# experience knows how to do this properly?

if TARGET_MACOSX
masternibdatadir = $(datadir)/sdl/SDL_main.nib
masternibdata_DATA =		\
	SDL_main.nib

nibdatadir = $(datadir)/sdl
nibdata_DATA =			\
	SDL_main.nib/classes.nib	\
	SDL_main.nib/info.nib	\
	SDL_main.nib/objects.nib	\
	Info.plist
endif

EXTRA_DIST = SDL_main.h SDL_main.m

# The nib and exports directories need to be copied into place
# when building a distribution.
dist-hook:
	(cd $(distdir) && rm -f $(BUILT_SOURCES))
	cp -rp SDL_main.nib $(distdir)
	cp -rp exports $(distdir)
	(cd $(distdir) && rm -rf `find . -name CVS`)
