/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#include "SDL_android.h"

extern "C" {
#include "events/SDL_events_c.h"
#include "video/android/SDL_androidkeyboard.h"
#include "video/android/SDL_androidvideo.h"

/* Impelemented in audio/android/SDL_androidaudio.c */
extern void Android_RunAudioThread();
} // C

/*******************************************************************************
 This file links the Java side of Android with libsdl
*******************************************************************************/
#include <jni.h>
#include <android/log.h>


/*******************************************************************************
                               Globals
*******************************************************************************/
static JavaVM* mVM = NULL;
static JNIEnv* mEnv = NULL;
static JNIEnv* mAudioEnv = NULL;

// Main activity
static jclass mActivityInstance;

// method signatures
static jmethodID midCreateGLContext;
static jmethodID midFlipBuffers;
static jmethodID midAudioInit;
static jmethodID midAudioWriteShortBuffer;
static jmethodID midAudioWriteByteBuffer;

// Accelerometer data storage
float fLastAccelerometer[3];


/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/

// Library init
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    mVM = vm;

    return JNI_VERSION_1_4;
}

// Called before SDL_main() to initialize JNI bindings
extern "C" void SDL_Android_Init(JNIEnv* env)
{
    mEnv = env;

    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL_Android_Init()");

    jclass cls = mEnv->FindClass ("org/libsdl/app/SDLActivity"); 
    mActivityInstance = cls;
    midCreateGLContext = mEnv->GetStaticMethodID(cls,"createGLContext","()V");
    midFlipBuffers = mEnv->GetStaticMethodID(cls,"flipBuffers","()V");
	midAudioInit = mEnv->GetStaticMethodID(cls, "audioInit", "(IZZI)Ljava/lang/Object;");
	midAudioWriteShortBuffer = mEnv->GetStaticMethodID(cls, "audioWriteShortBuffer", "([S)V");
	midAudioWriteByteBuffer = mEnv->GetStaticMethodID(cls, "audioWriteByteBuffer", "([B)V");

    if(!midCreateGLContext || !midFlipBuffers || !midAudioInit ||
       !midAudioWriteShortBuffer || !midAudioWriteByteBuffer) {
		__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL: Couldn't locate Java callbacks, check that they're named and typed correctly");
    }
}

// Resize
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeResize(
                                    JNIEnv* env, jobject obj,
                                    jint width, jint height, jint format)
{
    Android_SetScreenResolution(width, height, format);
}

// Keydown
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeKeyDown(
                                    JNIEnv* env, jobject obj, jint keycode)
{
    Android_OnKeyDown(keycode);
}

// Keyup
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeKeyUp(
                                    JNIEnv* env, jobject obj, jint keycode)
{
    Android_OnKeyUp(keycode);
}

// Touch
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeTouch(
                                    JNIEnv* env, jobject obj,
                                    jint action, jfloat x, jfloat y, jfloat p)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_INFO, "SDL", 
                        "SDL: native touch event %d @ %f/%f, pressure %f\n", 
                        action, x, y, p);
#endif

    //TODO: Pass this off to the SDL multitouch stuff
}

// Accelerometer
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeAccel(
                                    JNIEnv* env, jobject obj,
                                    jfloat x, jfloat y, jfloat z)
{
    fLastAccelerometer[0] = x;
    fLastAccelerometer[1] = y;
    fLastAccelerometer[2] = z;   
}

// Quit
extern "C" void Java_org_libsdl_app_SDLActivity_nativeQuit(
                                    JNIEnv* env, jobject obj)
{    
    // Inject a SDL_QUIT event
    SDL_SendQuit();
}

extern "C" void Java_org_libsdl_app_SDLActivity_nativeRunAudioThread(
                                    JNIEnv* env)
{
	mVM->AttachCurrentThread(&mAudioEnv, NULL);
	Android_RunAudioThread();
}


/*******************************************************************************
             Functions called by SDL into Java
*******************************************************************************/
extern "C" void Android_JNI_CreateContext()
{
    mEnv->CallStaticVoidMethod(mActivityInstance, midCreateGLContext); 
}

extern "C" void Android_JNI_SwapWindow()
{
    mEnv->CallStaticVoidMethod(mActivityInstance, midFlipBuffers); 
}

//
// Audio support
//
static jint audioBufferFrames = 0;
static bool audioBuffer16Bit = false;
static bool audioBufferStereo = false;

static jobject audioBuffer;
static void * audioPinnedBuffer;

extern "C" int Android_JNI_OpenAudioDevice(int sampleRate, int is16Bit, int channelCount, int desiredBufferFrames)
{
	__android_log_print(ANDROID_LOG_VERBOSE, "SDL", "SDL audio: opening device");
	audioBuffer16Bit = is16Bit;
	audioBufferStereo = channelCount > 1;

	audioBuffer = mEnv->CallStaticObjectMethod(mActivityInstance, midAudioInit, sampleRate, audioBuffer16Bit, audioBufferStereo, desiredBufferFrames);
	audioBuffer = mEnv->NewGlobalRef(audioBuffer);

	if (audioBuffer == NULL) {
		__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: didn't get back a good audio buffer!");
		return 0;
	}

	if (audioBufferStereo) {
		audioBufferFrames = mEnv->GetArrayLength((jshortArray)audioBuffer) / 2;
	} else {
		audioBufferFrames = mEnv->GetArrayLength((jbyteArray)audioBuffer);
	}

	return audioBufferFrames;
}

extern "C" void * Android_JNI_PinAudioBuffer()
{
	jboolean isCopy = JNI_FALSE;

	if (audioPinnedBuffer != NULL) {
		return audioPinnedBuffer;
	}

	if (audioBuffer16Bit) {
		audioPinnedBuffer = mAudioEnv->GetShortArrayElements((jshortArray)audioBuffer, &isCopy);
	} else {
		audioPinnedBuffer = mAudioEnv->GetByteArrayElements((jbyteArray)audioBuffer, &isCopy);
	}

	return audioPinnedBuffer;
}

extern "C" void Android_JNI_WriteAudioBufferAndUnpin()
{
	if (audioPinnedBuffer == NULL) {
		return;
	}

	if (audioBuffer16Bit) {
		mAudioEnv->ReleaseShortArrayElements((jshortArray)audioBuffer, (jshort *)audioPinnedBuffer, JNI_COMMIT);
		mAudioEnv->CallStaticVoidMethod(mActivityInstance, midAudioWriteShortBuffer, (jshortArray)audioBuffer);
	} else {
		mAudioEnv->ReleaseByteArrayElements((jbyteArray)audioBuffer, (jbyte *)audioPinnedBuffer, JNI_COMMIT);
		mAudioEnv->CallStaticVoidMethod(mActivityInstance, midAudioWriteByteBuffer, (jbyteArray)audioBuffer);
	}

	audioPinnedBuffer = NULL;
}

extern "C" void Android_JNI_CloseAudioDevice()
{
    if (audioBuffer) {
        mEnv->DeleteGlobalRef(audioBuffer);
        audioBuffer = NULL;
    }

	// TODO: Implement
}

/* vi: set ts=4 sw=4 expandtab: */
