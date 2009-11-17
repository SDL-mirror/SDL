/**
 * Automated SDL_Surface test.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


#include "SDL.h"
#include "SDL_at.h"

#include "common/common.h"


/**
 * @brief Compares a surface and a surface image for equality.
 */
int surface_compare( SDL_Surface *sur, const SurfaceImage_t *img )
{
   int ret;
   int i,j;
   int bpp;
   Uint8 *p, *pd;

   /* Make sure size is the same. */
   if ((sur->w != img->width) || (sur->h != img->height))
      return -1;

   SDL_LockSurface( sur );

   ret = 0;
   bpp = sur->format->BytesPerPixel;

   /* Compare image - should be same format. */
   for (j=0; j<sur->h; j++) {
      for (i=0; i<sur->w; i++) {
         p  = (Uint8 *)sur->pixels + j * sur->pitch + i * bpp;
         pd = (Uint8 *)img->pixel_data + (j*img->width + i) * img->bytes_per_pixel;
         switch (bpp) {
            case 1:
            case 2:
            case 3:
               ret += 1;
               printf("%d BPP not supported yet.\n",bpp);
               break;

            case 4:
               {
                  int fail;
                  Uint8 R, G, B, A;

                  SDL_GetRGBA(*(Uint32*)p, sur->format, &R, &G, &B, &A);

                  if (img->bytes_per_pixel == 3) {
                     fail = !( (R == pd[0]) &&
                               (G == pd[1]) &&
                               (B == pd[2]) );
                  } else {
                     fail = !( (R == pd[0]) &&
                               (G == pd[1]) &&
                               (B == pd[2]) &&
                               (A == pd[3]) );
                  }
                  if (fail) {
                     ++ret;
                  }
               }
               break;
         }
      }
   }

   SDL_UnlockSurface( sur );

   return ret;
}
