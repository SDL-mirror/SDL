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

/* This is the QNX Realtime Platform version for SDL YUV video overlays */

#include <stdlib.h>
#include <string.h>
//#include <ncurses.h> //only for bool
#ifndef bool
#define bool char
#define TRUE 1
#define FALSE 0
#endif
#include <errno.h>

#include <Ph.h>
#include <Pt.h>

#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_phyuv_c.h"
#include "SDL_yuvfuncs.h"

#if 0  //just for reference
/* YUV data formats			FourCC		   Layout		H sample (YUV)	V sample (YUV)	BPP */
#define Pg_VIDEO_FORMAT_IYU1		0x31555949	/* U2Y2Y2V2Y2Y2		144		111		12  */
#define Pg_VIDEO_FORMAT_IYU2		0x32555949	/* U4Y4V4U4Y4V4		111		111		24  */
#define Pg_VIDEO_FORMAT_UYVY		0x59565955	/* U8Y8V8Y8		122		111		16  */
#define Pg_VIDEO_FORMAT_YUY2		0x32595559	/* Y8U8Y8V8		122		111		16  */
#define Pg_VIDEO_FORMAT_YVYU		0x55595659	/* Y8V8Y8U8		122		111		16  */
#define Pg_VIDEO_FORMAT_V422		0x56343232	/* V8Y8U8Y8		122		111		16  */
#define Pg_VIDEO_FORMAT_CLJR		0x524a4c43	/* V6U6Y5Y5Y5Y5		133		111		8   */
#define Pg_VIDEO_FORMAT_YVU9		0x39555659	/* Planar YVU		144		144		9   */
#define Pg_VIDEO_FORMAT_YV12		0x32315659	/* Planar YUV		122		122		12  */

/* There seems to be no FourCC that matches this */
#define Pg_VIDEO_FORMAT_YUV420		0x00000100	/* Planar YUV		122		111		16  */

/* These formats are the same as YV12, except the U and V planes do not have to contiguously follow the Y plane */
/* but they're all the same to us, since we always have 3 plane pointers */
#define Pg_VIDEO_FORMAT_CLPL	Pg_VIDEO_FORMAT_YV12	/* Cirrus Logic Planar format */
#define Pg_VIDEO_FORMAT_VBPL	Pg_VIDEO_FORMAT_YV12	/* VooDoo Banshee planar format */

#define SDL_YV12_OVERLAY	0x32315659	/* Planar mode: Y + V + U */
#define SDL_IYUV_OVERLAY	0x56555949	/* Planar mode: Y + U + V */
#define SDL_YUY2_OVERLAY	0x32595559	/* Packed mode: Y0+U0+Y1+V0 */
#define SDL_UYVY_OVERLAY	0x59565955	/* Packed mode: U0+Y0+V0+Y1 */
#define SDL_YVYU_OVERLAY	0x55595659	/* Packed mode: Y0+V0+Y1+U0 */

#endif 


#define OVERLAY_STATE_UNINIT  0
#define OVERLAY_STATE_ACTIVE 1

/* The functions used to manipulate software video overlays */
static struct private_yuvhwfuncs ph_yuvfuncs = {
	ph_LockYUVOverlay,
	ph_UnlockYUVOverlay,
	ph_DisplayYUVOverlay,
	ph_FreeYUVOverlay
};


typedef struct {
  int id;
  int width, height;
  int data_size;              /* bytes */
  int num_planes;
  int *pitches;               /* bytes */
  int *offsets;               /* bytes */
  char *data;
  void *obdata;     
} XvImage;


struct private_yuvhwdata {
	XvImage *image;	
	FRAMEDATA *CurrentFrameData;
	FRAMEDATA *FrameData0;
	FRAMEDATA *FrameData1;
	PgScalerProps_t	props;
	PgScalerCaps_t		caps;
	PgVideoChannel_t *channel;
	SDL_Rect CurrentWindow;
	long format;
	int screen_width;
	int screen_height ;
	int screen_bpp ;    //2
	bool planar;
	bool scaler_on ;
	int current;
	long YStride;
	long VStride;
	long UStride;
	long chromakey;
	unsigned long State;
	long flags;
};

extern PgVideoChannel_t * PgCreateVideoChannel(unsigned type, unsigned flags);
extern int PgGetScalerCapabilities( PgVideoChannel_t *channel, int format_index, PgScalerCaps_t *vcaps );
extern int PgConfigScalerChannel(PgVideoChannel_t *channel, PgScalerProps_t *props);
extern void PgDestroyVideoChannel(PgVideoChannel_t *channel);
extern PgColor_t PgGetOverlayChromaColor(void);

void
grab_ptrs2(PgVideoChannel_t *channel, FRAMEDATA *Frame0, FRAMEDATA *Frame1 )
{

	/* Buffers have moved; re-obtain the pointers */
	Frame0->Y = (unsigned char *)PdGetOffscreenContextPtr(channel->yplane1);
	Frame1->Y = (unsigned char *)PdGetOffscreenContextPtr(channel->yplane2);
	Frame0->U = (unsigned char *)PdGetOffscreenContextPtr(channel->uplane1);
	Frame1->U = (unsigned char *)PdGetOffscreenContextPtr(channel->uplane2);
	Frame0->V = (unsigned char *)PdGetOffscreenContextPtr(channel->vplane1);
	Frame1->V = (unsigned char *)PdGetOffscreenContextPtr(channel->vplane2);

}

SDL_Overlay *ph_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface *display)
{
	SDL_Overlay *overlay;
	struct private_yuvhwdata *hwdata;
	int xv_port;
	int rtncode;
//	PhRect_t rect;
//	PhSysInfo_t info;
//	PhRegion_t region;
//	short x, y;
	PtArg_t argt;
	int i =0;
//	bool bCont = TRUE;
	int Priority[20];
	int Type[20];
	int entries, select, highest;

	PhDCSetCurrent(0);  //Need to set draw context to window esp. if we we in Offscreeen mode

	/* Create the overlay structure */
	overlay = (SDL_Overlay *)malloc(sizeof *overlay);
	if ( overlay == NULL ) {
		SDL_OutOfMemory();
		return(NULL);
	}
	memset(overlay, 0, (sizeof *overlay));

	/* Fill in the basic members */
	overlay->format = format;
	overlay->w = width;
	overlay->h = height;
	
	/* Set up the YUV surface function structure */
	overlay->hwfuncs = &ph_yuvfuncs;

	/* Create the pixel data and lookup tables */
	hwdata = (struct private_yuvhwdata *)malloc(sizeof *hwdata);
	overlay->hwdata = hwdata;
	if ( hwdata == NULL ) {
		SDL_OutOfMemory();
		SDL_FreeYUVOverlay(overlay);
		return(NULL);
	}
	
		if (overlay->hwdata->channel == NULL)
	{
	
  
		if ((overlay->hwdata->channel = PgCreateVideoChannel(Pg_VIDEO_CHANNEL_SCALER,0)) == NULL) 
		{
			SDL_SetError("Create channel failed:%s\n", strerror( errno ));
			free(overlay->hwdata);
			free(overlay);
			return(NULL);
		}
#if 0
		overlay->hwdata->caps.size = sizeof (overlay->hwdata->caps);
		PgGetScalerCapabilities(overlay->hwdata->channel, 0, &(overlay->hwdata->caps));
		if (overlay->hwdata->caps.flags & Pg_SCALER_CAP_DOUBLE_BUFFER)
			overlay->hwdata->props.flags |= Pg_SCALER_PROP_DOUBLE_BUFFER;
#endif	
	}

overlay->hwdata->CurrentWindow.x = 0;
overlay->hwdata->CurrentWindow.y = 0;
overlay->hwdata->CurrentWindow.w = 320;
overlay->hwdata->CurrentWindow.h = 240;



overlay->hwdata->State = OVERLAY_STATE_UNINIT;

overlay->hwdata->screen_bpp = 2;
overlay->hwdata->scaler_on = FALSE;

overlay->hwdata->screen_width = 1024;
overlay->hwdata->screen_height  = 768;

overlay->hwdata->FrameData0 = (FRAMEDATA *) malloc((size_t)(sizeof( FRAMEDATA)));
overlay->hwdata->FrameData1 = (FRAMEDATA *) malloc((size_t)(sizeof( FRAMEDATA)));

overlay->hwdata->caps.size = sizeof(overlay->hwdata->caps);


//Note you really don't need to do this for SDL as you are given a format, but this is a good example

xv_port = -1;
i=0;
	
while(PgGetScalerCapabilities(overlay->hwdata->channel, i++, &(overlay->hwdata->caps)) == 0) 
{

		if(overlay->hwdata->caps.format  == Pg_VIDEO_FORMAT_YV12) //in SDL
		{
			
			Priority[i-1] = 0;
			Type[i-1] = Pg_VIDEO_FORMAT_YV12;
			if(format == Pg_VIDEO_FORMAT_YV12)
			{
				overlay->hwdata->props.format =  Pg_VIDEO_FORMAT_YV12;
				xv_port = 1; //supported
				Priority[i-1] = 100; //force selected
			}
			
		}
		else if(overlay->hwdata->caps.format  == Pg_VIDEO_FORMAT_YVU9) //in SDL
		{
			
			Priority[i-1] = 0;
			Type[i-1] = Pg_VIDEO_FORMAT_YVU9;			
			if(format == Pg_VIDEO_FORMAT_YVU9)
			{
				overlay->hwdata->props.format =  Pg_VIDEO_FORMAT_YVU9;
				xv_port = 1; //supported
				Priority[i-1] = 100; //force selected
			}
			
		}
#if 0 //this part of SDL is YUV specific
		else if(overlay->hwdata->caps.format  == Pg_VIDEO_FORMAT_RGB555)
		{
			
			Priority[i-1] = 3;
			Type[i-1] = Pg_VIDEO_FORMAT_RGB555;			
		}
		else if(overlay->hwdata->caps.format  == Pg_VIDEO_FORMAT_RGB565)
		{
			
			Priority[i-1] =  2;
			Type[i-1] = Pg_VIDEO_FORMAT_RGB565;			
		}
		else if(overlay->hwdata->caps.format == Pg_VIDEO_FORMAT_RGB8888)
		{
			
			Priority[i-1] = 1;
			Type[i-1] = Pg_VIDEO_FORMAT_RGB8888;			
		}
#endif
		else if(overlay->hwdata->caps.format  == Pg_VIDEO_FORMAT_IYU1)
		{
			
			Priority[i-1] = 0;
			Type[i-1] = Pg_VIDEO_FORMAT_IYU1;
			
		}
		else if(overlay->hwdata->caps.format  == Pg_VIDEO_FORMAT_IYU2)
		{
			
			Priority[i-1] = 0;
			Type[i-1] = Pg_VIDEO_FORMAT_IYU2;			
		}

		else if(overlay->hwdata->caps.format  == Pg_VIDEO_FORMAT_UYVY) //in SDL
		{
			
			Priority[i-1] = 7;
			Type[i-1] = Pg_VIDEO_FORMAT_UYVY;
			if(format == Pg_VIDEO_FORMAT_UYVY)
			{
				overlay->hwdata->props.format =  Pg_VIDEO_FORMAT_UYVY;
				xv_port = 1; //supported
				Priority[i-1] = 100; //force selected
			}
			
		}
		else if(overlay->hwdata->caps.format == Pg_VIDEO_FORMAT_YUY2) //in SDL
		{
			
			Priority[i-1] = 8;
			Type[i-1] = Pg_VIDEO_FORMAT_YUY2;			
			if(format == Pg_VIDEO_FORMAT_YUY2)
			{
				overlay->hwdata->props.format =  Pg_VIDEO_FORMAT_YUY2;
				xv_port = 1; //supported
				Priority[i-1] = 100; //force selected
			}
			
		}
		else if(overlay->hwdata->caps.format  == Pg_VIDEO_FORMAT_YVYU) //in SDL
		{
			
			Priority[i-1] = 4;
			Type[i-1] = Pg_VIDEO_FORMAT_YVYU;	
			
			if(format == Pg_VIDEO_FORMAT_YVYU)
			{
				overlay->hwdata->props.format =  Pg_VIDEO_FORMAT_YVYU;
				xv_port = 1; //supported
				Priority[i-1] = 100; //force selected
				
			}
		
		}
		else if(overlay->hwdata->caps.format  == Pg_VIDEO_FORMAT_V422)
		{
			
			Priority[i-1] = 5;
			Type[i-1] = Pg_VIDEO_FORMAT_V422;			
		}		
		else if(overlay->hwdata->caps.format  == Pg_VIDEO_FORMAT_CLJR)
		{
			
			Priority[i-1] = 6;
			Type[i-1] = Pg_VIDEO_FORMAT_CLJR;		
		}	
		else
		{
		
		Priority[i-1] = 0;
		}
			
overlay->hwdata->caps.size = sizeof(overlay->hwdata->caps);
}

	if ( xv_port == -1 )
	{
		SDL_SetError("No available video ports for requested format");
		return(NULL);
	}
 
//Pick the highest priority format
entries = i -2;
highest = Priority[0]; //make first entry top at begining
select = 0;

for (i = 1; i < entries; i++)
{


   if(Priority[i] > highest)
   {
      highest = Priority[i];
      select  = i;
   }
} 

 
 
 overlay->hwdata->caps.size = sizeof (overlay->hwdata->caps	);
PgGetScalerCapabilities(overlay->hwdata->channel, select, &(overlay->hwdata->caps));
overlay->hwdata->props.format = overlay->hwdata->caps.format ;

    overlay->hwdata->format = overlay->hwdata->props.format;  //to make easier for apps to use


	overlay->hwdata->props.size = sizeof (overlay->hwdata->props);
    overlay->hwdata->props.src_dim.w = width;   
    overlay->hwdata->props.src_dim.h = height;   
	
	overlay->hwdata->chromakey = PgGetOverlayChromaColor();

	// Set chromakey in video widget so we can see overlay data
	/* I don't know where the container widget is!!!, I guess it is in hidden->window*/
	
	PtEnter(0);
	PtSetArg( &argt, Pt_ARG_FILL_COLOR, overlay->hwdata->chromakey, 0 );
	PtSetResources( window, 1, &argt ); 
	PtLeave(0);


	fflush( stderr );

	overlay->hwdata->props.viewport.ul.x = overlay->hwdata->CurrentWindow.x;
	overlay->hwdata->props.viewport.ul.y = overlay->hwdata->CurrentWindow.y;
	//Next line MIGHT have x and y reversed!!!!!!!!!!!!
	overlay->hwdata->props.viewport.lr.x = overlay->hwdata->CurrentWindow.x +overlay->hwdata->CurrentWindow.w;
	overlay->hwdata->props.viewport.lr.y = overlay->hwdata->CurrentWindow.y + overlay->hwdata->CurrentWindow.h;
		


	overlay->hwdata->props.flags =
	    ~Pg_SCALER_PROP_SCALER_ENABLE | Pg_SCALER_PROP_DOUBLE_BUFFER ;

	if (overlay->hwdata->chromakey) {
		overlay->hwdata->props.flags |= Pg_SCALER_PROP_CHROMA_ENABLE;
		overlay->hwdata->props.color_key = overlay->hwdata->chromakey;
		overlay->hwdata->props.color_key_mask = 0xffffff;
	} 
	else
	{
		overlay->hwdata->props.flags &= ~Pg_SCALER_PROP_CHROMA_ENABLE;
	}


	overlay->hwdata->scaler_on = FALSE;



 rtncode =    PgConfigScalerChannel(overlay->hwdata->channel, &(overlay->hwdata->props));	
	switch(rtncode)
	{
	case -1:
		SDL_SetError("PgConfigScalerChannel failed\n");
		SDL_FreeYUVOverlay(overlay);
		return(NULL);
   		break;
	case 1:
		grab_ptrs2(overlay->hwdata->channel, overlay->hwdata->FrameData0, overlay->hwdata->FrameData1);
		break;
	case 0:
	default:
   		break;
	}


	grab_ptrs2(overlay->hwdata->channel, overlay->hwdata->FrameData0, overlay->hwdata->FrameData1);

if(overlay->hwdata->channel->yplane1 != NULL)			
	overlay->hwdata->YStride = overlay->hwdata->channel->yplane1->pitch;
if(overlay->hwdata->channel->uplane1 != NULL)			
	overlay->hwdata->UStride = overlay->hwdata->channel->uplane1->pitch;
if(overlay->hwdata->channel->vplane1 != NULL)			
	overlay->hwdata->VStride = overlay->hwdata->channel->vplane1->pitch;


	overlay->hwdata->current = PgNextVideoFrame(overlay->hwdata->channel);



	if (overlay->hwdata->current == -1)
	{
		SDL_SetError("PgNextFrame failed, bailing out\n");
		SDL_FreeYUVOverlay(overlay);
		return(NULL);
	}
        
        //set current frame for double buffering
	if(overlay->hwdata->current == 0)
	{
		overlay->hwdata->CurrentFrameData = overlay->hwdata->FrameData0;
	}
	else
	{
		overlay->hwdata->CurrentFrameData = overlay->hwdata->FrameData1;
	}
    
	overlay->hwdata->State = OVERLAY_STATE_ACTIVE;


	/* We're all done.. */
	return(overlay);
}

int ph_LockYUVOverlay(_THIS, SDL_Overlay *overlay)
{
//int rtncode;

if(overlay == NULL)
   return 0;

//set current frame for double buffering
	if(overlay->hwdata->current == 0)
	{
		overlay->hwdata->CurrentFrameData = overlay->hwdata->FrameData0;
	}
	else
	{
		overlay->hwdata->CurrentFrameData = overlay->hwdata->FrameData1;
	}

	//Lock gets the pointer and passes it to the app. The app writes all yuv data into overlay->pixels
//Note this is defined as Uint8 **pixels;				/* Read-write */	
	overlay->pixels = &overlay->hwdata->CurrentFrameData->Y; 
	overlay->pitches  = &overlay->hwdata->YStride;
		
	return(0);
}

void ph_UnlockYUVOverlay(_THIS, SDL_Overlay *overlay)
{
int rtncode;

if(overlay == NULL)
   return ;

		if(overlay->hwdata->scaler_on == FALSE) 
		{
			
		
		overlay->hwdata->props.flags |= Pg_SCALER_PROP_SCALER_ENABLE;
        rtncode =PgConfigScalerChannel(overlay->hwdata->channel, &(overlay->hwdata->props));
        	switch(rtncode)
			{
				case -1:
					SDL_SetError("PgConfigScalerChannel failed\n");
					SDL_FreeYUVOverlay(overlay);
   					break;
				case 1:
					grab_ptrs2(overlay->hwdata->channel, overlay->hwdata->FrameData0, overlay->hwdata->FrameData1);
					overlay->hwdata->scaler_on = TRUE;
					break;
				case 0:
					default:
					overlay->hwdata->scaler_on = TRUE;
   					break;
			}
//This would be the best place to draw chromakey but we do not have a SDL_Surface in the args
//This means we might see a chromakey flicker at startup
		}
		overlay->hwdata->current = PgNextVideoFrame(overlay->hwdata->channel);


	if (overlay->hwdata->current == -1) {
		SDL_SetError("PgNextVideoFrame failed\n");
		SDL_FreeYUVOverlay(overlay);	
		return;	
	}

	overlay->pixels = NULL;
}

int ph_DisplayYUVOverlay(_THIS, SDL_Overlay *overlay, SDL_Rect *dstrect)
{
int rtncode;

if(overlay == NULL)
   return 0;


	/*SDL_Rect CurrentWindow*/
//If CurrentWindow has change, move the viewport
if((overlay->hwdata->CurrentWindow.x != dstrect->x) ||
  (overlay->hwdata->CurrentWindow.y != dstrect->y) ||
  (overlay->hwdata->CurrentWindow.w != dstrect->w) ||
  (overlay->hwdata->CurrentWindow.h != dstrect->h))
{
		if(overlay->hwdata->State == OVERLAY_STATE_UNINIT)
		return -1;

	overlay->hwdata->CurrentWindow.x = dstrect->x;
  	overlay->hwdata->CurrentWindow.y = dstrect->y;
  	overlay->hwdata->CurrentWindow.w = dstrect->w;
  	overlay->hwdata->CurrentWindow.h = dstrect->h;

	overlay->hwdata->props.viewport.ul.x = overlay->hwdata->CurrentWindow.x;
	overlay->hwdata->props.viewport.ul.y = overlay->hwdata->CurrentWindow.y;
	//Next line MIGHT have x and y reversed!!!!!!!!!!!!
	overlay->hwdata->props.viewport.lr.x = overlay->hwdata->CurrentWindow.x +overlay->hwdata->CurrentWindow.w;
	overlay->hwdata->props.viewport.lr.y = overlay->hwdata->CurrentWindow.y + overlay->hwdata->CurrentWindow.h;

	
     rtncode = PgConfigScalerChannel(overlay->hwdata->channel, &(overlay->hwdata->props));

	switch(rtncode)
	{
	case -1:
		SDL_SetError("PgConfigScalerChannel failed\n");
		SDL_FreeYUVOverlay(overlay);
		return(0);
   		break;
	case 1:
		grab_ptrs2(overlay->hwdata->channel, overlay->hwdata->FrameData0, overlay->hwdata->FrameData1);
		break;
	case 0:
	default:
   		break;
	}
}


//JB the X11 file did this. We do this in SDL_unlock, we need to confirm that lock and unlock are called for each frame!
//	XvShmPutImage(GFX_Display, hwdata->port, SDL_Window, SDL_GC,
//	              hwdata->image, 0, 0, overlay->w, overlay->h,
//	              dstrect->x, dstrect->y, dstrect->w, dstrect->h, False);	           
/*  This is what this call is
int XvShmPutImage (
   Display *dpy,
   XvPortID port,
   Drawable d,
   GC gc,
   XvImage *image,
   int src_x,
   int src_y,
   unsigned int src_w,
   unsigned int src_h,
   int dest_x, 
   int dest_y,
   unsigned int dest_w,
   unsigned int dest_h,
   Bool send_event
)
*/

	return(0);
}

void ph_FreeYUVOverlay(_THIS, SDL_Overlay *overlay)
{
	//struct private_yuvhwdata *hwdata;

	if(overlay == NULL)
		return;
	
	if(overlay->hwdata == NULL)
		return;
	
	overlay->hwdata->State = OVERLAY_STATE_UNINIT;

	if( overlay->hwdata->channel == NULL )
	{
		return;
	}	

	PgDestroyVideoChannel(overlay->hwdata->channel);

	overlay->hwdata->channel = NULL;
	overlay->hwdata->CurrentFrameData = NULL;  
	
	free(overlay->hwdata->FrameData0);
	free(overlay->hwdata->FrameData1);
	overlay->hwdata->FrameData0 = NULL;
	overlay->hwdata->FrameData1 = NULL;
	free(overlay->hwdata);

}
