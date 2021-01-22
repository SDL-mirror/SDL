/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2016 Sam Lantinga

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
#ifndef SDL_os2grop_h_
#define SDL_os2grop_h_

#include "SDL_video.h"
#include "SDL_grop.h"

#define _ULS_CALLCONV_
#define CALLCONV _System
#include <unidef.h> /* Unicode API */
#include <uconv.h>  /* Unicode API (codepage conversion) */

/* Private display data */
typedef struct SDL_PrivateVideoData SDL_PrivateVideoData;

/* Video mode bpp/size */
typedef struct _BPPSIZE {
  ULONG                 ulBPP;
  SDL_Rect              stSDLRect;
} BPPSIZE, *PBPPSIZE;

typedef struct _BPPSIZELIST {
  struct _BPPSIZELIST  *pNext;
  ULONG                 ulBPP;
  SDL_Rect             *apSDLRect[1];
} BPPSIZELIST, *PBPPSIZELIST;

struct SDL_PrivateVideoData {
  PGROPDATA             pGrop;
  PBPPSIZE              paBPPSize;
  PBPPSIZELIST          pBPPSizeList;
  PRECTL                prectlReserved;
  ULONG                 crectlReserved;
  UconvObject           ucoUnicode;
  HPOINTER              hptrWndIcon;

  /* Window size changes tracking */
  ULONG                 ulWinWidth;
  ULONG                 ulWinHeight;
  BOOL                  fWinResized;
  ULONG                 ulResizedReportTime;
};


/* Mouse pointer structure for OS/2 related code */

#define VBOX_HACK_SUPPORT 1
/* w/o VBOX_HACK_SUPPORT:  We use type cast for (WMcursor *) as HPOINTER.
 *                         Content of structure is not interested for us.
 * with VBOX_HACK_SUPPORT: We use only one field - hptr.
 */
struct WMcursor
{
  HBITMAP       hbm;
  HPOINTER      hptr;
  char* pchData;
};

#endif /* SDL_os2grop_h_ */
