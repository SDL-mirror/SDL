lib_LTLIBRARIES = libtestdummy.la
libtestdummy_la_SOURCES = testdummy.c ../SDL_test.c
libtestdummy_la_CLAGS = -fPIC -g
libtestdummy_la_LDFLAGS = `sdl-config --libs`

distclean-local:
	-rm *.dylib
	-rm *.so

