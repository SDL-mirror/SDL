/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga
    Copyright (C) 2001  Hsieh-Fu Tsai

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
    
    Hsieh-Fu Tsai
    clare@setabox.com
*/

#include <stdlib.h>

#include "SDL_error.h"

#include "SDL_nximage_c.h"

void NX_NormalUpdate (_THIS, int numrects, SDL_Rect * rects)
{
    int           i, j, xinc, yinc, destinc ;
    int           x, y, w, h ;
    unsigned char * src = NULL, * dest = NULL ;
            
    Dprintf ("enter NX_NormalUpdate\n") ;

    xinc = this -> screen -> format -> BytesPerPixel ;
    yinc = this -> screen -> pitch ;
        
    for (i = 0; i < numrects; ++ i) {
        x = rects [i].x, y = rects [i].y ;
        w = rects [i].w, h = rects [i].h ;
        src = SDL_Image + y * yinc + x * xinc ;
        dest = Image_buff ;
        destinc = w * xinc ;

        // apply GammaRamp table
#if (defined (NANOX_PIXEL_RGB) || defined (NANOX_PIXEL_0888) || \
     defined (NANOX_PIXEL_888))
        if (GammaRamp_R && GammaRamp_G && GammaRamp_B) {
            Uint8 * ptr ;
            int   k ;

            for (j = h; j > 0; -- j, src += yinc) {
                ptr = src - 1 ;
                for (k = w; k > 0; -- k) {
#ifdef NANOX_PIXEL_RGB
                    ptr += 2 ;
#endif
#ifdef NANOX_PIXEL_0888
                    ptr += 2 ;
#endif
#ifdef NANOX_PIXEL_888
                    ++ ptr ;
#endif
                    (* ptr) = GammaRamp_B [(* ptr)] ;
                    ++ ptr ;
                    (* ptr) = GammaRamp_G [(* ptr)] ;
                    ++ ptr ;
                    (* ptr) = GammaRamp_R [(* ptr)] ;
                }
            }
            src = SDL_Image + y * yinc + x * xinc ;
        }
#endif // apply Gamma table

        for (j = h; j > 0; -- j, src += yinc, dest += destinc) {
            memcpy (dest, src, destinc) ;
        }

        if (currently_fullscreen) {
            GrArea (FSwindow, SDL_GC, x + OffsetX, y + OffsetY, w, h, Image_buff, 
                pixel_type) ;
        } else {
            GrArea (SDL_Window, SDL_GC, x, y, w, h, Image_buff, pixel_type) ;
        }
    }

    Dprintf ("leave NX_NormalUpdate\n") ;
}

int NX_SetupImage (_THIS, SDL_Surface * screen)
{
    int size = screen -> h * screen -> pitch ;
    
    Dprintf ("enter NX_SetupImage\n") ;

    screen -> pixels = (void *) malloc (size) ;
    Image_buff = (unsigned char *) malloc (size) ;
    if (screen -> pixels == NULL || Image_buff == NULL) {
        free (screen -> pixels) ;
        free (Image_buff) ;
        SDL_OutOfMemory () ;
        return -1 ;
    }

    SDL_Image = (unsigned char *) screen -> pixels ;

    this -> UpdateRects = NX_NormalUpdate ;

    Dprintf ("leave NX_SetupImage\n") ;
    return 0 ;
}

void NX_DestroyImage (_THIS, SDL_Surface * screen)
{
    Dprintf ("enter NX_DestroyImage\n") ;
    
    if (SDL_Image) free (SDL_Image) ;
    if (Image_buff) free (Image_buff) ;
    if (screen) screen -> pixels = NULL ;
    
    Dprintf ("leave NX_DestroyImage\n") ;
}

int NX_ResizeImage (_THIS, SDL_Surface * screen, Uint32 flags)
{
    int            retval ;
    GR_SCREEN_INFO si ;

    Dprintf ("enter NX_ResizeImage\n") ;

    NX_DestroyImage (this, screen) ;
    retval = NX_SetupImage (this, screen) ;

    GrGetScreenInfo (& si) ;
    OffsetX = (si.cols - screen -> w) / 2 ;
    OffsetY = (si.rows - screen -> h) / 2 ;
    
    Dprintf ("leave NX_ResizeImage\n") ;
    return retval ;
}

void NX_RefreshDisplay (_THIS)
{
    Dprintf ("enter NX_RefreshDisplay\n") ;

    // Don't refresh a display that doesn't have an image (like GL)
    if (! SDL_Image) {
        return;
    }

    if (currently_fullscreen) {
        GrArea (FSwindow, SDL_GC, OffsetX, OffsetY, this -> screen -> w, 
            this -> screen -> h, SDL_Image, pixel_type) ;
    } else {
        GrArea (SDL_Window, SDL_GC, 0, 0, this -> screen -> w, 
            this -> screen -> h, SDL_Image, pixel_type) ;
    }

    Dprintf ("leave NX_RefreshDisplay\n") ;
}
