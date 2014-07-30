/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

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
#include "SDL_config.h"

/*
	Milan/CTPCI Xbios video functions

	Patrice Mandin
	Pawel Goralski
*/

#ifndef _SDL_xbios_milan_h
#define _SDL_xbios_milan_h

#include "SDL_xbios.h"

/*--- Defines ---*/

/* Vsetscreen() parameters */
#define MI_MAGIC	0x4D49	/* Milan */
#define VN_MAGIC	0x564E	/* CTPCI */

enum {
	/* Milan/CTPCI */
	CMD_GETMODE=0,
	CMD_SETMODE,
	CMD_GETINFO,
	CMD_ALLOCPAGE,
	CMD_FREEPAGE,
	CMD_FLIPPAGE,
	CMD_ALLOCMEM,
	CMD_FREEMEM,
	CMD_SETADR,
	CMD_ENUMMODES,

	/* CTPCI only */
	CMD_TESTMODE,
	CMD_COPYPAGE,
	CMD_FILLMEM,
	CMD_COPYMEM,
	CMD_TEXTUREMEM,
	CMD_GETVERSION,
	CMD_LINEMEM,
	CMD_CLIPMEM,
	CMD_SYNCMEM,
	CMD_BLANK
};

enum {
	ENUMMODE_EXIT=0,
	ENUMMODE_CONT
};

enum {
	BLK_ERR=0,
	BLK_OK,
	BLK_CLEARED
};

/* CTPCI block operations */
enum {
	BLK_CLEAR,
	BLK_AND,
	BLK_ANDREVERSE,
	BLK_COPY,
	BLK_ANDINVERTED,
	BLK_NOOP,
	BLK_XOR,
	BLK_OR,
	BLK_XNOR,
	BLK_EQUIV,
	BLK_INVERT,
	BLK_ORREVERSE,
	BLK_COPYINVERTED,
	BLK_ORINVERTED,
	BLK_NAND,
	BLK_SET 
};

/* scrFlags */
#define SCRINFO_OK 1

/* scrClut */
#define NO_CLUT 0
#define HARD_CLUT 1
#define SOFT_CLUT 2

/* scrFormat */
#define INTERLEAVE_PLANES 0
#define STANDARD_PLANES  1
#define PACKEDPIX_PLANES 2

/* bitFlags */
#define STANDARD_BITS  1
#define FALCON_BITS   2
#define INTEL_BITS   8

/*--- Structures ---*/

typedef struct _scrblk { 
	unsigned long	size;		/* size of strukture */ 
	unsigned long	blk_status;	/* status bits of blk */ 
	unsigned long	blk_start;	/* Start Adress */ 
	unsigned long	blk_len;	/* length of memblk */ 
	unsigned long	blk_x;		/* x pos in total screen*/ 
	unsigned long	blk_y;		/* y pos in total screen */ 
	unsigned long	blk_w;		/* width */ 
	unsigned long	blk_h;		/* height */ 
	unsigned long	blk_wrap;	/* width in bytes */ 
} SCRMEMBLK;

typedef struct screeninfo { 
	unsigned long	size;		/* Size of structure */ 
	unsigned long	devID;		/* device id number */ 
	unsigned char	name[64];	/* Friendly name of Screen */ 
	unsigned long	scrFlags;	/* some Flags */ 
	unsigned long	frameadr;	/* Adress of framebuffer */ 
	unsigned long	scrHeight;	/* visible X res */ 
	unsigned long	scrWidth;	/* visible Y res */ 
	unsigned long	virtHeight;	/* virtual X res */ 
	unsigned long	virtWidth;	/* virtual Y res */ 
	unsigned long	scrPlanes;	/* color Planes */ 
	unsigned long	scrColors;	/* # of colors */ 
	unsigned long	lineWrap;	/* # of Bytes to next line */ 
	unsigned long	planeWarp;	/* # of Bytes to next plane */ 
	unsigned long	scrFormat;	/* screen Format */ 
	unsigned long	scrClut;	/* type of clut */ 
	unsigned long	redBits;	/* Mask of Red Bits */ 
	unsigned long	greenBits;	/* Mask of Green Bits */ 
	unsigned long	blueBits;	/* Mask of Blue Bits */ 
	unsigned long	alphaBits;	/* Mask of Alpha Bits */ 
	unsigned long	genlockBits;	/* Mask of Genlock Bits */ 
	unsigned long	unusedBits;	/* Mask of unused Bits */ 
	unsigned long	bitFlags;	/* Bits organisation flags */ 
	unsigned long	maxmem;		/* max. memory in this mode */ 
	unsigned long	pagemem;	/* needed memory for one page */ 
	unsigned long	max_x;		/* max. possible width */ 
	unsigned long	max_y;		/* max. possible heigth */ 
} SCREENINFO; 

typedef struct _scrfillblk {
	long size;	/* size of structure           */
	long blk_status;/* status bits of blk          */
	long blk_op;	/* mode operation              */
	long blk_color;	/* background fill color       */
	long blk_x;	/* x pos in total screen       */
	long blk_y;	/* y pos in total screen       */
	long blk_w;	/* width                       */
	long blk_h;	/* height                      */
	long blk_unused;
} SCRFILLMEMBLK;
             
typedef struct _scrcopyblk {
	long size;	/* size of structure            */
	long blk_status;/* status bits of blk           */
	long blk_src_x;	/* x pos source in total screen */
	long blk_src_y;	/* y pos source in total screen */
	long blk_dst_x;	/* x pos dest in total screen   */
	long blk_dst_y;	/* y pos dest in total screen   */
	long blk_w;	/* width                        */
	long blk_h;	/* height                       */
	long blk_op;	/* block operation */
} SCRCOPYMEMBLK;

typedef struct _scrtextureblk {
	long size;		/* size of structure             */
	long blk_status;	/* status bits of blk            */
	long blk_src_x;		/* x pos source                  */
	long blk_src_y;		/* y pos source                  */
	long blk_dst_x;		/* x pos dest in total screen    */
	long blk_dst_y;		/* y pos dest in total screen    */
	long blk_w;		/* width                         */
	long blk_h;		/* height                        */
	long blk_op;		/* mode operation                */
	long blk_src_tex;	/* source texture address        */
	long blk_w_tex;		/* width texture                 */
	long blk_h_tex;		/* height texture                */
} SCRTEXTUREMEMBLK;

typedef struct _scrlineblk {
	long size;		/* size of structure            */
	long blk_status;	/* status bits of blk           */
	long blk_fgcolor;	/* foreground fill color        */
	long blk_bgcolor;	/* background fill color        */
	long blk_x1;		/* x1 pos dest in total screen  */
	long blk_y1;		/* y1 pos dest in total screen  */
	long blk_x2;		/* x2 pos dest in total screen  */
	long blk_y2;		/* y2 pos dest in total screen  */
	long blk_op;		/* mode operation               */
	long blk_pattern;	/* pattern (-1: solid line)     */
} SCRLINEMEMBLK;

#endif /* _SDL_xbios_milan_h */
