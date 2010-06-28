package org.libsdl.android;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.*;

import android.app.Activity;
import android.content.Context;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.os.Bundle;
import android.view.MotionEvent;
import android.util.Log;
import android.graphics.*;

import java.lang.*;


//http://www.mail-archive.com/android-beginners@googlegroups.com/msg01830.html

/*
In TestActivity::onResume() call SDL_Init
SDL_GL_CreateContext call SDLSurface::createSDLGLContext()
SDL_GL_FlipBuffers calls SDLSurface::flip()

*/



public class TestActivity extends Activity {

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mSurface = new SDLSurface(getApplication());
        setContentView(mSurface);
        SurfaceHolder holder = mSurface.getHolder();
        holder.setType(SurfaceHolder.SURFACE_TYPE_GPU); 
    }

    protected void onPause() {
        super.onPause();
    }

    protected void onResume() {
        super.onResume();

        //All set up. Start up SDL
       
       
    }

    private static SDLSurface mSurface;

    static {
        System.loadLibrary("sanangeles");
    }

    //C functions we call
    public static native void nativeInit();


    //Java functions called from C
    private static void createGLContext(){
        mSurface.initEGL();
    }

    public static void flipBuffers(){
        mSurface.flipBuffers();
    }
}

class SDLThread implements Runnable{
    public void run(){
        TestActivity.nativeInit();
    }
}

class SDLSurface extends SurfaceView implements SurfaceHolder.Callback{

    private EGLContext  mEGLContext;
    private EGLSurface  mEGLSurface;
    private EGLDisplay  mEGLDisplay;

    public void surfaceCreated(SurfaceHolder holder) {
        Log.v("SDL","Surface created"); 

        Thread runner = new Thread(new SDLThread(), "SDLThread"); // (1) Create a new thread.
		runner.start(); // (2) Start the thread     
        
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.v("SDL","Surface destroyed");
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
       
    }


    boolean initEGL(){
        Log.v("SDL","Starting up");

        try{

            // Get an EGL instance
            EGL10 egl = (EGL10)EGLContext.getEGL();

            // Get to the default display.
            EGLDisplay dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

            // We can now initialize EGL for that display
            int[] version = new int[2];
            egl.eglInitialize(dpy, version);

            // Specify a configuration for our opengl session
            // and grab the first configuration that matches is
            int[] configSpec = {
                    //EGL10.EGL_DEPTH_SIZE,   16,
                    EGL10.EGL_NONE
            };
            EGLConfig[] configs = new EGLConfig[1];
            int[] num_config = new int[1];
            egl.eglChooseConfig(dpy, configSpec, configs, 1, num_config);
            EGLConfig config = configs[0];

            // Create an OpenGL ES context. This must be done only once
            EGLContext ctx = egl.eglCreateContext(dpy, config, EGL10.EGL_NO_CONTEXT, null);

            // Create an EGL surface we can render into.
            EGLSurface surface = egl.eglCreateWindowSurface(dpy, config, this, null);

            // Before we can issue GL commands, we need to make sure
            // the context is current and bound to a surface.
            egl.eglMakeCurrent(dpy, surface, surface, ctx);

            mEGLContext = ctx;
            mEGLDisplay = dpy;
            mEGLSurface = surface;
        }catch(Exception e){
            Log.v("SDL", e + "");
        }

        Log.v("SDL","Done making!");

        return true;
    }

    public SDLSurface(Context context) {
        super(context);

        getHolder().addCallback(this);
      
    }

     public void onDraw(Canvas canvas) {

        
     }


    public void flipBuffers(){
        //Log.v("test","Draw!");

        try{
        
            EGL10 egl = (EGL10)EGLContext.getEGL();
            GL10 gl = (GL10)mEGLContext.getGL();

            egl.eglWaitNative(EGL10.EGL_NATIVE_RENDERABLE, null);

            //drawing here

            egl.eglWaitGL();

            egl.eglSwapBuffers(mEGLDisplay, mEGLSurface);

            
        }catch(Exception e){
            Log.v("SDL", e + "");
        }

    }

}


/*
class TestRenderer implements GLSurfaceView.Renderer {
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        nativeInit();
    }

    

    public void onSurfaceChanged(GL10 gl, int w, int h) {
        //gl.glViewport(0, 0, w, h);
        nativeResize(w, h);
    }

    public void onDrawFrame(GL10 gl) {
        nativeRender();
    }

    private static native void nativeInit();
    private static native void nativeResize(int w, int h);
    private static native void nativeRender();
    private static native void nativeDone();

}
*/
