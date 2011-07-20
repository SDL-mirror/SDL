ACLOCAL_AMFLAGS = -I acinclude -I build-scripts

SUBDIRS = testdummy testrect testplatform testaudio testsurface

bin_PROGRAMS = runner
runner_SOURCES = runner.c support.c 
runner_CLAGS = -W -Wall -Wextra -g `sdl-config --cflags` -DSDL_NO_COMPAT
runner_LDADD = libtest.la
runner_LDFLAGS = `sdl-config --libs` 

lib_LTLIBRARIES = libtest.la
libtest_la_SOURCES = SDL_test.c logger_helpers.c plain_logger.c xml_logger.c xml.c \
	common/common.c common/img_blit.c common/img_blitblend.c common/img_face.c common/img_primitives.c common/img_primitivesblend.c
libtest_la_CLAGS = -fPIC -g
libtest_la_LDFLAGS = `sdl-config --libs`

libtest: libtest.la
		echo "Test library compiled."

all-local: install-tests
install: install-tests
install-tests:
	$(SHELL) build-scripts/install-tests.sh

clean-local:
	-rm -Rf tests/ docs/

