
## Makefile.am for SDL using the MacOS X Quartz video driver

noinst_LTLIBRARIES = libvideo_quartz.la
libvideo_quartz_la_SOURCES = $(QUARTZ_SRCS)

# The SDL MacOS X Quartz video driver sources
QUARTZ_SRCS = 			\
	SDL_QuartzKeys.h	\
	SDL_QuartzVideo.h	\
	SDL_QuartzVideo.m

# These files are included by SDL_QuartzVideo.m (is that right??)
noinst_HEADERS =		\
	SDL_QuartzEvents.m	\
	SDL_QuartzWM.m		\
	SDL_QuartzWindow.m
