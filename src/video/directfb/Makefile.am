
## Makefile.am for SDL using the DirectFB video driver

INCLUDES = $(DIRECTFB_CFLAGS)

noinst_LTLIBRARIES = libvideo_directfb.la
libvideo_directfb_la_SOURCES = $(DIRECTFB_SRCS)
libvideo_directfb_la_LIBADD  = $(DIRECTFB_LIBS)

# The SDL DirectFB video driver sources
DIRECTFB_SRCS = 		\
	SDL_DirectFB_events.c	\
	SDL_DirectFB_events.h	\
	SDL_DirectFB_video.c	\
	SDL_DirectFB_video.h	\
	SDL_DirectFB_yuv.c	\
	SDL_DirectFB_yuv.h
