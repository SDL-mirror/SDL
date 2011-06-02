lib_LTLIBRARIES = libtestrect.la
libtestrect_la_SOURCES = testrect.c ../SDL_test.c
libtestrect_la_CLAGS = -fPIC -g
libtestrect_la_LDFLAGS = `sdl-config --libs`

distclean-local:
	-rm *.dylib
	-rm *.so

