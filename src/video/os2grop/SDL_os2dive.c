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

#define INCL_OS2MM
#define INCL_GPI
#define INCL_DOSERRORS
#define INCL_DOSMEMMGR
#define INCL_DOSSEMAPHORES
#include <os2.h>
#define __MEERROR_H__
#define  _MEERROR_H_
#include <mmioos2.h>
#include <os2me.h>
#define INCL_MM_OS2
#include <dive.h>
#include <fourcc.h>

#include "SDL_grop.h"

static BOOL vsInit(PVIDEOMODESLIST pModes, PVOID *ppVSData);
static VOID vsDone(PVOID pVSData, PVIDEOMODESLIST pModes);
static BOOL vsSetMode(PVOID pVSData, PVIDEOMODE pMode, BOOL fFullscreen, HWND hwndDT,
                      HDC hdcDT);
static PVOID vsVideoBufAlloc(PVOID pVSData, ULONG ulWidth, ULONG ulHeight,
                  ULONG ulBPP, ULONG fccColorEncoding, PULONG pulScanLineSize);
static VOID vsVideoBufFree(PVOID pVSData, PVOID pBuffer);
static VOID vsSetVisibleRegion(PVOID pVSData, HWND hwnd, PGROPSETMODE pUserMode,
                             PRECTL prectlWin, PRECTL prectlVA, BOOL fVisible);
static BOOL vsUpdate(PVOID pVSData, PGROPDATA pGrop, ULONG cRect, PRECTL rectl);
static BOOL vsSetPalette(PVOID pVSData, ULONG ulFirst, ULONG ulNumber,
                         PRGB2 pColors);

VIDEOSYS stVideoSysDIVE = {
  TRUE,     /* fFreeWinSize */
  TRUE,     /* fBPPConvSup  */
  vsInit,
  vsDone,
  vsSetMode,
  vsVideoBufAlloc,
  vsVideoBufFree,
  vsSetVisibleRegion,
  vsUpdate,
  vsSetPalette
};

typedef struct _DIVEData {
  HDIVE      hDive;
  BOOL       fBlitterReady;
} DIVEData, *PDIVEData;

/* BOOL vsInit(PVIDEOMODESLIST pModes, PVOID *ppVSData)
 *
 * Output video system initialization. This function should fill VIDEOMODESLIST
 * structure pointed by pModes.
 * Returns TRUE on success.
 */
static BOOL vsInit(PVIDEOMODESLIST pModes, PVOID *ppVSData)
{
  PDIVEData   pDIVEData;
  DIVE_CAPS   sDiveCaps = { 0 };
  FOURCC      fccFormats[100] = { 0 };
  PVIDEOMODE  pMode;
  ULONG       ulIdx, cModes;

  sDiveCaps.pFormatData    = fccFormats;
  sDiveCaps.ulFormatLength = 100;
  sDiveCaps.ulStructLen    = sizeof(DIVE_CAPS);

  if (DiveQueryCaps(&sDiveCaps, DIVE_BUFFER_SCREEN)) {
    dbgprintf("DIVE routines cannot function in this system environment.\n");
    return FALSE;
  }

  if (sDiveCaps.ulDepth < 8) {
    dbgprintf("Not enough screen colors to run DIVE. "
              "Must be at least 256 colors.\n");
    return FALSE;
  }

  pDIVEData = SDL_malloc(sizeof(DIVEData));
  if (pDIVEData == NULL) {
    SDL_OutOfMemory ();
    dbgprintf("Not enough memory\n");
    return FALSE;
  }

  if (DiveOpen(&pDIVEData->hDive, FALSE, NULL)) {
    dbgprintf("Unable to open an instance of DIVE.\n");
    SDL_free(pDIVEData);
    return FALSE;
  }
  pDIVEData->fBlitterReady = FALSE;
  *ppVSData = pDIVEData;

  /* Fake video modes for DIVE - all with desktop resolution and BPP from 8 to
   * current (desktop) mode BPP. */
  cModes = sDiveCaps.ulDepth >> 3;
  pMode = SDL_malloc(sizeof(VIDEOMODE) * cModes);
  if (pMode == NULL) {
    dbgprintf("Not enough memory\n");
    SDL_OutOfMemory ();
    DiveClose(pDIVEData->hDive);
    SDL_free(pDIVEData);
    return FALSE;
  }

  pModes->pList = pMode;
  pModes->ulCount = cModes;
  pModes->ulDesktopMode = cModes - 1;

  /* Fill modes list. */
  for (ulIdx = 1; ulIdx < cModes; ulIdx++, pMode++) {
    pMode->ulId           = ulIdx; /* Not used with DIVE */
    pMode->ulWidth        = sDiveCaps.ulHorizontalResolution;
    pMode->ulHeight       = sDiveCaps.ulVerticalResolution;
    /* DiveSetupBlitter() returns DIVE_ERR_INVALID_CONVERSION on my
     * system for 24bit buffer. Just dirty skip it here... */
    pMode->ulBPP          = ulIdx == 3 ? 16 : (ulIdx << 3);

    pMode->ulScanLineSize = sDiveCaps.ulHorizontalResolution * ulIdx;
    pMode->ulScanLineSize = (pMode->ulScanLineSize + 3) & ~3;

    switch (ulIdx) {
    case 1: /* 8 bit */
      pMode->fccColorEncoding = FOURCC_LUT8;
      break;

    case 2: /* 16 bit */
      pMode->fccColorEncoding = FOURCC_R565;
      break;

    case 3: /* 24 bit */
      pMode->fccColorEncoding = FOURCC_R664;/*FOURCC_RGB3*/
      break;

    case 4: /* 32 bit */
      pMode->fccColorEncoding = FOURCC_RGB4;
      break;
    }
  }

  /* Last mode is a desktop mode. */
  pMode->ulId                 = ulIdx;
  pMode->ulWidth              = sDiveCaps.ulHorizontalResolution;
  pMode->ulHeight             = sDiveCaps.ulVerticalResolution;
  pMode->fccColorEncoding     = sDiveCaps.fccColorEncoding;
  pMode->ulBPP                = sDiveCaps.ulDepth;
  pMode->ulScanLineSize       = sDiveCaps.ulScanLineBytes;

  return TRUE;
}

/* VOID vsDone(PVOID pVSData, PVIDEOMODESLIST pModes)
 *
 * Output video system finalization. This function should destroy
 * VIDEOMODESLIST structure pointed by pModes.
 */
static VOID vsDone(PVOID pVSData, PVIDEOMODESLIST pModes)
{
  PDIVEData pDIVEData = (PDIVEData)pVSData;

  if (pDIVEData->fBlitterReady)
    DiveSetupBlitter(pDIVEData->hDive, 0);

  SDL_free(pModes->pList);
  pModes->pList = NULL;

  DiveClose(pDIVEData->hDive);
  SDL_free(pDIVEData);
}

/* BOOL vsSetMode(PVOID pVSData, PVIDEOMODE pMode, BOOL fFullscreen,
 *                HWND hwndDT, HDC hdcDT)
 *
 * Sets video mode pMode - one of listed in vsInit(). Flag fFullscreen may be
 * FALSE only if pMode is the desktop mode defined in vsInit().
 * hwndDT, hdcDT - window handle & device context handle of the desktop window.
 * Returns TRUE on success.
 */
static BOOL vsSetMode(PVOID pVSData, PVIDEOMODE pMode, BOOL fFullscreen,
                      HWND hwndDT, HDC hdcDT)
{
  /* We don't real changes video mode with DIVE. */
  return TRUE;
}

static PVOID vsVideoBufAlloc(PVOID pVSData, ULONG ulWidth, ULONG ulHeight,
                             ULONG ulBPP, ULONG fccColorEncoding,
                             PULONG pulScanLineSize)
{
  PDIVEData pDIVEData = (PDIVEData)pVSData;
  ULONG     ulRC;
  ULONG     ulScanLineSize = ulWidth * (ulBPP >> 3);
  PVOID     pBuffer, pVideoBuffer;

  /* Bytes per line. */
  ulScanLineSize =  (ulScanLineSize + 3) & ~3; /* 4-byte aligning */
  *pulScanLineSize = ulScanLineSize;

  dbgprintf("Allocate %u bytes...\n", (ulHeight * ulScanLineSize) + sizeof(ULONG));
  ulRC = DosAllocMem(&pBuffer, (ulHeight * ulScanLineSize) + sizeof(ULONG),
                     PAG_COMMIT | PAG_EXECUTE | PAG_READ | PAG_WRITE);
  if (ulRC != NO_ERROR) {
    dbgprintf("DosAllocMem(), rc = %u\n", ulRC);
    return NULL;
  }

  /* First 4 bytes of allocated memory space will be used to save DIVE buffer
   * number. 5'th byte of allocated buffer is a begining of the user buffer. */
  pVideoBuffer = &((PULONG)pBuffer)[1];

  dbgprintf("Memory allocated: 0x%P, video buffer: 0x%P\n", pBuffer, pVideoBuffer);
  ulRC = DiveAllocImageBuffer(pDIVEData->hDive, (PULONG)pBuffer,
                              fccColorEncoding, ulWidth, ulHeight,
                              ulScanLineSize, pVideoBuffer);
  if (ulRC != DIVE_SUCCESS) {
    dbgprintf("DiveAllocImageBuffer(), rc = 0x%X\n", ulRC);
    DosFreeMem(pBuffer);
    return NULL;
  }

  dbgprintf("buffer: 0x%P, DIVE buffer number: %u\n",
            pVideoBuffer, *(PULONG)pBuffer);

  return pVideoBuffer;
}

static VOID vsVideoBufFree(PVOID pVSData, PVOID pBuffer)
{
  PDIVEData pDIVEData = (PDIVEData)pVSData;
  ULONG     ulRC;

  /* Move pointer pBuffer to the allocated memory space (-4 bytes). */
  pBuffer = (PVOID) (((PULONG)pBuffer) - 1);
  /* First 4 bytes of pBuffer now is the DIVE buffer number. */
  ulRC = DiveFreeImageBuffer(pDIVEData->hDive, *((PULONG)pBuffer));
  if (ulRC != DIVE_SUCCESS)
    dbgprintf("DiveFreeImageBuffer(,%u), rc = %u\n", *((PULONG)pBuffer), ulRC);

  ulRC = DosFreeMem(pBuffer);
  if (ulRC != NO_ERROR)
    dbgprintf("DosFreeMem(), rc = %u\n", ulRC);
}

static VOID vsSetVisibleRegion(PVOID pVSData, HWND hwnd, PGROPSETMODE pUserMode,
                               PRECTL prectlWin, PRECTL prectlVA, BOOL fVisible)
{
  PDIVEData pDIVEData = (PDIVEData)pVSData;
  HPS       hps;
  HRGN      hrgn;
  RGNRECT   rgnCtl;
  PRECTL    prectl = NULL;
  ULONG     ulRC;

  if (!fVisible) {
    if (pDIVEData->fBlitterReady) {
      pDIVEData->fBlitterReady = FALSE;
      DiveSetupBlitter(pDIVEData->hDive, 0);
    }
    return;
  }

  /* Query visible rectangles */
  hps = WinGetPS(hwnd);
  hrgn = GpiCreateRegion(hps, 0, NULL);
  if (!hrgn) {
    WinReleasePS(hps);
    dbgprintf("GpiCreateRegion() failed\n");
    return;
  }

  WinQueryVisibleRegion(hwnd, hrgn);
  rgnCtl.ircStart     = 1;
  rgnCtl.crc          = 0;
  rgnCtl.ulDirection  = 1;
  GpiQueryRegionRects(hps, hrgn, NULL, &rgnCtl, NULL);
  if (rgnCtl.crcReturned != 0) {
    prectl = SDL_malloc(rgnCtl.crcReturned * sizeof(RECTL));
    if (prectl != NULL) {
      rgnCtl.ircStart     = 1;
      rgnCtl.crc          = rgnCtl.crcReturned;
      rgnCtl.ulDirection  = 1;
      GpiQueryRegionRects(hps, hrgn, NULL, &rgnCtl, prectl);
    }
    else {
      SDL_OutOfMemory ();
      dbgprintf("Not enough memory\n");
    }
  }
  GpiDestroyRegion(hps, hrgn);
  WinReleasePS(hps);

  if (prectl != NULL) {
    /* Setup DIVE blitter. */
    SETUP_BLITTER sSetupBlitter;

    sSetupBlitter.ulStructLen       = sizeof(SETUP_BLITTER);
    sSetupBlitter.fccSrcColorFormat = pUserMode->fccColorEncoding;
    sSetupBlitter.fInvert           = FALSE;
    sSetupBlitter.ulSrcWidth        = pUserMode->ulWidth;
    sSetupBlitter.ulSrcHeight       = pUserMode->ulHeight;
    sSetupBlitter.ulSrcPosX         = 0;
    sSetupBlitter.ulSrcPosY         = 0;
    sSetupBlitter.ulDitherType      = 0;
    sSetupBlitter.fccDstColorFormat = FOURCC_SCRN;
    sSetupBlitter.ulDstWidth        = prectlVA->xRight - prectlVA->xLeft;
    sSetupBlitter.ulDstHeight       = prectlVA->yTop - prectlVA->yBottom;
    sSetupBlitter.lDstPosX          = prectlVA->xLeft;
    sSetupBlitter.lDstPosY          = prectlVA->yBottom;
    sSetupBlitter.lScreenPosX       = prectlWin->xLeft;
    sSetupBlitter.lScreenPosY       = prectlWin->yBottom;
    sSetupBlitter.ulNumDstRects     = rgnCtl.crcReturned;
    sSetupBlitter.pVisDstRects      = prectl;

    ulRC = DiveSetupBlitter(pDIVEData->hDive, &sSetupBlitter);
    if (ulRC != DIVE_SUCCESS) {
      dbgprintf("DiveSetupBlitter(), rc = 0x%X\n", ulRC);
      pDIVEData->fBlitterReady = FALSE;
      DiveSetupBlitter(pDIVEData->hDive, 0);
    }
    else {
      pDIVEData->fBlitterReady = TRUE;
      WinInvalidateRect(hwnd, NULL, TRUE);
    }

    SDL_free(prectl);
  }
}

static BOOL vsUpdate(PVOID pVSData, PGROPDATA pGrop, ULONG cRect, PRECTL prectl)
{
  PDIVEData pDIVEData = (PDIVEData)pVSData;
  ULONG     ulRC;
  BOOL      fFullUpdate = cRect == 0;
  ULONG     ulDiveBufNum;
  ULONG     ulSrcHeight = pGrop->stUserMode.ulHeight;

  if (pGrop->stUserMode.pBuffer == NULL || !pDIVEData->fBlitterReady)
    return FALSE;
  ulDiveBufNum = *(((PULONG)pGrop->stUserMode.pBuffer) - 1);

  if (!fFullUpdate) {
    LONG  lTop, lBottom;
    PBYTE pbLineMask;

    pbLineMask = alloca(ulSrcHeight);
    if (pbLineMask == NULL) {
      dbgprintf("Not enough stack size\n");
      return FALSE;
    }
    SDL_memset(pbLineMask, 0, ulSrcHeight);

    for ( ; (LONG)cRect > 0; cRect--, prectl++) {
      lTop = prectl->yTop > ulSrcHeight ? ulSrcHeight : prectl->yTop;
      lBottom = prectl->yBottom < 0 ? 0 : prectl->yBottom;

      if (lTop > lBottom)
        /* pbLineMask - up-to-down lines mask. */
        SDL_memset(&pbLineMask[ulSrcHeight - lTop], 1, lTop - lBottom);
    }

    ulRC = DiveBlitImageLines(pDIVEData->hDive, ulDiveBufNum,
                              DIVE_BUFFER_SCREEN, pbLineMask);
    if (ulRC != DIVE_SUCCESS)
      dbgprintf("DiveBlitImageLines(,%u,,), rc = %u\n", ulRC, ulDiveBufNum);
  }
  else {
    ulRC = DiveBlitImage(pDIVEData->hDive, ulDiveBufNum, DIVE_BUFFER_SCREEN);
    if (ulRC != DIVE_SUCCESS)
      dbgprintf("DiveBlitImage(,%u,), rc = %u\n", ulDiveBufNum, ulRC);
  }

  return TRUE;
}

static BOOL vsSetPalette(PVOID pVSData, ULONG ulFirst, ULONG ulNumber,
                         PRGB2 pColors)
{
/* http://www.edm2.com/index.php/Gearing_Up_For_Games_-_Part_2 */
  PDIVEData pDIVEData = (PDIVEData)pVSData;
  ULONG     ulRC;

  ulRC = DiveSetSourcePalette /*DiveSetDestinationPalette*/ (
            pDIVEData->hDive, ulFirst, ulNumber, (PBYTE)pColors);
  if (ulRC != DIVE_SUCCESS) {
    dbgprintf("DiveSetDestinationPalette(), rc = %u\n", ulRC);
    return FALSE;
  }

  return TRUE;
}

#endif /* SDL_VIDEO_DRIVER_OS2GROP */
