
## Makefile.am for the SDL audio library

noinst_LTLIBRARIES = libaudio.la

# Define which subdirectories need to be built
SUBDIRS = @AUDIO_SUBDIRS@
DIST_SUBDIRS = alsa arts baudio dma dmedia dsp esd macrom nas nto openbsd \
	paudio sun ums windib windx5 disk mint dc

DRIVERS = @AUDIO_DRIVERS@

# Include the architecture-independent sources
COMMON_SRCS =			\
	SDL_audio.c		\
	SDL_audio_c.h		\
	SDL_audiocvt.c		\
	SDL_audiodev.c		\
	SDL_audiodev_c.h	\
	SDL_audiomem.c		\
	SDL_audiomem.h		\
	SDL_mixer.c		\
	SDL_sysaudio.h		\
	SDL_wave.c		\
	SDL_wave.h

libaudio_la_SOURCES = $(COMMON_SRCS)
libaudio_la_LIBADD = $(DRIVERS)
libaudio_la_DEPENDENCIES = $(DRIVERS)
