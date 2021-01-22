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
#include "SDL_config.h"

#if SDL_VIDEO_DRIVER_OS2GROP

/*
  GrOp graphics library allows applications to create framebuffers, producing
  graphics output and provides access to keyboard and mouse. The output can be
  displayed in windows or fullscreen.

  Vasilkin Andrey, 2016.
*/

#include "SDL_stdinc.h"

/* for _beginthread and _endthread */
#include <stdlib.h>
#include <process.h>

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSPROCESS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIBITMAPS     /* GPI bit map functions  */
#define INCL_DOSRESOURCES   /* Dos Resource functions */
#include <os2.h>

#include "SDL_grop.h"

#define WIN_CLIENT_CLASS    "GrOp"

/* WM_GROP_QUERY
 * mp1: PGROPQUERY */
#define WM_GROP_QUERY       WM_USER + 10

/* Video systems */
extern VIDEOSYS stVideoSysDIVE;           /* vs_dive.c */
extern VIDEOSYS stVideoSysVMANCompatible; /* vs_vman.c */
extern VIDEOSYS stVideoSysVMAN;           /* vs_vman.c */

static PVIDEOSYS aVideoSys[] = {
  &stVideoSysDIVE,
  &stVideoSysVMANCompatible,
  &stVideoSysVMAN
};

#define VIDEOSYSNUM         (sizeof(aVideoSys) / sizeof(PVIDEOSYS))

/* Query to GROP window data structure pointed by mp1 paramether of
 * window message WM_GROP_QUERY. */
#define QUERY_SET_MODE          0
#define QUERY_SET_FS            1
#define QUERY_SET_POINTER       2
#define QUERY_SET_CAPTURE       3
#define QUERY_MINIMIZE          4
#define QUERY_MOUSEMOVE         5
#define QUERY_UPDATE            6
#define QUERY_SET_PALETTER      7
#define QUERY_SET_TITLE         8
#define QUERY_SET_ICON          9

#define _GROP_SET_FS_PANIC_OFF  100

struct _GQUPDATE {
  ULONG     cRect;
  PRECTL    prectl;
  ULONG     _pad;
};

struct _GQSETPAL {
  ULONG     ulFirst;
  ULONG     ulNumber;
  PRGB2     pColors;
};

typedef struct _GROPQUERY {
  ULONG     ulQuery;                    /* QUERY_SET_xxxxx */
  HEV       hevReady;
  BOOL      fSuccess;                   /* output: result  */

  /* Query type specific data. */
  union {
    PGROPSETMODE      pUserMode;        /* QUERY_SET_MODE */
    ULONG             ulSetFullscreen;  /* QUERY_SET_FS */
    HPOINTER          hptrPointer;      /* QUERY_SET_POINTER, QUERY_SET_ICON */
    BOOL              fCapture;         /* QUERY_SET_CAPTURE */
    POINTL            pointlPos;        /* QUERY_MOUSEMOVE */
    struct _GQUPDATE  GQUPDATE;         /* QUERY_UPDATE */
    struct _GQSETPAL  GQSETPAL;         /* QUERY_SET_PALETTER */
    PSZ               pszText;          /* WM_GROP_SET_TITLE */
  };
} GROPQUERY, *PGROPQUERY;


static VOID _captureMouse(PGROPDATA pGrop, BOOL fCapture)
{
  SWP     swp;
  POINTL  pointl;

  if (fCapture) {
    WinSetCapture(pGrop->hwndDT, pGrop->hwnd);

    WinQueryWindowPos(pGrop->hwnd, &swp);
    pointl.x = swp.cx / 2;
    pointl.y = swp.cy / 2;
    WinMapWindowPoints(pGrop->hwnd, pGrop->hwndDT, &pointl, 1);

    pGrop->lSkipMouseMove++;
    WinSetPointerPos(pGrop->hwndDT, pointl.x, pointl.y);
  }
  else
    WinSetCapture(pGrop->hwndDT, NULLHANDLE);
}

static BOOL _setFullscreen(PGROPDATA pGrop, ULONG ulModeIdx)
{
  PVIDEOMODE pMode = &pGrop->stModes.pList[ulModeIdx];
  RECTL rectl;
  LONG  lX, lY, lW, lH;

  /* Move _client_ window to up-left corner of screen, set client window size
   * equals mode size. */
  rectl.xLeft = 0;
  rectl.yBottom = 0;
  rectl.xRight = pMode->ulWidth;
  rectl.yTop = pMode->ulHeight;
  WinCalcFrameRect(pGrop->hwndFrame, &rectl, FALSE);

  lX = rectl.xLeft;
  lY = pGrop->ulDTHeight - pMode->ulHeight + rectl.yBottom;
  lW = rectl.xRight - rectl.xLeft;
  lH = rectl.yTop - rectl.yBottom;

  pGrop->fInternalResize = TRUE;
  WinSetWindowPos(pGrop->hwndFrame, HWND_TOP, lX, lY, lW, lH,
                  SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE);
  pGrop->fInternalResize = FALSE;

  _captureMouse(pGrop, TRUE);
#ifdef GROP_NO_MOUSE_IN_FULLSCREEN
  WinSetPointer(pGrop->hwndDT, NULLHANDLE);
#endif
  WinPostMsg(pGrop->hwnd, WM_VRNENABLED, 0, 0);
  WinSetVisibleRegionNotify(pGrop->hwnd, FALSE);

  if (!pGrop->pVideoSys->fnSetMode(pGrop->pVSData, pMode, TRUE,
                                   pGrop->hwndDT, pGrop->hdcDT))
    return FALSE;

  return TRUE;
}

static BOOL _setDesktop(PGROPDATA pGrop)
{
  PVIDEOMODE pMode = &pGrop->stModes.pList[pGrop->stModes.ulDesktopMode];

  if (!pGrop->pVideoSys->fnSetMode(pGrop->pVSData, pMode, FALSE,
                                   pGrop->hwndDT, pGrop->hdcDT))
    return FALSE;

  WinSetVisibleRegionNotify(pGrop->hwnd, TRUE);
  return TRUE;
}

static VOID _calcViewRect(PGROPDATA pGrop, BOOL fSendSize)
{
  PGROPSETMODE  pUserMode = &pGrop->stUserMode;
  LONG  lWinX, lWinY, lWinWidth, lWinHeight;
  LONG  lVAWidth;
  LONG  lVAHeight;

  if (pGrop->ulModeIdx == ~0)
    /* Mode was not selected. */
    return;

  if (pGrop->fFullscreen) {
    PVIDEOMODE  pMode = &pGrop->stModes.pList[pGrop->ulModeIdx];

    lWinWidth = pMode->ulWidth;
    lWinHeight = pMode->ulHeight;
    lWinX = 0;
    lWinY = pGrop->ulDTHeight - lWinHeight;
  }
  else {
    SWP     swp;
    POINTL  pointl = { 0 };

    WinQueryWindowPos(pGrop->hwnd, &swp);
    lWinWidth = swp.cx;
    lWinHeight = swp.cy;
    WinMapWindowPoints(pGrop->hwnd, HWND_DESKTOP, &pointl, 1);
    lWinX = pointl.x;
    lWinY = pointl.y;
  }

  /* Calc view rectangle size and position at center of window or screen */
  if (pGrop->pVideoSys->fFreeWinSize) {
    /* Fit output frame to window, keep proportional. */
    if (lWinWidth * pUserMode->ulHeight > lWinHeight * pUserMode->ulWidth) {
      lVAHeight = lWinHeight;
      lVAWidth = (lWinHeight * pUserMode->ulWidth) / pUserMode->ulHeight;
    }
    else {
      lVAWidth = lWinWidth;
      lVAHeight = (lWinWidth * pUserMode->ulHeight) / pUserMode->ulWidth;
    }
  }
  else {
    lVAWidth = pUserMode->ulWidth;
    lVAHeight = pUserMode->ulHeight;
  }

  pGrop->rectlViewArea.xLeft = (lWinWidth - lVAWidth) / 2;
  pGrop->rectlViewArea.yBottom = (lWinHeight - lVAHeight) / 2;
  pGrop->rectlViewArea.xRight = pGrop->rectlViewArea.xLeft + lVAWidth;
  pGrop->rectlViewArea.yTop = pGrop->rectlViewArea.yBottom + lVAHeight;
  pGrop->rectlWinArea.xLeft = lWinX;
  pGrop->rectlWinArea.yBottom = lWinY;
  pGrop->rectlWinArea.xRight = lWinX + lWinWidth;
  pGrop->rectlWinArea.yTop = lWinY + lWinHeight;
  /*
  printf("-1- %d %d %d %d - %d %d %d %d\n",
         pGrop->rectlViewArea.xLeft,
         pGrop->rectlViewArea.yBottom,
         pGrop->rectlViewArea.xRight,
         pGrop->rectlViewArea.yTop,
         pGrop->rectlWinArea.xLeft,
         pGrop->rectlWinArea.yBottom,
         pGrop->rectlWinArea.xRight,
         pGrop->rectlWinArea.yTop);
 */
}

static BOOL _setMode(PGROPDATA pGrop, ULONG ulModeIdx, BOOL fFullscreen)
{
  PVIDEOMODE  pMode = &pGrop->stModes.pList[ulModeIdx];
  /* fFirstTime - Mode was not selected, mode will be set first time. */
  BOOL  fFirstTime = pGrop->ulModeIdx == ~0;
  RECTL rectl;
  LONG  lX, lY, lW, lH;
  BOOL  fSuccess = TRUE;

  if (fFirstTime) {
    pGrop->swpOld.x = pGrop->ulDTWidth / 2;
    pGrop->swpOld.y = pGrop->ulDTHeight / 2;
  }

  fFullscreen &= (pGrop->stUserMode.ulWidth <= pMode->ulWidth) &&
                 (pGrop->stUserMode.ulHeight <= pMode->ulHeight);

  if (fFullscreen) {
    if (pGrop->fActive || fFirstTime) {
      /* Is the fullscreen and not on the descktop now - change fulscreen mode. */
      if (!pGrop->fFullscreen && !fFirstTime)
        /* Save windows position before switching to full-screen mode. */
        WinQueryWindowPos(pGrop->hwndFrame, &pGrop->swpOld);

      /* Do not change FS mode if it is the same as the current one. */
      fSuccess = (pGrop->fFullscreen && (pGrop->ulModeIdx == ulModeIdx)) ||
                 _setFullscreen(pGrop, ulModeIdx);
    }
    /* else: FS mode but on the desktop now, change mode later in _wmActive(). */

    pGrop->ulModeIdx = ulModeIdx;
    pGrop->fFullscreen = TRUE;
  }
  else /*if (pGrop->fFullscreen || pGrop->ulModeIdx != ulModeIdx)*/ {

    /* Change window if mode is not same as the current one. */
    if (pGrop->fFullscreen) {
      dbgprintf("Return to the desktop...\n");
      _setDesktop(pGrop);
    }

    dbgprintf("Calc new window size...\n");
    rectl.xLeft = 0;
    rectl.yBottom = 0;
    rectl.xRight = pGrop->stUserMode.ulWidth;
    rectl.yTop = pGrop->stUserMode.ulHeight;
    WinCalcFrameRect(pGrop->hwndFrame, &rectl, FALSE);
    lW = rectl.xRight - rectl.xLeft;
    lH = rectl.yTop - rectl.yBottom;

    if (!pGrop->fFullscreen && !fFirstTime) {
      /* Positioning relative to the current location. */
      dbgprintf("Query current window position...\n");
      WinQueryWindowPos(pGrop->hwndFrame, &pGrop->swpOld);
    }
    else
      dbgprintf("Window's location will be relative to the location before FS.\n");

    lX = pGrop->swpOld.x + (pGrop->swpOld.cx / 2) - (lW / 2);
    lY = pGrop->swpOld.y + (pGrop->swpOld.cy / 2) - (lH / 2);

    if (lW > pGrop->ulDTWidth)
      lX = (pGrop->ulDTWidth / 2) - (lW / 2);
    else if ((lX + lW) > pGrop->ulDTWidth)
      lX = pGrop->ulDTWidth - lW;
    else if (lX < 0)
      lX = 0;

    if (lY < 0)
      lY = 0;
    if ((lY + lH) > pGrop->ulDTHeight)
      lY = pGrop->ulDTHeight - lH;

    pGrop->ulModeIdx = ulModeIdx;
    pGrop->fFullscreen = FALSE;

    dbgprintf("Set window position...\n");
    pGrop->fInternalResize = TRUE;
    WinSetWindowPos(pGrop->hwndFrame, HWND_TOP, lX, lY, lW, lH,
       fFirstTime ? SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE
                  : SWP_MOVE | SWP_SIZE | SWP_SHOW);
    pGrop->fInternalResize = FALSE;

    dbgprintf("Call _captureMouse(,%s)...\n", pGrop->fCapture ? "TRUE" : "FALSE");
    _captureMouse(pGrop, pGrop->fCapture);
  }

  dbgprintf("Call _calcViewRect(,FALSE)...\n");
  _calcViewRect(pGrop, FALSE);
  dbgprintf("Call pVideoSys->fnUpdate()...\n");
  pGrop->pVideoSys->fnUpdate(pGrop->pVSData, pGrop, 0, NULL);

  return fSuccess;
}

static VOID _setVisibleRegion(PGROPDATA pGrop, BOOL fVisible)
{
  if (pGrop->pVideoSys->fnSetVisibleRegion != NULL) {
    pGrop->pVideoSys->fnSetVisibleRegion(pGrop->pVSData, pGrop->hwnd,
                                         &pGrop->stUserMode,
                                         &pGrop->rectlWinArea,
                                         &pGrop->rectlViewArea, fVisible);
  }
}

static BOOL _querySetMode(PGROPDATA pGrop, PGROPSETMODE pNewUserMode)
{
  PGROPSETMODE  pOldUserMode = &pGrop->stUserMode;
  ULONG         ulNewModeIdx = ~0;
  ULONG         ulIdx;
  PVIDEOMODE    pVideoMode, pList = pGrop->stModes.pList;
  BOOL  fNewFullscreen = (pNewUserMode->ulFlags & GROP_MODEFL_FULLSCREEN) != 0;

  dbgprintf("Requested mode: Width: %u, Height: %u, BPP: %u, Fullscreen: %s\n",
            pNewUserMode->ulWidth, pNewUserMode->ulHeight, pNewUserMode->ulBPP,
            fNewFullscreen ? "yes" : "no");

  if (fNewFullscreen)
    pNewUserMode->ulFlags &= ~GROP_MODEFL_RESIZABLE;

  /* Select mode. */

  /* Windowed mode or BPP not requested - select vireo mode with desktop BPP
   * and best resolution. */
  if (!fNewFullscreen || pNewUserMode->ulBPP == 0)
    pNewUserMode->ulBPP = pList[pGrop->stModes.ulDesktopMode].ulBPP;

  /* Check all available video modes for this resolution. */
  for (ulIdx = 0; ulIdx < pGrop->stModes.ulCount; ulIdx++) {
    pVideoMode = &pList[ulIdx];

    if ((pVideoMode->ulWidth == pNewUserMode->ulWidth) &&
        (pVideoMode->ulHeight == pNewUserMode->ulHeight)) {
      /* If good resolution, try to find the exact BPP, or something similar... */
      if (!fNewFullscreen) {
        /* For resizable (non-fullscreen) modes, we have to have the very same BPP as desktop! */
        if (pVideoMode->ulBPP == pNewUserMode->ulBPP)
          ulNewModeIdx = ulIdx;
      }
      /* For fullscreen modes we don't have to force to desktop BPP */
      else if ((ulNewModeIdx == ~0) ||
              /* ((pList[ulNewModeIdx].ulBPP != pNewUserMode->ulBPP) &&
                  (pGrop->stModes.pList[ulNewModeIdx].ulBPP < pVideoMode->ulBPP)
                )*/
                (abs((LONG)(pList[ulNewModeIdx].ulBPP - pNewUserMode->ulBPP)) >
                 abs((LONG)(pVideoMode->ulBPP - pNewUserMode->ulBPP)))
              )
        ulNewModeIdx = ulIdx;
    }
  }

  /* If we did not find a good video mode, then try a similar */
  if (ulNewModeIdx == ~0) {
    /* Go through the video modes again, and find a similar resolution! */
    for (ulIdx = 0; ulIdx < pGrop->stModes.ulCount; ulIdx++) {
      pVideoMode = &pList[ulIdx];

      /* Check all available fullscreen modes for this resolution */
      if (pVideoMode->ulWidth >= pNewUserMode->ulWidth &&
          pVideoMode->ulHeight >= pNewUserMode->ulHeight &&
          pVideoMode->ulBPP == pNewUserMode->ulBPP) {

        if (ulNewModeIdx == ~0 ||
             (((pList[ulNewModeIdx].ulWidth - pNewUserMode->ulWidth) *
               (pList[ulNewModeIdx].ulHeight - pNewUserMode->ulHeight)) >
              ((pVideoMode->ulWidth - pNewUserMode->ulWidth) *
               (pVideoMode->ulHeight - pNewUserMode->ulHeight))) ) {
          /* Found a mode which is closer than the current one */
          ulNewModeIdx = ulIdx;
        }
      }
    }
  }

  if (ulNewModeIdx == ~0) {
    if ((pList[pGrop->stModes.ulDesktopMode].ulWidth < pNewUserMode->ulWidth) ||
        (pList[pGrop->stModes.ulDesktopMode].ulHeight < pNewUserMode->ulHeight)) {
      dbgprintf("Appropriate video mode not found.\n");
      pNewUserMode->pBuffer = NULL; /* Set mode error; */
      return FALSE;
    }

    dbgprintf("Appropriate video mode not found. Use desktop video mode.\n");
    ulNewModeIdx = pGrop->stModes.ulDesktopMode;
  }

  pVideoMode = &pList[ulNewModeIdx];
  pNewUserMode->ulBPP = pVideoMode->ulBPP;
  pNewUserMode->fccColorEncoding = pVideoMode->fccColorEncoding;
  dbgprintf("Mode was found: #%u Width: %u, Height: %u, BPP: %u\n",
            pVideoMode->ulId,
            pVideoMode->ulWidth, pVideoMode->ulHeight, pVideoMode->ulBPP);

  /* Allocate a new buffer and set new mode. */
  {
    GROPSETMODE stSaveUserMode = *pOldUserMode;

    dbgprintf("Call pVideoSys->fnVideoBufAlloc()...\n");
    pNewUserMode->pBuffer = pGrop->pVideoSys->fnVideoBufAlloc(
                              pGrop->pVSData,
                              pNewUserMode->ulWidth, pNewUserMode->ulHeight,
                              pNewUserMode->ulBPP,
                              pNewUserMode->fccColorEncoding,
                              &pNewUserMode->ulScanLineSize);
    if (pNewUserMode->pBuffer == NULL)
      dbgprintf("Cannot allocate a new video buffer\n");
    else {
      dbgprintf("A new video buffer allocated: 0x%P\n", pNewUserMode->pBuffer);

      *pOldUserMode = *pNewUserMode;  /* We need this data while _setMode(). */

      /* Set new mode. */
      if (!_setMode(pGrop, ulNewModeIdx,
                    (pNewUserMode->ulFlags & GROP_MODEFL_FULLSCREEN) != 0)) {
        dbgprintf("Cannot set a new mode\n");
        /* Destroy allocated buffer. */
        pGrop->pVideoSys->fnVideoBufFree(pGrop->pVSData,
                                         pNewUserMode->pBuffer);
        pNewUserMode->pBuffer = NULL;
        *pOldUserMode = stSaveUserMode; /* Rollback user mode data. */
      }
      else if (stSaveUserMode.pBuffer != NULL) {
        dbgprintf("Deallocate old buffer: 0x%P\n", stSaveUserMode.pBuffer);
        /* Destroy old buffer. */
        pGrop->pVideoSys->fnVideoBufFree(pGrop->pVSData,
                                         stSaveUserMode.pBuffer);
      }
    }
  }

  dbgprintf("Done, result: 0x%P\n", pNewUserMode->pBuffer);
  return pNewUserMode->pBuffer != NULL;
}

static BOOL _queryMouseMove(PGROPDATA pGrop, LONG lX, LONG lY)
{
  POINTL  pointl;
  ULONG   ulVAWidth  = pGrop->rectlViewArea.xRight - pGrop->rectlViewArea.xLeft;
  ULONG   ulVAHeight = pGrop->rectlViewArea.yTop - pGrop->rectlViewArea.yBottom;

  /* For compatibility with some SDL-programs simulating the relative mouse
   * motion (for ex. OpenTyrian). */
  pGrop->fCapture = FALSE;

  /* Restrict the movement of the mouse user work area. */
  if (lX < 0)
    lX = 0;
  else if (lX >= pGrop->stUserMode.ulWidth)
    lX = pGrop->stUserMode.ulWidth - 1;
  if (lY < 0)
    lY = 0;
  else if (lY >= pGrop->stUserMode.ulHeight)
    lY = pGrop->stUserMode.ulHeight - 1;

  /* Scale coordinates from work area to the view area. */
  pointl.x = pGrop->rectlViewArea.xLeft +
               ((ulVAWidth * lX) / pGrop->stUserMode.ulWidth);
  pointl.y = pGrop->rectlViewArea.yBottom +
               (ulVAHeight * lY) / pGrop->stUserMode.ulHeight;

  WinMapWindowPoints(pGrop->hwnd, pGrop->hwndDT, &pointl, 1);

  pGrop->lSkipMouseMove++;
  return WinSetPointerPos(pGrop->hwndDT, pointl.x, pointl.y);
}


static VOID _wmGropQuery(PGROPDATA pGrop, PGROPQUERY pQuery)
{
  dbgprintf("pQuery->ulQuery = %u\n", pQuery->ulQuery);

  switch (pQuery->ulQuery) {
  case QUERY_SET_MODE:
    pQuery->fSuccess = _querySetMode(pGrop, pQuery->pUserMode);
    break;

  case QUERY_SET_FS:
    if (pGrop->ulModeIdx == ~0)
      /* Mode was not set. */
      pQuery->fSuccess = FALSE;
    else {
      BOOL  fFullscreen;
      switch (pQuery->ulSetFullscreen) {
      case GROP_SET_FS_ON:
        fFullscreen = TRUE;
        break;
      case GROP_SET_FS_SWITCH:
        fFullscreen = !pGrop->fFullscreen;
        break;
      default: /* GROP_SET_FS_OFF, _GROP_SET_FS_PANIC_OFF */
        fFullscreen = FALSE;
      }
      /* Fullscreen mode BPP and desktop mode BPP must be same to switch
       * from fullscreen to the desktop if video system does not support
       * conversion for BPP. Exception is _GROP_SET_FS_PANIC_OFF
       * - we should switch to the desktop at exit if not all GROP objects
       * was destroyed by user.
       *
       * Switching from fullscreen to window is always available for:
       * DIVE - can blit buffer with any BPP,
       * VMANComptible - manages just one mode.
       */
      if (!fFullscreen &&
          (pQuery->ulSetFullscreen != _GROP_SET_FS_PANIC_OFF) &&
          (!pGrop->pVideoSys->fBPPConvSup) &&
          (pGrop->stModes.pList[pGrop->stModes.ulDesktopMode].ulBPP !=
           pGrop->stModes.pList[pGrop->ulModeIdx].ulBPP))
          pQuery->fSuccess = FALSE;
        else
          pQuery->fSuccess = (fFullscreen == pGrop->fFullscreen) ||
                             _setMode(pGrop, pGrop->ulModeIdx, fFullscreen);
      }
    break;

  case QUERY_SET_POINTER:
    dbgprintf("QUERY_SET_POINTER: %u\n", pQuery->hptrPointer);
    pGrop->hptrPointer = pQuery->hptrPointer;
    pQuery->fSuccess = WinSetPointer(pGrop->hwndDT, pGrop->hptrPointer);
    dbgprintf("WinSetPointer(): %s\n", pQuery->fSuccess ? "TRUE" : "FALSE");

    /* Unvisible pointer when mouse captured or fullscreen - relative mouse
     * mode, we shoult move pointer to the center (_captureMouse() do it). */
    if ((pGrop->hptrPointer == NULLHANDLE) &&
        (pGrop->fFullscreen || pGrop->fCapture)) {
      dbgprintf("Relative mouse mode, call _captureMouse()\n");
      _captureMouse(pGrop, TRUE);
    }
    break;

  case QUERY_SET_CAPTURE:
    if (pGrop->fCapture != pQuery->fCapture) {
      dbgprintf("QUERY_SET_CAPTURE: %s\n", pQuery->fCapture ? "TRUE" : "FALSE");
      pGrop->fCapture = pQuery->fCapture;

      if (!pGrop->fFullscreen) /* Mouse already is captured in fullscreen. */
          _captureMouse(pGrop, pQuery->fCapture);
    }
    pQuery->fSuccess = TRUE;
    break;

  case QUERY_MINIMIZE:
    pQuery->fSuccess = WinSetWindowPos(pGrop->hwndFrame, HWND_BOTTOM,
                                       0, 0, 0, 0, SWP_MINIMIZE | SWP_DEACTIVATE);
    break;

  case QUERY_MOUSEMOVE:
    pQuery->fSuccess = _queryMouseMove(pGrop, pQuery->pointlPos.x,
                                       pQuery->pointlPos.y);
    break;

  case QUERY_UPDATE:
    /* Do not update when fullscreen but we are on the desktop now. */
    pQuery->fSuccess =
         (pGrop->fFullscreen && !pGrop->fActive) ||
         ((pGrop->ulModeIdx != ~0) &&
           pGrop->pVideoSys->fnUpdate(pGrop->pVSData, pGrop,
                           pQuery->GQUPDATE.cRect, pQuery->GQUPDATE.prectl));
    break;

  case QUERY_SET_PALETTER:
    pQuery->fSuccess = pGrop->pVideoSys->fnSetPalette(pGrop->pVSData,
                                                   pQuery->GQSETPAL.ulFirst,
                                                   pQuery->GQSETPAL.ulNumber,
                                                   pQuery->GQSETPAL.pColors);
    break;

  case QUERY_SET_TITLE:
    pQuery->fSuccess = WinSetWindowText(pGrop->hwndFrame, pQuery->pszText);
    break;

  case QUERY_SET_ICON:
    pQuery->fSuccess = (BOOL)WinSendMsg(pGrop->hwndFrame, WM_SETICON,
                                        MPFROMLONG(pQuery->hptrPointer), 0);
    break;

  default:
    dbgprintf("Unknown query %u\n", pQuery->ulQuery);
    pQuery->fSuccess = FALSE;
  }

  dbgprintf("Done, result: %s\n", pQuery->fSuccess ? "TRUE" : "FALSE");
  if (pQuery->hevReady != NULLHANDLE)
    DosPostEventSem(pQuery->hevReady);
}

static VOID _wmActive(PGROPDATA pGrop, BOOL fActive)
{
  if (pGrop->fFullscreen) {
    if (fActive) {
      _setFullscreen(pGrop, pGrop->ulModeIdx);
      pGrop->pVideoSys->fnUpdate(pGrop->pVSData, pGrop, 0, NULL);
    }
    else {
      _setDesktop(pGrop);

      _captureMouse(pGrop, FALSE);
      WinShowWindow(pGrop->hwndFrame, FALSE);
    }
  }
  else
    _captureMouse(pGrop, pGrop->fCapture && fActive);

  pGrop->fActive = fActive;

  if (pGrop->stCallback.fnActive != NULL)
    pGrop->stCallback.fnActive(pGrop, GROUP_ACTIVATE_WINDOW, fActive);
}

static VOID _wmMouseMove(PGROPDATA pGrop, SHORT lX, SHORT lY)
{
  POINTL  pointl;

  if ((pGrop->hptrPointer == NULLHANDLE) &&
      (pGrop->fCapture /*|| pGrop->fFullscreen*/)) {
    /* Unvisible pointer when mouse captured /or fullscreen/ - relative mouse
     * mode.  Send relative mouse coordinates (offset) to the application. */

    /* Mouse pointer is unvisible. */
    WinSetPointer(pGrop->hwndDT, NULLHANDLE);

    /* Internal information about pointer in relative coordinates mode. */
    pGrop->fMouseInViewArea = TRUE;
    pGrop->ulMouseX = pGrop->stUserMode.ulWidth / 2;
    pGrop->ulMouseY = pGrop->stUserMode.ulHeight / 2;

    /* Center of the window in window coordinates. */
    pointl.x = (pGrop->rectlWinArea.xRight - pGrop->rectlWinArea.xLeft) / 2;
    pointl.y = (pGrop->rectlWinArea.yTop - pGrop->rectlWinArea.yBottom) / 2;

    /* Call user's function with relative coordinates. */
    if (pGrop->stCallback.fnMouseMove != NULL)
      pGrop->stCallback.fnMouseMove(pGrop, TRUE,
                                    lX - pointl.x, lY - pointl.y);

    /* Keep mouse at center of the window. */

    /* Center of the window in screen coordinates. */
    pointl.x = (pGrop->rectlWinArea.xRight + pGrop->rectlWinArea.xLeft) / 2;
    pointl.y = (pGrop->rectlWinArea.yTop + pGrop->rectlWinArea.yBottom) / 2;
    /* Do not handle mouse move message when we moves to center of the window. */
    pGrop->lSkipMouseMove++;
    if (!WinSetPointerPos(pGrop->hwndDT, pointl.x, pointl.y))
      dbgprintf("WinSetPointerPos() failed\n");

    return;
  }

  if (pGrop->fFullscreen || (pGrop->fCapture && pGrop->fActive)) {
    /* Restrict the movement of the mouse view area. */
    SHORT lXva = lX, lYva = lY;

    if (lX < pGrop->rectlViewArea.xLeft)
      lX = pGrop->rectlViewArea.xLeft;
    else if (lX >= pGrop->rectlViewArea.xRight)
      lX = pGrop->rectlViewArea.xRight - 1;
    if (lY < pGrop->rectlViewArea.yBottom)
      lY = pGrop->rectlViewArea.yBottom;
    else if (lY >= pGrop->rectlViewArea.yTop)
      lY = pGrop->rectlViewArea.yTop - 1;

    if (lX != lXva || lY != lYva) {
      pointl.x = lX;
      pointl.y = lY;
      WinMapWindowPoints(pGrop->hwnd, pGrop->hwndDT, &pointl, 1);
      pGrop->lSkipMouseMove++;
      WinSetPointerPos(pGrop->hwndDT, pointl.x, pointl.y);
    }
  }

  /* Set application pointer when mouse on the view area or system pointer
   * when it on "unused" space of window (around view area in our window). */
  pointl.x = lX;
  pointl.y = lY;
  WinSetPointer(pGrop->hwndDT,
                WinPtInRect(pGrop->hab, &pGrop->rectlViewArea, &pointl)
                  ? pGrop->hptrPointer
                  : WinQuerySysPointer(HWND_DESKTOP, SPTR_ARROW, FALSE));

  pGrop->fMouseInViewArea = 
    lX >= pGrop->rectlViewArea.xLeft && lX < pGrop->rectlViewArea.xRight &&
    lY >= pGrop->rectlViewArea.yBottom && lY < pGrop->rectlViewArea.yTop;

  if (pGrop->fMouseInViewArea) {
    ULONG ulVAWidth  = pGrop->rectlViewArea.xRight - pGrop->rectlViewArea.xLeft;
    ULONG ulVAHeight = pGrop->rectlViewArea.yTop - pGrop->rectlViewArea.yBottom;

    /* Set coordinates relative to the bottom-left corner of the view area. */
    lX -= pGrop->rectlViewArea.xLeft;
    lY -= pGrop->rectlViewArea.yBottom;

    /* Scale coordinates to the user work area. */
    lX = pGrop->stUserMode.ulWidth * lX / ulVAWidth;
    lY = pGrop->stUserMode.ulHeight * lY / ulVAHeight;

    if (pGrop->ulMouseX != lX || pGrop->ulMouseY != lY) {
      /* Coordinates hava been changed. */
      pGrop->ulMouseX = lX;
      pGrop->ulMouseY = lY;

      /* Call user's function. */
      if (pGrop->stCallback.fnMouseMove != NULL)
        pGrop->stCallback.fnMouseMove(pGrop, FALSE, lX, lY);
    }
  }
}

static VOID _wmMouseButton(PGROPDATA pGrop, ULONG ulButton, BOOL fDown)
{
  if ((pGrop->stCallback.fnMouseButton != NULL) && pGrop->fMouseInViewArea)
    pGrop->stCallback.fnMouseButton(pGrop, ulButton, fDown);
}


MRESULT EXPENTRY wndFrameProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  HWND      hwndClient = WinQueryWindow(hwnd, QW_BOTTOM);
  PGROPDATA pGrop = (PGROPDATA)WinQueryWindowULong(hwndClient, 0);

  switch (msg) {
  case WM_ADJUSTFRAMEPOS:
    if (!pGrop->fInternalResize && !pGrop->pVideoSys->fFreeWinSize &&
        (pGrop->stUserMode.ulFlags & GROP_MODEFL_RESIZABLE) == 0 &&
        (((PSWP)mp1)->fl & (SWP_SIZE | SWP_MINIMIZE)) == SWP_SIZE) {
        RECTL rectl;

        rectl.xLeft = 0;
        rectl.yBottom = 0;
        rectl.xRight = pGrop->stUserMode.ulWidth;
        rectl.yTop = pGrop->stUserMode.ulHeight;
        WinCalcFrameRect(hwnd, &rectl, FALSE);

        ((PSWP)mp1)->cx = rectl.xRight - rectl.xLeft;
        ((PSWP)mp1)->cy = rectl.yTop - rectl.yBottom;
      }
    break;

  case WM_MINMAXFRAME:
    if ((((PSWP)mp1)->fl & SWP_MINIMIZE) != 0)
      /* To avoid call GROPCALLBACK.fnSize() on window minimization... */
      pGrop->lSkipWinResize++;

    if ((((PSWP)mp1)->fl & SWP_MAXIMIZE) != 0 && !pGrop->fInternalResize &&
           !pGrop->pVideoSys->fFreeWinSize &&
           (pGrop->stUserMode.ulFlags & GROP_MODEFL_RESIZABLE) == 0)
      ((PSWP)mp1)->fl &= ~(SWP_MAXIMIZE | SWP_MOVE);
    break;
  }

  return pGrop->fnOldFrameWinProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY wndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  PGROPDATA pGrop = (PGROPDATA)WinQueryWindowULong(hwnd, 0);

  switch (msg) {
  case WM_ACTIVATE:
    _wmActive(pGrop, (BOOL)mp1);
    break;

  case WM_SETFOCUS:
    if (pGrop->stCallback.fnActive != NULL)
      pGrop->stCallback.fnActive(pGrop, GROUP_ACTIVATE_FOCUS, LONGFROMMP(mp2));
    break;

  case WM_DESTROY:
    break;

  case WM_CLOSE:
  /*if ((pGrop->stCallback.fnClose != NULL) &&
        !pGrop->stCallback.fnClose(pGrop))
      return (MRESULT)FALSE;*/

    if (pGrop->fFullscreen && pGrop->fActive)
      _setDesktop(pGrop);

    _captureMouse(pGrop, FALSE);
    break;

  case WM_PAINT: {
    RECTL rctl;
    HPS   hps;

    hps = WinBeginPaint(hwnd, 0, &rctl);
        WinFillRect(hps, &rctl, CLR_BLACK);
        WinEndPaint(hps);

    if (pGrop->fInternalResize)
      dbgprintf("WM_PAINT & fInternalResize\n");
    /*else*/
      pGrop->pVideoSys->fnUpdate(pGrop->pVSData, pGrop, 0, NULL);
    }
    return (MRESULT)FALSE;

  case WM_SIZE:
    if (pGrop->lSkipWinResize > 0) {
      pGrop->lSkipWinResize--;
      break;
    }

    if (!pGrop->fFullscreen && !pGrop->fInternalResize && pGrop->fActive &&
        (pGrop->stUserMode.ulFlags & GROP_MODEFL_RESIZABLE) != 0 &&
        pGrop->stCallback.fnSize != NULL)
      pGrop->stCallback.fnSize(pGrop, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2));
    break;

  case WM_VRNENABLED:
    _calcViewRect(pGrop, FALSE);
    _setVisibleRegion(pGrop, TRUE);
    return (MRESULT)TRUE;

  case WM_VRNDISABLED:
    _setVisibleRegion(pGrop, FALSE);
    return (MRESULT)TRUE;

  case WM_MOUSEMOVE:
    if (pGrop->lSkipMouseMove > 0)
      pGrop->lSkipMouseMove--;
    else {
      if (pGrop->fInternalResize)
        dbgprintf("WM_MOUSEMOVE & fInternalResize\n");
      _wmMouseMove(pGrop, SHORT1FROMMP(mp1), SHORT2FROMMP(mp1));
    }
    return (MRESULT)FALSE;

  case WM_BUTTON1DOWN:
  case WM_BUTTON1DBLCLK:
    _wmMouseButton(pGrop, 0, TRUE);
    break;

  case WM_BUTTON1UP:
    _wmMouseButton(pGrop, 0, FALSE);
    break;

  case WM_BUTTON2DOWN:
  case WM_BUTTON2DBLCLK:
    _wmMouseButton(pGrop, 1, TRUE);
    break;

  case WM_BUTTON2UP:
    _wmMouseButton(pGrop, 1, FALSE);
    break;

  case WM_BUTTON3DOWN:
  case WM_BUTTON3DBLCLK:
    _wmMouseButton(pGrop, 2, TRUE);
    break;

  case WM_BUTTON3UP:
    _wmMouseButton(pGrop, 2, FALSE);
    break;

  case WM_TRANSLATEACCEL:
    /* ALT and acceleration keys not allowed (must be processed in WM_CHAR) */
    if (mp1 == NULL || ((PQMSG)mp1)->msg != WM_CHAR)
      break;
    return (MRESULT)FALSE;

  case WM_CHAR:
    if (pGrop->stCallback.fnKeyboard != NULL) {
      ULONG ulFlags = SHORT1FROMMP(mp1); /* WM_CHAR flags */

      pGrop->stCallback.fnKeyboard(
          pGrop, (ulFlags & KC_SCANCODE) == 0 ? 0 : CHAR4FROMMP(mp1),
          (ulFlags & KC_CHAR) == 0 ? 0 : SHORT1FROMMP(mp2), ulFlags);
    }
    return (MRESULT)TRUE;

  case WM_GROP_QUERY:
    _wmGropQuery(pGrop, (PGROPQUERY)PVOIDFROMMP(mp1));
    return (MRESULT)TRUE;
  }

  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}


void FAR wndThread(void FAR *pData)
{
  PGROPDATA pGrop = (PGROPDATA)pData;
  PTIB      tib;
  PPIB      pib;
  ULONG     ulFrameFlags;
  QMSG      qmsg;
  SWP       swp;
  PVIDEOSYS pVideoSys = aVideoSys[pGrop->ulVideoSysIdx];

  /* Change process type code for use Win* API from VIO session */
  DosGetInfoBlocks(&tib, &pib);
  if (pib->pib_ultype == 2 || pib->pib_ultype == 0) {
    /* VIO windowable or fullscreen protect-mode session. */
    pib->pib_ultype = 3; /* Presentation Manager protect-mode session. */
    /* ...and switch to the desktop (if we are in fullscreen now)? */
  }

  /* Select output video system. */
  if ((pGrop->ulVideoSysIdx < VIDEOSYSNUM) &&
      pVideoSys->fnInit(&pGrop->stModes, &pGrop->pVSData)) {
    dbgprintf("Requested video system %u initialized.\n", pGrop->ulVideoSysIdx);
    pGrop->pVideoSys = pVideoSys;
  }
  else {
    ULONG ulIdx;

    dbgprintf("Requested video system was not initialized. Try others...\n");
    pGrop->pVideoSys = NULL;
    for (ulIdx = 0; ulIdx < VIDEOSYSNUM; ulIdx++) {
      if (pGrop->ulVideoSysIdx == ulIdx)
        /* Ignore video system requested by the user (already verifed). */
        continue;

      pVideoSys = aVideoSys[ulIdx];
      if (pVideoSys->fnInit(&pGrop->stModes, &pGrop->pVSData)) {
        dbgprintf("Video system %u initialized.\n", ulIdx);
        pGrop->pVideoSys = pVideoSys;
        pGrop->ulVideoSysIdx = ulIdx;
        break;
      }
    }
  } /* if ((pGrop->ulVideoSysIdx < VIDEOSYSNUM) && pVideoSys->fnInit()) else */

  if (pGrop->pVideoSys == NULL) {
    dbgprintf("Video system is not initialized.\n");
  }
  else {
    /* Output video system selected. */

    /* Prepare PM stuff. */
    pGrop->hab = WinInitialize(0);
    pGrop->hmq = WinCreateMsgQueue(pGrop->hab, 0);
    pGrop->hwndDT = WinQueryDesktopWindow(
                      WinQueryAnchorBlock(HWND_DESKTOP), 0);
    pGrop->hdcDT = WinQueryWindowDC(pGrop->hwndDT);
    WinQueryWindowPos(pGrop->hwndDT, &swp);
    pGrop->ulDTWidth = swp.cx;
    pGrop->ulDTHeight = swp.cy;
    pGrop->ulModeIdx = ~0; /* ~0 - Mode is not selected yet. */
    pGrop->hptrPointer = WinQuerySysPointer(HWND_DESKTOP, SPTR_ARROW, FALSE);

    WinRegisterClass(pGrop->hab, WIN_CLIENT_CLASS, wndProc,
                     CS_SIZEREDRAW | CS_MOVENOTIFY | CS_SYNCPAINT,
                     sizeof(PGROPDATA));

    /* Create the window. */
    ulFrameFlags = FCF_TASKLIST | FCF_DLGBORDER | FCF_TITLEBAR |
                   FCF_SYSMENU | FCF_MINBUTTON | FCF_SHELLPOSITION |
                   FCF_SIZEBORDER | FCF_MINBUTTON | FCF_MAXBUTTON;
    pGrop->hwndFrame = WinCreateStdWindow(HWND_DESKTOP, 0, &ulFrameFlags,
                           WIN_CLIENT_CLASS, "grop", 0, 0, 1, &pGrop->hwnd);
    if (pGrop->hwndFrame == NULLHANDLE) {
      dbgprintf("WinCreateStdWindow() failed\n");
    }
    else {
      WinSetWindowULong(pGrop->hwnd, 0, (ULONG)pGrop);
      /* Subclass frame window to control size changes. */
      pGrop->fnOldFrameWinProc = WinSubclassWindow(pGrop->hwndFrame,
                                                   wndFrameProc);
      WinSetVisibleRegionNotify(pGrop->hwnd, TRUE);

      if (DosCreateMutexSem(NULL, &pGrop->hmtxData, 0, FALSE) != NO_ERROR)
        dbgprintf("DosCreateMutexSem() failed\n");
      else {
        ULONG ulRC;

        ulRC = DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 31, 0);
        if (ulRC != NO_ERROR)
          dbgprintf("DosSetPriority(), rc = %u\n", ulRC);

        ulRC = DosPostEventSem(pGrop->hevReady); /* Ready to work. */
        if (ulRC != NO_ERROR)
          dbgprintf("DosPostEventSem(), rc = %u\n", ulRC);

        while (TRUE) {
          /* Work... */
          while (WinGetMsg(pGrop->hab, &qmsg, (HWND)NULLHANDLE, 0L, 0L)) {
            ulRC = DosRequestMutexSem(pGrop->hmtxData, 500);
            if (ulRC != NO_ERROR)
              dbgprintf("DosRequestMutexSem(), rc = %u\n", ulRC);

            WinDispatchMsg(pGrop->hab, &qmsg);
            DosReleaseMutexSem(pGrop->hmtxData);
          }

          if (pGrop->stCallback.fnQuit == NULL)
            break;

          dbgprintf("Call stCallback.fnQuit()\n");
          pGrop->stCallback.fnQuit(pGrop);
        }

        /* Return to the desktop if we are in fullscreen. */
        dbgprintf("Fullscreen: %u\n", pGrop->fFullscreen);
        if (pGrop->fFullscreen) {
          PVIDEOMODE pMode = &pGrop->stModes.pList[pGrop->stModes.ulDesktopMode];

          dbgprintf("Return to the desktop...\n");
          pGrop->pVideoSys->fnSetMode(pGrop->pVSData, pMode, FALSE,
                                      pGrop->hwndDT, pGrop->hdcDT);
          dbgprintf("Returned to the desktop\n");
        }

        DosCloseMutexSem(pGrop->hmtxData);
      }

      dbgprintf("Destroy window\n");
      if (!WinDestroyWindow(pGrop->hwnd))
        dbgprintf("WinDestroyWindow() failed\n");
      pGrop->hwnd = NULLHANDLE;
      pGrop->hwndFrame = NULLHANDLE;
    }

    /* Destroy PM stuff. */
    dbgprintf("WinDestroyMsgQueue()...\n");
    WinDestroyMsgQueue(pGrop->hmq);
    dbgprintf("WinTerminate()...\n");
    WinTerminate(pGrop->hab);
    pGrop->hwndFrame = NULLHANDLE;
    pGrop->hwnd = NULLHANDLE;

    /* Destroy output video system. */
    if (pGrop->stUserMode.pBuffer != NULL) {
      dbgprintf("Free video system's buffer\n");
      pGrop->pVideoSys->fnVideoBufFree(pGrop->pVSData,
                                       pGrop->stUserMode.pBuffer);
      pGrop->stUserMode.pBuffer = NULL;
    }
    dbgprintf("Close video system\n");
    pGrop->pVideoSys->fnDone(pGrop->pVSData, &pGrop->stModes);
    pGrop->pVideoSys = NULL;
  } /* if (pGrop->pVideoSys == NULL) else */

  pGrop->tid = ((TID)(-1));
  dbgprintf("Post semaphore...\n");
  DosPostEventSem(pGrop->hevReady); /* Finalized. */
  dbgprintf("Exit thread.\n");
  _endthread();
}


/* BOOL _sendQuery(PGROPDATA pGrop, PGROPQUERY pQuery, BOOL fPrivateEvSem)
 *
 * Sends query to GROP via window message, waits for answer and returns result.
 */
static BOOL _sendQuery(PGROPDATA pGrop, PGROPQUERY pQuery, BOOL fPrivateEvSem)
{
  ULONG ulRC;
  BOOL  fSuccess;
  PTIB  tib;
  PPIB  pib;

  DosGetInfoBlocks(&tib, &pib);
  if (tib->tib_ptib2->tib2_ultid == pGrop->tid) {
    /* Query send from callback function. */
    dbgprintf("Query send from callback function.\n");

    pQuery->hevReady = NULLHANDLE;
    _wmGropQuery(pGrop, pQuery);
    return pQuery->fSuccess;
  /*return (BOOL)WinSendMsg(pGrop->hwnd, WM_GROP_QUERY, MPFROMP(pQuery), 0)
           && pQuery->fSuccess;*/
  }

  if (fPrivateEvSem) {
    ulRC = DosCreateEventSem(NULL, &pQuery->hevReady, 0, FALSE);
    if (ulRC != NO_ERROR) {
      dbgprintf("DosCreateEventSem(), rc = %u.\n", ulRC);
      return FALSE;
    }
  }
  else
    pQuery->hevReady = pGrop->hevReady;

  fSuccess = WinPostMsg(pGrop->hwnd, WM_GROP_QUERY, MPFROMP(pQuery), 0);
  if (!fSuccess) {
    dbgprintf("WinPostMsg() failed\n");
  }
  else {
    ulRC = DosWaitEventSem(pQuery->hevReady, 3000);
    if (ulRC != NO_ERROR) {
      dbgprintf("DosWaitEventSem(), rc = %u.\n", ulRC);
    }
    fSuccess = pQuery->fSuccess;
  }

  if (fPrivateEvSem)
    DosCloseEventSem(pQuery->hevReady);

  return fSuccess;
}

/* List of GROP objects.
 * ---------------------
 *
 * It is necessary to avoid situations when the program is terminated in full
 * screen mode. We will collect all open GROP objects to try to control it.
 */

static HMTX     hmtxGropList = NULLHANDLE;
static LINKSEQ  lsGropList;
static LONG     lInitTime = 0;

static VOID _gropAllToDesktop(void)
{
  PGROPDATA pGrop;

  if (hmtxGropList == NULLHANDLE)
    return;

  dbgprintf("%u GROP objects was not destroyed.\n",
            lnkseqGetCount(&lsGropList));
  DosRequestMutexSem(hmtxGropList, SEM_INDEFINITE_WAIT);

  for (pGrop = (PGROPDATA)lnkseqGetFirst(&lsGropList); pGrop != NULL;
       pGrop = (PGROPDATA)lnkseqGetNext(pGrop))
    gropSetFullscreen(pGrop, _GROP_SET_FS_PANIC_OFF);

  DosReleaseMutexSem(hmtxGropList);
}


/* Public routines.
 * ----------------
 */

/* BOOL gropInit()
 *
 * Module initialization. Returns TRUE on success.
 */
BOOL gropInit(void)
{
  ULONG ulRC;

  if (lInitTime == 0) {
    lnkseqInit(&lsGropList);
    ulRC = DosCreateMutexSem(NULL, &hmtxGropList, 0, FALSE);
    if (ulRC != NO_ERROR) {
       dbgprintf("DosCreateMutexSem(), rc = %u\n", ulRC);
       return FALSE;
    }

    /* In cases where gropDone() will/was not called by user. Do not destroy
     * objects, because gropDone() can be called later - atexit() can be used
     * by the user too. Switch to the desktop only. */
    atexit(_gropAllToDesktop);
  }
  lInitTime++;
  return TRUE;
}

VOID gropDone(void)
{
  if (lInitTime == 0)
    return;

  lInitTime--;
  if (lInitTime == 0) {
    dbgprintf("%u GROP objects left.\n", lnkseqGetCount(&lsGropList));
    _gropAllToDesktop();
    DosCloseMutexSem(hmtxGropList);
    hmtxGropList = NULLHANDLE;
  }
}

PGROPDATA gropNew(ULONG ulVideoSys, PGROPCALLBACK pCallback, PVOID pUser)
{
  ULONG     ulRC;
  PGROPDATA pGrop;

  if (hmtxGropList == NULLHANDLE) {
    dbgprintf("Module was not initialized.\n");
    return NULL;
  }

  pGrop = SDL_calloc(1, sizeof(GROPDATA));
  if (pGrop == NULL) {
    dbgprintf("Not enough memory.\n");
    return NULL;
  }

  pGrop->ulVideoSysIdx = ulVideoSys;
  pGrop->stCallback = *pCallback;
  pGrop->pUser = pUser;
  do {
    ulRC = DosCreateEventSem(NULL, &pGrop->hevReady, DCE_POSTONE, FALSE);
    if (ulRC != NO_ERROR) {
      dbgprintf("DosCreateEventSem(), rc = %u.\n", ulRC);
      break;
    }

    pGrop->tid = _beginthread(wndThread, NULL, 65536, (PVOID)pGrop);
    if (pGrop->tid == -1) {
      dbgprintf("_beginthread() failed.\n");
      break;
    }

    ulRC = DosWaitEventSem(pGrop->hevReady, 5000);
    if (ulRC == NO_ERROR) {
      if (pGrop->pVideoSys == NULL)
        break;

      /* Add new GROP object to the list. */
      ulRC = DosRequestMutexSem(hmtxGropList, SEM_INDEFINITE_WAIT);
      if (ulRC != NO_ERROR)
        dbgprintf("DosRequestMutexSem(), rc = %u\n", ulRC);
      lnkseqAdd(&lsGropList, pGrop);
      DosReleaseMutexSem(hmtxGropList);

      dbgprintf("Success.\n");
      return pGrop;
    }

    dbgprintf("DosWaitEventSem(), rc = %u.\n", ulRC);
    DosKillThread(pGrop->tid);
  }
  while (FALSE);

  DosCloseEventSem(pGrop->hevReady);
  SDL_free(pGrop);

  return NULL;
}

VOID gropFree(PGROPDATA pGrop)
{
  ULONG ulRC;
  HMQ   hmq;
  TID   tid;

  ulRC = DosRequestMutexSem(pGrop->hmtxData, 4000);
  if (ulRC != NO_ERROR)
    dbgprintf("DosRequestMutexSem(), rc = %u\n", ulRC);

  hmq = pGrop->hmq;
  tid = pGrop->tid;
  /* Tune off all callback functions. */
  SDL_memset(&pGrop->stCallback, 0, sizeof(GROPCALLBACK));
  DosReleaseMutexSem(pGrop->hmtxData);

  if (hmq == NULLHANDLE || !WinPostQueueMsg(hmq, WM_QUIT, 0, 0)) {
    dbgprintf("Hm... have not a thread?...\n");
    if (pGrop->fFullscreen)
      _setMode(pGrop, pGrop->ulModeIdx, FALSE);

    if (tid != (TID)(-1)) {
      dbgprintf("WTF?! %p %d\n", pGrop, tid);
      DosKillThread(tid);
    }
  }
  else if (DosWaitThread(&tid, DCWW_NOWAIT) != NO_ERROR) {
    dbgprintf("Wait thread semaphore\n");
    ulRC = DosWaitEventSem(pGrop->hevReady, 4000);
    if (ulRC != NO_ERROR) {
      PVIDEOMODE pMode = &pGrop->stModes.pList[pGrop->stModes.ulDesktopMode];

      dbgprintf("DosWaitEventSem(), rc = %u. Kill thread...\n", ulRC);
      DosKillThread(tid);

      if (pGrop->hwndDT != NULLHANDLE) {
        dbgprintf("Return to the desktop...\n");
        pGrop->pVideoSys->fnSetMode(pGrop->pVSData, pMode, FALSE,
                                    pGrop->hwndDT, pGrop->hdcDT);
      }
    }
    else {
      dbgprintf("Wait thread\n");
      DosWaitThread(&tid, DCWW_WAIT);
    }
  }
  else
    dbgprintf("Thread already terminated.\n");

  /* Remove GROP object from the list. */
  ulRC = DosRequestMutexSem(hmtxGropList, SEM_INDEFINITE_WAIT);
  if (ulRC != NO_ERROR)
    dbgprintf("DosRequestMutexSem(), rc = %u\n", ulRC);
  lnkseqRemove(&lsGropList, pGrop);
  DosReleaseMutexSem(hmtxGropList);

  DosCloseEventSem(pGrop->hevReady);
  SDL_free(pGrop);
  dbgprintf("Success.\n");
}

BOOL gropSetMode(PGROPDATA pGrop, PGROPSETMODE pUserMode)
{
  GROPQUERY stQuery;

  stQuery.ulQuery = QUERY_SET_MODE;
  stQuery.pUserMode = pUserMode;

  return _sendQuery(pGrop, &stQuery, TRUE);
}

BOOL gropSetFullscreen(PGROPDATA pGrop, ULONG ulSetFullscreen)
{
  GROPQUERY  stQuery;

  stQuery.ulQuery = QUERY_SET_FS;
  stQuery.ulSetFullscreen = ulSetFullscreen;

  return _sendQuery(pGrop, &stQuery, TRUE);
}

BOOL gropSetPointer(PGROPDATA pGrop, HPOINTER hptrPointer)
{
  GROPQUERY stQuery;

  stQuery.ulQuery = QUERY_SET_POINTER;
  stQuery.hptrPointer = hptrPointer;

  return _sendQuery(pGrop, &stQuery, TRUE);
}

BOOL gropCapture(PGROPDATA pGrop, BOOL fCapture)
{
  GROPQUERY stQuery;

  stQuery.ulQuery = QUERY_SET_CAPTURE;
  stQuery.fCapture = fCapture;

  return _sendQuery(pGrop, &stQuery, TRUE);
}

BOOL gropMinimize(PGROPDATA pGrop)
{
  GROPQUERY stQuery;

  stQuery.ulQuery = QUERY_MINIMIZE;

  return _sendQuery(pGrop, &stQuery, TRUE);
}

BOOL gropMouseMove(PGROPDATA pGrop, LONG lX, LONG lY)
{
  GROPQUERY stQuery;

  stQuery.ulQuery = QUERY_MOUSEMOVE;
  stQuery.pointlPos.x = lX;
  stQuery.pointlPos.y = lY;

  return _sendQuery(pGrop, &stQuery, FALSE);
}

BOOL gropGetMousePos(PGROPDATA pGrop, PULONG pulX, PULONG pulY)
{
  ULONG ulRC;

  ulRC = DosRequestMutexSem(pGrop->hmtxData, SEM_INDEFINITE_WAIT);
  if (ulRC != NO_ERROR) {
    dbgprintf("DosRequestMutexSem(), rc = %u\n", ulRC);
    return FALSE;
  }

  *pulX = pGrop->ulMouseX;
  *pulY = pGrop->ulMouseY;
  DosReleaseMutexSem(pGrop->hmtxData);

  return TRUE;
}

BOOL gropUpdate(PGROPDATA pGrop, ULONG cRect, PRECTL prectl)
{
#if 1
  /* Fast update (direct call video system function). */
  BOOL  fSuccess;
  ULONG ulRC;

  ulRC = DosRequestMutexSem(pGrop->hmtxData, 1000);
  if (ulRC != NO_ERROR) {
    dbgprintf("DosRequestMutexSem(), rc = %u\n", ulRC);
    return FALSE;
  }

  fSuccess = (pGrop->fFullscreen && !pGrop->fActive) ||
             (pGrop->ulModeIdx != ~0 &&
              pGrop->pVideoSys->fnUpdate(pGrop->pVSData, pGrop, cRect, prectl));
  DosReleaseMutexSem(pGrop->hmtxData);

  return fSuccess;
#else
  /* Update query. */
  GROPQUERY stQuery;

  stQuery.ulQuery = QUERY_UPDATE;
  stQuery.GQUPDATE.cRect = cRect;
  stQuery.GQUPDATE.prectl = prectl;

  return _sendQuery(pGrop, &stQuery, FALSE);
#endif
}

BOOL gropSetPaletter(PGROPDATA pGrop, ULONG ulFirst, ULONG ulNumber,
                     PRGB2 pColors)
{
  GROPQUERY stQuery;

  stQuery.ulQuery = QUERY_SET_PALETTER;
  stQuery.GQSETPAL.ulFirst = ulFirst;
  stQuery.GQSETPAL.ulNumber = ulNumber;
  stQuery.GQSETPAL.pColors = pColors;

  return _sendQuery(pGrop, &stQuery, FALSE);
}

BOOL gropSetWindowTitle(PGROPDATA pGrop, PSZ pszText)
{
  GROPQUERY stQuery;

  stQuery.ulQuery = QUERY_SET_TITLE;
  stQuery.pszText = pszText;

  return _sendQuery(pGrop, &stQuery, TRUE);
}

BOOL gropSetWindowIcon(PGROPDATA pGrop, HPOINTER hptrIcon)
{
  GROPQUERY stQuery;

  stQuery.ulQuery = QUERY_SET_ICON;
  stQuery.hptrPointer = hptrIcon;

  return _sendQuery(pGrop, &stQuery, TRUE);
}

BOOL gropClose(PGROPDATA pGrop)
{
  return WinPostMsg(pGrop->hwnd, WM_CLOSE, 0, 0);
}

BOOL gropLock(PGROPDATA pGrop)
{
  ULONG ulRC;

  ulRC = DosRequestMutexSem(pGrop->hmtxData, SEM_INDEFINITE_WAIT);
  if (ulRC != NO_ERROR) {
    dbgprintf("DosRequestMutexSem(), rc = %u\n", ulRC);
    return FALSE;
  }

  return TRUE;
}

VOID gropUnlock(PGROPDATA pGrop)
{
  DosReleaseMutexSem(pGrop->hmtxData);
}

#endif /* SDL_VIDEO_DRIVER_OS2GROP */
