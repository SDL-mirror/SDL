/*******************************************************************************
                               Headers
*******************************************************************************/
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <pthread.h>

#include "importgl.h"
#include "egl.h"

/*******************************************************************************
                               Globals
*******************************************************************************/
static long _getTime(void){
	struct timeval  now;
	gettimeofday(&now, NULL);
	return (long)(now.tv_sec*1000 + now.tv_usec/1000);
}

JNIEnv* mEnv = NULL;
JavaVM* mVM = NULL;

//Main activity
jclass mActivityInstance;

//method signatures
jmethodID midCreateGLContext;
jmethodID midFlipBuffers;

extern "C" int SDL_main();
extern "C" int Android_OnKeyDown(int keycode);
extern "C" int Android_OnKeyUp(int keycode);

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/	

extern "C" void Java_org_libsdl_android_SDLActivity_nativeInit( JNIEnv*  env, jobject obj )
{    
	__android_log_print(ANDROID_LOG_INFO, "SDL", "JNI: NativeInit");

	mEnv = env;

    SDL_main();
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return result;
    }

    mEnv = env;

    __android_log_print(ANDROID_LOG_INFO, "SDL", "JNI: OnLoad");

    jclass cls = mEnv->FindClass ("org/libsdl/android/SDLActivity"); 
    mActivityInstance = cls;
    midCreateGLContext = mEnv->GetStaticMethodID(cls,"createGLContext","()V");
    midFlipBuffers = mEnv->GetStaticMethodID(cls,"flipBuffers","()V");

    if(!midCreateGLContext || !midFlipBuffers){
        __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: Bad mids\n");
    }else{
        __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: Good mids\n");
    }
    
    return JNI_VERSION_1_4;
}

extern "C" void Java_org_libsdl_android_SDLActivity_onNativeKeyDown(JNIEnv*  env, 
               jobject obj, jint keycode){
    
    int r = Android_OnKeyDown(keycode);
    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: native key down %d, %d\n", keycode, r);
}

extern "C" void Java_org_libsdl_android_SDLActivity_onNativeKeyUp(JNIEnv*  env, 
               jobject obj, jint keycode){
    
    int r = Android_OnKeyUp(keycode);
    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: native key up %d, %d\n", keycode, r);
}



/*******************************************************************************
                 Functions called by SDL
*******************************************************************************/
extern "C" void sdl_create_context(){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: sdl_create_context()\n");

    mEnv->CallStaticVoidMethod(mActivityInstance, midCreateGLContext ); 
    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: sdl_create_context() return\n");

   // exit(1);
}

extern "C" void sdl_render(){

    //When we get here, we've accumulated a full frame
   //__android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: sdl_render()");
    
    mEnv->CallStaticVoidMethod(mActivityInstance, midFlipBuffers ); 
}

