// Include the main C header files
#include <stdlib.h>
#include <process.h>
#include <string.h>
#ifdef DEBUG_BUILD
#include <stdio.h>
#endif
// Then the needed OS/2 API stuffs
#define INCL_TYPES
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSEXCEPTIONS
#define INCL_GPI
#define INCL_DOSPROFILE
#include <os2.h>
#define INCL_GRE_DEVICE
#define INCL_GRE_DEVMISC
#include <pmddi.h>

// Also need some from Scitech SNAP
#include "snap/graphics.h"
#include "pmapi.h"
// And our header file
#include "FSLib.h"

// SW-Cursor hack is not ready yet. :(
//#define SWCURSOR_HACK

#define USE_ENABLEDIRECTACCESS
#define USE_SETDRAWSURFACE


#define FSLIB_SETVIDEOMODE_TIMEOUT 10000

// Per process global variables:

#pragma off (unreferenced);
#ifdef DEBUG_BUILD
static char *pchBuildID = "$$ FSLib - Debug build - "__DATE__" "__TIME__" $$";
#else
static char *pchBuildID = "$$ FSLib - Release build - "__DATE__" "__TIME__" $$";
#endif
#pragma on (unreferenced);

// SNAP-related
static GA_devCtx        *dc = NULL; // Driver context
static GA_initFuncs      initFuncs;
static GA_driverFuncs    driverFuncs;
static GA_configInfo     configInfo;
static GA_modeInfo       desktopModeInfo;
static FSLib_VideoMode   desktopVideoModeInfo; // In FSLib format
static int               desktopModeID;
static GA_modeInfo       currentModeInfo;
static int               currentModeID;
static int               iInPM; // True if the PM is the active mode
static REF2D_driver     *ref2ddrv = NULL;
static GA_2DRenderFuncs  draw2d;
static GA_2DStateFuncs   state2d;
#ifdef SWCURSOR_HACK
static GA_cursorFuncs    cursorFuncs;
#endif
static int               bUnloadRef2D = 0;
static HAB               hab;
static HMQ               hmq;

static int               tidSNAPThread;
static int               iSNAPThreadStatus;
static HMTX              hmtxUseSNAP = NULLHANDLE;
static HEV               hevSNAPCommandAvailable;  // Posted when there is a command for the SNAP-Thread!
static HEV               hevSNAPResultReady;       // Posted when the result is ready for the command.
static int               iSNAPCommand;             // Command code
static void             *pSNAPCommandParameter;    // Command parameter (if needed)

#define SNAPTHREADSTATUS_UNKNOWN        0
#define SNAPTHREADSTATUS_RUNNING_OK     1
#define SNAPTHREADSTATUS_STOPPED_ERROR  2
#define SNAPTHREADSTATUS_STOPPED_OK     3

#define SNAPTHREADCOMMAND_SHUTDOWN      0
#define SNAPTHREADCOMMAND_SETVIDEOMODE  1

// FSLib-related
typedef struct _FSLib_HWNDList
{
  HWND hwndClient;
  void *pNext;
} FSLib_HWNDList, *FSLib_HWNDList_p;

static FSLib_HWNDList_p  pHWNDListHead = NULL;
static HMTX              hmtxUseHWNDList = NULLHANDLE;
static int               bFSLib_Initialized = 0;

#ifdef DEBUG_BUILD
static unsigned long ulTimerFreq = 1;
static unsigned long long llStartTime = 0;

static void InitTimer()
{
  DosTmrQueryFreq(&ulTimerFreq);
  DosTmrQueryTime((PQWORD) &llStartTime);
}

static unsigned long long GetTimeElapsed()
{
  unsigned long long llTime;

  DosTmrQueryTime((PQWORD) &llTime);
  return llTime-llStartTime;
}
#endif

// Some prototypes
static int  SNAP_Initialize();
static void SNAP_Uninitialize();
static int  SNAP_StartVideoModeUsage();
static void SNAP_StopVideoModeUsage();
static void SNAP_SetVideoMode_core(GA_modeInfo *pVideoMode);
static void SNAP_SetVideoMode(GA_modeInfo *pVideoMode);
static void SNAP_ThreadFunc(void *pParm);

static int  FSLib_RegisterWindowClass();
static void FSLib_FindBestFSMode(FSLib_VideoMode_p pSrcBufferDesc,
				 GA_modeInfo *pFSModeInfo);

static void SNAP_StopVideoModeUsage()
{
  if (bUnloadRef2D)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_StopVideoModeUsage] : Unloading ref2ddrv\n");
#endif
    GA_unloadRef2d(dc); ref2ddrv = NULL;
    bUnloadRef2D = 0;
  }
}

static int SNAP_StartVideoModeUsage()
{
  int iResult = 1;

  bUnloadRef2D = 0;

  ref2ddrv = GA_getCurrentRef2d(dc->DeviceIndex);
  if (!ref2ddrv)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_StartVideoModeUsage] : No ref2ddrv, loading one!\n");
#endif
    if (!GA_loadRef2d(dc,
		      true,
		      &currentModeInfo,
		      currentModeInfo.YResolution * currentModeInfo.BytesPerScanLine,
		      &ref2ddrv))
    {
#ifdef DEBUG_BUILD
      printf("[SNAP_StartVideoModeUsage] : Could not load reference 2d driver!\n");
#endif
      iResult = 0;
    } else
    {
      GA_buffer drawBuf;

      // Set the active draw buffer
      drawBuf.dwSize = sizeof(drawBuf);
      drawBuf.Offset = 0;
      drawBuf.Stride = currentModeInfo.BytesPerScanLine;
      drawBuf.Width  = currentModeInfo.XResolution;
      drawBuf.Height = currentModeInfo.YResolution;
      if (ref2ddrv->SetDrawBuffer(&drawBuf,
				  dc->LinearMem,
				  currentModeInfo.BitsPerPixel,
				  &(currentModeInfo.PixelFormat),
				  dc,true) != 0)
      {
#ifdef DEBUG_BUILD
        printf("[SNAP_StartVideoModeUsage] : REF2D_SetDrawBuffer failed!\n");
#endif
      }

      bUnloadRef2D = 1;
    }
  }

  if (ref2ddrv)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_StartVideoModeUsage] : Query functions\n");
#endif
    draw2d.dwSize = sizeof(draw2d);
    if (!REF2D_queryFunctions(ref2ddrv, GA_GET_2DRENDERFUNCS, &draw2d))
    {
#ifdef DEBUG_BUILD
      printf("[SNAP_StartVideoModeUsage] : Could not query 2d render funcs!\n");
#endif
      iResult = 0;
    }
    state2d.dwSize = sizeof(state2d);
    if (!REF2D_queryFunctions(ref2ddrv, GA_GET_2DSTATEFUNCS, &state2d))
    {
#ifdef DEBUG_BUILD
      printf("[SNAP_StartVideoModeUsage] : Could not query 2d state funcs!\n");
#endif
      iResult = 0;
    }
#ifdef SWCURSOR_HACK
    cursorFuncs.dwSize = sizeof(cursorFuncs);
    if (!REF2D_queryFunctions(ref2ddrv, GA_GET_CURSORFUNCS, &cursorFuncs))
    {
#ifdef DEBUG_BUILD
      printf("[SNAP_StartVideoModeUsage] : Could not query cursor funcs!\n");
#endif
      iResult = 0;
    }
#endif
  }
  return iResult;
}


static void SNAP_Uninitialize()
{
  //  If current video mode != desktop video mode, then set desktop video mode!
  if (!iInPM)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_Uninitialize] : Setting back desktop mode!\n");
#endif
    if (DosRequestMutexSem(hmtxUseSNAP, 1000)==NO_ERROR) // 1 sec timeout
    {
      ULONG ulPostCount;
#ifdef DEBUG_BUILD
      printf("[SNAP_Uninitialize] : Asking SNAP-Thread to change video mode!\n");
#endif
      iSNAPCommand = SNAPTHREADCOMMAND_SETVIDEOMODE;
      pSNAPCommandParameter = &desktopModeInfo;
      DosResetEventSem(hevSNAPResultReady, &ulPostCount);
      DosPostEventSem(hevSNAPCommandAvailable);
      DosWaitEventSem(hevSNAPResultReady, 1500); // 1.5 sec timeout
      DosReleaseMutexSem(hmtxUseSNAP);
    } else
    {
      ULONG ulPostCount;
#ifdef DEBUG_BUILD
      printf("[SNAP_Uninitialize] : Timeout waiting for hmtxUseSNAP semaphore!\n");
      printf("                      Trying without semaphore!\n");
#endif
      iSNAPCommand = SNAPTHREADCOMMAND_SETVIDEOMODE;
      pSNAPCommandParameter = &desktopModeInfo;
      DosResetEventSem(hevSNAPResultReady, &ulPostCount);
      DosPostEventSem(hevSNAPCommandAvailable);
      DosWaitEventSem(hevSNAPResultReady, 1500); // 1.5 sec timeout
    }
  }

  // Tell the SNAP-Thread to uninitialize and shut down!
  if (iSNAPThreadStatus == SNAPTHREADSTATUS_RUNNING_OK)
  {
    if (DosRequestMutexSem(hmtxUseSNAP, 1000)==NO_ERROR) // 1 sec timeout
    {
      ULONG ulPostCount;
#ifdef DEBUG_BUILD
      printf("[SNAP_Uninitialize] : Shutting down SNAP-Thread...\n");
#endif
      iSNAPCommand = SNAPTHREADCOMMAND_SHUTDOWN;
      DosResetEventSem(hevSNAPResultReady, &ulPostCount);
      DosPostEventSem(hevSNAPCommandAvailable);
      DosWaitEventSem(hevSNAPResultReady, 1500); // 1.5 sec timeout
      DosReleaseMutexSem(hmtxUseSNAP);
    } else
    {
      ULONG ulPostCount;
#ifdef DEBUG_BUILD
      printf("[SNAP_Uninitialize] : Timeout waiting for hmtxUseSNAP semaphore!\n");
      printf("                      Trying without semaphore!\n");
#endif
      iSNAPCommand = SNAPTHREADCOMMAND_SHUTDOWN;
      DosResetEventSem(hevSNAPResultReady, &ulPostCount);
      DosPostEventSem(hevSNAPCommandAvailable);
      DosWaitEventSem(hevSNAPResultReady, 1500); // 1.5 sec timeout
    }
  }
}

static int SNAP_Initialize()
{
#ifdef DEBUG_BUILD
  printf("[SNAP_Initialize] : Initializing...\n");
#endif

  iInPM = 1;

  iSNAPThreadStatus = SNAPTHREADSTATUS_UNKNOWN;
  tidSNAPThread = _beginthread(SNAP_ThreadFunc, NULL, 16384, NULL);
  if (tidSNAPThread<=0)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_Initialize] : Could not start SNAP-Thread!\n");
#endif
    return 0;
  }
  // Wait for SNAP-Thread to initialize itself!
  while (iSNAPThreadStatus==SNAPTHREADSTATUS_UNKNOWN) DosSleep(32);

  // Check if SNAP-Thread initialized OK!
  if (iSNAPThreadStatus!=SNAPTHREADSTATUS_RUNNING_OK)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_Initialize] : SNAP-Thread could not initialize itself!\n");
#endif
    return 0;
  }

  // All done OK!
  return 1;
}


static int SNAP_SameVideoMode(GA_modeInfo *pVideoMode1, GA_modeInfo *pVideoMode2)
{
  int iResult = 0;

  if ((pVideoMode1->PhysicalXResolution == pVideoMode2->PhysicalXResolution) &&
      (pVideoMode1->PhysicalYResolution == pVideoMode2->PhysicalYResolution) &&
      (pVideoMode1->BitsPerPixel == pVideoMode2->BitsPerPixel) &&
      (pVideoMode1->PixelFormat.RedMask == pVideoMode2->PixelFormat.RedMask) &&
      (pVideoMode1->PixelFormat.RedPosition == pVideoMode2->PixelFormat.RedPosition) &&
      (pVideoMode1->PixelFormat.RedAdjust == pVideoMode2->PixelFormat.RedAdjust) &&
      (pVideoMode1->PixelFormat.GreenMask == pVideoMode2->PixelFormat.GreenMask) &&
      (pVideoMode1->PixelFormat.GreenPosition == pVideoMode2->PixelFormat.GreenPosition) &&
      (pVideoMode1->PixelFormat.GreenAdjust == pVideoMode2->PixelFormat.GreenAdjust) &&
      (pVideoMode1->PixelFormat.BlueMask == pVideoMode2->PixelFormat.BlueMask) &&
      (pVideoMode1->PixelFormat.BluePosition == pVideoMode2->PixelFormat.BluePosition) &&
      (pVideoMode1->PixelFormat.BlueAdjust == pVideoMode2->PixelFormat.BlueAdjust) &&
      (pVideoMode1->PixelFormat.AlphaMask == pVideoMode2->PixelFormat.AlphaMask) &&
      (pVideoMode1->PixelFormat.AlphaPosition == pVideoMode2->PixelFormat.AlphaPosition) &&
      (pVideoMode1->PixelFormat.AlphaAdjust == pVideoMode2->PixelFormat.AlphaAdjust)
     ) iResult = 1;

  return iResult;
}

static void SNAP_StopPresentationManager()
{
  HAB hab;
  HWND hwnd;
  HDC  hdc;

  hab = WinInitialize(0);
  if (!hab)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_StopPresentationManager] : Could not create HAB!\n");
#endif
    return;
  }

  hwnd = WinQueryDesktopWindow(hab, 0);
#ifdef DEBUG_BUILD
  if (hwnd==NULL)
    printf("[SNAP_StopPresentationManager] : Error at WinQueryDesktopWindow()!\n");
#endif
  hdc = WinQueryWindowDC(hwnd);                    // Get HDC of desktop
  if (hdc==NULLHANDLE)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_StopPresentationManager] : Error at WinQueryWindowDC(), trying other...!\n");
#endif
    hdc = WinOpenWindowDC(hwnd);                    // Get HDC of desktop
#ifdef DEBUG_BUILD
    if (hdc==NULLHANDLE)
      printf("[SNAP_StopPresentationManager] : Error at WinOpenWindowDC()!\n");
#endif
  }

  WinLockWindowUpdate(HWND_DESKTOP, HWND_DESKTOP); // Don't let other applications write to screen anymore!
  if (!GreDeath(hdc))                              // Tell the GRE that PM will die
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_StopPresentationManager] : Error at GreDeath()!\n");
#endif
  }
}

static void SNAP_RestartPresentationManager()
{
  HWND hwnd;
  HDC  hdc;

  hwnd = WinQueryDesktopWindow(hab, 0);
#ifdef DEBUG_BUILD
  if (hwnd==NULLHANDLE)
    printf("[SNAP_RestartPresentationManager] : Error at WinQueryDesktopWindow()!\n");
#endif
  hdc = WinQueryWindowDC(hwnd);                    // Get HDC of desktop
  if (hdc==NULLHANDLE)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_RestartPresentationManager] : Error at WinQueryWindowDC(), trying other...!\n");
#endif
    hdc = WinOpenWindowDC(hwnd);                    // Get HDC of desktop
#ifdef DEBUG_BUILD
    if (hdc==NULLHANDLE)
      printf("[SNAP_RestartPresentationManager] : Error at WinOpenWindowDC()!\n");
#endif
  }
  GreResurrection(hdc, 0, NULL);           // Tell GRE that PM can come back
  WinLockWindowUpdate(HWND_DESKTOP, 0);    // Let others write to the screen again...
#ifdef DEBUG_BUILD
  printf("[SNAP_RestartPresentationManager] : Successfully switched back to PM\n");
#endif
}


static void SNAP_SetVideoMode_core(GA_modeInfo *pVideoMode)
{
  int iModeID;

#ifdef DEBUG_BUILD
  printf("[SNAP_SetVideoMode_core] : Changing video mode to [%4dx%4d/%d bpp]\n",
         pVideoMode->PhysicalXResolution, pVideoMode->PhysicalYResolution, pVideoMode->BitsPerPixel);
#endif

  // Ok, now set video mode using SetVideoMode API.
#ifdef DEBUG_BUILD
  printf("[SNAP_SetVideoMode_core] : Searching for video mode ID...\n");
#endif
  // That API needs the video mode ID, so we have to go through
  // the available video modes and find the mode ID!
  // One exception is the desktop mode where we already know the ID!
  if (pVideoMode==(&desktopModeInfo))
  {
    iModeID = desktopModeID;
    // Make sure we'll switch back to PM, no matter what video mode we're in!
    if (!iInPM)
      currentModeID = desktopModeID+1;
  }
  else
  {
    GA_modeInfo modeInfo;
    N_uint16   *mode;

    // We'll query available modes from SNAP
    mode = dc->AvailableModes;
    iModeID = *mode;
    while (*mode!=0xFFFF)
    {
      modeInfo.dwSize = sizeof(modeInfo);
      if (!initFuncs.GetVideoModeInfo(*mode, &modeInfo))
      {
	// Ok, got info about this mode info!
	if (SNAP_SameVideoMode(&modeInfo, pVideoMode))
	{
	  // Found it!
	  iModeID = *mode;
	  break;
	}
      }
      mode++;
    }
  }
  // Ok, we got the modeID for new video mode!
#ifdef DEBUG_BUILD
  printf("[SNAP_SetVideoMode_core] : Video mode ID: 0x%x\n", iModeID);
#endif
  // Change video mode if it's really a new mode!
  if (iModeID!=currentModeID)
  {
    int virtualx, virtualy, bytesperline;
    int maxmem;

    SNAP_StopVideoModeUsage();

    virtualx = pVideoMode->XResolution;
    virtualy = pVideoMode->YResolution;
    bytesperline = pVideoMode->BytesPerScanLine;
    virtualx = virtualy = bytesperline = -1;

    iModeID = iModeID | gaLinearBuffer;

    if ((iInPM) && (pVideoMode!=(&desktopModeInfo)))
    {
      // Switching away from PM
      SNAP_StopPresentationManager();
    }
#ifdef DEBUG_BUILD
    printf("[SNAP_SetVideoMode_core] : SetVideoMode() call...\n");
#endif
    if (!initFuncs.SetVideoMode(iModeID,
				&virtualx, &virtualy,
				&bytesperline,
				&maxmem,
				0, NULL))
    {
      // If could change video mode then cool!
#ifdef DEBUG_BUILD
      printf("[SNAP_SetVideoMode_core] : Changed!\n");
#endif
      memcpy(&currentModeInfo, pVideoMode, sizeof(currentModeInfo));
      currentModeID = iModeID;

      if ((iInPM) && (pVideoMode!=(&desktopModeInfo)))
      {
	// We've switched away from PM
        iInPM = 0;
#ifdef DEBUG_BUILD
        printf("[SNAP_SetVideoMode_core] : Successfully switched away from PM\n");
#endif
      } else
      if ((!iInPM) && (pVideoMode==(&desktopModeInfo)))
      {
	// Switched back to PM mode, so restore video state!
        iInPM = 1;
        SNAP_RestartPresentationManager();
        WinInvalidateRegion(HWND_DESKTOP, NULLHANDLE, TRUE); // Redraw the whole desktop
#ifdef DEBUG_BUILD
        printf("[SNAP_SetVideoMode_core] : Successfully switched back to PM\n");
#endif
      }
    }
#ifdef DEBUG_BUILD
    else
    {
      printf("[SNAP_SetVideoMode_core] : Error at SetVideoMode() call\n");
    }
#endif

    SNAP_StartVideoModeUsage();
  }
}

// Forwarder function, to call the SNAP_ThreadFunc to do something!
static void SNAP_SetVideoMode(GA_modeInfo *pVideoMode)
{
  if (DosRequestMutexSem(hmtxUseSNAP, FSLIB_SETVIDEOMODE_TIMEOUT)==NO_ERROR)
  {
    ULONG ulPostCount;
#ifdef DEBUG_BUILD
    printf("[SNAP_SetVideoMode] : Asking SNAP-Thread to change video mode!\n");
#endif
    iSNAPCommand = SNAPTHREADCOMMAND_SETVIDEOMODE;
    pSNAPCommandParameter = pVideoMode;
    DosResetEventSem(hevSNAPResultReady, &ulPostCount);
    DosPostEventSem(hevSNAPCommandAvailable);
    DosWaitEventSem(hevSNAPResultReady, SEM_INDEFINITE_WAIT);
    DosReleaseMutexSem(hmtxUseSNAP);
  }
#ifdef DEBUG_BUILD
  else
    printf("[SNAP_SetVideoMode] : Timeout waiting for semaphore!\n");
#endif
}

static void SNAP_ThreadUninitialize(int iEmergency)
{
  if (iSNAPThreadStatus == SNAPTHREADSTATUS_RUNNING_OK)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] : Uninitializing...\n");
#endif
    SNAP_StopVideoModeUsage();
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] :   Closing semaphores...\n");
#endif
    if (hevSNAPResultReady)
    {
      DosCloseEventSem(hevSNAPResultReady); hevSNAPResultReady = NULLHANDLE;
    }
    if (hevSNAPCommandAvailable)
    {
      DosCloseEventSem(hevSNAPCommandAvailable); hevSNAPCommandAvailable = NULLHANDLE;
    }
    if (hmtxUseSNAP)
    {
      DosCloseMutexSem(hmtxUseSNAP); hmtxUseSNAP = NULLHANDLE;
    }
    if (dc)
    {
#ifdef DEBUG_BUILD
      printf("[SNAP_ThreadFunc] :   GA_unloadDriver()...\n");
#endif
      GA_unloadDriver(dc); dc = NULLHANDLE;
    }
    if ((hmq) && (!iEmergency))
    {
      // Don't touch Win* stuffs in exception handlers!
#ifdef DEBUG_BUILD
      printf("[SNAP_ThreadFunc] :   WinDestroyMsgQueue()...\n");
#endif
      WinDestroyMsgQueue(hmq); hmq = NULLHANDLE;
    }
    if ((hab) && (!iEmergency))
    {
      // Don't touch Win* stuffs in exception handlers!
#ifdef DEBUG_BUILD
      printf("[SNAP_ThreadFunc] :   WinTerminate()...\n");
#endif
      WinTerminate(hab); hab = NULLHANDLE;
    }
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] : Uninitialized ok.\n");
#endif

    iSNAPThreadStatus = SNAPTHREADSTATUS_STOPPED_OK;
  }
}

ULONG _System SNAP_ExceptionHandler(PEXCEPTIONREPORTRECORD pERepRec,
                                    PEXCEPTIONREGISTRATIONRECORD pERegRec,
                                    PCONTEXTRECORD pCtxRec,
                                    PVOID p)
{
#ifdef DEBUG_BUILD
  APIRET  arc = NO_ERROR;
  HMODULE hmod1 = NULLHANDLE;
  CHAR    szMod1[2*CCHMAXPATH] = "unknown";
  ULONG   ulObject = 0,
          ulOffset = 0;
#endif

  if (pERepRec->fHandlerFlags & EH_EXIT_UNWIND)
    return XCPT_CONTINUE_SEARCH;
  if (pERepRec->fHandlerFlags & EH_UNWINDING)
    return XCPT_CONTINUE_SEARCH;
  if (pERepRec->fHandlerFlags & EH_NESTED_CALL)
    return XCPT_CONTINUE_SEARCH;

#ifdef DEBUG_BUILD

  printf("[SNAP_ExceptionHandler] : Entered\n");
  printf("  Exception info:\n");
  printf("    PEXCEPTIONREPORTRECORD:\n");
  printf("      ExceptionNum  : 0x%x\n", pERepRec->ExceptionNum);
  printf("        Severity code : 0x%02x\n", pERepRec->ExceptionNum & XCPT_SEVERITY_CODE);
  printf("        Customer code : 0x%08x\n", pERepRec->ExceptionNum & XCPT_CUSTOMER_CODE);
  printf("        Exception code: 0x%08x\n", pERepRec->ExceptionNum & XCPT_EXCEPTION_CODE);
  printf("      fHandlerFlags : %d\n", pERepRec->fHandlerFlags);
  printf("      ExceptionAddr : %p\n", pERepRec->ExceptionAddress);
  printf("    PCONTEXTRECORD:\n");
  printf("      GS=0x%04x FS=0x%04x ES=0x%04x DS=0x%04x\n", pCtxRec->ctx_SegGs, pCtxRec->ctx_SegFs, pCtxRec->ctx_SegEs, pCtxRec->ctx_SegDs);
  printf("      EDI=0x%08x ESI=0x%08x EBP=0x%08x\n", pCtxRec->ctx_RegEdi, pCtxRec->ctx_RegEsi, pCtxRec->ctx_RegEbp);
  printf("      CS:EIP=0x%04x:0x%08x\n", pCtxRec->ctx_SegCs, pCtxRec->ctx_RegEip);

  printf("Exception is ");
  switch (pERepRec->ExceptionNum)
  {
    case XCPT_GUARD_PAGE_VIOLATION: printf("XCPT_GUARD_PAGE_VIOLATION\n"); break;
    case XCPT_UNABLE_TO_GROW_STACK: printf("XCPT_UNABLE_TO_GROW_STACK\n"); break;
    case XCPT_DATATYPE_MISALIGNMENT: printf("XCPT_DATATYPE_MISALIGNMENT\n"); break;
    case XCPT_BREAKPOINT: printf("XCPT_BREAKPOINT\n"); break;
    case XCPT_SINGLE_STEP: printf("XCPT_SINGLE_STEP\n"); break;
    case XCPT_ACCESS_VIOLATION:
        printf("XCPT_ACCESS_VIOLATION\n");
        printf("  Access violated by");
        if (pERepRec->ExceptionInfo[0] & XCPT_READ_ACCESS) printf(" READ_ACCESS");
        if (pERepRec->ExceptionInfo[0] & XCPT_WRITE_ACCESS) printf(" WRITE_ACCESS");
        if (pERepRec->ExceptionInfo[0] & XCPT_SPACE_ACCESS) printf(" SPACE_ACCESS");
        if (pERepRec->ExceptionInfo[0] & XCPT_LIMIT_ACCESS) printf(" LIMIT_ACCESS");
        printf("\n");
        break;
    case XCPT_ILLEGAL_INSTRUCTION: printf("ILLEGAL_INSTRUCTION\n"); break;
    case XCPT_PROCESS_TERMINATE:
        printf("XCPT_PROCESS_TERMINATE\n");
        printf("   Terminator thread is TID%d\n", pERepRec->ExceptionInfo[0]);
        break;
    case XCPT_ASYNC_PROCESS_TERMINATE:
        printf("XCPT_ASYNC_PROCESS_TERMINATE\n");
        printf("   Terminator thread is TID%d\n", pERepRec->ExceptionInfo[0]);
        break;
    case XCPT_SIGNAL:
        printf("XCPT_SIGNAL\n");
        printf("   Signal number is %d\n", pERepRec->ExceptionInfo[0]);
        break;
    default: printf("Other exception\n");
  }

  printf("Module info: ");

  printf(" CS:EIP : %08lX ", pERepRec->ExceptionAddress);
  arc = DosQueryModFromEIP(&hmod1,
                           &ulObject,
                           sizeof(szMod1), szMod1,
                           &ulOffset,
                           (ULONG) (pERepRec->ExceptionAddress));

  if (arc != NO_ERROR)
  {
      // error:
      printf(" %-8s Error: DosQueryModFromEIP returned %lu\n",
             szMod1,
             arc);
  }
  else
  {
      CHAR szFullName[2*CCHMAXPATH];

      printf(" %-8s %02lX:%08lX\n",
             szMod1,
             ulObject + 1,       // V0.9.12 (2001-05-12) [umoeller]
             ulOffset);          // V0.9.12 (2001-05-12) [umoeller]

      DosQueryModuleName(hmod1, sizeof(szFullName), szFullName);
      printf("Module name: [%s]\n", szFullName);
  }
#endif

  // Do cleanup at every fatal exception!
  if (((pERepRec->ExceptionNum & XCPT_SEVERITY_CODE) == XCPT_FATAL_EXCEPTION) &&
      (pERepRec->ExceptionNum != XCPT_BREAKPOINT) &&
      (pERepRec->ExceptionNum != XCPT_SINGLE_STEP)
     )
  {
    if (!iInPM)
    {
#ifdef DEBUG_BUILD
      printf("[SNAP_ExceptionHandler] : Set desktop video mode\n");
#endif
      // Set original desktop video mode
      SNAP_SetVideoMode_core(&desktopModeInfo);
    }
    SNAP_ThreadUninitialize(1); // 1 = Emergency mode!
  }
#ifdef DEBUG_BUILD
  printf("[SNAP_ExceptionHandler] : Leaving\n");
#endif
  return (XCPT_CONTINUE_SEARCH);
}

EXCEPTIONREGISTRATIONRECORD SNAP_xcpthand = {0, SNAP_ExceptionHandler};

static void SNAP_ThreadFunc(void *pParm)
{
  int rc;
  ULONG ulPostCount;

#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Starting...\n");
#endif
  // Set priority of SNAP-Thread to a bit better, so it will process
  // requests fast!
#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Calling DosSetPriority!\n");
#endif
  DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, 0);
#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Calling WinInitialize!\n");
#endif
  hab = WinInitialize(0);
  if (!hab)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] : Could not create HAB!\n");
#endif
    iSNAPThreadStatus = SNAPTHREADSTATUS_STOPPED_ERROR;
    _endthread();
    return;
  }
#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Calling WinCreateMsgQueue!\n");
#endif
  hmq = WinCreateMsgQueue(hab, 0);
  if (!hmq)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] : Could not create HMQ!\n");
    printf("                    Make sure that the application is a PM app!\n");
#endif
    WinTerminate(hab);
    iSNAPThreadStatus = SNAPTHREADSTATUS_STOPPED_ERROR;
    _endthread();
    return;
  }

#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Calling GA_loadDriver!\n");
#endif
  // First load the driver for primary display (0)
  if ((dc=GA_loadDriver(0, false))==NULL)
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] : Could not load graphics driver!\n");
#endif
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    iSNAPThreadStatus = SNAPTHREADSTATUS_STOPPED_ERROR;
    _endthread();
    return;
  }

  // Then query the init functions!
#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Query functions!\n");
#endif
  initFuncs.dwSize = sizeof(initFuncs);
  if (!GA_queryFunctions(dc, GA_GET_INITFUNCS, &initFuncs))
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] : Could not query init funcs\n");
#endif
    GA_unloadDriver(dc); dc = NULLHANDLE;
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    iSNAPThreadStatus = SNAPTHREADSTATUS_STOPPED_ERROR;
    _endthread();
    return;
  }

  // Query driver functions
  driverFuncs.dwSize = sizeof(driverFuncs);
  if (!GA_queryFunctions(dc, GA_GET_DRIVERFUNCS, &driverFuncs))
  {
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] : Could not query driver funcs\n");
#endif
    GA_unloadDriver(dc); dc = NULLHANDLE;
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    iSNAPThreadStatus = SNAPTHREADSTATUS_STOPPED_ERROR;
    _endthread();
    return;
  }
#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Get config info!\n");
#endif
  // Get configuration info
  configInfo.dwSize = sizeof(configInfo);
  initFuncs.GetConfigInfo(&configInfo);
#ifdef DEBUG_BUILD
  printf("  Using the following hardware:\n");
  printf("    Manufacturer: %s\n", configInfo.ManufacturerName);
  printf("    Chipset     : %s\n", configInfo.ChipsetName);
  printf("    DAC         : %s\n", configInfo.DACName);
  printf("    Clock       : %s\n", configInfo.ClockName);
  printf("    Driver version     : %s\n", configInfo.VersionInfo);
  printf("    Driver build date  : %s\n", configInfo.BuildDate);

  printf("[SNAP_ThreadFunc] : Get video mode!\n");
#endif
  // Save current video mode info as desktop mode info
  desktopModeID = initFuncs.GetVideoMode();
  desktopModeInfo.dwSize = sizeof(desktopModeInfo);
  initFuncs.GetCurrentVideoModeInfo(&desktopModeInfo);
#ifdef DEBUG_BUILD
  printf("  Current (desktop) video mode: 0x%x\n", desktopModeID);
  printf("    Attributes:   %x\n", desktopModeInfo.Attributes);
  printf("    Resolution:   %d x %d\n", desktopModeInfo.XResolution, desktopModeInfo.YResolution);
  printf("    BytesPerScanLine: %d\n", desktopModeInfo.BytesPerScanLine);
  printf("    BitsPerPixel: %d\n", desktopModeInfo.BitsPerPixel);
  printf("    DefRefrRate : %d\n", desktopModeInfo.DefaultRefreshRate);
  printf("    BitBltCaps  : %p\n", desktopModeInfo.BitBltCaps);
  printf("    VideoWindows: %p\n", desktopModeInfo.VideoWindows);
  printf("    LinearSize  : %d\n", desktopModeInfo.LinearSize);
#endif

  memcpy(&currentModeInfo, &desktopModeInfo, sizeof(currentModeInfo));
  currentModeID = desktopModeID;
  iInPM = 1; // We're in PM desktop yet

#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Create semaphores!\n");
#endif
  // Create semaphores
  if ((rc = DosCreateMutexSem(NULL, &hmtxUseSNAP, 0, FALSE))!=NO_ERROR)
  {
    GA_unloadDriver(dc); dc = NULLHANDLE;
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] : Could not create mutex semaphore! rc = %d\n", rc);
#endif
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    iSNAPThreadStatus = SNAPTHREADSTATUS_STOPPED_ERROR;
    _endthread();
    return;
  }
  if ((rc = DosCreateEventSem(NULL, &hevSNAPCommandAvailable, 0, FALSE))!=NO_ERROR)
  {
    DosCloseMutexSem(hmtxUseSNAP); hmtxUseSNAP = NULLHANDLE;
    GA_unloadDriver(dc); dc = NULLHANDLE;
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] : Could not create event semaphore! rc = %d\n", rc);
#endif
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    iSNAPThreadStatus = SNAPTHREADSTATUS_STOPPED_ERROR;
    _endthread();
    return;
  }
  if ((rc = DosCreateEventSem(NULL, &hevSNAPResultReady, 0, FALSE))!=NO_ERROR)
  {
    DosCloseEventSem(hevSNAPCommandAvailable); hevSNAPCommandAvailable = NULLHANDLE;
    DosCloseMutexSem(hmtxUseSNAP); hmtxUseSNAP = NULLHANDLE;
    GA_unloadDriver(dc); dc = NULLHANDLE;
#ifdef DEBUG_BUILD
    printf("[SNAP_ThreadFunc] : Could not create event semaphore! rc = %d\n", rc);
#endif
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    iSNAPThreadStatus = SNAPTHREADSTATUS_STOPPED_ERROR;
    _endthread();
    return;
  }

  // Set up 2D rasterizer for current video mode
#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Set up rasterizer!\n");
#endif
  if (!SNAP_StartVideoModeUsage())
  {
    DosCloseEventSem(hevSNAPResultReady); hevSNAPResultReady = NULLHANDLE;
    DosCloseEventSem(hevSNAPCommandAvailable); hevSNAPCommandAvailable = NULLHANDLE;
    DosCloseMutexSem(hmtxUseSNAP); hmtxUseSNAP = NULLHANDLE;
    GA_unloadDriver(dc); dc = NULLHANDLE;
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    iSNAPThreadStatus = SNAPTHREADSTATUS_STOPPED_ERROR;
    _endthread();
    return;
  }

  // Cool, everything is ready for the show.
  // Now install the exception handler to be able to do cleanup in case
  // of emergency!
  DosSetExceptionHandler(&SNAP_xcpthand);

  iSNAPThreadStatus = SNAPTHREADSTATUS_RUNNING_OK;
  // Main message processing loop
#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Entering main message loop...\n");
#endif
  do {
    DosResetEventSem(hevSNAPCommandAvailable, &ulPostCount);
    rc = DosWaitEventSem(hevSNAPCommandAvailable, SEM_INDEFINITE_WAIT);
    if (rc==NO_ERROR)
    {
#ifdef DEBUG_BUILD
      printf("[SNAP_ThreadFunc] : Got SNAPCommand: %d\n", iSNAPCommand);
#endif

      switch (iSNAPCommand)
      {
        case SNAPTHREADCOMMAND_SHUTDOWN:
          rc = 1; // Will break the main message processing loop
          break;
        case SNAPTHREADCOMMAND_SETVIDEOMODE:
          SNAP_SetVideoMode_core((GA_modeInfo *) pSNAPCommandParameter);
          break;
        default:
          break;
      }

#ifdef DEBUG_BUILD
      printf("[SNAP_ThreadFunc] : Posting hevSNAPResultReady\n");
#endif

      DosPostEventSem(hevSNAPResultReady);
    }
  } while (rc==NO_ERROR);

#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Remove exception handler\n");
#endif

  // Remove exception handler
  DosUnsetExceptionHandler(&SNAP_xcpthand);

#ifdef DEBUG_BUILD
  printf("[SNAP_ThreadFunc] : Ending SNAP Thread...\n");
#endif

  // Uninitiale everything we've created!
  SNAP_ThreadUninitialize(0); // 0 = Normal (non-emergency) mode!
  _endthread();
}

static void FSLib_GAmodeInfo2FSLibVideoMode(GA_modeInfo *pModeInfo,
					    FSLib_VideoMode_p pVideoMode)
{
  pVideoMode->uiXResolution = pModeInfo->XResolution;
  pVideoMode->uiYResolution = pModeInfo->YResolution;
  pVideoMode->uiScanLineSize = pModeInfo->BytesPerScanLine;
  pVideoMode->uiBPP = pModeInfo->BitsPerPixel;
  memcpy(&(pVideoMode->PixelFormat), &(pModeInfo->PixelFormat), sizeof(FSLib_PixelFormat));
}

DECLSPEC int               FSLIBCALL FSLib_Initialize()
{
  int rc;

  // Don't do anything if we're already initialized!
  if (bFSLib_Initialized) return 1;

  bFSLib_Initialized = 0;

  // The FSLib needs SNAP, so try to initialize SNAP first!
  if (!SNAP_Initialize())
  {
#ifdef DEBUG_BUILD
    printf("[FSLib_Initialize] : Could not initialize SNAP!\n");
#endif
    return 0;
  }

  // Initialize the list of HWNDs
  pHWNDListHead = NULL;
  if ((rc = DosCreateMutexSem(NULL, &hmtxUseHWNDList, 0, FALSE))!=NO_ERROR)
  {
    SNAP_Uninitialize();
#ifdef DEBUG_BUILD
    printf("[FSLib_Initialize] : Could not create mutex semaphore! rc = %d\n", rc);
#endif
    return 0;
  }

  // Register new class
  if (!FSLib_RegisterWindowClass())
  {
    DosCloseMutexSem(hmtxUseHWNDList); hmtxUseHWNDList = NULLHANDLE;
    SNAP_Uninitialize();
#ifdef DEBUG_BUILD
    printf("[FSLib_Initialize] : Could not register new window class!\n");
#endif
    return 0;
  }

  // Make desktop video mode info FSLib compatible!
  FSLib_GAmodeInfo2FSLibVideoMode(&desktopModeInfo,
                                  &desktopVideoModeInfo);
  bFSLib_Initialized = 1;
  return 1;
}

DECLSPEC void              FSLIBCALL FSLib_Uninitialize()
{
  if (bFSLib_Initialized)
  {
    // Free HWND list
    if (pHWNDListHead)
    {
      FSLib_HWNDList_p pHWND, pNextHWND;
#ifdef DEBUG_BUILD
      printf("[FSLib_Uninitialize] : There are undestroyed windows!\n");
#endif
      pHWND = pHWNDListHead;
      while (pHWND)
      {
        pNextHWND = pHWND->pNext;
#ifdef DEBUG_BUILD
        printf("[FSLib_Uninitialize] : Calling WinDestroyWindow() for %p\n", pHWND->hwndClient);
#endif
        WinDestroyWindow(pHWND->hwndClient);
        pHWND = pNextHWND;
      }
    }
    // Destroy semaphore
#ifdef DEBUG_BUILD
    printf("[FSLib_Uninitialize] : Destroying semaphore\n");
#endif
    DosCloseMutexSem(hmtxUseHWNDList); hmtxUseHWNDList = NULLHANDLE;
    // Uninitialize SNAP
#ifdef DEBUG_BUILD
    printf("[FSLib_Uninitialize] : Uninitializing SNAP\n");
#endif
    SNAP_Uninitialize();
    bFSLib_Initialized = 0;
#ifdef DEBUG_BUILD
    printf("[FSLib_Uninitialize] : Done!\n");
#endif
  }
}

static void FSLib_EmergencyUninitialize()
{
  if (bFSLib_Initialized)
  {
    // Free HWND list
    if (pHWNDListHead)
    {
      FSLib_HWNDList_p pHWND, pNextHWND;
#ifdef DEBUG_BUILD
      printf("[FSLib_EmergencyUninitialize] : There are undestroyed windows!\n");
#endif
      pHWND = pHWNDListHead;
      while (pHWND)
      {
        pNextHWND = pHWND->pNext;
#ifdef DEBUG_BUILD
        printf("[FSLib_EmergencyUninitialize] : Calling WinDestroyWindow() for %p\n", pHWND->hwndClient);
#endif
        WinDestroyWindow(pHWND->hwndClient);
        pHWND = pNextHWND;
      }
    }
    // Destroy semaphore
#ifdef DEBUG_BUILD
    printf("[FSLib_EmergencyUninitialize] : Destroying mutex semaphore\n");
#endif
    DosCloseMutexSem(hmtxUseHWNDList); hmtxUseHWNDList = NULLHANDLE;

    // We don't have to uninitialize SNAP here, it will be done by
    // SNAPThread's Exception handler
    bFSLib_Initialized = 0;
#ifdef DEBUG_BUILD
    printf("[FSLib_EmergencyUninitialize] : Done!\n");
#endif
  }
}

#define FSLIB_WINDOWCLASS_NAME  "FSLibWindowClass"

// Window storage for FSLibWindowClass windows:

// pUserDate
//   Pos  0   Size  4
#define FSLIB_WINDOWDATA_PVOID_USERDATA                      0
// hmtxUseWindowData
//   Pos  4   Size  4
#define FSLIB_WINDOWDATA_HMTX_USEWINDOWDATA                  4
// pfnwpUserWindowFn
//   Pos  8   Size  4
#define FSLIB_WINDOWDATA_PFNWP_USERWINDOWFN                  8
// pFSModeInfo
//   Pos  12  Size  4
#define FSLIB_WINDOWDATA_PGAMODEINFO_FSMODEINFO             12
// iVRNDisabled
//   Pos  16  Size  4
#define FSLIB_WINDOWDATA_INT_VRNDISABLED                    16
// iRunInFSMode
//   Pos  20  Size  4
#define FSLIB_WINDOWDATA_INT_RUNINFSMODE                    20
// prectlVisibleArea
//   Pos  24  Size  4
#define FSLIB_WINDOWDATA_PRECTL_VISIBLEAREA                 24
// iNumOfVisibleAreaRectangles
//   Pos  28  Size  4
#define FSLIB_WINDOWDATA_INT_NUMOFVISIBLEAREARECTANGLES     28
// iMaxNumOfVisibleAreaRectangles
//   Pos  32  Size  4
#define FSLIB_WINDOWDATA_INT_MAXNUMOFVISIBLEAREARECTANGLES  32
// FSLib_SrcBufferDesc
//   Pos  36  Size  4
#define FSLIB_WINDOWDATA_PVOID_SRCBUFFERDESC                36
// iWindowActive
//   Pos  40  Size  4
#define FSLIB_WINDOWDATA_INT_WINDOWACTIVE                   40
// pswpFrameSWP
//   Pos  44  Size  4
#define FSLIB_WINDOWDATA_PSWP_FRAMESWP                      44
// iDirtyWindow
//   Pos  48  Size  4
#define FSLIB_WINDOWDATA_INT_DIRTYWINDOW                    48

#define FSLIB_WINDOWDATA_SIZE                               52

// Special window messages to FSLib client window:
#define WM_FSLIBCOMMAND          WM_USER+1

// FSLib commands:
// ---------------
//
// (Pass in mp1, pass parameters in mp2)
// FSLC_INITIALIZED
//  Sent to client window when the remaining fields of the
//  window data have been filled, so the window can emulate
//  a WM_CREATE message to the user's window processing function
#define FSLC_INITIALIZED         0
// FSLC_TOGGLEFSMODE
//  Toggle fullscreen mode for window
#define FSLC_TOGGLEFSMODE        1
// FSLC_SETSRCBUFFERDESC
//  Changes source buffer parameters on the fly
//  It does not change window size in windowed mode,
//  but can change fullscreen video mode in fullscreen mode
//  if there is a better fsmode fit to new buffer!
#define FSLC_SETSRCBUFFERDESC    2

static MRESULT EXPENTRY FSLib_WndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  HMTX hmtxUseWindowData;
  PFNWP pfnwpUserWindowProc;

#ifdef DEBUG_BUILD
  unsigned long long llTime;
  static unsigned long long llPrevTime = 0;

  llTime = GetTimeElapsed();
  if (llPrevTime==0) llPrevTime = llTime;
  printf("[FSLib_WndProc] : Time: %Lu Delta: %Lu  Message (hwnd = %p) (msg = 0x%x)",
         llTime*1000/ulTimerFreq, (llTime-llPrevTime) * 1000 / ulTimerFreq,
         hwnd, msg);
  switch (msg)
  {
    case WM_PAINT:
        printf(" [WM_PAINT]");
        break;
    case WM_VRNDISABLED:
        printf(" [WM_VRNDISABLED]");
        break;
    case WM_VRNENABLED:
        printf(" [WM_VRNENABLED]");
        break;
    case WM_ERASEBACKGROUND:
        printf(" [WM_ERASEBACKGROUND]");
        break;
    default:
        break;
  }
  printf("\n");
  fflush(stdout);
  llPrevTime = llTime;
#endif


  switch (msg)
  {
    case WM_DESTROY:
      {
	MRESULT rc;
	void *pData;
	int iRunInFSMode;
        int iWindowActive;
#ifdef DEBUG_BUILD
        printf("[FSLib_WndProc] : WM_DESTROY (hwnd = %p)\n", hwnd);
        fflush(stdout);
#endif
	// Switch back to desktop mode if needed
	iRunInFSMode  = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_RUNINFSMODE);
	iWindowActive = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_WINDOWACTIVE);
	if ((iRunInFSMode) && (iWindowActive))
        {
#ifdef DEBUG_BUILD
	  printf("[FSLib_WndProc] : Switching back to desktop mode!\n");
	  fflush(stdout);
#endif
          WinSendMsg(hwnd, WM_FSLIBCOMMAND, (MPARAM) FSLC_TOGGLEFSMODE, (MPARAM) 0);
	}

        // Free allocated resources
	hmtxUseWindowData = (HMTX) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_HMTX_USEWINDOWDATA);
	DosRequestMutexSem(hmtxUseWindowData, SEM_INDEFINITE_WAIT);
	DosCloseMutexSem(hmtxUseWindowData);
	pData = (void *) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PGAMODEINFO_FSMODEINFO);
	if (pData) free(pData);
        pData = (void *) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PRECTL_VISIBLEAREA);
	if (pData) free(pData);
        pData = (void *) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PVOID_SRCBUFFERDESC);
	if (pData) free(pData);
        pData = (void *) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PSWP_FRAMESWP);
	if (pData) free(pData);

	// Remove ourselves from the list of FSLib windows!
	if (DosRequestMutexSem(hmtxUseHWNDList, SEM_INDEFINITE_WAIT)==NO_ERROR)
	{
	  FSLib_HWNDList_p pHWNDList, pPrev;
          pPrev = NULL;
	  pHWNDList = pHWNDListHead;
	  while ((pHWNDList) && (pHWNDList->hwndClient!=hwnd))
	  {
	    pPrev = pHWNDList;
	    pHWNDList = pHWNDList->pNext;
	  }
	  if (pHWNDList)
	  {
	    if (pPrev)
	      pPrev->pNext = pHWNDList->pNext;
	    else
	      pHWNDListHead = pHWNDList->pNext;
	    free(pHWNDList);
	  }
          DosReleaseMutexSem(hmtxUseHWNDList);
	}

	// Call user window proc if exists
	pfnwpUserWindowProc = (PFNWP) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PFNWP_USERWINDOWFN);
	if (pfnwpUserWindowProc)
	  rc = (*pfnwpUserWindowProc)(hwnd, msg, mp1, mp2);
	else
	  rc = WinDefWindowProc(hwnd, msg, mp1, mp2);

	// Cool, all done!
        return rc;
      }
    case WM_CREATE:
      {
	GA_modeInfo *pFSModeInfo;
        FSLib_VideoMode_p pSrcBufferDesc;
	PSWP pswpFrameSWP;
        FSLib_HWNDList_p pHWNDList;
#ifdef DEBUG_BUILD
	printf("[FSLib_WndProc] : WM_CREATE (hwnd = %p)\n", hwnd);
	fflush(stdout);
#endif
	// Set up window storage initial values
	WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_PVOID_USERDATA, 0);
	DosCreateMutexSem(NULL, &hmtxUseWindowData, 0, FALSE);
	WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_HMTX_USEWINDOWDATA, (ULONG) hmtxUseWindowData);
	WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_PFNWP_USERWINDOWFN, 0);
	pFSModeInfo = (GA_modeInfo *) malloc(sizeof(GA_modeInfo));
        memcpy(pFSModeInfo, &desktopModeInfo, sizeof(GA_modeInfo));
	WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_PGAMODEINFO_FSMODEINFO, (ULONG) pFSModeInfo);
	WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_VRNDISABLED, 1); // True
	WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_RUNINFSMODE, 0); // False
	WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_PRECTL_VISIBLEAREA, 0);
	WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_NUMOFVISIBLEAREARECTANGLES, 0);
        WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_MAXNUMOFVISIBLEAREARECTANGLES, 0);
	pSrcBufferDesc = (FSLib_VideoMode_p) malloc(sizeof(FSLib_VideoMode));
        FSLib_GAmodeInfo2FSLibVideoMode(pFSModeInfo, pSrcBufferDesc);
	WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_PVOID_SRCBUFFERDESC, (ULONG) pSrcBufferDesc);
        WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_WINDOWACTIVE, 0); // False
        WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_DIRTYWINDOW, 0); // False
        pswpFrameSWP = (PSWP) malloc(sizeof(SWP));
        WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_PSWP_FRAMESWP, (ULONG) pswpFrameSWP);

	// Add ourselves to the list of FSLib windows!
	if (DosRequestMutexSem(hmtxUseHWNDList, SEM_INDEFINITE_WAIT)==NO_ERROR)
	{
	  pHWNDList = (FSLib_HWNDList_p) malloc(sizeof(FSLib_HWNDList));
	  if (pHWNDList)
	  {
	    pHWNDList->hwndClient = hwnd;
	    pHWNDList->pNext = pHWNDListHead;
	    pHWNDListHead = pHWNDList;
	  }
          DosReleaseMutexSem(hmtxUseHWNDList);
	}

        break;
      }
    case WM_SIZE:
    case WM_MINMAXFRAME:
      {
	int iRunInFSMode;
#ifdef DEBUG_BUILD
	printf("[FSLib_WndProc] : WM_SIZE or WM_MINMAXFRAME\n");
	fflush(stdout);
#endif
	iRunInFSMode  = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_RUNINFSMODE);
	// TODO: check if it's a solution or not!
	if (iRunInFSMode)
	{
          WinSetActiveWindow(HWND_DESKTOP, hwnd);
	  return 0;
	}
        break;
      }
    case WM_ACTIVATE:     // activation/deactivation of window
      {
	int iActive;
        int iRunInFSMode;
        int iWindowActive;
        GA_modeInfo *pFSModeInfo;
#ifdef DEBUG_BUILD
	printf("[FSLib_WndProc] : WM_ACTIVATE (hwnd = %p)\n", hwnd);
	fflush(stdout);
#endif
	hmtxUseWindowData = (HMTX) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_HMTX_USEWINDOWDATA);
	if (DosRequestMutexSem(hmtxUseWindowData, SEM_INDEFINITE_WAIT) == NO_ERROR)
	{
	  iActive = (int) mp1;
          iRunInFSMode = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_RUNINFSMODE);
          iWindowActive = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_WINDOWACTIVE);
          pFSModeInfo = (GA_modeInfo *) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PGAMODEINFO_FSMODEINFO);

	  if (!iActive)
	  { // Switching away from the application
            // Let's check if the app is in fullscreen mode or not!
	    if ((iRunInFSMode) && (iWindowActive))
            {
#ifdef DEBUG_BUILD
              printf("  Switching away from FS mode\n");
#endif
              // Switching away from an application running in fullscreen mode:
              //   Hide frame window
              WinShowWindow(WinQueryWindow(hwnd, QW_PARENT), FALSE);
              //   Turn off VRN
              WinSetVisibleRegionNotify(hwnd, FALSE );
              WinPostMsg(hwnd, WM_VRNDISABLED, 0, 0);
              //   Release captured mouse
              WinSetCapture(HWND_DESKTOP, 0);
              //   Change video mode
	      SNAP_SetVideoMode(&desktopModeInfo);
	      //   Redraw desktop
              WinInvalidateRegion(HWND_DESKTOP, NULLHANDLE, TRUE);
	    }
	  } else
	  { // Switching to the application
	    if (iRunInFSMode)  // Switching to the application that should run in FS mode!
            {
              RECTL rectl;
              HWND hwndFrame;
              SWP swpDesktop;
#ifdef DEBUG_BUILD
              printf("  Switching back to FS mode\n");
#endif
              // Switching to the application which should run in fullscreen mode:
              //   Change window position and size so that the client area will
              //   be at the top-left corner of the screen
              hwndFrame = WinQueryWindow(hwnd, QW_PARENT);
              rectl.xLeft = 0;
              rectl.yBottom = 0;
              rectl.xRight = pFSModeInfo->XResolution;
              rectl.yTop = pFSModeInfo->YResolution;
              WinCalcFrameRect(hwndFrame, &rectl, FALSE);
              WinQueryWindowPos(HWND_DESKTOP, &swpDesktop);
              WinSetWindowPos(hwndFrame, HWND_TOP,
                              rectl.xLeft, swpDesktop.cy-pFSModeInfo->YResolution+rectl.yBottom,
                              rectl.xRight-rectl.xLeft, rectl.yTop-rectl.yBottom,
                              SWP_SIZE | SWP_MOVE | SWP_ZORDER | SWP_SHOW);
              //   Capture mouse
              WinSetCapture(HWND_DESKTOP, hwnd);
              //   Turn on VRN
              WinSetVisibleRegionNotify(hwnd, TRUE);
              WinPostMsg(hwnd, WM_VRNENABLED, 0, 0);
              //   Change video mode
              SNAP_SetVideoMode(pFSModeInfo);
	    }
	  }
          // Store window active flag in window data
          WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_WINDOWACTIVE, (ULONG) iActive);
          DosReleaseMutexSem(hmtxUseWindowData);
	}
      }
      break;
    case WM_VRNDISABLED: // Visible region notification
      {
#ifdef DEBUG_BUILD
	printf("[FSLib_WndProc] : WM_VRNDISABLED (hwnd = %p)\n", hwnd);
	fflush(stdout);
#endif
	hmtxUseWindowData = (HMTX) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_HMTX_USEWINDOWDATA);
	if (DosRequestMutexSem(hmtxUseWindowData, SEM_INDEFINITE_WAIT) == NO_ERROR)
	{
	  // Set iVRNDisabled to TRUE!
          WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_VRNDISABLED, (ULONG) 1); // True

          DosReleaseMutexSem(hmtxUseWindowData);
	}
	break;
      }
    case WM_VRNENABLED:  // Visible region notification
      {
#ifdef DEBUG_BUILD
	printf("[FSLib_WndProc] : WM_VRNENABLED (hwnd = %p)\n", hwnd);
	fflush(stdout);
#endif
	hmtxUseWindowData = (HMTX) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_HMTX_USEWINDOWDATA);
	if (DosRequestMutexSem(hmtxUseWindowData, SEM_INDEFINITE_WAIT) == NO_ERROR)
	{
	  HPS ps;
	  HRGN rgn;
	  RGNRECT rgnctl;
	  int iMaxNumOfVisibleAreaRectangles;
	  int iNumOfVisibleAreaRectangles;
	  PRECTL prectlVisibleArea;

	  // Get data from window storage
	  iMaxNumOfVisibleAreaRectangles = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_MAXNUMOFVISIBLEAREARECTANGLES);
          iNumOfVisibleAreaRectangles = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_NUMOFVISIBLEAREARECTANGLES);
          prectlVisibleArea = (PRECTL) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PRECTL_VISIBLEAREA);

          // Start getting visible area for this window
	  ps = WinGetPS(hwnd);
	  rgn = GpiCreateRegion(ps, 0, NULL);

	  WinQueryVisibleRegion(hwnd, rgn);

	  if (!prectlVisibleArea)
	  {
            iMaxNumOfVisibleAreaRectangles = 50;
	    prectlVisibleArea = malloc(iMaxNumOfVisibleAreaRectangles*sizeof(RECTL));
          }

	  // Get the all ORed rectangles
          if (prectlVisibleArea)
	  do {
	    rgnctl.ircStart = 1;
	    rgnctl.crc = iMaxNumOfVisibleAreaRectangles;
	    rgnctl.crcReturned = 0;
	    rgnctl.ulDirection = 1;

	    GpiQueryRegionRects(ps, rgn, NULL, &rgnctl, prectlVisibleArea);

	    iNumOfVisibleAreaRectangles = rgnctl.crcReturned;

	    // If there was no space to get all the rectangles, we'll
	    // increase space and try again!
	    if (rgnctl.crcReturned >= rgnctl.crc)
	    {
	      PRECTL prectlNewVisibleArea;
	      iMaxNumOfVisibleAreaRectangles+=50;
	      prectlNewVisibleArea = (PRECTL) malloc(iMaxNumOfVisibleAreaRectangles*sizeof(RECTL));
	      if (prectlNewVisibleArea)
	      {
		free(prectlVisibleArea);
		prectlVisibleArea = prectlNewVisibleArea;
	      } else
	      {
		iMaxNumOfVisibleAreaRectangles -= 50;
		break; // break the loop!
	      }
	    }
          } while ((rgnctl.crcReturned >= rgnctl.crc) && (iMaxNumOfVisibleAreaRectangles<100000));

	  GpiDestroyRegion(ps, rgn);

	  WinReleasePS(ps);

          // Store variables back to window storage
	  WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_MAXNUMOFVISIBLEAREARECTANGLES,
			    (ULONG) iMaxNumOfVisibleAreaRectangles);
          WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_NUMOFVISIBLEAREARECTANGLES,
			    (ULONG) iNumOfVisibleAreaRectangles);
          WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_PRECTL_VISIBLEAREA,
			    (ULONG) prectlVisibleArea);
          // Set iVRNDisabled to FALSE!
          WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_VRNDISABLED, (ULONG) 0); // False

          if (WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_DIRTYWINDOW))
          {
#ifdef DEBUG_BUILD
            printf("[FSLib_WndProc] : WM_VRNENABLED : Redrawing dirty window.\n");
	    fflush(stdout);
#endif
            WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_DIRTYWINDOW, 0); // False
            WinInvalidateRect(hwnd, NULLHANDLE, TRUE);
          }

#ifdef DEBUG_BUILD
	  printf("[FSLib_WndProc] : WM_VRNENABLED : Visible rects: %d\n", iNumOfVisibleAreaRectangles);
	  fflush(stdout);
#endif
          DosReleaseMutexSem(hmtxUseWindowData);
	}
	break;
      }
    case WM_FSLIBCOMMAND:
      {
#ifdef DEBUG_BUILD
	printf("[FSLib_WndProc] : WM_FSLIBCOMMAND (hwnd = %p)\n", hwnd);
	fflush(stdout);
#endif
	switch ((int) mp1)
	{
	  case FSLC_INITIALIZED:
            {
#ifdef DEBUG_BUILD
              printf("[FSLib_WndProc] : FSLC_INITIALIZED\n");
#endif
	      pfnwpUserWindowProc = (PFNWP) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PFNWP_USERWINDOWFN);
	      if (pfnwpUserWindowProc)
		return (*pfnwpUserWindowProc)(hwnd, WM_CREATE, 0, 0);
	      else
                return (MRESULT) 0;
	    }
	  case FSLC_TOGGLEFSMODE:
            {
#ifdef DEBUG_BUILD
              printf("[FSLib_WndProc] : FSLC_TOGGLEFSMODE (%d)\n", (int) mp2);
#endif
	      hmtxUseWindowData = (HMTX) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_HMTX_USEWINDOWDATA);
	      if (DosRequestMutexSem(hmtxUseWindowData, SEM_INDEFINITE_WAIT) == NO_ERROR)
	      {
		int iWindowActive;
                int iRunInFSMode;
                GA_modeInfo *pFSModeInfo;
                PSWP pswpFrame;
                HWND hwndFrame;
                RECTL rectl;
                SWP swpDesktop;

		iRunInFSMode  = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_RUNINFSMODE);
                iWindowActive = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_WINDOWACTIVE);
                pswpFrame = (PSWP) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PSWP_FRAMESWP);
                pFSModeInfo = (GA_modeInfo *) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PGAMODEINFO_FSMODEINFO);

		if ((int) mp2)
		{
		  // Switch to FS mode
		  if (!iRunInFSMode)
		  {
		    // Do something only if not in FS mode yet!
		    if (!iWindowActive)
                    {
                      // This window is not active, and should be switched to FS mode!
                      //  - Get frame window
                      hwndFrame = WinQueryWindow(hwnd, QW_PARENT);
                      //  - Mark flag that it's running in FS mode
                      WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_RUNINFSMODE, (ULONG) 1);
                      //  - Save old frame window position and size (SWP)
                      WinQueryWindowPos(hwndFrame, pswpFrame);
                      //  - Hide/Minimize frame window
                      WinShowWindow(hwndFrame, FALSE);
		    } else
		    {
                      // This window is active, and should be switched to FS mode!
                      //  - Get frame window
                      hwndFrame = WinQueryWindow(hwnd, QW_PARENT);
                      //  - Mark flag that it's running in FS mode
                      WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_RUNINFSMODE, (ULONG) 1);
                      //  - Save old window position and size (SWP)
                      WinQueryWindowPos(hwndFrame, pswpFrame);
                      //  - Set new window position and size
                      rectl.xLeft = 0;
                      rectl.yBottom = 0;
                      rectl.xRight = pFSModeInfo->XResolution;
                      rectl.yTop = pFSModeInfo->YResolution;
                      WinCalcFrameRect(hwndFrame, &rectl, FALSE);
                      WinQueryWindowPos(HWND_DESKTOP, &swpDesktop);
                      WinSetWindowPos(hwndFrame, HWND_TOP,
                                      rectl.xLeft, swpDesktop.cy-pFSModeInfo->YResolution+rectl.yBottom,
                                      rectl.xRight-rectl.xLeft, rectl.yTop-rectl.yBottom,
                                      SWP_SIZE | SWP_MOVE | SWP_ZORDER | SWP_SHOW);
                      //  - Capture mouse
                      WinSetCapture(HWND_DESKTOP, hwnd);
                      //  - Change video mode
                      SNAP_SetVideoMode(pFSModeInfo);
		    }
		  }
		} else
		{
                  // Switch to Windowed mode
		  if (iRunInFSMode)
		  {
		    // Do something only if running in FS mode!
		    if (!iWindowActive)
		    {
		      // This window is not active, and should be switched to windowed mode!

                      //  - Get frame window
                      hwndFrame = WinQueryWindow(hwnd, QW_PARENT);
                      //  - Mark flag that it's running in windowed mode
                      WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_RUNINFSMODE, (ULONG) 0);
                      //  - restore old window position and size (SWP)
                      WinSetWindowPos(hwndFrame,
                                      pswpFrame->hwndInsertBehind,
                                      pswpFrame->x, pswpFrame->y,
                                      pswpFrame->cx, pswpFrame->cy,
                                      pswpFrame->fl);
		    } else
		    {
		      // This window is active, and should be switched to windowed mode!

                      //  - Get frame window
                      hwndFrame = WinQueryWindow(hwnd, QW_PARENT);
                      //  - Mark flag that it's running in windowed mode
                      WinSetWindowULong(hwnd, FSLIB_WINDOWDATA_INT_RUNINFSMODE, (ULONG) 0);
                      //  - Change video mode
                      SNAP_SetVideoMode(&desktopModeInfo);
                      //  - Restore old window position and size (SWP)
                      WinSetWindowPos(hwndFrame,
                                      pswpFrame->hwndInsertBehind,
                                      pswpFrame->x, pswpFrame->y,
                                      pswpFrame->cx, pswpFrame->cy,
                                      pswpFrame->fl);
                      //  - Release captured mouse
		      WinSetCapture(HWND_DESKTOP, 0);
                      //  - Redraw desktop
		      WinInvalidateRegion(HWND_DESKTOP, NULLHANDLE, TRUE);
		    }
		  }
		}
		DosReleaseMutexSem(hmtxUseWindowData);
                // notify user proc!
		pfnwpUserWindowProc = (PFNWP) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PFNWP_USERWINDOWFN);
		if (pfnwpUserWindowProc)
                  (*pfnwpUserWindowProc)(hwnd, WM_FSLIBNOTIFICATION, FSLN_TOGGLEFSMODE, mp2);
                return (MRESULT) 1;
              }
	      return (MRESULT) 0;
	    }
	  case FSLC_SETSRCBUFFERDESC:
	    {
	      HMTX hmtxUseWindowData;
	      GA_modeInfo *pmodeInfo;
	      GA_modeInfo FSModeInfo;
	      FSLib_VideoMode_p pWndSrcBufferDesc;
	      int iRunInFSMode;
              int iWindowActive;
#ifdef DEBUG_BUILD
	      printf("[FSLib_WndProc] : FSLC_SETSRCBUFFERDESC\n");
#endif
	      hmtxUseWindowData = (HMTX) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_HMTX_USEWINDOWDATA);
	      if (DosRequestMutexSem(hmtxUseWindowData, SEM_INDEFINITE_WAIT)==NO_ERROR)
	      {
		// Set new FSModeInfo, set new SrcBufferDesc, change video mode if needed.
		FSLib_VideoMode_p pSrcBufferDesc = (FSLib_VideoMode_p) mp2;

                iRunInFSMode  = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_RUNINFSMODE);
                iWindowActive = (int) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_INT_WINDOWACTIVE);

		FSLib_FindBestFSMode(pSrcBufferDesc, &FSModeInfo);

		// Set FS mode info
		pmodeInfo = (GA_modeInfo *) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PGAMODEINFO_FSMODEINFO);
		if (pmodeInfo)
		  memcpy(pmodeInfo, &FSModeInfo, sizeof(GA_modeInfo));
		// Set source buffer format
		pWndSrcBufferDesc = (FSLib_VideoMode_p) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PVOID_SRCBUFFERDESC);
		if (pWndSrcBufferDesc)
		  memcpy(pWndSrcBufferDesc, pSrcBufferDesc, sizeof(FSLib_VideoMode));

		// Ok, if we're running in fullscreen mode, then
		// also change video mode!
		if ((iRunInFSMode) && (iWindowActive))
		  SNAP_SetVideoMode(pmodeInfo);

		DosReleaseMutexSem(hmtxUseWindowData);
	      }
	      return (MRESULT) 1;
	    }
	  default:
            return 0;
	}
      }
    default:
#ifdef DEBUG_BUILD
      printf("[FSLib_WndProc] : Unhandled: %x\n", msg);
      fflush(stdout);
#endif
      break;
  }

  // Pass unhandled messages to user window proc or default window proc
  pfnwpUserWindowProc = (PFNWP) WinQueryWindowULong(hwnd, FSLIB_WINDOWDATA_PFNWP_USERWINDOWFN);
  if (pfnwpUserWindowProc)
    return (*pfnwpUserWindowProc)(hwnd, msg, mp1, mp2);
  else
    return WinDefWindowProc(hwnd, msg, mp1, mp2);
}



static int FSLib_RegisterWindowClass()
{
  HAB hab;

  hab = WinInitialize(0);
  if (!hab)
  {
#ifdef DEBUG_BUILD
    printf("[FSLib_RegisterWindowClass] : Could not create HAB!\n");
    printf("                              Make sure the process is a PM process!\n");
#endif
    return 0;
  }

  WinRegisterClass(hab, FSLIB_WINDOWCLASS_NAME, FSLib_WndProc, CS_SIZEREDRAW, FSLIB_WINDOWDATA_SIZE);

  return 1;
}

// Find the best FS mode for given source buffer format!
static void FSLib_FindBestFSMode(FSLib_VideoMode_p pSrcBufferDesc,
				 GA_modeInfo *pFSModeInfo)
{
  GA_modeInfo modeInfo;
  N_uint16   *mode;
  int iFirstModeFound;
  // We'll find the best fsmode for given source buffer format
  // If we have a one-to-one mode, use that. Otherwise, we prefer
  // to have the same resolution, then
  // to have the same bpp, then
  // to have the same pixel format

  // Check parameters
  if ((!pSrcBufferDesc) || (!pFSModeInfo)) return;

  // We'll query available modes from SNAP
  if (DosRequestMutexSem(hmtxUseSNAP, SEM_INDEFINITE_WAIT)==NO_ERROR)
  {
    // First find the best video mode we have for this buffer!
    iFirstModeFound = 0;

    mode = dc->AvailableModes;
    while (*mode!=0xFFFF)
    {
      modeInfo.dwSize = sizeof(modeInfo);
      if (initFuncs.GetVideoModeInfo(*mode, &modeInfo)==0)
      {
        // Ok, got info about this mode info!
        // If it's a graphical mode, and at least high color, then
        // we can use it. We do not support palettized modes!
        if ((!(modeInfo.Attributes & gaIsTextMode)) &&
            (modeInfo.BitsPerPixel>=15))
	{

          // If there is no mode found yet, use the first one!
	  if (!iFirstModeFound)
	  {
            memcpy(pFSModeInfo, &modeInfo, sizeof(modeInfo));
	    iFirstModeFound = 1;
	    // If we found the best possible video mode, then break the loop!
	    if ((modeInfo.XResolution == pSrcBufferDesc->uiXResolution) &&
		(modeInfo.YResolution == pSrcBufferDesc->uiYResolution))
              break;
	  } else
	  {
	    // If this mode is bigger or equal to what we need, then
            // check if it's better than what we have now!
	    if ((modeInfo.XResolution >= pSrcBufferDesc->uiXResolution) &&
		(modeInfo.YResolution >= pSrcBufferDesc->uiYResolution))
	    {
	      // If the mode we found yet is smaller than what needed,
              // or if this mode is better fit, then use this mode!
	      if (
		  (pFSModeInfo->XResolution<pSrcBufferDesc->uiXResolution) ||

		  (pFSModeInfo->YResolution<pSrcBufferDesc->uiYResolution) ||

		  (((modeInfo.XResolution - pSrcBufferDesc->uiXResolution) *
		    (modeInfo.YResolution - pSrcBufferDesc->uiYResolution)) <
                   ((pFSModeInfo->XResolution - pSrcBufferDesc->uiXResolution) *
		    (pFSModeInfo->YResolution - pSrcBufferDesc->uiYResolution)))
		 )
	      {
                memcpy(pFSModeInfo, &modeInfo, sizeof(modeInfo));
		// If we found the best possible video mode, then break the loop!
		if ((modeInfo.XResolution == pSrcBufferDesc->uiXResolution) &&
		    (modeInfo.YResolution == pSrcBufferDesc->uiYResolution))
                  break;
	      }
	    }
	  }
	}
      }
      mode++;
    }

    if (pFSModeInfo->BitsPerPixel != pSrcBufferDesc->uiBPP)
    {
      // Then find the best bpp between these resolution modes!
      mode = dc->AvailableModes;
      while (*mode!=0xFFFF)
      {
	modeInfo.dwSize = sizeof(modeInfo);
	if (initFuncs.GetVideoModeInfo(*mode, &modeInfo)==0)
	{
	  // Ok, got info about this mode info!
	  // If it's a graphical mode, and at least high color, then
	  // we can use it. We do not support palettized modes!
	  if ((!(modeInfo.Attributes & gaIsTextMode)) &&
	      (modeInfo.BitsPerPixel>=15))
	  {
	    // We take care only the video modes with the resolution we found!
	    if ((modeInfo.XResolution == pFSModeInfo->XResolution) &&
		(modeInfo.YResolution == pFSModeInfo->YResolution))
	    {
	      if (modeInfo.BitsPerPixel == pSrcBufferDesc->uiBPP)
	      {
		// Cool, found the perfect one!
                memcpy(pFSModeInfo, &modeInfo, sizeof(modeInfo));
                break;
	      } else
	      if ((pFSModeInfo->BitsPerPixel<pSrcBufferDesc->uiBPP) ||
		  ((pFSModeInfo->BitsPerPixel-pSrcBufferDesc->uiBPP) > (modeInfo.BitsPerPixel - pSrcBufferDesc->uiBPP))
		 )
                memcpy(pFSModeInfo, &modeInfo, sizeof(modeInfo));
	    }
	  }
	}
	mode++;
      }
    }
    DosReleaseMutexSem(hmtxUseSNAP);
  }
#ifdef DEBUG_BUILD
  printf("[FSLib_FindBestFSMode] : Wanted: %d x %d / %d bpp\n", pSrcBufferDesc->uiXResolution, pSrcBufferDesc->uiYResolution, pSrcBufferDesc->uiBPP);
  printf("                         Found : %d x %d / %d bpp\n", pFSModeInfo->XResolution, pFSModeInfo->YResolution, pFSModeInfo->BitsPerPixel);
#endif
}

// -- Definition of DosGetInfoSeg() API to be called from 32bits code: --
USHORT APIENTRY16 DOS16GETINFOSEG(PUSHORT pselGlobal, PUSHORT pselLocal);
#define DosGetInfoSeg DOS16GETINFOSEG

/* Global Information Segment */
typedef struct _GINFOSEG {	/* gis */
    ULONG   time;
    ULONG   msecs;
    UCHAR   hour;
    UCHAR   minutes;
    UCHAR   seconds;
    UCHAR   hundredths;
    USHORT  timezone;
    USHORT  cusecTimerInterval;
    UCHAR   day;
    UCHAR   month;
    USHORT  year;
    UCHAR   weekday;
    UCHAR   uchMajorVersion;
    UCHAR   uchMinorVersion;
    UCHAR   chRevisionLetter;
    UCHAR   sgCurrent;
    UCHAR   sgMax;
    UCHAR   cHugeShift;
    UCHAR   fProtectModeOnly;
    USHORT  pidForeground;
    UCHAR   fDynamicSched;
    UCHAR   csecMaxWait;
    USHORT  cmsecMinSlice;
    USHORT  cmsecMaxSlice;
    USHORT  bootdrive;
    UCHAR   amecRAS[32];
    UCHAR   csgWindowableVioMax;
    UCHAR   csgPMMax;
    USHORT  SIS_Syslog;		 /* Error logging status (NOT DOCUMENTED) */
    USHORT  SIS_MMIOBase;	 /* Memory mapped I/O selector */
    USHORT  SIS_MMIOAddr;	 /* Memory mapped I/O selector */
    UCHAR   SIS_MaxVDMs;	 /* Max. no. of Virtual DOS machines */
    UCHAR   SIS_Reserved;
} GINFOSEG;
typedef GINFOSEG FAR *PGINFOSEG;

static int FSLib_IsPMTheForegroundFSSession()
{
  APIRET         rc;
  USHORT         globalSeg, localSeg;
  GINFOSEG       *pInfo;

  // Returns TRUE if PM is the foreground (active) fullscreen session

  // For this, we query the current FS session ID, and check if that's 1.
  // The session ID of the PM is always 1.

  globalSeg = localSeg = 0;
  rc = DosGetInfoSeg(&globalSeg, &localSeg);

  if (rc!=NO_ERROR)
    return 0; // Default: no

  // Make FLAT pointer from selector
  pInfo = (PGINFOSEG) (((ULONG)(globalSeg & 0xFFF8))<<13);
  if (!pInfo)
    return 0; // Default: no

  // PM is in foreground, if the current fullscreen session ID is 1
  return (pInfo->sgCurrent==1);
}


// Get all available fullscreen video modes
DECLSPEC FSLib_VideoMode_p FSLIBCALL FSLib_GetVideoModeList()
{
  FSLib_VideoMode_p pResult, pLast, pNewEntry;
  GA_modeInfo modeInfo;
  N_uint16   *mode;

  pLast = pResult = NULL;

  // We'll query available modes from SNAP
  if (DosRequestMutexSem(hmtxUseSNAP, SEM_INDEFINITE_WAIT)==NO_ERROR)
  {
    mode = dc->AvailableModes;
    while (*mode!=0xFFFF)
    {
      modeInfo.dwSize = sizeof(modeInfo);
      if (initFuncs.GetVideoModeInfo(*mode, &modeInfo)==0)
      {
        // Ok, got info about this mode info!
        // If it's a graphical mode, and at least high color, then
        // we can use it. We do not support palettized modes!
        if ((!(modeInfo.Attributes & gaIsTextMode)) &&
            (modeInfo.BitsPerPixel>=15))
        {
          pNewEntry = (FSLib_VideoMode_p) malloc(sizeof(FSLib_VideoMode));
          if (pNewEntry)
          {
	    // Prepare new entry
            FSLib_GAmodeInfo2FSLibVideoMode(&modeInfo, pNewEntry);
            pNewEntry->pNext = NULL;
            // link it into linked list
            if (!pLast)
              pResult = pNewEntry;
            else
              pLast->pNext = pNewEntry;

            pLast = pNewEntry;
          }
        }
      }
      mode++;
    }
    DosReleaseMutexSem(hmtxUseSNAP);
  }
  return pResult;
}

// Free list of available fullscreen video modes
DECLSPEC int               FSLIBCALL FSLib_FreeVideoModeList(FSLib_VideoMode_p pVideoModeListHead)
{
  FSLib_VideoMode_p pToDelete;

  while (pVideoModeListHead)
  {
    pToDelete = pVideoModeListHead;
    pVideoModeListHead = pVideoModeListHead->pNext;
    free(pToDelete);
  }

  return 1;
}
// Get pointer to desktop video mode (Don't free it, it's static!)
DECLSPEC FSLib_VideoMode_p FSLIBCALL FSLib_GetDesktopVideoMode()
{
  return &desktopVideoModeInfo;
}

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
                                              PHWND  phwndFrame)     // Result 2 : frame window handle
{
  // Check parameters first
  if ((!phwndClient) || (!phwndFrame) || (!pSrcBufferDesc) || (!pfnwpUserWindowProc))
  {
#ifdef DEBUG_BUILD
    printf("[FSLib_CreateWindow] : Invalid parameter(s)!\n");
#endif
    return 0;
  }

  // Create window
  *phwndFrame = WinCreateStdWindow(hwndParent,
                                   flStyle,
                                   pflCreateFlags,
                                   FSLIB_WINDOWCLASS_NAME,
                                   pszTitle,
                                   0, // Client window style,
                                   hmod, idResources, // Module handle and resource ID
                                   phwndClient);

  // If the window could be created, then fill window data with initial values!
  // Note, that some fields have already been filled by WM_CREATE!
  if (*phwndClient)
  {
    HMTX hmtxUseWindowData;
    GA_modeInfo *pmodeInfo;
    GA_modeInfo FSModeInfo;
    FSLib_VideoMode_p pWndSrcBufferDesc;

    FSLib_FindBestFSMode(pSrcBufferDesc, &FSModeInfo);

    hmtxUseWindowData = (HMTX) WinQueryWindowULong(*phwndClient, FSLIB_WINDOWDATA_HMTX_USEWINDOWDATA);
    if (DosRequestMutexSem(hmtxUseWindowData, SEM_INDEFINITE_WAIT)==NO_ERROR)
    {
      // Set user window function
      WinSetWindowULong(*phwndClient, FSLIB_WINDOWDATA_PFNWP_USERWINDOWFN, (ULONG) pfnwpUserWindowProc);
      // Set FS mode info
      pmodeInfo = (GA_modeInfo *) WinQueryWindowULong(*phwndClient, FSLIB_WINDOWDATA_PGAMODEINFO_FSMODEINFO);
      if (pmodeInfo)
	memcpy(pmodeInfo, &FSModeInfo, sizeof(GA_modeInfo));
      // Set source buffer format
      pWndSrcBufferDesc = (FSLib_VideoMode_p) WinQueryWindowULong(*phwndClient, FSLIB_WINDOWDATA_PVOID_SRCBUFFERDESC);
      if (pWndSrcBufferDesc)
        memcpy(pWndSrcBufferDesc, pSrcBufferDesc, sizeof(FSLib_VideoMode));

      DosReleaseMutexSem(hmtxUseWindowData);
      // Notification to window that everything have been initialized
      WinSendMsg(*phwndClient, WM_FSLIBCOMMAND, (MPARAM) FSLC_INITIALIZED, (MPARAM) 0);
      // Make sure we'll always know where to draw the image!
      WinSetVisibleRegionNotify(*phwndClient, TRUE);
      WinPostMsg(*phwndClient, WM_VRNENABLED, 0, 0 );
    }
    return 1;
  } else
  {
#ifdef DEBUG_BUILD
    HAB hab;
    printf("[FSLib_CreateWindow] : WinCreateStdWindow returned NULLHANDLE!\n");
    hab = WinQueryAnchorBlock(hwndParent);
    printf("                       WinGetLastError = %x\n", WinGetLastError(hab));
#endif
    return 0;
  }
}

// Set an user parameter to FSLib client window
DECLSPEC int               FSLIBCALL FSLib_AddUserParm(HWND hwndClient,
                                             void *pUserParm)
{
  // Check if hwndClient is really an FSLib class window!
  if (!FSLib_IsFSLibWindow(hwndClient)) return 0;
  WinSetWindowULong(hwndClient, FSLIB_WINDOWDATA_PVOID_USERDATA, (ULONG) pUserParm);
  return 1;
}

// Get the user parameter from FSLib client window
DECLSPEC void            * FSLIBCALL FSLib_GetUserParm(HWND hwndClient)
{
  // Check if hwndClient is really an FSLib class window!
  if (!FSLib_IsFSLibWindow(hwndClient)) return 0;
  return (void *) WinQueryWindowULong(hwndClient, FSLIB_WINDOWDATA_PVOID_USERDATA);
}

// Switch to/from Fullscreen mode
DECLSPEC int               FSLIBCALL FSLib_ToggleFSMode(HWND hwndClient,
                                              int iInFullscreenMode)
{
  // Check if hwndClient is really an FSLib class window!
  if (!FSLib_IsFSLibWindow(hwndClient)) return 0;
  WinPostMsg(hwndClient, WM_FSLIBCOMMAND, (MPARAM) FSLC_TOGGLEFSMODE, (MPARAM) iInFullscreenMode);
  return 1;
}

// Query if the application runs in Fullscreen mode
DECLSPEC int               FSLIBCALL FSLib_QueryFSMode(HWND hwndClient)
{
  // Check if hwndClient is really an FSLib class window!
  if (!FSLib_IsFSLibWindow(hwndClient)) return 0;

  return (int) WinQueryWindowULong(hwndClient, FSLIB_WINDOWDATA_INT_RUNINFSMODE);
}

// Change source buffer parameters on the fly
// (does not change window size in windowed mode,
//  but can change fullscreen video mode in fullscreen mode!)
DECLSPEC int               FSLIBCALL FSLib_SetSrcBufferDesc(HWND hwndClient,
                                          FSLib_VideoMode_p pSrcBufferDesc) // Description of source buffer format
{
  // Check if hwndClient is really an FSLib class window!
  if (!FSLib_IsFSLibWindow(hwndClient)) return 0;

  WinSendMsg(hwndClient, WM_FSLIBCOMMAND, (MPARAM) FSLC_SETSRCBUFFERDESC, (MPARAM) pSrcBufferDesc);
  return 1;
}

DECLSPEC int               FSLIBCALL FSLib_IsFSLibWindow(HWND hwndClient)
{
  HAB hab;
  char achWindowClass[80];

  hab = WinQueryAnchorBlock(hwndClient);
  if (!hab) return 0;
  if (!WinIsWindow(hab, hwndClient)) return 0;

  achWindowClass[0] = 0;
  WinQueryClassName(hwndClient, sizeof(achWindowClass), achWindowClass);

  if (strcmp(achWindowClass, FSLIB_WINDOWCLASS_NAME))
  {
//    printf("[FSLib_IsFSLibWindow] : %s != %s\n", achWindowClass, FSLIB_WINDOWCLASS_NAME);
    return 0;
  }

  return 1;
}

#define FSLib_IsTheSamePixelFormat(FSLibPixelFormat, SNAPPixelFormat)        \
  ((FSLibPixelFormat.ucRedMask==SNAPPixelFormat.RedMask) &&                  \
   (FSLibPixelFormat.ucRedPosition==SNAPPixelFormat.RedPosition) &&          \
   (FSLibPixelFormat.ucRedAdjust==SNAPPixelFormat.RedAdjust) &&              \
   (FSLibPixelFormat.ucGreenMask==SNAPPixelFormat.GreenMask) &&              \
   (FSLibPixelFormat.ucGreenPosition==SNAPPixelFormat.GreenPosition) &&      \
   (FSLibPixelFormat.ucGreenAdjust==SNAPPixelFormat.GreenAdjust) &&          \
   (FSLibPixelFormat.ucBlueMask==SNAPPixelFormat.BlueMask) &&                \
   (FSLibPixelFormat.ucBluePosition==SNAPPixelFormat.BluePosition) &&        \
   (FSLibPixelFormat.ucBlueAdjust==SNAPPixelFormat.BlueAdjust) &&            \
   (FSLibPixelFormat.ucAlphaMask==SNAPPixelFormat.AlphaMask) &&                \
   (FSLibPixelFormat.ucAlphaPosition==SNAPPixelFormat.AlphaPosition) &&        \
   (FSLibPixelFormat.ucAlphaAdjust==SNAPPixelFormat.AlphaAdjust))

// Blit something into the window
// (it will take care of clipping, resizing and color space conversion if needed)
DECLSPEC int               FSLIBCALL FSLib_BitBlt(HWND hwndClient,
                                        void *pSourceImage,
                                        unsigned int uiTop,
                                        unsigned int uiLeft,
                                        unsigned int uiWidth,
                                        unsigned int uiHeight)
{
  HMTX hmtxUseWindowData;
  int rc = 0;
  GA_modeInfo *pFSModeInfo;
  int iVRNDisabled;
  int iRunInFSMode;
  int iWindowActive;
  PRECTL prectlVisibleArea;
  int iNumOfVisibleAreaRectangles;
  PRECTL prectlModifiedVisibleArea;
  int iNumOfModifiedVisibleAreaRectangles;
  FSLib_VideoMode_p pSrcBufferDesc;
  int i;

#ifdef DEBUG_BUILD
  printf("[FSLib_BitBlt] : Enter\n");
#endif
  // Check parameters
  if ((!pSourceImage) || (!uiWidth) || (!uiHeight)) return 0;

#ifdef DEBUG_BUILD
  printf("[FSLib_BitBlt] : Check window class\n");
#endif

  // Check if hwndClient is really an FSLib class window!
  if (!FSLib_IsFSLibWindow(hwndClient)) return 0;

#ifdef DEBUG_BUILD
  printf("[FSLib_BitBlt] : Grab mutex\n");
#endif

  hmtxUseWindowData = (HMTX) WinQueryWindowULong(hwndClient, FSLIB_WINDOWDATA_HMTX_USEWINDOWDATA);
  if (DosRequestMutexSem(hmtxUseWindowData, SEM_INDEFINITE_WAIT)==NO_ERROR)
  {
    // Get variables from window data
    pFSModeInfo = (GA_modeInfo *) WinQueryWindowULong(hwndClient, FSLIB_WINDOWDATA_PGAMODEINFO_FSMODEINFO);
    iVRNDisabled = (int) WinQueryWindowULong(hwndClient, FSLIB_WINDOWDATA_INT_VRNDISABLED);
    iRunInFSMode = (int) WinQueryWindowULong(hwndClient, FSLIB_WINDOWDATA_INT_RUNINFSMODE);
    iWindowActive = (int) WinQueryWindowULong(hwndClient, FSLIB_WINDOWDATA_INT_WINDOWACTIVE);
    prectlVisibleArea = (PRECTL) WinQueryWindowULong(hwndClient, FSLIB_WINDOWDATA_PRECTL_VISIBLEAREA);
    iNumOfVisibleAreaRectangles = (int) WinQueryWindowULong(hwndClient, FSLIB_WINDOWDATA_INT_NUMOFVISIBLEAREARECTANGLES);
    pSrcBufferDesc = (FSLib_VideoMode_p) WinQueryWindowULong(hwndClient, FSLIB_WINDOWDATA_PVOID_SRCBUFFERDESC);

    // Check if we're in FS mode!
    if (iRunInFSMode)
    {
#ifdef DEBUG_BUILD
      printf("[FSLib_BitBlt] : In FS mode.\n");
#endif

      // We're in FS mode, we should do non-stretched blitting to
      // the middle of the screen!
      if (!iWindowActive)
      {
#ifdef DEBUG_BUILD
        printf("[FSLib_BitBlt] : In FS mode, Window not active. Exit.\n");
#endif

	// But we're not the active application, so we're not
	// visible. Let's swallow this blit request!
        rc = 1;
      } else
      {
	int iStartX, iStartY;

	// Calculate starting pos for top-left corner of image, to make
	// it centered in fullscreen mode!
	iStartX = (pFSModeInfo->XResolution - pSrcBufferDesc->uiXResolution)/2;
	iStartY = (pFSModeInfo->YResolution - pSrcBufferDesc->uiYResolution)/2;
	// Check if we need color conversion or not!
	// We'll use different blitter.
	if (FSLib_IsTheSamePixelFormat(pSrcBufferDesc->PixelFormat,
				       pFSModeInfo->PixelFormat))
        {
#ifdef SWCURSOR_HACK
          // Hide mouse cursor if have to
          if ((cursorFuncs.BeginAccess) && (cursorFuncs.EndAccess))
            cursorFuncs.BeginAccess(iStartX,
                                    iStartY,
                                    iStartX + pSrcBufferDesc->uiXResolution-1,
                                    iStartY + pSrcBufferDesc->uiYResolution-1);
#endif
#ifdef DEBUG_BUILD
          printf("[FSLib_BitBlt] : (FS) : Same pixel format!\n");
#endif
	  if (draw2d.BitBltSys)
	    draw2d.BitBltSys(pSourceImage,
			     pSrcBufferDesc->uiScanLineSize,
			     0, 0,
			     pSrcBufferDesc->uiXResolution,
			     pSrcBufferDesc->uiYResolution,
			     iStartX,
			     iStartY,
			     GA_REPLACE_MIX,
                             0); // Flip
#ifdef DEBUG_BUILD
	  else
            printf("[FSLib_BitBlt] : (FS) : No BitBltSys available!\n");
#endif

#ifdef SWCURSOR_HACK
          // Restore mouse cursor if have to
          if ((cursorFuncs.BeginAccess) && (cursorFuncs.EndAccess))
            cursorFuncs.EndAccess();
#endif
        } else
	{
          GA_bltFx fx;
	  GA_pixelFormat SrcPixelFormat;
#ifdef DEBUG_BUILD
          printf("[FSLib_BitBlt] : (FS) : Different pixel format!\n");
#endif
          // Prepare source pixel format description
	  SrcPixelFormat.RedMask = pSrcBufferDesc->PixelFormat.ucRedMask;
	  SrcPixelFormat.RedPosition = pSrcBufferDesc->PixelFormat.ucRedPosition;
          SrcPixelFormat.RedAdjust = pSrcBufferDesc->PixelFormat.ucRedAdjust;
	  SrcPixelFormat.GreenMask = pSrcBufferDesc->PixelFormat.ucGreenMask;
	  SrcPixelFormat.GreenPosition = pSrcBufferDesc->PixelFormat.ucGreenPosition;
          SrcPixelFormat.GreenAdjust = pSrcBufferDesc->PixelFormat.ucGreenAdjust;
	  SrcPixelFormat.BlueMask = pSrcBufferDesc->PixelFormat.ucBlueMask;
	  SrcPixelFormat.BluePosition = pSrcBufferDesc->PixelFormat.ucBluePosition;
	  SrcPixelFormat.BlueAdjust = pSrcBufferDesc->PixelFormat.ucBlueAdjust;
	  SrcPixelFormat.AlphaMask = pSrcBufferDesc->PixelFormat.ucAlphaMask;
	  SrcPixelFormat.AlphaPosition = pSrcBufferDesc->PixelFormat.ucAlphaPosition;
          SrcPixelFormat.AlphaAdjust = pSrcBufferDesc->PixelFormat.ucAlphaAdjust;

	  fx.dwSize = sizeof(fx);
	  fx.Flags = gaBltConvert;
	  fx.BitsPerPixel = pSrcBufferDesc->uiBPP;
	  fx.PixelFormat = &SrcPixelFormat;

#ifdef SWCURSOR_HACK
          // Hide mouse cursor if have to
          if ((cursorFuncs.BeginAccess) && (cursorFuncs.EndAccess))
            cursorFuncs.BeginAccess(iStartX,
                                    iStartY,
                                    iStartX + pSrcBufferDesc->uiXResolution-1,
                                    iStartY + pSrcBufferDesc->uiYResolution-1);
#endif

          if (draw2d.BitBltFxSys)
	    draw2d.BitBltFxSys(pSourceImage,
			       pSrcBufferDesc->uiScanLineSize,
			       0, 0,
			       pSrcBufferDesc->uiXResolution,
			       pSrcBufferDesc->uiYResolution,
			       iStartX,
			       iStartY,
			       pSrcBufferDesc->uiXResolution,
			       pSrcBufferDesc->uiYResolution,
			       &fx);
#ifdef DEBUG_BUILD
          else
            printf("[FSLib_BitBlt] : (FS) : No BitBltFxSys available!\n");
#endif
#ifdef SWCURSOR_HACK
          // Restore mouse cursor if have to
          if ((cursorFuncs.BeginAccess) && (cursorFuncs.EndAccess))
            cursorFuncs.EndAccess();
#endif
	}
        rc = 1;
      }
    } else
    {
      // We're running in windowed mode.

#ifdef DEBUG_BUILD
      printf("[FSLib_BitBlt] : In window\n");
#endif

      // Let's see if the blitting is allowed or not, and if there is visible region
      // or not!
      if ((iVRNDisabled) || (iNumOfVisibleAreaRectangles<=0) || (!prectlVisibleArea))
      {
#ifdef DEBUG_BUILD
        printf("[FSLib_BitBlt] : (Win) : Blitting disabled or no visible area available!\n");
#endif
        if (iVRNDisabled)
        {
#ifdef DEBUG_BUILD
          printf("[FSLib_BitBlt] : (Win) : Blitting disabled so marking window dirty!\n");
#endif
          WinSetWindowULong(hwndClient, FSLIB_WINDOWDATA_INT_DIRTYWINDOW, (ULONG) 1); // True
        }
	// Blitting disabled or no visible area available!
        rc = 1;
      } else
      {
        int iSL, iST; // SourceLeft, SourceTop
        int iSW, iSH; // SourceWidth, SourceHeight
        int iSBufW, iSBufH; // SourceBufferWidth, SourceBufferHeight;
        int iDBufW, iDBufH; // DestBufferWidth, DestBufferHeight;
        int iDL, iDT; // DestLeft, DestTop;
        int iDR, iDB; // DestRight, DestBottom;
        int iDW, iDH; // DestWidth, DestHeight;
        int iDL_s, iDT_s; // DestLeft and DestTop in screen (buffer coordinates)
	POINTL ptlScreen; // DestRight and DestTop in screen coordinates
        RECTL rectlTemp1, rectlTemp2, rectlDest;
	SWP swpClient, swpDesktop;
        HAB habClient;

        // Blitting enabled, and there is visible area.
#ifdef DEBUG_BUILD
        printf("[FSLib_BitBlt] : (Win) : Blitting enabled and there's visible area available!\n");
#endif


        // Calculate the resulting position and size based on window size and position.
        // To make it good even when scaling, we do the following:
	// - Calculate window points from image points
        // - Add extra clip rectangles so only these stuffs will be blitted
        // - Blit all image

        // Get current window size and position
        WinQueryWindowPos(hwndClient, &swpClient);
        // Also get desktop size (will be needed later)
        WinQueryWindowPos(HWND_DESKTOP, &swpDesktop);

        // Calculate destination position and size
        iSBufW = pSrcBufferDesc->uiXResolution;
        iSBufH = pSrcBufferDesc->uiYResolution;
        iDBufW = swpClient.cx;
        iDBufH = swpClient.cy;
        // Make sure we won'd do division by zero!
        if ((iSBufW) && (iSBufH) && (iDBufW) && (iDBufH))
        {
#ifdef DEBUG_BUILD
          printf("[FSLib_BitBlt] : (Win) : No Div0\n");
#endif

          // Get source area
          iSL = uiLeft;
          iST = uiTop;
          iSW = uiWidth;
          iSH = uiHeight;

          // Calculate destination area (scaled)
	  iDL = (iSL * iDBufW + iSBufW-1) / iSBufW;
          iDT = (iST * iDBufH + iSBufH-1) / iSBufH;
	  iDW = (iSW * iDBufW + iSBufW-1) / iSBufW;
          iDH = (iSH * iDBufH + iSBufH-1) / iSBufH;
	  iDR = iDL -1 + iDW;
	  iDB = iDT -1 + iDH;

          // Make a clip area from destination area (rectlTemp1)
          ptlScreen.x = iDL;
          ptlScreen.y = swpClient.cy - iDB -1;
          WinMapWindowPoints(hwndClient, HWND_DESKTOP, &ptlScreen, 1);
          rectlTemp1.xLeft = ptlScreen.x;
          rectlTemp1.yBottom = ptlScreen.y;
          ptlScreen.x = iDR;
          ptlScreen.y = swpClient.cy - iDT -1;
          WinMapWindowPoints(hwndClient, HWND_DESKTOP, &ptlScreen, 1);
          rectlTemp1.xRight = ptlScreen.x+1;
          rectlTemp1.yTop = ptlScreen.y+1;

          // Calculate destination buffer position on screen
          ptlScreen.x = 0;
          ptlScreen.y = swpClient.cy-1;
          WinMapWindowPoints(hwndClient, HWND_DESKTOP, &ptlScreen, 1);
          iDL_s = ptlScreen.x;
          iDT_s = swpDesktop.cy - ptlScreen.y - 1;

          // Create new visible are list, by intersecting all the currently
          // visible ares with our clip area!
          prectlModifiedVisibleArea = (PRECTL) malloc(sizeof(RECTL) * iNumOfVisibleAreaRectangles);
          if (prectlModifiedVisibleArea)
          {
            iNumOfModifiedVisibleAreaRectangles = 0;
            habClient = WinQueryAnchorBlock(hwndClient);
            // Go through all the visible areas, and intersect them with this area!
            for (i=0; i<iNumOfVisibleAreaRectangles; i++)
            {
              // Get clip area,
              // convert to desktop coordinates!

              ptlScreen.x = prectlVisibleArea[i].xLeft;
              ptlScreen.y = prectlVisibleArea[i].yTop;
              WinMapWindowPoints(hwndClient, HWND_DESKTOP, &ptlScreen, 1);
              rectlTemp2.xLeft   = ptlScreen.x;
              rectlTemp2.yTop    = ptlScreen.y;
              ptlScreen.x = prectlVisibleArea[i].xRight;
              ptlScreen.y = prectlVisibleArea[i].yBottom;
              WinMapWindowPoints(hwndClient, HWND_DESKTOP, &ptlScreen, 1);
              rectlTemp2.xRight  = ptlScreen.x;
              rectlTemp2.yBottom = ptlScreen.y;

#ifdef DEBUG_BUILD
              printf("[FSLib_BitBlt] : (Win) : Checking intersect (%d %d)->(%d %d) and (%d %d)->(%d %d)\n",
                     rectlTemp1.xLeft, rectlTemp1.yBottom, rectlTemp1.xRight, rectlTemp1.yTop,
                     rectlTemp2.xLeft, rectlTemp2.yBottom, rectlTemp2.xRight, rectlTemp2.yTop
                    );
#endif

              if (WinIntersectRect(habClient,
                                   &rectlDest,
                                   &rectlTemp1,
                                   &rectlTemp2))
              {
                // These do intersect, so put it into final visible region list!
                iNumOfModifiedVisibleAreaRectangles++;
                memcpy(&(prectlModifiedVisibleArea[iNumOfModifiedVisibleAreaRectangles-1]),
                       &rectlDest,
                       sizeof(rectlDest));
#ifdef DEBUG_BUILD
                printf("[FSLib_BitBlt] : (Win) : Intersect (%d)\n", iNumOfModifiedVisibleAreaRectangles);
#endif

              }
            }

            if  (FSLib_IsPMTheForegroundFSSession())
            {
              // Ok, we have direct screen access.
#ifdef DEBUG_BUILD
              printf("[FSLib_BitBlt] : (Win) : PM is foreground\n");
#endif
              // Now, we need access to SNAP functions!
              if (DosRequestMutexSem(hmtxUseSNAP, SEM_INDEFINITE_WAIT)==NO_ERROR)
              {
                GA_pixelFormat PixelFormat;
                GA_bltFx fx;
#ifdef SWCURSOR_HACK
                int bMouseHidden = 0;
#endif

                PM_lockSNAPAccess(0, 1);

#ifdef USE_ENABLEDIRECTACCESS
                if (state2d.EnableDirectAccess)
                  state2d.EnableDirectAccess();
#endif

#ifdef USE_SETDRAWSURFACE
                ref2ddrv->SetDrawSurface(dc->LinearMem,
                                         currentModeInfo.XResolution,
                                         currentModeInfo.YResolution,
                                         currentModeInfo.BytesPerScanLine,
                                         currentModeInfo.BitsPerPixel,
                                         &(currentModeInfo.PixelFormat));
#endif

#ifdef SWCURSOR_HACK
                // Hide mouse cursor if have to
                if ((cursorFuncs.BeginAccess) && (cursorFuncs.EndAccess) &&
                    (cursorFuncs.IsHardwareCursor)
//                    && (!cursorFuncs.IsHardwareCursor())
                   )
                {
                  bMouseHidden = 1;
                  cursorFuncs.BeginAccess(iDL_s,
                                          iDT_s,
                                          iDL_s + iDBufW-1,
                                          iDT_s + iDBufH-1);
#ifdef DEBUG_BUILD
                  printf("[FSLib_BitBlt] : (Win) : HideMouse\n");
#endif

                  //WinShowPointer(HWND_DESKTOP, FALSE);
                }
#endif
                // Prepare effects descriptor
                memset(&fx, 0, sizeof(fx));

                fx.dwSize = sizeof(fx);
                fx.Flags = gaBltClip;
                fx.Mix = GA_REPLACE_MIX;
                fx.BitsPerPixel = pSrcBufferDesc->uiBPP;
                fx.PixelFormat = &PixelFormat;

                // Prepare source pixel format description
                PixelFormat.RedMask = pSrcBufferDesc->PixelFormat.ucRedMask;
                PixelFormat.RedPosition = pSrcBufferDesc->PixelFormat.ucRedPosition;
                PixelFormat.RedAdjust = pSrcBufferDesc->PixelFormat.ucRedAdjust;
                PixelFormat.GreenMask = pSrcBufferDesc->PixelFormat.ucGreenMask;
                PixelFormat.GreenPosition = pSrcBufferDesc->PixelFormat.ucGreenPosition;
                PixelFormat.GreenAdjust = pSrcBufferDesc->PixelFormat.ucGreenAdjust;
                PixelFormat.BlueMask = pSrcBufferDesc->PixelFormat.ucBlueMask;
                PixelFormat.BluePosition = pSrcBufferDesc->PixelFormat.ucBluePosition;
                PixelFormat.BlueAdjust = pSrcBufferDesc->PixelFormat.ucBlueAdjust;
                PixelFormat.AlphaMask = pSrcBufferDesc->PixelFormat.ucAlphaMask;
                PixelFormat.AlphaPosition = pSrcBufferDesc->PixelFormat.ucAlphaPosition;
                PixelFormat.AlphaAdjust = pSrcBufferDesc->PixelFormat.ucAlphaAdjust;

                // Let's see if we need color space conversion!
                if (!FSLib_IsTheSamePixelFormat(pSrcBufferDesc->PixelFormat,
                                                desktopModeInfo.PixelFormat))
                  fx.Flags |= gaBltConvert;

                // Let's see if we have to resize stuffs!
                if ((pSrcBufferDesc->uiXResolution != swpClient.cx) ||
                    (pSrcBufferDesc->uiYResolution != swpClient.cy))
                  fx.Flags |= gaBltAnyStretch;

#ifdef DEBUG_BUILD
                printf("[FSLib_BitBlt] : (Win) : Blit %d rects\n", iNumOfModifiedVisibleAreaRectangles);
#endif


                for (i=0; i<iNumOfModifiedVisibleAreaRectangles; i++)
                {
                  // Get modified clip area,
                  // They are already converted to desktop coordinates, and then to screen!

                  fx.ClipLeft = prectlModifiedVisibleArea[i].xLeft;
                  fx.ClipTop = swpDesktop.cy - prectlModifiedVisibleArea[i].yTop;
                  fx.ClipRight = prectlModifiedVisibleArea[i].xRight;
                  fx.ClipBottom = swpDesktop.cy - prectlModifiedVisibleArea[i].yBottom;

#ifdef DEBUG_BUILD
                  printf("[FSLib_BitBlt] : Blitting src: %d;%d->%d;%d , dst: %d;%d->%d;%d, clip: %d;%d->%d;%d\n",
                         0,                                            // Source rectangle
                         0,
                         iSBufW,
                         iSBufH,
                         iDL_s,                                           // Destination rectangle
                         iDT_s,
                         iDBufW,
                         iDBufH,
                         fx.ClipLeft,
                         fx.ClipTop,
                         fx.ClipRight,
                         fx.ClipBottom
                        );
                  fflush(stdout);
#endif
                  
                  // Do the blitting for this visible area!
                  // Blit to destination buffer (screen) from source buffer
                  if (draw2d.BitBltFxSys)
                    draw2d.BitBltFxSys(pSourceImage,
                                       pSrcBufferDesc->uiScanLineSize,
                                       0,                                            // Source rectangle
                                       0,
                                       iSBufW,
                                       iSBufH,
                                       iDL_s,                                           // Destination rectangle
                                       iDT_s,
                                       iDBufW,
                                       iDBufH,
                                       &fx);                                            // Effects

#ifdef DEBUG_BUILD
                  else
                    printf("[FSLib_BitBlt] : No BitBltFxSys available!\n");
#endif
#ifdef DEBUG_BUILD
                  printf("[FSLib_BitBlt] : End of Blitting\n");
                  fflush(stdout);
#endif

                }
#ifdef SWCURSOR_HACK
                // Resore mouse cursor if have to
                if ((cursorFuncs.BeginAccess) && (cursorFuncs.EndAccess) && (bMouseHidden))
                {
                  cursorFuncs.EndAccess();
                  //WinShowPointer(HWND_DESKTOP, TRUE);
                }
#endif

#ifdef USE_ENABLEDIRECTACCESS
                if (state2d.DisableDirectAccess)
                  state2d.DisableDirectAccess();
#endif
                PM_unlockSNAPAccess(0);

                DosReleaseMutexSem(hmtxUseSNAP);
              }
              
            }
#ifdef DEBUG_BUILD
            else
              printf("[FSLib_BitBlt] : (Win) : PM is *NOT* foreground\n");
#endif

            // Free Modified visible area list
            free(prectlModifiedVisibleArea);
          }
#ifdef DEBUG_BUILD
          else
          {
            printf("[FSLib_BitBlt] : Not enough memory for modified visible area!\n");
          }
#endif

        }
      }
    }
    DosReleaseMutexSem(hmtxUseWindowData);
  }
  return rc;
}

// Set the PixelFormat field to some pre-defined formats
DECLSPEC int               FSLIBCALL FSLib_SetPixelFormat(FSLib_PixelFormat_p pPixelFormat, int iFormat)
{
  if (!pPixelFormat) return 0;
  switch (iFormat)
  {
    case FSLIB_PIXELFORMAT_555_15BPP:
      pPixelFormat->ucRedMask        = 0x1f;
      pPixelFormat->ucRedPosition    = 0x0a;
      pPixelFormat->ucRedAdjust      = 3;

      pPixelFormat->ucGreenMask      = 0x1f;
      pPixelFormat->ucGreenPosition  = 0x05;
      pPixelFormat->ucGreenAdjust    = 3;

      pPixelFormat->ucBlueMask       = 0x1f;
      pPixelFormat->ucBluePosition   = 0x00;
      pPixelFormat->ucBlueAdjust     = 3;

      pPixelFormat->ucAlphaMask      = 0x01;
      pPixelFormat->ucAlphaPosition  = 0x0f;
      pPixelFormat->ucAlphaAdjust    = 7;

      return 1;
    case FSLIB_PIXELFORMAT_565_16BPP:
      pPixelFormat->ucRedMask        = 0x1f;
      pPixelFormat->ucRedPosition    = 0x0b;
      pPixelFormat->ucRedAdjust      = 3;

      pPixelFormat->ucGreenMask      = 0x3f;
      pPixelFormat->ucGreenPosition  = 0x05;
      pPixelFormat->ucGreenAdjust    = 2;

      pPixelFormat->ucBlueMask       = 0x1f;
      pPixelFormat->ucBluePosition   = 0x00;
      pPixelFormat->ucBlueAdjust     = 3;

      pPixelFormat->ucAlphaMask      = 0x00;
      pPixelFormat->ucAlphaPosition  = 0x00;
      pPixelFormat->ucAlphaAdjust    = 0;

      return 1;
    case FSLIB_PIXELFORMAT_RGB_24BPP:
      pPixelFormat->ucRedMask        = 0xff;
      pPixelFormat->ucRedPosition    = 0x10;
      pPixelFormat->ucRedAdjust      = 0;

      pPixelFormat->ucGreenMask      = 0xff;
      pPixelFormat->ucGreenPosition  = 0x08;
      pPixelFormat->ucGreenAdjust    = 0;

      pPixelFormat->ucBlueMask       = 0xff;
      pPixelFormat->ucBluePosition   = 0x00;
      pPixelFormat->ucBlueAdjust     = 0;

      pPixelFormat->ucAlphaMask      = 0x00;
      pPixelFormat->ucAlphaPosition  = 0x00;
      pPixelFormat->ucAlphaAdjust    = 0;

      return 1;
    case FSLIB_PIXELFORMAT_BGR_24BPP:
      pPixelFormat->ucRedMask        = 0xff;
      pPixelFormat->ucRedPosition    = 0x00;
      pPixelFormat->ucRedAdjust      = 0;

      pPixelFormat->ucGreenMask      = 0xff;
      pPixelFormat->ucGreenPosition  = 0x08;
      pPixelFormat->ucGreenAdjust    = 0;

      pPixelFormat->ucBlueMask       = 0xff;
      pPixelFormat->ucBluePosition   = 0x10;
      pPixelFormat->ucBlueAdjust     = 0;

      pPixelFormat->ucAlphaMask      = 0x00;
      pPixelFormat->ucAlphaPosition  = 0x00;
      pPixelFormat->ucAlphaAdjust    = 0;

      return 1;
    case FSLIB_PIXELFORMAT_ARGB_32BPP:
      pPixelFormat->ucRedMask        = 0xff;
      pPixelFormat->ucRedPosition    = 0x10;
      pPixelFormat->ucRedAdjust      = 0;

      pPixelFormat->ucGreenMask      = 0xff;
      pPixelFormat->ucGreenPosition  = 0x08;
      pPixelFormat->ucGreenAdjust    = 0;

      pPixelFormat->ucBlueMask       = 0xff;
      pPixelFormat->ucBluePosition   = 0x00;
      pPixelFormat->ucBlueAdjust     = 0;

      pPixelFormat->ucAlphaMask      = 0xff;
      pPixelFormat->ucAlphaPosition  = 0x18;
      pPixelFormat->ucAlphaAdjust    = 0;

      return 1;
    case FSLIB_PIXELFORMAT_ABGR_32BPP:
      pPixelFormat->ucRedMask        = 0xff;
      pPixelFormat->ucRedPosition    = 0x00;
      pPixelFormat->ucRedAdjust      = 0;

      pPixelFormat->ucGreenMask      = 0xff;
      pPixelFormat->ucGreenPosition  = 0x08;
      pPixelFormat->ucGreenAdjust    = 0;

      pPixelFormat->ucBlueMask       = 0xff;
      pPixelFormat->ucBluePosition   = 0x10;
      pPixelFormat->ucBlueAdjust     = 0;

      pPixelFormat->ucAlphaMask      = 0xff;
      pPixelFormat->ucAlphaPosition  = 0x18;
      pPixelFormat->ucAlphaAdjust    = 0;

      return 1;
    case FSLIB_PIXELFORMAT_RGBA_32BPP:
      pPixelFormat->ucRedMask        = 0xff;
      pPixelFormat->ucRedPosition    = 0x18;
      pPixelFormat->ucRedAdjust      = 0;

      pPixelFormat->ucGreenMask      = 0xff;
      pPixelFormat->ucGreenPosition  = 0x10;
      pPixelFormat->ucGreenAdjust    = 0;

      pPixelFormat->ucBlueMask       = 0xff;
      pPixelFormat->ucBluePosition   = 0x08;
      pPixelFormat->ucBlueAdjust     = 0;

      pPixelFormat->ucAlphaMask      = 0xff;
      pPixelFormat->ucAlphaPosition  = 0x00;
      pPixelFormat->ucAlphaAdjust    = 0;

      return 1;
    case FSLIB_PIXELFORMAT_BGRA_32BPP:
      pPixelFormat->ucRedMask        = 0xff;
      pPixelFormat->ucRedPosition    = 0x08;
      pPixelFormat->ucRedAdjust      = 0;

      pPixelFormat->ucGreenMask      = 0xff;
      pPixelFormat->ucGreenPosition  = 0x10;
      pPixelFormat->ucGreenAdjust    = 0;

      pPixelFormat->ucBlueMask       = 0xff;
      pPixelFormat->ucBluePosition   = 0x18;
      pPixelFormat->ucBlueAdjust     = 0;

      pPixelFormat->ucAlphaMask      = 0xff;
      pPixelFormat->ucAlphaPosition  = 0x00;
      pPixelFormat->ucAlphaAdjust    = 0;

      return 1;
    default:
      return 0;
  }
}


// The main DLL entry for DLL Initialization and Uninitialization:

unsigned _System LibMain(unsigned hmod, unsigned termination)
{
  if (termination)
  {
    if (bFSLib_Initialized)
      FSLib_EmergencyUninitialize();
    return 1;
  } else
  {
#ifdef DEBUG_BUILD
    // Make stdout and stderr unbuffered!
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    // Initialize timer
    InitTimer();
#endif
    // Unfortunately, we cannot call FSLib_Initialize() from here,
    // because WinCreateMsgQueue() hangs the initialization process
    // for some reason. So, we have to move the FSLib_Initialize() and
    // FSLib_Uninitialize() to outside. But, we can still do the clean up
    // in the termination branch!
    return 1;
  }
}
