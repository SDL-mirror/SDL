/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/*
 * GEM SDL video driver implementation
 * inspired from the Dummy SDL driver
 * 
 * Patrice Mandin
 * and work from
 * Olivier Landemarre, Johan Klockars, Xavier Joubert, Claude Attard
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mint includes */
#include <gem.h>
#include <gemx.h>
#include <mint/osbind.h>
#include <sys/cookie.h>

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"

#include "SDL_ataric2p_s.h"
#include "SDL_ataric2p060_c.h"
#include "SDL_atarieddi_s.h"
#include "SDL_atarimxalloc_c.h"
#include "SDL_gemvideo.h"
#include "SDL_gemevents_c.h"
#include "SDL_gemmouse_c.h"
#include "SDL_gemwm_c.h"

/* Defines */

#define GEM_VID_DRIVER_NAME "gem"

/* Variables */

static unsigned char vdi_index[256] = {
	0,  2,  3,  6,  4,  7,  5,   8,
	9, 10, 11, 14, 12, 15, 13, 255
};

static const unsigned char empty_name[]="";

/* Initialization/Query functions */
static int GEM_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **GEM_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *GEM_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int GEM_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void GEM_VideoQuit(_THIS);

/* Hardware surface functions */
static int GEM_AllocHWSurface(_THIS, SDL_Surface *surface);
static int GEM_LockHWSurface(_THIS, SDL_Surface *surface);
static int GEM_FlipHWSurface(_THIS, SDL_Surface *surface);
static void GEM_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void GEM_FreeHWSurface(_THIS, SDL_Surface *surface);
static void GEM_UpdateRects(_THIS, int numrects, SDL_Rect *rects);
#if 0
static int GEM_ToggleFullScreen(_THIS, int on);
#endif

/* Internal functions */
static void GEM_FreeBuffers(_THIS);
static void refresh_window(_THIS, int winhandle, short *rect);

/* GEM driver bootstrap functions */

static int GEM_Available(void)
{
	short ap_id;
	const char *envr = getenv("SDL_VIDEODRIVER");

	/* Check if user asked a different video driver */
	if ((envr) && (strcmp(envr, GEM_VID_DRIVER_NAME)!=0)) {
		return 0;
	}

	/* Test if AES available */
	ap_id = appl_init();
	if (ap_id == -1)
		return 0;

	appl_exit();
	return 1;
}

static void GEM_DeleteDevice(SDL_VideoDevice *device)
{
	free(device->hidden);
	free(device);
}

static SDL_VideoDevice *GEM_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			free(device);
		}
		return(0);
	}
	memset(device->hidden, 0, (sizeof *device->hidden));

	atari_test_cpu060_present();

	/* Set the function pointers */
	device->VideoInit = GEM_VideoInit;
	device->ListModes = GEM_ListModes;
	device->SetVideoMode = GEM_SetVideoMode;
	device->SetColors = GEM_SetColors;
	device->UpdateRects = NULL /*GEM_UpdateRects*/;
	device->VideoQuit = GEM_VideoQuit;
	device->AllocHWSurface = GEM_AllocHWSurface;
	device->LockHWSurface = GEM_LockHWSurface;
	device->UnlockHWSurface = GEM_UnlockHWSurface;
	device->FlipHWSurface = GEM_FlipHWSurface;
	device->FreeHWSurface = GEM_FreeHWSurface;
	device->ToggleFullScreen = NULL /*GEM_ToggleFullScreen*/;

	/* Window manager */
	device->SetCaption = GEM_SetCaption;
	device->SetIcon = NULL /*GEM_SetIcon*/;
	device->IconifyWindow = GEM_IconifyWindow;
	device->GrabInput = GEM_GrabInput;

	/* Events */
	device->InitOSKeymap = GEM_InitOSKeymap;
	device->PumpEvents = GEM_PumpEvents;

	/* Mouse */
	device->FreeWMCursor = GEM_FreeWMCursor;
	device->CreateWMCursor = GEM_CreateWMCursor;
	device->ShowWMCursor = GEM_ShowWMCursor;
	device->WarpWMCursor = GEM_WarpWMCursor;
	device->CheckMouseMode = GEM_CheckMouseMode;

	device->free = GEM_DeleteDevice;

	return device;
}

VideoBootStrap GEM_bootstrap = {
	GEM_VID_DRIVER_NAME, "Atari GEM video driver",
	GEM_Available, GEM_CreateDevice
};

static void VDI_ReadExtInfo(_THIS, short *work_out)
{
	unsigned long EdDI_version;
	unsigned long cookie_EdDI;
	Uint32 num_colours;
	Uint16 clut_type, num_bits;

	/* Read EdDI informations */
	if  (Getcookie(C_EdDI, &cookie_EdDI) == C_NOTFOUND) {
		return;
	}
	
	EdDI_version = Atari_get_EdDI_version( (void *)cookie_EdDI);

	vq_scrninfo(VDI_handle, work_out);

	VDI_format = work_out[0];
	clut_type = work_out[1];
	num_bits = work_out[2];
	num_colours = *((Uint32 *) &work_out[3]);

	/* With EdDI>=1.1, we can have screen pitch, address and format
	 * so we can directly write to screen without using vro_cpyfm
	 */
	if (EdDI_version >= EDDI_11) {
		VDI_pitch = work_out[5];
		VDI_screen = (void *) *((unsigned long *) &work_out[6]);

		switch(num_colours) {
			case 32768UL:
				if (work_out[14] & (1<<7)) {
					/* Little endian */
					if (work_out[14] & (1<<1)) {
						/* Falcon */
						VDI_alphamask = 1 << 13;
						VDI_redmask = 31 << 3;
						VDI_greenmask = (3 << 14) | 7;
						VDI_bluemask = 31 << 8;
					} else {
						/* Others */
						VDI_alphamask = 1 << 7;
						VDI_redmask = 31 << 2;
						VDI_greenmask = (7 << 13) | 3;
						VDI_bluemask = 31 << 8;
					}
				} else {
					/* Big endian */
					if (work_out[14] & (1<<1)) {
						/* Falcon */
						VDI_alphamask = 1 << 5;
						VDI_redmask = 31 << 11;
						VDI_greenmask = 31 << 6;
						VDI_bluemask = 31;
					} else {
						/* Others */
						VDI_alphamask = 1 << 15;
						VDI_redmask = 31 << 10;
						VDI_greenmask = 31 << 5;
						VDI_bluemask = 31;
					}
				}
				break;
			case 65536UL:
				if (work_out[14] & (1<<7)) {
					/* Little endian */
					VDI_alphamask = 0;
					VDI_redmask = 31 << 3;
					VDI_greenmask = (7 << 13) | 7;
					VDI_bluemask = 31 << 8;
				} else {
					/* Big endian */
					VDI_alphamask = 0;
					VDI_redmask = 31 << 11;
					VDI_greenmask = 63 << 5;
					VDI_bluemask = 31;
				}
				break;
			case 16777216UL:
				if (work_out[14] & (1<<7)) {
					/* Little endian */
					switch(num_bits) {
						case 24:
							VDI_alphamask = 0;
							VDI_redmask = 255;
							VDI_greenmask = 255 << 8;
							VDI_bluemask = 255 << 16;
							break;
						case 32:
							VDI_alphamask = 255;
							VDI_redmask = 255 << 8;
							VDI_greenmask = 255 << 16;
							VDI_bluemask = 255 << 24;
							break;
					}
				} else {
					/* Big endian */
					switch(num_bits) {
						case 24:
							VDI_alphamask = 0;
							VDI_redmask = 255 << 16;
							VDI_greenmask = 255 << 8;
							VDI_bluemask = 255;
							break;
						case 32:
							VDI_alphamask = 255 << 24;
							VDI_redmask = 255 << 16;
							VDI_greenmask = 255 << 8;
							VDI_bluemask = 255;
							break;
					}
				}
				break;
		}
	}

	switch(clut_type) {
		case VDI_CLUT_HARDWARE:
			{
				int i;
				Uint16 *tmp_p;

				tmp_p = (Uint16 *)&work_out[16];

				for (i=0;i<256;i++) {
					vdi_index[i] = *tmp_p++;
				}
			}
			break;
		case VDI_CLUT_SOFTWARE:
			if (EdDI_version < EDDI_11) {
				int component; /* red, green, blue, alpha, overlay */
				int num_bit;
				unsigned short *tmp_p;

				/* We can build masks with info here */
				tmp_p = (unsigned short *) &work_out[16];
				for (component=0;component<5;component++) {
					for (num_bit=0;num_bit<16;num_bit++) {
						unsigned short valeur;

						valeur = *tmp_p++;

						if (valeur == 0xffff) {
							continue;
						}

						switch(component) {
							case 0:
								VDI_redmask |= 1<< valeur;
								break;
							case 1:
								VDI_greenmask |= 1<< valeur;
								break;
							case 2:
								VDI_bluemask |= 1<< valeur;
								break;
							case 3:
								VDI_alphamask |= 1<< valeur;
								break;
						}
					}
				}

				/* Remove lower green bits for Intel endian screen */
				if ((VDI_greenmask == ((7<<13)|3)) || (VDI_greenmask == ((7<<13)|7))) {
					VDI_greenmask &= ~(7<<13);
				}
			}
			break;
		case VDI_CLUT_NONE:
			break;
	}
}

int GEM_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	int i;
	short work_in[12], work_out[272], dummy;

	/* Open AES (Application Environment Services) */
	GEM_ap_id = appl_init();
	if (GEM_ap_id == -1) {
		fprintf(stderr,"Can not open AES\n");
		return 1;
	}

	/* Read version and features */
	GEM_version = aes_global[0];
	if (GEM_version >= 0x0400) {
		short ap_gout[4];
		
		GEM_wfeatures=0;
		if (appl_getinfo(AES_WINDOW, &ap_gout[0], &ap_gout[1], &ap_gout[2], &ap_gout[3])==0) {
			GEM_wfeatures=ap_gout[0];			
		}
	}	

	/* Ask VDI physical workstation handle opened by AES */
	VDI_handle = graf_handle(&dummy, &dummy, &dummy, &dummy);
	if (VDI_handle<1) {
		fprintf(stderr,"Wrong VDI handle %d returned by AES\n",VDI_handle);
		return 1;
	}

	/* Open virtual VDI workstation */
	work_in[0]=Getrez()+2;
	for(i = 1; i < 10; i++)
		work_in[i] = 1;
	work_in[10] = 2;

	v_opnvwk(work_in, &VDI_handle, work_out);
	if (VDI_handle == 0) {
		fprintf(stderr,"Can not open VDI virtual workstation\n");
		return 1;
	}

	/* Read fullscreen size */
	VDI_w = work_out[0] + 1;
	VDI_h = work_out[1] + 1;

	/* Read desktop size and position */
	if (!wind_get(DESKTOP_HANDLE, WF_WORKXYWH, &GEM_desk_x, &GEM_desk_y, &GEM_desk_w, &GEM_desk_h)) {
		fprintf(stderr,"Can not read desktop properties\n");
		return 1;
	}

	/* Read bit depth */
	vq_extnd(VDI_handle, 1, work_out);
	VDI_bpp = work_out[4];
	VDI_oldnumcolors=0;

	switch(VDI_bpp) {
		case 8:
			VDI_pixelsize=1;
			break;
		case 15:
		case 16:
			VDI_pixelsize=2;
			break;
		case 24:
			VDI_pixelsize=3;
			break;
		case 32:
			VDI_pixelsize=4;
			break;
		default:
			fprintf(stderr,"%d bits colour depth not supported\n",VDI_bpp);
			return 1;
	}

	/* Setup hardware -> VDI palette mapping */
	for(i = 16; i < 255; i++) {
		vdi_index[i] = i;
	}
	vdi_index[255] = 1;

	/* Save current palette */
	if (VDI_bpp>8) {
		VDI_oldnumcolors=1<<8;
	} else {
		VDI_oldnumcolors=1<<VDI_bpp;
	}
	
	for(i = 0; i < VDI_oldnumcolors; i++) {
		short rgb[3];

		vq_color(VDI_handle, i, 0, rgb);

		VDI_oldpalette[i][0] = rgb[0];
		VDI_oldpalette[i][1] = rgb[1];
		VDI_oldpalette[i][2] = rgb[2];
	}

	/* Setup screen info */
	GEM_title_name = empty_name;
	GEM_icon_name = empty_name;

	GEM_handle = -1;
	GEM_locked = SDL_FALSE;
	GEM_win_fulled = SDL_FALSE;

	VDI_screen = NULL;
	VDI_ReadExtInfo(this, work_out);
	if (VDI_screen == NULL) {
		VDI_pitch = VDI_w * ((VDI_bpp)>>3);
		VDI_format = VDI_FORMAT_UNKNOWN;
		VDI_redmask = VDI_greenmask = VDI_bluemask = VDI_alphamask = 0;
	}

	/* Setup destination mfdb */
	VDI_dst_mfdb.fd_addr = NULL;

	/* Update hardware info */
	this->info.hw_available = 0;
	this->info.video_mem = 0;

	/*	TC, screen : no shadow (direct)
     *  8P, screen: no shadow (direct)
     *  8I, screen: shadow, c2p (shadow -> c2p)
	 *  TC, no screen: shadow (vro_cpyfm)
	 *  8P, no screen: shadow (vro_cpyfm)
	 *  8I/U, no screen: shadow, shadow_c2p, c2p (shadow -> c2p -> vro_cpyfm)
	 */

	/* Determine the screen depth */
	/* we change this during the SDL_SetVideoMode implementation... */
	vformat->BitsPerPixel = VDI_bpp;

	/* Set mouse cursor to arrow */
	graf_mouse(ARROW, NULL);

	/* Init chunky to planar routine */
	Atari_C2pInit = Atari_C2pInit8;
	if (atari_cpu060_avail) {
		Atari_C2pConvert = Atari_C2pConvert8_060;
	} else {
		Atari_C2pConvert = Atari_C2pConvert8;
	}
	Atari_C2pInit();

	/* We're done! */
	return(0);
}

SDL_Rect **GEM_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	if (format->BitsPerPixel == VDI_bpp) {
		return((SDL_Rect **)-1);
	} else {
		return ((SDL_Rect **)NULL);
	}
}

static void GEM_FreeBuffers(_THIS)
{
	/* Release buffer */
	if ( GEM_buffer ) {
		free( GEM_buffer );
		GEM_buffer=NULL;
	}

	/* Destroy window */
	if (GEM_handle>=0) {
		wind_close(GEM_handle);
		wind_delete(GEM_handle);
		GEM_handle=-1;
	}
}

SDL_Surface *GEM_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	int maxwidth, maxheight;
	Uint32 modeflags, screensize;
	SDL_bool use_shadow;

	modeflags = SDL_HWPALETTE;
	GEM_FreeBuffers(this);

	/*--- Verify if asked mode can be used ---*/
	if (flags & SDL_FULLSCREEN) {
		maxwidth=VDI_w;
		maxheight=VDI_h;
	} else {
		/* Windowed mode */
		maxwidth=GEM_desk_w;
		maxheight=GEM_desk_h;
	}

	if ((maxwidth < width) || (maxheight < height) || (VDI_bpp != bpp)) {
		SDL_SetError("Couldn't find requested mode in list");
		return(NULL);
	}

	/*--- Allocate the new pixel format for the screen ---*/
	if ( ! SDL_ReallocFormat(current, VDI_bpp, VDI_redmask, VDI_greenmask, VDI_bluemask, VDI_alphamask) ) {
		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return(NULL);
	}

	/*--- Allocate shadow buffer if needed ---*/
	use_shadow=SDL_FALSE;
	if (flags & SDL_FULLSCREEN) {
		if (!VDI_screen) {
			use_shadow=SDL_TRUE;
		} else if (VDI_format==VDI_FORMAT_INTER) {
			use_shadow=SDL_TRUE;
		}
	} else {
		use_shadow=SDL_TRUE;
	}

	if (use_shadow) {
		screensize = width * height * VDI_pixelsize;

		GEM_buffer = Atari_SysMalloc(screensize, MX_PREFTTRAM);
		if (GEM_buffer==NULL) {
			fprintf(stderr,"Unable to allocate shadow buffer\n");
			return NULL;
		}
		memset(GEM_buffer, 0, screensize);
	}

	/*--- Initialize screen ---*/
	if (flags & SDL_FULLSCREEN) {
		short rgb[3]={0,0,0};
		short pxy[4];

		if (!GEM_locked) {
			/* Reserve memory space, used to be sure of compatibility */
			form_dial( FMD_START, 0,0,0,0, 0,0,VDI_w,VDI_h);
			/* Lock AES */
			while (!wind_update(BEG_UPDATE|BEG_MCTRL));

			GEM_locked=SDL_TRUE;
		}

		/* Clear screen */
		pxy[0] = pxy[1] = 0;
		pxy[2] = VDI_w - 1;
		pxy[3] = VDI_h - 1;
		vs_color(VDI_handle, vdi_index[0], rgb);
		vsf_color(VDI_handle,0);
		vsf_interior(VDI_handle,1);
		vsf_perimeter(VDI_handle,0);
		v_bar(VDI_handle,pxy);

		modeflags |= SDL_FULLSCREEN;
		if (VDI_screen && (VDI_format==VDI_FORMAT_PACK)) {
			modeflags |= SDL_HWSURFACE;
		} else {
			modeflags |= SDL_SWSURFACE;
		}
	} else {
		int posx,posy;
		short x2,y2,w2,h2;

		if (GEM_locked) {
			/* Restore screen memory, and send REDRAW to all apps */
			form_dial( FMD_FINISH, 0,0,0,0, 0,0,VDI_w,VDI_h);
			/* Unlock AES */
			wind_update(END_UPDATE|END_MCTRL);
			GEM_locked=SDL_FALSE;
		}

		/* Center our window */
		posx = GEM_desk_x;
		posy = GEM_desk_y;
		if (width<GEM_desk_w)
			posx += (GEM_desk_w - width) >> 1;
		if (height<GEM_desk_h)
			posy += (GEM_desk_h - height) >> 1;

		/* Calculate our window size and position */
		if (!(flags & SDL_NOFRAME)) {
			GEM_win_type=NAME|MOVER|CLOSER|SMALLER;
			if (flags & SDL_RESIZABLE) {
				GEM_win_type |= FULLER|SIZER;
				modeflags |= SDL_RESIZABLE;
			}
		} else {
			GEM_win_type=0;
			modeflags |= SDL_NOFRAME;
		}

		if (!wind_calc(0, GEM_win_type, posx, posy, width, height, &x2, &y2, &w2, &h2)) {
			GEM_FreeBuffers(this);
			fprintf(stderr,"Can not calculate window attributes\n");
			return NULL;
		}

		/* Create window */
		GEM_handle=wind_create(GEM_win_type, x2, y2, w2, h2);
		if (GEM_handle<0) {
			GEM_FreeBuffers(this);
			fprintf(stderr,"Can not create window\n");
			return NULL;
		}

		/* Setup window name */
		wind_set(GEM_handle,WF_NAME,(short)(((unsigned long)GEM_title_name)>>16),(short)(((unsigned long)GEM_title_name) & 0xffff),0,0);
	
		/* Open the window */
		wind_open(GEM_handle,x2,y2,w2,h2);

		modeflags |= SDL_SWSURFACE;
	}

	/* Set up the new mode framebuffer */
	current->flags = modeflags;
	current->w = width;
	current->h = height;
	if (use_shadow) {
		current->pixels = GEM_buffer;
		current->pitch = width * (VDI_bpp >> 3);
	} else {
		current->pixels = VDI_screen;
		current->pixels += VDI_pitch * ((VDI_h - height) >> 1);
		current->pixels += VDI_pixelsize * ((VDI_w - width) >> 1);
		current->pitch = VDI_pitch;
	}

	this->UpdateRects = GEM_UpdateRects;

	/* We're done */
	return(current);
}

/* We don't actually allow hardware surfaces other than the main one */
static int GEM_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return -1;
}
static void GEM_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int GEM_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

static void GEM_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static void GEM_UpdateRectsFullscreen(_THIS, int numrects, SDL_Rect *rects)
{
	SDL_Surface *surface;

	surface = this->screen;

	if (VDI_screen) {
		if (VDI_format==VDI_FORMAT_INTER) {
			void *destscr;
			int destx;
			
			destscr = VDI_screen;
			destscr += VDI_pitch * ((VDI_h - surface->h) >> 1);
			destx = (VDI_w - surface->w) >> 1;
			destx &= ~15;
			destscr += VDI_pixelsize * destx;

			/* Convert chunky to planar screen */
			Atari_C2pConvert(
				surface->pixels,
				destscr,
				surface->w,
				surface->h,
				SDL_FALSE,
				surface->pitch,
				VDI_pitch
			);
		}
	} else {
		MFDB mfdb_src;
		short blitcoords[8];
		int i;

		mfdb_src.fd_addr=surface->pixels;
		mfdb_src.fd_w=surface->w;
		mfdb_src.fd_h=surface->h;
		mfdb_src.fd_wdwidth=(surface->w) >> 4;
		mfdb_src.fd_stand=0;
	  	mfdb_src.fd_nplanes=surface->format->BitsPerPixel;
		mfdb_src.fd_r1=0;
		mfdb_src.fd_r2=0;
		mfdb_src.fd_r3=0;

		for ( i=0; i<numrects; ++i ) {
			blitcoords[0] = rects[i].x;
			blitcoords[1] = rects[i].y;
			blitcoords[2] = blitcoords[0] + rects[i].w - 1;
			blitcoords[3] = blitcoords[1] + rects[i].h - 1;

			blitcoords[4] = rects[i].x + ((VDI_w - surface->w) >> 1);
			blitcoords[5] = rects[i].y + ((VDI_h - surface->h) >> 1);
			blitcoords[6] = blitcoords[4] + rects[i].w - 1;
			blitcoords[7] = blitcoords[5] + rects[i].h - 1;

			vro_cpyfm(VDI_handle, S_ONLY, blitcoords, &mfdb_src, &VDI_dst_mfdb);
		}
	}
}

static void GEM_UpdateRectsWindowed(_THIS, int numrects, SDL_Rect *rects)
{
	short pxy[8], wind_pxy[8];
	int i;

	wind_get(GEM_handle, WF_WORKXYWH, &wind_pxy[0], &wind_pxy[1], &wind_pxy[2], &wind_pxy[3]);

	for ( i=0; i<numrects; ++i ) {
		pxy[0] = wind_pxy[0] + rects[i].x;
		pxy[1] = wind_pxy[1] + rects[i].y;
		pxy[2] = rects[i].w;
		pxy[3] = rects[i].h;

		GEM_wind_redraw(this, GEM_handle, pxy);
	}
}

static void GEM_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	SDL_Surface *surface;

	surface = this->screen;

	if (surface->flags & SDL_FULLSCREEN) {
		GEM_UpdateRectsFullscreen(this, numrects, rects);
	} else {
		GEM_UpdateRectsWindowed(this, numrects, rects);
	}
}

static int GEM_FlipHWSurfaceFullscreen(_THIS, SDL_Surface *surface)
{
	if (VDI_screen) {
		if (VDI_format==VDI_FORMAT_INTER) {
			void *destscr;
			int destx;
			
			/* Center on destination screen */
			destscr = VDI_screen;
			destscr += VDI_pitch * ((VDI_h - surface->h) >> 1);
			destx = (VDI_w - surface->w) >> 1;
			destx &= ~15;
			destscr += VDI_pixelsize * destx;

			/* Convert chunky to planar screen */
			Atari_C2pConvert(
				surface->pixels,
				destscr,
				surface->w,
				surface->h,
				SDL_FALSE,
				surface->pitch,
				VDI_pitch
			);
		}
	} else {
		MFDB mfdb_src;
		short blitcoords[8];

		mfdb_src.fd_addr=surface->pixels;
		mfdb_src.fd_w=surface->w;
		mfdb_src.fd_h=surface->h;
		mfdb_src.fd_wdwidth=(surface->w) >> 4;
		mfdb_src.fd_stand=0;
	  	mfdb_src.fd_nplanes=surface->format->BitsPerPixel;
		mfdb_src.fd_r1=0;
		mfdb_src.fd_r2=0;
		mfdb_src.fd_r3=0;

		blitcoords[0] = 0;
		blitcoords[1] = 0;
		blitcoords[2] = surface->w - 1;
		blitcoords[3] = surface->h - 1;
		blitcoords[4] = (VDI_w - surface->w) >> 1;
		blitcoords[5] = (VDI_h - surface->h) >> 1;
		blitcoords[6] = blitcoords[4] + surface->w - 1;
		blitcoords[7] = blitcoords[5] + surface->h - 1;

		vro_cpyfm(VDI_handle, S_ONLY, blitcoords, &mfdb_src, &VDI_dst_mfdb);
	}

	return(0);
}

static int GEM_FlipHWSurfaceWindowed(_THIS, SDL_Surface *surface)
{
	short	pxy[8];

	/* Update the whole window */
	wind_get(GEM_handle, WF_WORKXYWH, &pxy[0], &pxy[1], &pxy[2], &pxy[3]);

	GEM_wind_redraw(this, GEM_handle, pxy);

	return(0);
}

static int GEM_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	if (surface->flags & SDL_FULLSCREEN) {
		return GEM_FlipHWSurfaceFullscreen(this, surface);
	} else {
		return GEM_FlipHWSurfaceWindowed(this, surface);
	}
}

static int GEM_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	int i;
	SDL_Surface *surface;

	/* Do not change palette in True Colour */
	surface = this->screen;
	if (surface->format->BitsPerPixel > 8) {
		return 1;
	}


	for(i = 0; i < ncolors; i++)
	{
		int		r, g, b;
		short	rgb[3];

		r = colors[i].r;
		g = colors[i].g;
		b = colors[i].b;

		rgb[0] = (1000 * r) / 255;
		rgb[1] = (1000 * g) / 255;
		rgb[2] = (1000 * b) / 255;

		vs_color(VDI_handle, vdi_index[firstcolor+i], rgb);
	}

	return(1);
}

#if 0
static int GEM_ToggleFullScreen(_THIS, int on)
{
	if (on) {
		if (!GEM_locked) {
			/* Lock AES */
			while (!wind_update(BEG_UPDATE|BEG_MCTRL));
			GEM_locked=SDL_TRUE;
		}
	} else {
		if (GEM_locked) {
			/* Unlock AES */
			wind_update(END_UPDATE|END_MCTRL);
			GEM_locked=SDL_FALSE;
		}
		/* Redraw all screen */
	}

	return(1);
}
#endif

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void GEM_VideoQuit(_THIS)
{
	GEM_FreeBuffers(this);

	if (GEM_locked) {
		/* Restore screen memory, and send REDRAW to all apps */
		form_dial( FMD_FINISH, 0,0,0,0, 0,0,VDI_w,VDI_h);
		/* Unlock AES */
		wind_update(END_UPDATE|END_MCTRL);
		GEM_locked=SDL_FALSE;
	}

	/* Close AES application */
	appl_exit();

	/* Restore palette */
	if (VDI_oldnumcolors) {
		int i;

		for(i = 0; i < VDI_oldnumcolors; i++) {
			short	rgb[3];

			rgb[0] = VDI_oldpalette[i][0];
			rgb[1] = VDI_oldpalette[i][1];
			rgb[2] = VDI_oldpalette[i][2];

			vs_color(VDI_handle, i, rgb);
		}
	}

	/* Close VDI workstation */
	if (VDI_handle) {
		v_clsvwk(VDI_handle);
	}

	/* Free mode list */
	if (SDL_modelist[0]) {
		free(SDL_modelist[0]);
		SDL_modelist[0]=NULL;
	}

	this->screen->pixels = NULL;	
}

void GEM_wind_redraw(_THIS, int winhandle, short *inside)
{
	short todo[4];

	/* Tell AES we are going to update */
	while (!wind_update(BEG_UPDATE));

	v_hide_c(VDI_handle);

	/* Browse the rectangle list to redraw */
	wind_get(winhandle, WF_FIRSTXYWH, &todo[0], &todo[1], &todo[2], &todo[3]);

	while (todo[2] && todo[3]) {

		if (rc_intersect((GRECT *)inside,(GRECT *)todo)) {
			todo[2] += todo[0]-1;
			todo[3] += todo[1]-1;
			refresh_window(this, winhandle, todo);
		}

		wind_get(winhandle, WF_NEXTXYWH, &todo[0], &todo[1], &todo[2], &todo[3]);
	}

	/* Update finished */
	wind_update(END_UPDATE);

	v_show_c(VDI_handle,1);
}

static void refresh_window(_THIS, int winhandle, short *rect)
{
	MFDB	mfdb_src;
	short	pxy[8], wind_pxy[8];
	int iconified;
	SDL_Surface *surface;

	surface=this->screen;

	/* Is window iconified ? */
	iconified = 0;
	if (GEM_wfeatures & (1<<WF_ICONIFY)) {
		wind_get(winhandle, WF_ICONIFY, &wind_pxy[0], &wind_pxy[1], &wind_pxy[2], &wind_pxy[3]);
		iconified = pxy[0];
	}

	wind_get(winhandle, WF_WORKXYWH, &wind_pxy[0], &wind_pxy[1], &wind_pxy[2], &wind_pxy[3]);

	if (iconified) {
		/* Refresh icon */
		mfdb_src.fd_addr=surface->pixels;	/* Should be icon image */
		mfdb_src.fd_w=surface->w;
		mfdb_src.fd_h=surface->h;
		mfdb_src.fd_wdwidth=mfdb_src.fd_w>>4;
	  	mfdb_src.fd_stand=0;
  		mfdb_src.fd_nplanes=surface->format->BitsPerPixel;
		mfdb_src.fd_r1=0;
  		mfdb_src.fd_r2=0;
	  	mfdb_src.fd_r3=0;
	} else {
		/* Refresh window */
		mfdb_src.fd_addr=surface->pixels;
		mfdb_src.fd_w=surface->w;
		mfdb_src.fd_h=surface->h;
		mfdb_src.fd_wdwidth=mfdb_src.fd_w>>4;
	  	mfdb_src.fd_stand=0;
  		mfdb_src.fd_nplanes=surface->format->BitsPerPixel;
		mfdb_src.fd_r1=0;
  		mfdb_src.fd_r2=0;
	  	mfdb_src.fd_r3=0;
	}

	pxy[0] = rect[0] - wind_pxy[0];
	pxy[1] = rect[1] - wind_pxy[1];
 	pxy[2] = pxy[0] + rect[2] - rect[0];   
 	pxy[3] = pxy[1] + rect[3] - rect[1];  

	pxy[4] = rect[0];
	pxy[5] = rect[1];
	pxy[6] = rect[2];  
	pxy[7] = rect[3];

	vro_cpyfm( VDI_handle, S_ONLY, pxy, &mfdb_src, &VDI_dst_mfdb);
}
