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

    This file based on Apple sample code. We haven't changed the file name, 
    so if you want to see the original search for it on apple.com/developer
*/

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  AudioFilePlayer.cpp
//
#include "AudioFilePlayer.h"

extern const char* AudioFilePlayerErrorStr (OSStatus error)
{
    const char *str;
    
    switch (error) {
    case kAudioFileUnspecifiedError:               str = "wht?"; break;
    case kAudioFileUnsupportedFileTypeError:       str = "typ?"; break;
    case kAudioFileUnsupportedDataFormatError:     str = "fmt?"; break;
    case kAudioFileUnsupportedPropertyError:       str = "pty?"; break;
    case kAudioFileBadPropertySizeError:           str = "!siz"; break;
    case kAudioFileNotOptimizedError:              str = "optm"; break;
    case kAudioFilePermissionsError:               str = "prm?"; break;
    case kAudioFileFormatNameUnavailableError:     str = "nme?"; break;
    case kAudioFileInvalidChunkError:              str = "chk?"; break;
    case kAudioFileDoesNotAllow64BitDataSizeError: str = "off?"; break;
    default: str = "error unspecified";
    }
    
    return str;
}

void ThrowResult (OSStatus result, const char* str)
{
    SDL_SetError ("Error: %s %d (%s)",
                   str, result, AudioFilePlayerErrorStr(result));
    throw result;
}

#if DEBUG
void PrintStreamDesc (AudioStreamBasicDescription *inDesc)
{
    if (!inDesc) {
        printf ("Can't print a NULL desc!\n");
        return;
    }
    
    printf ("- - - - - - - - - - - - - - - - - - - -\n");
    printf ("  Sample Rate:%f\n", inDesc->mSampleRate);
    printf ("  Format ID:%s\n", (char*)&inDesc->mFormatID);
    printf ("  Format Flags:%lX\n", inDesc->mFormatFlags);
    printf ("  Bytes per Packet:%ld\n", inDesc->mBytesPerPacket);
    printf ("  Frames per Packet:%ld\n", inDesc->mFramesPerPacket);
    printf ("  Bytes per Frame:%ld\n", inDesc->mBytesPerFrame);
    printf ("  Channels per Frame:%ld\n", inDesc->mChannelsPerFrame);
    printf ("  Bits per Channel:%ld\n", inDesc->mBitsPerChannel);
    printf ("- - - - - - - - - - - - - - - - - - - -\n");
}
#endif

OSStatus    AudioFileManager::FileInputProc (void                       *inRefCon, 
                                             AudioUnitRenderActionFlags inActionFlags,
                                             const AudioTimeStamp       *inTimeStamp, 
                                             UInt32                     inBusNumber, 
                                             AudioBuffer                *ioData)
{
    AudioFileManager* THIS = (AudioFileManager*)inRefCon;
    return THIS->Render(*ioData);
}

OSStatus    AudioFileManager::Render (AudioBuffer &ioData)
{
    OSStatus result = AudioConverterFillBuffer(mParentConverter, 
                                    AudioFileManager::ACInputProc, 
                                    this, 
                                    &ioData.mDataByteSize, 
                                    ioData.mData);
    if (result) {
        SDL_SetError ("AudioConverterFillBuffer:%ld\n", result);
        mParent.DoNotification (result);
    } else {
        mByteCounter += ioData.mDataByteSize / 2;
        AfterRender();
    }
    return result;
}

OSStatus    AudioFileManager::ACInputProc (AudioConverterRef            inAudioConverter,
                                            UInt32*                     outDataSize,
                                            void**                      outData,
                                            void*                       inUserData)
{
    AudioFileManager* THIS = (AudioFileManager*)inUserData;
    return THIS->GetFileData(outData, outDataSize);
}

AudioFileManager::~AudioFileManager ()
{
    if (mFileBuffer) {
        free (mFileBuffer);
        mFileBuffer = 0;
    }
}

AudioFilePlayer::AudioFilePlayer (const FSRef           *inFileRef)
    : mConnected (false),
      mAudioFileManager (0),
      mConverter (0),
      mNotifier (0),
      mStartFrame (0)
{
    SInt64 fileDataSize  = 0;

    OpenFile (inFileRef, fileDataSize);
        
    // we want about a seconds worth of data for the buffer
    int secsBytes = UInt32 (mFileDescription.mSampleRate * mFileDescription.mBytesPerFrame);
    
#if DEBUG
    printf("File format:\n");
    PrintStreamDesc (&mFileDescription);
#endif
    
        //round to a 32K boundary
    //if ((secsBytes & 0xFFFF8000) > (128 * 1024))
        //secsBytes &= 0xFFFF8000;
    //else
        //secsBytes = (secsBytes + 0x7FFF) & 0xFFFF8000;
                    
    mAudioFileManager = new AudioFileReaderThread (*this, 
                                                mAudioFileID, 
                                                fileDataSize,
                                                secsBytes);
}

// you can put a rate scalar here to play the file faster or slower
// by multiplying the same rate by the desired factor 
// eg fileSampleRate * 2 -> twice as fast
// before you create the AudioConverter
void    AudioFilePlayer::SetDestination (AudioUnit              &inDestUnit, 
                                int                             inBusNumber)
{
    if (mConnected) throw static_cast<OSStatus>(-1); //can't set dest if already engaged
 
    mPlayUnit = inDestUnit;
    mBusNumber = inBusNumber;

    OSStatus result = noErr;
    
    if (mConverter) {
        result = AudioConverterDispose (mConverter);
            THROW_RESULT("AudioConverterDispose")
    }
    
    AudioStreamBasicDescription     destDesc;
    UInt32  size = sizeof (destDesc);
    result = AudioUnitGetProperty (inDestUnit,
                                   kAudioUnitProperty_StreamFormat,
                                   kAudioUnitScope_Input,
                                   inBusNumber,
                                   &destDesc,
                                   &size);
        THROW_RESULT("AudioUnitGetProperty")

#if DEBUG
    printf("Destination format:\n");
    PrintStreamDesc (&destDesc);
#endif

        //we can "down" cast a component instance to a component
    ComponentDescription desc;
    result = GetComponentInfo ((Component)inDestUnit, &desc, 0, 0, 0);
        THROW_RESULT("GetComponentInfo")
        
        // we're going to use this to know which convert routine to call
        // a v1 audio unit will have a type of 'aunt'
        // a v2 audio unit will have one of several different types.
    mIsAUNTUnit = (desc.componentType == kAudioUnitComponentType);
    
    if (!mIsAUNTUnit) {
        result = badComponentInstance;
        THROW_RESULT("BAD COMPONENT")
    }

    
    // HACK - the AIFF files on CDs are in little endian order!
    if (mFileDescription.mFormatFlags == 0xE)
        mFileDescription.mFormatFlags &= ~kAudioFormatFlagIsBigEndian;
    

    result = AudioConverterNew (&mFileDescription, &destDesc, &mConverter);
        THROW_RESULT("AudioConverterNew")


/*
    // if we have a mono source, we're going to copy each channel into
    // the destination's channel source...
    if (mFileDescription.mChannelsPerFrame == 1) {
    
        SInt32* channelMap = new SInt32 [destDesc.mChannelsPerFrame];
        for (unsigned int i = 0; i < destDesc.mChannelsPerFrame; ++i)
            channelMap[i] = 0; //set first channel to all output channels
            
        result = AudioConverterSetProperty(mConverter,
                            kAudioConverterChannelMap,
                            (sizeof(SInt32) * destDesc.mChannelsPerFrame),
                            channelMap);
            THROW_RESULT("AudioConverterSetProperty")
        
        delete [] channelMap;
    }
*/
    assert (mFileDescription.mChannelsPerFrame == 2);
    
#if 0
    // this uses the better quality SRC
    UInt32 srcID = kAudioUnitSRCAlgorithm_Polyphase;
    result = AudioConverterSetProperty(mConverter,
                    kAudioConverterSampleRateConverterAlgorithm, 
                    sizeof(srcID), 
                    &srcID);
        THROW_RESULT("AudioConverterSetProperty")
#endif
}

void    AudioFilePlayer::SetStartFrame (int frame)
{
    SInt64 position = frame * 2352;

    mStartFrame = frame;
    mAudioFileManager->SetPosition (position);
}

    
int    AudioFilePlayer::GetCurrentFrame ()
{
    return mStartFrame + (mAudioFileManager->GetByteCounter() / 2352);
}
    
void    AudioFilePlayer::SetStopFrame (int frame)
{
    SInt64 position  = frame * 2352;
    
    mAudioFileManager->SetEndOfFile (position);
}
    
AudioFilePlayer::~AudioFilePlayer()
{
    Disconnect();
        
    if (mAudioFileManager) {
        delete mAudioFileManager;
        mAudioFileManager = 0;
    }
    
    if (mAudioFileID) {
        ::AudioFileClose (mAudioFileID);
        mAudioFileID = 0;
    }

    if (mConverter) {
        AudioConverterDispose (mConverter);
        mConverter = 0;
    }
}

void    AudioFilePlayer::Connect()
{
#if DEBUG
    printf ("Connect:%x,%ld, engaged=%d\n", (int)mPlayUnit, mBusNumber, (mConnected ? 1 : 0));
#endif
    if (!mConnected)
    {           
        mAudioFileManager->Connect(mConverter);
                
        // set the render callback for the file data to be supplied to the sound converter AU
        if (mIsAUNTUnit) {
            mInputCallback.inputProc = AudioFileManager::FileInputProc;
            mInputCallback.inputProcRefCon = mAudioFileManager;

            OSStatus result = AudioUnitSetProperty (mPlayUnit, 
                                kAudioUnitProperty_SetInputCallback, 
                                kAudioUnitScope_Input, 
                                mBusNumber,
                                &mInputCallback, 
                                sizeof(mInputCallback));
            THROW_RESULT("AudioUnitSetProperty")
        }
        mConnected = true;
    }
}

// warning noted, now please go away ;-)
// #warning This should redirect the calling of notification code to some other thread
void    AudioFilePlayer::DoNotification (OSStatus inStatus) const
{
    AudioFilePlayer* THIS = const_cast<AudioFilePlayer*>(this);
        
    if (mNotifier) {
        (*mNotifier) (mRefCon, inStatus);
    } 
    
    else {
        SDL_SetError ("Notification posted with no notifier in place");
        
        if (inStatus == kAudioFilePlay_FileIsFinished)
            THIS->Disconnect();
        else if (inStatus != kAudioFilePlayErr_FilePlayUnderrun)
            THIS->Disconnect();
    }
}

void    AudioFilePlayer::Disconnect ()
{
#if DEBUG
    printf ("Disconnect:%x,%ld, engaged=%d\n", (int)mPlayUnit, mBusNumber, (mConnected ? 1 : 0));
#endif
    if (mConnected)
    {
        mConnected = false;
            
        if (mIsAUNTUnit) {
            mInputCallback.inputProc = 0;
            mInputCallback.inputProcRefCon = 0;
            OSStatus result = AudioUnitSetProperty (mPlayUnit, 
                                            kAudioUnitProperty_SetInputCallback, 
                                            kAudioUnitScope_Input, 
                                            mBusNumber,
                                            &mInputCallback, 
                                            sizeof(mInputCallback));
            if (result) 
                SDL_SetError ("AudioUnitSetProperty:RemoveInputCallback:%ld", result);

        }
        
        mAudioFileManager->Disconnect();
    }
}

void    AudioFilePlayer::OpenFile (const FSRef *inRef, SInt64& outFileDataSize)
{       
    OSStatus result = AudioFileOpen (inRef, fsRdPerm, 0, &mAudioFileID);
        THROW_RESULT("AudioFileOpen")
        
    UInt32 dataSize = sizeof(AudioStreamBasicDescription);
    result = AudioFileGetProperty (mAudioFileID, 
                            kAudioFilePropertyDataFormat, 
                            &dataSize, 
                            &mFileDescription);
        THROW_RESULT("AudioFileGetProperty")
    
    dataSize = sizeof (SInt64);
    result = AudioFileGetProperty (mAudioFileID, 
                            kAudioFilePropertyAudioDataByteCount, 
                            &dataSize, 
                            &outFileDataSize);
        THROW_RESULT("AudioFileGetProperty")
}