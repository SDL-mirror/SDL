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
//  AudioFilePlayer.h
//
#ifndef __AudioFilePlayer_H__
#define __AudioFilePlayer_H__

#include <CoreServices/CoreServices.h>

#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

#include "SDL_error.h"

const char* AudioFilePlayerErrorStr (OSStatus error);

void ThrowResult (OSStatus result, const char *str);

#define THROW_RESULT(str)                                       \
    if (result) {                                               \
        ThrowResult (result, str);                              \
    }

typedef void (*AudioFilePlayNotifier)(void          *inRefCon,
                                    OSStatus        inStatus);

enum {
    kAudioFilePlayErr_FilePlayUnderrun = -10000,
    kAudioFilePlay_FileIsFinished = -10001,
    kAudioFilePlay_PlayerIsUninitialized = -10002
};


class AudioFileManager;

#pragma mark __________ AudioFilePlayer
class AudioFilePlayer
{
public:
    AudioFilePlayer (const FSRef    *inFileRef);
    
    ~AudioFilePlayer();

    void            SetDestination (AudioUnit                   &inDestUnit, 
                                int                             inBusNumber);
    
    void            SetNotifier (AudioFilePlayNotifier inNotifier, void *inRefCon)
    {
        mNotifier = inNotifier;
        mRefCon = inRefCon;
    }
    
    void            SetStartFrame (int frame); // seek in the file
    
    int             GetCurrentFrame (); // get the current frame position
    
    void            SetStopFrame (int frame);   // set limit in the file
    
    void            Connect();
    
    void            Disconnect();

    void            DoNotification (OSStatus inError) const;
    
    bool            IsConnected () const { return mConnected; }

    UInt32          GetBusNumber () const { return mBusNumber; }
    
    AudioUnit       GetDestUnit () const { return mPlayUnit; }
    
    AudioConverterRef   GetAudioConverter() const { return mConverter; }

#if DEBUG    
    void            Print() const 
    {
        CAShow (mAudioFileID);
        printf ("Destination Bus:%ld\n", GetBusNumber());
        printf ("Is 'aunt' unit:%s\n", (mIsAUNTUnit ? "true" : "false"));
        printf ("Is Connected:%s\n", (IsConnected() ? "true" : "false"));
        if (mConverter) CAShow (mConverter);
          printf ("- - - - - - - - - - - - - - \n");
    }
#endif

    const AudioStreamBasicDescription&      GetFileFormat() const { return mFileDescription; }
    
private:
    AudioUnit                       mPlayUnit;
    UInt32                          mBusNumber;
    AudioFileID                     mAudioFileID;
    
    AudioUnitInputCallback          mInputCallback;

    AudioStreamBasicDescription     mFileDescription;
    
    bool                            mConnected;
    bool                            mIsAUNTUnit;
    
    AudioFileManager*               mAudioFileManager;
    AudioConverterRef               mConverter;
    
    AudioFilePlayNotifier           mNotifier;
    void*                           mRefCon;
    
    int                             mStartFrame;
    
#pragma mark __________ Private_Methods
    
    void        OpenFile (const FSRef *inRef, SInt64& outFileSize);
};

#pragma mark __________ AudioFileManager
class AudioFileManager
{
public:
    AudioFileManager (AudioFilePlayer& inParent, AudioFileID inFile)
        : mParent (inParent),
          mAudioFileID (inFile),
          mFileBuffer (0),
          mByteCounter (0)
        {}
    
    virtual ~AudioFileManager();
    
    
    void                Connect (AudioConverterRef inConverter) 
    {
        mParentConverter = inConverter;
        DoConnect();
    }

        // this method should NOT be called by an object of this class
        // as it is called by the parent's Disconnect() method
    virtual void        Disconnect () {}

    const AudioFileID&  GetFileID() const { return mAudioFileID; }

    const char*         GetFileBuffer () { return mFileBuffer; }

    const AudioFilePlayer&  GetParent () const { return mParent; }
    
    virtual void        SetPosition (SInt64 pos) = 0;  // seek/rewind in the file
    
    virtual int         GetByteCounter () { return mByteCounter; } // return actual bytes streamed to audio hardware
    
    virtual void        SetEndOfFile (SInt64 pos) = 0;  // set the "EOF" (will behave just like it reached eof)
   
protected:
    AudioFilePlayer&            mParent;
    AudioConverterRef           mParentConverter;
    const AudioFileID           mAudioFileID;
    
    char*                       mFileBuffer;

    OSStatus            Render (AudioBuffer &ioData);

    int                         mByteCounter;
    
    virtual OSStatus    GetFileData (void** inOutData, UInt32 *inOutDataSize) = 0;
    
    virtual void        DoConnect () = 0;
        
    virtual void        AfterRender () = 0;

public:
    static OSStatus     FileInputProc (void                             *inRefCon, 
                                        AudioUnitRenderActionFlags      inActionFlags,
                                        const AudioTimeStamp            *inTimeStamp, 
                                        UInt32                          inBusNumber, 
                                        AudioBuffer                     *ioData);
    static OSStatus     ACInputProc (AudioConverterRef          inAudioConverter,
                                            UInt32*                     outDataSize,
                                            void**                      outData,
                                            void*                       inUserData);
};


#pragma mark __________ AudioFileReaderThread
class AudioFileReaderThread 
    : public AudioFileManager
{
public:
    const UInt32    mChunkSize;
    SInt64          mFileLength;
    SInt64          mReadFilePosition;
    bool            mWriteToFirstBuffer;
    bool            mFinishedReadingData;
    
    AudioFileReaderThread (AudioFilePlayer  &inParent, 
                            AudioFileID     &inFile, 
                            SInt64          inFileLength,
                            UInt32          inChunkSize);
    
    virtual void        Disconnect ();

    virtual void        SetPosition (SInt64 pos);  // seek/rewind in the file
    
    virtual void        SetEndOfFile (SInt64 pos);  // set the "EOF" (will behave just like it reached eof)
    
protected:
    virtual void        DoConnect ();

    virtual OSStatus    GetFileData (void** inOutData, UInt32 *inOutDataSize);

    virtual void        AfterRender ();

private:
    bool                        mReadFromFirstBuffer;
    bool                        mLockUnsuccessful;
    bool                        mIsEngaged;
    
    int                         mNumTimesAskedSinceFinished;
};


#endif
