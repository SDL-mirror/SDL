
## Makefile.am for SDL using the GGI video driver

noinst_LTLIBRARIES = libvideo_ggi.la
libvideo_ggi_la_SOURCES = $(GGI_SRCS)
libvideo_ggi_la_LIBADD = -lggi -lgii -lgg

# The SDL GGI video driver sources
GGI_SRCS = 			\
	SDL_ggikeys.h		\
	SDL_ggivideo.h		\
	SDL_ggievents.c		\
	SDL_ggimouse.c		\
	SDL_ggivideo.c		\
	SDL_ggievents_c.h	\
       	SDL_ggimouse_c.h
