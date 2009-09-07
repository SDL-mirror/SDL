

CFLAGS  := -W -Wall -Wextra -g -I. `sdl-config --cflags`
LDFLAGS := `sdl-config --libs`

# If it doesn't pick up defaults
#CFLAGS  := -I. -D_GNU_SOURCE=1 -D_REENTRANT -I/usr/local/include/SDL
#LDFLAGS := -lm -ldl -lesd -lpthread

SRC         	:= testsdl.c \
						rwops/rwops.c \
						platform/platform.c \
						surface/surface.c \
						render/render.c \
						audio/audio.c
COMMON_SRC     := SDL_at.c common/common.c
COMMON_INCLUDE := SDL_at.h

TESTS_ALL := testsdl \
	rwops/rwops \
	platform/platform \
	surface/surface \
	render/render \
	audio/audio


.PHONY: all clean test


all: $(TESTS_ALL)

test: all
	@./testsdl

testsdl: $(SRC) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(COMMON_SRC)

rwops/rwops: rwops/rwops.c $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ rwops/rwops.c $(COMMON_SRC) -DTEST_STANDALONE

platform/platform: platform/platform.c $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ platform/platform.c $(COMMON_SRC) -DTEST_STANDALONE

surface/surface: surface/surface.c $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ surface/surface.c $(COMMON_SRC) -DTEST_STANDALONE

render/render: render/render.c $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ render/render.c $(COMMON_SRC) -DTEST_STANDALONE

audio/audio: audio/audio.c $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ audio/audio.c $(COMMON_SRC) -DTEST_STANDALONE

clean:
	$(RM) $(TESTS_ALL)
