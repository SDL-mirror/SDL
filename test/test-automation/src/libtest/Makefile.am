lib_LTLIBRARIES = libtest.la
libtest_la_SOURCES = SDL_test.c \
					 logger_helpers.c \
					 plain_logger.c \
					 xml_logger.c xml.c \
					 common/common.c \
					 common/img_blit.c \
					 common/img_blitblend.c \
					 common/img_face.c \
					 common/img_primitives.c \
					 common/img_primitivesblend.c \
					 fuzzer/utl_crc32.c \
					 fuzzer/utl_md5.c \
					 fuzzer/utl_random.c \
					 fuzzer/fuzzer.c \
					 ../runner/logger.h
INCLUDE = -I../../include
libtest_la_CLAGS = -fPIC -g
libtest_la_LDFLAGS = `sdl-config --libs`

libtest: libtest.la
	echo "Test library compiled."
