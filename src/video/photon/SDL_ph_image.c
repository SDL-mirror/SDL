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

#include "SDL_error.h"
#include "SDL_endian.h"
#include "SDL_ph_image_c.h"

/* remove this line, if photon headers updates */
int PgWaitHWIdle(void);

int ph_SetupImage(_THIS, SDL_Surface *screen)
{
    int type=0;
    PgColor_t* palette=NULL;

    /* Determine image type */
    switch(screen->format->BitsPerPixel)
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
            /* should never get here */
            fprintf(stderr,"error: unsupported bbp = %d\n",
                    screen->format->BitsPerPixel);
            return -1;
        }
        break;
    }

    /* palette emulation code */
    if ((screen->format->BitsPerPixel==8) && (desktoppal==SDLPH_PAL_EMULATE))
    {
        /* creating image palette */
        palette=malloc(_Pg_MAX_PALETTE*sizeof(PgColor_t));
        PgGetPalette(palette);

        /* using shared memory for speed (set last param to 1) */
        if ((SDL_Image = PhCreateImage(NULL, screen->w, screen->h, type, palette, _Pg_MAX_PALETTE, 1)) == NULL)
        {
            fprintf(stderr,"ph_SetupImage: PhCreateImage failed for bpp=8.\n");
            return -1;
        }
    }
    else
    {
        /* using shared memory for speed (set last param to 1) */
        if ((SDL_Image = PhCreateImage(NULL, screen->w, screen->h, type, NULL, 0, 1)) == NULL)
        {
            fprintf(stderr,"ph_SetupImage: PhCreateImage failed.\n");
            return -1;
        }
    }

    screen->pixels = SDL_Image->image;

    this->UpdateRects = ph_NormalUpdate;

    return 0;
}

int ph_SetupOCImage(_THIS, SDL_Surface *screen)
{
	int type = 0;

	/* Determine image type */
	switch(screen->format->BitsPerPixel)
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
		/* should never get here */
			fprintf(stderr,"error: unsupported bbp = %d\n",
					screen->format->BitsPerPixel);
			return -1;
		}
		break;
	}

	OCImage.FrameData0 = (FRAMEDATA *) malloc((size_t)(sizeof( FRAMEDATA)));
	OCImage.FrameData1 = (FRAMEDATA *) malloc((size_t)(sizeof( FRAMEDATA)));

	if(OCImage.direct_context == NULL)
	   OCImage.direct_context = PdCreateDirectContext();

	OCImage.offscreen_context = PdCreateOffscreenContext(0,screen->w,screen->h, Pg_OSC_MEM_PAGE_ALIGN);
				
	if (OCImage.offscreen_context == NULL)
	{
	   printf("PdCreateOffscreenContext  failed\n");
	   return -1;
	}

	OCImage.Stride = OCImage.offscreen_context->pitch;	

        if (OCImage.flags & SDL_DOUBLEBUF)
      	   printf("hardware flag for doublebuf offscreen context\n");

			
			OCImage.dc_ptr.ptr8 = (unsigned char *) PdGetOffscreenContextPtr(OCImage.offscreen_context);
			
			OCImage.CurrentFrameData = OCImage.FrameData0;
			OCImage.CurrentFrameData->Y = OCImage.dc_ptr.ptr8;
			OCImage.CurrentFrameData->U = NULL;
			OCImage.CurrentFrameData->V = NULL;
			OCImage.current = 0;
	
			if(OCImage.dc_ptr.ptr8 == NULL)
			{
 				printf("PdGetOffscreenContextPtr failed\n");
 				return -1;
			}
			
			PhDCSetCurrent(OCImage.offscreen_context);

			screen->pixels = OCImage.CurrentFrameData->Y;
	
			this->UpdateRects = ph_OCUpdate;

	return 0;
}

int ph_SetupOpenGLImage(_THIS, SDL_Surface* screen)
{
   this->UpdateRects = ph_OpenGLUpdate;
   
   return 0;
}

void ph_DestroyImage(_THIS, SDL_Surface *screen)
{
    if (OCImage.offscreen_context != NULL)
    {
        PhDCRelease(OCImage.offscreen_context);
        OCImage.offscreen_context = NULL;
        free(OCImage.FrameData0);
        OCImage.FrameData0 = NULL;
        free(OCImage.FrameData1);
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

int ph_ResizeImage(_THIS, SDL_Surface *screen, Uint32 flags)
{
    ph_DestroyImage(this, screen);
    
    if (flags & SDL_HWSURFACE)
    {
        OCImage.flags = flags;  /* needed for SDL_DOUBLEBUF check */
        return ph_SetupOCImage(this, screen);
    }
    else if (flags & SDL_OPENGL)
    {
        return ph_SetupOpenGLImage(this, screen);
    } 
    else
    {
        return ph_SetupImage(this, screen);
    }      
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
    if ((surface == SDL_VideoSurface) && blit_queued) {
	PgFlush();
        blit_queued = 0;
    }

    return(0);
}

void ph_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
    return;
}

static PhPoint_t ph_pos;
static PhRect_t ph_rect;
static int i;

void ph_OpenGLUpdate(_THIS, int numrects, SDL_Rect* rects)
{
   this->GL_SwapBuffers(this);
   
   return;
}

void ph_NormalUpdate(_THIS, int numrects, SDL_Rect *rects)
{
    for ( i=0; i<numrects; ++i ) 
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
            fprintf(stderr,"ph_NormalUpdate: PgDrawPhImageRectmx failed.\n");
        }
    }

    if (PgFlush() < 0)
    {
    	fprintf(stderr,"ph_NormalUpdate: PgFlush failed.\n");
    }
}
void ph_OCUpdate(_THIS, int numrects, SDL_Rect *rects)
{
    PhPoint_t zero = {0};
    PhRect_t src_rect;
    PhRect_t dest_rect;

    if(OCImage.direct_context == NULL)
    {
        return;
    }

    PgSetRegion(PtWidgetRid(window));
    PgSetClipping(0,NULL);
    PgWaitHWIdle();

    for (i=0; i<numrects; ++i)
    {
        if (rects[i].w == 0)  /* Clipped? */
        {
            continue;
        }

        src_rect.ul.x=rects[i].x;
        src_rect.ul.y=rects[i].y;
        dest_rect.ul.x=rects[i].x;
        dest_rect.ul.y=rects[i].y;

        dest_rect.lr.x=src_rect.lr.x= rects[i].x +rects[i].w;
        dest_rect.lr.y=src_rect.lr.y= rects[i].y +rects[i].h;

        zero.x = zero.y = 0;
        PgSetTranslation (&zero, 0);
        PgSetRegion(PtWidgetRid(window));
        PgSetClipping(0,NULL);
        PgContextBlitArea(OCImage.offscreen_context, (PhArea_t *)(&src_rect), NULL, (PhArea_t *)(&dest_rect));

    }
    if (PgFlush() < 0)
    {
        fprintf(stderr,"ph_OCUpdate: PgFlush failed.\n");
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
