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
int gAppAlive = 1;

static int sWindowWidth  = 320;
static int sWindowHeight = 480;
static int sDemoStopped  = 0;

static long _getTime(void){
	struct timeval  now;
	gettimeofday(&now, NULL);
	return (long)(now.tv_sec*1000 + now.tv_usec/1000);
}


/*******************************************************************************
                               Things used by libsdl
*******************************************************************************/
pthread_mutex_t mSDLRenderMutex;
pthread_cond_t mSDLRenderCondition;

EGLContext mContext;
EGLDisplay mDisplay;
EGLSurface mRead;
EGLSurface mDraw;
	
/*******************************************************************************
                      SDL thread
*******************************************************************************/
pthread_t mSDLThread = 0;

void* sdlThreadProc(void* args){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "Thread Entry");

	if(!eglMakeCurrent(mDisplay, mDraw, mRead, mContext)){
		__android_log_print(ANDROID_LOG_INFO, "SDL", "Couldn't make current: 0x%x", eglGetError());
		return NULL;
	}

	
	return (void *)SDL_main();
}
   
/*******************************************************************************
                      Initialize the graphics state
*******************************************************************************/
void Java_org_libsdl_android_TestRenderer_nativeInit( JNIEnv*  env )
{
	importGLInit();

	gAppAlive    = 1;
	sDemoStopped = 0;

	__android_log_print(ANDROID_LOG_INFO, "SDL", "Entry point");

	pthread_mutex_init(&mSDLRenderMutex, NULL);
	pthread_cond_init (&mSDLRenderCondition, NULL);

	//Get some egl stuff we need
	mContext = eglGetCurrentContext();
	mDisplay = eglGetCurrentDisplay();
	mRead = eglGetCurrentSurface(EGL_READ);
	mDraw = eglGetCurrentSurface(EGL_DRAW);

	//We need to abandon our context so SDL can have it
	if(!eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)){
		__android_log_print(ANDROID_LOG_INFO, "SDL", "Couldn't abandon context: 0x%x", eglGetError());
		return NULL;
	}

	//Spin up the SDL thread
	int r = pthread_create(&mSDLThread, NULL, sdlThreadProc, NULL);

	if(r != 0){
		__android_log_print(ANDROID_LOG_INFO, "SDL", "Couldn't spawn thread: %d", r);
	}else{		
		__android_log_print(ANDROID_LOG_INFO, "SDL", "Started SDL thread");
	}

}

/*******************************************************************************
                                 Resize
*******************************************************************************/
void Java_org_libsdl_android_TestRenderer_nativeResize( JNIEnv*  env, 
														jobject  thiz, 
														jint w,
														jint h )
{
	sWindowWidth  = w;
	sWindowHeight = h;
	__android_log_print(ANDROID_LOG_INFO, "SDL", "resize w=%d h=%d", w, h);

}

/*******************************************************************************
                         Finalize (ie: shutdown)
*******************************************************************************/
void Java_org_libsdl_android_TestRenderer_nativeDone( JNIEnv*  env )
{

	//shut down the app

	importGLDeinit();

	__android_log_print(ANDROID_LOG_INFO, "SDL", "Finalize");
}

/*******************************************************************************
                   Pause (ie: stop as soon as possible)
*******************************************************************************/
void Java_org_libsdl_android_TestGLSurfaceView_nativePause( JNIEnv*  env )
{
	sDemoStopped = !sDemoStopped;
	if (sDemoStopped) {
		//we paused
		__android_log_print(ANDROID_LOG_INFO, "SDL", "Pause");
	} else {
		//we resumed
		__android_log_print(ANDROID_LOG_INFO, "SDL", "Resume");
	}
}

/*******************************************************************************
                     Render the next frame
*******************************************************************************/

volatile int frames = 0;
volatile int startSDL = 0;

//eglSwapBuffers(mDisplay, mDraw);
	
void Java_org_libsdl_android_TestRenderer_nativeRender( JNIEnv*  env )
{    
	__android_log_print(ANDROID_LOG_INFO, "SDL", "JNI: BeginRender");

    //Let the SDL thread do an entire run
    int lastFrames = frames;
    startSDL = 1;

    //wait for it to finish
    while(lastFrames == frames){
        ;   
    }

	__android_log_print(ANDROID_LOG_INFO, "SDL", "JNI: EndRender");
}

void sdl_render(){

    //When we get here, we've accumulated a full frame

    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: BeginRender");
    
    frames++;

    while(startSDL == 0){
        ;
    }
    startSDL = 0;

    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL: EndRender");
}

