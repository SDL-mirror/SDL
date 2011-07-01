ACLOCAL_AMFLAGS = -I acinclude -I build-scripts

SUBDIRS = testdummy testrect testplatform

bin_PROGRAMS = runner
runner_SOURCES = runner.c SDL_test.c logger.c xml_logger.c plain_logger.c xml.c logger_helpers.c
runner_CLAGS = -W -Wall -Wextra -g `sdl-config --cflags` -DSDL_NO_COMPAT
runner_LDFLAGS = `sdl-config --libs` 

install: install-tests
install-tests:
	$(SHELL) install-tests.sh

distclean-local:
	-rm -Rf tests/ docs/

