ACLOCAL_AMFLAGS = -I acinclude -I build-scripts

SUBDIRS = tests

bin_PROGRAMS = runner
runner_SOURCES = runner.c
runner_CLAGS = -W -Wall -Wextra -g `sdl-config --cflags` -DSDL_NO_COMPAT
runner_LDFLAGS = `sdl-config --libs`

install: install-tests
	;

install-tests:
	cp -f tests/.libs/*.dylib tests/


