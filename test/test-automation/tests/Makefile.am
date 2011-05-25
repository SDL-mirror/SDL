lib_LTLIBRARIES = libtest.la
libtest_la_SOURCES = test.c asserts.c
libtest_la_CLAGS = -fPIC
libtest_la_LDFLAGS = `sdl-config --libs`

distclean-local:
	rm *.dylib

