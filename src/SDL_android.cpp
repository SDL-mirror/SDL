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
}

/*******************************************************************************
 This file links the Java side of Android with libsdl
*******************************************************************************/
#include <jni.h>
#include <android/log.h>


/*******************************************************************************
                               Globals
*******************************************************************************/
JavaVM* mVM = NULL;
JNIEnv* mEnv = NULL;
JNIEnv* mAudioThreadEnv = NULL; //See the note below for why this is necessary

//Main activity
jclass mActivityInstance;

//method signatures
jmethodID midCreateGLContext;
jmethodID midFlipBuffers;
jmethodID midUpdateAudio;

//Feature IDs
static const int FEATURE_AUDIO = 1;
static const int FEATURE_ACCEL = 2;

//Accelerometer data storage
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
    midUpdateAudio = mEnv->GetStaticMethodID(cls,"updateAudio","([B)V");

    if(!midCreateGLContext || !midFlipBuffers || !midUpdateAudio) {
        __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: Bad mids\n");
    } else {
#ifdef DEBUG
        __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: Good mids\n");
#endif
    }
}

// Keydown
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeKeyDown(JNIEnv* env, 
               jobject obj, jint keycode)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_INFO, "SDL", 
                        "SDL: native key down %d\n", keycode);
#endif
    Android_OnKeyDown(keycode);
}

// Keyup
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeKeyUp(JNIEnv* env, 
               jobject obj, jint keycode)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_INFO, "SDL", 
                        "SDL: native key up %d\n", keycode);
#endif
    Android_OnKeyUp(keycode);
}

// Touch
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeTouch(JNIEnv* env, 
               jobject obj, jint action, jfloat x, jfloat y, jfloat p)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_INFO, "SDL", 
                        "SDL: native touch event %d @ %f/%f, pressure %f\n", 
                        action, x, y, p);
#endif

    //TODO: Pass this off to the SDL multitouch stuff
}

// Quit
extern "C" void Java_org_libsdl_app_SDLActivity_nativeQuit( JNIEnv*  env, 
                                                                jobject obj )
{    
    // Inject a SDL_QUIT event
    SDL_SendQuit();
}

// Resize
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeResize(
                                        JNIEnv*  env, jobject obj, jint width, 
                                        jint height, jint format)
{
    Android_SetScreenResolution(width, height, format);
}

extern "C" void Java_org_libsdl_app_SDLActivity_onNativeAccel(
                                        JNIEnv*  env, jobject obj,
                                        jfloat x, jfloat y, jfloat z)
{
    fLastAccelerometer[0] = x;
    fLastAccelerometer[1] = y;
    fLastAccelerometer[2] = z;   
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

extern "C" void Android_JNI_UpdateAudioBuffer(unsigned char *buf, int len)
{
    //Annoyingly we can't just call into Java from any thread. Because the audio
    //callback is dispatched from the SDL audio thread (that wasn't made from
    //java, we have to do some magic here to let the JVM know about the thread.
    //Because everything it touches on the Java side is static anyway, it's 
    //not a big deal, just annoying.
    if(!mAudioThreadEnv) {
        __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: Need to set up audio thread env\n");

        mVM->AttachCurrentThread(&mAudioThreadEnv, NULL);

        __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: ok\n");
    }
    
    jbyteArray arr = mAudioThreadEnv->NewByteArray(len);

    //blah. We probably should rework this so we avoid the copy. 
    mAudioThreadEnv->SetByteArrayRegion(arr, 0, len, (jbyte *)buf);
    
    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: copied\n");

    mAudioThreadEnv->CallStaticVoidMethod(  mActivityInstance, 
                                            midUpdateAudio, arr );

    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: invoked\n");
    
}

/* vi: set ts=4 sw=4 expandtab: */
