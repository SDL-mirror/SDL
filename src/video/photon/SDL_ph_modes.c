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

#include "SDL_ph_modes_c.h"

static unsigned long key1, key2;
static PgVideoModeInfo_t mode_info;
static PgVideoModes_t mode_list;

/* The current list of available video modes */
SDL_Rect  SDL_modelist[PH_MAX_VIDEOMODES];
SDL_Rect* SDL_modearray[PH_MAX_VIDEOMODES];

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

SDL_Rect **ph_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
    int i = 0;
    int j = 0;
    SDL_Rect Amodelist[PH_MAX_VIDEOMODES];

    for (i=0; i<PH_MAX_VIDEOMODES; i++)
    {
        SDL_modearray[i]=&SDL_modelist[i];
    }

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
            fprintf(stderr,"error: PgGetVideoModeInfo failed on mode: 0x%x\n", mode_list.modes[i]);
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
	
    /* reorder biggest for smallest, assume width dominates */

    for(i=0; i<j; i++)
    {
        SDL_modelist[i].w = Amodelist[j - i -1].w;
        SDL_modelist[i].h = Amodelist[j - i -1].h;
        SDL_modelist[i].x = Amodelist[j - i -1].x;
        SDL_modelist[i].y = Amodelist[j - i -1].y;
    }
    SDL_modearray[j]=NULL;
	
    return SDL_modearray;
}

void ph_FreeVideoModes(_THIS)
{
   return;
}

/* return the mode associated with width, height and bpp */
/* if there is no mode then zero is returned             */
int get_mode(int width, int height, int bpp)
{
    int i;

    if(width<640)
    {
        width=640;
    }
    if(height<480)
    {
        height=480;
    }

    if (PgGetVideoModeList(&mode_list) < 0)
    {
        fprintf(stderr,"error: PgGetVideoModeList failed\n");
        return -1;
    }

    /* search list for exact match */
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
		/* get closest bpp */
		closest = i++;
		if (mode_info.bits_per_pixel == bpp)
			return mode_list.modes[closest];

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
		return mode_list.modes[closest];
	}
	else
    return 0;
}

int ph_ToggleFullScreen(_THIS, int on)
{
    if (currently_fullscreen)
    {
        return ph_LeaveFullScreen(this);
    }
    else
    {
        return ph_EnterFullScreen(this);
    }
      
    return 0;     
}

int ph_EnterFullScreen(_THIS)
{
    if (!currently_fullscreen)
    {
        if (this->screen)
        {
            if ((this->screen->flags & SDL_OPENGL)==SDL_OPENGL)
            {
#ifdef HAVE_OPENGL
#endif /* HAVE_OPENGL */
                return 0;
            }
        }

        if (OCImage.direct_context==NULL)
        {
            OCImage.direct_context=(PdDirectContext_t*)PdCreateDirectContext();
        }

        if (!OCImage.direct_context)
        {
            fprintf(stderr, "ph_EnterFullScreen(): Can't create direct context !\n");
            return 0;
        }

        OCImage.oldDC=PdDirectStart(OCImage.direct_context);

        currently_fullscreen = 1;
    }

    return 1;
}

int ph_LeaveFullScreen(_THIS)
{
    PgDisplaySettings_t mymode_settings;
       
    if (currently_fullscreen)
    {
        if ((this->screen->flags & SDL_OPENGL)==SDL_OPENGL)
        {
#ifdef HAVE_OPENGL
#endif /* HAVE_OPENGL */
           return 0;
        }
        else
        {
            PdDirectStop(OCImage.direct_context);
            PdReleaseDirectContext(OCImage.direct_context);
            PhDCSetCurrent(OCImage.oldDC);

            currently_fullscreen=0;

            /* Restore old video mode */
            if (old_video_mode != -1)
            {
                mymode_settings.mode= (unsigned short) old_video_mode;
                mymode_settings.refresh= (unsigned short) old_refresh_rate;
                mymode_settings.flags= 0;
                
                if (PgSetVideoMode(&mymode_settings) < 0)
                {
                    fprintf(stderr, "Ph_LeaveFullScreen(): PgSetVideoMode failed !\n");
                    return 0;
                }
            }

            old_video_mode=-1;
            old_refresh_rate=-1;
        }

    }
    return 1;
}
