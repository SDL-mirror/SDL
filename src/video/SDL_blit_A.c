/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

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

#include <stdio.h>

#include "SDL_types.h"
#include "SDL_video.h"
#include "SDL_blit.h"

#if (defined(i386) || defined(__x86_64__)) && defined(__GNUC__) && defined(USE_ASMBLIT)
#define MMX_ASMBLIT
#endif

#ifdef MMX_ASMBLIT
/* Function to check the CPU flags */
#include "SDL_cpuinfo.h"
#include "mmx.h"
#endif

/* Functions to perform alpha blended blitting */

/* N->1 blending with per-surface alpha */
static void BlitNto1SurfaceAlpha(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint8 *dst = info->d_pixels;
	int dstskip = info->d_skip;
	Uint8 *palmap = info->table;
	SDL_PixelFormat *srcfmt = info->src;
	SDL_PixelFormat *dstfmt = info->dst;
	int srcbpp = srcfmt->BytesPerPixel;

	const unsigned A = srcfmt->alpha;

	while ( height-- ) {
	    DUFFS_LOOP4(
	    {
		Uint32 pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		DISEMBLE_RGB(src, srcbpp, srcfmt, pixel, sR, sG, sB);
		dR = dstfmt->palette->colors[*dst].r;
		dG = dstfmt->palette->colors[*dst].g;
		dB = dstfmt->palette->colors[*dst].b;
		ALPHA_BLEND(sR, sG, sB, A, dR, dG, dB);
		dR &= 0xff;
		dG &= 0xff;
		dB &= 0xff;
		/* Pack RGB into 8bit pixel */
		if ( palmap == NULL ) {
		    *dst =((dR>>5)<<(3+2))|
			  ((dG>>5)<<(2))|
			  ((dB>>6)<<(0));
		} else {
		    *dst = palmap[((dR>>5)<<(3+2))|
				  ((dG>>5)<<(2))  |
				  ((dB>>6)<<(0))];
		}
		dst++;
		src += srcbpp;
	    },
	    width);
	    src += srcskip;
	    dst += dstskip;
	}
}

/* N->1 blending with pixel alpha */
static void BlitNto1PixelAlpha(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint8 *dst = info->d_pixels;
	int dstskip = info->d_skip;
	Uint8 *palmap = info->table;
	SDL_PixelFormat *srcfmt = info->src;
	SDL_PixelFormat *dstfmt = info->dst;
	int srcbpp = srcfmt->BytesPerPixel;

	/* FIXME: fix alpha bit field expansion here too? */
	while ( height-- ) {
	    DUFFS_LOOP4(
	    {
		Uint32 pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned sA;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		DISEMBLE_RGBA(src,srcbpp,srcfmt,pixel,sR,sG,sB,sA);
		dR = dstfmt->palette->colors[*dst].r;
		dG = dstfmt->palette->colors[*dst].g;
		dB = dstfmt->palette->colors[*dst].b;
		ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB);
		dR &= 0xff;
		dG &= 0xff;
		dB &= 0xff;
		/* Pack RGB into 8bit pixel */
		if ( palmap == NULL ) {
		    *dst =((dR>>5)<<(3+2))|
			  ((dG>>5)<<(2))|
			  ((dB>>6)<<(0));
		} else {
		    *dst = palmap[((dR>>5)<<(3+2))|
				  ((dG>>5)<<(2))  |
				  ((dB>>6)<<(0))  ];
		}
		dst++;
		src += srcbpp;
	    },
	    width);
	    src += srcskip;
	    dst += dstskip;
	}
}

/* colorkeyed N->1 blending with per-surface alpha */
static void BlitNto1SurfaceAlphaKey(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint8 *dst = info->d_pixels;
	int dstskip = info->d_skip;
	Uint8 *palmap = info->table;
	SDL_PixelFormat *srcfmt = info->src;
	SDL_PixelFormat *dstfmt = info->dst;
	int srcbpp = srcfmt->BytesPerPixel;
	Uint32 ckey = srcfmt->colorkey;

	const int A = srcfmt->alpha;

	while ( height-- ) {
	    DUFFS_LOOP(
	    {
		Uint32 pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		DISEMBLE_RGB(src, srcbpp, srcfmt, pixel, sR, sG, sB);
		if ( pixel != ckey ) {
		    dR = dstfmt->palette->colors[*dst].r;
		    dG = dstfmt->palette->colors[*dst].g;
		    dB = dstfmt->palette->colors[*dst].b;
		    ALPHA_BLEND(sR, sG, sB, A, dR, dG, dB);
		    dR &= 0xff;
		    dG &= 0xff;
		    dB &= 0xff;
		    /* Pack RGB into 8bit pixel */
		    if ( palmap == NULL ) {
			*dst =((dR>>5)<<(3+2))|
			      ((dG>>5)<<(2)) |
			      ((dB>>6)<<(0));
		    } else {
			*dst = palmap[((dR>>5)<<(3+2))|
				      ((dG>>5)<<(2))  |
				      ((dB>>6)<<(0))  ];
		    }
		}
		dst++;
		src += srcbpp;
	    },
	    width);
	    src += srcskip;
	    dst += dstskip;
	}
}

#ifdef MMX_ASMBLIT
/* fast RGB888->(A)RGB888 blending with surface alpha=128 special case */
static void BlitRGBtoRGBSurfaceAlpha128MMX(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint32 *srcp = (Uint32 *)info->s_pixels;
	int srcskip = info->s_skip >> 2;
	Uint32 *dstp = (Uint32 *)info->d_pixels;
	int dstskip = info->d_skip >> 2;
        Uint8 load[8];
  
        *(Uint64 *)load = 0x00fefefe00fefefeULL;/* alpha128 mask */
        movq_m2r(*load, mm4); /* alpha128 mask -> mm4 */
        *(Uint64 *)load = 0x0001010100010101ULL;/* !alpha128 mask */
        movq_m2r(*load, mm3); /* !alpha128 mask -> mm3 */
        *(Uint64 *)load = 0xFF000000FF000000ULL;/* dst alpha mask */
        movq_m2r(*load, mm7); /* dst alpha mask -> mm7 */
	while(height--) {
            DUFFS_LOOP_DOUBLE2(
            {
		    Uint32 s = *srcp++;
		    Uint32 d = *dstp;
		    *dstp++ = ((((s & 0x00fefefe) + (d & 0x00fefefe)) >> 1)
			       + (s & d & 0x00010101)) | 0xff000000;
            },{
	            movq_m2r((*dstp), mm2);/* 2 x dst -> mm2(ARGBARGB) */
	            movq_r2r(mm2, mm6); /* 2 x dst -> mm6(ARGBARGB) */
	      
	            movq_m2r((*srcp), mm1);/* 2 x src -> mm1(ARGBARGB) */
	            movq_r2r(mm1, mm5); /* 2 x src -> mm5(ARGBARGB) */
		
	            pand_r2r(mm4, mm6); /* dst & mask -> mm6 */
	            pand_r2r(mm4, mm5); /* src & mask -> mm5 */
	            paddd_r2r(mm6, mm5); /* mm6 + mm5 -> mm5 */
	            psrld_i2r(1, mm5); /* mm5 >> 1 -> mm5 */
	
	            pand_r2r(mm1, mm2); /* src & dst -> mm2 */
	            pand_r2r(mm3, mm2); /* mm2 & !mask -> mm2 */
	            paddd_r2r(mm5, mm2); /* mm5 + mm2 -> mm2 */
	            por_r2r(mm7, mm2); /* mm7(full alpha) | mm2 -> mm2 */
	            movq_r2m(mm2, (*dstp));/* mm2 -> 2 x dst pixels */
	            dstp += 2;
	            srcp += 2;
            }, width);
	    srcp += srcskip;
	    dstp += dstskip;
	}
	emms();
}

/* fast RGB888->(A)RGB888 blending with surface alpha */
static void BlitRGBtoRGBSurfaceAlphaMMX(SDL_BlitInfo *info)
{
	unsigned alpha = info->src->alpha;
	if(alpha == 128) {
		BlitRGBtoRGBSurfaceAlpha128MMX(info);
	} else {
		int width = info->d_width;
		int height = info->d_height;
		Uint32 *srcp = (Uint32 *)info->s_pixels;
		int srcskip = info->s_skip >> 2;
		Uint32 *dstp = (Uint32 *)info->d_pixels;
		int dstskip = info->d_skip >> 2;
                Uint8 load[8] = {alpha, alpha, alpha, alpha,
    					alpha, alpha, alpha, alpha};
					
                movq_m2r(*load, mm4); /* alpha -> mm4 */
		*(Uint64 *)load = 0x00FF00FF00FF00FFULL;
                movq_m2r(*load, mm3); /* mask -> mm3 */
		pand_r2r(mm3, mm4); /* mm4 & mask -> 0A0A0A0A -> mm4 */
		*(Uint64 *)load = 0xFF000000FF000000ULL;/* dst alpha mask */
		movq_m2r(*load, mm7); /* dst alpha mask -> mm7 */
		
		while(height--) {
			DUFFS_LOOP_DOUBLE2({
				/* One Pixel Blend */
	                        movd_m2r((*srcp), mm1);/* src(ARGB) -> mm1 (0000ARGB)*/
                                punpcklbw_r2r(mm1, mm1); /* AARRGGBB -> mm1 */
                                pand_r2r(mm3, mm1); /* 0A0R0G0B -> mm1 */
			  
	                        movd_m2r((*dstp), mm2);/* dst(ARGB) -> mm2 (0000ARGB)*/
			        movq_r2r(mm2, mm6);/* dst(ARGB) -> mm6 (0000ARGB)*/
                                punpcklbw_r2r(mm2, mm2); /* AARRGGBB -> mm2 */
                                pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
			  
                                psubw_r2r(mm2, mm1);/* src - dst -> mm1 */
	                        pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	                        psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	                        paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	                        pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	                        packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	                        por_r2r(mm7, mm2); /* mm7(full alpha) | mm2 -> mm2 */
			        movd_r2m(mm2, *dstp);/* mm2 -> pixel */
				++srcp;
				++dstp;
			},{
			        /* Two Pixels Blend */
				movq_m2r((*srcp), mm0);/* 2 x src -> mm0(ARGBARGB)*/
			        movq_r2r(mm0, mm1); /* 2 x src -> mm1(ARGBARGB) */
                                punpcklbw_r2r(mm0, mm0); /* low - AARRGGBB -> mm0 */
			        pand_r2r(mm3, mm0); /* 0A0R0G0B -> mm0(src1) */
			        punpckhbw_r2r(mm1, mm1); /* high - AARRGGBB -> mm1 */
	                        pand_r2r(mm3, mm1); /* 0A0R0G0B -> mm1(src2) */
	
	                        movq_m2r((*dstp), mm2);/* 2 x dst -> mm2(ARGBARGB) */
	                        movq_r2r(mm2, mm5); /* 2 x dst -> mm5(ARGBARGB) */
			        movq_r2r(mm2, mm6); /* 2 x dst -> mm6(ARGBARGB) */
                                punpcklbw_r2r(mm2, mm2); /* low - AARRGGBB -> mm2 */
	                        punpckhbw_r2r(mm6, mm6); /* high - AARRGGBB -> mm6 */
                                pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2(dst1) */
	                  
                                psubw_r2r(mm2, mm0);/* src1 - dst1 -> mm0 */
	                        pmullw_r2r(mm4, mm0); /* mm0 * alpha -> mm0 */
			        pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6(dst2) */
			        psrlw_i2r(8, mm0); /* mm0 >> 8 -> mm1 */
			        psubw_r2r(mm6, mm1);/* src2 - dst2 -> mm1 */
	                        pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
				paddw_r2r(mm0, mm2); /* mm0 + mm2(dst1) -> mm2 */
	                        psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm0 */
				pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
	                        paddw_r2r(mm1, mm6); /* mm1 + mm6(dst2) -> mm6 */
	                        pand_r2r(mm3, mm6); /* 0A0R0G0B -> mm6 */
	                        packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
	                        packuswb_r2r(mm6, mm6);  /* ARGBARGB -> mm6 */
	                        psrlq_i2r(32, mm2); /* mm2 >> 32 -> mm2 */
	                        psllq_i2r(32, mm6); /* mm6 << 32 -> mm6 */
	                        por_r2r(mm6, mm2); /* mm6 | mm2 -> mm2 */				
				por_r2r(mm7, mm2); /* mm7(full alpha) | mm2 -> mm2 */
                                movq_r2m(mm2, *dstp);/* mm2 -> 2 x pixel */
				srcp += 2;
				dstp += 2;
			}, width);
			srcp += srcskip;
			dstp += dstskip;
		}
		emms();
	}
}

/* fast ARGB888->(A)RGB888 blending with pixel alpha */
static void BlitRGBtoRGBPixelAlphaMMX(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint32 *srcp = (Uint32 *)info->s_pixels;
	int srcskip = info->s_skip >> 2;
	Uint32 *dstp = (Uint32 *)info->d_pixels;
	int dstskip = info->d_skip >> 2;
        Uint32 alpha = 0;
        Uint8 load[8];
	                
	*(Uint64 *)load = 0x00FF00FF00FF00FFULL;
        movq_m2r(*load, mm3); /* mask -> mm2 */
	*(Uint64 *)load = 0x00FF000000000000ULL;
        movq_m2r(*load, mm7); /* dst alpha mask -> mm2 */
        *(Uint64 *)load = 0x00FFFFFF00FFFFFFULL;
        movq_m2r(*load, mm0); /* alpha 255 mask -> mm0 */
        *(Uint64 *)load = 0xFF000000FF000000ULL;
        movq_m2r(*load, mm6); /* alpha 255 !mask -> mm6 */
	while(height--) {
	    DUFFS_LOOP4({
	        alpha = *srcp;
	        alpha >>= 24;
		/* FIXME: Here we special-case opaque alpha since the
		   compositioning used (>>8 instead of /255) doesn't handle
		   it correctly. Also special-case alpha=0 for speed?
		   Benchmark this! */
		if(alpha) {   
		  if(alpha == SDL_ALPHA_OPAQUE) {
		    movd_m2r((*srcp), mm1);/* src(ARGB) -> mm1 (0000ARGB)*/
		    movd_m2r((*dstp), mm2);/* dst(ARGB) -> mm2 (0000ARGB)*/
		    pand_r2r(mm0, mm1);
		    pand_r2r(mm6, mm2);
		    por_r2r(mm1, mm2);
		    movd_r2m(mm2, (*dstp));
		  } else {
		    movd_m2r((*srcp), mm1);/* src(ARGB) -> mm1 (0000ARGB)*/
                    punpcklbw_r2r(mm1, mm1); /* AARRGGBB -> mm1 */
                    pand_r2r(mm3, mm1); /* 0A0R0G0B -> mm1 */
			  
	            movd_m2r((*dstp), mm2);/* dst(ARGB) -> mm2 (0000ARGB)*/
                    punpcklbw_r2r(mm2, mm2); /* AARRGGBB -> mm2 */
                    pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
		
		    movq_r2r(mm2, mm5);/* mm2(0A0R0G0B) -> mm5 */
		    pand_r2r(mm7, mm5); /* mm5 & dst alpha mask -> mm5(0A000000) */
		    psrlq_i2r(24, mm5); /* mm5 >> 24 -> mm5 (0000A000)*/
		    
		    movq_r2r(mm1, mm4);/* mm1(0A0R0G0B) -> mm4 */
		    psrlq_i2r(48, mm4); /* mm4 >> 48 -> mm4(0000000A) */
		    punpcklwd_r2r(mm4, mm4); /* 00000A0A -> mm4 */
                    punpcklwd_r2r(mm4, mm4); /* 0A0A0A0A -> mm4 */
		                        		    
                    /* blend */		    
                    psubw_r2r(mm2, mm1);/* src - dst -> mm1 */
	            pmullw_r2r(mm4, mm1); /* mm1 * alpha -> mm1 */
	            psrlw_i2r(8, mm1); /* mm1 >> 8 -> mm1 */
	            paddw_r2r(mm1, mm2); /* mm1 + mm2(dst) -> mm2 */
	            pand_r2r(mm3, mm2); /* 0A0R0G0B -> mm2 */
		    packuswb_r2r(mm2, mm2);  /* ARGBARGB -> mm2 */
		    pand_r2r(mm0, mm2); /* 0RGB0RGB -> mm2 */
		    por_r2r(mm5, mm2); /* dst alpha | mm2 -> mm2 */
		    movd_r2m(mm2, *dstp);/* mm2 -> dst */
		  }
		}
		++srcp;
		++dstp;
	    }, width);
	    srcp += srcskip;
	    dstp += dstskip;
	}
	emms();
}
#endif

/* fast RGB888->(A)RGB888 blending with surface alpha=128 special case */
static void BlitRGBtoRGBSurfaceAlpha128(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint32 *srcp = (Uint32 *)info->s_pixels;
	int srcskip = info->s_skip >> 2;
	Uint32 *dstp = (Uint32 *)info->d_pixels;
	int dstskip = info->d_skip >> 2;

	while(height--) {
	    DUFFS_LOOP4({
		    Uint32 s = *srcp++;
		    Uint32 d = *dstp;
		    *dstp++ = ((((s & 0x00fefefe) + (d & 0x00fefefe)) >> 1)
			       + (s & d & 0x00010101)) | 0xff000000;
	    }, width);
	    srcp += srcskip;
	    dstp += dstskip;
	}
}

/* fast RGB888->(A)RGB888 blending with surface alpha */
static void BlitRGBtoRGBSurfaceAlpha(SDL_BlitInfo *info)
{
	unsigned alpha = info->src->alpha;
	if(alpha == 128) {
		BlitRGBtoRGBSurfaceAlpha128(info);
	} else {
		int width = info->d_width;
		int height = info->d_height;
		Uint32 *srcp = (Uint32 *)info->s_pixels;
		int srcskip = info->s_skip >> 2;
		Uint32 *dstp = (Uint32 *)info->d_pixels;
		int dstskip = info->d_skip >> 2;
		Uint32 s;
		Uint32 d;
		Uint32 s1;
		Uint32 d1;

		while(height--) {
			DUFFS_LOOP_DOUBLE2({
				/* One Pixel Blend */
				s = *srcp;
				d = *dstp;
				s1 = s & 0xff00ff;
				d1 = d & 0xff00ff;
				d1 = (d1 + ((s1 - d1) * alpha >> 8))
				     & 0xff00ff;
				s &= 0xff00;
				d &= 0xff00;
				d = (d + ((s - d) * alpha >> 8)) & 0xff00;
				*dstp = d1 | d | 0xff000000;
				++srcp;
				++dstp;
			},{
			        /* Two Pixels Blend */
				s = *srcp;
				d = *dstp;
				s1 = s & 0xff00ff;
				d1 = d & 0xff00ff;
				d1 += (s1 - d1) * alpha >> 8;
				d1 &= 0xff00ff;
				     
				s = ((s & 0xff00) >> 8) | 
					((srcp[1] & 0xff00) << 8);
				d = ((d & 0xff00) >> 8) |
					((dstp[1] & 0xff00) << 8);
				d += (s - d) * alpha >> 8;
				d &= 0x00ff00ff;
				
				*dstp++ = d1 | ((d << 8) & 0xff00) | 0xff000000;
				++srcp;
				
			        s1 = *srcp;
				d1 = *dstp;
				s1 &= 0xff00ff;
				d1 &= 0xff00ff;
				d1 += (s1 - d1) * alpha >> 8;
				d1 &= 0xff00ff;
				
				*dstp = d1 | ((d >> 8) & 0xff00) | 0xff000000;
				++srcp;
				++dstp;
			}, width);
			srcp += srcskip;
			dstp += dstskip;
		}
	}
}

/* fast ARGB888->(A)RGB888 blending with pixel alpha */
static void BlitRGBtoRGBPixelAlpha(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint32 *srcp = (Uint32 *)info->s_pixels;
	int srcskip = info->s_skip >> 2;
	Uint32 *dstp = (Uint32 *)info->d_pixels;
	int dstskip = info->d_skip >> 2;

	while(height--) {
	    DUFFS_LOOP4({
		Uint32 dalpha;
		Uint32 d;
		Uint32 s1;
		Uint32 d1;
		Uint32 s = *srcp;
		Uint32 alpha = s >> 24;
		/* FIXME: Here we special-case opaque alpha since the
		   compositioning used (>>8 instead of /255) doesn't handle
		   it correctly. Also special-case alpha=0 for speed?
		   Benchmark this! */
		if(alpha) {   
		  if(alpha == SDL_ALPHA_OPAQUE) {
		    *dstp = (s & 0x00ffffff) | (*dstp & 0xff000000);
		  } else {
		    /*
		     * take out the middle component (green), and process
		     * the other two in parallel. One multiply less.
		     */
		    d = *dstp;
		    dalpha = d & 0xff000000;
		    s1 = s & 0xff00ff;
		    d1 = d & 0xff00ff;
		    d1 = (d1 + ((s1 - d1) * alpha >> 8)) & 0xff00ff;
		    s &= 0xff00;
		    d &= 0xff00;
		    d = (d + ((s - d) * alpha >> 8)) & 0xff00;
		    *dstp = d1 | d | dalpha;
		  }
		}
		++srcp;
		++dstp;
	    }, width);
	    srcp += srcskip;
	    dstp += dstskip;
	}
}

#ifdef MMX_ASMBLIT
/* fast (as in MMX with prefetch) ARGB888->(A)RGB888 blending with pixel alpha */
inline static void BlitRGBtoRGBPixelAlphaMMX3DNOW(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint32 *srcp = (Uint32 *)info->s_pixels;
	int srcskip = info->s_skip >> 2;
	Uint32 *dstp = (Uint32 *)info->d_pixels;
	int dstskip = info->d_skip >> 2;

	Uint32 s;
	Uint32 alpha;

	__asm__ (
	/* make mm6 all zeros. */
	"pxor       %%mm6, %%mm6\n"
	
	/* Make a mask to preserve the alpha. */
	"pcmpeqb   %%mm7, %%mm7\n\t"            /* mm7(s) = FF FF FF FF | FF FF FF FF */
	"psrlq     $16, %%mm7\n\t"		    /* mm7(s) = 00 00 FF FF | FF FF FF FF */

		: );

	while(height--) {

	    DUFFS_LOOP4({

		__asm__ (
		"prefetch 64(%0)\n"
		"prefetch 64(%1)\n"
			: : "r" (srcp), "r" (dstp) );

		s = *srcp;
		alpha = s >> 24;
		/* FIXME: Here we special-case opaque alpha since the
		   compositioning used (>>8 instead of /255) doesn't handle
		   it correctly. Also special-case alpha=0 for speed?
		   Benchmark this! */
		
		if(alpha == SDL_ALPHA_OPAQUE) {
		    *dstp = (s & 0x00ffffff) | (*dstp & 0xff000000);
		} 

		else {
			    __asm__ (
		    /* load in the source, and dst. */
		    "movd      (%0), %%mm0\n"		    /* mm0(s) = 0 0 0 0 | As Rs Gs Bs */
		    "movd      (%1), %%mm1\n"		    /* mm1(d) = 0 0 0 0 | Ad Rd Gd Bd */

		    /* Move the src alpha into mm2 */

		    /* if supporting pshufw */
		    /*"pshufw     $0x55, %%mm0, %%mm2\n" */ /* mm2 = 0 As 0 As |  0 As  0  As */
		    /*"psrlw     $8, %%mm2\n" */
		    
		    /* else: */
		    "movq      %%mm0, %%mm2\n"
		    "psrld     $24, %%mm2\n"                /* mm2 = 0 0 0 0 | 0  0  0  As */
		    "punpcklwd	%%mm2, %%mm2\n"	            /* mm2 = 0 0 0 0 |  0 As  0  As */
		    "punpckldq	%%mm2, %%mm2\n"             /* mm2 = 0 As 0 As |  0 As  0  As */

		    /* move the colors into words. */
		    "punpcklbw %%mm6, %%mm0\n"		    /* mm0 = 0 As 0 Rs | 0 Gs 0 Bs */
		    "punpcklbw %%mm6, %%mm1\n"              /* mm0 = 0 Ad 0 Rd | 0 Gd 0 Bd */

		    /* src - dst */
		    "psubw    %%mm1, %%mm0\n"		    /* mm0 = As-Ad Rs-Rd | Gs-Gd  Bs-Bd */

		    /* A * (src-dst) */
		    "pmullw    %%mm2, %%mm0\n"		    /* mm0 = As*As-d As*Rs-d | As*Gs-d  As*Bs-d */
		    "pand      %%mm7, %%mm0\n"              /* to preserve dest alpha */
		    "psrlw     $8,    %%mm0\n"		    /* mm0 = Ac>>8 Rc>>8 | Gc>>8  Bc>>8 */
		    "paddb     %%mm1, %%mm0\n"		    /* mm0 = Ac+Ad Rc+Rd | Gc+Gd  Bc+Bd */

		    "packuswb  %%mm0, %%mm0\n"              /* mm0 =             | Ac Rc Gc Bc */
		    
		    "movd      %%mm0, (%1)\n"               /* result in mm0 */

		     : : "r" (srcp), "r" (dstp) );

		}
		++srcp;
		++dstp;
	    }, width);
	    srcp += srcskip;
	    dstp += dstskip;
	}

	__asm__ (
	"emms\n"
		:   );
}
#endif

/* 16bpp special case for per-surface alpha=50%: blend 2 pixels in parallel */

/* blend a single 16 bit pixel at 50% */
#define BLEND16_50(d, s, mask)						\
	((((s & mask) + (d & mask)) >> 1) + (s & d & (~mask & 0xffff)))

/* blend two 16 bit pixels at 50% */
#define BLEND2x16_50(d, s, mask)					     \
	(((s & (mask | mask << 16)) >> 1) + ((d & (mask | mask << 16)) >> 1) \
	 + (s & d & (~(mask | mask << 16))))

static void Blit16to16SurfaceAlpha128(SDL_BlitInfo *info, Uint16 mask)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint16 *srcp = (Uint16 *)info->s_pixels;
	int srcskip = info->s_skip >> 1;
	Uint16 *dstp = (Uint16 *)info->d_pixels;
	int dstskip = info->d_skip >> 1;

	while(height--) {
		if(((unsigned long)srcp ^ (unsigned long)dstp) & 2) {
			/*
			 * Source and destination not aligned, pipeline it.
			 * This is mostly a win for big blits but no loss for
			 * small ones
			 */
			Uint32 prev_sw;
			int w = width;

			/* handle odd destination */
			if((unsigned long)dstp & 2) {
				Uint16 d = *dstp, s = *srcp;
				*dstp = BLEND16_50(d, s, mask);
				dstp++;
				srcp++;
				w--;
			}
			srcp++;	/* srcp is now 32-bit aligned */

			/* bootstrap pipeline with first halfword */
			prev_sw = ((Uint32 *)srcp)[-1];

			while(w > 1) {
				Uint32 sw, dw, s;
				sw = *(Uint32 *)srcp;
				dw = *(Uint32 *)dstp;
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
					s = (prev_sw << 16) + (sw >> 16);
				else
					s = (prev_sw >> 16) + (sw << 16);
				prev_sw = sw;
				*(Uint32 *)dstp = BLEND2x16_50(dw, s, mask);
				dstp += 2;
				srcp += 2;
				w -= 2;
			}

			/* final pixel if any */
			if(w) {
				Uint16 d = *dstp, s;
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
					s = prev_sw;
				else
					s = prev_sw >> 16;
				*dstp = BLEND16_50(d, s, mask);
				srcp++;
				dstp++;
			}
			srcp += srcskip - 1;
			dstp += dstskip;
		} else {
			/* source and destination are aligned */
			int w = width;

			/* first odd pixel? */
			if((unsigned long)srcp & 2) {
				Uint16 d = *dstp, s = *srcp;
				*dstp = BLEND16_50(d, s, mask);
				srcp++;
				dstp++;
				w--;
			}
			/* srcp and dstp are now 32-bit aligned */

			while(w > 1) {
				Uint32 sw = *(Uint32 *)srcp;
				Uint32 dw = *(Uint32 *)dstp;
				*(Uint32 *)dstp = BLEND2x16_50(dw, sw, mask);
				srcp += 2;
				dstp += 2;
				w -= 2;
			}

			/* last odd pixel? */
			if(w) {
				Uint16 d = *dstp, s = *srcp;
				*dstp = BLEND16_50(d, s, mask);
				srcp++;
				dstp++;
			}
			srcp += srcskip;
			dstp += dstskip;
		}
	}
}

#ifdef MMX_ASMBLIT
/* fast RGB565->RGB565 blending with surface alpha */
static void Blit565to565SurfaceAlphaMMX(SDL_BlitInfo *info)
{
	unsigned alpha = info->src->alpha; /* downscale alpha to 5 bits */
	if(alpha == 128) {
		Blit16to16SurfaceAlpha128(info, 0xf7de);
	} else {
		int width = info->d_width;
		int height = info->d_height;
		Uint16 *srcp = (Uint16 *)info->s_pixels;
		int srcskip = info->s_skip >> 1;
		Uint16 *dstp = (Uint16 *)info->d_pixels;
		int dstskip = info->d_skip >> 1;
	        Uint32 s, d;
	        Uint8 load[8];
	  
		alpha &= ~(1+2+4);		/* cut alpha to get the exact same behaviour */
	        *(Uint64 *)load = alpha;
		alpha >>= 3;		/* downscale alpha to 5 bits */

                movq_m2r(*load, mm0); /* alpha(0000000A) -> mm0 */
                punpcklwd_r2r(mm0, mm0); /* 00000A0A -> mm0 */
                punpcklwd_r2r(mm0, mm0); /* 0A0A0A0A -> mm0 */
	  
 	        /* Setup the 565 color channel masks */
	        *(Uint64 *)load = 0xF800F800F800F800ULL;
		movq_m2r(*load, mm1); /* MASKRED -> mm1 */
		*(Uint64 *)load = 0x07E007E007E007E0ULL;
		movq_m2r(*load, mm4); /* MASKGREEN -> mm4 */
		*(Uint64 *)load = 0x001F001F001F001FULL;
		movq_m2r(*load, mm7); /* MASKBLUE -> mm7 */
		while(height--) {
                        DUFFS_LOOP_QUATRO2(
                        {
	                        s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x07e0f81f;
				d = (d | d << 16) & 0x07e0f81f;
				d += (s - d) * alpha >> 5;
				d &= 0x07e0f81f;
				*dstp++ = d | d >> 16;
                        },{
	                        s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x07e0f81f;
				d = (d | d << 16) & 0x07e0f81f;
				d += (s - d) * alpha >> 5;
				d &= 0x07e0f81f;
				*dstp++ = d | d >> 16;
			        s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x07e0f81f;
				d = (d | d << 16) & 0x07e0f81f;
				d += (s - d) * alpha >> 5;
				d &= 0x07e0f81f;
				*dstp++ = d | d >> 16;
                        },{
	                        movq_m2r((*dstp), mm3);/* 4 dst pixels -> mm3 */
	                        movq_m2r((*srcp), mm2);/* 4 src pixels -> mm2 */
			  
	                        /* RED */
	                        movq_r2r(mm2, mm5); /* src -> mm5 */
	                        pand_r2r(mm1 , mm5); /* src & MASKRED -> mm5 */
	                        psrlq_i2r(11, mm5); /* mm5 >> 11 -> mm5 [000r 000r 000r 000r] */
	
	                        movq_r2r(mm3, mm6); /* dst -> mm6 */
	                        pand_r2r(mm1 , mm6); /* dst & MASKRED -> mm6 */
	                        psrlq_i2r(11, mm6); /* mm6 >> 11 -> mm6 [000r 000r 000r 000r] */
	
	                        /* blend */
	                        psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	                        pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	                        psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	                        paddw_r2r(mm5, mm6); /* mm5 + mm6(dst) -> mm6 */
	                        psllq_i2r(11, mm6); /* mm6 << 11 -> mm6 */
	                        pand_r2r(mm1, mm6); /* mm6 & MASKRED -> mm6 */
	
	                        movq_r2r(mm4, mm5); /* MASKGREEN -> mm5 */
	                        por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	                        pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKRED) -> mm3 */
	                        por_r2r(mm6, mm3); /* save new reds in dsts */
	
	                        /* green */
	                        movq_r2r(mm2, mm5); /* src -> mm5 */
	                        pand_r2r(mm4 , mm5); /* src & MASKGREEN -> mm5 */
	                        psrlq_i2r(5, mm5); /* mm5 >> 5 -> mm5 [000g 000g 000g 000g] */
	
	                        movq_r2r(mm3, mm6); /* dst -> mm6 */
	                        pand_r2r(mm4 , mm6); /* dst & MASKGREEN -> mm6 */
	                        psrlq_i2r(5, mm6); /* mm6 >> 5 -> mm6 [000g 000g 000g 000g] */
	
	                        /* blend */
	                        psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	                        pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	                        psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	                        paddw_r2r(mm5, mm6); /* mm5 + mm6(dst) -> mm6 */
	                        psllq_i2r(5, mm6); /* mm6 << 5 -> mm6 */
	                        pand_r2r(mm4, mm6); /* mm6 & MASKGREEN -> mm6 */
	
	                        movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	                        por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	                        pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKGREEN) -> mm3 */
	                        por_r2r(mm6, mm3); /* save new greens in dsts */
	
	                        /* blue */
	                        movq_r2r(mm2, mm5); /* src -> mm5 */
	                        pand_r2r(mm7 , mm5); /* src & MASKRED -> mm5[000b 000b 000b 000b] */
		
	                        movq_r2r(mm3, mm6); /* dst -> mm6 */
	                        pand_r2r(mm7 , mm6); /* dst & MASKBLUE -> mm6[000b 000b 000b 000b] */
	
	                        /* blend */
	                        psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	                        pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	                        psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	                        paddw_r2r(mm5, mm6); /* mm5 + mm6(dst) -> mm6 */
	                        pand_r2r(mm7, mm6); /* mm6 & MASKBLUE -> mm6 */
	
	                        movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	                        por_r2r(mm4, mm5);  /* MASKGREEN | mm5 -> mm5 */
	                        pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKBLUE) -> mm3 */
	                        por_r2r(mm6, mm3); /* save new blues in dsts */
	
	                        movq_r2m(mm3, *dstp);/* mm3 -> 4 dst pixels */
	
	                        srcp += 4;
	                        dstp += 4;
                        }, width);			
			srcp += srcskip;
			dstp += dstskip;
		}
		emms();
	}
}

/* fast RGB555->RGB555 blending with surface alpha */
static void Blit555to555SurfaceAlphaMMX(SDL_BlitInfo *info)
{
	unsigned alpha = info->src->alpha; /* downscale alpha to 5 bits */
	if(alpha == 128) {
		Blit16to16SurfaceAlpha128(info, 0xfbde);
	} else {
		int width = info->d_width;
		int height = info->d_height;
		Uint16 *srcp = (Uint16 *)info->s_pixels;
		int srcskip = info->s_skip >> 1;
		Uint16 *dstp = (Uint16 *)info->d_pixels;
		int dstskip = info->d_skip >> 1;
	        Uint32 s, d;
	        Uint8 load[8];
	  
		alpha &= ~(1+2+4);		/* cut alpha to get the exact same behaviour */
	        *(Uint64 *)load = alpha;
		alpha >>= 3;		/* downscale alpha to 5 bits */

                movq_m2r(*load, mm0); /* alpha(0000000A) -> mm0 */
                punpcklwd_r2r(mm0, mm0); /* 00000A0A -> mm0 */
                punpcklwd_r2r(mm0, mm0); /* 0A0A0A0A -> mm0 */
	  
 	        /* Setup the 555 color channel masks */
	        *(Uint64 *)load = 0x7C007C007C007C00ULL;
		movq_m2r(*load, mm1); /* MASKRED -> mm1 */
		*(Uint64 *)load = 0x03E003E003E003E0ULL;
		movq_m2r(*load, mm4); /* MASKGREEN -> mm4 */
		*(Uint64 *)load = 0x001F001F001F001FULL;
		movq_m2r(*load, mm7); /* MASKBLUE -> mm7 */
		while(height--) {
                        DUFFS_LOOP_QUATRO2(
                        {
	                        s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x03e07c1f;
				d = (d | d << 16) & 0x03e07c1f;
				d += (s - d) * alpha >> 5;
				d &= 0x03e07c1f;
				*dstp++ = d | d >> 16;
                        },{
	                        s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x03e07c1f;
				d = (d | d << 16) & 0x03e07c1f;
				d += (s - d) * alpha >> 5;
				d &= 0x03e07c1f;
				*dstp++ = d | d >> 16;
			        s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x03e07c1f;
				d = (d | d << 16) & 0x03e07c1f;
				d += (s - d) * alpha >> 5;
				d &= 0x03e07c1f;
				*dstp++ = d | d >> 16;
                        },{
	                        movq_m2r((*dstp), mm3);/* 4 dst pixels -> mm3 */
	                        movq_m2r((*srcp), mm2);/* 4 src pixels -> mm2 */
			  
	                        /* RED */
	                        movq_r2r(mm2, mm5); /* src -> mm5 */
	                        pand_r2r(mm1 , mm5); /* src & MASKRED -> mm5 */
	                        psrlq_i2r(10, mm5); /* mm5 >> 10 -> mm5 [000r 000r 000r 000r] */
	
	                        movq_r2r(mm3, mm6); /* dst -> mm6 */
	                        pand_r2r(mm1 , mm6); /* dst & MASKRED -> mm6 */
	                        psrlq_i2r(10, mm6); /* mm6 >> 10 -> mm6 [000r 000r 000r 000r] */
	
	                        /* blend */
	                        psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	                        pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	                        psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	                        paddw_r2r(mm5, mm6); /* mm5 + mm6(dst) -> mm6 */
	                        psllq_i2r(10, mm6); /* mm6 << 10 -> mm6 */
	                        pand_r2r(mm1, mm6); /* mm6 & MASKRED -> mm6 */
	
	                        movq_r2r(mm4, mm5); /* MASKGREEN -> mm5 */
	                        por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	                        pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKRED) -> mm3 */
	                        por_r2r(mm6, mm3); /* save new reds in dsts */
	
	                        /* green */
	                        movq_r2r(mm2, mm5); /* src -> mm5 */
	                        pand_r2r(mm4 , mm5); /* src & MASKGREEN -> mm5 */
	                        psrlq_i2r(5, mm5); /* mm5 >> 5 -> mm5 [000g 000g 000g 000g] */
	
	                        movq_r2r(mm3, mm6); /* dst -> mm6 */
	                        pand_r2r(mm4 , mm6); /* dst & MASKGREEN -> mm6 */
	                        psrlq_i2r(5, mm6); /* mm6 >> 5 -> mm6 [000g 000g 000g 000g] */
	
	                        /* blend */
	                        psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	                        pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	                        psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	                        paddw_r2r(mm5, mm6); /* mm5 + mm6(dst) -> mm6 */
	                        psllq_i2r(5, mm6); /* mm6 << 5 -> mm6 */
	                        pand_r2r(mm4, mm6); /* mm6 & MASKGREEN -> mm6 */
	
	                        movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	                        por_r2r(mm7, mm5);  /* MASKBLUE | mm5 -> mm5 */
	                        pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKGREEN) -> mm3 */
	                        por_r2r(mm6, mm3); /* save new greens in dsts */
	
	                        /* blue */
	                        movq_r2r(mm2, mm5); /* src -> mm5 */
	                        pand_r2r(mm7 , mm5); /* src & MASKRED -> mm5[000b 000b 000b 000b] */
		
	                        movq_r2r(mm3, mm6); /* dst -> mm6 */
	                        pand_r2r(mm7 , mm6); /* dst & MASKBLUE -> mm6[000b 000b 000b 000b] */
	
	                        /* blend */
	                        psubw_r2r(mm6, mm5);/* src - dst -> mm5 */
	                        pmullw_r2r(mm0, mm5); /* mm5 * alpha -> mm5 */
	                        psrlw_i2r(8, mm5); /* mm5 >> 8 -> mm5 */
	                        paddw_r2r(mm5, mm6); /* mm5 + mm6(dst) -> mm6 */
	                        pand_r2r(mm7, mm6); /* mm6 & MASKBLUE -> mm6 */
	
	                        movq_r2r(mm1, mm5); /* MASKRED -> mm5 */
	                        por_r2r(mm4, mm5);  /* MASKGREEN | mm5 -> mm5 */
	                        pand_r2r(mm5, mm3); /* mm3 & mm5(!MASKBLUE) -> mm3 */
	                        por_r2r(mm6, mm3); /* save new blues in dsts */
	
	                        movq_r2m(mm3, *dstp);/* mm3 -> 4 dst pixels */
	
	                        srcp += 4;
	                        dstp += 4;
                        }, width);			
			srcp += srcskip;
			dstp += dstskip;
		}
		emms();
	}
}
#endif

/* fast RGB565->RGB565 blending with surface alpha */
static void Blit565to565SurfaceAlpha(SDL_BlitInfo *info)
{
	unsigned alpha = info->src->alpha;
	if(alpha == 128) {
		Blit16to16SurfaceAlpha128(info, 0xf7de);
	} else {
		int width = info->d_width;
		int height = info->d_height;
		Uint16 *srcp = (Uint16 *)info->s_pixels;
		int srcskip = info->s_skip >> 1;
		Uint16 *dstp = (Uint16 *)info->d_pixels;
		int dstskip = info->d_skip >> 1;
		alpha >>= 3;	/* downscale alpha to 5 bits */

		while(height--) {
			DUFFS_LOOP4({
				Uint32 s = *srcp++;
				Uint32 d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x07e0f81f;
				d = (d | d << 16) & 0x07e0f81f;
				d += (s - d) * alpha >> 5;
				d &= 0x07e0f81f;
				*dstp++ = d | d >> 16;
			}, width);
			srcp += srcskip;
			dstp += dstskip;
		}
	}
}

/* fast RGB555->RGB555 blending with surface alpha */
static void Blit555to555SurfaceAlpha(SDL_BlitInfo *info)
{
	unsigned alpha = info->src->alpha; /* downscale alpha to 5 bits */
	if(alpha == 128) {
		Blit16to16SurfaceAlpha128(info, 0xfbde);
	} else {
		int width = info->d_width;
		int height = info->d_height;
		Uint16 *srcp = (Uint16 *)info->s_pixels;
		int srcskip = info->s_skip >> 1;
		Uint16 *dstp = (Uint16 *)info->d_pixels;
		int dstskip = info->d_skip >> 1;
		alpha >>= 3;		/* downscale alpha to 5 bits */

		while(height--) {
			DUFFS_LOOP4({
				Uint32 s = *srcp++;
				Uint32 d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x03e07c1f;
				d = (d | d << 16) & 0x03e07c1f;
				d += (s - d) * alpha >> 5;
				d &= 0x03e07c1f;
				*dstp++ = d | d >> 16;
			}, width);
			srcp += srcskip;
			dstp += dstskip;
		}
	}
}

/* fast ARGB8888->RGB565 blending with pixel alpha */
static void BlitARGBto565PixelAlpha(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint32 *srcp = (Uint32 *)info->s_pixels;
	int srcskip = info->s_skip >> 2;
	Uint16 *dstp = (Uint16 *)info->d_pixels;
	int dstskip = info->d_skip >> 1;

	while(height--) {
	    DUFFS_LOOP4({
		Uint32 s = *srcp;
		unsigned alpha = s >> 27; /* downscale alpha to 5 bits */
		/* FIXME: Here we special-case opaque alpha since the
		   compositioning used (>>8 instead of /255) doesn't handle
		   it correctly. Also special-case alpha=0 for speed?
		   Benchmark this! */
		if(alpha) {   
		  if(alpha == (SDL_ALPHA_OPAQUE >> 3)) {
		    *dstp = (s >> 8 & 0xf800) + (s >> 5 & 0x7e0)
			  + (s >> 3  & 0x1f);
		  } else {
		    Uint32 d = *dstp;
		    /*
		     * convert source and destination to G0RAB65565
		     * and blend all components at the same time
		     */
		    s = ((s & 0xfc00) << 11) + (s >> 8 & 0xf800)
		      + (s >> 3 & 0x1f);
		    d = (d | d << 16) & 0x07e0f81f;
		    d += (s - d) * alpha >> 5;
		    d &= 0x07e0f81f;
		    *dstp = d | d >> 16;
		  }
		}
		srcp++;
		dstp++;
	    }, width);
	    srcp += srcskip;
	    dstp += dstskip;
	}
}

/* fast ARGB8888->RGB555 blending with pixel alpha */
static void BlitARGBto555PixelAlpha(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint32 *srcp = (Uint32 *)info->s_pixels;
	int srcskip = info->s_skip >> 2;
	Uint16 *dstp = (Uint16 *)info->d_pixels;
	int dstskip = info->d_skip >> 1;

	while(height--) {
	    DUFFS_LOOP4({
		unsigned alpha;
		Uint32 s = *srcp;
		alpha = s >> 27; /* downscale alpha to 5 bits */
		/* FIXME: Here we special-case opaque alpha since the
		   compositioning used (>>8 instead of /255) doesn't handle
		   it correctly. Also special-case alpha=0 for speed?
		   Benchmark this! */
		if(alpha) {   
		  if(alpha == (SDL_ALPHA_OPAQUE >> 3)) {
		    *dstp = (s >> 9 & 0x7c00) + (s >> 6 & 0x3e0)
			  + (s >> 3  & 0x1f);
		  } else {
		    Uint32 d = *dstp;
		    /*
		     * convert source and destination to G0RAB65565
		     * and blend all components at the same time
		     */
		    s = ((s & 0xf800) << 10) + (s >> 9 & 0x7c00)
		      + (s >> 3 & 0x1f);
		    d = (d | d << 16) & 0x03e07c1f;
		    d += (s - d) * alpha >> 5;
		    d &= 0x03e07c1f;
		    *dstp = d | d >> 16;
		  }
		}
		srcp++;
		dstp++;
	    }, width);
	    srcp += srcskip;
	    dstp += dstskip;
	}
}

/* General (slow) N->N blending with per-surface alpha */
static void BlitNtoNSurfaceAlpha(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint8 *dst = info->d_pixels;
	int dstskip = info->d_skip;
	SDL_PixelFormat *srcfmt = info->src;
	SDL_PixelFormat *dstfmt = info->dst;
	int srcbpp = srcfmt->BytesPerPixel;
	int dstbpp = dstfmt->BytesPerPixel;
	unsigned sA = srcfmt->alpha;
	unsigned dA = dstfmt->Amask ? SDL_ALPHA_OPAQUE : 0;

	if(sA) {
	  while ( height-- ) {
	    DUFFS_LOOP4(
	    {
		Uint32 pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		DISEMBLE_RGB(src, srcbpp, srcfmt, pixel, sR, sG, sB);
		DISEMBLE_RGB(dst, dstbpp, dstfmt, pixel, dR, dG, dB);
		ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB);
		ASSEMBLE_RGBA(dst, dstbpp, dstfmt, dR, dG, dB, dA);
		src += srcbpp;
		dst += dstbpp;
	    },
	    width);
	    src += srcskip;
	    dst += dstskip;
	  }
	}
}

/* General (slow) colorkeyed N->N blending with per-surface alpha */
static void BlitNtoNSurfaceAlphaKey(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint8 *dst = info->d_pixels;
	int dstskip = info->d_skip;
	SDL_PixelFormat *srcfmt = info->src;
	SDL_PixelFormat *dstfmt = info->dst;
	Uint32 ckey = srcfmt->colorkey;
	int srcbpp = srcfmt->BytesPerPixel;
	int dstbpp = dstfmt->BytesPerPixel;
	unsigned sA = srcfmt->alpha;
	unsigned dA = dstfmt->Amask ? SDL_ALPHA_OPAQUE : 0;

	while ( height-- ) {
	    DUFFS_LOOP4(
	    {
		Uint32 pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		RETRIEVE_RGB_PIXEL(src, srcbpp, pixel);
		if(sA && pixel != ckey) {
		    RGB_FROM_PIXEL(pixel, srcfmt, sR, sG, sB);
		    DISEMBLE_RGB(dst, dstbpp, dstfmt, pixel, dR, dG, dB);
		    ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB);
		    ASSEMBLE_RGBA(dst, dstbpp, dstfmt, dR, dG, dB, dA);
		}
		src += srcbpp;
		dst += dstbpp;
	    },
	    width);
	    src += srcskip;
	    dst += dstskip;
	}
}

/* General (slow) N->N blending with pixel alpha */
static void BlitNtoNPixelAlpha(SDL_BlitInfo *info)
{
	int width = info->d_width;
	int height = info->d_height;
	Uint8 *src = info->s_pixels;
	int srcskip = info->s_skip;
	Uint8 *dst = info->d_pixels;
	int dstskip = info->d_skip;
	SDL_PixelFormat *srcfmt = info->src;
	SDL_PixelFormat *dstfmt = info->dst;

	int  srcbpp;
	int  dstbpp;

	/* Set up some basic variables */
	srcbpp = srcfmt->BytesPerPixel;
	dstbpp = dstfmt->BytesPerPixel;

	/* FIXME: for 8bpp source alpha, this doesn't get opaque values
	   quite right. for <8bpp source alpha, it gets them very wrong
	   (check all macros!)
	   It is unclear whether there is a good general solution that doesn't
	   need a branch (or a divide). */
	while ( height-- ) {
	    DUFFS_LOOP4(
	    {
		Uint32 pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		unsigned sA;
		unsigned dA;
		DISEMBLE_RGBA(src, srcbpp, srcfmt, pixel, sR, sG, sB, sA);
		if(sA) {
		  DISEMBLE_RGBA(dst, dstbpp, dstfmt, pixel, dR, dG, dB, dA);
		  ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB);
		  ASSEMBLE_RGBA(dst, dstbpp, dstfmt, dR, dG, dB, dA);
		}
		src += srcbpp;
		dst += dstbpp;
	    },
	    width);
	    src += srcskip;
	    dst += dstskip;
	}
}


SDL_loblit SDL_CalculateAlphaBlit(SDL_Surface *surface, int blit_index)
{
    SDL_PixelFormat *sf = surface->format;
    SDL_PixelFormat *df = surface->map->dst->format;

    if(sf->Amask == 0) {
	if((surface->flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY) {
	    if(df->BytesPerPixel == 1)
		return BlitNto1SurfaceAlphaKey;
	    else
		return BlitNtoNSurfaceAlphaKey;
	} else {
	    /* Per-surface alpha blits */
	    switch(df->BytesPerPixel) {
	    case 1:
		return BlitNto1SurfaceAlpha;

	    case 2:
		if(surface->map->identity) {
		    if(df->Gmask == 0x7e0)
		    {
#ifdef MMX_ASMBLIT
		if(SDL_HasMMX())
			return Blit565to565SurfaceAlphaMMX;
		else
#endif
			return Blit565to565SurfaceAlpha;
		    }
		    else if(df->Gmask == 0x3e0)
		    {
#ifdef MMX_ASMBLIT
		if(SDL_HasMMX())
			return Blit555to555SurfaceAlphaMMX;
		else
#endif
			return Blit555to555SurfaceAlpha;
		    }
		}
		return BlitNtoNSurfaceAlpha;

	    case 4:
		if(sf->Rmask == df->Rmask
		   && sf->Gmask == df->Gmask
		   && sf->Bmask == df->Bmask
		   && (sf->Rmask | sf->Gmask | sf->Bmask) == 0xffffff
		   && sf->BytesPerPixel == 4)
		{
#ifdef MMX_ASMBLIT
		if(SDL_HasMMX())
		    return BlitRGBtoRGBSurfaceAlphaMMX;
		else
#endif
		    return BlitRGBtoRGBSurfaceAlpha;
		}
		else
		    return BlitNtoNSurfaceAlpha;

	    case 3:
	    default:
		return BlitNtoNSurfaceAlpha;
	    }
	}
    } else {
	/* Per-pixel alpha blits */
	switch(df->BytesPerPixel) {
	case 1:
	    return BlitNto1PixelAlpha;

	case 2:
	    if(sf->BytesPerPixel == 4 && sf->Amask == 0xff000000
	       && sf->Gmask == 0xff00
	       && ((sf->Rmask == 0xff && df->Rmask == 0x1f)
		   || (sf->Bmask == 0xff && df->Bmask == 0x1f))) {
		if(df->Gmask == 0x7e0)
		    return BlitARGBto565PixelAlpha;
		else if(df->Gmask == 0x3e0)
		    return BlitARGBto555PixelAlpha;
	    }
	    return BlitNtoNPixelAlpha;

	case 4:
	    if(sf->Amask == 0xff000000
	       && sf->Rmask == df->Rmask
	       && sf->Gmask == df->Gmask
	       && sf->Bmask == df->Bmask
	       && sf->BytesPerPixel == 4)
	    {
#ifdef MMX_ASMBLIT
		if(SDL_Has3DNow())
		    return BlitRGBtoRGBPixelAlphaMMX3DNOW;
		else
		if(SDL_HasMMX())
		    return BlitRGBtoRGBPixelAlphaMMX;
		else
#endif
		    return BlitRGBtoRGBPixelAlpha;
	    }
	    return BlitNtoNPixelAlpha;

	case 3:
	default:
	    return BlitNtoNPixelAlpha;
	}
    }
}

