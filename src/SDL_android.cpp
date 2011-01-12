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

extern "C" {
#include "events/SDL_events_c.h"
#include "video/android/SDL_androidkeyboard.h"
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
jmethodID midEnableFeature;
jmethodID midUpdateAudio;

extern "C" void Android_SetScreenResolution(int width, int height);
extern "C" int SDL_SendQuit();

//If we're not the active app, don't try to render
bool bRenderingEnabled = false;

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
    midEnableFeature = mEnv->GetStaticMethodID(cls,"enableFeature","(II)V");
    midUpdateAudio = mEnv->GetStaticMethodID(cls,"updateAudio","([B)V");

    if(!midCreateGLContext || !midFlipBuffers || !midEnableFeature ||
        !midUpdateAudio) {
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
    // Stop rendering as we're no longer in the foreground
    bRenderingEnabled = false;

    // Inject a SDL_QUIT event
    SDL_SendQuit();
}

// Screen size
extern "C" void Java_org_libsdl_app_SDLActivity_nativeSetScreenSize(
                JNIEnv*  env, jobject obj, jint width, jint height)
{
    __android_log_print(ANDROID_LOG_INFO, "SDL", 
                        "SDL: Set screen size on init: %d/%d\n", width, height);
    Android_SetScreenResolution(width, height);
}

// Resize
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeResize(
                                        JNIEnv*  env, jobject obj, jint width, 
                                        jint height, jint format)
{
    /* FIXME: What is the relationship between this and the window? */
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
extern "C" void Android_CreateContext()
{
    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: sdl_create_context()\n");

    bRenderingEnabled = true;

    mEnv->CallStaticVoidMethod(mActivityInstance, midCreateGLContext ); 
}

extern "C" void Android_Render()
{
    if (!bRenderingEnabled) {
        return;
    }

    // When we get here, we've accumulated a full frame    
    mEnv->CallStaticVoidMethod(mActivityInstance, midFlipBuffers); 
}

extern "C" void Android_EnableFeature(int featureid, bool enabled)
{
    mEnv->CallStaticVoidMethod(mActivityInstance, midEnableFeature, 
                                featureid, (int)enabled); 
}

extern "C" void Android_UpdateAudioBuffer(unsigned char *buf, int len)
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
