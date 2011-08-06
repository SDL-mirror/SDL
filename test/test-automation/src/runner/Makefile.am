INCLUDE = -I../libtest -I../../include

bin_PROGRAMS = runner
runner_SOURCES = runner.c support.c
runner_LDADD = ../libtest/libtest.la
runner_CFLAGS = -W -Wall -Wextra -g `sdl-config --cflags` -DSDL_NO_COMPAT
runner_LDFLAGS = `sdl-config --libs`
