#ifndef __FSLIB_H__
#define __FSLIB_H__

#define INCL_WIN
#include <os2.h>

#define FSLIBCALL _System
#define DECLSPEC  __declspec(dllexport)

typedef struct _FSLib_PixelFormat
{
  unsigned char ucRedMask;
  unsigned char ucRedPosition;
  unsigned char ucRedAdjust;

  unsigned char ucGreenMask;
  unsigned char ucGreenPosition;
  unsigned char ucGreenAdjust;

  unsigned char ucBlueMask;
  unsigned char ucBluePosition;
  unsigned char ucBlueAdjust;

  unsigned char ucAlphaMask;
  unsigned char ucAlphaPosition;
  unsigned char ucAlphaAdjust;

} FSLib_PixelFormat, *FSLib_PixelFormat_p;

typedef struct _FSLib_VideoMode
{
  unsigned int       uiXResolution;
  unsigned int       uiYResolution;
  unsigned int       uiScanLineSize;
  unsigned int       uiBPP;
  FSLib_PixelFormat  PixelFormat;

  void *pNext;

} FSLib_VideoMode, *FSLib_VideoMode_p;

// Initialization of FSLib for the calling process.
// Returns 1 if successful!
DECLSPEC int               FSLIBCALL FSLib_Initialize();

// Uninitialization of FSLib.
DECLSPEC void              FSLIBCALL FSLib_Uninitialize();

// Get all available fullscreen video modes
DECLSPEC FSLib_VideoMode_p FSLIBCALL FSLib_GetVideoModeList();
// Free list of available fullscreen video modes
DECLSPEC int               FSLIBCALL FSLib_FreeVideoModeList(FSLib_VideoMode_p pVideoModeListHead);
// Get pointer to desktop video mode (Don't free it, it's static!)
DECLSPEC FSLib_VideoMode_p FSLIBCALL FSLib_GetDesktopVideoMode();


// Create a FSLib client window with given frame, telling the
// initial source buffer parameters
DECLSPEC int               FSLIBCALL FSLib_CreateWindow(HWND   hwndParent,     // Parent window handle
                                              ULONG  flStyle,        // Frame window style
                                              PULONG pflCreateFlags, //Frame-creation flags
                                              PSZ    pszTitle,       // Title bar text
                                              HMODULE hmod,          // Optional module handle for resources
                                              ULONG idResources,     // Optional resources
                                              FSLib_VideoMode_p pSrcBufferDesc, // Description of source buffer format
                                              PFNWP  pfnwpUserWindowProc, // User window proc
                                              PHWND  phwndClient,    // Result 1 : client window handle
                                              PHWND  phwndFrame);    // Result 2 : frame window handle

// Set an user parameter to FSLib client window
DECLSPEC int               FSLIBCALL FSLib_AddUserParm(HWND hwndClient,
                                             void *pUserParm);
// Get the user parameter from FSLib client window
DECLSPEC void            * FSLIBCALL FSLib_GetUserParm(HWND hwndClient);

// Switch to/from Fullscreen mode
DECLSPEC int               FSLIBCALL FSLib_ToggleFSMode(HWND hwndClient,
                                              int iInFullscreenMode);
// Query if the application runs in Fullscreen mode
DECLSPEC int               FSLIBCALL FSLib_QueryFSMode(HWND hwndClient);

// Change source buffer parameters on the fly
// (does not change window size in windowed mode,
//  but can change fullscreen video mode in fullscreen mode!)
DECLSPEC int               FSLIBCALL FSLib_SetSrcBufferDesc(HWND hwndClient,
                                                  FSLib_VideoMode_p pSrcBufferDesc); // Description of source buffer format

// Blit something into the window
// (it will take care of clipping, resizing and color space conversion if needed)
DECLSPEC int               FSLIBCALL FSLib_BitBlt(HWND hwndClient,
                                        void *pSourceImage,
                                        unsigned int uiTop,
                                        unsigned int uiLeft,
                                        unsigned int uiWidth,
                                        unsigned int uiHeight);

// Set the PixelFormat field to some pre-defined formats:
#define FSLIB_PIXELFORMAT_555_15BPP     0
#define FSLIB_PIXELFORMAT_565_16BPP     1
#define FSLIB_PIXELFORMAT_RGB_24BPP     2
#define FSLIB_PIXELFORMAT_BGR_24BPP     3
#define FSLIB_PIXELFORMAT_ARGB_32BPP    4
#define FSLIB_PIXELFORMAT_ABGR_32BPP    5
#define FSLIB_PIXELFORMAT_RGBA_32BPP    6
#define FSLIB_PIXELFORMAT_BGRA_32BPP    7
DECLSPEC int               FSLIBCALL FSLib_SetPixelFormat(FSLib_PixelFormat_p pPixelFormat, int iFormat);
DECLSPEC int               FSLIBCALL FSLib_IsFSLibWindow(HWND hwndClient);

// FSLib notifications to user windowproc:
// ---------------------------------------
//
#define WM_FSLIBNOTIFICATION     WM_USER
//
// (Passed in mp1, parameters passed in mp2)
// FSLN_TOGGLEFSMODE
//   Notifies user window proc about changing
//   to/from fullscreen mode (flag is in mp2)
#define FSLN_TOGGLEFSMODE        0

#endif
