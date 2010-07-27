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
jmethodID midEnableFeature;

extern "C" int SDL_main();
extern "C" int Android_OnKeyDown(int keycode);
extern "C" int Android_OnKeyUp(int keycode);
extern "C" void Android_SetScreenResolution(int width, int height);
extern "C" void Android_OnResize(int width, int height, int format);
extern "C" int SDL_SendQuit();
extern "C" void Android_EnableFeature(int featureid, bool enabled);

//If we're not the active app, don't try to render
bool bRenderingEnabled = false;

//Feature IDs
static const int FEATURE_SOUND = 1;
static const int FEATURE_ACCEL = 2;

//Accelerometer data storage
float fLastAccelerometer[3];

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/	

//Library init
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved){

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
    midEnableFeature = mEnv->GetStaticMethodID(cls,"enableFeature","(I, I)V");

    if(!midCreateGLContext || !midFlipBuffers || !midEnableFeature){
        __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: Bad mids\n");
    }else{
        __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: Good mids\n");
    }
    
    return JNI_VERSION_1_4;
}

//Start up the SDL app
extern "C" void Java_org_libsdl_android_SDLActivity_nativeInit( JNIEnv* env, 
                                                                jobject obj ){ 
                                                                   
	__android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: Native Init");

	mEnv = env;
	bRenderingEnabled = true;

	Android_EnableFeature(FEATURE_ACCEL, true);

    SDL_main();
}

//Keydown
extern "C" void Java_org_libsdl_android_SDLActivity_onNativeKeyDown(JNIEnv* env, 
               jobject obj, jint keycode){
    
    int r = Android_OnKeyDown(keycode);
    __android_log_print(ANDROID_LOG_INFO, "SDL", 
                        "SDL: native key down %d, %d\n", keycode, r);
                        
}

//Keyup
extern "C" void Java_org_libsdl_android_SDLActivity_onNativeKeyUp(JNIEnv* env, 
               jobject obj, jint keycode){
    
    int r = Android_OnKeyUp(keycode);
    __android_log_print(ANDROID_LOG_INFO, "SDL", 
                        "SDL: native key up %d, %d\n", keycode, r);
                        
}

//Touch
extern "C" void Java_org_libsdl_android_SDLActivity_onNativeTouch(JNIEnv* env, 
               jobject obj, jint action, jfloat x, jfloat y, jfloat p){

    __android_log_print(ANDROID_LOG_INFO, "SDL", 
                        "SDL: native touch event %d @ %f/%f, pressure %f\n", 
                        action, x, y, p);

    //TODO: Pass this off to the SDL multitouch stuff
                        
}

//Quit
extern "C" void Java_org_libsdl_android_SDLActivity_nativeQuit( JNIEnv*  env, 
                                                                jobject obj ){    

    //Stop rendering as we're no longer in the foreground
	bRenderingEnabled = false;

    //Inject a SDL_QUIT event
    int r = SDL_SendQuit();

    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: Native quit %d", r);        
}

//Screen size
extern "C" void Java_org_libsdl_android_SDLActivity_nativeSetScreenSize(
                JNIEnv*  env, jobject obj, jint width, jint height){

    __android_log_print(ANDROID_LOG_INFO, "SDL", 
                        "SDL: Set screen size on init: %d/%d\n", width, height);
    Android_SetScreenResolution(width, height);
                        
}

//Resize
extern "C" void Java_org_libsdl_android_SDLActivity_onNativeResize(
                                        JNIEnv*  env, jobject obj, jint width, 
                                        jint height, jint format){
    Android_OnResize(width, height, format);
}

extern "C" void Java_org_libsdl_android_SDLActivity_onNativeAccel(
                                        JNIEnv*  env, jobject obj,
                                        jfloat x, jfloat y, jfloat z){
    fLastAccelerometer[0] = x;
    fLastAccelerometer[1] = y;
    fLastAccelerometer[2] = z;   
}



/*******************************************************************************
             Functions called by SDL into Java
*******************************************************************************/
extern "C" void Android_CreateContext(){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: sdl_create_context()\n");

	bRenderingEnabled = true;

    mEnv->CallStaticVoidMethod(mActivityInstance, midCreateGLContext ); 
}

extern "C" void Android_Render(){

    if(!bRenderingEnabled){
        return;
    }

    //When we get here, we've accumulated a full frame    
    mEnv->CallStaticVoidMethod(mActivityInstance, midFlipBuffers ); 
}

extern "C" void Android_EnableFeature(int featureid, bool enabled){

    mEnv->CallStaticVoidMethod(mActivityInstance, midFlipBuffers, 
                                featureid, (int)enabled); 
}

