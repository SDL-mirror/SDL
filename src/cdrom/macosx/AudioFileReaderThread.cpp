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
// AudioFileManager.cpp
//
#include "AudioFilePlayer.h"
#include <mach/mach.h> //used for setting policy of thread
#include "CAGuard.h"
#include <pthread.h>

#include <list>

class FileReaderThread {
public:
    FileReaderThread ();

    CAGuard&                    GetGuard() { return mGuard; }
    
    void                        AddReader();
    
    void                        RemoveReader (AudioFileManager* inItem);
        
        // returns true if succeeded
    bool                        TryNextRead (AudioFileManager* inItem)
    {
        bool didLock = false;
        bool succeeded = false;
        if (mGuard.Try (didLock))
        {
            mFileData.push_back (inItem);
            mGuard.Notify();
            succeeded = true;

            if (didLock)
                mGuard.Unlock();
        }
                
        return succeeded;
    }   
    
    int     mThreadShouldDie;
    
private:
    typedef std::list<AudioFileManager*> FileData;

    CAGuard             mGuard;
    UInt32              mThreadPriority;
    
    int                 mNumReaders;    
    FileData            mFileData;


    void                        ReadNextChunk ();
    
    void                        StartFixedPriorityThread ();
    static UInt32               GetThreadBasePriority (pthread_t inThread);
    
    static void*                DiskReaderEntry (void *inRefCon);
};

FileReaderThread::FileReaderThread ()
      : mThreadPriority (62),
        mNumReaders (0)
{
}

void    FileReaderThread::AddReader()
{
    if (mNumReaders == 0)
    {
        mThreadShouldDie = false;
    
        StartFixedPriorityThread ();
    }
    mNumReaders++;
}

void    FileReaderThread::RemoveReader (AudioFileManager* inItem)
{
    if (mNumReaders > 0)
    {
        CAGuard::Locker fileReadLock (mGuard);
        
        mFileData.remove (inItem);
        
        if (--mNumReaders == 0) {
            mThreadShouldDie = true;
            mGuard.Notify(); // wake up thread so it will quit
            mGuard.Wait();   // wait for thread to die
        }
    }   
}

void    FileReaderThread::StartFixedPriorityThread ()
{
    pthread_attr_t      theThreadAttrs;
    pthread_t           pThread;
    
    OSStatus result = pthread_attr_init(&theThreadAttrs);
        THROW_RESULT("pthread_attr_init - Thread attributes could not be created.")
    
    result = pthread_attr_setdetachstate(&theThreadAttrs, PTHREAD_CREATE_DETACHED);
        THROW_RESULT("pthread_attr_setdetachstate - Thread attributes could not be detached.")
    
    result = pthread_create (&pThread, &theThreadAttrs, DiskReaderEntry, this);
        THROW_RESULT("pthread_create - Create and start the thread.")
    
    pthread_attr_destroy(&theThreadAttrs);
    
    // we've now created the thread and started it
    // we'll now set the priority of the thread to the nominated priority
    // and we'll also make the thread fixed
    thread_extended_policy_data_t       theFixedPolicy;
    thread_precedence_policy_data_t     thePrecedencePolicy;
    SInt32                              relativePriority;
    
    // make thread fixed
    theFixedPolicy.timeshare = false;   // set to true for a non-fixed thread
    result = thread_policy_set (pthread_mach_thread_np(pThread), THREAD_EXTENDED_POLICY, (thread_policy_t)&theFixedPolicy, THREAD_EXTENDED_POLICY_COUNT);
        THROW_RESULT("thread_policy - Couldn't set thread as fixed priority.")
    // set priority
    // precedency policy's "importance" value is relative to spawning thread's priority
    relativePriority = mThreadPriority - FileReaderThread::GetThreadBasePriority (pthread_self());
        
    thePrecedencePolicy.importance = relativePriority;
    result = thread_policy_set (pthread_mach_thread_np(pThread), THREAD_PRECEDENCE_POLICY, (thread_policy_t)&thePrecedencePolicy, THREAD_PRECEDENCE_POLICY_COUNT);
        THROW_RESULT("thread_policy - Couldn't set thread priority.")
}

UInt32  FileReaderThread::GetThreadBasePriority (pthread_t inThread)
{
    thread_basic_info_data_t            threadInfo;
    policy_info_data_t                  thePolicyInfo;
    unsigned int                        count;
    
    // get basic info
    count = THREAD_BASIC_INFO_COUNT;
    thread_info (pthread_mach_thread_np (inThread), THREAD_BASIC_INFO, (integer_t*)&threadInfo, &count);
    
    switch (threadInfo.policy) {
        case POLICY_TIMESHARE:
            count = POLICY_TIMESHARE_INFO_COUNT;
            thread_info(pthread_mach_thread_np (inThread), THREAD_SCHED_TIMESHARE_INFO, (integer_t*)&(thePolicyInfo.ts), &count);
            return thePolicyInfo.ts.base_priority;
            break;
            
        case POLICY_FIFO:
            count = POLICY_FIFO_INFO_COUNT;
            thread_info(pthread_mach_thread_np (inThread), THREAD_SCHED_FIFO_INFO, (integer_t*)&(thePolicyInfo.fifo), &count);
            if (thePolicyInfo.fifo.depressed) {
                return thePolicyInfo.fifo.depress_priority;
            } else {
                return thePolicyInfo.fifo.base_priority;
            }
            break;
            
        case POLICY_RR:
            count = POLICY_RR_INFO_COUNT;
            thread_info(pthread_mach_thread_np (inThread), THREAD_SCHED_RR_INFO, (integer_t*)&(thePolicyInfo.rr), &count);
            if (thePolicyInfo.rr.depressed) {
                return thePolicyInfo.rr.depress_priority;
            } else {
                return thePolicyInfo.rr.base_priority;
            }
            break;
    }
    
    return 0;
}

void    *FileReaderThread::DiskReaderEntry (void *inRefCon)
{
    FileReaderThread *This = (FileReaderThread *)inRefCon;
    This->ReadNextChunk();
    #if DEBUG
    printf ("finished with reading file\n");
    #endif
    
    return 0;
}

void    FileReaderThread::ReadNextChunk ()
{
    OSStatus result;
    UInt32  dataChunkSize;
    AudioFileManager* theItem = 0;

    for (;;) 
    {
        { // this is a scoped based lock
            CAGuard::Locker fileReadLock (mGuard);
            
            if (this->mThreadShouldDie) {
            
                mGuard.Notify();
                return;
            }
            
            if (mFileData.empty())
            {
                mGuard.Wait();
            }
                        
            // kill thread
            if (this->mThreadShouldDie) {
            
                mGuard.Notify();
                return;
            }

            theItem = mFileData.front();
            mFileData.pop_front();
        }
    
        if ((theItem->mFileLength - theItem->mReadFilePosition) < theItem->mChunkSize)
            dataChunkSize = theItem->mFileLength - theItem->mReadFilePosition;
        else
            dataChunkSize = theItem->mChunkSize;
        
            // this is the exit condition for the thread
        if (dataChunkSize <= 0) {
            theItem->mFinishedReadingData = true;
            continue;
        }
            // construct pointer
        char* writePtr = const_cast<char*>(theItem->GetFileBuffer() + 
                                (theItem->mWriteToFirstBuffer ? 0 : theItem->mChunkSize));
    
            // read data
        result = theItem->Read(writePtr, &dataChunkSize);
        if (result != noErr && result != eofErr) {
            theItem->GetParent().DoNotification(result);
            continue;
        }
        
        if (dataChunkSize != theItem->mChunkSize)
        {
            writePtr += dataChunkSize;

            // can't exit yet.. we still have to pass the partial buffer back
            memset (writePtr, 0, (theItem->mChunkSize - dataChunkSize));
        }
        
        theItem->mWriteToFirstBuffer = !theItem->mWriteToFirstBuffer;   // switch buffers
        
        if (result == eofErr)
            theItem->mReadFilePosition = theItem->mFileLength;
        else
            theItem->mReadFilePosition += dataChunkSize;        // increment count
    }
}


static FileReaderThread sReaderThread;

AudioFileManager::AudioFileManager (AudioFilePlayer &inParent, 
                                    SInt16          inForkRefNum, 
                                    SInt64          inFileLength,
                                    UInt32          inChunkSize)
    : mParent (inParent),
      mForkRefNum (inForkRefNum),
      mFileBuffer (0),
      mByteCounter (0),
      mLockUnsuccessful (false),
      mIsEngaged (false),

      mChunkSize (inChunkSize),
      mFileLength (inFileLength),
      mReadFilePosition (0),
      mWriteToFirstBuffer (false),
      mFinishedReadingData (false)

{
    mFileBuffer = (char*) malloc (mChunkSize * 2);
    FSGetForkPosition(mForkRefNum, &mAudioDataOffset);
    assert (mFileBuffer != NULL);
}

void    AudioFileManager::DoConnect ()
{
    if (!mIsEngaged)
    {
        //mReadFilePosition = 0;
        mFinishedReadingData = false;

        mNumTimesAskedSinceFinished = 0;
        mLockUnsuccessful = false;
        
        OSStatus result;
        UInt32 dataChunkSize;
        
        if ((mFileLength - mReadFilePosition) < mChunkSize)
            dataChunkSize = mFileLength - mReadFilePosition;
        else
            dataChunkSize = mChunkSize;
        
        result = Read(mFileBuffer, &dataChunkSize);
           THROW_RESULT("AudioFileManager::DoConnect(): Read")

        mReadFilePosition += dataChunkSize;
                
        mWriteToFirstBuffer = false;
        mReadFromFirstBuffer = true;

        sReaderThread.AddReader();
        
        mIsEngaged = true;
    }
    else
        throw static_cast<OSStatus>(-1); //thread has already been started
}

void    AudioFileManager::Disconnect ()
{
    if (mIsEngaged) 
    {
        sReaderThread.RemoveReader (this);
        mIsEngaged = false;
    }
}

OSStatus AudioFileManager::Read(char *buffer, UInt32 *len)
{
    return FSReadFork (mForkRefNum,
                       fsFromStart,
                       mReadFilePosition + mAudioDataOffset,
                       *len,
                       buffer,
                       len);
}

OSStatus AudioFileManager::GetFileData (void** inOutData, UInt32 *inOutDataSize)
{
    if (mFinishedReadingData) 
    {
        ++mNumTimesAskedSinceFinished;
        *inOutDataSize = 0;
        *inOutData = 0;
        return noErr;
    }
    
    if (mReadFromFirstBuffer == mWriteToFirstBuffer) {
        #if DEBUG
        printf ("* * * * * * * Can't keep up with reading file:%ld\n", mParent.GetBusNumber());
        #endif
        
        mParent.DoNotification (kAudioFilePlayErr_FilePlayUnderrun);
        *inOutDataSize = 0;
        *inOutData = 0;
    } else {
        *inOutDataSize = mChunkSize;
        *inOutData = mReadFromFirstBuffer ? mFileBuffer : (mFileBuffer + mChunkSize);
    }

    mLockUnsuccessful = !sReaderThread.TryNextRead (this);
    
    mReadFromFirstBuffer = !mReadFromFirstBuffer;

    return noErr;
}

void    AudioFileManager::AfterRender ()
{
    if (mNumTimesAskedSinceFinished > 0)
    {
        bool didLock = false;
        if (sReaderThread.GetGuard().Try (didLock)) {
            mParent.DoNotification (kAudioFilePlay_FileIsFinished);
            if (didLock)
                sReaderThread.GetGuard().Unlock();
        }
    }

    if (mLockUnsuccessful)
        mLockUnsuccessful = !sReaderThread.TryNextRead (this);
}

void    AudioFileManager::SetPosition (SInt64 pos)
{
    if (pos < 0 || pos >= mFileLength) {
        SDL_SetError ("AudioFileManager::SetPosition - position invalid: %d filelen=%d\n", 
            (unsigned int)pos, (unsigned int)mFileLength);
        pos = 0;
    }
        
    mReadFilePosition = pos;
}
    
void    AudioFileManager::SetEndOfFile (SInt64 pos)
{
    if (pos <= 0 || pos > mFileLength) {
        SDL_SetError ("AudioFileManager::SetEndOfFile - position beyond actual eof\n");
        pos = mFileLength;
    }
    
    mFileLength = pos;
}
