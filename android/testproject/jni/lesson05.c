/*
 * This code was created by Jeff Molofee '99 
 * (ported to Linux/SDL by Ti Leggett '01)
 *
 * If you've found this code useful, please let me know.
 *
 * Visit Jeff at http://nehe.gamedev.net/
 * 
 * or for port-specific comments, questions, bugreports etc. 
 * email to leggett@eecs.tulane.edu
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <signal.h>

#include <android/log.h>


#ifdef ANDROID
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include "SDL.h"

/* screen width, height, and bit depth */
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 430
#define SCREEN_BPP     16

/* Define our booleans */
#define TRUE  1
#define FALSE 0

/* This is our SDL surface */
SDL_Surface *surface;

int rotation = 0;


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





/* function to release/destroy our resources and restoring the old desktop */
void Quit( int returnCode )
{
    /* clean up the window */
    SDL_Quit( );

    /* and exit appropriately */
    exit( returnCode );
}

/* function to reset our viewport after a window resize */
int resizeWindow( int width, int height )
{
    /* Height / width ration */
    GLfloat ratio;
 
    /* Protect against a divide by zero */
   if ( height == 0 )
	height = 1;

    ratio = ( GLfloat )width / ( GLfloat )height;

    /* Setup our viewport. */
    glViewport( 0, 0, ( GLsizei )width, ( GLsizei )height );

    /* change to the projection matrix and set our viewing volume. */
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );

    /* Set our perspective */
    gluPerspective( 45.0f, ratio, 0.1f, 100.0f );

    /* Make sure we're chaning the model view and not the projection */
    glMatrixMode( GL_MODELVIEW );

    /* Reset The View */
    glLoadIdentity( );

    return( TRUE );
}

/* function to handle key press events */
void handleKeyPress( SDL_keysym *keysym )
{
    switch ( keysym->sym )
	{
	case SDLK_ESCAPE:
	    /* ESC key was pressed */
	    Quit( 0 );
	    break;
	case SDLK_F1:
	    /* F1 key was pressed
	     * this toggles fullscreen mode
	     */
	    SDL_WM_ToggleFullScreen( surface );
	    break;
    case SDLK_LEFT:
        rotation -= 30;
        break;

    case SDLK_RIGHT:
        rotation += 30;
        break;
        
	default:
	    break;
	}

    __android_log_print(ANDROID_LOG_INFO, "SDL","Keycode: %d, %d, %d\n", keysym->sym, SDLK_LEFT, SDLK_RIGHT);

    return;
}

/* general OpenGL initialization function */
int initGL( GLvoid )
{

    /* Enable smooth shading */
    glShadeModel( GL_SMOOTH );

    /* Set the background black */
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

    /* Depth buffer setup */
    //glClearDepth( 1.0f );

    /* Enables Depth Testing */
    glEnable( GL_DEPTH_TEST );

    /* The Type Of Depth Test To Do */
    glDepthFunc( GL_LEQUAL );

    /* Really Nice Perspective Calculations */
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

    return( TRUE );
}

/* Here goes our drawing code */
int drawGLScene( GLvoid )
{
      
	static int Frames = 0;
	static int T0 = 0;
	
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glClearColorx(0,0,0,255);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.5f, 150);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

	//Camera
	gluLookAt(0,0,5, 0,0,0, 0,1,0);
			
	//Draw a triangle
	//glRotatef(iRot, 0, 1, 0);

	glRotatef( rotation, 0.0f, 1.0f, 0.0f );


	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);
	
	/* Rotate The Triangle On The Y axis ( NEW ) */
    //glRotatef( Frames % 360, 0.0f, 1.0f, 0.0f );

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

    /* Draw it to the screen */
    SDL_GL_SwapBuffers( );

    /* Gather our frames per second */
    Frames++;
    {
	GLint t = SDL_GetTicks();
	if (t - T0 >= 5000) {
	    GLfloat seconds = (t - T0) / 1000.0;
	    GLfloat fps = Frames / seconds;
	    __android_log_print(ANDROID_LOG_INFO, "SDL","%d frames in %g seconds = %g FPS\n", Frames, seconds, fps);
	    T0 = t;
	    Frames = 0;
	}
    }

    rotation++;

    return( TRUE );
}


struct
{
    SDL_AudioSpec spec;
    Uint8 *sound;               /* Pointer to wave data */
    Uint32 soundlen;            /* Length of wave data */
    int soundpos;               /* Current play position */
} wave;

void SDLCALL
fillerup(void *unused, Uint8 * stream, int len)
{
    __android_log_print(ANDROID_LOG_INFO, "SDL","FILLERUP\n");
    
    Uint8 *waveptr;
    int waveleft;

    /* Set up the pointers */
    waveptr = wave.sound + wave.soundpos;
    waveleft = wave.soundlen - wave.soundpos;

    /* Go! */
    while (waveleft <= len) {
        SDL_memcpy(stream, waveptr, waveleft);
        stream += waveleft;
        len -= waveleft;
        waveptr = wave.sound;
        waveleft = wave.soundlen;
        wave.soundpos = 0;
    }
    SDL_memcpy(stream, waveptr, len);
    wave.soundpos += len;
}

void testAudio(){

    const char *file = "/sdcard/sample.wav";

    /* Load the SDL library */
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        __android_log_print(ANDROID_LOG_INFO, "SDL","Couldn't initialize SDL Audio: %s\n", SDL_GetError());
        return;
    }else{
        __android_log_print(ANDROID_LOG_INFO, "SDL","Init audio ok\n");
    }

    /* Load the wave file into memory */
    if (SDL_LoadWAV(file, &wave.spec, &wave.sound, &wave.soundlen) == NULL) {
        __android_log_print(ANDROID_LOG_INFO, "SDL", "Couldn't load %s: %s\n", file, SDL_GetError());
        return;
    }

    wave.spec.callback = fillerup;

    __android_log_print(ANDROID_LOG_INFO, "SDL","Loaded: %d\n", wave.soundlen);


    /* Initialize fillerup() variables */
    if (SDL_OpenAudio(&wave.spec, NULL) < 0) {
        __android_log_print(ANDROID_LOG_INFO, "SDL", "Couldn't open audio: %s\n", SDL_GetError());
        SDL_FreeWAV(wave.sound);
        return;
    }

     __android_log_print(ANDROID_LOG_INFO, "SDL","Using audio driver: %s\n", SDL_GetCurrentAudioDriver());

    /* Let the audio run */
    SDL_PauseAudio(0);

     __android_log_print(ANDROID_LOG_INFO, "SDL","Playing\n");
    
    while (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING){
         //__android_log_print(ANDROID_LOG_INFO, "SDL","Still playing\n");
        SDL_Delay(100);
    }

     __android_log_print(ANDROID_LOG_INFO, "SDL","Closing down\n");

    /* Clean up on signal */
    SDL_CloseAudio();
    SDL_FreeWAV(wave.sound);
}

int SDL_main( int argc, char **argv )
{

	__android_log_print(ANDROID_LOG_INFO, "SDL","entry\n");

    /* Flags to pass to SDL_SetVideoMode */
    int videoFlags;
    /* main loop variable */
    int done = FALSE;
    /* used to collect events */
    SDL_Event event;
    /* this holds some info about our display */
    const SDL_VideoInfo *videoInfo;
    /* whether or not the window is active */
    int isActive = TRUE;

    /* initialize SDL */
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
	    __android_log_print(ANDROID_LOG_INFO, "SDL", "Video initialization failed: %s\n",
		     SDL_GetError( ) );
	    Quit( 1 );
	}

    /* Fetch the video info */
    videoInfo = SDL_GetVideoInfo( );

    if ( !videoInfo )
	{
	    __android_log_print(ANDROID_LOG_INFO, "SDL", "Video query failed: %s\n",
		     SDL_GetError( ) );
	    Quit( 1 );
	}

    /* the flags to pass to SDL_SetVideoMode */
    videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
    videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */
    videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
    videoFlags |= SDL_RESIZABLE;       /* Enable window resizing */

    /* This checks to see if surfaces can be stored in memory */
    if ( videoInfo->hw_available )
	videoFlags |= SDL_HWSURFACE;
    else
	videoFlags |= SDL_SWSURFACE;

    /* This checks if hardware blits can be done */
    if ( videoInfo->blit_hw )
	videoFlags |= SDL_HWACCEL;

    /* Sets up OpenGL double buffering */
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    /* get a SDL surface */
    surface = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,
				videoFlags );

    /* Verify there is a surface */
    if ( !surface )
	{
	    __android_log_print(ANDROID_LOG_INFO, "SDL",  "Video mode set failed: %s\n", SDL_GetError( ) );
	    Quit( 1 );
	}

	__android_log_print(ANDROID_LOG_INFO, "SDL","Made a video mode!\n");

    /* initialize OpenGL */
    initGL( );

    /* resize the initial window */
    resizeWindow( SCREEN_WIDTH, SCREEN_HEIGHT );


    testAudio();


    /* wait for events */ 
    while ( !done )
	{
	    /* handle the events in the queue */

	    while ( SDL_PollEvent( &event ) )
		{
		    switch( event.type )
			{
			case SDL_ACTIVEEVENT:
			    /* Something's happend with our focus
			     * If we lost focus or we are iconified, we
			     * shouldn't draw the screen
			     */
			    if ( event.active.gain == 0 )
				isActive = FALSE;
			    else
				isActive = TRUE;
			    break;			    
			case SDL_VIDEORESIZE:
			    /* handle resize event */
			    surface = SDL_SetVideoMode( event.resize.w,
							event.resize.h,
							16, videoFlags );
			    if ( !surface )
				{
				    __android_log_print(ANDROID_LOG_INFO, "SDL","Could not get a surface after resize: %s\n", SDL_GetError( ) );
				    Quit( 1 );
				}
			    resizeWindow( event.resize.w, event.resize.h );
			    break;
			case SDL_KEYDOWN:
			    /* handle key presses */
			    handleKeyPress( &event.key.keysym );
			    break;
			case SDL_QUIT:
			    /* handle quit requests */
			    done = TRUE;
			    __android_log_print(ANDROID_LOG_INFO, "SDL","App is shutting down\n");
			    break;
			default:
			    break;
			}
		}

	    /* draw the scene */
	    if ( isActive )
		drawGLScene( );
	}

    /* clean ourselves up and exit */
    Quit( 0 );

    /* Should never get here */
    return( 0 );
}


