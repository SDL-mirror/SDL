/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"
#include "SDL_os4video.h"
#include "SDL_os4utils.h"
#include "SDL_os4blit.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/Picasso96API.h>
#include <proto/icon.h>

//#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

/*
 * Libraries required by OS4 video driver
 */

static struct Library	*gfxbase;
static struct Library	*layersbase;
static struct Library	*p96base;
static struct Library	*intuitionbase;
static struct Library	*iconbase;
static struct Library	*workbenchbase;
static struct Library	*keymapbase;

struct GraphicsIFace	*SDL_IGraphics;
struct LayersIFace		*SDL_ILayers;
struct P96IFace			*SDL_IP96;
struct IntuitionIFace	*SDL_IIntuition;
struct IconIFace		*SDL_IIcon;
struct WorkbenchIFace	*SDL_IWorkbench;
struct KeymapIFace		*SDL_IKeymap;

#define MIN_LIB_VERSION	51

static BOOL open_libraries(void)
{
	gfxbase       = IExec->OpenLibrary("graphics.library", MIN_LIB_VERSION);
	layersbase    = IExec->OpenLibrary("layers.library", MIN_LIB_VERSION);
	p96base       = IExec->OpenLibrary("Picasso96API.library", 0);
	intuitionbase = IExec->OpenLibrary("intuition.library", MIN_LIB_VERSION);
	iconbase      = IExec->OpenLibrary("icon.library", MIN_LIB_VERSION);
	workbenchbase = IExec->OpenLibrary("workbench.library", MIN_LIB_VERSION);
	keymapbase    = IExec->OpenLibrary("keymap.library", MIN_LIB_VERSION);

	if (!gfxbase || !layersbase || !p96base || !intuitionbase || !iconbase || !workbenchbase || !keymapbase)
		return FALSE;

	SDL_IGraphics  = (struct GraphicsIFace *)  IExec->GetInterface(gfxbase, "main", 1, NULL);
	SDL_ILayers    = (struct LayersIFace *)    IExec->GetInterface(layersbase, "main", 1, NULL);
	SDL_IP96       = (struct P96IFace *)       IExec->GetInterface(p96base, "main", 1, NULL);
	SDL_IIntuition = (struct IntuitionIFace *) IExec->GetInterface(intuitionbase, "main", 1, NULL);
	SDL_IIcon      = (struct IconIFace *)      IExec->GetInterface(iconbase, "main", 1, NULL);
	SDL_IWorkbench = (struct WorkbenchIFace *) IExec->GetInterface(workbenchbase, "main", 1, NULL);
	SDL_IKeymap    = (struct KeymapIFace *)    IExec->GetInterface(keymapbase, "main", 1, NULL);

	if (!SDL_IGraphics || !SDL_ILayers || !SDL_IP96 || !SDL_IIntuition || !SDL_IIcon || !SDL_IWorkbench || !SDL_IKeymap)
		return FALSE;

	return TRUE;
}

static BOOL close_libraries(void)
{
	if (SDL_IKeymap) {
		IExec->DropInterface((struct Interface *) SDL_IKeymap);
		SDL_IKeymap = NULL;
	}
	if (SDL_IWorkbench) {
		IExec->DropInterface((struct Interface *) SDL_IWorkbench);
		SDL_IWorkbench = NULL;
	}
	if (SDL_IIcon) {
		IExec->DropInterface((struct Interface *) SDL_IIcon);
		SDL_IIcon = NULL;
	}
	if (SDL_IIntuition) {
		IExec->DropInterface((struct Interface *) SDL_IIntuition);
		SDL_IIntuition = NULL;
	}
	if (SDL_IP96) {
		IExec->DropInterface((struct Interface *) SDL_IP96);
		SDL_IP96 = NULL;
	}
	if (SDL_ILayers) {
		IExec->DropInterface((struct Interface *) SDL_ILayers);
		SDL_ILayers = NULL;
	}
	if (SDL_IGraphics) {
		IExec->DropInterface((struct Interface *) SDL_IGraphics);
		SDL_IGraphics = NULL;
	}

	if (keymapbase) {
		IExec->CloseLibrary(keymapbase);
		keymapbase = NULL;
	}
	if (workbenchbase) {
		IExec->CloseLibrary(workbenchbase);
		workbenchbase = NULL;
	}
	if (iconbase) {
		IExec->CloseLibrary(iconbase);
		iconbase = NULL;
	}
	if (intuitionbase) {
		IExec->CloseLibrary(intuitionbase);
		intuitionbase = NULL;
	}
	if (p96base) {
		IExec->CloseLibrary(p96base);
		p96base = NULL;
	}
	if (layersbase) {
		IExec->CloseLibrary(layersbase);
		layersbase = NULL;
	}
	if (gfxbase) {
		IExec->CloseLibrary(gfxbase);
		gfxbase = NULL;
	}
}


int os4video_Available(void);
SDL_VideoDevice *os4video_CreateDevice(int);

int 			os4video_VideoInit(_THIS, SDL_PixelFormat *vformat);
SDL_Rect **		os4video_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
SDL_Surface *	os4video_SetVideoMode(_THIS, SDL_Surface *current,
					int width, int height, int bpp, Uint32 flags);
int 			os4video_ToggleFullScreen(_THIS, int on);
void 			os4video_UpdateMouse(_THIS);
SDL_Overlay *	os4video_CreateYUVOverlay(_THIS, int width, int height,
	                                 Uint32 format, SDL_Surface *display);
int 			os4video_SetColors(_THIS, int firstcolor, int ncolors,
			 		SDL_Color *colors);
void 			os4video_UpdateRectsFullscreenDB(_THIS, int numrects, SDL_Rect *rects);
void 			os4video_UpdateRectsOffscreen(_THIS, int numrects, SDL_Rect *rects);
void 			os4video_UpdateRectsOffscreen_8bit(_THIS, int numrects, SDL_Rect *rects);
void 			os4video_UpdateRectsNone(_THIS, int numrects, SDL_Rect *rects);
void 			os4video_VideoQuit(_THIS);
int 			os4video_AllocHWSurface(_THIS, SDL_Surface *surface);
int 			os4video_CheckHWBlit(_THIS, SDL_Surface *src, SDL_Surface *dst);
int 			os4video_FillHWRect(_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color);
int 			os4video_SetHWColorKey(_THIS, SDL_Surface *surface, Uint32 key);
int 			os4video_SetHWAlpha(_THIS, SDL_Surface *surface, Uint8 value);
int 			os4video_LockHWSurface(_THIS, SDL_Surface *surface);
void 			os4video_UnlockHWSurface(_THIS, SDL_Surface *surface);
int 			os4video_FlipHWSurface(_THIS, SDL_Surface *surface);
void 			os4video_FreeHWSurface(_THIS, SDL_Surface *surface);
int 			os4video_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);
int 			os4video_GL_MakeCurrent(_THIS);
void 			os4video_GL_SwapBuffers(_THIS);
void *			os4video_GL_GetProcAddress(_THIS, const char *proc);
int 			os4video_GL_LoadLibrary(_THIS, const char *path);
void 			os4video_SetCaption(_THIS, const char *title, const char *icon);
void 			os4video_SetIcon(_THIS, SDL_Surface *icon, Uint8 *mask);
int 			os4video_IconifyWindow(_THIS);
SDL_GrabMode 	os4video_GrabInput(_THIS, SDL_GrabMode mode);
int 			os4video_GetWMInfo(_THIS, SDL_SysWMinfo *info);
void 			os4video_FreeWMCursor(_THIS, WMcursor *cursor);
WMcursor *		os4video_CreateWMCursor(_THIS,
					Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y);
int 			os4video_ShowWMCursor(_THIS, WMcursor *cursor);
void 			os4video_WarpWMCursor(_THIS, Uint16 x, Uint16 y);
void 			os4video_CheckMouseMode(_THIS);
void 			os4video_InitOSKeymap(_THIS);
void 			os4video_PumpEvents(_THIS);

int 			os4video_GL_Init(_THIS);
void 			os4video_GL_Term(_THIS);

extern BOOL os4video_PixelFormatFromModeID(SDL_PixelFormat *vformat, uint32 displayID);
void os4video_DeleteCurrentDisplay(_THIS, SDL_Surface *current, BOOL keepOffScreenBuffer);
extern void ResetMouseColors(_THIS);
extern void ResetMouseState(_THIS);
extern void os4video_ResetCursor(struct SDL_PrivateVideoData *hidden);
extern void DeleteAppIcon(_THIS);

VideoBootStrap os4video_bootstrap =
{
        "OS4",
		"AmigaOS 4 Video",
		os4video_Available,
		os4video_CreateDevice
};

static inline uint16
swapshort(uint16 x)
{
	return ((x&0xff)<<8) | ((x&0xff00)>>8);
}

int
os4video_Available(void)
{
	struct Library *p96;

	dprintf("Probing Picasso96API.library\n");

	p96 = IExec->OpenLibrary("Picasso96API.library", 0);
	if (p96)
	{
		dprintf("Success\n");
		IExec->CloseLibrary(p96);
		return 1;
	}

	dprintf("Not found\n");
	return 0;
}

void
os4video_DeleteDevice(_THIS)
{
	if (_this)
	{
		while (!IsMsgPortEmpty(_this->hidden->userPort))
		{
			struct Message *msg = IExec->GetMsg(_this->hidden->userPort);
			IExec->ReplyMsg(msg);
		}

		if (_this->hidden->mouse)
			IExec->FreeVec(_this->hidden->mouse);

		IExec->ObtainSemaphore(_this->hidden->poolSemaphore);
		IExec->FreeSysObject(ASOT_MEMPOOL,_this->hidden->pool);
		IExec->ReleaseSemaphore(_this->hidden->poolSemaphore);
		IExec->FreeSysObject(ASOT_SEMAPHORE, _this->hidden->poolSemaphore);
		IExec->FreeSysObject(ASOT_PORT, _this->hidden->userPort);
		IExec->FreeSysObject(ASOT_PORT, _this->hidden->appPort);

		if (_this->hidden->currentIcon)
		{
			SDL_IIcon->FreeDiskObject(_this->hidden->currentIcon);
		}

		close_libraries();

		IExec->FreeVec(_this->hidden);
		IExec->FreeVec(_this);

	}
}

SDL_VideoDevice *
os4video_CreateDevice(int devnum)
{
	SDL_VideoDevice *os4video_device;
	dprintf("Creating OS4 video device\n");

	os4video_device = (SDL_VideoDevice *)IExec->AllocVecTags(sizeof(SDL_VideoDevice), AVT_ClearWithValue, 0, AVT_Type, MEMF_SHARED, TAG_DONE );
	if (!os4video_device)
	{
		dprintf("No memory for device\n");
		SDL_OutOfMemory();
		return 0;
	}

	os4video_device->hidden = (struct SDL_PrivateVideoData *)IExec->AllocVecTags(sizeof(struct SDL_PrivateVideoData), AVT_ClearWithValue, 0, AVT_Type, MEMF_SHARED, TAG_DONE );
	if (!os4video_device)
	{
		SDL_OutOfMemory();
		dprintf("No memory for private data\n");
		IExec->FreeVec(os4video_device);
		return 0;
	}

	/* Create the semaphore used for the pool */
	os4video_device->hidden->poolSemaphore = IExec->AllocSysObjectTags(ASOT_SEMAPHORE, TAG_DONE);
	if (!os4video_device->hidden->poolSemaphore)
		goto fail;

	/* Create the pool we'll be using (Shared, might be used from threads) */
	os4video_device->hidden->pool = IExec->AllocSysObjectTags( ASOT_MEMPOOL,
	  ASOPOOL_MFlags,    MEMF_SHARED,
	  ASOPOOL_Threshold, 16384,
	  ASOPOOL_Puddle,    16384,
	  TAG_DONE );
	if (!os4video_device->hidden->pool)
		goto fail;

	/* Allocate some storage for the mouse pointer */
	os4video_device->hidden->mouse = IExec->AllocVecTags( 8, AVT_ClearWithValue, 0, AVT_Type, MEMF_SHARED, TAG_DONE );


	/* Setup the user port */
	os4video_device->hidden->userPort = IExec->AllocSysObject(ASOT_PORT, 0);
	if (!os4video_device->hidden->userPort)
		goto fail;

	/* Setup application port */
	os4video_device->hidden->appPort = IExec->AllocSysObject(ASOT_PORT, 0);
	if (!os4video_device->hidden->appPort)
		goto fail;

	if (!open_libraries())
		goto fail;

	SDL_strlcpy(os4video_device->hidden->currentCaption, "SDL_Window", 128);
	SDL_strlcpy(os4video_device->hidden->currentIconCaption, "SDL Application", 128);

	os4video_device->VideoInit = os4video_VideoInit;
	os4video_device->ListModes = os4video_ListModes;
	os4video_device->SetVideoMode = os4video_SetVideoMode;

	os4video_device->VideoQuit = os4video_VideoQuit;
    os4video_device->SetColors = os4video_SetColors;
    os4video_device->UpdateRects = os4video_UpdateRectsNone;
    os4video_device->AllocHWSurface = os4video_AllocHWSurface;
	os4video_device->CheckHWBlit = os4video_CheckHWBlit;
	os4video_device->FillHWRect = os4video_FillHWRect;
    os4video_device->LockHWSurface = os4video_LockHWSurface;
    os4video_device->UnlockHWSurface = os4video_UnlockHWSurface;
    os4video_device->FlipHWSurface = os4video_FlipHWSurface;
    os4video_device->FreeHWSurface = os4video_FreeHWSurface;
    os4video_device->SetCaption = os4video_SetCaption;
    os4video_device->SetIcon = os4video_SetIcon;
    os4video_device->IconifyWindow = os4video_IconifyWindow;
    os4video_device->GrabInput = os4video_GrabInput;
    os4video_device->InitOSKeymap = os4video_InitOSKeymap;
    os4video_device->PumpEvents = os4video_PumpEvents;
	os4video_device->CreateWMCursor = os4video_CreateWMCursor;
	os4video_device->ShowWMCursor = os4video_ShowWMCursor;
	os4video_device->FreeWMCursor = os4video_FreeWMCursor;
	os4video_device->WarpWMCursor = os4video_WarpWMCursor;
	os4video_device->UpdateMouse = os4video_UpdateMouse;
	os4video_device->CheckMouseMode = os4video_CheckMouseMode;
	os4video_device->ToggleFullScreen = os4video_ToggleFullScreen;

#if SDL_VIDEO_OPENGL
	os4video_device->GL_LoadLibrary = os4video_GL_LoadLibrary;
	os4video_device->GL_GetProcAddress = os4video_GL_GetProcAddress;
	os4video_device->GL_GetAttribute = os4video_GL_GetAttribute;
	os4video_device->GL_MakeCurrent = os4video_GL_MakeCurrent;
	os4video_device->GL_SwapBuffers = os4video_GL_SwapBuffers;
#endif

	os4video_device->free = os4video_DeleteDevice;

	dprintf("Device created\n");

	return os4video_device;

fail:
	SDL_OutOfMemory();

	close_libraries();

	if (os4video_device->hidden->userPort)
		IExec->FreeSysObject(ASOT_PORT, os4video_device->hidden->userPort);

	if (os4video_device->hidden->appPort)
		IExec->FreeSysObject(ASOT_PORT, os4video_device->hidden->appPort);

	if (os4video_device->hidden->poolSemaphore)
		IExec->FreeSysObject(ASOT_SEMAPHORE, os4video_device->hidden->poolSemaphore);

	if (os4video_device->hidden->pool)
		IExec->FreeSysObject(ASOT_MEMPOOL,os4video_device->hidden->pool);

	IExec->FreeVec(os4video_device->hidden);
	IExec->FreeVec(os4video_device);

	return 0;
}

int
os4video_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;
	uint32 displayID, freeMem;

	hidden->dontdeletecontext = FALSE;

	/* Get the default public screen. For the time being
	 * we don't care about its screen mode. Assume it's RTG.
	 */
	hidden->publicScreen = SDL_IIntuition->LockPubScreen(NULL);
	if (!hidden->publicScreen)
	{
		SDL_SetError("Cannot lock default PubScreen");
		return -1;
	}

	if (!SDL_IIntuition->GetScreenAttr(hidden->publicScreen, SA_DisplayID, &displayID, sizeof(uint32)))
	{
		SDL_SetError("Cannot get screen attributes\n");
		return -1;
	}

	/* Set up the integral SDL_VideoInfo */
	SDL_memset(&_this->info, 0, sizeof(_this->info));
	_this->info.hw_available = 1;
	_this->info.wm_available = 1;
	_this->info.blit_hw      = 1;
	_this->info.blit_fill    = 1;

	/* Get Video Mem */
	SDL_IP96->p96GetBoardDataTags(0, P96BD_FreeMemory, &freeMem);
	_this->info.video_mem = freeMem;

	if (FALSE == os4video_PixelFormatFromModeID(vformat, displayID))
	{
		SDL_SetError("Cannot get pixel format from screenmode ID");
		return -1;
	}

	return 0;
}

void
os4video_VideoQuit(_THIS)
{
	int i;
	struct SDL_PrivateVideoData *hidden;

	dprintf("In VideoQuit, this = %p\n", _this);

	hidden = _this->hidden;

	dprintf("DeleteCurrentDisplay\n");
	os4video_DeleteCurrentDisplay(_this, 0, FALSE);

	dprintf("Checking pubscreen\n");
	if (hidden->publicScreen)
	{
		SDL_IIntuition->UnlockPubScreen(NULL, hidden->publicScreen);
		hidden->publicScreen = NULL;
		for (i=0; i<RGBFB_MaxFormats; i++)
		{
			if (hidden->Modes[i]) IExec->FreeVec(hidden->Modes[i]);
		}
	}
}

SDL_Rect **
os4video_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;

	if ((flags & SDL_FULLSCREEN) != SDL_FULLSCREEN)
	{
		/* Unless fullscreen is requested, claim we support any size */
		return (SDL_Rect **)-1;
	}

	if (format->Rmask == 0)
	{
		/* Handle the case when we've been supplied a desired
		 * bits-per-pixel, but no colour masks.
		 *
		 * We search for screens modes in appropriate P96 pixel formats
		 * for this depth (in order of preference) until we find a pixel
		 * format that has valid modes.
		 */
		const RGBFTYPE *p96format;
		SDL_PixelFormat sdlformat;

		dprintf("Listing %d-bit screenmodes\n", format->BitsPerPixel);

		p96format = os4video_GetP96FormatsForBpp(format->BitsPerPixel);

		while (*p96format != RGBFB_NONE)
		{
			dprintf("Looking for modes with p96 format=%d\n", *p96format);

			if (hidden->Modes[*p96format] == NULL)
			{
				/* If we haven't already got a list of modes for this
				 * format, try to build one now
				 */
				os4video_PPFtoPF(&sdlformat, *p96format);
				hidden->Modes[*p96format] = os4video_MakeResArray(&sdlformat);
			}

			/* Stop searching, if we have modes in this p96 format */
			if (hidden->Modes[*p96format] != NULL)
			{
				dprintf("Found some\n");

				break;
			}

			/* Otherwise, no modes found for this format. Try next one */
			p96format++;
		}

		if (*p96format != RGBFB_NONE)
		{
			/* We found some modes with this format. Return the list */
			return hidden->Modes[*p96format];
		}
		else
		{
			dprintf("Found no %d-bit modes in any pixel format\n", format->BitsPerPixel);

			/* We found no modes in any p96format at this bpp */
			return (SDL_Rect **)NULL;
		}
	}
	else
    {
		/* We have been supplied colour masks. Look for modes
		 * which match precisely
		 */
		RGBFTYPE p96format = os4video_PFtoPPF(format);

		dprintf("Listing %d-bit modes with p96 format=%d\n", format->BitsPerPixel, p96format);

		if (p96format == RGBFB_NONE)
		{
			dprintf("No modes\n");
			return (SDL_Rect **)NULL;
		}

		if (hidden->Modes[p96format])
		{
			dprintf("Returning prebuilt array\n");
			return hidden->Modes[p96format];
		}

		hidden->Modes[p96format] = os4video_MakeResArray(format);

		return hidden->Modes[p96format];
	}
}


static struct Screen *openSDLscreen(int width, int height, uint32 modeId)
{
	uint32         openError = 0;
	struct Screen *scr;
	uint32         screen_width;
	uint32         screen_height;
	uint32         screen_leftedge = 0;

	/* Get the real width/height of this mode */
	screen_width  = SDL_IP96->p96GetModeIDAttr(modeId, P96IDA_WIDTH);
	screen_height = SDL_IP96->p96GetModeIDAttr(modeId, P96IDA_HEIGHT);

	/* If requested width is smaller than this mode's width, then centre
	 * the screen horizontally.
	 *
	 * A similar tactic won't work for centring vertically, because P96
	 * screens don't support the SA_Top propery. We'll tackle that
	 * another way shortly...
	 */
	if (width < (int)screen_width)
		screen_leftedge = (screen_width - width) / 2;

	/* Open the screen */
	scr = SDL_IIntuition->OpenScreenTags(NULL,
									 SA_Left,		screen_leftedge,
									 SA_Width, 		width,
									 SA_Height,		height,
									 SA_Depth,		8,
									 SA_DisplayID,	modeId,
									 SA_Quiet,		TRUE,
									 SA_ShowTitle,	FALSE,
									 SA_ErrorCode,	&openError,
									 TAG_DONE);
	if (!scr)
	{
		dprintf("Screen didn't open (err:%d)\n", openError);
		switch (openError)
		{
			case OSERR_NOMONITOR:
				SDL_SetError("Monitor for display mode not available");
				break;
			case OSERR_NOCHIPS:
				SDL_SetError("Newer custom chips requires (yeah, sure!)");
				break;
			case OSERR_NOMEM:
			case OSERR_NOCHIPMEM:
				SDL_OutOfMemory();
				break;
			case OSERR_PUBNOTUNIQUE:
				SDL_SetError("Public screen name not unique");
				break;
			case OSERR_UNKNOWNMODE:
			case OSERR_TOODEEP:
				SDL_SetError("Unknown display mode");
				break;
			case OSERR_ATTACHFAIL:
				SDL_SetError("Attachment failed");
				break;
			default:
				SDL_SetError("OpenScreen failed");
		}
		return NULL;
	}

	{
		/* Clear screen's bitmap */
		struct RastPort tmpRP;

		SDL_IGraphics->InitRastPort(&tmpRP);
		tmpRP.BitMap = scr->RastPort.BitMap;

		SDL_IP96->p96RectFill(&tmpRP, 0, 0, width, height, 0);
	}

	dprintf("Screen opened\n");

	return scr;
}

/*
 * Get the origin and dimensions of the visble area of the specified screen
 * as an IBox struct. This may be different than the Intuition screen as a whole
 * for "auto-scroll" screens.
 */
static BOOL getVisibleScreenBox (struct Screen *screen, struct IBox *screenBox)
{
	struct Rectangle dclip; /* The display clip - the bounds of the display mode
							 * relative to top/left corner of the overscan area */

	/* Get the screen's display clip */
	if (!SDL_IIntuition->GetScreenAttr (screen, SA_DClip, &dclip, sizeof dclip))
		return FALSE;

	/* Work out the geometry of the visible area
	 * Not sure this is 100% correct */
	screenBox->Left   = dclip.MinX - screen->ViewPort.DxOffset;
	screenBox->Top    = dclip.MinY - screen->ViewPort.DyOffset;
	screenBox->Width  = dclip.MaxX - dclip.MinX + 1;
	screenBox->Height = dclip.MaxY - dclip.MinY + 1;

	return TRUE;
}

/*
 * Calculate an appropriate position to place the top-left corner of a window of
 * size <width> x <height> on the specified screen.
 */
static void getBestWindowPosition (struct Screen *screen, int width, int height, uint32 *left, uint32 *top, BOOL addBorders)
{
	/* Geometry of screen's visible area */
	struct IBox screenBox =  {
		/* Some sensible defaults - just in case */
		0, 0, screen->Width - 1, screen->Height -1
	};

	/* Get geometry of visible screen */
	getVisibleScreenBox(screen, &screenBox);

	dprintf("Visible screen: (%d,%d)/(%dx%d)\n", screenBox.Left, screenBox.Top, screenBox.Width, screenBox.Height);

	if (addBorders)
	{
		/*  The supplied window dimensions don't include the window frame,
		 * so add on the size of the window borders
		 */
		height += screen->WBorTop + screen->Font->ta_YSize + 1 + screen->WBorBottom;
		width  += screen->WBorLeft + screen->WBorRight;
	}

	/* If window fits within the visible screen area, then centre,
	 * the window on that area; otherwise position the
	 * window at the top-left of the visible screen (which will do
	 * for just now).
	 */
	if (width >= screenBox.Width)
		*left = screenBox.Left;
	else
		*left = screenBox.Left + (screenBox.Width - width) / 2;

	if (height >= screenBox.Height)
		*top = screenBox.Top;
	else
		*top = screenBox.Top + (screenBox.Height - height) / 2;
}

/*
 * Layer backfill hook for the SDL window on high/true-colour screens
 */
static void do_blackBackFill (const struct Hook *hook, struct RastPort *rp, const int *message)
{
	struct Rectangle *rect = (struct Rectangle *)(message + 1);  // The area to back-fill
	struct RastPort backfillRP;

	/* Create our own temporary rastport so that we can render safely */
	SDL_IGraphics->InitRastPort(&backfillRP);
	backfillRP.BitMap = rp->BitMap;

	SDL_IP96->p96RectFill(&backfillRP, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY, 0);
}

static const struct Hook blackBackFillHook = {
	{0, 0},							/* h_MinNode */
	(ULONG(*)())do_blackBackFill,	/* h_Entry */
	0,								/* h_SubEntry */
	0		 						/* h_Data */
};

/*
 * Open and initialize intuition window for SDL to use
 */
static struct Window *
openSDLwindow(int width, int height, struct Screen *screen, struct MsgPort *userport, Uint32 flags, const char *caption)
{
	struct Window *w;
	uint32 windowFlags;
	uint32 IDCMPFlags  = IDCMP_NEWSIZE | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE
					   | IDCMP_DELTAMOVE | IDCMP_RAWKEY | IDCMP_ACTIVEWINDOW
					   | IDCMP_INACTIVEWINDOW | IDCMP_INTUITICKS
					   | IDCMP_EXTENDEDMOUSE;
	uint32 wX;
	uint32 wY;
	const struct Hook *backfillHook = &blackBackFillHook;

	if (flags & SDL_FULLSCREEN)
	{
		windowFlags = WFLG_BORDERLESS | WFLG_SIMPLE_REFRESH | WFLG_BACKDROP;
		wX = wY = 0;

		/* No-op backfill in full-screen mode */
		backfillHook = LAYERS_NOBACKFILL;
	}
	else
	{
		if (flags & SDL_NOFRAME)
			windowFlags = WFLG_SMART_REFRESH | WFLG_NOCAREREFRESH | WFLG_NEWLOOKMENUS | WFLG_BORDERLESS;
		else
		{
			windowFlags = WFLG_SMART_REFRESH | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET
						| WFLG_NOCAREREFRESH | WFLG_NEWLOOKMENUS;
			IDCMPFlags |= IDCMP_CLOSEWINDOW;
		}

		if (flags & SDL_RESIZABLE)
		{
			windowFlags |= WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM;
			IDCMPFlags  |= IDCMP_SIZEVERIFY;
		}

		getBestWindowPosition (screen, width, height, &wX, &wY, !(flags & SDL_NOFRAME));

		/* In windowed mode, use our custom backfill to clear the layer to black for
		 * high/true-colour screens; otherwise, use the default clear-to-pen-0
		 * backfill */
		if (SDL_IP96->p96GetBitMapAttr(screen->RastPort.BitMap, P96BMA_DEPTH) > 8)
			backfillHook = &blackBackFillHook;
		else
			backfillHook = LAYERS_BACKFILL;
	}

	windowFlags |= WFLG_REPORTMOUSE|WFLG_RMBTRAP;

	dprintf("Trying to open window at (%d,%d) of size (%dx%d)\n", wX, wY, width, height);

	w = SDL_IIntuition->OpenWindowTags (NULL,
									WA_Left,			wX,
									WA_Top,				wY,
									WA_InnerWidth,		width,
									WA_InnerHeight,		height,
									WA_Flags,			windowFlags,
									WA_PubScreen,		screen,
									WA_UserPort,		userport,
									WA_IDCMP,			IDCMPFlags,
									WA_AutoAdjust,		FALSE,
									WA_BackFill,		backfillHook,
									TAG_DONE);

	if (w)
	{
		if (flags & SDL_RESIZABLE)
		{
			/* If this window is resizable, reset window size limits
			 * so that the user can actually resize it.
			 *
			 * What's a useful minimum size, anyway?
			 */
			SDL_IIntuition->WindowLimits(w,
									 w->BorderLeft + w->BorderRight  + 100,
									 w->BorderTop  + w->BorderBottom + 100,
									 -1,
									 -1);
		}

		/* Set window titles */
		SDL_IIntuition->SetWindowTitles(w, caption, caption);

		/* We're ready to go. Bring screen to front
		 * and activate window.
		 */
		if (flags & SDL_FULLSCREEN)
			SDL_IIntuition->ScreenToFront(screen);

		SDL_IIntuition->ActivateWindow(w);
	}

	return w;
}


/*
 * Allocate and initialize an off-screen buffer suitable for use
 * as a software SDL surface with the specified SDL pixel format.
 */
static BOOL
initOffScreenBuffer(struct OffScreenBuffer *offBuffer, uint32 width, uint32 height, SDL_PixelFormat *format)
{
	BOOL     success   = FALSE;
	RGBFTYPE p96format = os4video_PFtoPPF(format);
	uint32   bpp       = os4video_RTGFB2Bits(p96format);

	/* Round up width/height to nearest 4 pixels */
	width  = (width + 3) & (~3);
	height = (height + 3) & (~3);

	dprintf("Allocating a %dx%dx%d off-screen buffer with rgbtype=%d\n", width, height, bpp, p96format);

	/* Allocate private p96 bitmap using the pixel format */
	offBuffer->bitmap = SDL_IP96->p96AllocBitMap(width, height, bpp, BMF_CLEAR | BMF_USERPRIVATE, NULL, p96format);

	if (offBuffer->bitmap != NULL)
	{
		offBuffer->width  =  width;
		offBuffer->height =  height;
		offBuffer->format = *format;

		offBuffer->pixels =  (void*)SDL_IP96->p96GetBitMapAttr(offBuffer->bitmap, P96BMA_MEMORY);
	    offBuffer->pitch  =         SDL_IP96->p96GetBitMapAttr(offBuffer->bitmap, P96BMA_BYTESPERROW);

		success = TRUE;
	}
	else
	{
		dprintf ("Failed to allocate off-screen buffer\n");
	}

	return success;
}

static void
freeOffScreenBuffer(struct OffScreenBuffer *offBuffer)
{
	if (offBuffer->bitmap)
	{
		dprintf("Freeing off-screen buffer\n");

		SDL_IP96->p96FreeBitMap(offBuffer->bitmap);

		offBuffer->bitmap = NULL;
	}
}

static BOOL
resizeOffScreenBuffer(struct OffScreenBuffer *offBuffer, uint32 width, uint32 height)
{
	BOOL success = TRUE;

	if (width  > offBuffer->width || height > offBuffer->height
		|| (offBuffer->width - 4) > width || (offBuffer->height - 4) > height)
	{
		/* If current surface is too small or too large, free it and
		 * create a new one
		 */
		SDL_PixelFormat format = offBuffer->format;		/* Remember the pixel format */
		freeOffScreenBuffer(offBuffer);
		success = initOffScreenBuffer(offBuffer, width, height, &format);
    }

	return success;
}


/*
 * Allocate and intialize resources required for supporting
 * full-screen double-buffering.
 */
static BOOL
initDoubleBuffering(struct DoubleBufferData *dbData, struct Screen *screen)
{
	dbData->sb[0] = SDL_IIntuition->AllocScreenBuffer(screen, NULL, SB_SCREEN_BITMAP);
	dbData->sb[1] = SDL_IIntuition->AllocScreenBuffer(screen, NULL, 0);

	dbData->SafeToFlip_MsgPort  = IExec->AllocSysObjectTags( ASOT_PORT, TAG_DONE );
	dbData->SafeToWrite_MsgPort = IExec->AllocSysObjectTags( ASOT_PORT, TAG_DONE );

	if (!dbData->sb[0] || !dbData->sb[1] || !dbData->SafeToFlip_MsgPort || !dbData->SafeToWrite_MsgPort)
	{
		dprintf("Failed\n");

		if (dbData->sb[0])
			SDL_IIntuition->FreeScreenBuffer(screen, dbData->sb[0]);
		if (dbData->sb[1])
			SDL_IIntuition->FreeScreenBuffer(screen, dbData->sb[1]);
		if (dbData->SafeToFlip_MsgPort)
			IExec->FreeSysObject(ASOT_PORT,dbData->SafeToFlip_MsgPort);
		if (dbData->SafeToWrite_MsgPort)
			IExec->FreeSysObject(ASOT_PORT,dbData->SafeToWrite_MsgPort);

		dbData->sb[0]  = 0;
		dbData->sb[1]  = 0;

		return FALSE;
	}

	/* Set up message ports for receiving SafeToWrite and SafeToFlip msgs
	 * from gfx.lib */
	dbData->sb[0]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort = dbData->SafeToWrite_MsgPort;
	dbData->sb[0]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort = dbData->SafeToFlip_MsgPort;
	dbData->sb[1]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort = dbData->SafeToWrite_MsgPort;
	dbData->sb[1]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort = dbData->SafeToFlip_MsgPort;

	dbData->SafeToFlip  = TRUE;
	dbData->SafeToWrite = TRUE;
	dbData->currentSB   = 1;

	return TRUE;
}

/*
 * Free resources associated with full-screen double-buffering
 */
static void
freeDoubleBuffering(struct DoubleBufferData *dbData, struct Screen *screen)
{
	if (dbData->SafeToFlip_MsgPort) {
		while (IExec->GetMsg (dbData->SafeToFlip_MsgPort) != NULL)
			;
		IExec->FreeSysObject (ASOT_PORT,dbData->SafeToFlip_MsgPort);
	}

	if (dbData->SafeToWrite_MsgPort) {
		while (IExec->GetMsg (dbData->SafeToWrite_MsgPort) != NULL)
			;
		IExec->FreeSysObject (ASOT_PORT,dbData->SafeToWrite_MsgPort);
	}

	dbData->SafeToFlip_MsgPort  = 0;
	dbData->SafeToWrite_MsgPort = 0;

	if (dbData->sb[0])
		SDL_IIntuition->FreeScreenBuffer(screen, dbData->sb[0]);
	if (dbData->sb[1])
		SDL_IIntuition->FreeScreenBuffer(screen, dbData->sb[1]);

	dbData->sb[0] = 0;
	dbData->sb[1] = 0;
}


void
os4video_DeleteCurrentDisplay(_THIS, SDL_Surface *current, BOOL keepOffScreenBuffer)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;

	DeleteAppIcon(_this);

	_this->UpdateRects = os4video_UpdateRectsNone;

	ResetMouseColors(_this);

#if SDL_VIDEO_OPENGL
	if (hidden->OpenGL)
		os4video_GL_Term(_this);
#endif

	if (hidden->scr) {
		SDL_IIntuition->ScreenToBack (hidden->scr);

		/* Wait for next frame to make sure screen has
		 * gone to back */
		SDL_IGraphics->WaitTOF();
	}

	if (hidden->win)
	{
		dprintf("Closing window\n");
		SDL_IIntuition->CloseWindow(hidden->win);
		hidden->win = 0;
	}

	if (hidden->scr)
	{
		dprintf("Freeing double-buffering resources\n");

		freeDoubleBuffering(&hidden->dbData, hidden->scr);

		dprintf("Closing screen\n");

		SDL_IIntuition->CloseScreen(hidden->scr);
		hidden->scr = 0;
	}

	if (!keepOffScreenBuffer)
		freeOffScreenBuffer(&hidden->offScreenBuffer);

	if (current)
	{
		current->flags &= ~SDL_HWSURFACE;

		if (current->hwdata)
			current->hwdata = 0;

		current->w = current->h = 0;
	}

	/* Clear hardware record for the display surface - just in case */
	hidden->screenHWData.type = hwdata_other;
	hidden->screenHWData.lock = 0;
	hidden->screenHWData.bm   = 0;
}

BOOL
os4video_CreateDisplay(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags, BOOL newOffScreenSurface)
{
	struct Screen *scr;
	struct SDL_PrivateVideoData *hidden = _this->hidden;
	int scr_depth;

	dprintf("Creating a %dx%dx%d display\n", width, height, bpp);

	// ALWAYS set prealloc
	current->flags |= SDL_PREALLOC;
	flags |= SDL_PREALLOC;

	if (flags & SDL_OPENGL)
	{
		flags |= SDL_HWSURFACE;

		// The double buffering is handled seperately for OpenGL
		flags &= ~SDL_DOUBLEBUF;
	}

	/*
	 * Set up hardware record for this display surface
	 */
	current->hwdata = &hidden->screenHWData;
	SDL_memset(current->hwdata, 0, sizeof(struct private_hwdata));

	/*
	 * Do we want a windowed or full-screen surface?
	 */
	if (!(flags & SDL_FULLSCREEN))
	{
		/* Windowed mode wanted. */

		dprintf("Window mode\n");

		/* Use the (already locked) public screen */
		scr = hidden->publicScreen;
		hidden->scr = 0;

		/* Check depth of screen */
		hidden->screenP96Format = SDL_IP96->p96GetBitMapAttr(scr->RastPort.BitMap, P96BMA_RGBFORMAT);
		scr_depth				= os4video_RTGFB2Bits(hidden->screenP96Format);

		dprintf("Screen depth:%d pixel format:%d\n", scr_depth, hidden->screenP96Format);

		if(scr_depth > 8)
		{
			/* Mark the surface as windowed */
			if( (flags&SDL_OPENGL) == 0 )
				flags &= ~(SDL_FULLSCREEN | SDL_HWSURFACE | SDL_DOUBLEBUF);
			current->flags  = flags;
		}
		else
		{

			/* Don't allow palette-mapped windowed surfaces -
			 * We can't get exclusive access to the palette and
			 * the results are ugly. Force a screen instead.
			 */
			dprintf("Forcing full-screenmode\n");

			flags |= SDL_FULLSCREEN;
			if( flags&SDL_OPENGL )
			{
				flags &= ~SDL_RESIZABLE;
			} else {
				flags &= ~(SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
			}
		}
	}

	if (flags & SDL_FULLSCREEN)
	{
		/* Full-screen wanted - open a custom screen */

		uint32 modeId;
		uint32 fmt;

		dprintf("Fullscreen\n");

		modeId = os4video_FindMode(width, height, bpp, flags | SDL_ANYFORMAT);

		if (modeId == INVALID_ID)
		{
			dprintf("No mode for this resolution\n");
			return FALSE;
		}

		/* Open screen */
		scr = hidden->scr = openSDLscreen(width, height, modeId);

		if (!scr)
			return FALSE;

		current->flags |= SDL_FULLSCREEN;

		/* Check if this screen uses a little-endian pixel format (e.g.,
		 * the early Radeon drivers only supported little-endian modes) which
		 * we cannot express to SDL with a simple shift and mask alone.
		 */
		fmt = SDL_IP96->p96GetModeIDAttr(modeId, P96IDA_RGBFORMAT);

		if (fmt == RGBFB_R5G6B5PC || fmt == RGBFB_R5G5B5PC
			|| fmt == RGBFB_B5G6R5PC || fmt == RGBFB_B5G5R5PC)
		{
			dprintf("Unsupported mode, switching to off-screen rendering\n");
			flags &= ~(SDL_HWSURFACE | SDL_DOUBLEBUF);
		}

		if (flags & SDL_HWSURFACE)
		{
			/* We render to the screen's BitMap */
			current->hwdata->bm = scr->RastPort.BitMap;

			/* Set update function */
			_this->UpdateRects = os4video_UpdateRectsNone;

			/* Mark the surface as fullscreen */
			current->flags |= SDL_HWSURFACE;
		}

		/* Check depth of screen */
		hidden->screenP96Format = SDL_IP96->p96GetBitMapAttr(scr->RastPort.BitMap, P96BMA_RGBFORMAT);
		scr_depth       		= os4video_RTGFB2Bits(hidden->screenP96Format);

		dprintf("Screen depth:%d pixel format:%d\n", scr_depth, hidden->screenP96Format);
	}

	/*
	 * Set up SDL pixel format for surface
	 */
	os4video_PPFtoPF(&hidden->screenFormat, hidden->screenP96Format);

	if (bpp == scr_depth)
	{
		/* Set pixel format of surface to match the screen's format */
		SDL_ReallocFormat (current, bpp,
						   hidden->screenFormat.Rmask,
						   hidden->screenFormat.Gmask,
						   hidden->screenFormat.Bmask,
						   hidden->screenFormat.Amask);
	}
	else
		/* Set pixel format of surface to default for this depth */
		SDL_ReallocFormat (current, bpp, 0, 0, 0, 0);

	/*
	 * Set up SWSURFACE
	 */
	if (!(flags & SDL_HWSURFACE))
	{
		/*
		 * Initialize off-screen buffer for this surface
		 */
		if (newOffScreenSurface && !initOffScreenBuffer(&hidden->offScreenBuffer, width, height, current->format))
		{
			dprintf ("Failed to allocate off-screen buffer\n");
			SDL_OutOfMemory();
			return FALSE;
		}

		/* We render to this off-screen bitmap */
		current->hwdata->bm = hidden->offScreenBuffer.bitmap;

		/* Set update function for windowed surface */
		if (bpp > 8 || scr_depth == 8  || (flags & SDL_FULLSCREEN))
			_this->UpdateRects = os4video_UpdateRectsOffscreen;
		else
			_this->UpdateRects = os4video_UpdateRectsOffscreen_8bit;
	}


	/*
	 * Set up double-buffering if requested
	 */
	if (flags & SDL_DOUBLEBUF)
	{
		dprintf("Allocating resources for double-buffering\n");

		if (initDoubleBuffering(&hidden->dbData, hidden->scr))
		{
			/* Set surface to render to off-screen buffer */
			current->hwdata->bm = hidden->dbData.sb[hidden->dbData.currentSB]->sb_BitMap;

			/* Double-buffering requires an update */
			_this->UpdateRects = os4video_UpdateRectsFullscreenDB;

			/* Mark the surface as double buffered */
			current->flags |= SDL_DOUBLEBUF;
		}
		else
		{
			SDL_IIntuition->CloseScreen(hidden->scr);
			hidden->scr = NULL;
			SDL_OutOfMemory();
			return FALSE;
		}
	}
	else
	{
		/* Single-buffered */
		current->flags &= ~SDL_DOUBLEBUF;
	}

	/* Setup current */
	current->w 		= width;
	current->h 		= height;
	current->pixels = (current->flags & SDL_HWSURFACE) ? (uint8*)0xdeadbeef : hidden->offScreenBuffer.pixels;
	current->pitch  = (current->flags & SDL_HWSURFACE) ? 0                  : hidden->offScreenBuffer.pitch;
	current->offset = 0;

	if (bpp <= 8)
		current->flags |= SDL_HWPALETTE;

	current->flags &= ~SDL_HWACCEL; //FIXME

	/* Init hwdata */
	current->hwdata->lock = 0;
	current->hwdata->type = (current->flags & SDL_HWSURFACE) ? hwdata_display_hw : hwdata_display_sw;

	/* Initialize pointer state */
	hidden->pointerState = pointer_dont_know;

	/*
	 * Open window
	 */
	hidden->win = openSDLwindow(width, height, scr, hidden->userPort, flags, hidden->currentCaption);

	if (!hidden->win)
	{
		dprintf("Failed\n");
		os4video_DeleteCurrentDisplay(_this, current, !newOffScreenSurface);
		return FALSE;
	}

#if SDL_VIDEO_OPENGL
	if (flags & SDL_OPENGL)
	{
//		dprintf("Checking for OpenGL\n");

		if (os4video_GL_Init(_this) != 0)
		{
//			dprintf("Failed OpenGL init\n");
			os4video_DeleteCurrentDisplay(_this, current, !newOffScreenSurface);
			return FALSE;
		}
		else
		{
//			dprintf("OpenGL init successfull\n");
			current->flags |= SDL_OPENGL;

			/* Hack. We assert HWSURFACE above to simplify
			 * initialization of GL surfaces, but we cannot pass these flags
			 * back to SDL.
			 * Need to re-work surface set-up code so that this nonsense isn't
			 * necessary
			 */
			current->flags &= ~SDL_HWSURFACE;
		}
	}
#endif
//	dprintf("Done\n");

	return TRUE;
}

#ifdef DEBUG
static char *get_flags_str(Uint32 flags)
{
    static char buffer[256];

    buffer[0] = '\0';

	if  (flags & SDL_ANYFORMAT)						SDL_strlcat(buffer, "ANYFORMAT ", 256);
	if  (flags & SDL_HWSURFACE)						SDL_strlcat(buffer, "HWSURFACE ", 256);
	if  (flags & SDL_HWPALETTE)						SDL_strlcat(buffer, "HWPALETTE ", 256);
	if  (flags & SDL_DOUBLEBUF)						SDL_strlcat(buffer, "DOUBLEFUF ", 256);
	if  (flags & SDL_FULLSCREEN)					SDL_strlcat(buffer, "FULLSCREEN ", 256);
	if  (flags & SDL_OPENGL)						SDL_strlcat(buffer, "OPENGL ", 256);
	if ((flags & SDL_OPENGLBLIT) == SDL_OPENGLBLIT)	SDL_strlcat(buffer, "OPENGLBLIT ", 256);
	if  (flags & SDL_RESIZABLE)						SDL_strlcat(buffer, "RESIZEABLE ", 256);
	if  (flags & SDL_NOFRAME)						SDL_strlcat(buffer, "NOFRAME ", 256);

    return buffer;
}
#endif

SDL_Surface *
os4video_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;
	BOOL needNew    = FALSE;
	BOOL needResize = FALSE;
	int success = TRUE;

	const Uint32 flagMask = SDL_HWPALETTE | SDL_DOUBLEBUF | SDL_FULLSCREEN
						  | SDL_OPENGL | SDL_OPENGLBLIT | SDL_RESIZABLE | SDL_NOFRAME | SDL_ANYFORMAT;

	dprintf("Requesting new video mode %dx%dx%d\n", width, height, bpp);
	dprintf("Requested flags: %s\n", get_flags_str(flags));
	dprintf("Current mode %dx%dx%d\n", current->w, current->h, current->format->BitsPerPixel);
	dprintf("Current mode flags %s\n", get_flags_str(current->flags));
	dprintf("Current hwdata %p\n", current->hwdata);

	/* If there's an existing primary surface open, check whether it fits the request */
	if (hidden->win)
	{
		/* Compare size */
		if (current->w != width || current->h != height)
		{
			if (!(current->flags & SDL_FULLSCREEN))
				/* If not full-screen, we can just re-size the window */
				needResize = TRUE;
			else
				/* Otherwise, we need to tear down and open a new screen */
				needNew = TRUE;
		}

		/* Compare depth */
		if (current->format->BitsPerPixel != bpp)
			needNew = TRUE;

		/* Compare flags */
		if ((current->flags & flagMask) ^ flags)
			needNew = TRUE;
	}
	else
		needNew = TRUE;


	/* Has just the surface size changed? */
	if (needResize && !needNew)
	{
		struct Window *w = hidden->win;

		/* If in windowed mode and only a change of size is requested.
		 * simply re-size the window. Don't bother opening a new one for this
		 * request */
		SDL_Lock_EventThread();

		dprintf("Resizing window: %dx%d\n", width, height);

		SDL_IIntuition->ChangeWindowBox(w,
									w->LeftEdge,
									w->TopEdge,
									w->BorderLeft + w->BorderRight  + width,
									w->BorderTop  + w->BorderBottom + height);

		/* We also need to re-size the OffscreenBuffer */
		success = resizeOffScreenBuffer(&hidden->offScreenBuffer, width, height);

		current->w = width;
		current->h = height;
		current->pixels = hidden->offScreenBuffer.pixels;
		current->pitch  = hidden->offScreenBuffer.pitch;

#if SDL_VIDEO_OPENGL
		if ((current->flags & SDL_OPENGL) == SDL_OPENGL)
		{
			/* Dimensions changed reallocate and update bitmaps. */
			if(hidden->m_frontBuffer)
			{
				SDL_IP96->p96FreeBitMap(hidden->m_frontBuffer);
				hidden->m_frontBuffer = NULL;
			}
			if(hidden->m_backBuffer)
			{
				SDL_IP96->p96FreeBitMap(hidden->m_backBuffer);
				hidden->m_backBuffer = NULL;
			}

			if(!(hidden->m_frontBuffer = SDL_IP96->p96AllocBitMap(width,height,16,BMF_MINPLANES | BMF_DISPLAYABLE,hidden->win->RPort->BitMap,0)))
			{
				dprintf("Fatal error: Can't allocate memory for buffer bitmap\n");
				SDL_Quit();
				return NULL;
			}

			if(!(hidden->m_backBuffer = SDL_IP96->p96AllocBitMap(width,height,16,BMF_MINPLANES | BMF_DISPLAYABLE,hidden->win->RPort->BitMap,0)))
			{
				SDL_IP96->p96FreeBitMap(hidden->m_frontBuffer);
				dprintf("Fatal error: Can't allocate memory for buffer bitmap\n");
				SDL_Quit();
				return NULL;
			}
			hidden->IGL->MGLUpdateContextTags(
							MGLCC_FrontBuffer,hidden->m_frontBuffer,
							MGLCC_BackBuffer,hidden->m_backBuffer,
							TAG_DONE);

	        hidden->IGL->GLViewport(0,0,width,height);
		}
#endif

		SDL_Unlock_EventThread();

		if (!success)
		{
			dprintf("Failed to resize window.\n");
			os4video_DeleteCurrentDisplay(_this, current, TRUE);
		}
	}

	if (needNew)
	{
		/* We need to create a new display */
		uint32 ow, oh, obpp, oflags;

		dprintf("Creating new display\n");

		SDL_Lock_EventThread();

		ow = current->w;
		oh = current->h;
		obpp = current->format->BitsPerPixel;
		oflags = current->flags & flagMask;

		/* Remove the old display (might want to resize window if not fullscreen) */
		dprintf("Deleting old display\n");
		os4video_DeleteCurrentDisplay(_this, current, FALSE);

		/* Open the new one */
		dprintf("Calling CreateDisplay\n");
		if (os4video_CreateDisplay(_this, current, width, height, bpp, flags, TRUE))
		{
			dprintf ("New display created\n");
			dprintf ("Obtained flags: %s\n", get_flags_str(current->flags));
		}
		else
		{
			success = FALSE;

			/* Didn't open, re-try the old mode if it's a valid mode */
			if (hidden->win)
			{
				dprintf("Retrying old mode\n");
				os4video_CreateDisplay(_this, current, ow, oh, obpp, oflags, TRUE);
			}
		}
		SDL_Unlock_EventThread();
	}

	return success ? current : NULL;
}


int	os4video_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;
	int i;
	uint32 *current;

	dprintf("Loading colors %d to %d\n", firstcolor, firstcolor+ncolors-1);
	if (!hidden->scr)
	{
		/* Windowed mode, or palette on direct color screen */
		dprintf("Windowed\n");
		dprintf("Using color format %d bpp, masks = %p, %p, %p\n",
			hidden->screenFormat.BitsPerPixel,
			hidden->screenFormat.Rmask,
			hidden->screenFormat.Gmask,
			hidden->screenFormat.Bmask);

		switch (hidden->screenP96Format)
		{
			case RGBFB_R5G6B5PC:
			case RGBFB_R5G5B5PC:
			case RGBFB_B5G6R5PC:
			case RGBFB_B5G5R5PC:
				dprintf("Little endian screen format\n");
				for (i = firstcolor; i < firstcolor+ncolors; i++)
				{
					hidden->offScreenBuffer.palette[i] =
						swapshort(SDL_MapRGB(&hidden->screenFormat,
							colors[i-firstcolor].r,
							colors[i-firstcolor].g,
							colors[i-firstcolor].b));
					hidden->currentPalette[i] = colors[i-firstcolor];
				}
				break;
			default:
				dprintf("Big endian screen format\n");
				for (i = firstcolor; i < firstcolor+ncolors; i++)
				{
					hidden->offScreenBuffer.palette[i] =
						SDL_MapRGB(&hidden->screenFormat,
							colors[i-firstcolor].r,
							colors[i-firstcolor].g,
							colors[i-firstcolor].b);
					hidden->currentPalette[i] = colors[i-firstcolor];
				}
				break;
		}
		/* Redraw screen so that palette changes take effect */
		SDL_UpdateRect(SDL_VideoSurface, 0,0,0,0);
	}
	else
	{
		/* Fullscreen mode. First, convert to LoadRGB32 format */
		uint32 colTable[ncolors*3+3];

		dprintf("Fullscreen\n");

		colTable[0] = (ncolors << 16) | firstcolor;

		current = &colTable[1];

		for (i = 0; i < ncolors; i++)
		{
			//dprintf("Setting %d to %d,%d,%d\n", i, colors[i].r, colors[i].g, colors[i].b);
			*current++ = ((uint32)colors[i/*+firstcolor*/].r << 24) | 0xffffff;
			*current++ = ((uint32)colors[i/*+firstcolor*/].g << 24) | 0xffffff;
			*current++ = ((uint32)colors[i/*+firstcolor*/].b << 24) | 0xffffff;
			hidden->currentPalette[i] = colors[i-firstcolor];
		}
		*current = 0;

		/* Load it */
		dprintf("Loading\n");
		SDL_IGraphics->LoadRGB32(&hidden->scr->ViewPort, colTable);
	}

	dprintf("Done\n");
	return 1;
}


int os4video_ToggleFullScreen(_THIS, int on)
{
	struct SDL_PrivateVideoData *hidden = _this->hidden;
	SDL_Surface *current = SDL_VideoSurface;
	Uint32 event_thread;
	uint32 oldFlags = current->flags,
		   newFlags = oldFlags,
 		   w = current->w,
		   h = current->h,
		   bpp = current->format->BitsPerPixel;
	SDL_Rect screenRect;

    // Don't switch if we don't own the window
    if (!hidden->windowActive)
    	return 0;

	dprintf("Trying to toggle fullscreen\n");
	dprintf("Current flags:%s\n", get_flags_str(current->flags));

	if (on)
	{
		newFlags |= SDL_FULLSCREEN;
	}
	else
	{
		newFlags &= ~SDL_FULLSCREEN;
	}

	screenRect.x = 0;
	screenRect.y = 0;
	screenRect.w = current->w;
	screenRect.h = current->h;

	/* FIXME: Save and transfer palette */
	/* Make sure we're the only one */
	event_thread = SDL_EventThreadID();
	if ( event_thread && (SDL_ThreadID() == event_thread) ) {
		event_thread = 0;
	}
	if ( event_thread ) {
		SDL_Lock_EventThread();
	}

  	hidden->dontdeletecontext = TRUE;

	/* Close old display */
	os4video_DeleteCurrentDisplay(_this, current, TRUE);

	/* Open the new one */
	if (os4video_CreateDisplay(_this, current, w, h, bpp, newFlags, FALSE))
	{
		hidden->dontdeletecontext = FALSE;

#if SDL_VIDEO_OPENGL
		if (oldFlags & SDL_OPENGL)
		{
			/* Dimensions changed reallocate and update bitmaps. */
			if(hidden->m_frontBuffer)
			{
				SDL_IP96->p96FreeBitMap(hidden->m_frontBuffer);
				hidden->m_frontBuffer = NULL;
			}
			if(hidden->m_backBuffer)
			{
				SDL_IP96->p96FreeBitMap(hidden->m_backBuffer);
				hidden->m_backBuffer = NULL;
			}

			if(!(hidden->m_frontBuffer = SDL_IP96->p96AllocBitMap(w,h,16,BMF_MINPLANES | BMF_DISPLAYABLE,hidden->win->RPort->BitMap,0)))
			{
				dprintf("Fatal error: Can't allocate memory for buffer bitmap\n");
				SDL_Quit();
				return -1;
			}

			if(!(hidden->m_backBuffer = SDL_IP96->p96AllocBitMap(w,h,16,BMF_MINPLANES | BMF_DISPLAYABLE,hidden->win->RPort->BitMap,0)))
			{
				SDL_IP96->p96FreeBitMap(hidden->m_frontBuffer);
				dprintf("Fatal error: Can't allocate memory for buffer bitmap\n");
				SDL_Quit();
				return -1;
			}

			hidden->IGL->MGLUpdateContextTags(
                                     MGLCC_FrontBuffer,		hidden->m_frontBuffer,
                                     MGLCC_BackBuffer,		hidden->m_backBuffer,
							TAG_DONE);

	        hidden->IGL->GLViewport(0,0,w,h);
		}
#endif

		if ( event_thread ) {
			SDL_Unlock_EventThread();
		}

		_this->UpdateRects(_this, 1, &screenRect);

		if (on && bpp == 8)
			_this->SetColors(_this, 0, 256, hidden->currentPalette);

		SDL_ResetKeyboard();
		ResetMouseState(_this);

		dprintf("Success\n");
	        dprintf("Obtained flags:%s\n", get_flags_str(current->flags));

		return 1;
	}

	dprintf("Switch failed, re-setting old display\n");
	if (os4video_CreateDisplay(_this, current, w, h, bpp, oldFlags, FALSE))
	{
		hidden->dontdeletecontext = FALSE;

#if SDL_VIDEO_OPENGL
		if (oldFlags & SDL_OPENGL)
		{
			/* Dimensions changed reallocate and update bitmaps. */
			if(hidden->m_frontBuffer)
			{
				SDL_IP96->p96FreeBitMap(hidden->m_frontBuffer);
				hidden->m_frontBuffer = NULL;
			}
			if(hidden->m_backBuffer)
			{
				SDL_IP96->p96FreeBitMap(hidden->m_backBuffer);
				hidden->m_backBuffer = NULL;
			}

			if(!(hidden->m_frontBuffer = SDL_IP96->p96AllocBitMap(w,h,16,BMF_MINPLANES | BMF_DISPLAYABLE,hidden->win->RPort->BitMap,0)))
			{
				dprintf("Fatal error: Can't allocate memory for buffer bitmap\n");
				SDL_Quit();
				return -1;
			}

			if(!(hidden->m_backBuffer = SDL_IP96->p96AllocBitMap(w,h,16,BMF_MINPLANES | BMF_DISPLAYABLE,hidden->win->RPort->BitMap,0)))
			{
				SDL_IP96->p96FreeBitMap(hidden->m_frontBuffer);
				dprintf("Fatal error: Can't allocate memory for buffer bitmap\n");
				SDL_Quit();
				return -1;
			}
			hidden->IGL->MGLUpdateContextTags(
							MGLCC_FrontBuffer,hidden->m_frontBuffer,
							MGLCC_BackBuffer,hidden->m_backBuffer,
							TAG_DONE);

	        hidden->IGL->GLViewport(0,0,w,h);
		}
#endif

		SDL_Unlock_EventThread();
		ResetMouseState(_this);

		_this->UpdateRects(_this, 1, &screenRect);

		if (!on && bpp == 8)
			_this->SetColors(_this, 0, 256, hidden->currentPalette);

		dprintf("No Success\n");
		return 0;
	}

	hidden->dontdeletecontext=FALSE;
#if SDL_VIDEO_OPENGL
	if (hidden->OpenGL)
		os4video_GL_Term(_this);
#endif

	/* If we get here, we're botched. */
	dprintf("Fatal error: Can't restart video\n");

	SDL_Quit();

	return -1;
}

#if SDL_VIDEO_OPENGL
/* These symbols are weak so they replace the MiniGL symbols in case libmgl.a is
 * not linked
 */

// Rich - These two declarations conflict with MiniGL 1.1 headers. Does this still
// even build with static libmgl? We can't do that for license reasons anyway...
//
//typedef struct {} GLcontext;

//GLcontext mini_CurrentContext __attribute__((weak));

#define WEAK(X)										\
void X(void) __attribute__((weak));					\
													\
void X(void) {};

WEAK(MGLCreateContextFromBitmap)
WEAK(MGLSetBitmap)
//WEAK(MGLGetProcAddress)
WEAK(MGLInit)
WEAK(MGLLockMode)
WEAK(MGLDeleteContext)
WEAK(MGLTerm)
WEAK(MGLUnlockDisplay)
WEAK(GLBegin)
WEAK(GLBindTexture)
WEAK(GLBlendFunc)
WEAK(GLColor4f)
WEAK(MGLSetState)
WEAK(GLEnd)
WEAK(GLFlush)
WEAK(GLGenTextures)
WEAK(GLGetString)
WEAK(GLLoadIdentity)
WEAK(GLMatrixMode)
WEAK(GLOrtho)
WEAK(GLPixelStorei)
WEAK(GLPopMatrix)
WEAK(GLPushMatrix)
WEAK(GLTexCoord2f)
WEAK(GLTexEnvi)
WEAK(GLTexImage2D)
WEAK(GLTexParameteri)
WEAK(GLTexSubImage2D)
WEAK(GLVertex4f)
WEAK(GLViewport)
WEAK(GLPushAttrib)
WEAK(GLPopAttrib)
WEAK(GLGetIntegerv)



#endif
