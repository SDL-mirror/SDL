/*******************************************************************************
                               Headers
*******************************************************************************/
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>
#include <stdint.h>

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
                      Initialize the graphics state
*******************************************************************************/
void Java_org_libsdl_android_TestRenderer_nativeInit( JNIEnv*  env )
{
	importGLInit();

	gAppAlive    = 1;
	sDemoStopped = 0;
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
}

/*******************************************************************************
                   Pause (ie: stop as soon as possible)
*******************************************************************************/
void Java_org_libsdl_android_TestGLSurfaceView_nativePause( JNIEnv*  env )
{
	sDemoStopped = !sDemoStopped;
	if (sDemoStopped) {
		//we paused
	} else {
		//we resumed
	}
}

/*******************************************************************************
                     Render the next frame
*******************************************************************************/
void Java_org_libsdl_android_TestRenderer_nativeRender( JNIEnv*  env )
{    
	//TODO: Render here
}
