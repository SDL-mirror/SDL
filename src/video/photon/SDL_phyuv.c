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

/* This is the QNX Realtime Platform version for SDL YUV video overlays */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <Ph.h>
#include <Pt.h>

#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_phyuv_c.h"
#include "SDL_yuvfuncs.h"

#define OVERLAY_STATE_UNINIT 0
#define OVERLAY_STATE_ACTIVE 1

/* The functions used to manipulate software video overlays */
static struct private_yuvhwfuncs ph_yuvfuncs = {
    ph_LockYUVOverlay,
    ph_UnlockYUVOverlay,
    ph_DisplayYUVOverlay,
    ph_FreeYUVOverlay
};

struct private_yuvhwdata {
    FRAMEDATA* CurrentFrameData;
    FRAMEDATA* FrameData0;
    FRAMEDATA* FrameData1;
    PgScalerProps_t   props;
    PgScalerCaps_t    caps;
    PgVideoChannel_t* channel;
    PhArea_t CurrentWindow;
    long format;
    int planar;
    int scaler_on;
    int current;
    long YStride;
    long VStride;
    long UStride;
    int ischromakey;
    long chromakey;
    unsigned long State;
    long flags;
    int locked;
};

int grab_ptrs2(PgVideoChannel_t* channel, FRAMEDATA* Frame0, FRAMEDATA* Frame1 )
{
    int planes = 0;

    /* Buffers have moved; re-obtain the pointers */
    Frame0->Y = (unsigned char *)PdGetOffscreenContextPtr(channel->yplane1);
    Frame1->Y = (unsigned char *)PdGetOffscreenContextPtr(channel->yplane2);
    Frame0->U = (unsigned char *)PdGetOffscreenContextPtr(channel->uplane1);
    Frame1->U = (unsigned char *)PdGetOffscreenContextPtr(channel->uplane2);
    Frame0->V = (unsigned char *)PdGetOffscreenContextPtr(channel->vplane1);
    Frame1->V = (unsigned char *)PdGetOffscreenContextPtr(channel->vplane2);

    if (Frame0->Y)
        planes++;

    if (Frame0->U)
        planes++;

    if (Frame0->V)
        planes++;

    return planes;
}

SDL_Overlay* ph_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface *display)
{
    SDL_Overlay *overlay;
    struct private_yuvhwdata *hwdata;
    int xv_port;
    int rtncode;
    int planes;
    int i=0;
    PhPoint_t pos;

    /* Create the overlay structure */
    overlay = calloc(1, sizeof(SDL_Overlay));

    if (overlay == NULL) {
        SDL_OutOfMemory();
        return (NULL);
    }

    /* Fill in the basic members */
    overlay->format = format;
    overlay->w = width;
    overlay->h = height;
	
    /* Set up the YUV surface function structure */
    overlay->hwfuncs = &ph_yuvfuncs;

    /* Create the pixel data and lookup tables */
    hwdata = calloc(1, sizeof(struct private_yuvhwdata));

    overlay->hwdata = hwdata;
    if (hwdata == NULL) {
        SDL_OutOfMemory();
        SDL_FreeYUVOverlay(overlay);
        return(NULL);
    }

    PhDCSetCurrent(0);
    if (overlay->hwdata->channel == NULL)
    {
        if ((overlay->hwdata->channel = PgCreateVideoChannel(Pg_VIDEO_CHANNEL_SCALER,0)) == NULL) 
        {
            SDL_SetError("ph_CreateYUVOverlay(): Create channel failed: %s\n", strerror(errno));
            SDL_FreeYUVOverlay(overlay);

            return(NULL);

        }
    }

    PtGetAbsPosition(window, &pos.x, &pos.y);
    overlay->hwdata->CurrentWindow.pos.x = pos.x;
    overlay->hwdata->CurrentWindow.pos.y = pos.y;
    overlay->hwdata->CurrentWindow.size.w = width;
    overlay->hwdata->CurrentWindow.size.h = height;
    overlay->hwdata->State = OVERLAY_STATE_UNINIT;
    overlay->hwdata->FrameData0 = (FRAMEDATA *) calloc(1, sizeof(FRAMEDATA));
    overlay->hwdata->FrameData1 = (FRAMEDATA *) calloc(1, sizeof(FRAMEDATA));

    xv_port = -1;
    i=0;
    
    overlay->hwdata->ischromakey=0;

    do {
        memset(&overlay->hwdata->caps, 0x00, sizeof(PgScalerCaps_t));
        overlay->hwdata->caps.size = sizeof(PgScalerCaps_t);
        rtncode = PgGetScalerCapabilities(overlay->hwdata->channel, i, &overlay->hwdata->caps);
        if (rtncode==0)
        { 
            if (overlay->hwdata->caps.format==format)
            {
               if ((overlay->hwdata->caps.flags & Pg_SCALER_CAP_DST_CHROMA_KEY) == Pg_SCALER_CAP_DST_CHROMA_KEY)
               {
                   overlay->hwdata->ischromakey=1;
               }
               xv_port=1;
               break;
            }
        }
        else
        {
           break;
        }
        i++;
    } while(1);


    if (xv_port == -1)
    {
        SDL_SetError("No available video ports for requested format\n");
        SDL_FreeYUVOverlay(overlay);
        return(NULL);
    }
 
    overlay->hwdata->format = format;
    overlay->hwdata->props.format = format;
    overlay->hwdata->props.size = sizeof(PgScalerProps_t);
    overlay->hwdata->props.src_dim.w = width;   
    overlay->hwdata->props.src_dim.h = height;   
	
    /* Don't use chromakey for now, blitting a surface will cover the window,
     * and therefore the chroma. */
    overlay->hwdata->chromakey = 0;
    PtSetResource(window, Pt_ARG_FILL_COLOR, overlay->hwdata->chromakey, 0);

    PhAreaToRect(&overlay->hwdata->CurrentWindow, &overlay->hwdata->props.viewport);

    overlay->hwdata->props.flags = Pg_SCALER_PROP_DOUBLE_BUFFER;

    if ((overlay->hwdata->ischromakey)&&(overlay->hwdata->chromakey))
    {
        overlay->hwdata->props.flags |= Pg_SCALER_PROP_CHROMA_ENABLE;
        overlay->hwdata->props.color_key = overlay->hwdata->chromakey;
        overlay->hwdata->props.color_key_mask = 0x00FFFFFFUL;
    } 
    else
    {
        overlay->hwdata->props.flags &= ~Pg_SCALER_PROP_CHROMA_ENABLE;
    }

    rtncode = PgConfigScalerChannel(overlay->hwdata->channel, &overlay->hwdata->props);

    switch(rtncode)
    {
        case -1: SDL_SetError("PgConfigScalerChannel failed\n");
		 SDL_FreeYUVOverlay(overlay);
		 return(NULL);
   		 break;
        case 1:
        case 0:
        default:
                 break;
    }

    planes = grab_ptrs2(overlay->hwdata->channel, overlay->hwdata->FrameData0, overlay->hwdata->FrameData1);

    if(overlay->hwdata->channel->yplane1 != NULL)			
        overlay->hwdata->YStride = overlay->hwdata->channel->yplane1->pitch;
    if(overlay->hwdata->channel->uplane1 != NULL)			
        overlay->hwdata->UStride = overlay->hwdata->channel->uplane1->pitch;
    if(overlay->hwdata->channel->vplane1 != NULL)			
        overlay->hwdata->VStride = overlay->hwdata->channel->vplane1->pitch;

    overlay->hwdata->current = PgNextVideoFrame(overlay->hwdata->channel);

    if(overlay->hwdata->current==0)
        overlay->hwdata->CurrentFrameData = overlay->hwdata->FrameData0;
    else
        overlay->hwdata->CurrentFrameData = overlay->hwdata->FrameData1;

    overlay->hwdata->locked = 1;

    /* Find the pitch and offset values for the overlay */
    overlay->planes = planes;
    overlay->pitches = calloc(overlay->planes, sizeof(Uint16));
    overlay->pixels  = calloc(overlay->planes, sizeof(Uint8*));
    if (!overlay->pitches || !overlay->pixels)
    {
        SDL_OutOfMemory();
        SDL_FreeYUVOverlay(overlay);
        return(NULL);
    }

    if (overlay->planes > 0)
    {
        overlay->pitches[0] = overlay->hwdata->channel->yplane1->pitch;
        overlay->pixels[0]  = overlay->hwdata->CurrentFrameData->Y;
    }
    if (overlay->planes > 1)
    {
        overlay->pitches[1] = overlay->hwdata->channel->uplane1->pitch;
        overlay->pixels[1]  = overlay->hwdata->CurrentFrameData->U;
    }
    if (overlay->planes > 2)
    {
        overlay->pitches[2] = overlay->hwdata->channel->vplane1->pitch;
        overlay->pixels[2]  = overlay->hwdata->CurrentFrameData->V;
    }

    overlay->hwdata->State = OVERLAY_STATE_ACTIVE;
    overlay->hwdata->scaler_on = 0;
    overlay->hw_overlay = 1;

    return (overlay);
}

int ph_LockYUVOverlay(_THIS, SDL_Overlay *overlay)
{
    if (overlay == NULL)
        return 0;

    overlay->hwdata->current = PgNextVideoFrame(overlay->hwdata->channel);
    if (overlay->hwdata->current == -1)
    {
        SDL_SetError("PgNextFrame failed, bailing out\n");
        SDL_FreeYUVOverlay(overlay);
        return(NULL);
    }

    overlay->hwdata->locked = 1;

    /* set current frame for double buffering */
    if (overlay->hwdata->current == 0)
        overlay->hwdata->CurrentFrameData = overlay->hwdata->FrameData0;
    else
        overlay->hwdata->CurrentFrameData = overlay->hwdata->FrameData1;

    if (overlay->planes > 0)
    {
        overlay->pitches[0] = overlay->hwdata->channel->yplane1->pitch;
        overlay->pixels[0]  = overlay->hwdata->CurrentFrameData->Y;
    }
    if (overlay->planes > 1)
    {
        overlay->pitches[1] = overlay->hwdata->channel->uplane1->pitch;
        overlay->pixels[1]  = overlay->hwdata->CurrentFrameData->U;
    }
    if (overlay->planes > 2)
    {
        overlay->pitches[2] = overlay->hwdata->channel->vplane1->pitch;
        overlay->pixels[2]  = overlay->hwdata->CurrentFrameData->V;
    }

    return(0);
}

void ph_UnlockYUVOverlay(_THIS, SDL_Overlay *overlay)
{
    int rtncode;

    if(overlay == NULL)
         return;

    if(overlay->hwdata->scaler_on == 1) 
    {
        rtncode =PgConfigScalerChannel(overlay->hwdata->channel, &(overlay->hwdata->props));
        switch(rtncode)
        {
            case -1:
                     SDL_SetError("PgConfigScalerChannel failed\n");
                     SDL_FreeYUVOverlay(overlay);
                     break;
            case 1:
                     grab_ptrs2(overlay->hwdata->channel, overlay->hwdata->FrameData0, overlay->hwdata->FrameData1);
                     break;
            case 0:
            default:
                     break;
        }
    }

    /* This would be the best place to draw chromakey but we do not have a SDL_Surface in the args
     * This means we might see a chromakey flicker at startup. */
}

int ph_DisplayYUVOverlay(_THIS, SDL_Overlay *overlay, SDL_Rect *dstrect)
{
    int rtncode;
    PhPoint_t pos;

    if(overlay == NULL)
        return -1;

    /* If CurrentWindow has change, move the viewport */
    if((overlay->hwdata->CurrentWindow.pos.x != dstrect->x) ||
       (overlay->hwdata->CurrentWindow.pos.y != dstrect->y) ||
       (overlay->hwdata->CurrentWindow.size.w != dstrect->w) ||
       (overlay->hwdata->CurrentWindow.size.h != dstrect->h) ||
       (overlay->hwdata->scaler_on==0))
    {
        if(overlay->hwdata->State == OVERLAY_STATE_UNINIT)
            return -1;

        overlay->hwdata->props.flags |= Pg_SCALER_PROP_SCALER_ENABLE;
        overlay->hwdata->scaler_on = 1;

        PtGetAbsPosition(window, &pos.x, &pos.y);
        overlay->hwdata->CurrentWindow.pos.x = pos.x + dstrect->x;
        overlay->hwdata->CurrentWindow.pos.y = pos.y + dstrect->y;
        overlay->hwdata->CurrentWindow.size.w = dstrect->w;
        overlay->hwdata->CurrentWindow.size.h = dstrect->h;

        PhAreaToRect(&overlay->hwdata->CurrentWindow, &overlay->hwdata->props.viewport);

        rtncode = PgConfigScalerChannel(overlay->hwdata->channel, &(overlay->hwdata->props));

        switch(rtncode)
        {
            case -1:
                     SDL_SetError("PgConfigScalerChannel failed\n");
                     SDL_FreeYUVOverlay(overlay);
                     return (0);
            case 1:
                     grab_ptrs2(overlay->hwdata->channel, overlay->hwdata->FrameData0, overlay->hwdata->FrameData1);
                     break;
            case 0:
            default:
                     break;
        }
    }

    if (!overlay->hwdata->locked)
    {
        overlay->hwdata->current = PgNextVideoFrame(overlay->hwdata->channel);
        if (overlay->hwdata->current == -1)
        {
            SDL_SetError("PgNextVideoFrame failed\n");
            SDL_FreeYUVOverlay(overlay);
            return 0;
        }
        if (overlay->hwdata->current == 0)
            overlay->hwdata->CurrentFrameData = overlay->hwdata->FrameData0;
        else
            overlay->hwdata->CurrentFrameData = overlay->hwdata->FrameData1;

        if (overlay->planes > 0)
        {
            overlay->pitches[0] = overlay->hwdata->channel->yplane1->pitch;
            overlay->pixels[0]  = overlay->hwdata->CurrentFrameData->Y;
        }
        if (overlay->planes > 1)
        {
            overlay->pitches[1] = overlay->hwdata->channel->uplane1->pitch;
            overlay->pixels[1]  = overlay->hwdata->CurrentFrameData->U;
        }
        if (overlay->planes > 2)
        {
            overlay->pitches[2] = overlay->hwdata->channel->vplane1->pitch;
            overlay->pixels[2]  = overlay->hwdata->CurrentFrameData->V;
        }
    }
        
    return 0;
}

void ph_FreeYUVOverlay(_THIS, SDL_Overlay *overlay)
{
    if (overlay == NULL)
        return;

    if (overlay->hwdata == NULL)
        return;

    /* it is need for some buggy drivers, that can't hide overlay before */
    /* freeing buffer, so we got trash on the srceen                     */
    overlay->hwdata->props.flags &= ~Pg_SCALER_PROP_SCALER_ENABLE;
    PgConfigScalerChannel(overlay->hwdata->channel, &(overlay->hwdata->props));

    overlay->hwdata->scaler_on = 0;
    overlay->hwdata->State = OVERLAY_STATE_UNINIT;

    if (overlay->hwdata->channel != NULL)
    {
        PgDestroyVideoChannel(overlay->hwdata->channel);
        overlay->hwdata->channel = NULL;
        return;
    }	

    overlay->hwdata->CurrentFrameData = NULL;  
	
    free(overlay->hwdata->FrameData0);
    free(overlay->hwdata->FrameData1);
    overlay->hwdata->FrameData0 = NULL;
    overlay->hwdata->FrameData1 = NULL;
    free(overlay->hwdata);
}
