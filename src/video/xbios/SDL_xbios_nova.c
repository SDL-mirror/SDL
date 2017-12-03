/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/*
	Nova video card driver

	Patrice Mandin
*/

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/* Mint includes */
#include <mint/cookie.h>
#include <mint/sysvars.h>
#include <mint/osbind.h>

#include "SDL_video.h"

#include "../ataricommon/SDL_atarimxalloc_c.h"
#include "../ataricommon/SDL_atarisuper.h"
#include "SDL_xbios.h"
#include "SDL_xbios_nova.h"

#define NOVA_FILENAME	"\\auto\\sta_vdi.bib"

/*--- ---*/

static nova_xcb_t *NOVA_xcb;			/* Pointer to Nova infos */
static nova_resolution_t *NOVA_modes;	/* Video modes loaded from a file */
static int NOVA_modecount;				/* Number of loaded modes */
static unsigned char NOVA_blnk_time;	/* Original blank time */

/*--- Functions ---*/

static void XBIOS_DeleteDevice_NOVA(_THIS);

static void listModes(_THIS, int actually_add);
static void saveMode(_THIS, SDL_PixelFormat *vformat);
static void setMode(_THIS, xbiosmode_t *new_video_mode);
static void restoreMode(_THIS);
static void vsync_NOVA(_THIS);
static void getScreenFormat(_THIS, int bpp, Uint32 *rmask, Uint32 *gmask, Uint32 *bmask, Uint32 *amask);
static int getLineWidth(_THIS, xbiosmode_t *new_video_mode, int width, int bpp);
static void swapVbuffers(_THIS);
static int allocVbuffers(_THIS, int num_buffers, int bufsize);
static void freeVbuffers(_THIS);
static int setColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);

/* Internal functions */

static void NOVA_SetMode(_THIS, int num_mode);
static void NOVA_SetScreen(_THIS, void *screen);
static void NOVA_SetColor(_THIS, int index, int r, int g, int b);
static nova_resolution_t *NOVA_LoadModes(int *num_modes);

/* Nova driver bootstrap functions */

void SDL_XBIOS_VideoInit_Nova(_THIS, void *cookie_nova)
{
	NOVA_xcb = (nova_xcb_t *) cookie_nova;
	NOVA_modes = NULL;
	NOVA_modecount = 0;

	this->free = XBIOS_DeleteDevice_NOVA;

	/* Initialize video mode list */
	NOVA_modes = NOVA_LoadModes(&NOVA_modecount);
	if (!NOVA_modes) {
		return;
	}

	XBIOS_listModes = listModes;
	XBIOS_saveMode = saveMode;
	XBIOS_setMode = setMode;
	XBIOS_restoreMode = restoreMode;
	XBIOS_vsync = vsync_NOVA;
	XBIOS_getScreenFormat = getScreenFormat;
	XBIOS_getLineWidth = getLineWidth;
	XBIOS_swapVbuffers = swapVbuffers;
	XBIOS_allocVbuffers = allocVbuffers;
	XBIOS_freeVbuffers = freeVbuffers;

	this->SetColors = setColors;
}

static void XBIOS_DeleteDevice_NOVA(_THIS)
{
	if (NOVA_modes) {
		SDL_free(NOVA_modes);
	}

	SDL_free(this->hidden);
	SDL_free(this);
}

static void listModes(_THIS, int actually_add)
{
	int i;
	xbiosmode_t modeinfo;

	for (i=0; i<NOVA_modecount; i++) {
		int width = NOVA_modes[i].max_x + 1;
		int height = NOVA_modes[i].max_y + 1;
		int bpp = NOVA_modes[i].planes;

		/* Only 8, 16, 24, 32,bpp modes */
		if ((NOVA_modes[i].mode <= 1 ) || (NOVA_modes[i].mode == 3 ))
			continue;

		modeinfo.number = i;
		modeinfo.width = width;
		modeinfo.height = height;
		modeinfo.depth = bpp;
		modeinfo.flags = 0;

		SDL_XBIOS_AddMode(this, actually_add, &modeinfo);
	}
}

static void saveMode(_THIS, SDL_PixelFormat *vformat)
{
	int curwidth, curheight, curbpp;

	curwidth = NOVA_xcb->max_x + 1;
	curheight = NOVA_xcb->max_y + 1;
	curbpp = NOVA_xcb->planes;
	if (NOVA_xcb->mode == 3 ) {
		curbpp = 15;
	}

	/* Determine the current screen size */
	this->info.current_w = curwidth;
	this->info.current_h = curheight;

	/* Determine the screen depth (use default 8-bit depth) */
	vformat->BitsPerPixel = curbpp;

	/* Update hardware info */
	this->info.hw_available = 1;
	this->info.video_mem = NOVA_xcb->mem_size;

	XBIOS_oldvmode = NOVA_xcb->resolution;
	XBIOS_oldvbase = NOVA_xcb->base;

	/* TODO: save palette ? */

	NOVA_blnk_time = NOVA_xcb->blnk_time;
	NOVA_xcb->blnk_time = 0;
}

static void setMode(_THIS, xbiosmode_t *new_video_mode)
{
	NOVA_SetMode(this, new_video_mode->number);
}

static void restoreMode(_THIS)
{
	NOVA_SetScreen(this, XBIOS_oldvbase);
	NOVA_SetMode(this, XBIOS_oldvmode);

	/* TODO: restore palette ? */

	NOVA_xcb->blnk_time = NOVA_blnk_time;
}

static void vsync_NOVA(_THIS)
{
	NOVA_xcb->p_vsync();
}

static void getScreenFormat(_THIS, int bpp, Uint32 *rmask, Uint32 *gmask, Uint32 *bmask, Uint32 *amask)
{
	*rmask = *gmask = *bmask = *amask = 0;

	switch(bpp) {
		case 15:
			*rmask = 31<<2;
			*gmask = 3;
			*bmask = 31<<8;
			*amask = 1<<7;
			break;
		case 16:
			*rmask = 31<<3;
			*gmask = 7;
			*bmask = 31<<8;
			break;
		case 24:
			*rmask = 255;
			*gmask = 255<<8;
			*bmask = 255<<16;
			break;
		case 32:
			*rmask = 255<<24;
			*gmask = 255<<16;
			*bmask = 255<<8;
			*amask = 255;
			break;
	}
}

static int getLineWidth(_THIS, xbiosmode_t *new_video_mode, int width, int bpp)
{
	return (NOVA_modes[new_video_mode->number].pitch);
}

static void swapVbuffers(_THIS)
{
	NOVA_SetScreen(this, XBIOS_screens[XBIOS_fbnum]);
}

static int allocVbuffers(_THIS, int num_buffers, int bufsize)
{
	XBIOS_screens[0] = NOVA_xcb->base;
	if (num_buffers>1) {
		XBIOS_screens[1] = XBIOS_screens[0] + NOVA_xcb->scrn_sze;
	}

	return(1);
}

static void freeVbuffers(_THIS)
{
}

static int setColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	int i;

	for (i=0; i<ncolors; i++) {
		NOVA_SetColor(this, firstcolor+i,  colors[i].r, colors[i].g, colors[i].b);
	}

	return(1);
}

static void NOVA_SetMode(_THIS, int num_mode)
{
	void *oldstack;

	if ((num_mode<0) || (num_mode>=NOVA_modecount)) {
		return;
	}

	oldstack = (void *)Super(NULL);

	__asm__ __volatile__ (
			"moveql	#0,d0\n\t"
			"movel	%0,a0\n\t"
			"movel	%1,a1\n\t"
			"jsr	a1@"
		: /* no return value */
		: /* input */
			"g"(&NOVA_modes[num_mode]), "g"(NOVA_xcb->p_chres)
		: /* clobbered registers */
			"d0", "d1", "d2", "a0", "a1", "cc", "memory"
	);

	SuperToUser(oldstack);
}

static void NOVA_SetScreen(_THIS, void *screen)
{
	void *oldstack;

	oldstack = (void *)Super(NULL);

	__asm__ __volatile__ (
			"movel	%0,a0\n\t"
			"movel	%1,a1\n\t"
			"jsr	a1@"
		: /* no return value */
		: /* input */
			"g"(screen), "g"(NOVA_xcb->p_setscr)
		: /* clobbered registers */
			"d0", "d1", "d2", "a0", "a1", "cc", "memory"
	);

	SuperToUser(oldstack);
}

static void NOVA_SetColor(_THIS, int index, int r, int g, int b)
{
	Uint8 color[3];
	void *oldstack;

	color[0] = r;
	color[1] = g;
	color[2] = b;

	oldstack = (void *)Super(NULL);

	__asm__ __volatile__ (
			"movel	%0,d0\n\t"
			"movel	%1,a0\n\t"
			"movel	%2,a1\n\t"
			"jsr	a1@"
		: /* no return value */
		: /* input */
			"g"(index), "g"(color), "g"(NOVA_xcb->p_setcol)
		: /* clobbered registers */
			"d0", "d1", "d2", "a0", "a1", "cc", "memory"
	);

	SuperToUser(oldstack);
}

static nova_resolution_t *NOVA_LoadModes(int *num_modes)
{
	char filename[32];
	unsigned char bootdrive;
	void *oldstack;
	int handle, length;
	nova_resolution_t *buffer;

	/* Find boot drive */
	oldstack = (void *)Super(NULL);
	bootdrive='A'+ *((volatile unsigned short *)_bootdev);
	SuperToUser(oldstack);

	sprintf(&filename[0], "%c:" NOVA_FILENAME, bootdrive);

	/* Load file */
	handle = open(filename, O_RDONLY);
	if (handle<0) {
		SDL_SetError("Unable to open %s\n", filename);
		return NULL;
	}

	length = lseek(handle, 0, SEEK_END);
	lseek(handle, 0, SEEK_SET);

	buffer = (nova_resolution_t *)SDL_malloc(length);
	if (buffer==NULL) {
		SDL_SetError("Unable to allocate %d bytes\n", length);
		return NULL;
	}

	read(handle, buffer, length);
	close(handle);

	if (num_modes!=NULL) {
		*num_modes=length/sizeof(nova_resolution_t);
	}

	return buffer;
}
