lib_LTLIBRARIES = libtest.la
libtest_la_SOURCES = test.c ../SDL_test.c
libtest_la_CLAGS = -fPIC -g
libtest_la_LDFLAGS = `sdl-config --libs`

distclean-local:
	-rm *.dylib
	-rm *.so

