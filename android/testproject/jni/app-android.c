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
                      SDL thread
*******************************************************************************/
pthread_t mSDLThread = 0;

void* sdlThreadProc(void* args){
	__android_log_print(ANDROID_LOG_INFO, "SDL", "Thread Entry");
	return 0;
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

void Java_org_libsdl_android_TestRenderer_nativeRender( JNIEnv*  env )
{    
	//TODO: Render here

}
