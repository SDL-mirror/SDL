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

#include "SDL_error.h"
#include "SDL_stdinc.h"

#define INCL_DOSERRORS
#define INCL_DOSPROCESS
#define INCL_DOSMODULEMGR
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIBITMAPS /* GPI bit map functions */
#include <os2.h>

#include "SDL_gradd.h"
#include "SDL_grop.h"

#define _USE_EXIT_LIST

static BOOL vsInit(PVIDEOMODESLIST pModes, PVOID *ppVSData);
static BOOL vsInitCompatible(PVIDEOMODESLIST pModes, PVOID *ppVSData);
static VOID vsDone(PVOID pVSData, PVIDEOMODESLIST pModes);
static BOOL vsSetMode(PVOID pVSData, PVIDEOMODE pMode, BOOL fFullscreen,
                      HWND hwndDT, HDC hdcDT);
static BOOL vsSetModeCompatible(PVOID pVSData, PVIDEOMODE pMode,
                                BOOL fFullscreen, HWND hwndDT, HDC hdcDT);
static PVOID vsVideoBufAlloc(PVOID pVSData, ULONG ulWidth, ULONG ulHeight,
                             ULONG ulBPP, ULONG fccColorEncoding,
                             PULONG pulScanLineSize);
static VOID vsVideoBufFree(PVOID pVSData, PVOID pBuffer);
static VOID vsSetVisibleRegion(PVOID pVSData, HWND hwnd, PGROPSETMODE pUserMode,
                             PRECTL prectlWin, PRECTL prectlVA, BOOL fVisible);
static BOOL vsUpdate(PVOID pVSData, PGROPDATA pGrop, ULONG cRect, PRECTL prectl);
static BOOL vsSetPalette(PVOID pVSData, ULONG ulFirst, ULONG ulNumber,
                         PRGB2 pColors);

VIDEOSYS stVideoSysVMANCompatible = {
  FALSE,    /* fFreeWinSize */
  FALSE,    /* fBPPConvSup  */
  vsInitCompatible,
  vsDone,
  vsSetModeCompatible,
  vsVideoBufAlloc,
  vsVideoBufFree,
  vsSetVisibleRegion,
  vsUpdate,
  vsSetPalette
};

VIDEOSYS stVideoSysVMAN = {
  FALSE,    /* fFreeWinSize */
  FALSE,    /* fBPPConvSup  */
  vsInit,
  vsDone,
  vsSetMode,
  vsVideoBufAlloc,
  vsVideoBufFree,
  vsSetVisibleRegion,
  vsUpdate,
  vsSetPalette
};

typedef struct _VMANDATA {
  HRGN       hrgnVisible;
} VMANDATA, *PVMANDATA;

static HMODULE      hmodVMan = NULLHANDLE;
static FNVMIENTRY  *pfnVMIEntry = NULL;
static ULONG        ulVRAMAddress = 0;
static LONG         lVManInit = 0;


#ifdef _USE_EXIT_LIST
VOID APIENTRY ExitVMan(VOID)
{
  if (ulVRAMAddress != 0 && hmodVMan != NULLHANDLE) {
    pfnVMIEntry(0, VMI_CMD_TERMPROC, NULL, NULL);
    DosFreeModule(hmodVMan);
  }

  DosExitList(EXLST_EXIT, (PFNEXITLIST)NULL);
}
#endif


/* VOID _clearScreen(PVIDEOMODE pMode)
 *
 * Cleans (fill black color) screen in fullscreen mode
 */
static VOID _clearScreen(PVIDEOMODE pMode)
{
  LINEINFO  sLineInfo = { 0 };
  BMAPINFO  bmiDst;
  LINEPACK  sLine = { 0 };
  RECTL     rclBounds;
  HWREQIN   sHWReqIn;
  ULONG     ulIdx;

  bmiDst.ulLength = sizeof(BMAPINFO);
  bmiDst.ulType = BMAP_VRAM;
  bmiDst.pBits = (PBYTE)ulVRAMAddress;
  bmiDst.ulWidth = pMode->ulWidth;
  bmiDst.ulHeight = pMode->ulHeight;
  bmiDst.ulBpp = pMode->ulBPP;
  bmiDst.ulBytesPerLine = pMode->ulScanLineSize;

  rclBounds.xLeft = 0;
  rclBounds.yBottom = 0;
  rclBounds.xRight = bmiDst.ulWidth;
  rclBounds.yTop = bmiDst.ulHeight;

  sHWReqIn.ulLength = sizeof(HWREQIN);
  sHWReqIn.ulFlags = REQUEST_HW;
  sHWReqIn.cScrChangeRects = 1;
  sHWReqIn.arectlScreen = &rclBounds;

  if (pfnVMIEntry(0, VMI_CMD_REQUESTHW, &sHWReqIn, NULL) != RC_SUCCESS) {
    dbgprintf("pfnVMIEntry: VMI_CMD_REQUESTHW failed\n");
    return;
  }

  sLine.ulFlags = LINE_DIR_Y_POSITIVE | LINE_DIR_X_POSITIVE |
                  LINE_HORIZONTAL | LINE_DO_FIRST_PEL | LINE_DO_LAST_PEL;
  sLine.plpkNext = NULL;
  sLine.ptlClipEnd.x = rclBounds.xRight;
  sLine.ptlEnd.x = rclBounds.xRight;

  sLineInfo.ulLength = sizeof(LINEINFO);
  sLineInfo.ulType = LINE_SOLID;
  sLineInfo.cLines = 1;
  sLineInfo.pDstBmapInfo = &bmiDst;
  sLineInfo.alpkLinePack = &sLine;
  sLineInfo.prclBounds = &rclBounds;

  for (ulIdx = 0; ulIdx < rclBounds.yTop; ulIdx++) {
    sLine.ptlClipStart.y = ulIdx;
    sLine.ptlClipEnd.y   = ulIdx;
    sLine.ptlStart.y     = ulIdx;
    sLine.ptlEnd.y       = ulIdx;
    if (pfnVMIEntry(0, VMI_CMD_LINE, &sLineInfo, NULL) != RC_SUCCESS) {
      dbgprintf("VMIEntry: VMI_CMD_LINE failed\n");
      break;
    }
  }

  sHWReqIn.ulFlags = 0;
  if (pfnVMIEntry(0, VMI_CMD_REQUESTHW, &sHWReqIn, NULL) != RC_SUCCESS)
    dbgprintf("VMIEntry: VMI_CMD_REQUESTHW failed\n");
}

/* BOOL _vmanInit()
 *
 * One-time VMAN module initialization.
 */
static BOOL _vmanInit(void)
{
  ULONG       ulRC;
  CHAR        acBuf[256];
  INITPROCOUT sInitProcOut;

  lVManInit++;
  if (lVManInit > 1)
    return TRUE;

  if (lVManInit < 1) {
    dbgprintf("WTF?!\n");
    return TRUE;
  }

  /* Load vman.dll */
  ulRC = DosLoadModule(acBuf, sizeof(acBuf), "VMAN", &hmodVMan);
  if (ulRC != NO_ERROR) {
    dbgprintf("Could not load VMAN.DLL, rc = %u : %s\n", ulRC, acBuf);
    hmodVMan = NULLHANDLE;
    return FALSE;
  }

  /* Get VMIEntry. */
  ulRC = DosQueryProcAddr(hmodVMan, 0L, "VMIEntry", (PFN *)&pfnVMIEntry);
  if (ulRC != NO_ERROR)
    dbgprintf("Could not query address of pfnVMIEntry func. of VMAN.DLL, "
              "rc = %u\n", ulRC);
  else {
    /* VMAN initialization. */
    sInitProcOut.ulLength = sizeof(sInitProcOut);
    ulRC = pfnVMIEntry(0, VMI_CMD_INITPROC, NULL, &sInitProcOut);
    if (ulRC != RC_SUCCESS)
      dbgprintf("Could not initialize VMAN for this process\n");
    else {
      /* Store video memory virtual address. */
      ulVRAMAddress = sInitProcOut.ulVRAMVirt;
#ifdef _USE_EXIT_LIST
      if (DosExitList(EXLST_ADD | 0x00001000, (PFNEXITLIST)ExitVMan)
           != NO_ERROR)
        dbgprintf("DosExitList() failed\n");
#endif
      return TRUE;
    }
  }

  DosFreeModule(hmodVMan);
  hmodVMan = NULLHANDLE;
  return FALSE;
}

/* VOID _vmanDone()
 *
 * One-time VMAN module finalization.
 */
static VOID _vmanDone(void)
{
  lVManInit--;
  if (lVManInit > 0)
    return;

  if (lVManInit < 0) {
    dbgprintf("WTF?!\n");
    lVManInit = 0;
    return;
  }

#ifndef _USE_EXIT_LIST
  if (hmodVMan != NULLHANDLE) {
    DosFreeModule(hmodVMan);
    hmodVMan = NULLHANDLE;
  }
#endif
}


/* BOOL vsInitCompatible(PVIDEOMODESLIST pModes, PVOID *ppVSData)
 * BOOL vsInit(PVIDEOMODESLIST pModes, PVOID *ppVSData)
 *
 * Output video system initialization. This function should fill VIDEOMODESLIST
 * structure pointed by pModes.
 * Returns TRUE on success.
 */
static BOOL vsInitCompatible(PVIDEOMODESLIST pModes, PVOID *ppVSData)
{
  ULONG       ulRC;
  GDDMODEINFO sCurModeInfo;
  PVMANDATA   pVMANData;

  /* VMAN module initialization. */
  if (!_vmanInit())
    return FALSE;

  pVMANData = SDL_calloc(1, sizeof(VMANDATA));
  if (pVMANData == NULL) {
    SDL_OutOfMemory ();
    dbgprintf("Not enough memory\n");
    return FALSE;
  }
  *ppVSData = pVMANData;

  pModes->ulCount = 0;
  pModes->pList = NULL;

  do {
    /* Make list of video modes. */

    /* Query current (desktop) mode. */
    ulRC = pfnVMIEntry(0, VMI_CMD_QUERYCURRENTMODE, NULL, &sCurModeInfo);
    if (ulRC != RC_SUCCESS) {
      dbgprintf("Could not query desktop video mode.\n");
      break;
    }

    /* Allocate memory for GROP modes list. Only one mode for "VMAN compatible"
     * video system - current desktop mode. */
    pModes->pList = SDL_malloc(sizeof(VIDEOMODE));
    if (pModes->pList == NULL) {
      SDL_OutOfMemory ();
      dbgprintf("Not enough memory\n");
      break;
    }

    /* Fill GROP modes list (only one mode). */
    pModes->ulCount                 = 1;
    pModes->ulDesktopMode           = 0;
    pModes->pList->ulId             = sCurModeInfo.ulModeId;
    pModes->pList->ulBPP            = sCurModeInfo.ulBpp;
    pModes->pList->ulWidth          = sCurModeInfo.ulHorizResolution;
    pModes->pList->ulHeight         = sCurModeInfo.ulVertResolution;
    pModes->pList->ulScanLineSize   = sCurModeInfo.ulScanLineSize;
    pModes->pList->fccColorEncoding = sCurModeInfo.fccColorEncoding;

    dbgprintf("%u x %u, bpp: %u\n",
              sCurModeInfo.ulHorizResolution, sCurModeInfo.ulVertResolution,
              sCurModeInfo.ulBpp);

    return TRUE;
  }
  while (FALSE);

  /* Error */
  vsDone(*ppVSData, pModes);

  return FALSE;
}

static BOOL vsInit(PVIDEOMODESLIST pModes, PVOID *ppVSData)
{
  ULONG         ulRC, ulOp;
  ULONG         ulIdx;
  PVIDEOMODE    pMode;
  GDDMODEINFO   sCurModeInfo;
  PGDDMODEINFO  pGDDModeInfList = NULL, pGDDModeInf;
  PVMANDATA     pVMANData;

  /* VMAN module initialization. */
  if (!_vmanInit())
    return FALSE;

  pVMANData = SDL_calloc(1, sizeof(VMANDATA));
  if (pVMANData == NULL) {
    SDL_OutOfMemory ();
    dbgprintf("Not enough memory\n");
    return FALSE;
  }
  *ppVSData = pVMANData;

  pModes->ulCount = 0;
  pModes->pList = NULL;

  do {
    /* Make list of video modes. */

    /* Query current (desktop) mode. */
    ulRC = pfnVMIEntry(0, VMI_CMD_QUERYCURRENTMODE, NULL, &sCurModeInfo);
    if (ulRC != RC_SUCCESS) {
      dbgprintf("Could not query desktop video mode.\n");
      break;
    }

    /* Query number of available video modes. */
    ulOp = QUERYMODE_NUM_MODES;
    ulRC = pfnVMIEntry(0, VMI_CMD_QUERYMODES, &ulOp, &pModes->ulCount);
    if (ulRC != RC_SUCCESS) {
      dbgprintf("VMIEntry: VMI_CMD_QUERYMODES/QUERYMODE_NUM_MODES failed\n");
      return FALSE;
    }
    dbgprintf("Number of modes: %u\n", pModes->ulCount);

    /* Allocate memory for video modes information. */
    pGDDModeInfList = SDL_malloc(sizeof(GDDMODEINFO) * pModes->ulCount);
    if (pGDDModeInfList == NULL) {
      SDL_OutOfMemory ();
      dbgprintf("Not enough memory\n");
      break;
    }

    /* Query video modes information. */
    ulOp = QUERYMODE_MODE_DATA;
    ulRC = pfnVMIEntry(0, VMI_CMD_QUERYMODES, &ulOp, pGDDModeInfList);
    if (ulRC != RC_SUCCESS) {
      dbgprintf("VMIEntry: VMI_CMD_QUERYMODES/QUERYMODE_MODE_DATA failed\n");
      break;
    }

    /* Allocate memory for GROP modes list. */
    pModes->pList = SDL_malloc(sizeof(VIDEOMODE) * pModes->ulCount);
    if (pModes->pList == NULL) {
      SDL_OutOfMemory ();
      dbgprintf("Not enough memory\n");
      break;
    }

    /* Fill GROP modes list. Get data from video modes information. */
    pModes->ulDesktopMode = ~0;
    pMode = pModes->pList;
    pGDDModeInf = pGDDModeInfList;
    for (ulIdx = 0; ulIdx < pModes->ulCount; ulIdx++, pMode++, pGDDModeInf++) {
      pMode->ulId                = pGDDModeInf->ulModeId;
      pMode->ulBPP               = pGDDModeInf->ulBpp;
      pMode->ulWidth             = pGDDModeInf->ulHorizResolution;
      pMode->ulHeight            = pGDDModeInf->ulVertResolution;
      pMode->ulScanLineSize      = pGDDModeInf->ulScanLineSize;
      pMode->fccColorEncoding    = pGDDModeInf->fccColorEncoding;

      if (sCurModeInfo.ulModeId == pGDDModeInf->ulModeId)
        pModes->ulDesktopMode = ulIdx;

      dbgprintf("#%u %u x %u, bpp: %u%s\n", pMode->ulId, pMode->ulWidth,
                pMode->ulHeight, pMode->ulBPP,
                sCurModeInfo.ulModeId == pGDDModeInf->ulModeId ?
                " - current" : "");
    }

    if (pModes->ulDesktopMode == ~0) {
      dbgprintf("Desktop mode not found in the video modes list\n");
      break;
    }

    /* Free video modes information memory. */
    SDL_free(pGDDModeInfList);
    return TRUE;
  }
  while (FALSE);

  /* Error */
  if (pGDDModeInfList != NULL)
    SDL_free(pGDDModeInfList);

  vsDone(*ppVSData, pModes);

  return FALSE;
}


/* VOID vsDone(PVOID pVSData, PVIDEOMODESLIST pModes)
 *
 * Output video system finalization. This function should destroy
 * VIDEOMODESLIST structure pointed by pModes.
 */
static VOID vsDone(PVOID pVSData, PVIDEOMODESLIST pModes)
{
  if (pVSData != NULL) {
    PVMANDATA pVMANData = (PVMANDATA)pVSData;

    if (pVMANData->hrgnVisible != NULLHANDLE) {
      dbgprintf("pVMANData->hrgnVisible was not destroyed\n");
    /* GpiDestroyRegion(hps, pVMANData->hrgnVisible);*/
    }

    SDL_free(pVSData);
  }

  /* Destroy video modes list. */
  if (pModes->pList != NULL) {
    SDL_free(pModes->pList);
    pModes->pList = NULL;
  }
  pModes->ulCount = 0;

  /* VMAN module finalization. */
  _vmanDone();
}


/* BOOL vsSetModeCompatible(PVOID pVSData, PVIDEOMODE pMode,
 *                          BOOL fFullscreen, HWND hwndDT, HDC hdcDT)
 * BOOL vsSetMode(PVOID pVSData, PVIDEOMODE pMode, BOOL fFullscreen,
 *                HWND hwndDT, HDC hdcDT)
 *
 * Sets video mode pMode - one of listed in vsInit(). Flag fFullscreen may be
 * FALSE only if pMode is the desktop mode defined in vsInit().
 * hwndDT, hdcDT - window handle & device context handle of the desktop window.
 * Returns TRUE on success.
 */
static BOOL vsSetModeCompatible(PVOID pVSData, PVIDEOMODE pMode,
                                BOOL fFullscreen, HWND hwndDT, HDC hdcDT)
{
  if (fFullscreen)
    _clearScreen(pMode);
  return TRUE;
}

static BOOL vsSetMode(PVOID pVSData, PVIDEOMODE pMode, BOOL fFullscreen,
                      HWND hwndDT, HDC hdcDT)
{
  BOOL     fSuccess;
#ifdef GROP_NO_MOUSE_IN_FULLSCREEN
  HWSHOWPTRIN hwspi;

  hwspi.ulLength = sizeof(HWSHOWPTRIN);
#endif

  if (fFullscreen) {
#ifdef GROP_NO_MOUSE_IN_FULLSCREEN
    /* Hide mouse pointer. */
    hwspi.fShow = FALSE;
    pfnVMIEntry(0, VMI_CMD_SHOWPTR, &hwspi, NULL);
#endif

    /* Info PM that we're away in FS mode. */
    WinLockWindowUpdate(hwndDT, hwndDT);
    GreDeath(hdcDT);
    DosSleep(256);

    /* Set video mode. */
    if (pfnVMIEntry(0, VMI_CMD_SETMODE, &pMode->ulId, NULL) != RC_SUCCESS) {
      dbgprintf("VMIEntry: VMI_CMD_SETMODE failed\n");
      GreResurrection(hdcDT, 0, NULL);
      WinLockWindowUpdate(HWND_DESKTOP, 0);
      WinInvalidateRect(hwndDT, NULL, TRUE);
      fSuccess = FALSE;
    }
    else {
      _clearScreen(pMode);
      fSuccess = TRUE;
    }
  }
  else {
    /* Set desktop video mode. */
    if (pfnVMIEntry(0, VMI_CMD_SETMODE, &pMode->ulId, NULL) != RC_SUCCESS) {
      dbgprintf("VMIEntry: VMI_CMD_SETMODE failed\n");
      fSuccess = FALSE;
    }
    else {
      fSuccess = TRUE;
      DosSleep(256);
    }

    /* Restore PM. */
    GreResurrection(hdcDT, 0, NULL);
    WinLockWindowUpdate(hwndDT, 0);
    WinInvalidateRect(hwndDT, NULL, TRUE);

#ifdef GROP_NO_MOUSE_IN_FULLSCREEN
    /* Show mouse pointer. */
    hwspi.fShow = TRUE;
    pfnVMIEntry(0, VMI_CMD_SHOWPTR, &hwspi, NULL);
#endif
  }

  return fSuccess;
}


static PVOID vsVideoBufAlloc(PVOID pVSData, ULONG ulWidth, ULONG ulHeight,
                             ULONG ulBPP, ULONG fccColorEncoding,
                             PULONG pulScanLineSize)
{
  ULONG ulRC;
  ULONG ulScanLineSize = ulWidth * (ulBPP >> 3);
  PVOID pBuffer;

  /* Bytes per line. */
  ulScanLineSize =  (ulScanLineSize + 3) & ~3; /* 4-byte aligning */
  *pulScanLineSize = ulScanLineSize;

  dbgprintf("Allocate %u bytes...\n", ulHeight * ulScanLineSize);
  ulRC = DosAllocMem(&pBuffer, ulHeight * ulScanLineSize,
                     PAG_COMMIT | PAG_EXECUTE | PAG_READ | PAG_WRITE);
  if (ulRC != NO_ERROR) {
    dbgprintf("DosAllocMem(), rc = %u\n", ulRC);
    return NULL;
  }
  dbgprintf("Memory allocated: 0x%P\n", pBuffer);

  return pBuffer;
}


static VOID vsVideoBufFree(PVOID pVSData, PVOID pBuffer)
{
  ULONG ulRC = DosFreeMem(pBuffer);

  if (ulRC != NO_ERROR)
    dbgprintf("DosFreeMem(), rc = %u\n", ulRC);
}

static VOID vsSetVisibleRegion(PVOID pVSData, HWND hwnd, PGROPSETMODE pUserMode,
                               PRECTL prectlWin, PRECTL prectlVA, BOOL fVisible)
{
  PVMANDATA pVMANData = (PVMANDATA)pVSData;
  HPS       hps = WinGetPS(hwnd);

  if (hps == NULLHANDLE) {
    dbgprintf("Cannot get window PS\n");
    return;
  }

  if (pVMANData->hrgnVisible != NULLHANDLE) {
    GpiDestroyRegion(hps, pVMANData->hrgnVisible);
    pVMANData->hrgnVisible = NULLHANDLE;
  }

  if (fVisible) {
    pVMANData->hrgnVisible = GpiCreateRegion(hps, 0, NULL);
    if (pVMANData->hrgnVisible == RGN_ERROR ||
        pVMANData->hrgnVisible == NULLHANDLE)
      dbgprintf("GpiCreateRegion() failed\n");
    else if (WinQueryVisibleRegion(hwnd, pVMANData->hrgnVisible) == RGN_ERROR)
      dbgprintf("WinQueryVisibleRegion() failed\n");
    else {
      dbgprintf("Visible region is set\n");
      WinInvalidateRect(hwnd, NULL, TRUE);
    }
  }

  WinReleasePS(hps);
}

/* BOOL vsUpdate(PVOID pVSData, PGROPDATA pGrop, ULONG cRect, PRECTL prectl)
 *
 * Updates given user space rectangles list on the screen.
 */
static BOOL vsUpdate(PVOID pVSData, PGROPDATA pGrop, ULONG cRect,
                     PRECTL prectl)
{
  PVMANDATA   pVMANData = (PVMANDATA)pVSData;
  PVIDEOMODE  pMode = &pGrop->stModes.pList[
        pGrop->fFullscreen ? pGrop->ulModeIdx : pGrop->stModes.ulDesktopMode ];
  BMAPINFO    bmiSrc;
  BMAPINFO    bmiDst;
  PPOINTL     pptlSrcOrg;
  PBLTRECT    pbrDst;
  POINTL      ptlSrcOrg;
  BLTRECT     brDst;
  HWREQIN     sHWReqIn;
  BITBLTINFO  sBitbltInfo = { 0 };
  ULONG       ulIdx;
  RECTL       rectlScreenUpdate;

  if (pGrop->stUserMode.pBuffer == NULL)
    return FALSE;

  if (pVMANData->hrgnVisible == NULLHANDLE) {
    dbgprintf("No visible region\n");
    return TRUE;
  }

  bmiSrc.ulLength = sizeof(BMAPINFO);
  bmiSrc.ulType = BMAP_MEMORY;
  bmiSrc.ulWidth = pGrop->stUserMode.ulWidth;
  bmiSrc.ulHeight = pGrop->stUserMode.ulHeight;
  bmiSrc.ulBpp = pGrop->stUserMode.ulBPP;
  bmiSrc.ulBytesPerLine = pGrop->stUserMode.ulScanLineSize;
  bmiSrc.pBits = (PBYTE)pGrop->stUserMode.pBuffer;

  bmiDst.ulLength = sizeof(BMAPINFO);
  bmiDst.ulType = BMAP_VRAM;
  bmiDst.pBits = (PBYTE)ulVRAMAddress;
  bmiDst.ulWidth = pGrop->rectlViewArea.xRight - pGrop->rectlViewArea.xLeft;
  bmiDst.ulHeight = pGrop->rectlViewArea.yTop - pGrop->rectlViewArea.yBottom;
  bmiDst.ulBpp = pMode->ulBPP;
  bmiDst.ulBytesPerLine = pMode->ulScanLineSize;

  if (pGrop->fFullscreen) {
    /* In fullscreen mode we don't need to make list of visible update area. */
    ptlSrcOrg.x = 0;
    ptlSrcOrg.y = 0;
    pptlSrcOrg = &ptlSrcOrg;

    brDst.ulXOrg = pGrop->rectlWinArea.xLeft + pGrop->rectlViewArea.xLeft;
    brDst.ulYOrg = pMode->ulHeight - pGrop->rectlViewArea.yTop;
    brDst.ulXExt = bmiDst.ulWidth;
    brDst.ulYExt = bmiDst.ulHeight;
    pbrDst = &brDst;
    cRect = 1;
  }
  else {
    /* In window mode we will make list of update rectangles. This is the
     * intersection of requested rectangles and visible rectangles. */
    PRECTL   prectlDst, prectlScan;
    HPS      hps;
    HRGN     hrgnUpdate;
    RGNRECT  rgnCtl;
    RECTL    rectlDef;

    if (cRect == 0) {
      /* Full update requested. */
      rectlDef.xLeft = 0;
      rectlDef.yBottom = 0;
      rectlDef.xRight = bmiSrc.ulWidth;
      rectlDef.yTop = bmiSrc.ulHeight;
      prectl = &rectlDef;
      cRect = 1;
    }

    /* Make list of destionation rectangles (prectlDst) list from the source
     * list (prectl). */
    prectlDst = SDL_malloc(cRect * sizeof(RECTL));
    if (prectlDst == NULL) {
      SDL_OutOfMemory ();
      dbgprintf("Not enough memory\n");
      return FALSE;
    }
    prectlScan = prectlDst;
    for (ulIdx = 0; ulIdx < cRect; ulIdx++, prectl++, prectlScan++) {
      prectlScan->xLeft = prectl->xLeft + pGrop->rectlViewArea.xLeft;
      prectlScan->yBottom = prectl->yBottom + pGrop->rectlViewArea.yBottom;
      prectlScan->xRight = prectl->xRight + pGrop->rectlViewArea.xLeft;
      prectlScan->yTop = prectl->yTop + pGrop->rectlViewArea.yBottom;
    }

    hps = WinGetPS(pGrop->hwnd);
    /* Make destination region to update. */
    hrgnUpdate = GpiCreateRegion(hps, cRect, prectlDst);
    /* "AND" on visible and destination regions, result is update region. */
    GpiCombineRegion(hps, hrgnUpdate, hrgnUpdate, pVMANData->hrgnVisible,
                     CRGN_AND);

    /* Get rectangles of region to update. */
    rgnCtl.ircStart     = 1;
    rgnCtl.crc          = 0;
    rgnCtl.ulDirection  = 1;
    rgnCtl.crcReturned  = 0;
    GpiQueryRegionRects(hps, hrgnUpdate, NULL, &rgnCtl, NULL);
    if (rgnCtl.crcReturned == 0) {
      GpiDestroyRegion(hps, hrgnUpdate);
      SDL_free(prectlDst);
      WinReleasePS(hps);
      return TRUE;
    }
    /* We don't need prectlDst, use it again to store rects of update region. */
    prectl = SDL_realloc(prectlDst, rgnCtl.crcReturned * sizeof(RECTL));
    if (prectl == NULL) {
      dbgprintf("Not enough memory\n");
      SDL_OutOfMemory ();
      GpiDestroyRegion(hps, hrgnUpdate);
      SDL_free(prectlDst);
      WinReleasePS(hps);
      return FALSE;
    }
    prectlDst = prectl;
    rgnCtl.ircStart     = 1;
    rgnCtl.crc          = rgnCtl.crcReturned;
    rgnCtl.ulDirection  = 1;
    GpiQueryRegionRects(hps, hrgnUpdate, NULL, &rgnCtl, prectlDst);
    GpiDestroyRegion(hps, hrgnUpdate);
    WinReleasePS(hps);
    cRect = rgnCtl.crcReturned;

    /* Now cRect/prectlDst - list of regions in window (update && visible). */

    /* Make lists for blitting from update regions. */

    pbrDst = SDL_malloc(cRect * sizeof(BLTRECT));
    if (pbrDst == NULL) {
      SDL_OutOfMemory ();
      dbgprintf("Not enough memory\n");
      SDL_free(prectlDst);
      return FALSE;
    }

    pptlSrcOrg = (PPOINTL)prectlDst; /* Yes, we will use this memory again. */
    prectlScan = prectlDst;
    for (ulIdx = 0; ulIdx < cRect; ulIdx++, prectlScan++, pptlSrcOrg++) {
      pbrDst[ulIdx].ulXOrg = pGrop->rectlWinArea.xLeft + prectlScan->xLeft;
      pbrDst[ulIdx].ulYOrg = pMode->ulHeight - (pGrop->rectlWinArea.yBottom +
                                                prectlScan->yTop);
      pbrDst[ulIdx].ulXExt = prectlScan->xRight - prectlScan->xLeft;
      pbrDst[ulIdx].ulYExt = prectlScan->yTop - prectlScan->yBottom;

      pptlSrcOrg->x = prectlScan->xLeft - pGrop->rectlViewArea.xLeft;
      pptlSrcOrg->y = bmiSrc.ulHeight -
                             (prectlScan->yTop - pGrop->rectlViewArea.yBottom);
    }
    pptlSrcOrg = (PPOINTL)prectlDst; /* Return to the beginning of the list. */
  }

  rectlScreenUpdate.xLeft = pGrop->rectlWinArea.xLeft + pGrop->rectlViewArea.xLeft;
  rectlScreenUpdate.yBottom = pGrop->rectlWinArea.yBottom + pGrop->rectlViewArea.yBottom;
  rectlScreenUpdate.xRight = pGrop->rectlWinArea.xLeft + pGrop->rectlViewArea.xRight;
  rectlScreenUpdate.yTop = pGrop->rectlWinArea.yBottom + pGrop->rectlViewArea.yTop;

  /* Request HW */
  sHWReqIn.ulLength = sizeof(HWREQIN);
  sHWReqIn.ulFlags = REQUEST_HW;
  sHWReqIn.cScrChangeRects = 1;
  sHWReqIn.arectlScreen = &rectlScreenUpdate;
  if (pfnVMIEntry(0, VMI_CMD_REQUESTHW, &sHWReqIn, NULL) != RC_SUCCESS) {
    dbgprintf("pfnVMIEntry(,VMI_CMD_REQUESTHW,,) failed\n");
    sHWReqIn.cScrChangeRects = 0; /* for fail signal only. */
  }
  else {
    RECTL rclSrcBounds, rclDstBounds;

    rclSrcBounds.xLeft = 0;
    rclSrcBounds.yBottom = 0;
    rclSrcBounds.xRight = bmiSrc.ulWidth;
    rclSrcBounds.yTop = bmiSrc.ulHeight;
    rclDstBounds = rectlScreenUpdate;

    sBitbltInfo.ulLength = sizeof(BITBLTINFO);
    sBitbltInfo.ulBltFlags = BF_DEFAULT_STATE | BF_ROP_INCL_SRC | BF_PAT_HOLLOW;
    sBitbltInfo.cBlits = cRect;
    sBitbltInfo.ulROP = ROP_SRCCOPY;
    sBitbltInfo.pSrcBmapInfo = &bmiSrc;
    sBitbltInfo.pDstBmapInfo = &bmiDst;
    sBitbltInfo.prclSrcBounds = &rclSrcBounds;
    sBitbltInfo.prclDstBounds = &rclDstBounds;
    sBitbltInfo.aptlSrcOrg = pptlSrcOrg;
    sBitbltInfo.abrDst = pbrDst;

    /* Screen update. */
    if (pfnVMIEntry(0, VMI_CMD_BITBLT, &sBitbltInfo, NULL) != RC_SUCCESS) {
      dbgprintf("pfnVMIEntry(,VMI_CMD_BITBLT,,) failed\n");
      sHWReqIn.cScrChangeRects = 0; /* for fail signal only. */
    }
    else {
      /* Release HW */
      sHWReqIn.ulFlags = 0;
      if (pfnVMIEntry(0, VMI_CMD_REQUESTHW, &sHWReqIn, NULL) != RC_SUCCESS)
        dbgprintf("pfnVMIEntry(,VMI_CMD_REQUESTHW,,) failed\n");
    }
  }

  if (pptlSrcOrg != &ptlSrcOrg) /* We don't take this memory for fullscreen.*/
    SDL_free(pptlSrcOrg);       /* It was allocated for prectlDst. :) */

  if (pbrDst != &brDst) /* We don't take this memory for the fullscreen. */
    SDL_free(pbrDst);

  return sHWReqIn.cScrChangeRects != 0;
}


static BOOL vsSetPalette(PVOID pVSData, ULONG ulFirst, ULONG ulNumber,
                         PRGB2 pColors)
{
  HWPALETTEINFO sPalInfo;
  ULONG ulIdx;
  ULONG ulRC;

  sPalInfo.ulLength = sizeof(HWPALETTEINFO);
  sPalInfo.fFlags = PALETTE_SET;
  sPalInfo.ulStartIndex = ulFirst;
  sPalInfo.ulNumEntries = ulNumber;
  sPalInfo.pRGBs = pColors;

  for (ulIdx = 0; ulIdx < ulNumber; ulIdx++, pColors++)
    pColors->fcOptions = 0;

  ulRC = pfnVMIEntry(0, VMI_CMD_PALETTE, &sPalInfo, NULL);
  if (ulRC != RC_SUCCESS) {
    dbgprintf("pfnVMIEntry(,VMI_CMD_PALETTE,,), rc = %u\n", ulRC);
    return FALSE;
  }

  return TRUE;
}

#endif /* SDL_VIDEO_DRIVER_OS2GROP */
