lib_LTLIBRARIES = libtestplatform.la
libtestplatform_la_SOURCES = testplatform.c ../SDL_test.c
libtestplatform_la_CLAGS = -fPIC -g
libtestplatform_la_LDFLAGS = `sdl-config --libs`

distclean-local:
	-rm *.dylib
	-rm *.so

