# On OS X, install the SDLMain.nib.
# We use an ugly hack to force the creation of the
# destination dir, maybe somebody with more automake
# experience knows how to do this properly?

if TARGET_MACOSX
masternibdatadir = $(datadir)/sdl/SDLMain.nib
masternibdata_DATA =		\
	SDLMain.nib

nibdatadir = $(datadir)/sdl
nibdata_DATA =			\
	SDLMain.nib/classes.nib	\
	SDLMain.nib/info.nib	\
	SDLMain.nib/objects.nib	\
	Info.plist
endif
