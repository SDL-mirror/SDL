ACLOCAL_AMFLAGS = -I acinclude -I build-scripts

SUBDIRS = testdummy testrect testplatform

bin_PROGRAMS = runner
runner_SOURCES = runner.c SDL_test.c
runner_CLAGS = -W -Wall -Wextra -g `sdl-config --cflags` -DSDL_NO_COMPAT
runner_LDFLAGS = `sdl-config --libs` 

install: install-tests
install-tests:
	-mkdir tests
	-cp -f testdummy/.libs/*.dylib tests/ 2> /dev/null
	-cp -f testdummy/.libs/*.so tests/ 2> /dev/null
	-cp -f testrect/.libs/*.dylib tests/ 2> /dev/null
	-cp -f testrect/.libs/*.so tests/ 2> /dev/null
	-cp -f testplatform/.libs/*.dylib tests/ 2> /dev/null
	-cp -f testplatform/.libs/*.so tests/ 2> /dev/null

distclean-local:
	-rm -Rf tests/ docs/

