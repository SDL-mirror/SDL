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
#include "SDL_ph_modes_c.h"

int ph_SetupImage(_THIS, SDL_Surface *screen)
{
    PgColor_t* palette=NULL;
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
            SDL_SetError("ph_SetupImage(): unsupported bpp=%d !\n", bpp);
            return -1;
        }
        break;
    }

    /* palette emulation code */
    if ((bpp==8) && (desktoppal==SDLPH_PAL_EMULATE))
    {
        /* creating image palette */
        palette=malloc(_Pg_MAX_PALETTE*sizeof(PgColor_t));
        if (palette==NULL)
        {
            SDL_SetError("ph_SetupImage(): can't allocate memory for palette !\n");
            return -1;
        }
        PgGetPalette(palette);

        /* using shared memory for speed (set last param to 1) */
        if ((SDL_Image = PhCreateImage(NULL, screen->w, screen->h, type, palette, _Pg_MAX_PALETTE, 1)) == NULL)
        {
            SDL_SetError("ph_SetupImage(): PhCreateImage() failed for bpp=8 !\n");
            free(palette);
            return -1;
        }
    }
    else
    {
        /* using shared memory for speed (set last param to 1) */
        if ((SDL_Image = PhCreateImage(NULL, screen->w, screen->h, type, NULL, 0, 1)) == NULL)
        {
            SDL_SetError("ph_SetupImage(): PhCreateImage() failed for bpp=%d !\n", bpp);
            return -1;
        }
    }

    screen->pixels = SDL_Image->image;
    screen->pitch = SDL_Image->bpl;

    this->UpdateRects = ph_NormalUpdate;

    return 0;
}

int ph_SetupOCImage(_THIS, SDL_Surface *screen)
{
    int type = 0;
    int bpp;

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
                    SDL_SetError("ph_SetupOCImage(): unsupported bpp=%d !\n", bpp);
                    return -1;
                }
                break;
    }

    /* Currently only offscreen contexts with the same bit depth as the
     * display can be created. */
    OCImage.offscreen_context = PdCreateOffscreenContext(0, screen->w, screen->h, Pg_OSC_MEM_PAGE_ALIGN);

    if (OCImage.offscreen_context == NULL)
    {
        SDL_SetError("ph_SetupOCImage(): PdCreateOffscreenContext() function failed !\n");
        return -1;
    }

    screen->pitch = OCImage.offscreen_context->pitch;

    OCImage.dc_ptr = (unsigned char *) PdGetOffscreenContextPtr(OCImage.offscreen_context);

    if (OCImage.dc_ptr == NULL)
    {
        SDL_SetError("ph_SetupOCImage(): PdGetOffscreenContextPtr function failed !\n");
        PhDCRelease(OCImage.offscreen_context);
        return -1;
    }

    OCImage.FrameData0 = OCImage.dc_ptr;
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
    OCImage.flags = screen->flags;

    /* Begin direct mode */
    if (!ph_EnterFullScreen(this, screen))
    {
        return -1;
    }

    /* store palette for fullscreen */
    if ((screen->format->BitsPerPixel==8) && (desktopbpp!=8))
    {
        PgGetPalette(savedpal);
        PgGetPalette(syspalph);
    }

    OCImage.offscreen_context = PdCreateOffscreenContext(0, 0, 0, Pg_OSC_MAIN_DISPLAY);
    if (OCImage.offscreen_context == NULL)
    {
        SDL_SetError("ph_SetupFullScreenImage(): PdCreateOffscreenContext() function failed !\n");
        return -1;
    }
    
    if ((screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF)
    {
        OCImage.offscreen_backcontext = PdDupOffscreenContext(OCImage.offscreen_context, Pg_OSC_CRTC_SAFE);
        if (OCImage.offscreen_backcontext == NULL)
        {
            SDL_SetError("ph_SetupFullScreenImage(): PdCreateOffscreenContext(back) function failed !\n");
            return -1;
        }
    }

    OCImage.FrameData0 = (unsigned char *)PdGetOffscreenContextPtr(OCImage.offscreen_context);
    if (OCImage.FrameData0 == NULL)
    {
        SDL_SetError("ph_SetupFullScreenImage(): PdGetOffscreenContextPtr() function failed !\n");
        ph_DestroyImage(this, screen);
        return -1;
    }

    if ((screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF)
    {
        OCImage.FrameData1 = (unsigned char *)PdGetOffscreenContextPtr(OCImage.offscreen_backcontext);
        if (OCImage.FrameData1 == NULL)
        {
            SDL_SetError("ph_SetupFullScreenImage(back): PdGetOffscreenContextPtr() function failed !\n");
            ph_DestroyImage(this, screen);
            return -1;
        }
    }

    /* wait for the hardware */
    PgWaitHWIdle();

    if ((screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF)
    {
        OCImage.current = 1;
        PhDCSetCurrent(OCImage.offscreen_backcontext);
        screen->pitch = OCImage.offscreen_backcontext->pitch;
        screen->pixels = OCImage.FrameData1;
        PgSwapDisplay(OCImage.offscreen_context, 0);
    }
    else
    {
        OCImage.current = 0;
        PhDCSetCurrent(OCImage.offscreen_context);
        screen->pitch = OCImage.offscreen_context->pitch;
        screen->pixels = OCImage.FrameData0;
    }

    this->UpdateRects = ph_OCDCUpdate;

    PgFlush();

    return 0;
}

void ph_DestroyImage(_THIS, SDL_Surface *screen)
{
    if (currently_fullscreen)
    {
        /* if we right now in 8bpp fullscreen we must release palette */
        if ((screen->format->BitsPerPixel==8) && (desktopbpp!=8))
        {
            PgSetPalette(syspalph, 0, -1, 0, 0, 0);
            PgSetPalette(savedpal, 0, 0, _Pg_MAX_PALETTE, Pg_PALSET_GLOBAL | Pg_PALSET_FORCE_EXPOSE, 0);
            PgFlush();
        }
        ph_LeaveFullScreen(this);
    }

    if (OCImage.offscreen_context != NULL)
    {
        PhDCRelease(OCImage.offscreen_context);
        OCImage.offscreen_context = NULL;
        OCImage.FrameData0 = NULL;
    }
    if (OCImage.offscreen_backcontext != NULL)
    {
        PhDCRelease(OCImage.offscreen_backcontext);
        OCImage.offscreen_backcontext = NULL;
        OCImage.FrameData1 = NULL;
    }
    OCImage.CurrentFrameData = NULL;

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

int ph_FlipHWSurface(_THIS, SDL_Surface *screen)
{
    PhArea_t area;

    area.pos.x=0;
    area.pos.y=0;
    area.size.w=screen->w;
    area.size.h=screen->h;

    if ((screen->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN)
    {
        if (OCImage.current==0)
        {
            PgSwapDisplay(OCImage.offscreen_context, 0);
            OCImage.current=1;
            screen->pitch = OCImage.offscreen_backcontext->pitch;
            screen->pixels = OCImage.FrameData1;
//            memcpy(OCImage.FrameData1, OCImage.FrameData0, OCImage.offscreen_context->shared_size);
            PgContextBlitArea(OCImage.offscreen_context, &area, OCImage.offscreen_backcontext, &area);
            PhDCSetCurrent(OCImage.offscreen_backcontext);
            PgFlush();
        }
        else
        {
            PgSwapDisplay(OCImage.offscreen_backcontext, 0);
            OCImage.current=0;
            screen->pitch = OCImage.offscreen_context->pitch;
            screen->pixels = OCImage.FrameData0;
//            memcpy(OCImage.FrameData0, OCImage.FrameData1, OCImage.offscreen_context->shared_size);
            PgContextBlitArea(OCImage.offscreen_backcontext, &area, OCImage.offscreen_context, &area);
            PhDCSetCurrent(OCImage.offscreen_context);
            PgFlush();
        }
    }
    return 0;
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

    	if (rects[i].h==0) /* Clipped? */
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
            SDL_SetError("ph_NormalUpdate(): PgDrawPhImageRectmx failed !\n");
        }
    }

    if (PgFlush() < 0)
    {
    	SDL_SetError("ph_NormalUpdate(): PgFlush failed.\n");
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

        if (rects[i].h == 0)  /* Clipped? */
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
        SDL_SetError("ph_OCUpdate(): PgFlush failed.\n");
    }
}

void ph_OCDCUpdate(_THIS, int numrects, SDL_Rect *rects)
{
    PgWaitHWIdle();

    if (PgFlush() < 0)
    {
        SDL_SetError("ph_OCDCUpdate(): PgFlush failed.\n");
    }
}
