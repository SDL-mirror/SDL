/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

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

#include <stdlib.h>
#include <Ph.h>
#include <photon/Pg.h>

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_endian.h"
#include "SDL_video.h"
#include "SDL_pixels_c.h"
#include "SDL_ph_image_c.h"

/* Mask values for SDL_ReallocFormat() */
struct ColourMasks
{
    Uint32 red;
    Uint32 green;
    Uint32 blue;
    Uint32 alpha;
    Uint32 bpp;
};

static const struct ColourMasks *ph_GetColourMasks( int format )
{
    /* The alpha mask doesn't appear to be needed */
    static const struct ColourMasks phColorMasks[5] = {
        /*  8 bit      */  {0, 0, 0, 0, 8},
        /* 15 bit ARGB */  {0x7C00, 0x03E0, 0x001F, 0x8000, 16},
        /* 16 bit  RGB */  {0xF800, 0x07E0, 0x001F, 0x0000, 16},
        /* 24 bit  RGB */  {0xFF0000, 0x00FF00, 0x0000FF, 0x000000, 24},
        /* 32 bit ARGB */  {0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, 32},
    };

    switch( format )
    {
        case Pg_IMAGE_PALETTE_BYTE:
             return &phColorMasks[0];
             break;
        case Pg_IMAGE_DIRECT_1555:
        case Pg_IMAGE_DIRECT_555:
             return &phColorMasks[1];
             break;
        case Pg_IMAGE_DIRECT_565:
             return &phColorMasks[2];
             break;
        case Pg_IMAGE_DIRECT_888:
             return &phColorMasks[3];
             break;
        case Pg_IMAGE_DIRECT_8888:
             return &phColorMasks[4];
             break;
    }
    return NULL;
}

int ph_SetupImage(_THIS, SDL_Surface *screen)
{
    PgColor_t* palette=NULL;
    const struct ColourMasks* mask;
    int type=0;
    int bpp;
    
    bpp=screen->format->BitsPerPixel;

    /* Determine image type */
    switch(bpp)
    {
        case 8:{
            type = Pg_IMAGE_PALETTE_BYTE;
        }
        break;
        case 15:{
            type = Pg_IMAGE_DIRECT_555; 
        }
        break;
        case 16:{
            type = Pg_IMAGE_DIRECT_565; 
        }
        break;
        case 24:{
            type = Pg_IMAGE_DIRECT_888;
        }
        break;
        case 32:{
            type = Pg_IMAGE_DIRECT_8888;
        }
        break;
        default:{
            fprintf(stderr,"ph_SetupImage(): unsupported bpp=%d !\n", bpp);
            return -1;
        }
        break;
    }

    /* palette emulation code */
    if ((bpp==8) && (desktoppal==SDLPH_PAL_EMULATE))
    {
        /* creating image palette */
        palette=malloc(_Pg_MAX_PALETTE*sizeof(PgColor_t));
        PgGetPalette(palette);

        /* using shared memory for speed (set last param to 1) */
        if ((SDL_Image = PhCreateImage(NULL, screen->w, screen->h, type, palette, _Pg_MAX_PALETTE, 1)) == NULL)
        {
            fprintf(stderr,"ph_SetupImage(): PhCreateImage failed for bpp=8 !\n");
            return -1;
        }
    }
    else
    {
        /* using shared memory for speed (set last param to 1) */
        if ((SDL_Image = PhCreateImage(NULL, screen->w, screen->h, type, NULL, 0, 1)) == NULL)
        {
            fprintf(stderr,"ph_SetupImage: PhCreateImage failed !\n");
            return -1;
        }
    }

    screen->pixels = SDL_Image->image;
    screen->pitch = SDL_Image->bpl; /* Recalculated pitch, created by PhCreateImage */

    mask = ph_GetColourMasks(type);
    if (mask != NULL)
    {
        SDL_ReallocFormat(screen, mask->bpp, mask->red, mask->green, mask->blue, 0);
    }

    this->UpdateRects = ph_NormalUpdate;

    return 0;
}

int ph_SetupOCImage(_THIS, SDL_Surface *screen)
{
    const struct ColourMasks *mask;
    int type = 0;
    int bpp;

    screen->flags &= ~SDL_DOUBLEBUF;
    OCImage.flags = screen->flags;
    
    bpp=screen->format->BitsPerPixel;

    /* Determine image type */
    switch(bpp)
    {
        case 8: {
                    type = Pg_IMAGE_PALETTE_BYTE;
                }
                break;
        case 15:{
                    type = Pg_IMAGE_DIRECT_555; 
		}
		break;
        case 16:{
                    type = Pg_IMAGE_DIRECT_565; 
                }
                break;
        case 24:{
                    type = Pg_IMAGE_DIRECT_888;
                }
                break;
        case 32:{
                    type = Pg_IMAGE_DIRECT_8888;
                }
                break;
        default:{
                    fprintf(stderr,"ph_SetupOCImage(): unsupported bpp=%d !\n", bpp);
                    return -1;
                }
                break;
    }

    /* Currently only offscreen contexts with the same bit depth as the
     * display can be created. */
    OCImage.offscreen_context = PdCreateOffscreenContext(0, screen->w, screen->h, Pg_OSC_MEM_PAGE_ALIGN);

    if (OCImage.offscreen_context == NULL)
    {
        fprintf(stderr, "ph_SetupOCImage(): PdCreateOffscreenContext failed !\n");
        return -1;
    }

    /* If the bit depth of the context is different than was requested,
     * these values need to be updated accordingly.  SDL will
     * allocate a shadow surface if it needs to. */
    mask = ph_GetColourMasks(OCImage.offscreen_context->format);
    if (mask != NULL)
    {
        SDL_ReallocFormat(screen, mask->bpp, mask->red, mask->green, mask->blue, 0);

        if (mask->bpp > 8)
        {
            screen->flags &= ~SDL_HWPALETTE;
        }
    }

    screen->pitch = OCImage.offscreen_context->pitch; /* Recalculated pitch */

    OCImage.dc_ptr.ptr8 = (unsigned char *) PdGetOffscreenContextPtr(OCImage.offscreen_context);

    if (OCImage.dc_ptr.ptr8 == NULL)
    {
        fprintf(stderr, "ph_SetupOCImage(): PdGetOffscreenContextPtr failed !\n");
        return -1;
    }

    OCImage.FrameData0 = OCImage.dc_ptr.ptr8;
    OCImage.CurrentFrameData = OCImage.FrameData0;
    OCImage.current = 0;

    PhDCSetCurrent(OCImage.offscreen_context);

    screen->pixels = OCImage.CurrentFrameData;

    this->UpdateRects = ph_OCUpdate;

    return 0;
}

int ph_SetupOpenGLImage(_THIS, SDL_Surface* screen)
{
   this->UpdateRects = ph_OpenGLUpdate;
   
   return 0;
}

int ph_SetupFullScreenImage(_THIS, SDL_Surface* screen)
{
    const struct ColourMasks *mask;
    screen->flags &= ~SDL_DOUBLEBUF;
    OCImage.flags = screen->flags;

    OCImage.offscreen_context = PdCreateOffscreenContext(0, 0, 0, Pg_OSC_MAIN_DISPLAY);

    if (OCImage.offscreen_context == NULL)
    {
        fprintf(stderr, "ph_SetupFullScreenImage(): PdCreateOffscreenContext failed !\n");
        return -1;
    }

    /* If the bit depth of the context is different than was requested,
     * these values need to be updated accordingly.  SDL will
     * allocate a shadow surface if it needs to. */
    mask = ph_GetColourMasks(OCImage.offscreen_context->format);
    if (mask != NULL)
    {
        SDL_ReallocFormat(screen, mask->bpp, mask->red, mask->green, mask->blue, 0);

        if (mask->bpp > 8)
        {
            screen->flags &= ~SDL_HWPALETTE;
        }
    }

    screen->pitch = OCImage.offscreen_context->pitch; /* Recalculated pitch */

    OCImage.dc_ptr.ptr8 = (unsigned char *)PdGetOffscreenContextPtr(OCImage.offscreen_context);

    if (OCImage.dc_ptr.ptr8 == NULL)
    {
        fprintf(stderr, "ph_SetupOCImage(): PdGetOffscreenContextPtr failed !\n");
        return -1;
    }

    /* wait for hw */
    PgWaitHWIdle();

    OCImage.FrameData0 = OCImage.dc_ptr.ptr8;
    OCImage.CurrentFrameData = OCImage.FrameData0;
    OCImage.current = 0;

    PhDCSetCurrent(OCImage.offscreen_context);

    screen->pixels = OCImage.CurrentFrameData;

    this->UpdateRects = ph_OCUpdate;

    return 0;
}

void ph_DestroyImage(_THIS, SDL_Surface *screen)
{
    if (OCImage.offscreen_context != NULL)
    {
        PhDCRelease(OCImage.offscreen_context);
        OCImage.offscreen_context = NULL;
        OCImage.FrameData0 = NULL;
        OCImage.FrameData1 = NULL;
    }

    if (SDL_Image)
    {
        /* if palette allocated, free it */
        if (SDL_Image->palette)
        {
            free(SDL_Image->palette);
        }
        PgShmemDestroy(SDL_Image->image);
        free(SDL_Image);
    }

    /* Must be zeroed everytime */
    SDL_Image = NULL;

    if (screen)
    {
        screen->pixels = NULL;
    }
}

int ph_SetupUpdateFunction(_THIS, SDL_Surface *screen, Uint32 flags)
{
    ph_DestroyImage(this, screen);
    
    if ((flags & SDL_FULLSCREEN)==SDL_FULLSCREEN)
    {
        return ph_SetupFullScreenImage(this, screen);
    }
    if ((flags & SDL_HWSURFACE)==SDL_HWSURFACE)
    {
        return ph_SetupOCImage(this, screen);
    }
    if ((flags & SDL_OPENGL)==SDL_OPENGL)
    {
        return ph_SetupOpenGLImage(this, screen);
    } 

    return ph_SetupImage(this, screen);
}
int ph_AllocHWSurface(_THIS, SDL_Surface *surface)
{
    return(-1);
}

void ph_FreeHWSurface(_THIS, SDL_Surface *surface)
{
    return;
}

int ph_FlipHWSurface(_THIS, SDL_Surface *surface)
{
    return(0);
}

int ph_LockHWSurface(_THIS, SDL_Surface *surface)
{
    return(0);
}

void ph_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
    return;
}

void ph_OpenGLUpdate(_THIS, int numrects, SDL_Rect* rects)
{
   this->GL_SwapBuffers(this);
   
   return;
}

void ph_NormalUpdate(_THIS, int numrects, SDL_Rect *rects)
{
    PhPoint_t ph_pos;
    PhRect_t ph_rect;
    int i;

    for (i=0; i<numrects; ++i) 
    {
    	if (rects[i].w==0) /* Clipped? */
        { 
            continue;
        }

        ph_pos.x = rects[i].x;
        ph_pos.y = rects[i].y;
        ph_rect.ul.x = rects[i].x;
        ph_rect.ul.y = rects[i].y;
        ph_rect.lr.x = rects[i].x + rects[i].w;
        ph_rect.lr.y = rects[i].y + rects[i].h;

        if (PgDrawPhImageRectmx(&ph_pos, SDL_Image, &ph_rect, 0) < 0)
        {
            fprintf(stderr,"ph_NormalUpdate(): PgDrawPhImageRectmx failed !\n");
        }
    }

    if (PgFlush() < 0)
    {
    	fprintf(stderr,"ph_NormalUpdate(): PgFlush failed.\n");
    }
}

void ph_OCUpdate(_THIS, int numrects, SDL_Rect *rects)
{
    int i;

    PhPoint_t zero = {0};
    PhArea_t src_rect;
    PhArea_t dest_rect;

    PgSetRegion(PtWidgetRid(window));
    PgSetClipping(0, NULL);
    PgWaitHWIdle();

    for (i=0; i<numrects; ++i)
    {
        if (rects[i].w == 0)  /* Clipped? */
        {
            continue;
        }

        src_rect.pos.x=rects[i].x;
        src_rect.pos.y=rects[i].y;
        dest_rect.pos.x=rects[i].x;
        dest_rect.pos.y=rects[i].y;

        src_rect.size.w=rects[i].w;
        src_rect.size.h=rects[i].h;
        dest_rect.size.w=rects[i].w;
        dest_rect.size.h=rects[i].h;

        zero.x = 0;
        zero.y = 0;
        PgSetTranslation(&zero, 0);
        PgSetRegion(PtWidgetRid(window));
        PgSetClipping(0, NULL);
        PgContextBlitArea(OCImage.offscreen_context, &src_rect, NULL, &dest_rect);
    }

    if (PgFlush() < 0)
    {
        fprintf(stderr,"ph_OCUpdate(): PgFlush failed.\n");
    }
    
    /* later used to toggling double buffer */
    if (OCImage.current == 0)
    {
        OCImage.CurrentFrameData = OCImage.FrameData0;
    }
    else
    {
        OCImage.CurrentFrameData = OCImage.FrameData1;
    }
}
