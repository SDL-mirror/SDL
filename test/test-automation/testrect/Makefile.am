lib_LTLIBRARIES = libtestrect.la
libtestrect_la_SOURCES = testrect.c ../SDL_test.c ../logger.c ../logger_helpers.c ../plain_logger.c ../xml_logger.c ../xml.c
libtestrect_la_CLAGS = -fPIC -g
libtestrect_la_LDFLAGS = `sdl-config --libs`

distclean-local:
	-rm *.dylib
	-rm *.so

