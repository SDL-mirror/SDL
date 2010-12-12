/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <afletdinov@gmail.com>        *
 *                                                                         *
 *   WinCE RAW/GAPI video driver                                           *
 *                                                                         *
 *   Part of the SDL - (Simple DirectMedia Layer)                          *
 *   http://www.libsdl.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "SDL_config.h"

#if SDL_VIDEO_RENDER_GAPI

#include "SDL_win32video.h"
#include "SDL_win32window.h"
#include "../SDL_yuv_sw_c.h"

// RawFrameBufferInfo
typedef struct
{
   WORD wFormat;
   WORD wBPP;
   VOID *pFramePointer;
   int  cxStride;
   int  cyStride;
   int  cxPixels;
   int  cyPixels;
} RawFrameBufferInfo;

// GXDeviceInfo
typedef struct
{
    long Version;
    void* pvFrameBuffer;
    unsigned long cbStride;
    unsigned long cxWidth;
    unsigned long cyHeight;
    unsigned long cBPP;
    unsigned long ffFormat;
    char unknown[0x84 - 7 * 4];
} GXDeviceInfo;

// wince: GXDisplayProperties
struct GXDisplayProperties
{
    DWORD cxWidth;
    DWORD cyHeight;
    long cbxPitch;
    long cbyPitch;
    long cBPP;
    DWORD ffFormat;
};

// gx.dll
typedef int   (*PFNGXOpenDisplay)(HWND hWnd, DWORD dwFlags);
typedef int   (*PFNGXCloseDisplay)();
typedef void* (*PFNGXBeginDraw)();
typedef int   (*PFNGXEndDraw)();
typedef struct GXDisplayProperties (*PFNGXGetDisplayProperties)();
typedef int   (*PFNGXSuspend)();
typedef int   (*PFNGXResume)();

typedef struct
{
    // gx.dll
    HMODULE                   hGapiLib;
    PFNGXOpenDisplay          GXOpenDisplay;
    PFNGXCloseDisplay         GXCloseDisplay;
    PFNGXBeginDraw            GXBeginDraw;
    PFNGXEndDraw              GXEndDraw;
    PFNGXGetDisplayProperties GXGetDisplayProperties;
    PFNGXSuspend              GXSuspend;
    PFNGXResume               GXResume;
} GapiInfo;

//#ifndef DM_DISPLAYORIENTATION
//#define DM_DISPLAYORIENTATION 0x00800000L
//#endif

#define FORMAT_565		1
#define FORMAT_555		2
#define FORMAT_OTHER		3

#define GETRAWFRAMEBUFFER	0x00020001
#define GETGXINFO		0x00020000

#define kfPalette		0x10
#define kfDirect		0x20
#define kfDirect555		0x40
#define kfDirect565		0x80

#define GX_FULLSCREEN		0x01

enum ScreenOrientation { ORIENTATION_UNKNOWN = -1, ORIENTATION_UP = DMDO_0, ORIENTATION_DOWN = DMDO_180, ORIENTATION_LEFT = DMDO_270, ORIENTATION_RIGHT = DMDO_90 };
enum ScreenGeometry { GEOMETRY_UNKNOWN, GEOMETRY_PORTRAIT, GEOMETRY_LANDSCAPE, GEOMETRY_SQUARE };
enum FrameBufferFlags { FB_SKIP_OFFSET = 0x0001, FB_RAW_MODE = 0x0002, FB_SUSPENDED = 0x0004 };

// private framebuffer info
typedef struct
{
    int width;
    int height;
    int xpitch;
    int ypitch;
    int offset;
} FrameBufferInfo;

// private display data
typedef struct
{
    unsigned char* pixels;	// video memory
    int format;			// video format
    FrameBufferInfo fb;		// framebuffer geometry
    GapiInfo* gapi;		// GAPI module
    int userOrientation;
    int systemOrientation;
    int hardwareGeometry;
    int flags;			// fb flags
    float scale;		// scale pointer position
    int debug;

} WINCE_RenderData;

typedef struct
{
    SDL_SW_YUVTexture *yuv;
    Uint32 format;
    void *pixels;
    int pitch;

} WINCE_TextureData;


// system func
SDL_Renderer*	WINCE_CreateRenderer(SDL_Window* window, Uint32 flags);
void		WINCE_DestroyRenderer(SDL_Renderer* renderer);

int		WINCE_CreateTexture(SDL_Renderer* renderer, SDL_Texture* texture);
void		WINCE_DestroyTexture(SDL_Renderer* renderer, SDL_Texture* texture);
int		WINCE_QueryTexturePixels(SDL_Renderer* renderer, SDL_Texture* texture, void** pixels, int* pitch);
int		WINCE_UpdateTexture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch);
int		WINCE_LockTexture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* rect, int dirty, void** pixels, int* pitch);
void		WINCE_UnlockTexture(SDL_Renderer* renderer, SDL_Texture* texture);

int		WINCE_Available(void);
void		WINCE_SetupOrientation(WINCE_RenderData* data, int width, int height);

int		WINCE_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srect, const SDL_Rect* drect);
void		WINCE_ShowWindow(_THIS, SDL_Window* window, int visible);

void		WINCE_RenderPresent(SDL_Renderer* renderer);
int		WINCE_RenderDrawPoints(SDL_Renderer* renderer, const SDL_Point* points, int count);
int		WINCE_RenderDrawLines(SDL_Renderer* renderer, const SDL_Point* points, int count);
int		WINCE_RenderDrawRects(SDL_Renderer* renderer, const SDL_Rect ** rects, int count);
int		WINCE_RenderFillRects(SDL_Renderer* renderer, const SDL_Rect** rects, int count);

void		WINCE_PointerCoordinateTransform(SDL_Window* window, POINT* pt);
void		WINCE_DumpVideoInfo(WINCE_RenderData* data);
void		WINCE_PortraitTransform(WINCE_RenderData* data, int width, int height);
void		WINCE_LandscapeTransform(WINCE_RenderData* data, int width, int height);
void		WINCE_SquareTransform(WINCE_RenderData* data, int width, int height);
int		WINCE_FixedGeometry(FrameBufferInfo* fb, int bpp, int debug);
int		WINCE_GetDMOrientation(void);
int		WINCE_SetDMOrientation(int orientation);
void		WINCE_UpdateYUVTextureData(SDL_Texture* texture);

// gapi engine specific
int		GAPI_Init(WINCE_RenderData* data, HWND hwnd);
void		GAPI_Quit(WINCE_RenderData* data);

// raw engine specific
int		RAW_Init(WINCE_RenderData* data);
void		RAW_Quit(WINCE_RenderData* data);

// tools
void		FrameBufferRotate(FrameBufferInfo* src, int orientation);
int		GetFrameBufferOrientation(const FrameBufferInfo* src);
void		PointerRotate(POINT* pt, const FrameBufferInfo* fb, int orientation);
void		FrameBufferInitialize(FrameBufferInfo* fb);
void		FrameBufferDumpInfo(const FrameBufferInfo* fb, const char*);
const		char* GetOrientationName(int orientation);
void		UpdateLine16to16(const FrameBufferInfo* fb, const Uint16* src, Uint16* dst, Uint16 width);

// stdlib
inline int	__abs(int x){ return x < 0 ? -x : x; };
inline void	__swap(int* a, int* b){ int t = *a; *a = *b; *b = t; };

#define GAPI_RENDER_NAME	"gapi"
#define RAW_RENDER_NAME		"raw"
//
SDL_RenderDriver GAPI_RenderDriver = {
    WINCE_CreateRenderer,
    {
	GAPI_RENDER_NAME,
	(SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTDISCARD),
	(SDL_TEXTUREMODULATE_NONE),
	(SDL_BLENDMODE_NONE),
	(SDL_SCALEMODE_NONE),
	7,
	{
	    SDL_PIXELFORMAT_RGB555,
	    SDL_PIXELFORMAT_RGB565,
	    SDL_PIXELFORMAT_YV12,
	    SDL_PIXELFORMAT_IYUV,
	    SDL_PIXELFORMAT_YUY2,
	    SDL_PIXELFORMAT_UYVY,
	    SDL_PIXELFORMAT_YVYU
	},
	0,
	0
    }
};

SDL_RenderDriver RAW_RenderDriver = {
    WINCE_CreateRenderer,
    {
	RAW_RENDER_NAME,
	(SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTDISCARD),
	(SDL_TEXTUREMODULATE_NONE),
	(SDL_BLENDMODE_NONE),
	(SDL_SCALEMODE_NONE),
	7,
	{
	    SDL_PIXELFORMAT_RGB555,
	    SDL_PIXELFORMAT_RGB565,
	    SDL_PIXELFORMAT_YV12,
	    SDL_PIXELFORMAT_IYUV,
	    SDL_PIXELFORMAT_YUY2,
	    SDL_PIXELFORMAT_UYVY,
	    SDL_PIXELFORMAT_YVYU
	},
	0,
	0
    }
};

int WINCE_Available(void)
{
    const char* preferably = SDL_getenv("SDL_VIDEO_RENDERER");

    // raw check
    RawFrameBufferInfo rfbi = { 0 };
    HDC hdc = GetDC(NULL);
    int render_raw = ExtEscape(hdc, GETRAWFRAMEBUFFER, 0, NULL, sizeof(RawFrameBufferInfo), (char *) &rfbi);
    ReleaseDC(NULL, hdc);

    if(render_raw != 0 && rfbi.cxPixels != 0 && rfbi.cyPixels != 0 &&
        rfbi.pFramePointer != 0 && rfbi.cxStride != 0 && rfbi.cyStride != 0)
	    render_raw = 1;

    if(preferably && 0 == SDL_strcasecmp(preferably, RAW_RENDER_NAME)) return 0 != render_raw;

    // gapi check
    HMODULE render_gapi = LoadLibrary(TEXT("\\Windows\\gx.dll"));
    if(0 == render_gapi)
        render_gapi = LoadLibrary(TEXT("gx.dll"));
    FreeLibrary(render_gapi);

    if(preferably && 0 == SDL_strcasecmp(preferably, GAPI_RENDER_NAME)) return 0 != render_gapi;

    return 0 != render_raw || 0 != render_gapi;
}

void WINCE_AddRenderDriver(_THIS)
{
    HDC hdc;
    HMODULE render_gapi;
    int render_raw, ii;
    const char* preferably = SDL_getenv("SDL_VIDEO_RENDERER");

   // raw check
    RawFrameBufferInfo rfbi = { 0 };
    hdc = GetDC(NULL);
    render_raw = ExtEscape(hdc, GETRAWFRAMEBUFFER, 0, NULL, sizeof(RawFrameBufferInfo), (char *) &rfbi);
    ReleaseDC(NULL, hdc);

    if(render_raw != 0 && rfbi.cxPixels != 0 && rfbi.cyPixels != 0 &&
    	rfbi.pFramePointer != 0 && rfbi.cxStride != 0 && rfbi.cyStride != 0)
	    render_raw = 1;

    // gapi check
    render_gapi = LoadLibrary(TEXT("\\Windows\\gx.dll"));
    if(0 == render_gapi)
        render_gapi = LoadLibrary(TEXT("gx.dll"));

    if(render_gapi)
	FreeLibrary(render_gapi);

    for(ii = 0; ii < _this->num_displays; ++ii)
    {
	if(preferably)
	{
	    if(0 == SDL_strcasecmp(preferably, RAW_RENDER_NAME) && render_raw)
    		SDL_AddRenderDriver(&_this->displays[ii], &RAW_RenderDriver);
	    else
	    if(0 == SDL_strcasecmp(preferably, GAPI_RENDER_NAME) && render_gapi)
    		SDL_AddRenderDriver(&_this->displays[ii], &GAPI_RenderDriver);
	}
	else
	{
	    if(render_raw)
    		SDL_AddRenderDriver(&_this->displays[ii], &RAW_RenderDriver);
	    if(render_gapi)
    		SDL_AddRenderDriver(&_this->displays[ii], &GAPI_RenderDriver);
	}
    }
}

SDL_Renderer* WINCE_CreateRenderer(SDL_Window* window, Uint32 flags)
{
    SDL_VideoDisplay* display = window->display;
    SDL_DisplayMode* displayMode = &display->current_mode;
    SDL_WindowData* windowdata = (SDL_WindowData *) window->driverdata;
    SDL_Renderer* renderer;
    WINCE_RenderData* data;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if(!(window->flags & SDL_WINDOW_FULLSCREEN))
	window->flags |= SDL_WINDOW_FULLSCREEN;

    if(!SDL_PixelFormatEnumToMasks(displayMode->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask))
    {
        SDL_SetError("Unknown display format");
        return NULL;
    }

    switch(window->fullscreen_mode.format)
    {
	case SDL_PIXELFORMAT_RGB555:
	case SDL_PIXELFORMAT_RGB565:
	    break;

	default:
    	    SDL_SetError("Support only 16 or 15 bpp");
	    return NULL;
    }

    renderer = (SDL_Renderer*) SDL_calloc(1, sizeof(SDL_Renderer));
    if(!renderer)
    {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (WINCE_RenderData*) SDL_calloc(1, sizeof(WINCE_RenderData));
    if(!data)
    {
        WINCE_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }

    // initialize internal engine
    if(!RAW_Init(data) && !GAPI_Init(data, windowdata->hwnd))
    {
        WINCE_DestroyRenderer(renderer);
        return NULL;
    }


    // set debug
    data->debug	= SDL_getenv("DEBUG_VIDEO_GAPI") || SDL_getenv("GAPI_RENDERER_DEBUG") ? 1 : 0;
#if defined(DEBUG_VIDEO_GAPI) || defined(GAPI_RENDERER_DEBUG)
    data->debug	= 1;
#endif

    windowdata->videodata->render = data->gapi ? RENDER_GAPI : RENDER_RAW;
    windowdata->videodata->CoordTransform = WINCE_PointerCoordinateTransform;

    window->display->device->MaximizeWindow = NULL;
    window->display->device->MinimizeWindow = NULL;

    WINCE_SetupOrientation(data, window->w, window->h);

    renderer->CreateTexture = WINCE_CreateTexture;
    renderer->DestroyTexture = WINCE_DestroyTexture;
    renderer->QueryTexturePixels = WINCE_QueryTexturePixels;
    renderer->UpdateTexture = WINCE_UpdateTexture;
    renderer->LockTexture = WINCE_LockTexture;
    renderer->UnlockTexture = WINCE_UnlockTexture;

    renderer->RenderCopy = WINCE_RenderCopy;
    renderer->DestroyRenderer = WINCE_DestroyRenderer;

    renderer->RenderPresent = WINCE_RenderPresent;
    renderer->RenderDrawPoints = WINCE_RenderDrawPoints;
    renderer->RenderDrawLines = WINCE_RenderDrawLines;
    renderer->RenderDrawRects = WINCE_RenderDrawRects;
    renderer->RenderFillRects = WINCE_RenderFillRects;

    renderer->info = data->gapi ? GAPI_RenderDriver.info : RAW_RenderDriver.info;

    renderer->window = window;
    renderer->driverdata = data;

    return renderer;
}

void WINCE_DestroyRenderer(SDL_Renderer* renderer)
{
    WINCE_RenderData *renderdata = (WINCE_RenderData*) renderer->driverdata;

    if(renderdata)
    {
	if(renderdata->gapi)
    	    GAPI_Quit(renderdata);
	else
	    RAW_Quit(renderdata);

        SDL_free(renderdata);
    }

    SDL_free(renderer);
}

int WINCE_CreateTexture(SDL_Renderer* renderer, SDL_Texture* texture)
{
    WINCE_TextureData* texturedata = (WINCE_TextureData*) SDL_calloc(1, sizeof(WINCE_TextureData));
    if(NULL == texturedata)
    {
        SDL_OutOfMemory();
        return -1;
    }

    texturedata->pitch = texture->w * SDL_BYTESPERPIXEL(texture->format);
    texturedata->pixels = SDL_malloc(texture->h * texturedata->pitch);
    if(NULL == texturedata->pixels)
    {
        SDL_OutOfMemory();
        return -1;
    }

    if(SDL_ISPIXELFORMAT_FOURCC(texture->format))
    {
	texturedata->yuv = SDL_SW_CreateYUVTexture(texture->format, texture->w, texture->h);
        if(NULL == texturedata->yuv)
	{
    	    SDL_OutOfMemory();
            return -1;
        }
	SDL_Window* window = renderer->window;
	SDL_VideoDisplay* display = window->display;
	texturedata->format = display->current_mode.format;
    }
    else
    {
	texturedata->yuv = NULL;
	texturedata->format = texture->format;
    }

    texture->driverdata = texturedata;

    return 0;
}

void WINCE_DestroyTexture(SDL_Renderer* renderer, SDL_Texture* texture)
{
    WINCE_TextureData *texturedata = (WINCE_TextureData*) texture->driverdata;

    if(texturedata)
    {
	if(texturedata->yuv) SDL_SW_DestroyYUVTexture(texturedata->yuv);
	if(texturedata->pixels) SDL_free(texturedata->pixels);
	SDL_free(texturedata);
	texture->driverdata = NULL;
    }
}

int WINCE_QueryTexturePixels(SDL_Renderer* renderer, SDL_Texture* texture, void** pixels, int* pitch)
{
    WINCE_TextureData* texturedata = (WINCE_TextureData*) texture->driverdata;

    if(texturedata->yuv)
        return SDL_SW_QueryYUVTexturePixels(texturedata->yuv, pixels, pitch);

    *pixels = texturedata->pixels;
    *pitch = texturedata->pitch;

    return 0;
}

int WINCE_UpdateTexture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch)
{
    WINCE_TextureData* texturedata = (WINCE_TextureData*) texture->driverdata;

    if(texturedata->yuv)
    {
        if(SDL_SW_UpdateYUVTexture(texturedata->yuv, rect, pixels, pitch) < 0)
            return -1;
        WINCE_UpdateYUVTextureData(texture);
        return 0;
    }

    if(0 < rect->w && 0 < rect->h)
    {
	const unsigned char *src = ((const unsigned char*) pixels);
	unsigned char *dst = ((unsigned char*) texturedata->pixels) +
				rect->y * texturedata->pitch +
				rect->x * SDL_BYTESPERPIXEL(texture->format);
        int length = rect->w * SDL_BYTESPERPIXEL(texture->format);
	int height = rect->h;

	while(height--)
	{
	    SDL_memcpy(dst, src, length);
	    dst += texturedata->pitch;
	    src += pitch;
	}
    }

    return 0;
}

int WINCE_LockTexture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* rect, int dirty, void** pixels, int* pitch)
{
    WINCE_TextureData *texturedata = (WINCE_TextureData*) texture->driverdata;

    if(texturedata->yuv)
        return SDL_SW_LockYUVTexture(texturedata->yuv, rect, dirty, pixels, pitch);

    *pixels = (void *) ((unsigned char*) texturedata->pixels +
		    rect->y * texturedata->pitch +
		    rect->x * SDL_BYTESPERPIXEL(texture->format));
    *pitch = texturedata->pitch;
}

void WINCE_UnlockTexture(SDL_Renderer* renderer, SDL_Texture* texture)
{
    WINCE_TextureData *texturedata = (WINCE_TextureData*) texture->driverdata;

    if(texturedata->yuv)
    {
        SDL_SW_UnlockYUVTexture(texturedata->yuv);
	WINCE_UpdateYUVTextureData(texture);
    }
}

int WINCE_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srect, const SDL_Rect* drect)
{
    WINCE_RenderData* dstdata = (WINCE_RenderData*) renderer->driverdata;
    WINCE_TextureData* srcdata = (WINCE_TextureData*) texture->driverdata;

    if((dstdata->flags & FB_SUSPENDED) ||
       0 >= srect->w || 0 >= srect->h) return;

    // lock gapi
    if(dstdata->gapi) dstdata->gapi->GXBeginDraw();

    const unsigned char *src = ((const unsigned char*) srcdata->pixels);
    unsigned char *dst = dstdata->pixels + (dstdata->flags & FB_SKIP_OFFSET ? 0 : dstdata->fb.offset) +
				drect->y * dstdata->fb.ypitch +
				drect->x * dstdata->fb.xpitch;
    if(srcdata->yuv)
    {
	return SDL_SW_CopyYUVToRGB(srcdata->yuv,
                                   srect, srcdata->format,
                                   drect->w, drect->h, dst,
                                   dstdata->fb.ypitch);
    }
    else
    {
	int height = drect->h;
	int length = drect->w * SDL_BYTESPERPIXEL(texture->format); // in bytes

	while(height--)
	{
	    switch(SDL_BYTESPERPIXEL(texture->format))
	    {
		case 2: UpdateLine16to16(&dstdata->fb, (Uint16*) src, (Uint16*) dst, length >> 1); break;

		default: break;
	    }

	    dst += dstdata->fb.ypitch;
	    src += srcdata->pitch;
	}
    }

    // unlock gapi
    if(dstdata->gapi) dstdata->gapi->GXEndDraw();

    return 0;
}

void WINCE_RenderPresent(SDL_Renderer* renderer)
{
}

int WINCE_RenderDrawPoints(SDL_Renderer* renderer, const SDL_Point* points, int count)
{
    SDL_Unsupported();
    return -1;
}

int WINCE_RenderDrawLines(SDL_Renderer* renderer, const SDL_Point* points, int count)
{
    SDL_Unsupported();
    return -1;
}

int WINCE_RenderDrawRects(SDL_Renderer* renderer, const SDL_Rect ** rects, int count)
{
    SDL_Unsupported();
    return -1;
}

int WINCE_RenderFillRects(SDL_Renderer* renderer, const SDL_Rect** rects, int count)
{
    SDL_Unsupported();
    return -1;
}



void WINCE_SetupOrientation(WINCE_RenderData* data, int width, int height)
{
    const float maxW1 = GetSystemMetrics(SM_CXSCREEN) > GetSystemMetrics(SM_CYSCREEN) ? GetSystemMetrics(SM_CXSCREEN) : GetSystemMetrics(SM_CYSCREEN);
    const float maxW2 = data->fb.width > data->fb.height ? data->fb.width : data->fb.height;

    // scale define
    data->scale = maxW2 / maxW1;

    // init fb values
    FrameBufferInitialize(&data->fb);

    // orientation values
    data->userOrientation = ORIENTATION_UP;
    data->systemOrientation = WINCE_GetDMOrientation();
    data->hardwareGeometry = data->fb.width == data->fb.height ? GEOMETRY_SQUARE :
				(data->fb.width < data->fb.height ? GEOMETRY_PORTRAIT : GEOMETRY_LANDSCAPE);

    if(data->debug)
	WINCE_DumpVideoInfo(data);

    if(data->systemOrientation == ORIENTATION_UNKNOWN)
	data->systemOrientation == ORIENTATION_UP;

    data->userOrientation = ORIENTATION_UP;

    switch(data->hardwareGeometry)
    {
	case GEOMETRY_PORTRAIT:  WINCE_PortraitTransform(data, width, height); break;
	case GEOMETRY_LANDSCAPE: WINCE_LandscapeTransform(data, width, height); break;
	case GEOMETRY_SQUARE:    WINCE_SquareTransform(data, width, height); break;
	default: break;
    }

    // debug
    if(data->debug)
    {
	printf("\n");
	printf("user video width:          %d\n", width);
	printf("user video height:         %d\n", height);
	FrameBufferDumpInfo(&data->fb, "user");
    }
}

void WINCE_DumpVideoInfo(WINCE_RenderData* data)
{
    // get oem info
    WCHAR oemInfo[48];
    SDL_memset(oemInfo, 0, sizeof(oemInfo));
    SystemParametersInfo(SPI_GETOEMINFO, sizeof(oemInfo) - sizeof(WCHAR), oemInfo, 0);

    printf("hardware oem: ");
    wprintf(oemInfo);
    printf("\n");

    printf("video driver mode:             %s\n", (data->flags & FB_RAW_MODE ? RAW_RENDER_NAME : GAPI_RENDER_NAME));
    printf("GetSystemMetrics(SM_CXSCREEN): %d\n", GetSystemMetrics(SM_CXSCREEN));
    printf("GetSystemMetrics(SM_CYSCREEN): %d\n", GetSystemMetrics(SM_CYSCREEN));
    printf("scale coord:                   %f\n", data->scale);

    FrameBufferDumpInfo(&data->fb, "hardware");

    printf("display format:                %p\n", data->format);
    printf("display bits per pixel:        %d\n", SDL_BITSPERPIXEL(data->format));
    printf("display bytes per pixel:       %d\n", SDL_BYTESPERPIXEL(data->format));
    printf("display memory:                %p\n", data->pixels);
    printf("system orientation:            %d, %s\n", data->systemOrientation, GetOrientationName(data->systemOrientation));
    printf("hardware geometry:             %d\n", data->hardwareGeometry);
}

void WINCE_ShowWindow(_THIS, SDL_Window* window, int visible)
{
    SDL_WindowData* windowdata = (SDL_WindowData*) window->driverdata;
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    SDL_Renderer* renderer = (SDL_Renderer*) window->renderer;

    if(visible)
    {
	if(window->flags & SDL_WINDOW_FULLSCREEN)
	{
	    if(videodata->SHFullScreen)
		videodata->SHFullScreen(windowdata->hwnd, SHFS_HIDETASKBAR | SHFS_HIDESTARTICON | SHFS_HIDESIPBUTTON);
	    ShowWindow(FindWindow(TEXT("HHTaskBar"), NULL), SW_HIDE);
	}

	ShowWindow(windowdata->hwnd, SW_SHOW);
        SetForegroundWindow(windowdata->hwnd);

	if(renderer &&
	    (videodata->render == RENDER_GAPI || videodata->render == RENDER_RAW))
	{
	    WINCE_RenderData* renderdata = (WINCE_RenderData*) renderer->driverdata;
	    renderdata->flags &= ~FB_SUSPENDED;
	    if(renderdata->gapi) renderdata->gapi->GXResume();
	}
    }
    else
    {
	if(renderer &&
	    (videodata->render == RENDER_GAPI || videodata->render == RENDER_RAW))
	{
	    WINCE_RenderData* renderdata = (WINCE_RenderData*) renderer->driverdata;
	    if(renderdata->gapi) renderdata->gapi->GXSuspend();
	    renderdata->flags |= FB_SUSPENDED;
	}

	ShowWindow(windowdata->hwnd, SW_HIDE);

	if(window->flags & SDL_WINDOW_FULLSCREEN)
	{
	    if(videodata->SHFullScreen)
		videodata->SHFullScreen(windowdata->hwnd, SHFS_SHOWTASKBAR | SHFS_SHOWSTARTICON | SHFS_SHOWSIPBUTTON);
	    ShowWindow(FindWindow(TEXT("HHTaskBar"), NULL), SW_SHOW);
	}
    }
}


void WINCE_PointerCoordinateTransform(SDL_Window* window, POINT* pt)
{
    WINCE_RenderData* data = (WINCE_RenderData*) window->renderer->driverdata;

    pt->x *= data->scale;
    pt->y *= data->scale;

    PointerRotate(pt, &data->fb, data->userOrientation);
}

void WINCE_PortraitTransform(WINCE_RenderData* data, int width, int height)
{
    if(data->systemOrientation != ORIENTATION_UP)
	FrameBufferRotate(&data->fb, data->systemOrientation);

    if(data->fb.width != width || data->fb.height != height)
    switch(data->systemOrientation)
    {
	case ORIENTATION_UP:
	case ORIENTATION_LEFT: data->userOrientation = ORIENTATION_RIGHT; break;
	case ORIENTATION_RIGHT:
	case ORIENTATION_DOWN: data->userOrientation = ORIENTATION_LEFT; break;
	default: break;
    }

    if(data->userOrientation != ORIENTATION_UP)
	FrameBufferRotate(&data->fb, data->userOrientation);
}

void WINCE_LandscapeTransform(WINCE_RenderData* data, int width, int height)
{
    switch(data->systemOrientation)
    {
	case ORIENTATION_UP:  FrameBufferRotate(&data->fb, ORIENTATION_LEFT); break;
	case ORIENTATION_LEFT:FrameBufferRotate(&data->fb, ORIENTATION_DOWN); break;
	case ORIENTATION_DOWN:FrameBufferRotate(&data->fb, ORIENTATION_RIGHT); break;
	default: break;
    }

    if(data->fb.width != width || data->fb.height != height)
    switch(data->systemOrientation)
    {
	case ORIENTATION_UP:
	case ORIENTATION_LEFT: data->userOrientation = ORIENTATION_RIGHT; break;
	case ORIENTATION_RIGHT:
	case ORIENTATION_DOWN: data->userOrientation = ORIENTATION_LEFT; break;
	default: break;
    }

    if(data->userOrientation != ORIENTATION_UP)
	FrameBufferRotate(&data->fb, data->userOrientation);
}

void WINCE_SquareTransform(WINCE_RenderData* data, int width, int height)
{
    WINCE_PortraitTransform(data, width, height);
}

int WINCE_FixedGeometry(FrameBufferInfo* fb, int bpp, int debug)
{
    // check square
    if(GetSystemMetrics(SM_CXSCREEN) == GetSystemMetrics(SM_CYSCREEN) &&
	fb->width != fb->height)
    {
	if(fb->width < fb->height)
	    fb->height = fb->width;
	else
	if(fb->height < fb->width)
	    fb->width = fb->height;
    }

    // check width
    if(__abs(fb->xpitch) == bpp &&
	fb->width  != __abs(fb->ypitch) / bpp)
    {
	if(fb->height == __abs(fb->ypitch) / bpp)
    	{
	    __swap(&fb->width, &fb->height);

	    if(debug)
		printf("WINCE_FixedGeometry: width: %d, height: %d\n", fb->width, fb->height);
	}
	else
	    return -1;
    }
    else
    // check height
    if(__abs(fb->ypitch) == bpp &&
	fb->height != __abs(fb->xpitch) / bpp)
    {
	if(fb->width  == __abs(fb->xpitch) / bpp)
    	{
	    __swap(&fb->width, &fb->height);

	    if(debug)
		printf("WINCE_FixedGeometry: width: %d, height: %d\n", fb->width, fb->height);
	}
	else
	    return -1;
    }

    return 0;
}

void WINCE_UpdateYUVTextureData(SDL_Texture* texture)
{
    WINCE_TextureData* texturedata = (WINCE_TextureData*) texture->driverdata;
    SDL_Rect rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = texture->w;
    rect.h = texture->h;
    SDL_SW_CopyYUVToRGB(texturedata->yuv, &rect, texturedata->format, texture->w, texture->h, texturedata->pixels, texturedata->pitch);
}

int GAPI_Init(WINCE_RenderData* data, HWND hwnd)
{
    if(NULL == data->gapi)
    {
	const char* preferably = SDL_getenv("SDL_VIDEO_RENDERER");
	if(preferably && 0 != SDL_strcasecmp(preferably, GAPI_RENDER_NAME)) return 0;

        data->gapi = (GapiInfo *) SDL_calloc(1, sizeof(GapiInfo));
        if(NULL == data->gapi)
        {
            SDL_OutOfMemory();
            return 0;
        }

	data->gapi->hGapiLib = LoadLibrary(TEXT("\\Windows\\gx.dll"));
	if(0 == data->gapi->hGapiLib)
	{
	    data->gapi->hGapiLib = LoadLibrary(TEXT("gx.dll"));
	    if(0 == data->gapi->hGapiLib) return 0;
	}

	// load gapi library
#define LINK(type,name,import) name=(PFN##type)GetProcAddress(data->gapi->hGapiLib,TEXT(import))
	LINK(GXOpenDisplay,         data->gapi->GXOpenDisplay,         "?GXOpenDisplay@@YAHPAUHWND__@@K@Z");
	LINK(GXCloseDisplay,        data->gapi->GXCloseDisplay,        "?GXCloseDisplay@@YAHXZ");
	LINK(GXBeginDraw,           data->gapi->GXBeginDraw,           "?GXBeginDraw@@YAPAXXZ");
	LINK(GXEndDraw,             data->gapi->GXEndDraw,             "?GXEndDraw@@YAHXZ");
	LINK(GXGetDisplayProperties,data->gapi->GXGetDisplayProperties,"?GXGetDisplayProperties@@YA?AUGXDisplayProperties@@XZ");
	LINK(GXSuspend,             data->gapi->GXSuspend,             "?GXSuspend@@YAHXZ");
	LINK(GXResume,              data->gapi->GXResume,              "?GXResume@@YAHXZ");
#undef LINK

	int enable = data->gapi->GXGetDisplayProperties && data->gapi->GXCloseDisplay && data->gapi->GXOpenDisplay &&
	    data->gapi->GXBeginDraw && data->gapi->GXEndDraw && data->gapi->GXSuspend && data->gapi->GXResume;

	if(!enable)
	{
	    SDL_SetError("GAPI_Init: error gx.dll: internal error");
	    GAPI_Quit(data);
	    return 0;
	}

	if(0 == data->gapi->GXOpenDisplay(hwnd, GX_FULLSCREEN))
	{
	    SDL_SetError("GAPI_Init: couldn't initialize GAPI");
	    GAPI_Quit(data);
	    return 0;
	}

	struct GXDisplayProperties gxProperties = data->gapi->GXGetDisplayProperties();

	// fill FrameBufferInfo
	data->fb.xpitch = gxProperties.cbxPitch;
	data->fb.ypitch = gxProperties.cbyPitch;
	data->fb.width  = gxProperties.cxWidth;
	data->fb.height = gxProperties.cyHeight;
	data->fb.offset = 0;

        if((gxProperties.ffFormat & kfDirect565) || 16 == gxProperties.cBPP)
	    data->format = SDL_PIXELFORMAT_RGB565;
	else
	if((gxProperties.ffFormat & kfDirect555) || 15 == gxProperties.cBPP)
	    data->format = SDL_PIXELFORMAT_RGB555;
	else
	    data->format = 0;

	// get pixels
	GXDeviceInfo gxInfo = { 0 };
	HDC hdc = GetDC(NULL);

	gxInfo.Version = 100;
	int result = ExtEscape(hdc, GETGXINFO, 0, NULL, sizeof(gxInfo), (char *) &gxInfo);
	ReleaseDC(NULL, hdc);

	if(result > 0)
	{
	    // more debug
	    if(data->debug)
	    {
		printf("GXDeviceInfo.pvFrameBuffer:    %p\n", gxInfo.pvFrameBuffer);
    		printf("GXDeviceInfo.cxWidth:          %d\n", gxInfo.cxWidth);
    		printf("GXDeviceInfo.cyHeight:         %d\n", gxInfo.cyHeight);
    		printf("GXDeviceInfo.cbStride:         %d\n", gxInfo.cbStride);
    		printf("GXDeviceInfo.cBPP:             %d\n", gxInfo.cBPP);
    		printf("GXDeviceInfo.ffFormat:        0x%x\n", gxInfo.ffFormat);

    		printf("GXDeviceInfo.unk:\n");
		int ii; for(ii = 0; ii <  sizeof(gxInfo.unknown); ++ii)
		printf("0x%02hhX,", gxInfo.unknown[ii]);
		printf("\n");
	    }

    	    if(gxInfo.ffFormat && gxInfo.ffFormat != gxProperties.ffFormat)
	    {
		if((gxInfo.ffFormat & kfDirect565) || 16 == gxInfo.cBPP)
		    data->format = SDL_PIXELFORMAT_RGB565;
		else
		if((gxInfo.ffFormat & kfDirect555) || 15 == gxInfo.cBPP)
		    data->format = SDL_PIXELFORMAT_RGB555;
	    }

    	    data->pixels = gxInfo.pvFrameBuffer;
        }
	else
	{
	    data->flags |= FB_SKIP_OFFSET;
	    data->pixels = data->gapi->GXBeginDraw();
	    data->gapi->GXEndDraw();

	    if(data->debug)
	    {
		printf("GAPI_Init\n");
		printf("use GXBeginDraw:               %p\n", data->pixels);
		printf("use skip offset\n");
	    }
	}

	if(0 == data->format ||
	    0 > WINCE_FixedGeometry(&data->fb, SDL_BYTESPERPIXEL(data->format), data->debug))
	{
	    SDL_SetError("GAPI_Init: unknown hardware");
	    GAPI_Quit(data);
	    return 0;
	}
    }

    return data->gapi && data->pixels ? 1 : 0;
}

void GAPI_Quit(WINCE_RenderData* data)
{
    if(data->gapi)
    {
	if(data->gapi->GXCloseDisplay) data->gapi->GXCloseDisplay(); 
	if(data->gapi->hGapiLib)  FreeLibrary(data->gapi->hGapiLib);

	SDL_free(data->gapi);
        data->gapi = NULL;
    }
}

int RAW_Init(WINCE_RenderData* data)
{
    const char* preferably = SDL_getenv("SDL_VIDEO_RENDERER");
    if(preferably && 0 != SDL_strcasecmp(preferably, RAW_RENDER_NAME)) return 0;

    RawFrameBufferInfo rfbi = { 0 };
    HDC hdc = GetDC(NULL);
    int result = ExtEscape(hdc, GETRAWFRAMEBUFFER, 0, NULL, sizeof(RawFrameBufferInfo), (char *) &rfbi);
    ReleaseDC(NULL, hdc);

    //disable
    if(result == 0 || rfbi.pFramePointer == 0 ||
    	rfbi.cxPixels == 0 || rfbi.cyPixels == 0 ||
	rfbi.cxStride == 0 || rfbi.cyStride == 0) return 0;

    data->flags     = FB_RAW_MODE;

    // fill FrameBufferInfo
    SDL_memset(&data->fb, 0, sizeof(FrameBufferInfo));

    data->fb.xpitch = rfbi.cxStride;
    data->fb.ypitch = rfbi.cyStride;
    data->fb.width  = rfbi.cxPixels;
    data->fb.height = rfbi.cyPixels;
    data->fb.offset = 0;

    if((FORMAT_565 & rfbi.wFormat) || 16 == rfbi.wBPP)
	data->format = SDL_PIXELFORMAT_RGB565;
    else
    if((FORMAT_555 & rfbi.wFormat) || 15 == rfbi.wBPP)
	data->format = SDL_PIXELFORMAT_RGB555;
    else
	data->format = 0;

    if(0 == data->format ||
	0 > WINCE_FixedGeometry(&data->fb, SDL_BYTESPERPIXEL(data->format), data->debug))
    {
	SDL_SetError("RAW_Init: unknown hardware");
	RAW_Quit(data);
	return 0;
    }

    data->pixels = rfbi.pFramePointer;

    return data->pixels ? 1 : 0;
}

void RAW_Quit(WINCE_RenderData* data)
{
}

void FrameBufferInitialize(FrameBufferInfo* fb)
{
    int orientation = GetFrameBufferOrientation(fb);

    // set correct start offset
    switch(orientation)
    {
	case ORIENTATION_UP:
	    fb->offset = 0;
	    break;

	case ORIENTATION_LEFT:
	    fb->offset = __abs(fb->ypitch * (fb->height - 1));
	    break;

	case ORIENTATION_RIGHT:
	    fb->offset = __abs(fb->xpitch * (fb->width - 1));
	    break;

	case ORIENTATION_DOWN:
	    fb->offset = __abs(fb->xpitch * (fb->width - 1) +
				fb->ypitch * (fb->height - 1));
	    break;

	default: break;
    }

    //if(orientation != ORIENTATION_UP)
    switch(orientation)
    {
	case ORIENTATION_LEFT: FrameBufferRotate(fb, ORIENTATION_RIGHT); break;
	case ORIENTATION_RIGHT:FrameBufferRotate(fb, ORIENTATION_LEFT); break;
	case ORIENTATION_DOWN: FrameBufferRotate(fb, ORIENTATION_DOWN); break;

	default: break;
    }
}

int GetFrameBufferOrientation(const FrameBufferInfo* src)
{
    if(src->xpitch > 0 && src->ypitch > 0)
	return ORIENTATION_UP;
    else
    if(src->xpitch > 0 && src->ypitch < 0)
	return ORIENTATION_LEFT;
    else
    if(src->xpitch < 0 && src->ypitch > 0)
	return ORIENTATION_RIGHT;
    else
    if(src->xpitch < 0 && src->ypitch < 0)
	return ORIENTATION_DOWN;

    return ORIENTATION_UNKNOWN;
}

void FrameBufferRotate(FrameBufferInfo* dst, int orientation)
{
    FrameBufferInfo src;
    // copy dst -> src
    SDL_memcpy(&src, dst, sizeof(FrameBufferInfo));

    switch(orientation)
    {
        case ORIENTATION_LEFT:
	    dst->width  = src.height;
	    dst->height = src.width;
	    dst->xpitch = src.ypitch;
	    dst->ypitch = -src.xpitch;
	    dst->offset = src.offset + src.xpitch * (src.width - 1);
	    break;

        case ORIENTATION_RIGHT:
	    dst->width  = src.height;
	    dst->height = src.width;
	    dst->xpitch = -src.ypitch;
	    dst->ypitch = src.xpitch;
	    dst->offset = src.offset + src.ypitch * (src.height - 1);
	    break;

        case ORIENTATION_DOWN:
	    FrameBufferRotate(dst, ORIENTATION_LEFT);
	    FrameBufferRotate(dst, ORIENTATION_LEFT);
	    break;

        default:
	    break;
    }
}

void PointerRotate(POINT* pt, const FrameBufferInfo* fb, int orientation)
{
    switch(orientation)
    {
	case ORIENTATION_UP:
	    break;

	case ORIENTATION_LEFT:
	{
	    int temp = pt->y;
	    pt->y = fb->height - pt->x;
	    pt->x = temp;
	}
	    break;

	case ORIENTATION_RIGHT:
	{
	    int temp = pt->x;
	    pt->x = fb->width - pt->y;
	    pt->y = temp;
	}
	    break;

	case ORIENTATION_DOWN:
	    pt->x = fb->width  - pt->x;
	    pt->y = fb->height - pt->y;
	    break;

	default: break;
    }
}

const char* GetOrientationName(int orientation)
{
    switch(orientation)
    {
        case ORIENTATION_UP:        return "UP";
        case ORIENTATION_DOWN:      return "DOWN";
        case ORIENTATION_LEFT:      return "LEFT";
        case ORIENTATION_RIGHT:     return "RIGHT";
        default: break;
    }

    return "UNKNOWN";
}

int WINCE_GetDMOrientation(void)
{
    DEVMODE sDevMode = {0};
    sDevMode.dmSize = sizeof(DEVMODE);
    sDevMode.dmFields = DM_DISPLAYORIENTATION;

    // DMDO_0, DMDO_90, DMDO_180, DMDO_270
    if(DISP_CHANGE_BADMODE != ChangeDisplaySettingsEx(NULL, &sDevMode, 0, CDS_TEST, NULL))
	switch(sDevMode.dmDisplayOrientation)
	{
	    case DMDO_0:	return DMDO_0;
	    case DMDO_90:	return DMDO_90;
	    case DMDO_180:	return DMDO_180;
	    case DMDO_270:	return DMDO_270;
	    default: break;
	}

    SDL_SetError("WINCE_GetDMOrientation: ChangeDisplaySettingsEx return BADMODE");
    return -1;
}

int WINCE_SetDMOrientation(int orientation)
{
    DEVMODE sDevMode = {0};
    sDevMode.dmSize = sizeof(DEVMODE);
    sDevMode.dmFields = DM_DISPLAYORIENTATION;

    switch(orientation)
    {
	case DMDO_0:	sDevMode.dmDisplayOrientation = DMDO_0;   break;
	case DMDO_90:	sDevMode.dmDisplayOrientation = DMDO_90;  break;
	case DMDO_180:	sDevMode.dmDisplayOrientation = DMDO_180; break;
	case DMDO_270:	sDevMode.dmDisplayOrientation = DMDO_270; break;
	default: return 0;
    }

    if(DISP_CHANGE_BADMODE != ChangeDisplaySettingsEx(NULL, &sDevMode, 0, CDS_RESET, NULL))
	return 1;

    SDL_SetError("WINCE_SetDMOrientation: ChangeDisplaySettingsEx return BADMODE");
    return -1;
}

void FrameBufferDumpInfo(const FrameBufferInfo* fb, const char* name)
{
    printf("%s fb.width:       %d\n", name, fb->width);
    printf("%s fb.height:      %d\n", name, fb->height);
    printf("%s fb.xpitch:      %d\n", name, fb->xpitch);
    printf("%s fb.ypitch:      %d\n", name, fb->ypitch);
    printf("%s fb.offset:      %d\n", name, fb->offset);

    int orientation = GetFrameBufferOrientation(fb);
    printf("%s fb.orientation: %d, %s\n", name, orientation, GetOrientationName(orientation));
}

void UpdateLine16to16(const FrameBufferInfo* fb, const Uint16* src, Uint16* dst, Uint16 width)
{
    if(2 == fb->xpitch)
    {
	switch(width)
	{
	    case 1:
		*dst = *src;
		break;

	    case 2:
		*((Uint32*) dst) = *((Uint32*) src);
		break;

	    default:
		SDL_memcpy(dst, src, width * 2);
		break;
	}
    }
    else
    if(-2 == fb->xpitch)
    {
	while(width--)
	    *dst-- = *src++;
    }
    else
    {
	while(width--)
	{
	    *dst = *src++;
	    dst += fb->xpitch / 2;
	}
    }
}

#endif // SDL_VIDEO_RENDER_GAPI
