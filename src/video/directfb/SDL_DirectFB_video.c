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
	slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* DirectFB video driver implementation.
*/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <directfb.h>

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"
#include "SDL_DirectFB_video.h"
#include "SDL_DirectFB_events.h"


/* Initialization/Query functions */
static int DirectFB_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **DirectFB_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *DirectFB_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int DirectFB_SetColors(_THIS, int firstcolor, int ncolors,
			 SDL_Color *colors);
static void DirectFB_VideoQuit(_THIS);

/* Hardware surface functions */
static int DirectFB_AllocHWSurface(_THIS, SDL_Surface *surface);
static int DirectFB_FillHWRect(_THIS, SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color);
static int DirectFB_LockHWSurface(_THIS, SDL_Surface *surface);
static void DirectFB_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void DirectFB_FreeHWSurface(_THIS, SDL_Surface *surface);
static int DirectFB_CheckHWBlit(_THIS, SDL_Surface *src, SDL_Surface *dst);
static int DirectFB_HWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect,
                                SDL_Surface *dst, SDL_Rect *dstrect);
static int DirectFB_SetHWColorKey(_THIS, SDL_Surface *surface, Uint32 key);
static int DirectFB_SetHWAlpha(_THIS, SDL_Surface *surface, Uint8 alpha);
static int DirectFB_FlipHWSurface(_THIS, SDL_Surface *surface);

/* Various screen update functions available */
static void DirectFB_DirectUpdate(_THIS, int numrects, SDL_Rect *rects);
static void DirectFB_WindowedUpdate(_THIS, int numrects, SDL_Rect *rects);

/* This is the rect EnumModes2 uses */
struct DirectFBEnumRect {
	SDL_Rect r;
	struct DirectFBEnumRect* next;
};

static struct DirectFBEnumRect *enumlists[NUM_MODELISTS];


/* DirectFB driver bootstrap functions */

static int DirectFB_Available(void)
{
  return 1;
}

static void DirectFB_DeleteDevice(SDL_VideoDevice *device)
{
  free(device->hidden);
  free(device);
}

static SDL_VideoDevice *DirectFB_CreateDevice(int devindex)
{
  SDL_VideoDevice *device;

  /* Initialize all variables that we clean on shutdown */
  device = (SDL_VideoDevice *)malloc(sizeof(SDL_VideoDevice));
  if (device)
    {
      memset (device, 0, (sizeof *device));
      device->hidden = (struct SDL_PrivateVideoData *) malloc (sizeof (*device->hidden));
    }
  if (device == NULL  ||  device->hidden == NULL)
    {
      SDL_OutOfMemory();
      if (device)
        {
          free (device);
        }
      return(0);
    }
  memset (device->hidden, 0, sizeof (*device->hidden));

  /* Set the function pointers */
  device->VideoInit = DirectFB_VideoInit;
  device->ListModes = DirectFB_ListModes;
  device->SetVideoMode = DirectFB_SetVideoMode;
  device->SetColors = DirectFB_SetColors;
  device->UpdateRects = NULL;
  device->VideoQuit = DirectFB_VideoQuit;
  device->AllocHWSurface = DirectFB_AllocHWSurface;
  device->CheckHWBlit = DirectFB_CheckHWBlit;
  device->FillHWRect = DirectFB_FillHWRect;
  device->SetHWColorKey = DirectFB_SetHWColorKey;
  device->SetHWAlpha = DirectFB_SetHWAlpha;
  device->LockHWSurface = DirectFB_LockHWSurface;
  device->UnlockHWSurface = DirectFB_UnlockHWSurface;
  device->FlipHWSurface = DirectFB_FlipHWSurface;
  device->FreeHWSurface = DirectFB_FreeHWSurface;
  device->SetCaption = NULL;
  device->SetIcon = NULL;
  device->IconifyWindow = NULL;
  device->GrabInput = NULL;
  device->GetWMInfo = NULL;
  device->InitOSKeymap = DirectFB_InitOSKeymap;
  device->PumpEvents = DirectFB_PumpEvents;

  device->free = DirectFB_DeleteDevice;

  return device;
}

VideoBootStrap DirectFB_bootstrap = {
  "directfb", "DirectFB",
  DirectFB_Available, DirectFB_CreateDevice
};

static DFBEnumerationResult EnumModesCallback (unsigned int  width,
                                               unsigned int  height,
                                               unsigned int  bpp,
                                               void         *data)
{
  SDL_VideoDevice *this = (SDL_VideoDevice *)data;
  struct DirectFBEnumRect *enumrect;

  switch (bpp)
    {
    case 8:
    case 15:
    case 16:
    case 24:
    case 32:
      bpp /= 8; --bpp;
      ++HIDDEN->SDL_nummodes[bpp];
      enumrect = (struct DirectFBEnumRect*)malloc(sizeof(struct DirectFBEnumRect));
      if ( !enumrect )
        {
          SDL_OutOfMemory();
          return DFENUM_CANCEL;
        }
      enumrect->r.x = 0;
      enumrect->r.y = 0;
      enumrect->r.w = width;
      enumrect->r.h = height;
      enumrect->next = enumlists[bpp];
      enumlists[bpp] = enumrect;
      break;
    }

  return DFENUM_OK;
}

struct private_hwdata {
  IDirectFBSurface *surface;
};

void SetDirectFBerror (const char *function, DFBResult code)
{
  const char *error = DirectFBErrorString (code);

  if (error)
    SDL_SetError("%s: %s", function, error);
  else
    SDL_SetError("Unknown error code from %s", function);
}

static DFBSurfacePixelFormat SDLToDFBPixelFormat (SDL_PixelFormat *format)
{
  if (format->Rmask && format->Gmask && format->Bmask)
    {
      switch (format->BitsPerPixel)
        {
        case 16:
          if (format->Rmask == 0xF800 &&
              format->Gmask == 0x07E0 &&
              format->Bmask == 0x001F)
            return DSPF_RGB16;
          /* fall through */
          
        case 15:
          if (format->Rmask == 0x7C00 &&
              format->Gmask == 0x03E0 &&
              format->Bmask == 0x001F)
            return DSPF_RGB15;
          break;
          
        case 24:
          if (format->Rmask == 0xFF0000 &&
              format->Gmask == 0x00FF00 &&
              format->Bmask == 0x0000FF)
            return DSPF_RGB24;
          break;
          
        case 32:
          if (format->Rmask == 0xFF0000 &&
              format->Gmask == 0x00FF00 &&
              format->Bmask == 0x0000FF)
            {
              if (format->Amask == 0xFF000000)
                return DSPF_ARGB;
              else
                return DSPF_RGB32;
            }
          break;
        }
    }
  else
    {
      switch (format->BitsPerPixel)
	{
	case 15:
	  return DSPF_RGB15;
	case 16:
	  return DSPF_RGB16;
	case 24:
	  return DSPF_RGB24;
	case 32:
	  return DSPF_RGB32;
	}
    }

  return DSPF_UNKNOWN;
}

static int DFBToSDLPixelFormat (DFBSurfacePixelFormat pixelformat, SDL_PixelFormat *format)
{
  format->BitsPerPixel = 0;
  format->Amask = format->Rmask = format->Gmask = format->Bmask = 0;

  switch (pixelformat)
    {
    case DSPF_A8:
      format->Amask = 0x000000FF;
      break;
    case DSPF_RGB15:
      format->Rmask = 0x00007C00;
      format->Gmask = 0x000003E0;
      format->Bmask = 0x0000001F;
      break;
    case DSPF_RGB16:
      format->Rmask = 0x0000F800;
      format->Gmask = 0x000007E0;
      format->Bmask = 0x0000001F;
      break;
    case DSPF_ARGB:
      format->Amask = 0xFF000000;
      /* fall through */
    case DSPF_RGB24:
    case DSPF_RGB32:
      format->Rmask = 0x00FF0000;
      format->Gmask = 0x0000FF00;
      format->Bmask = 0x000000FF;
      break;
    default:
      return -1;
    }

  format->BitsPerPixel = BITS_PER_PIXEL(pixelformat);

  return 0;
}


int DirectFB_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
  int                    i, j;
  DFBResult              ret;
  IDirectFB             *dfb;
  DFBCardCapabilities    caps;
  IDirectFBDisplayLayer *layer;
  DFBDisplayLayerConfig  dlc;
  IDirectFBInputBuffer  *inputbuffer;


  ret = DirectFBInit (NULL, NULL);
  if (ret)
    {
      SetDirectFBerror ("DirectFBInit", ret);
      return -1;
    }

  ret = DirectFBCreate (&dfb);
  if (ret)
    {
      SetDirectFBerror ("DirectFBCreate", ret);
      return -1;
    }

  ret = dfb->GetDisplayLayer (dfb, DLID_PRIMARY, &layer);
  if (ret)
    {
      SetDirectFBerror ("dfb->GetDisplayLayer", ret);
      dfb->Release (dfb);
      return -1;
    }

  ret = dfb->CreateInputBuffer (dfb, DICAPS_BUTTONS | DICAPS_AXIS | DICAPS_KEYS,
                                &inputbuffer);
  if (ret)
    {
      SetDirectFBerror ("dfb->CreateInputBuffer", ret);
      layer->Release (layer);
      dfb->Release (dfb);
      return -1;
    }
  
  layer->EnableCursor (layer, 1);

  /* Query layer configuration to determine the current mode and pixelformat */
  layer->GetConfiguration (layer, &dlc);

  if (DFBToSDLPixelFormat (dlc.pixelformat, vformat))
    {
      SDL_SetError ("Unsupported pixelformat");
      layer->Release (layer);
      dfb->Release (dfb);
      return -1;
    }

  /* Enumerate the available fullscreen modes */
  for ( i=0; i<NUM_MODELISTS; ++i )
    enumlists[i] = NULL;

  ret = dfb->EnumVideoModes (dfb, EnumModesCallback, this);
  if (ret)
    {
      SetDirectFBerror ("dfb->EnumVideoModes", ret);
      layer->Release (layer);
      dfb->Release (dfb);
      return(-1);
    }
  for ( i=0; i<NUM_MODELISTS; ++i )
    {
      struct DirectFBEnumRect *rect;
      HIDDEN->SDL_modelist[i] = (SDL_Rect **) malloc
        ((HIDDEN->SDL_nummodes[i]+1)*sizeof(SDL_Rect *));
      if ( HIDDEN->SDL_modelist[i] == NULL )
        {
          SDL_OutOfMemory();
          return(-1);
        }
      for ( j = 0, rect = enumlists[i]; rect; ++j, rect = rect->next )
        {
          HIDDEN->SDL_modelist[i][j]=(SDL_Rect *)rect;
        }
      HIDDEN->SDL_modelist[i][j] = NULL;
    }

  /* Query card capabilities to get the video memory size */
  dfb->GetCardCapabilities (dfb, &caps);

  this->info.wm_available = 1;
  this->info.hw_available = 1;
  this->info.blit_hw      = 1;
  this->info.blit_hw_CC   = 1;
  this->info.blit_hw_A    = 1;
  this->info.blit_fill    = 1;
  this->info.video_mem    = caps.video_memory / 1024;

  HIDDEN->initialized = 1;
  HIDDEN->dfb         = dfb;
  HIDDEN->layer       = layer;
  HIDDEN->inputbuffer = inputbuffer;

  return 0;
}

static SDL_Rect **DirectFB_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
  if (flags & SDL_FULLSCREEN)
    return HIDDEN->SDL_modelist[((format->BitsPerPixel + 7) / 8) - 1];
  else
    if (SDLToDFBPixelFormat (format) != DSPF_UNKNOWN)
      return (SDL_Rect**) -1;

  return NULL;
}

SDL_Surface *DirectFB_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags)
{
  DFBResult             ret;
  DFBSurfaceDescription dsc;
  DFBSurfacePixelFormat pixelformat;

  fprintf (stderr, "SDL DirectFB_SetVideoMode: %dx%d@%d, flags: 0x%08x\n",
           width, height, bpp, flags);

  flags |= SDL_FULLSCREEN;

  /* Release previous primary surface */
  if (current->hwdata && current->hwdata->surface)
    {
      current->hwdata->surface->Release (current->hwdata->surface);
      current->hwdata->surface = NULL;
    }
  else if (!current->hwdata)
    {
      /* Allocate the hardware acceleration data */
      current->hwdata = (struct private_hwdata *) malloc (sizeof(*current->hwdata));
      if (!current->hwdata)
        {
          SDL_OutOfMemory();
          return NULL;
	}
      memset (current->hwdata, 0, sizeof(*current->hwdata));
    }

  /* Set cooperative level depending on flag SDL_FULLSCREEN */
  if (flags & SDL_FULLSCREEN)
    {
      ret = HIDDEN->dfb->SetCooperativeLevel (HIDDEN->dfb, DFSCL_FULLSCREEN);
      if (ret)
        {
          DirectFBError ("dfb->SetCooperativeLevel", ret);
          flags &= ~SDL_FULLSCREEN;
        }
    }
  else
    HIDDEN->dfb->SetCooperativeLevel (HIDDEN->dfb, DFSCL_NORMAL);

  /* Set video mode */
  ret = HIDDEN->dfb->SetVideoMode (HIDDEN->dfb, width, height, bpp);
  if (ret)
    {
      if (flags & SDL_FULLSCREEN)
        {
          flags &= ~SDL_FULLSCREEN;
          HIDDEN->dfb->SetCooperativeLevel (HIDDEN->dfb, DFSCL_NORMAL);
          ret = HIDDEN->dfb->SetVideoMode (HIDDEN->dfb, width, height, bpp);
        }

      if (ret)
        {
          SetDirectFBerror ("dfb->SetVideoMode", ret);
          return NULL;
        }
    }

  /* Create primary surface */
  dsc.flags = DSDESC_CAPS;
  dsc.caps  = DSCAPS_PRIMARY | ((flags & SDL_DOUBLEBUF) ? DSCAPS_FLIPPING : 0);

  ret = HIDDEN->dfb->CreateSurface (HIDDEN->dfb, &dsc, &current->hwdata->surface);
  if (ret && (flags & SDL_DOUBLEBUF))
    {
      /* Try without double buffering */
      dsc.caps &= ~DSCAPS_FLIPPING;
      ret = HIDDEN->dfb->CreateSurface (HIDDEN->dfb, &dsc, &current->hwdata->surface);
    }
  if (ret)
    {
      SetDirectFBerror ("dfb->CreateSurface", ret);
      current->hwdata->surface = NULL;
      return NULL;
    }

  current->w     = width;
  current->h     = height;
  current->flags = SDL_HWSURFACE | SDL_PREALLOC;

  if (flags & SDL_FULLSCREEN)
    {
      current->flags |= SDL_FULLSCREEN;
      this->UpdateRects = DirectFB_DirectUpdate;
    }
  else
    this->UpdateRects = DirectFB_WindowedUpdate;

  if (dsc.caps & DSCAPS_FLIPPING)
    current->flags |= SDL_DOUBLEBUF;

  current->hwdata->surface->GetPixelFormat (current->hwdata->surface, &pixelformat);
  DFBToSDLPixelFormat (pixelformat, current->format);

  return current;
}

static int DirectFB_AllocHWSurface(_THIS, SDL_Surface *surface)
{
  DFBResult             ret;
  DFBSurfaceDescription dsc;

  /*  fprintf(stderr, "SDL: DirectFB_AllocHWSurface (%dx%d@%d, flags: 0x%08x)\n",
      surface->w, surface->h, surface->format->BitsPerPixel, surface->flags);*/

  if (surface->w < 8 || surface->h < 8)
    return -1;

  /* fill surface description */
  dsc.flags  = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS;
  dsc.width  = surface->w;
  dsc.height = surface->h;
  dsc.caps   = surface->flags & SDL_DOUBLEBUF ? DSCAPS_FLIPPING : 0;

  /* find the right pixelformat */
  dsc.pixelformat = SDLToDFBPixelFormat (surface->format);
  if (dsc.pixelformat == DSPF_UNKNOWN)
    return -1;

  /* Allocate the hardware acceleration data */
  surface->hwdata = (struct private_hwdata *) malloc (sizeof(*surface->hwdata));
  if (surface->hwdata == NULL)
    {
      SDL_OutOfMemory();
      return -1;
    }

  /* Create the surface */
  ret = HIDDEN->dfb->CreateSurface (HIDDEN->dfb, &dsc, &surface->hwdata->surface);
  if (ret)
    {
      SetDirectFBerror ("dfb->CreateSurface", ret);
      free (surface->hwdata);
      surface->hwdata = NULL;
      return -1;
    }

  surface->flags |= SDL_HWSURFACE | SDL_PREALLOC;

  return 0;
}

static void DirectFB_FreeHWSurface(_THIS, SDL_Surface *surface)
{
  if (surface->hwdata && HIDDEN->initialized)
    {
      surface->hwdata->surface->Release (surface->hwdata->surface);
      free (surface->hwdata);
      surface->hwdata = NULL;
    }
}

static int DirectFB_CheckHWBlit(_THIS, SDL_Surface *src, SDL_Surface *dst)
{
  /*  fprintf(stderr, "SDL: DirectFB_CheckHWBlit (src->hwdata: %p, dst->hwdata: %p)\n",
      src->hwdata, dst->hwdata);*/

  if (!src->hwdata || !dst->hwdata)
    return 0;

  src->flags |= SDL_HWACCEL;
  src->map->hw_blit = DirectFB_HWAccelBlit;

  return 1;
}

static int DirectFB_HWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect,
                                SDL_Surface *dst, SDL_Rect *dstrect)
{
  DFBRectangle             sr, dr;
  IDirectFBSurface        *surface;
  DFBSurfaceBlittingFlags  flags = DSBLIT_NOFX;

  sr.x = srcrect->x;
  sr.y = srcrect->y;
  sr.w = srcrect->w;
  sr.h = srcrect->h;

  dr.x = dstrect->x;
  dr.y = dstrect->y;
  dr.w = dstrect->w;
  dr.h = dstrect->h;

  surface = dst->hwdata->surface;

  if (src->flags & SDL_SRCCOLORKEY)
    {
      flags |= DSBLIT_SRC_COLORKEY;
      surface->SetSrcColorKey (surface, src->format->colorkey);
    }

  if (src->flags & SDL_SRCALPHA)
    {
      flags |= DSBLIT_BLEND_COLORALPHA;
      surface->SetColor (surface, 0xff, 0xff, 0xff, src->format->alpha);
    }

  surface->SetBlittingFlags (surface, flags);

  if (sr.w == dr.w && sr.h == dr.h)
    surface->Blit (surface, src->hwdata->surface, &sr, dr.x, dr.y);
  else
    surface->StretchBlit (surface, src->hwdata->surface, &sr, &dr);

  return 0;
}

static int DirectFB_FillHWRect(_THIS, SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color)
{
  SDL_PixelFormat  *fmt     = dst->format;
  IDirectFBSurface *surface = dst->hwdata->surface;

  /* ugly */
  surface->SetColor (surface,
                     (color & fmt->Rmask) >> (fmt->Rshift - fmt->Rloss),
                     (color & fmt->Gmask) >> (fmt->Gshift - fmt->Gloss),
                     (color & fmt->Bmask) << (fmt->Bloss - fmt->Bshift), 0xFF);
  surface->FillRectangle (surface, dstrect->x, dstrect->y, dstrect->w, dstrect->h);

  return 0;
}

static int DirectFB_SetHWColorKey(_THIS, SDL_Surface *surface, Uint32 key)
{
  return 0;
}

static int DirectFB_SetHWAlpha(_THIS, SDL_Surface *surface, Uint8 alpha)
{
  return 0;
}

static int DirectFB_FlipHWSurface(_THIS, SDL_Surface *surface)
{
  return surface->hwdata->surface->Flip (surface->hwdata->surface, NULL, DSFLIP_WAITFORSYNC);
}

static int DirectFB_LockHWSurface(_THIS, SDL_Surface *surface)
{
  DFBResult  ret;
  void      *data;
  int        pitch;

  ret = surface->hwdata->surface->Lock (surface->hwdata->surface,
                                        DSLF_WRITE, &data, &pitch);
  if (ret)
    {
      SetDirectFBerror ("surface->Lock", ret);
      return -1;
    }

  surface->pixels = data;
  surface->pitch  = pitch;

  return 0;
}

static void DirectFB_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
  surface->hwdata->surface->Unlock (surface->hwdata->surface);
  surface->pixels = NULL;
}

static void DirectFB_DirectUpdate(_THIS, int numrects, SDL_Rect *rects)
{
}

static void DirectFB_WindowedUpdate(_THIS, int numrects, SDL_Rect *rects)
{
  IDirectFBSurface *surface = this->screen->hwdata->surface;
  DFBRegion region = { rects->x, rects->y,
                       rects->x + rects->w - 1,
                       rects->y + rects->h - 1 };

  while (--numrects)
    {
      int x2, y2;

      rects++;

      if (rects->x < region.x1)
        region.x1 = rects->x;

      if (rects->y < region.y1)
        region.y1 = rects->y;

      x2 = rects->x + rects->w - 1;
      y2 = rects->y + rects->h - 1;

      if (x2 > region.x2)
        region.x2 = x2;

      if (y2 > region.y2)
        region.y2 = y2;
    }

  surface->Flip (surface, &region, DSFLIP_WAITFORSYNC);
}

int DirectFB_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
  fprintf(stderr, "SDL: Unimplemented DirectFB_SetColors!\n");
  return 0;
}
	
void DirectFB_VideoQuit(_THIS)
{
  int i, j;

  HIDDEN->inputbuffer->Release (HIDDEN->inputbuffer);
  HIDDEN->layer->Release (HIDDEN->layer);
  HIDDEN->dfb->Release (HIDDEN->dfb);

  /* Free video mode lists */
  for ( i=0; i<NUM_MODELISTS; ++i )
    {
      if ( HIDDEN->SDL_modelist[i] != NULL )
        {
          for ( j=0; HIDDEN->SDL_modelist[i][j]; ++j )
            free(HIDDEN->SDL_modelist[i][j]);
          free(HIDDEN->SDL_modelist[i]);
          HIDDEN->SDL_modelist[i] = NULL;
        }
    }

  HIDDEN->initialized = 0;
}

void DirectFB_FinalQuit(void) 
{
}
