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


#include "SDL_ph_modes_c.h"

static unsigned long key1, key2;
static PgVideoModeInfo_t mode_info;
static PgVideoModes_t mode_list;
  /* The current list of available video modes */
 SDL_Rect SDL_modelist[127];
 SDL_Rect *SDLmod_ptr;
 SDL_Rect **SDLmod_ptrptr ;

static int compare_modes_by_res(const void* mode1, const void* mode2)
{

	if (PgGetVideoModeInfo(*(unsigned short*)mode1, &mode_info) < 0)
	{
	    fprintf(stderr,"error: In compare_modes_by_res PgGetVideoModeInfo failed on mode: 0x%x\n",
        	    *(unsigned short*)mode1);
    	return 0;
	}
	key1 = mode_info.width * mode_info.height;

	if (PgGetVideoModeInfo(*(unsigned short*)mode2, &mode_info) < 0)
	{
    	fprintf(stderr,"error: In compare_modes_by_res PgGetVideoModeInfo failed on mode: 0x%x\n",
	            *(unsigned short*)mode2);
	    return 0;
	}
    key2 = mode_info.width * mode_info.height;

	if (key1 > key2)
		return 1;
	else if (key1 == key2)
		return 0;
	else
		return -1;
}

static int compare_modes_by_bpp(const void* mode1, const void* mode2)
{

    if (PgGetVideoModeInfo(*(unsigned short*)mode1, &mode_info) < 0)
    {
        fprintf(stderr,"error: In compare_modes_by_bpp PgGetVideoModeInfo failed on mode: 0x%x\n",
                *(unsigned short*)mode1);
        return 0;
    }
    key1 = mode_info.bits_per_pixel;

    if (PgGetVideoModeInfo(*(unsigned short*)mode2, &mode_info) < 0)
    {
        fprintf(stderr,"error: In compare_modes_by_bpp PgGetVideoModeInfo failed on mode: 0x%x\n",
                *(unsigned short*)mode2);
        return 0;
    }
    key2 = mode_info.bits_per_pixel;

    if (key1 > key2)
        return 1;
    else if (key1 == key2)
        return 0;
    else
        return -1;
}

int ph_GetVideoModes(_THIS)
{
	unsigned short *front;
	int i, bpp_group_size;

	// TODO: add mode_list member to _THIS
	if (PgGetVideoModeList( &mode_list ) < 0)
	{
		fprintf(stderr,"error: PgGetVideoModeList failed\n");
		return -1;
	}
	
	// sort list first by bits per pixel (bpp), 
	// then sort groups with same bpp by resolution.
	qsort(mode_list.modes, mode_list.num_modes, sizeof(unsigned short), compare_modes_by_bpp);
	bpp_group_size = 1;
	front = &mode_list.modes[0];
	for(i=0;i<mode_list.num_modes-2;i++)
	{
		if (compare_modes_by_bpp(&mode_list.modes[i],&mode_list.modes[i+1]))
		{
			qsort(front, bpp_group_size, sizeof(unsigned short), compare_modes_by_res);
			front = &mode_list.modes[i+1];
			bpp_group_size = 1;
		}
		else
		{
			bpp_group_size++;
		}
	}

	//SDL_modelist = (SDL_Rect **)malloc((mode_list.num_modes+1)*sizeof(SDL_Rect *));
	if ( SDL_modelist ) {
		for (i=0;i<mode_list.num_modes;i++) {
        //	SDL_modelist[i] = (SDL_Rect *)malloc(sizeof(SDL_Rect));
	     //   if ( SDL_modelist[i] == NULL ) {
    	  //      break;
	     //   }
		    if (PgGetVideoModeInfo(mode_list.modes[i], &mode_info) < 0)
		    {
        		fprintf(stderr,"error: PgGetVideoModeInfo failed on mode: 0x%x\n",
                		mode_list.modes[i]);
		        return -1;
    		}
    	    SDL_modelist[i].x = 0;
	        SDL_modelist[i].y = 0;
    	    SDL_modelist[i].w = mode_info.height;
        	SDL_modelist[i].h = mode_info.width;
	    }
    	//SDL_modelist[i] = NULL;
	}
	else
	{
		fprintf(stderr,"error: malloc failed on SDL_modelist\n");
		return -1;
	}
	
	return 0;
}

static SDL_Rect** ph_SupportedVisual( SDL_PixelFormat *format)
{
	int i = 0;
	int j = 0;
	SDL_Rect Amodelist[127];


	if (PgGetVideoModeList( &mode_list ) < 0)
    {
        fprintf(stderr,"error: PgGetVideoModeList failed\n");
        return NULL;
    }

	mode_info.bits_per_pixel = 0;

	for (i=0; i < mode_list.num_modes; i++) 
	{
            if (PgGetVideoModeInfo(mode_list.modes[i], &mode_info) < 0)
            {
                fprintf(stderr,"error: PgGetVideoModeInfo failed on mode: 0x%x\n",
                        mode_list.modes[i]);
                return NULL;
            }
            
            if(mode_info.bits_per_pixel == format->BitsPerPixel)
            {
				Amodelist[j].w = mode_info.width;
				Amodelist[j].h = mode_info.height;
				Amodelist[j].x = 0;
				Amodelist[j].y = 0;
				j++;	
			}
	}
	
	//reorder biggest for smallest , assume width dominates
	   for(i=0; i< j ; i++)
	   {
	     SDL_modelist[i].w = Amodelist[j - i -1].w;
	     SDL_modelist[i].h = Amodelist[j - i -1].h;
	     SDL_modelist[i].x = Amodelist[j - i -1].x;
	     SDL_modelist[i].y = Amodelist[j - i -1].y;
	   }
	
	SDLmod_ptr = SDL_modelist;
	SDLmod_ptrptr = &SDLmod_ptr;
	return SDLmod_ptrptr;
}

SDL_Rect **ph_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
    return ph_SupportedVisual( format);
}

void ph_FreeVideoModes(_THIS)
{
//    int i;

 //   if ( SDL_modelist ) {
 //       for ( i=0; SDL_modelist[i]; ++i ) {
 //           free(SDL_modelist[i]);
 //       }
 //       free(SDL_modelist);
 //       SDL_modelist = NULL;
//    }
}

static void set_best_resolution(_THIS, int width, int height)
{

    if ( use_vidmode ) {
		PgDisplaySettings_t 	settings;
		PgVideoModeInfo_t		current_mode_info;
		PgHWCaps_t my_hwcaps;
		unsigned short			current_bpp;
        int i;
	/*
		if (PgGetVideoMode( &settings ) < 0)
		{
			fprintf(stderr,"error: PgGetVideoMode failed\n");
			return;
		}
		if (PgGetVideoModeInfo( settings.mode, &current_mode_info ) < 0)
		{
			fprintf(stderr,"error: PgGetVideoModeInfo failed\n");
			return;
		}
		*/
		//lu_zero 
         if (PgGetGraphicsHWCaps(&my_hwcaps) < 0)
         	{
                fprintf(stderr,"set_best_resolution:  GetGraphicsHWCaps failed!! \n");
      			//that HAVE to work
            }
         if (PgGetVideoModeInfo(my_hwcaps.current_video_mode, &current_mode_info) < 0)
            {
                fprintf(stderr,"set_best_resolution:  PgGetVideoModeInfo failed\n");
            }
		current_bpp = current_mode_info.bits_per_pixel;

        if (PgGetVideoModeList(&mode_list) >= 0)
		{
			qsort(mode_list.modes, mode_list.num_modes, sizeof(unsigned short), compare_modes_by_res);
#ifdef PH_DEBUG
  			printf("Available modes:\n");
  			for ( i = 0; i < mode_list.num_modes; ++i ) 
			{
				PgGetVideoModeInfo(mode_list.modes[i], &mode_info);
    			printf("Mode %d: %dx%d\n", i, mode_info.width, mode_info.height);
  			}
#endif
            for ( i = mode_list.num_modes-1; i >= 0 ; --i ) 
			{
				if (PgGetVideoModeInfo(mode_list.modes[i], &mode_info) < 0)
				{
					fprintf(stderr,"error: PgGetVideoModeInfo failed\n");
				}
                if ( (mode_info.width >= width) &&
                     (mode_info.height >= height) &&
					 (mode_info.bits_per_pixel == current_bpp) )
                    break;
            }
			if (i >= 0)
			{
                if ( (mode_info.width != current_mode_info.width) ||
                     (mode_info.height != current_mode_info.height) ) 
				{
					settings.mode = mode_list.modes[i];
					if(PgSetVideoMode( &settings ) < 0)	
					{
						fprintf(stderr,"error: PgSetVideoMode failed\n");
					}
                }
            }
        }
    }
}

static void get_real_resolution(_THIS, int* w, int* h)
{

    if ( use_vidmode ) {
        //PgDisplaySettings_t settings;
	    PgVideoModeInfo_t		current_mode_info;
	    	PgHWCaps_t my_hwcaps;
        int unused;
		/*
        if (PgGetVideoMode( &settings ) >= 0) {
			*w = settings.xres;
			*h = settings.yres;
            return;
        }*/
            if (PgGetGraphicsHWCaps(&my_hwcaps) >= 0)
         	{
                 if (PgGetVideoModeInfo(my_hwcaps.current_video_mode, &current_mode_info) < 0)
           		 {
                fprintf(stderr,"get_real_resolution:  PgGetVideoModeInfo failed\n");
            		 }
				*w = current_mode_info.width;
				*h = current_mode_info.height;            
            }
    }
//    *w = DisplayWidth(SDL_Display, SDL_Screen);
//    *h = DisplayHeight(SDL_Display, SDL_Screen);
}

int ph_ResizeFullScreen(_THIS)
{

    if ( currently_fullscreen ) {
        set_best_resolution(this, current_w, current_h);
    }
    return(1);
}

int get_mode(int width, int height, int bpp)
/* return the mode associated with width, height and bpp */
/* if there is no mode then zero is returned */
{
	int i;


if(width <640)
   width = 640;
if(height < 480)
  height = 480;


	if (PgGetVideoModeList( &mode_list ) < 0)
	{
	    fprintf(stderr,"error: PgGetVideoModeList failed\n");
    	return -1;
	}

	// search list for exact match
	for (i=0;i<mode_list.num_modes;i++)
	{
		if (PgGetVideoModeInfo(mode_list.modes[i], &mode_info) < 0)
		{
			fprintf(stderr,"error: PgGetVideoModeInfo failed\n");
			return 0;
		}
		
		
		if ((mode_info.width == width) && 
			(mode_info.height == height) && 
			(mode_info.bits_per_pixel == bpp))
			{
			return mode_list.modes[i];
			}
	}
	return (i == mode_list.num_modes) ? 0 : mode_list.modes[i];
}

int get_mode_any_format(int width, int height, int bpp)
/* return the mode associated with width, height and bpp */
/* if requested bpp is not found the mode with closest bpp is returned */
{
    int i, closest, delta, min_delta;

	if (PgGetVideoModeList( &mode_list ) < 0)
	{
	    fprintf(stderr,"error: PgGetVideoModeList failed\n");
	    return -1;
	}

	qsort(mode_list.modes, mode_list.num_modes, sizeof(unsigned short), compare_modes_by_res);
	for(i=0;i<mode_list.num_modes;i++)
	{
       if (PgGetVideoModeInfo(mode_list.modes[i], &mode_info) < 0)
       {
           fprintf(stderr,"error: PgGetVideoModeInfo failed\n");
           return 0;
       }
       if ((mode_info.width == width) &&
           (mode_info.height == height))
           break;
	}
	if (i<mode_list.num_modes)
	{
		// get closest bpp
		closest = i++;
		if (mode_info.bits_per_pixel == bpp)
			return mode_list.modes[ closest ];

		min_delta = abs(mode_info.bits_per_pixel - bpp);
		while(1)
		{
			if (PgGetVideoModeInfo(mode_list.modes[i], &mode_info) < 0)
			{
			    fprintf(stderr,"error: PgGetVideoModeInfo failed\n");
			    return 0;
			}

			if ((mode_info.width != width) ||
				(mode_info.height != height))
				break;
			else if (mode_info.bits_per_pixel == bpp)
			{
				closest = i;
				break;
			}
			else
			{
				delta = abs(mode_info.bits_per_pixel - bpp);
				if (delta < min_delta)
				{
					closest = i;
					min_delta = delta;
				}
				i++;
			}
		}
		return mode_list.modes[ closest ];
	}
	else
		return 0;
}

void ph_WaitMapped(_THIS);
void ph_WaitUnmapped(_THIS);
void ph_QueueEnterFullScreen(_THIS);

int ph_ToggleFullScreen(_THIS, int on)
{

   if(currently_fullscreen)
        ph_LeaveFullScreen(this);
   else
        ph_EnterFullScreen(this);
      
   return 0;     

}

int ph_EnterFullScreen(_THIS)
{
	if ( ! currently_fullscreen ) 
	{

		if (old_video_mode==-1)
		{
			PgGetGraphicsHWCaps(&graphics_card_caps);
			old_video_mode=graphics_card_caps.current_video_mode;
			old_refresh_rate=graphics_card_caps.current_rrate;
		}


		if(OCImage.direct_context == NULL)
       	 OCImage.direct_context=(PdDirectContext_t*)PdCreateDirectContext();
        if( !OCImage.direct_context )
             fprintf(stderr, "error: Can't create direct context\n" );
             
       
		/* Remove the cursor if in full screen mode */		
/*
		region_info.cursor_type = Ph_CURSOR_NONE;
		region_info.rid=PtWidgetRid(window);
		PhRegionChange(Ph_REGION_CURSOR,0,&region_info,NULL,NULL);
*/

	 	PdDirectStart( OCImage.direct_context );

		currently_fullscreen = 1;
		}


		
	return 1;
}

int ph_LeaveFullScreen(_THIS )
{
   PgDisplaySettings_t mymode_settings;
       
	if ( currently_fullscreen )
	{
		PdDirectStop(OCImage.direct_context);
		PdReleaseDirectContext(OCImage.direct_context);

		//Restore old video mode	
		if (old_video_mode != -1)
		{
			mymode_settings.mode= (unsigned short) old_video_mode;
			mymode_settings.refresh= (unsigned short) old_refresh_rate;
			mymode_settings.flags = 0;
			if(PgSetVideoMode(&mymode_settings) < 0)
			{
            fprintf(stderr,"error: PgSetVideoMode failed\n");
        	}
		}
	
		old_video_mode=-1;
		old_refresh_rate=-1;	
	
		// Restore cursor	
	
	}
	return 1;
}
