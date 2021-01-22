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

#include "SDL_video.h"
#include "SDL_mouse.h"
#define INCL_WIN        /* Type RGB2. */
#define INCL_DOSMISC    /* DosQuerySysInfo(). */
#define INCL_GPIBITMAPS /* GPI bit map functions to make pointer. */
#include <os2.h>

#define __MEERROR_H__
#define  _MEERROR_H_
#include <mmioos2.h>
#include <fourcc.h>
#ifndef FOURCC_R666
#define FOURCC_R666 mmioFOURCC('R','6','6','6')
#endif

#include "SDL_os2grop.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

/* GROP callback. */
static VOID cbMouseMove(PGROPDATA pGrop, BOOL fRelative, LONG lX, LONG lY);
static VOID cbMouseButton(PGROPDATA pGrop, ULONG ulButton, BOOL fDown);
static VOID cbKeyboard(PGROPDATA pGrop, ULONG ulScanCode, ULONG ulChar,
                       ULONG ulFlags);
static VOID cbActive(PGROPDATA pGrop, ULONG ulType, BOOL fSet);
static VOID cbSize(PGROPDATA pGrop, ULONG ulWidth, ULONG ulHeight);
static BOOL cbQuit(PGROPDATA pGrop);

static GROPCALLBACK stGropCallback = {
  cbMouseMove,
  cbMouseButton,
  cbKeyboard,
  cbActive,
  cbSize,
  cbQuit
};

/* Keyboard data. */
typedef struct _SCAN2SDLKEY {
  ULONG      ulScan;
  SDLKey     enSDLKey;
} SCAN2SDLKEY;

static SCAN2SDLKEY aScan2SDLKey[] = {
  /* First line of keyboard: */
  {0x1, SDLK_ESCAPE}, {0x3b, SDLK_F1}, {0x3c, SDLK_F2}, {0x3d, SDLK_F3},
  {0x3e, SDLK_F4}, {0x3f, SDLK_F5}, {0x40, SDLK_F6}, {0x41, SDLK_F7},
  {0x42, SDLK_F8}, {0x43, SDLK_F9}, {0x44, SDLK_F10}, {0x57, SDLK_F11},
  {0x58, SDLK_F12}, {0x5d, SDLK_PRINT}, {0x46, SDLK_SCROLLOCK},
  {0x5f, SDLK_PAUSE},
  /* Second line of keyboard: */
  {0x29, SDLK_BACKQUOTE}, {0x2, SDLK_1}, {0x3, SDLK_2}, {0x4, SDLK_3},
  {0x5, SDLK_4}, {0x6, SDLK_5}, {0x7, SDLK_6}, {0x8, SDLK_7}, {0x9, SDLK_8},
  {0xa, SDLK_9}, {0xb, SDLK_0}, {0xc, SDLK_MINUS}, {0xd, SDLK_EQUALS},
  {0xe, SDLK_BACKSPACE}, {0x68, SDLK_INSERT}, {0x60, SDLK_HOME},
  {0x62, SDLK_PAGEUP}, {0x45, SDLK_NUMLOCK}, {0x5c, SDLK_KP_DIVIDE},
  {0x37, SDLK_KP_MULTIPLY}, {0x4a, SDLK_KP_MINUS},
  /* Third line of keyboard: */
  {0xf, SDLK_TAB}, {0x10, SDLK_q}, {0x11, SDLK_w}, {0x12, SDLK_e},
  {0x13, SDLK_r}, {0x14, SDLK_t}, {0x15, SDLK_y}, {0x16, SDLK_u},
  {0x17, SDLK_i}, {0x18, SDLK_o}, {0x19, SDLK_p}, {0x1a, SDLK_LEFTBRACKET},
  {0x1b, SDLK_RIGHTBRACKET}, {0x1c, SDLK_RETURN}, {0x69, SDLK_DELETE},
  {0x65, SDLK_END}, {0x67, SDLK_PAGEDOWN}, {0x47, SDLK_KP7}, {0x48, SDLK_KP8},
  {0x49, SDLK_KP9}, {0x4e, SDLK_KP_PLUS},
  /* Fourth line of keyboard: */
  {0x3a, SDLK_CAPSLOCK}, {0x1e, SDLK_a}, {0x1f, SDLK_s}, {0x20, SDLK_d},
  {0x21, SDLK_f}, {0x22, SDLK_g}, {0x23, SDLK_h}, {0x24, SDLK_j},
  {0x25, SDLK_k}, {0x26, SDLK_l}, {0x27, SDLK_SEMICOLON}, {0x28, SDLK_QUOTE},
  {0x2b, SDLK_BACKSLASH}, {0x4b, SDLK_KP4}, {0x4c, SDLK_KP5}, {0x4d, SDLK_KP6},
  /* Fifth line of keyboard: */
  {0x2a, SDLK_LSHIFT}, {0x56, SDLK_WORLD_1}, /* Code 161, letter i' on hungarian keyboard */
  {0x2c, SDLK_z}, {0x2d, SDLK_x}, {0x2e, SDLK_c}, {0x2f, SDLK_v},
  {0x30, SDLK_b}, {0x31, SDLK_n}, {0x32, SDLK_m}, {0x33, SDLK_COMMA},
  {0x34, SDLK_PERIOD}, {0x35, SDLK_SLASH}, {0x36, SDLK_RSHIFT}, {0x61, SDLK_UP},
  {0x4f, SDLK_KP1}, {0x50, SDLK_KP2}, {0x51, SDLK_KP3}, {0x5a, SDLK_KP_ENTER},
  /* Sixth line of keyboard: */
  {0x1d, SDLK_LCTRL}, {0x7e, SDLK_LSUPER}, /* Windows key */
  {0x38, SDLK_LALT}, {0x39, SDLK_SPACE},
  {0x5e, SDLK_RALT}, {0x7f, SDLK_RSUPER}, {0x7c, SDLK_MENU}, {0x5b, SDLK_RCTRL},
  {0x63, SDLK_LEFT}, {0x66, SDLK_DOWN}, {0x64, SDLK_RIGHT}, {0x52, SDLK_KP0},
  {0x53, SDLK_KP_PERIOD} };

typedef struct _UNICODESHIFTKEY {
  SDLKey    enSDLKey;
  SDLKey    enSDLShiftKey;
  ULONG     ulSDLKey;
} UNICODESHIFTKEY;

static SDLKey aScanSDLKeyMap[0xFF] = { SDLK_UNKNOWN };

static UNICODESHIFTKEY aUnicodeShiftKey[] = {
  {SDLK_BACKQUOTE, '~'}, {SDLK_1, SDLK_EXCLAIM}, {SDLK_2, SDLK_AT},
  {SDLK_3, SDLK_HASH}, {SDLK_4, SDLK_DOLLAR}, {SDLK_5, '%'},
  {SDLK_6, SDLK_CARET}, {SDLK_7, SDLK_AMPERSAND}, {SDLK_8, SDLK_ASTERISK},
  {SDLK_9, SDLK_LEFTPAREN}, {SDLK_0, SDLK_RIGHTPAREN},
  {SDLK_MINUS, SDLK_UNDERSCORE}, {SDLK_PLUS, SDLK_EQUALS},
  {SDLK_LEFTBRACKET, '{'}, {SDLK_RIGHTBRACKET, '}'},
  {SDLK_SEMICOLON, SDLK_COLON}, {SDLK_QUOTE, SDLK_QUOTEDBL},
  {SDLK_BACKSLASH, '|'}, {SDLK_COMMA, SDLK_LESS}, {SDLK_PERIOD, SDLK_GREATER},
  {SDLK_SLASH, SDLK_QUESTION}
};


/* Utilites.
 * ---------
 */
static VOID _getSDLPixelFormat(SDL_PixelFormat *pSDLPixelFormat,
                               ULONG ulBPP, ULONG fccColorEncoding)
{
  ULONG ulRshift, ulGshift, ulBshift;
  ULONG ulRmask, ulGmask, ulBmask;
  ULONG ulRloss, ulGloss, ulBloss;

  pSDLPixelFormat->BitsPerPixel = ulBPP;
  pSDLPixelFormat->BytesPerPixel = (pSDLPixelFormat->BitsPerPixel + 7) / 8;

  switch (fccColorEncoding) {
  case FOURCC_LUT8:
    ulRshift = 0; ulGshift = 0; ulBshift = 0;
    ulRmask = 0; ulGmask = 0; ulBmask = 0;
    ulRloss = 8; ulGloss = 8; ulBloss = 8;
    break;

  case FOURCC_R555:
    ulRshift = 10; ulGshift = 5; ulBshift = 0;
    ulRmask = 0x7C00; ulGmask = 0x03E0; ulBmask = 0x001F;
    ulRloss = 3; ulGloss = 3; ulBloss = 3;
    break;

  case FOURCC_R565:
    ulRshift = 11; ulGshift = 5; ulBshift = 0;
    ulRmask = 0xF800; ulGmask = 0x07E0; ulBmask = 0x001F;
    ulRloss = 3; ulGloss = 2; ulBloss = 3;
    break;

  case FOURCC_R664:
    ulRshift = 10; ulGshift = 4; ulBshift = 0;
    ulRmask = 0xFC00; ulGmask = 0x03F0; ulBmask = 0x000F;
    ulRloss = 2; ulGloss = 4; ulBloss = 3;
    break;

  case FOURCC_R666:
    ulRshift = 12; ulGshift = 6; ulBshift = 0;
    ulRmask = 0x03F000; ulGmask = 0x000FC0; ulBmask = 0x00003F;
    ulRloss = 2; ulGloss = 2; ulBloss = 2;
    break;

  case FOURCC_RGB3:
  case FOURCC_RGB4:
    ulRshift = 0; ulGshift = 8; ulBshift = 16;
    ulRmask = 0x0000FF; ulGmask = 0x00FF00; ulBmask = 0xFF0000;
    ulRloss = 0x00; ulGloss = 0x00; ulBloss = 0x00;
    break;

  case FOURCC_BGR3:
  case FOURCC_BGR4:
    ulRshift = 16; ulGshift = 8; ulBshift = 0;
    ulRmask = 0xFF0000; ulGmask = 0x00FF00; ulBmask = 0x0000FF;
    ulRloss = 0; ulGloss = 0; ulBloss = 0;
    break;

  default:
    dbgprintf("Unknown color encoding: %.4s\n", fccColorEncoding);
  }

  pSDLPixelFormat->Rshift = ulRshift;
  pSDLPixelFormat->Gshift = ulGshift;
  pSDLPixelFormat->Bshift = ulBshift;
  pSDLPixelFormat->Rmask  = ulRmask;
  pSDLPixelFormat->Gmask  = ulGmask;
  pSDLPixelFormat->Bmask  = ulBmask;
  pSDLPixelFormat->Rloss  = ulRloss;
  pSDLPixelFormat->Gloss  = ulGloss;
  pSDLPixelFormat->Bloss  = ulBloss;

  pSDLPixelFormat->Ashift = 0x00;
  pSDLPixelFormat->Amask  = 0x00;
  pSDLPixelFormat->Aloss  = 0x00;
}

static int _compBPPSize(const void *p1, const void *p2)
{
  PBPPSIZE  pBPPSize1 = (PBPPSIZE)p1;
  PBPPSIZE  pBPPSize2 = (PBPPSIZE)p2;

  return (pBPPSize2->stSDLRect.w * pBPPSize2->stSDLRect.h) -
         (pBPPSize1->stSDLRect.w * pBPPSize1->stSDLRect.h);
}

static VOID _memnot(PBYTE pDst, PBYTE pSrc, ULONG ulLen)
{
  while (ulLen-- > 0)
    *pDst++ = ~*pSrc++;
}

static VOID _memxor(PBYTE pDst, PBYTE pSrc1, PBYTE pSrc2, ULONG ulLen)
{
  while (ulLen-- > 0)
    *pDst++ = (*pSrc1++)^(*pSrc2++);
}


/* GROP callback routines.
 * -----------------------
 */
static VOID cbMouseMove(PGROPDATA pGrop, BOOL fRelative, LONG lX, LONG lY)
{
  SDL_PrivateMouseMotion(0, fRelative, lX,
                         fRelative ? -lY : pGrop->stUserMode.ulHeight - lY);
}

static VOID cbMouseButton(PGROPDATA pGrop, ULONG ulButton, BOOL fDown)
{
  static ULONG  aBtnGROP2SDL[3] = { SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT,
                                    SDL_BUTTON_MIDDLE };

  SDL_PrivateMouseButton(fDown ? SDL_PRESSED : SDL_RELEASED,
                         aBtnGROP2SDL[ulButton], 0, 0);
}

static VOID cbKeyboard(PGROPDATA pGrop, ULONG ulScanCode, ULONG ulChar,
                       ULONG ulFlags)
{
  SDL_PrivateVideoData *pPVData = (SDL_PrivateVideoData *)gropGetUserPtr(pGrop);
  ULONG       ulIdx;
  SDLKey      enSDLKey;
  SDL_keysym  stSDLKeysym;

  /* Check for fastkeys: ALT+HOME to toggle FS mode
   *                     ALT+END to close app
   */
  if ((ulFlags & (KC_KEYUP | KC_ALT)) == KC_ALT) { /* Key pressed with ALT */
    switch (ulScanCode) {
    case 0x60: /* <HOME> */
      /* Alt+Home - switch fullscreen / window. */
      dbgprintf("Alt+Home pressed\n");
      if (!gropSetFullscreen(pPVData->pGrop, GROP_SET_FS_SWITCH))
          dbgprintf("gropSetFullscreen() failed\n");
      return;

    case 0x65: /* <END> */
      /* Alt+End - exit program. */
      dbgprintf("Alt+End pressed\n");
      SDL_PrivateQuit();
      /*gropClose(pPVData->pGrop);*/
      return;
    }
  }

#if 0
  if (((ulFlags & KC_KEYUP) == 0) &&    /* Key pressed. */
      (ulScanCode == 0x1C) &&           /* <ENTER>      */
      ((SDL_GetModState() & (KMOD_RALT | KMOD_MODE)) != 0)) { /* <AltGr> */
    dbgprintf("GrAlt+Enter pressed\n");
    if (!gropSetFullscreen(pPVData->pGrop, GROP_SET_FS_SWITCH))
      dbgprintf("gropSetFullscreen() failed\n");
    return;
  }
#endif

  /* Send key event to SDL. */
  enSDLKey = aScanSDLKeyMap[ulScanCode];
  stSDLKeysym.unicode = 0;

  if (SDL_TranslateUNICODE != 0) {
    if (ulChar != 0 && pPVData->ucoUnicode != NULL) {
      /* Detect unicode value for key */
      CHAR    achInput[2];
      PCHAR   pchInput = achInput;
      size_t  cInput = sizeof(achInput);
      UniChar auchOutput[4];
      UniChar *puchOutput = auchOutput;
      size_t  cOutput = sizeof(auchOutput);
      size_t  cSubst;
      ULONG   ulRC;

      achInput[0] = ulChar;
      achInput[1] = 0;
      ulRC = UniUconvToUcs(pPVData->ucoUnicode, (void**)&pchInput,
                           &cInput, &puchOutput, &cOutput, &cSubst);
      if (ulRC != ULS_SUCCESS)
        dbgprintf("UniUconvToUcs(), rc = %u\n", ulRC);
      else
        stSDLKeysym.unicode = auchOutput[0];
    }

    /* SDL-style convert key codes for unicode when Shift pressed */
    if ((ulFlags & KC_SHIFT) != 0)
      for (ulIdx = 0;
           ulIdx < sizeof(aUnicodeShiftKey) / sizeof(UNICODESHIFTKEY);
           ulIdx++)
        if (aUnicodeShiftKey[ulIdx].enSDLKey == enSDLKey) {
          enSDLKey = aUnicodeShiftKey[ulIdx].enSDLShiftKey;
          break;
        }
  }

  stSDLKeysym.scancode = ulScanCode;
  stSDLKeysym.mod = KMOD_NONE;
  stSDLKeysym.sym = enSDLKey;

  SDL_PrivateKeyboard((ulFlags & KC_KEYUP) != 0 ? SDL_RELEASED : SDL_PRESSED,
                       &stSDLKeysym);
}

static VOID cbActive(PGROPDATA pGrop, ULONG ulType, BOOL fSet)
{
  switch (ulType) {
  case GROUP_ACTIVATE_WINDOW:
    SDL_PrivateAppActive(fSet, SDL_APPACTIVE);
    ulType = SDL_APPMOUSEFOCUS;
    break;

  case GROUP_ACTIVATE_FOCUS:
    ulType = SDL_APPINPUTFOCUS;
    break;

  default:
    return;
  }

  SDL_PrivateAppActive(fSet, ulType);
}

static VOID cbSize(PGROPDATA pGrop, ULONG ulWidth, ULONG ulHeight)
{
  SDL_PrivateVideoData *pPVData = (SDL_PrivateVideoData *)gropGetUserPtr(pGrop);

  pPVData->ulWinWidth  = ulWidth;
  pPVData->ulWinHeight = ulHeight;
  pPVData->fWinResized = TRUE;
}

static BOOL cbQuit(PGROPDATA pGrop)
{
  /* Message WM_QUIT received. */
  SDL_PrivateQuit();
  return TRUE;
}


/* SDL interface routines.
 * -----------------------
 */
static void os2_InitOSKeymap(SDL_VideoDevice *pDevice)
{
  ULONG ulIdx;

  for (ulIdx = 0; ulIdx < sizeof(aScan2SDLKey) / sizeof(SCAN2SDLKEY); ulIdx++)
    aScanSDLKeyMap[aScan2SDLKey[ulIdx].ulScan] = aScan2SDLKey[ulIdx].enSDLKey;
}

static int os2_VideoInit(SDL_VideoDevice *pDevice,
                         SDL_PixelFormat *pSDLPixelFormat)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  PGROPDATA     pGrop;
  PBPPSIZE      pBPPSize;
  PVIDEOMODE    pMode;
  ULONG         ulIdx;
  ULONG         ulBPP, ulBPPMin = ~0, ulBPPMax = 0;
  ULONG         cModes;
  PBPPSIZELIST  pBPPSizeList;
  ULONG         ulRC;
  char         *driver;

  /* Select output video system. */
  ulIdx = GROP_VS_DIVE;
  driver = SDL_getenv("SDL_VIDEODRIVER");

  if (driver != NULL) {
    if (SDL_strcasecmp(driver, "VMAN") == 0)
      ulIdx = GROP_VS_VMAN_COMPATIBLE;
    else if (SDL_strcasecmp(driver, "VMANFS") == 0)
      ulIdx = GROP_VS_VMAN;
  }

  /* Create a new GrOp object. */
  pGrop = gropNew(ulIdx, &stGropCallback, (PVOID)pPVData);

  if (pGrop == NULL) {
    dbgprintf("gropNew() failed\n");
    return -1;
  }
  pPVData->pGrop = pGrop;

  pMode = &pGrop->stModes.pList[pGrop->stModes.ulDesktopMode];
  _getSDLPixelFormat(pSDLPixelFormat, pMode->ulBPP, pMode->fccColorEncoding);

  /* Make video modes BPP-sizes sorted lists for os2_ListModes(). */

  /* pPVData->paBPPSize - list of BPPs and sizes for all available video modes. */
  pPVData->paBPPSize = SDL_malloc(sizeof(BPPSIZE) * pGrop->stModes.ulCount);
  if (pPVData->paBPPSize == NULL) {
    SDL_OutOfMemory();
    dbgprintf("Not enough memory\n");
    gropFree(pGrop);
    pPVData->pGrop = NULL;
    return -1;
  }

  /* List BPPs and sizes of all video modes. */
  pBPPSize = pPVData->paBPPSize;
  pMode = pGrop->stModes.pList;
  for (ulIdx = 0; ulIdx < pGrop->stModes.ulCount; ulIdx++, pBPPSize++, pMode++) {
    pBPPSize->ulBPP = pMode->ulBPP;
    pBPPSize->stSDLRect.x = 0;
    pBPPSize->stSDLRect.y = 0;
    pBPPSize->stSDLRect.w = pMode->ulWidth;
    pBPPSize->stSDLRect.h = pMode->ulHeight;

    if (pMode->ulBPP < ulBPPMin)
      ulBPPMin = pMode->ulBPP;
    if (pMode->ulBPP > ulBPPMax)
      ulBPPMax = pMode->ulBPP;

    dbgprintf("#%u bpp: %u, %u x %u\n", ulIdx, pBPPSize->ulBPP,
              pBPPSize->stSDLRect.w, pBPPSize->stSDLRect.h);
  }
  /* Sort list by video mode sizes from largest to smallest. */
  SDL_qsort(pPVData->paBPPSize, pGrop->stModes.ulCount, sizeof(BPPSIZE),
            _compBPPSize);

  /* Make lists of sizes for the each available BPP. */
  for (ulBPP = ulBPPMin; ulBPP <= ulBPPMax; ulBPP++) {
    /* Count number of modes for current BPP. */
    cModes = 0;
    pBPPSize = pPVData->paBPPSize;
    for (ulIdx = 0; ulIdx < pGrop->stModes.ulCount; ulIdx++, pBPPSize++) {
      if (pBPPSize->ulBPP == ulBPP)
        cModes++;
    }

    if (cModes == 0)
      continue;

    /* We have BPP (ulBPP) and number of modes for this BPP (cModes). */

    /* Make a new list of sizes for current BPP. */
    pBPPSizeList = SDL_malloc(sizeof(BPPSIZELIST) * cModes);
    if (pBPPSizeList == NULL) {
      SDL_OutOfMemory ();
      dbgprintf("Not enough memory\n");
      break;
    }
    pBPPSizeList->ulBPP = ulBPP;

    /* Collect pointers to sizes (SDL_Rect objects) for current BPP (ulBPP). */
    cModes = 0;
    pBPPSize = pPVData->paBPPSize;
    dbgprintf("Video mode sizes for BPP %u:\n", ulBPP);
    for (ulIdx = 0; ulIdx < pGrop->stModes.ulCount; ulIdx++, pBPPSize++) {
      if (pBPPSize->ulBPP != ulBPP)
        continue;

      dbgprintf("  %u x %u\n", pBPPSize->stSDLRect.w, pBPPSize->stSDLRect.h);
      pBPPSizeList->apSDLRect[cModes] = &pBPPSize->stSDLRect;
      cModes++;
    }
    pBPPSizeList->apSDLRect[cModes] = NULL; /* NULL is end-of-list for SDL. */

    /* Insert the new BPP-sizes list in the linked list. */
    pBPPSizeList->pNext = pPVData->pBPPSizeList;
    pPVData->pBPPSizeList = pBPPSizeList;
  }

  /* Create uconv object. */
  ulRC = UniCreateUconvObject(L"", &pPVData->ucoUnicode);
  if (ulRC != ULS_SUCCESS) {
    dbgprintf("UniCreateUconvObject(), rc = %u\n", ulRC);
    pPVData->ucoUnicode = NULL;
  }

  return 0;
}

static void os2_VideoQuit(SDL_VideoDevice *pDevice)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  PBPPSIZELIST  pNextBPPSizeList;

  if (pPVData->pGrop != NULL) {
    gropFree(pPVData->pGrop);
    pPVData->pGrop = NULL;
  }

  while (pPVData->pBPPSizeList != NULL) {
    pNextBPPSizeList = pPVData->pBPPSizeList->pNext;
    SDL_free(pPVData->pBPPSizeList);
    pPVData->pBPPSizeList = pNextBPPSizeList;
  }

  if (pPVData->paBPPSize != NULL) {
    SDL_free(pPVData->paBPPSize);
    pPVData->paBPPSize = NULL;
  }

  if (pPVData->prectlReserved != NULL) {
    SDL_free(pPVData->prectlReserved);
    pPVData->prectlReserved = NULL;
  }

  if (pPVData->ucoUnicode != NULL) {
    UniFreeUconvObject(pPVData->ucoUnicode);
    pPVData->ucoUnicode = NULL;
  }

  if (pPVData->hptrWndIcon != NULLHANDLE) {
    WinDestroyPointer(pPVData->hptrWndIcon);
    pPVData->hptrWndIcon = NULLHANDLE;
  }
}

static SDL_Rect **os2_ListModes(SDL_VideoDevice *pDevice,
                                SDL_PixelFormat *pSDLPixelFormat,
                                Uint32 uiFlags)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  PBPPSIZELIST  pBPPSizeList = pPVData->pBPPSizeList;

  dbgprintf("Requested BPP: %u\n", pSDLPixelFormat->BitsPerPixel);

  for ( ; pBPPSizeList != NULL; pBPPSizeList = pBPPSizeList->pNext) {
    if (pBPPSizeList->ulBPP == pSDLPixelFormat->BitsPerPixel)
      return pBPPSizeList->apSDLRect;
  }

  dbgprintf("Unsupported BPP: %u\n", pSDLPixelFormat->BitsPerPixel);
  return NULL;
}

static SDL_Surface *os2_SetVideoMode(SDL_VideoDevice *pDevice,
                                     SDL_Surface *pSDLSurfaceCurrent,
                                     int iWidth, int iHeight, int iBPP,
                                     Uint32 uiFlags)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  GROPSETMODE     stGropSetMode;
  SDL_PixelFormat stSDLPixelFormat;
  SDL_Surface    *pSDLSurface;

  if (uiFlags & SDL_OPENGL) {
    SDL_SetError("OS2 driver not configured with OpenGL");
    return NULL;
  }

  /* Adjust the flags to be compatible with our system. */
  if ((uiFlags & SDL_FULLSCREEN) != 0)
    uiFlags &= ~SDL_RESIZABLE;

  if ((uiFlags & SDL_HWSURFACE) != 0)
    uiFlags |= SDL_SWSURFACE;

  uiFlags &= ~(SDL_DOUBLEBUF | SDL_HWSURFACE);

  /* Set video mode for GROP. */
  stGropSetMode.ulFlags = ((uiFlags & SDL_FULLSCREEN) != 0) ? GROP_MODEFL_FULLSCREEN : 0;
  if ((uiFlags & SDL_RESIZABLE) != 0)
    stGropSetMode.ulFlags |= GROP_MODEFL_RESIZABLE;

  stGropSetMode.ulWidth = iWidth;
  stGropSetMode.ulHeight = iHeight;
  stGropSetMode.ulBPP = iBPP;   /* input: recommended / output: used. */
  dbgprintf("Call gropSetMode()...\n");
  if (!gropSetMode(pPVData->pGrop, &stGropSetMode)) {
    dbgprintf("gropSetMode() failed");
    return NULL;
  }
  dbgprintf("Returned from gropSetMode()\n");

  /* Convert OS/2 color encoding of the new video mode to SDL color masks. */
  _getSDLPixelFormat(&stSDLPixelFormat, stGropSetMode.ulBPP,
                     stGropSetMode.fccColorEncoding);

  /* Create new SDL surface for the new video mode. */
  pSDLSurface = SDL_CreateRGBSurfaceFrom(
                    stGropSetMode.pBuffer,
                    stGropSetMode.ulWidth, stGropSetMode.ulHeight,
                    stGropSetMode.ulBPP, stGropSetMode.ulScanLineSize,
                    stSDLPixelFormat.Rmask, stSDLPixelFormat.Gmask,
                    stSDLPixelFormat.Bmask, stSDLPixelFormat.Amask);
  if (pSDLSurface == NULL) {
    dbgprintf("SDL_CreateRGBSurfaceFrom() failed\n");
    gropSetFullscreen(pPVData->pGrop, GROP_SET_FS_OFF);

    /* It is a potentially dangerous situation when the function
     * SDL_CreateRGBSurfaceFrom() returns NULL - old video buffer already
     * destroyed by gropSetMode() and current surface can not work correctly
     * any more. Let us hope that the set field "pixels" to NULL will suffice. */
    pSDLSurfaceCurrent->pixels = NULL;
    return NULL;
  }

  pSDLSurface->flags |= (uiFlags & (SDL_FULLSCREEN | SDL_RESIZABLE | SDL_HWPALETTE));

  /* Destroy old SDL surface. We are not worried about the old video buffer,
   * the function gropSetMode() keeps track of it. */
  SDL_FreeSurface(pSDLSurfaceCurrent);

  return pSDLSurface;
}

static void os2_UpdateRects(SDL_VideoDevice *pDevice, int cRects,
                            SDL_Rect *pRects)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  PRECTL  prectl, prectlList;
  ULONG   ulHeight = pPVData->pGrop->stUserMode.ulHeight;
  ULONG   ulIdx;

  /* Reallocate or get reserved buffer for the list of rectangles. */
  if (pPVData->crectlReserved < cRects) {
    prectlList = SDL_realloc(pPVData->prectlReserved,
                             sizeof(RECTL) * cRects);
    if (prectlList == NULL) {
      SDL_OutOfMemory ();
      dbgprintf("Not enough memory\n");
      return;
    }
    pPVData->crectlReserved = cRects;
    pPVData->prectlReserved = prectlList;
  }
  else
    prectlList = pPVData->prectlReserved;

  /* Convert rectangles from SDL format to GROP format. */
  prectl = prectlList;
  for (ulIdx = 0; ulIdx < cRects; ulIdx++, prectl++, pRects++) {
    prectl->xLeft = pRects->x;
    prectl->xRight = pRects->x + pRects->w;

    prectl->yTop = ulHeight - pRects->y;
    prectl->yBottom = prectl->yTop - pRects->h;
  }

  /* Update requested rectangles. */
  if (!gropUpdate(pPVData->pGrop, cRects, prectlList))
    dbgprintf("gropUpdate() failed\n");
}

static int os2_ToggleFullScreen(SDL_VideoDevice *pDevice, int iOn)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;

  if (!gropSetFullscreen(pPVData->pGrop,
                         iOn == 0 ? GROP_SET_FS_OFF : GROP_SET_FS_ON)) {
    dbgprintf("gropSetFullscreen() failed\n");
    return 0;
  }

  return 1;
}

static void os2_UpdateMouse(SDL_VideoDevice *pDevice)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  ULONG ulX, ulY;

  if (!gropGetMousePos(pPVData->pGrop, &ulX, &ulY))
    dbgprintf("gropGetMousePos() failed\n");
  else {
    dbgprintf("call SDL_PrivateMouseMotion(0, 0, %u, %u)\n", ulX, ulY);
    SDL_PrivateMouseMotion(0, 0, ulX, ulY);
  }
}

static int os2_SetColors(SDL_VideoDevice *pDevice, int iFirst, int iCount,
                         SDL_Color *pColors)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  PRGB2 pRGB2Colors = SDL_malloc(sizeof(RGB2) * iCount);
  PRGB2 pRGB2;
  ULONG ulIdx;
  BOOL  fSuccess;

  if (pRGB2Colors == NULL) {
    SDL_OutOfMemory ();
    dbgprintf("Not enough memory\n");
    return 0;
  }

  dbgprintf("iFirst = %d, iCount = %d\n", iFirst, iCount);
  /* Convert SDL color records of palette to GROP color records. */
  pRGB2 = pRGB2Colors;
  for (ulIdx = 0; ulIdx < iCount; ulIdx++, pColors++, pRGB2++) {
    *(PULONG)pRGB2 = 0; /* .fcOptions = 0 */
    pRGB2->bBlue   = pColors->b;
    pRGB2->bGreen  = pColors->g;
    pRGB2->bRed    = pColors->r;
  }

  fSuccess = gropSetPaletter(pPVData->pGrop, iFirst, iCount, pRGB2Colors);
  SDL_free(pRGB2Colors);

  if (!fSuccess) {
    dbgprintf("gropSetPaletter() failed\n");
    return 0;
  }

  return 1;
}

static int os2_AllocHWSurface(SDL_VideoDevice *pDevice,
                              SDL_Surface *pSDLSurface)
{
  return -1;
}

static void os2_FreeHWSurface(SDL_VideoDevice *pDevice,
                              SDL_Surface *pSDLSurface)
{
}

static int os2_LockHWSurface(SDL_VideoDevice *pDevice, SDL_Surface *pSDLSurface)
{
  return 0;
}

static void os2_UnlockHWSurface(SDL_VideoDevice *pDevice,
                                SDL_Surface *pSDLSurface)
{
}

static void os2_SetCaption(SDL_VideoDevice *pDevice, const char *pcTitle,
                           const char *pcIcon)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;

  if (!gropSetWindowTitle(pPVData->pGrop, (PSZ)pcTitle))
    dbgprintf("gropSetWindowTitle() failed\n");
}

static void os2_SetIcon(SDL_VideoDevice *pDevice, SDL_Surface *pSDLSurfaceIcon,
                        Uint8 *puiMask)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  SDL_Surface       *SDLSurfIcon;
  HBITMAP           hbm;
  BITMAPINFOHEADER2 bmih = { 0 };
  BITMAPINFO        bmi = { 0 };
  HPS               hps;
  PULONG            pulBitmap;
  SDL_Rect          sSDLRect;
  ULONG             ulX, ulY;
  CHAR              chMask;
  PULONG            pulDst, pulSrc, pulDstMask;

  /* Remove previous window's icon */
  if (pPVData->hptrWndIcon != NULLHANDLE) {
    WinDestroyPointer(pPVData->hptrWndIcon);
    pPVData->hptrWndIcon = NULLHANDLE;
    gropSetWindowIcon(pPVData->pGrop, NULLHANDLE);
  }

  if (pSDLSurfaceIcon == NULL || puiMask == NULL)
    return;

  sSDLRect.x = 0;
  sSDLRect.y = 0;
  sSDLRect.w = pSDLSurfaceIcon->w;
  sSDLRect.h = pSDLSurfaceIcon->h;

  /* Make icon surface (32 bpp). It will be read to the result bitmap. */

  SDLSurfIcon = SDL_CreateRGBSurface(SDL_SWSURFACE, sSDLRect.w, sSDLRect.h,
                                     32, 0, 0, 0, 0);
  if (SDLSurfIcon == NULL) {
    dbgprintf("SDL_CreateRGBSurface() failed\n");
    return;
  }

  if (SDL_LowerBlit(pSDLSurfaceIcon, &sSDLRect, SDLSurfIcon, &sSDLRect) < 0) {
    dbgprintf("SDL_LowerBlit() failed\n");
    SDL_FreeSurface(SDLSurfIcon);
    return;
  }

  /* Make result bitmap: image (upper half) + mask (lower half)
   * It fills from bottom to top line, source image and mask read forward. */
  pulBitmap = SDL_malloc(2 * (sSDLRect.h * sSDLRect.w * 4));
  if (pulBitmap == NULL) {
    dbgprintf("Not enough memory\n");
    SDL_OutOfMemory ();
    SDL_FreeSurface(SDLSurfIcon);
    return;
  }
  /* pulDst - last line of icon (image) part of the result bitmap's */
  pulDst = &pulBitmap[(sSDLRect.h - 1) * sSDLRect.w];
  /* pulDstMask - last line of mask part of the result bitmap's */
  pulDstMask = &pulBitmap[(2 * sSDLRect.h - 1) * sSDLRect.w];
  /* pulSrc - first line of source image */
  pulSrc = (PULONG) (((PCHAR)SDLSurfIcon->pixels));

  /* Fill icon and mask parts of the result bitmap */
  for (ulY = 0; ulY < sSDLRect.h; ulY++) {
    for (ulX = 0; ulX < sSDLRect.w; ulX++) {
      /* Get next bit from mask */
      if ((ulX % 8) == 0) {
        chMask = *puiMask;
        puiMask++;
      }
      else
        chMask <<= 1;

      if ((chMask & 0x80) != 0) {
        pulDst[ulX] = pulSrc[ulX];
        pulDstMask[ulX] = 0;
      }
      else {
        pulDst[ulX] = 0;
        pulDstMask[ulX] = 0xFFFFFFFF;
      }
    }

    /* Set image and mask pointers on one line up */
    pulDst -= sSDLRect.w;
    pulDstMask -= sSDLRect.w;
    /* Set source image pointer to the next line */
    pulSrc = (PULONG) (((PCHAR)pulSrc) + SDLSurfIcon->pitch);
  }

  SDL_FreeSurface(SDLSurfIcon);

  /* Create system bitmap object */
  bmih.cbFix          = sizeof(BITMAPINFOHEADER2);
  bmih.cx             = sSDLRect.w;
  bmih.cy             = 2 * sSDLRect.h;
  bmih.cPlanes        = 1;
  bmih.cBitCount      = 32;
  bmih.ulCompression  = BCA_UNCOMP;
  bmih.cbImage        = bmih.cx * bmih.cy * 4;

  bmi.cbFix           = sizeof(BITMAPINFOHEADER);
  bmi.cx              = bmih.cx;
  bmi.cy              = bmih.cy;
  bmi.cPlanes         = 1;
  bmi.cBitCount       = 32;

  hps = WinGetPS(HWND_DESKTOP);
  hbm = GpiCreateBitmap(hps, (PBITMAPINFOHEADER2)&bmih, CBM_INIT,
                        (PBYTE)pulBitmap, (PBITMAPINFO2)&bmi);
  if (hbm == GPI_ERROR)
    dbgprintf("GpiCreateBitmap() failed\n");
  else {
    /* Create pointer (icon) object. The bit map will be stretched (if
     * necessary) to the system pointer dimensions (third argument is TRUE). */
    pPVData->hptrWndIcon = WinCreatePointer(HWND_DESKTOP, hbm, TRUE, 0, 0);
    GpiDeleteBitmap(hbm);

    if (pPVData->hptrWndIcon == NULLHANDLE)
      dbgprintf("WinCreatePointer() failed\n");
    else if (!gropSetWindowIcon(pPVData->pGrop, pPVData->hptrWndIcon))
      dbgprintf("gropSetIcon() failed\n");
  }
  WinReleasePS(hps);
  SDL_free(pulBitmap);
}

static int os2_IconifyWindow(SDL_VideoDevice *pDevice)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;

  if (!gropMinimize(pPVData->pGrop))
  {
    dbgprintf("gropMinimize() failed\n");
    return 0;
  }

  return 1;
}

static SDL_GrabMode os2_GrabInput(SDL_VideoDevice *pDevice, SDL_GrabMode mode)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  BOOL  fCapture = mode != SDL_GRAB_OFF;

  dbgprintf("mode (%d): %s\n", mode, fCapture ? "capture" : "release");
  if (!gropCapture(pPVData->pGrop, fCapture))
    dbgprintf("gropCapture() failed\n");

  return mode;
}

static WMcursor *os2_CreateWMCursor(SDL_VideoDevice *pDevice,
                                    Uint8 *puiData, Uint8 *puiMask,
                                    int iW, int iH, int iHotX, int iHotY)
{
/*SDL_PrivateVideoData *pPVData = pDevice->hidden;*/
  BITMAPINFOHEADER  bmih = { 0 };
  BITMAPINFO        bmi;
  HPS               hps;
  PBYTE             pcImage;
  PBYTE             pcPixels;
  ULONG             ulMaxX = WinQuerySysValue(HWND_DESKTOP, SV_CXPOINTER);
  ULONG             ulMaxY = WinQuerySysValue(HWND_DESKTOP, SV_CYPOINTER);
  ULONG             ulMaxXBRnd, ulBRnd, ulPad;
  ULONG             ulIdx;
  HBITMAP           hbm;
  WMcursor *pResult;

  if (ulMaxX == 0 || ulMaxY == 0) {
    dbgprintf("WinQuerySysValue() failed\n");
    return NULL;
  }

  if (iW > ulMaxX || iH > ulMaxY) {
    dbgprintf("Given size is: %u x %u, max. is %u x %u\n", iW, iH, ulMaxX, ulMaxY);
    return NULL;
  }

  /* Make image and mask of cursor at buffer. */
  ulMaxXBRnd = (ulMaxX + 7) / 8;
  ulBRnd = (iW + 7) / 8;
  ulPad = ulMaxXBRnd - ulBRnd;

  pcImage = SDL_malloc(ulMaxXBRnd * ulMaxY * 2);
  if (pcImage == NULL) {
    SDL_OutOfMemory ();
    dbgprintf("Not enough memory\n");
    return NULL;
  }

  for (ulIdx = 0; ulIdx < iH; ulIdx++) {
    pcPixels = &pcImage[ulMaxXBRnd * (ulMaxY - 1 - ulIdx)];
    _memxor(pcPixels, puiData, puiMask, ulBRnd);
    SDL_memset(&pcPixels[ulBRnd],  0, ulPad);
    puiData += ulBRnd;

    pcPixels = &pcImage[ulMaxXBRnd * ((2 * ulMaxY) - 1 - ulIdx)];
    _memnot(pcPixels, puiMask, ulBRnd);
    SDL_memset(&pcPixels[ulBRnd], 0xFF, ulPad);
    puiMask += ulBRnd;
  }

  for ( ; ulIdx < ulMaxY; ulIdx++) {
    SDL_memset(&pcImage[ulMaxXBRnd * (ulMaxY - 1 - ulIdx)],  0, ulMaxXBRnd);
    SDL_memset(&pcImage[ulMaxXBRnd * ((2 * ulMaxY) - 1 - ulIdx)], 0xFF,
                ulMaxXBRnd);
  }

  /* Create cursor object. */
  bmi.cbFix     = sizeof(BITMAPINFOHEADER);
  bmi.cx        = ulMaxX;
  bmi.cy        = ulMaxY * 2;
  bmi.cPlanes   = 1;
  bmi.cBitCount = 1;
  bmi.argbColor[0].bBlue  = 0x00;
  bmi.argbColor[0].bGreen = 0x00;
  bmi.argbColor[0].bRed   = 0x00;
  bmi.argbColor[1].bBlue  = 0x00;
  bmi.argbColor[1].bGreen = 0x00;
  bmi.argbColor[1].bRed   = 0xFF;

  bmih.cbFix      = sizeof(BITMAPINFOHEADER);
  bmih.cx         = ulMaxX;
  bmih.cy         = ulMaxY * 2;
  bmih.cPlanes    = 1;
  bmih.cBitCount  = 1;

  /* Create OS/2 pointer. */
  hps = WinGetPS(HWND_DESKTOP);
  hbm = GpiCreateBitmap(hps, (PBITMAPINFOHEADER2)&bmih, CBM_INIT, pcImage,
                        (PBITMAPINFO2)&bmi);
  if (hbm == NULLHANDLE)
    pResult = NULL;
  else {
    HPOINTER  hPointer = WinCreatePointer(HWND_DESKTOP, hbm, TRUE, iHotX,
                                          ulMaxY - iHotY - 1);

#ifdef VBOX_HACK_SUPPORT
    pResult = SDL_malloc(sizeof(struct WMcursor));
    if (pResult != NULL)
      pResult->hptr = hPointer;
#else
    pResult = (WMcursor *)hPointer;
#endif

    GpiDeleteBitmap(hbm);
  }
  WinReleasePS(hps);

  SDL_free(pcImage);

  dbgprintf("New cursor 0x%P\n", pResult);

  return pResult;
}

static void os2_FreeWMCursor(SDL_VideoDevice *pDevice, WMcursor *pCursor)
{
#ifdef VBOX_HACK_SUPPORT
  if (pCursor == NULL)
    return;

  WinDestroyPointer(pCursor->hptr);
  SDL_free(pCursor);
#else
  if ((HPOINTER)pCursor == NULLHANDLE)
    return;

  WinDestroyPointer((HPOINTER)pCursor);
#endif
}

static int os2_ShowWMCursor(SDL_VideoDevice *pDevice, WMcursor *pCursor)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;

  dbgprintf("Set cursor 0x%P\n", pCursor);
  if (!gropSetPointer(pPVData->pGrop,
#ifdef VBOX_HACK_SUPPORT
                      pCursor == NULL ? NULLHANDLE : pCursor->hptr
#else
                      (HPOINTER)pCursor
#endif
      )) {
    dbgprintf("gropSetPointer() failed\n");
    return 0;
  }

  return 1;
}

static void os2_WarpWMCursor(SDL_VideoDevice *pDevice, Uint16 uiX, Uint16 uiY)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  ULONG ulHeight = pPVData->pGrop->stUserMode.ulHeight;

  if (!gropMouseMove(pPVData->pGrop, uiX, ulHeight - uiY))
    dbgprintf("gropMouseMove() failed\n");
}

static void os2_PumpEvents(SDL_VideoDevice *pDevice)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;
  BOOL  fWinResized;
  ULONG ulWinWidth, ulWinHeight;
  ULONG ulTime;

  gropLock(pPVData->pGrop);             /* Block callbacks calls. */
  fWinResized = pPVData->fWinResized;   /* Get size changes flag. */
  if (fWinResized) {
    pPVData->fWinResized = FALSE;       /* Clear size changes flag. */
    ulWinWidth  = pPVData->ulWinWidth;  /* Get new window width. */
    ulWinHeight = pPVData->ulWinHeight; /* Get new window height. */
  }
  gropUnlock(pPVData->pGrop);           /* Allow callbacks calls. */

  if (fWinResized) {
    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &ulTime, sizeof(ULONG));

    /* Do not call SDL_PrivateResize() too often (min. time interval - 1 sec.). */
    if ((ULONG)(ulTime - pPVData->ulResizedReportTime) >= 1000) {
      pPVData->ulResizedReportTime = ulTime;
      SDL_PrivateResize(ulWinWidth, ulWinHeight);
    }
  }
}

static void os2_DeleteDevice(SDL_VideoDevice *pDevice)
{
  SDL_PrivateVideoData *pPVData = pDevice->hidden;

  gropDone();
  SDL_free(pPVData);
  SDL_free(pDevice);
}

static SDL_VideoDevice *os2_CreateDevice(int devindex)
{
  SDL_VideoDevice      *pDevice;
  SDL_PrivateVideoData *pPVData;

  pDevice = SDL_calloc(1, sizeof(SDL_VideoDevice));
  if (pDevice == NULL) {
    SDL_OutOfMemory ();
    dbgprintf("Not enough memory\n");
    return NULL;
  }

  /* Driver can center a smaller surface to simulate fullscreen */
  pDevice->handles_any_size = 1;

  pDevice->InitOSKeymap     = os2_InitOSKeymap;
  pDevice->VideoInit        = os2_VideoInit;
  pDevice->VideoQuit        = os2_VideoQuit;
  pDevice->ListModes        = os2_ListModes;
  pDevice->SetVideoMode     = os2_SetVideoMode;
  pDevice->UpdateRects      = os2_UpdateRects;
  pDevice->ToggleFullScreen = os2_ToggleFullScreen;
  pDevice->UpdateMouse      = os2_UpdateMouse;
  pDevice->SetColors        = os2_SetColors;
  pDevice->AllocHWSurface   = os2_AllocHWSurface;
  pDevice->LockHWSurface    = os2_LockHWSurface;
  pDevice->UnlockHWSurface  = os2_UnlockHWSurface;
  pDevice->FreeHWSurface    = os2_FreeHWSurface;
  pDevice->SetCaption       = os2_SetCaption;
  pDevice->SetIcon          = os2_SetIcon;
  pDevice->IconifyWindow    = os2_IconifyWindow;
  pDevice->GrabInput        = os2_GrabInput;
  pDevice->CreateWMCursor   = os2_CreateWMCursor;
  pDevice->FreeWMCursor     = os2_FreeWMCursor;
  pDevice->ShowWMCursor     = os2_ShowWMCursor;
  pDevice->WarpWMCursor     = os2_WarpWMCursor;
  pDevice->PumpEvents       = os2_PumpEvents;
  pDevice->free             = os2_DeleteDevice;

  /* Private video data */
  pPVData = SDL_calloc(1, sizeof(struct SDL_PrivateVideoData));
  if (pPVData == NULL) {
    SDL_OutOfMemory ();
    dbgprintf("Not enough memory\n");
    SDL_free(pDevice);
    return NULL;
  }
  pDevice->hidden = pPVData;

  if (!gropInit()) {
    dbgprintf("gropInit() failed\n");
    SDL_free(pPVData);
    SDL_free(pDevice);
    return NULL;
  }

  return pDevice;
}

static int os2_Available(void)
{
  return 1;
}

VideoBootStrap OS2GROP_bootstrap = {
 "OS2", "OS/2 GrOp",
 os2_Available, os2_CreateDevice
};

#endif /* SDL_VIDEO_DRIVER_OS2GROP */
