# On OS X, install the SDL.rsrc.

EXTRA_DIST = SDL_main.c SDL.rsrc

rsrcdatadir = $(datadir)/sdl
rsrcdata_DATA =			\
	SDL.rsrc

SDL.rsrc: SDL.r
	/Developer/Tools/Rez -o $@ -useDF $<

# The rsrc and export directory need to be copied into place
# when building a distribution.
dist-hook:
	cp -rp SDL.rsrc $(distdir)
	cp -rp exports $(distdir)
	(cd $(distdir)/exports && rm -rf `find . -name CVS`)
