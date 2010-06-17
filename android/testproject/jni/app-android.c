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

/**************************************
	gluperspective implementation
**************************************/
void gluPerspective(double fovy, double aspect, double zNear, double zFar){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double xmin, xmax, ymin, ymax;
	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;
	glFrustumf(xmin, xmax, ymin, ymax, zNear, zFar);
}


/**************************************
	  glulookat implementation
**************************************/
void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
          GLfloat centerx, GLfloat centery, GLfloat centerz,
          GLfloat upx, GLfloat upy, GLfloat upz)
{
    GLfloat m[16];
    GLfloat x[3], y[3], z[3];
    GLfloat mag;
    
    /* Make rotation matrix */
    
    /* Z vector */
    z[0] = eyex - centerx;
    z[1] = eyey - centery;
    z[2] = eyez - centerz;
    mag = sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    if (mag) {          /* mpichler, 19950515 */
        z[0] /= mag;
        z[1] /= mag;
        z[2] /= mag;
    }
    
    /* Y vector */
    y[0] = upx;
    y[1] = upy;
    y[2] = upz;
    
    /* X vector = Y cross Z */
    x[0] = y[1] * z[2] - y[2] * z[1];
    x[1] = -y[0] * z[2] + y[2] * z[0];
    x[2] = y[0] * z[1] - y[1] * z[0];
    
    /* Recompute Y = Z cross X */
    y[0] = z[1] * x[2] - z[2] * x[1];
    y[1] = -z[0] * x[2] + z[2] * x[0];
    y[2] = z[0] * x[1] - z[1] * x[0];
    
    /* mpichler, 19950515 */
    /* cross product gives area of parallelogram, which is < 1.0 for
     * non-perpendicular unit-length vectors; so normalize x, y here
     */
    
    mag = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    if (mag) {
        x[0] /= mag;
        x[1] /= mag;
        x[2] /= mag;
    }
    
    mag = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    if (mag) {
        y[0] /= mag;
        y[1] /= mag;
        y[2] /= mag;
    }
    
#define M(row,col)  m[col*4+row]
    M(0, 0) = x[0];
    M(0, 1) = x[1];
    M(0, 2) = x[2];
    M(0, 3) = 0.0;
    M(1, 0) = y[0];
    M(1, 1) = y[1];
    M(1, 2) = y[2];
    M(1, 3) = 0.0;
    M(2, 0) = z[0];
    M(2, 1) = z[1];
    M(2, 2) = z[2];
    M(2, 3) = 0.0;
    M(3, 0) = 0.0;
    M(3, 1) = 0.0;
    M(3, 2) = 0.0;
    M(3, 3) = 1.0;
#undef M
    glMultMatrixf(m);
    
    /* Translate Eye to Origin */
    glTranslatef(-eyex, -eyey, -eyez);
    
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


	 /* Enable smooth shading */
    glShadeModel( GL_SMOOTH );

    /* Set the background black */
    glClearColor( 1.0f, 0.0f, 0.0f, 0.0f );

    /* Depth buffer setup */
    //glClearDepth( 1.0f );

    /* Enables Depth Testing */
    glEnable( GL_DEPTH_TEST );

    /* The Type Of Depth Test To Do */
    glDepthFunc( GL_LEQUAL );

    /* Really Nice Perspective Calculations */
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );


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



	 /* Height / width ration */
    GLfloat ratio;
 
    /* Protect against a divide by zero */
   if ( h == 0 )
	h = 1;

    ratio = ( GLfloat )w / ( GLfloat )h;

    /* Setup our viewport. */
    glViewport( 0, 0, ( GLsizei )w, ( GLsizei )h );

    /* change to the projection matrix and set our viewing volume. */
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );

    /* Set our perspective */
    gluPerspective( 45.0f, ratio, 0.1f, 100.0f );

    /* Make sure we're chaning the model view and not the projection */
    glMatrixMode( GL_MODELVIEW );

    /* Reset The View */
    glLoadIdentity( );


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

const GLbyte vertex []=
{
	0,1,0,
	-1,0,0,
	1,0,0
};

const GLubyte color []=
{
	255,0,0,
	0,255,0,
	0,0,255
};

int iRot = 0;
int Frames = 0;


static void prepareFrame(int width, int height)
{
    glViewport(0, 0, width, height);

    glClearColorx(0,0,0,255);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (float)width / height, 0.5f, 150);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
}

void Java_org_libsdl_android_TestRenderer_nativeRender( JNIEnv*  env )
{    
	//TODO: Render here

	prepareFrame(sWindowWidth, sWindowHeight);
	
	//Camera
	gluLookAt(0,0,5, 0,0,0, 0,1,0);
			
	//Draw a triangle
	//glRotatef(iRot, 0, 1, 0);

	glRotatef( Frames % 360, 0.0f, 1.0f, 0.0f );


	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);
	
	/* Rotate The Triangle On The Y axis ( NEW ) */
    glRotatef( Frames % 360, 0.0f, 1.0f, 0.0f );

    /* GLES variant of drawing a triangle */
    const GLfloat triVertices[][9] = {
      {     /* Front Triangle */
	 0.0f,  1.0f,  0.0f,               /* Top Of Triangle               */
	-1.0f, -1.0f,  1.0f,               /* Left Of Triangle              */
	 1.0f, -1.0f,  1.0f                /* Right Of Triangle             */
      }, {  /* Right Triangle */
	 0.0f,  1.0f,  0.0f,               /* Top Of Triangle               */
	 1.0f, -1.0f,  1.0f,               /* Left Of Triangle              */
	 1.0f, -1.0f, -1.0f                /* Right Of Triangle             */
      }, {  /* Back Triangle */
	 0.0f,  1.0f,  0.0f,               /* Top Of Triangle               */
	 1.0f, -1.0f, -1.0f,               /* Left Of Triangle              */
	-1.0f, -1.0f, -1.0f                /* Right Of Triangle             */
      }, {  /* Left Triangle */
	 0.0f,  1.0f,  0.0f,               /* Top Of Triangle               */
	-1.0f, -1.0f, -1.0f,               /* Left Of Triangle              */
	-1.0f, -1.0f,  1.0f                /* Right Of Triangle             */
      }
    };

    /* unlike GL, GLES does not support RGB. We have to use RGBA instead */
    const GLfloat triColors[][12] = {
      {     /* Front triangle */
        1.0f, 0.0f, 0.0f, 1.0f,            /* Red                           */
	0.0f, 1.0f, 0.0f, 1.0f,            /* Green                         */
	0.0f, 0.0f, 1.0f, 1.0f             /* Blue                          */
      }, {  /* Right triangle */
        1.0f, 0.0f, 0.0f, 1.0f,            /* Red                           */
	0.0f, 0.0f, 1.0f, 1.0f,            /* Blue                          */
	0.0f, 1.0f, 0.0f, 1.0f             /* Green                         */
      }, {  /* Back triangle */
        1.0f, 0.0f, 0.0f, 1.0f,            /* Red                           */
	0.0f, 1.0f, 0.0f, 1.0f,            /* Green                         */
	0.0f, 0.0f, 1.0f, 1.0f             /* Blue                          */
      }, {  /* Left triangle */
        1.0f, 0.0f, 0.0f, 1.0f,            /* Red                           */
	0.0f, 0.0f, 1.0f, 1.0f,            /* Blue                          */
	0.0f, 1.0f, 0.0f, 1.0f             /* Green                         */
      }
    };

    glEnableClientState(GL_COLOR_ARRAY);

    int tri=0;

    /* Loop through all Triangles */
    for(tri=0;tri<sizeof(triVertices)/(9*sizeof(GLfloat));tri++) 
    {
      glVertexPointer(3, GL_FLOAT, 0, triVertices[tri]);
      glColorPointer(4, GL_FLOAT, 0, triColors[tri]);
      
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
    }
		
    //__android_log_print(ANDROID_LOG_INFO, "SDL", "render %d", Frames++);

    Frames++;

}
