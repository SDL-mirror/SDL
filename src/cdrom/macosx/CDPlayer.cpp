/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

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

#include "CDPlayer.h"
#include "AudioFilePlayer.h"
#include "CAGuard.h"

// we're exporting these functions into C land for SDL_syscdrom.c
extern "C" {

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  Constants
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#define kAudioCDFilesystemID   (UInt16)(('J' << 8) | 'H') // 'JH'; this avoids compiler warning

// XML PList keys
#define kRawTOCDataString           "Format 0x02 TOC Data"
#define kSessionsString             "Sessions"
#define kSessionTypeString          "Session Type"
#define kTrackArrayString           "Track Array"
#define kFirstTrackInSessionString      "First Track"
#define kLastTrackInSessionString       "Last Track"
#define kLeadoutBlockString         "Leadout Block"
#define kDataKeyString              "Data"
#define kPointKeyString             "Point"
#define kSessionNumberKeyString         "Session Number"
#define kStartBlockKeyString            "Start Block"   
    
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  Globals
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma mark -- Globals --

static bool             playBackWasInit = false;
static AudioUnit        theUnit;
static AudioFilePlayer* thePlayer = NULL;
static CDPlayerCompletionProc   completionProc = NULL;
static SDL_mutex       *apiMutex = NULL;
static SDL_sem         *callbackSem;
static SDL_CD*          theCDROM;

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  Prototypes
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma mark -- Prototypes --

static OSStatus CheckInit ();

static OSStatus MatchAUFormats (AudioUnit theUnit, UInt32 theInputBus);

static void     FilePlayNotificationHandler (void* inRefCon, OSStatus inStatus);

static int      RunCallBackThread (void* inRefCon);


#pragma mark -- Public Functions --

void     Lock ()
{
    if (!apiMutex) {
        apiMutex = SDL_CreateMutex();
    }
    SDL_mutexP(apiMutex);
}

void     Unlock ()
{
    SDL_mutexV(apiMutex);
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  DetectAudioCDVolumes
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

int DetectAudioCDVolumes(FSVolumeRefNum *volumes, int numVolumes)
{
    int volumeIndex;
    int cdVolumeCount = 0;
    OSStatus result = noErr;
    
    for (volumeIndex = 1; result == noErr || result != nsvErr; volumeIndex++)
    {
        FSVolumeRefNum  actualVolume;
        FSVolumeInfo    volumeInfo;
        
        memset (&volumeInfo, 0, sizeof(volumeInfo));
        
        result = FSGetVolumeInfo (kFSInvalidVolumeRefNum,
                                  volumeIndex,
                                  &actualVolume,
                                  kFSVolInfoFSInfo,
                                  &volumeInfo,
                                  NULL,
                                  NULL); 
         
        if (result == noErr)
        {
            if (volumeInfo.filesystemID == kAudioCDFilesystemID) // It's an audio CD
            {
                if (volumes != NULL && cdVolumeCount < numVolumes)
                    volumes[cdVolumeCount] = actualVolume;
            
                cdVolumeCount++;
            }
        }
        else 
        {
            // I'm commenting this out because it seems to be harmless
            //SDL_SetError ("DetectAudioCDVolumes: FSGetVolumeInfo returned %d", result);
        }
    }
        
    return cdVolumeCount;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  ReadTOCData
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

int ReadTOCData (FSVolumeRefNum theVolume, SDL_CD *theCD)
{
    HFSUniStr255      dataForkName;
    OSStatus          theErr;
    SInt16            forkRefNum;
    SInt64            forkSize;
    Ptr               forkData = 0;
    ByteCount         actualRead;
    CFDataRef         dataRef = 0;
    CFPropertyListRef propertyListRef = 0;

    FSRefParam      fsRefPB;
    FSRef           tocPlistFSRef;
    
    const char* error = "Unspecified Error";
    
    // get stuff from .TOC.plist                                                   
    fsRefPB.ioCompletion = NULL;
    fsRefPB.ioNamePtr = "\p.TOC.plist";
    fsRefPB.ioVRefNum = theVolume;
    fsRefPB.ioDirID = 0;
    fsRefPB.newRef = &tocPlistFSRef;
    
    theErr = PBMakeFSRefSync (&fsRefPB);
    if(theErr != noErr) {
        error = "PBMakeFSRefSync";
        goto bail;
    }
    
    // Load and parse the TOC XML data

    theErr = FSGetDataForkName (&dataForkName);
    if (theErr != noErr) {
        error = "FSGetDataForkName";
        goto bail;
    }
    
    theErr = FSOpenFork (&tocPlistFSRef, dataForkName.length, dataForkName.unicode, fsRdPerm, &forkRefNum);
    if (theErr != noErr) {
        error = "FSOpenFork";
        goto bail;
    }
    
    theErr = FSGetForkSize (forkRefNum, &forkSize);
    if (theErr != noErr) {
        error = "FSGetForkSize";
        goto bail;
    }
    
    // Allocate some memory for the XML data
    forkData = NewPtr (forkSize);
    if(forkData == NULL) {
        error = "NewPtr";
        goto bail;
    }
    
    theErr = FSReadFork (forkRefNum, fsFromStart, 0 /* offset location */, forkSize, forkData, &actualRead);
    if(theErr != noErr) {
        error = "FSReadFork";
        goto bail;
    }
    
    dataRef = CFDataCreate (kCFAllocatorDefault, (UInt8 *)forkData, forkSize);
    if(dataRef == 0) {
        error = "CFDataCreate";
        goto bail;
    }

    propertyListRef = CFPropertyListCreateFromXMLData (kCFAllocatorDefault,
                                                       dataRef,
                                                       kCFPropertyListImmutable,
                                                       NULL);
    if (propertyListRef == NULL) {
        error = "CFPropertyListCreateFromXMLData";
        goto bail;
    }

    // Now we got the Property List in memory. Parse it.
    
    // First, make sure the root item is a CFDictionary. If not, release and bail.
    if(CFGetTypeID(propertyListRef)== CFDictionaryGetTypeID())
    {
        CFDictionaryRef dictRef = (CFDictionaryRef)propertyListRef;
        
        CFDataRef   theRawTOCDataRef;
        CFArrayRef  theSessionArrayRef;
        CFIndex     numSessions;
        CFIndex     index;
        
        // This is how we get the Raw TOC Data
        theRawTOCDataRef = (CFDataRef)CFDictionaryGetValue (dictRef, CFSTR(kRawTOCDataString));
        
        // Get the session array info.
        theSessionArrayRef = (CFArrayRef)CFDictionaryGetValue (dictRef, CFSTR(kSessionsString));
        
        // Find out how many sessions there are.
        numSessions = CFArrayGetCount (theSessionArrayRef);
        
        // Initialize the total number of tracks to 0
        theCD->numtracks = 0;
        
        // Iterate over all sessions, collecting the track data
        for(index = 0; index < numSessions; index++)
        {
            CFDictionaryRef theSessionDict;
            CFNumberRef     leadoutBlock;
            CFArrayRef      trackArray;
            CFIndex         numTracks;
            CFIndex         trackIndex;
            UInt32          value = 0;
            
            theSessionDict      = (CFDictionaryRef) CFArrayGetValueAtIndex (theSessionArrayRef, index);
            leadoutBlock        = (CFNumberRef) CFDictionaryGetValue (theSessionDict, CFSTR(kLeadoutBlockString));
            
            trackArray = (CFArrayRef)CFDictionaryGetValue (theSessionDict, CFSTR(kTrackArrayString));
            
            numTracks = CFArrayGetCount (trackArray);

            for(trackIndex = 0; trackIndex < numTracks; trackIndex++) {
                    
                CFDictionaryRef theTrackDict;
                CFNumberRef     trackNumber;
                CFNumberRef     sessionNumber;
                CFNumberRef     startBlock;
                CFBooleanRef    isDataTrack;
                UInt32          value;
                
                theTrackDict  = (CFDictionaryRef) CFArrayGetValueAtIndex (trackArray, trackIndex);
                
                trackNumber   = (CFNumberRef)  CFDictionaryGetValue (theTrackDict, CFSTR(kPointKeyString));
                sessionNumber = (CFNumberRef)  CFDictionaryGetValue (theTrackDict, CFSTR(kSessionNumberKeyString));
                startBlock    = (CFNumberRef)  CFDictionaryGetValue (theTrackDict, CFSTR(kStartBlockKeyString));
                isDataTrack   = (CFBooleanRef) CFDictionaryGetValue (theTrackDict, CFSTR(kDataKeyString));
                                                        
                // Fill in the SDL_CD struct
                int idx = theCD->numtracks++;

                CFNumberGetValue (trackNumber, kCFNumberSInt32Type, &value);
                theCD->track[idx].id = value;
                
                CFNumberGetValue (startBlock, kCFNumberSInt32Type, &value);
                theCD->track[idx].offset = value;

                theCD->track[idx].type = (isDataTrack == kCFBooleanTrue) ? SDL_DATA_TRACK : SDL_AUDIO_TRACK;

                // Since the track lengths are not stored in .TOC.plist we compute them.
                if (trackIndex > 0) {
                    theCD->track[idx-1].length = theCD->track[idx].offset - theCD->track[idx-1].offset;
                }
            }
            
            // Compute the length of the last track
            CFNumberGetValue (leadoutBlock, kCFNumberSInt32Type, &value);
            
            theCD->track[theCD->numtracks-1].length = 
                value - theCD->track[theCD->numtracks-1].offset;

            // Set offset to leadout track
            theCD->track[theCD->numtracks].offset = value;
        }
    
    }

    theErr = 0;
    goto cleanup;
bail:
    SDL_SetError ("ReadTOCData: %s returned %d", error, theErr);
    theErr = -1;
cleanup:

    if (propertyListRef != NULL)
        CFRelease(propertyListRef);
    if (dataRef != NULL)
        CFRelease(dataRef);
    if (forkData != NULL)
        DisposePtr(forkData);
        
    FSCloseFork (forkRefNum);

    return theErr;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  ListTrackFiles
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

int ListTrackFiles (FSVolumeRefNum theVolume, FSRef *trackFiles, int numTracks)
{
    OSStatus        result = -1;
    FSIterator      iterator;
    ItemCount       actualObjects;
    FSRef           rootDirectory;
    FSRef           ref;
    HFSUniStr255    nameStr;
    
    result = FSGetVolumeInfo (theVolume,
                              0,
                              NULL,
                              kFSVolInfoFSInfo,
                              NULL,
                              NULL,
                              &rootDirectory); 
                                 
    if (result != noErr) {
        SDL_SetError ("ListTrackFiles: FSGetVolumeInfo returned %d", result);
        return result;
    }

    result = FSOpenIterator (&rootDirectory, kFSIterateFlat, &iterator);
    if (result == noErr) {
        do
        {
            result = FSGetCatalogInfoBulk (iterator, 1, &actualObjects,
                                           NULL, kFSCatInfoNone, NULL, &ref, NULL, &nameStr);
            if (result == noErr) {
                
                CFStringRef  name;
                name = CFStringCreateWithCharacters (NULL, nameStr.unicode, nameStr.length);
                
                // Look for .aiff extension
                if (CFStringHasSuffix (name, CFSTR(".aiff")) ||
                    CFStringHasSuffix (name, CFSTR(".cdda"))) {
                    
                    // Extract the track id from the filename
                    int trackID = 0, i = 0;
                    while (i < nameStr.length && !isdigit(nameStr.unicode[i])) {
                        ++i;
                    }
                    while (i < nameStr.length && isdigit(nameStr.unicode[i])) {
                        trackID = 10 * trackID +(nameStr.unicode[i] - '0');
                        ++i;
                    }

                    #if DEBUG_CDROM
                    printf("Found AIFF for track %d: '%s'\n", trackID, 
                    CFStringGetCStringPtr (name, CFStringGetSystemEncoding()));
                    #endif
                    
                    // Track ID's start at 1, but we want to start at 0
                    trackID--;
                    
                    assert(0 <= trackID && trackID <= SDL_MAX_TRACKS);
                    
                    if (trackID < numTracks)
                        memcpy (&trackFiles[trackID], &ref, sizeof(FSRef));
                }
                CFRelease (name);
            }
        } while(noErr == result);
        FSCloseIterator (iterator);
    }
    
    return 0;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  LoadFile
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

int LoadFile (const FSRef *ref, int startFrame, int stopFrame)
{
    int error = -1;
    
    if (CheckInit () < 0)
        goto bail;
    
    // release any currently playing file
    if (ReleaseFile () < 0)
        goto bail;
    
    #if DEBUG_CDROM
    printf ("LoadFile: %d %d\n", startFrame, stopFrame);
    #endif
    
    try {
    
        // create a new player, and attach to the audio unit
        
        thePlayer = new AudioFilePlayer(ref);
        if (thePlayer == NULL) {
            SDL_SetError ("LoadFile: Could not create player");
            throw (-3);
        }
            
        thePlayer->SetDestination(theUnit, 0);
        
        if (startFrame >= 0)
            thePlayer->SetStartFrame (startFrame);
        
        if (stopFrame >= 0 && stopFrame > startFrame)
            thePlayer->SetStopFrame (stopFrame);
        
        // we set the notifier later
        //thePlayer->SetNotifier(FilePlayNotificationHandler, NULL);
            
        thePlayer->Connect();
    
        #if DEBUG_CDROM
        thePlayer->Print();
        fflush (stdout);
        #endif
    }
    catch (...)
    {
        goto bail;
    }
        
    error = 0;

    bail:
    return error;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  ReleaseFile
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

int ReleaseFile ()
{
    int error = -1;
        
    try {
        if (thePlayer != NULL) {
            
            thePlayer->Disconnect();
            
            delete thePlayer;
            
            thePlayer = NULL;
        }
    }
    catch (...)
    {
        goto bail;
    }
    
    error = 0;
    
  bail:
    return error;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  PlayFile
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

int PlayFile ()
{
    OSStatus result = -1;
    
    if (CheckInit () < 0)
        goto bail;
        
    try {
    
        // start processing of the audio unit
        result = AudioOutputUnitStart (theUnit);
            THROW_RESULT("PlayFile: AudioOutputUnitStart")    
        
    }
    catch (...)
    {
        goto bail;
    }
    
    result = 0;
    
bail:
    return result;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  PauseFile
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

int PauseFile ()
{
    OSStatus result = -1;
    
    if (CheckInit () < 0)
        goto bail;
            
    try {
    
        // stop processing the audio unit
        result = AudioOutputUnitStop (theUnit);
            THROW_RESULT("PauseFile: AudioOutputUnitStop")
    }
    catch (...)
    {
        goto bail;
    }
    
    result = 0;
bail:
    return result;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  SetCompletionProc
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void SetCompletionProc (CDPlayerCompletionProc proc, SDL_CD *cdrom)
{
    assert(thePlayer != NULL);

    theCDROM = cdrom;
    completionProc = proc;
    thePlayer->SetNotifier (FilePlayNotificationHandler, cdrom);
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//  GetCurrentFrame
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

int GetCurrentFrame ()
{    
    int frame;
    
    if (thePlayer == NULL)
        frame = 0;
    else
        frame = thePlayer->GetCurrentFrame ();
        
    return frame; 
}


#pragma mark -- Private Functions --

static OSStatus CheckInit ()
{    
    if (playBackWasInit)
        return 0;
    
    OSStatus result = noErr;
    
    // Create the callback semaphore
    callbackSem = SDL_CreateSemaphore(0);

    // Start callback thread
    SDL_CreateThread(RunCallBackThread, NULL);

    try {
        ComponentDescription desc;
    
        desc.componentType = kAudioUnitComponentType;
        desc.componentSubType = kAudioUnitSubType_Output;
        desc.componentManufacturer = kAudioUnitID_DefaultOutput;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        
        Component comp = FindNextComponent (NULL, &desc);
        if (comp == NULL) {
            SDL_SetError ("CheckInit: FindNextComponent returned NULL");
            throw(internalComponentErr);
        }
        
        result = OpenAComponent (comp, &theUnit);
            THROW_RESULT("CheckInit: OpenAComponent")
                    
        // you need to initialize the output unit before you set it as a destination
        result = AudioUnitInitialize (theUnit);
            THROW_RESULT("CheckInit: AudioUnitInitialize")
        
                    
        // In this case we first want to get the output format of the OutputUnit
        // Then we set that as the input format. Why?
        // So that only a single conversion process is done
        // when SetDestination is called it will get the input format of the
        // unit its supplying data to. This defaults to 44.1K, stereo, so if
        // the device is not that, then we lose a possibly rendering of data
        
        result = MatchAUFormats (theUnit, 0);
            THROW_RESULT("CheckInit: MatchAUFormats")
    
        playBackWasInit = true;
    }
    catch (...)
    {
        return -1;
    }
    
    return 0;
}


static OSStatus MatchAUFormats (AudioUnit theUnit, UInt32 theInputBus)
{
    AudioStreamBasicDescription theDesc;
    UInt32 size = sizeof (theDesc);
    OSStatus result = AudioUnitGetProperty (theUnit,
                                            kAudioUnitProperty_StreamFormat,
                                            kAudioUnitScope_Output,
                                            0,
                                            &theDesc,
                                            &size);
        THROW_RESULT("MatchAUFormats: AudioUnitGetProperty")

    result = AudioUnitSetProperty (theUnit,
                                   kAudioUnitProperty_StreamFormat,
                                   kAudioUnitScope_Input,
                                   theInputBus,
                                   &theDesc,
                                   size);
    
    return result;
}

static void FilePlayNotificationHandler(void * inRefCon, OSStatus inStatus)
{
    if (inStatus == kAudioFilePlay_FileIsFinished) {
    
        // notify non-CA thread to perform the callback
        SDL_SemPost(callbackSem);
        
    } else if (inStatus == kAudioFilePlayErr_FilePlayUnderrun) {
    
        SDL_SetError ("CDPlayer Notification: buffer underrun");
    } else if (inStatus == kAudioFilePlay_PlayerIsUninitialized) {
    
        SDL_SetError ("CDPlayer Notification: player is uninitialized");
    } else {
        
        SDL_SetError ("CDPlayer Notification: unknown error %ld", inStatus);
    }
}

static int RunCallBackThread (void *param)
{
    for (;;) {
    
	SDL_SemWait(callbackSem);

        if (completionProc && theCDROM) {
            #if DEBUG_CDROM
            printf ("callback!\n");
            #endif
            (*completionProc)(theCDROM);
        } else {
            #if DEBUG_CDROM
            printf ("callback?\n");
            #endif
        }
    }
    
    #if DEBUG_CDROM
    printf ("thread dying now...\n");
    #endif
    
    return 0;
}

}; // extern "C"
