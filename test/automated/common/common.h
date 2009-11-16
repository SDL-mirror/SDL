/**
 * Automated SDL test common framework.
 *
 * Written by Edgar Simo "bobbens"
 *
 * Released under Public Domain.
 */


#ifndef COMMON_H
#  define COMMON_H


#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
#  define FORMAT  SDL_PIXELFORMAT_RGBA8888
#  define RMASK   0xff000000 /**< Red bit mask. */
#  define GMASK   0x00ff0000 /**< Green bit mask. */
#  define BMASK   0x0000ff00 /**< Blue bit mask. */
#  define AMASK   0x000000ff /**< Alpha bit mask. */
#else
#  define FORMAT  SDL_PIXELFORMAT_ABGR8888
#  define RMASK   0x000000ff /**< Red bit mask. */
#  define GMASK   0x0000ff00 /**< Green bit mask. */
#  define BMASK   0x00ff0000 /**< Blue bit mask. */
#  define AMASK   0xff000000 /**< Alpha bit mask. */
#endif


typedef struct SurfaceImage_s {
   int width;
   int height;
   unsigned int  bytes_per_pixel; /* 3:RGB, 4:RGBA */ 
   const unsigned char pixel_data[];
} SurfaceImage_t;


/**
 * @brief Compares a surface and a surface image for equality.
 *
 *    @param sur Surface to compare.
 *    @param img Image to compare against.
 *    @return 0 if they are the same, -1 on error and positive if different.
 */
int surface_compare( SDL_Surface *sur, const SurfaceImage_t *img );


#endif /* COMMON_H */

