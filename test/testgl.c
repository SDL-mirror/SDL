#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "SDL.h"

#ifdef HAVE_OPENGL
#ifdef WIN32
#include <windows.h>
#endif
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#define SHADED_CUBE


void HotKey_ToggleFullScreen(void)
{
	SDL_Surface *screen;

	screen = SDL_GetVideoSurface();
	if ( SDL_WM_ToggleFullScreen(screen) ) {
		printf("Toggled fullscreen mode - now %s\n",
		    (screen->flags&SDL_FULLSCREEN) ? "fullscreen" : "windowed");
	} else {
		printf("Unable to toggle fullscreen mode\n");
	}
}

void HotKey_ToggleGrab(void)
{
	SDL_GrabMode mode;

	printf("Ctrl-G: toggling input grab!\n");
	mode = SDL_WM_GrabInput(SDL_GRAB_QUERY);
	if ( mode == SDL_GRAB_ON ) {
		printf("Grab was on\n");
	} else {
		printf("Grab was off\n");
	}
	mode = SDL_WM_GrabInput(!mode);
	if ( mode == SDL_GRAB_ON ) {
		printf("Grab is now on\n");
	} else {
		printf("Grab is now off\n");
	}
}

void HotKey_Iconify(void)
{
	printf("Ctrl-Z: iconifying window!\n");
	SDL_WM_IconifyWindow();
}

int HandleEvent(SDL_Event *event)
{
	int done;

	done = 0;
	switch( event->type ) {
	    case SDL_ACTIVEEVENT:
		/* See what happened */
		printf( "app %s ", event->active.gain ? "gained" : "lost" );
		if ( event->active.state & SDL_APPACTIVE ) {
			printf( "active " );
		} else if ( event->active.state & SDL_APPMOUSEFOCUS ) {
			printf( "mouse " );
		} else if ( event->active.state & SDL_APPINPUTFOCUS ) {
			printf( "input " );
		}
		printf( "focus\n" );
		break;
		

	    case SDL_KEYDOWN:
		if ( event->key.keysym.sym == SDLK_ESCAPE ) {
			done = 1;
		}
		if ( (event->key.keysym.sym == SDLK_g) &&
		     (event->key.keysym.mod & KMOD_CTRL) ) {
			HotKey_ToggleGrab();
		}
		if ( (event->key.keysym.sym == SDLK_z) &&
		     (event->key.keysym.mod & KMOD_CTRL) ) {
			HotKey_Iconify();
		}
		if ( (event->key.keysym.sym == SDLK_RETURN) &&
		     (event->key.keysym.mod & KMOD_ALT) ) {
			HotKey_ToggleFullScreen();
		}
		printf("key '%s' pressed\n", 
			SDL_GetKeyName(event->key.keysym.sym));
		break;
	    case SDL_QUIT:
		done = 1;
		break;
	}
	return(done);
}

void DrawSDLLogo(void)
{
	static SDL_Surface *image = NULL;
	static int x = 0;
	static int y = 0;
	static int delta_x = 1;
	static int delta_y = 1;
	static Uint32 last_moved = 0;

	SDL_Rect dst;
	SDL_Surface *screen;

	if ( image == NULL ) {
		SDL_Surface *temp;

		temp = SDL_LoadBMP("icon.bmp");
		if ( temp == NULL ) {
			return;
		}
		image = SDL_CreateRGBSurface(
				SDL_SWSURFACE,
				temp->w, temp->h,
				32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				0x000000FF, 
				0x0000FF00, 
				0x00FF0000, 
 			       0xFF000000
#else
 			       0xFF000000,
 			       0x00FF0000, 
 			       0x0000FF00, 
 			       0x000000FF
#endif
 			       );
		if ( image != NULL ) {
			SDL_BlitSurface(temp, NULL, image, NULL);
		}
		SDL_FreeSurface(temp);
		if ( image == NULL ) {
			return;
		}
	}

	screen = SDL_GetVideoSurface();

	/* Show the image on the screen */
	dst.x = x;
	dst.y = y;
	dst.w = image->w;
	dst.h = image->h;

	/* Move it around
           Note that we do not clear the old position.  This is because we
           perform a glClear() which clears the framebuffer and then only
           update the new area.
           Note that you can also achieve interesting effects by modifying
           the screen surface alpha channel.  It's set to 255 by default..
         */
	if ( (SDL_GetTicks() - last_moved) > 100 ) {
		x += delta_x;
		if ( x < 0 ) {
			x = 0;
			delta_x = -delta_x;
		} else
		if ( (x+image->w) > screen->w ) {
			x = screen->w-image->w;
			delta_x = -delta_x;
		}
		y += delta_y;
		if ( y < 0 ) {
			y = 0;
			delta_y = -delta_y;
		} else
		if ( (y+image->h) > screen->h ) {
			y = screen->h-image->h;
			delta_y = -delta_y;
		}
		SDL_BlitSurface(image, NULL, screen, &dst);
	}
	SDL_UpdateRects(screen, 1, &dst);
}

int RunGLTest( int argc, char* argv[],
               int logo, int slowly, int bpp, float gamma )
{
	int i;
	int rgb_size[3];
	int w = 640;
	int h = 480;
	int done = 0;
	int frames;
	Uint32 start_time, this_time;
        float color[8][3]= {{ 1.0,  1.0,  0.0}, 
			    { 1.0,  0.0,  0.0},
			    { 0.0,  0.0,  0.0},
			    { 0.0,  1.0,  0.0},
			    { 0.0,  1.0,  1.0},
			    { 1.0,  1.0,  1.0},
			    { 1.0,  0.0,  1.0},
			    { 0.0,  0.0,  1.0}};
	float cube[8][3]= {{ 0.5,  0.5, -0.5}, 
			   { 0.5, -0.5, -0.5},
			   {-0.5, -0.5, -0.5},
			   {-0.5,  0.5, -0.5},
			   {-0.5,  0.5,  0.5},
			   { 0.5,  0.5,  0.5},
			   { 0.5, -0.5,  0.5},
			   {-0.5, -0.5,  0.5}};
	Uint32 video_flags;
	int value;

	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		fprintf(stderr,"Couldn't initialize SDL: %s\n",SDL_GetError());
		exit( 1 );
	}

	/* See if we should detect the display depth */
	if ( bpp == 0 ) {
		if ( SDL_GetVideoInfo()->vfmt->BitsPerPixel <= 8 ) {
			bpp = 8;
		} else {
			bpp = 16;  /* More doesn't seem to work */
		}
	}

	/* Set the flags we want to use for setting the video mode */
	if ( logo ) {
		video_flags = SDL_OPENGLBLIT;
	} else {
		video_flags = SDL_OPENGL;
	}
	for ( i=1; argv[i]; ++i ) {
		if ( strcmp(argv[1], "-fullscreen") == 0 ) {
			video_flags |= SDL_FULLSCREEN;
		}
	}

	/* Initialize the display */
	switch (bpp) {
	    case 8:
		rgb_size[0] = 2;
		rgb_size[1] = 3;
		rgb_size[2] = 3;
		break;
	    case 15:
	    case 16:
		rgb_size[0] = 5;
		rgb_size[1] = 5;
		rgb_size[2] = 5;
		break;
            default:
		rgb_size[0] = 8;
		rgb_size[1] = 8;
		rgb_size[2] = 8;
		break;
	}
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, rgb_size[0] );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, rgb_size[1] );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, rgb_size[2] );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	if ( SDL_SetVideoMode( w, h, bpp, video_flags ) == NULL ) {
		fprintf(stderr, "Couldn't set GL mode: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}

	printf("Screen BPP: %d\n", SDL_GetVideoSurface()->format->BitsPerPixel);
	printf("\n");
	printf( "Vendor     : %s\n", glGetString( GL_VENDOR ) );
	printf( "Renderer   : %s\n", glGetString( GL_RENDERER ) );
	printf( "Version    : %s\n", glGetString( GL_VERSION ) );
	printf( "Extensions : %s\n", glGetString( GL_EXTENSIONS ) );
	printf("\n");

	SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &value );
	printf( "SDL_GL_RED_SIZE: requested %d, got %d\n", rgb_size[0],value);
	SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &value );
	printf( "SDL_GL_GREEN_SIZE: requested %d, got %d\n", rgb_size[1],value);
	SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &value );
	printf( "SDL_GL_BLUE_SIZE: requested %d, got %d\n", rgb_size[2],value);
	SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &value );
	printf( "SDL_GL_DEPTH_SIZE: requested %d, got %d\n", bpp, value );
	SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &value );
	printf( "SDL_GL_DOUBLEBUFFER: requested 1, got %d\n", value );

	/* Set the window manager title bar */
	SDL_WM_SetCaption( "SDL GL test", "testgl" );

	/* Set the gamma for the window */
	if ( gamma != 0.0 ) {
		SDL_SetGamma(gamma, gamma, gamma);
	}

	glViewport( 0, 0, w, h );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );

	glOrtho( -2.0, 2.0, -2.0, 2.0, -20.0, 20.0 );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );

	glEnable(GL_DEPTH_TEST);

	glDepthFunc(GL_LESS);

	glShadeModel(GL_SMOOTH);

	/* Loop until done. */
	start_time = SDL_GetTicks();
	frames = 0;
	while( !done ) {
		GLenum gl_error;
		char* sdl_error;
		SDL_Event event;

		/* Do our drawing, too. */
		glClearColor( 0.0, 0.0, 0.0, 1.0 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBegin( GL_QUADS );

#ifdef SHADED_CUBE
			glColor3fv(color[0]);
			glVertex3fv(cube[0]);
			glColor3fv(color[1]);
			glVertex3fv(cube[1]);
			glColor3fv(color[2]);
			glVertex3fv(cube[2]);
			glColor3fv(color[3]);
			glVertex3fv(cube[3]);
			
			glColor3fv(color[3]);
			glVertex3fv(cube[3]);
			glColor3fv(color[4]);
			glVertex3fv(cube[4]);
			glColor3fv(color[7]);
			glVertex3fv(cube[7]);
			glColor3fv(color[2]);
			glVertex3fv(cube[2]);
			
			glColor3fv(color[0]);
			glVertex3fv(cube[0]);
			glColor3fv(color[5]);
			glVertex3fv(cube[5]);
			glColor3fv(color[6]);
			glVertex3fv(cube[6]);
			glColor3fv(color[1]);
			glVertex3fv(cube[1]);
			
			glColor3fv(color[5]);
			glVertex3fv(cube[5]);
			glColor3fv(color[4]);
			glVertex3fv(cube[4]);
			glColor3fv(color[7]);
			glVertex3fv(cube[7]);
			glColor3fv(color[6]);
			glVertex3fv(cube[6]);

			glColor3fv(color[5]);
			glVertex3fv(cube[5]);
			glColor3fv(color[0]);
			glVertex3fv(cube[0]);
			glColor3fv(color[3]);
			glVertex3fv(cube[3]);
			glColor3fv(color[4]);
			glVertex3fv(cube[4]);

			glColor3fv(color[6]);
			glVertex3fv(cube[6]);
			glColor3fv(color[1]);
			glVertex3fv(cube[1]);
			glColor3fv(color[2]);
			glVertex3fv(cube[2]);
			glColor3fv(color[7]);
			glVertex3fv(cube[7]);
#else // flat cube
			glColor3f(1.0, 0.0, 0.0);
			glVertex3fv(cube[0]);
			glVertex3fv(cube[1]);
			glVertex3fv(cube[2]);
			glVertex3fv(cube[3]);
			
			glColor3f(0.0, 1.0, 0.0);
			glVertex3fv(cube[3]);
			glVertex3fv(cube[4]);
			glVertex3fv(cube[7]);
			glVertex3fv(cube[2]);
			
			glColor3f(0.0, 0.0, 1.0);
			glVertex3fv(cube[0]);
			glVertex3fv(cube[5]);
			glVertex3fv(cube[6]);
			glVertex3fv(cube[1]);
			
			glColor3f(0.0, 1.0, 1.0);
			glVertex3fv(cube[5]);
			glVertex3fv(cube[4]);
			glVertex3fv(cube[7]);
			glVertex3fv(cube[6]);

			glColor3f(1.0, 1.0, 0.0);
			glVertex3fv(cube[5]);
			glVertex3fv(cube[0]);
			glVertex3fv(cube[3]);
			glVertex3fv(cube[4]);

			glColor3f(1.0, 0.0, 1.0);
			glVertex3fv(cube[6]);
			glVertex3fv(cube[1]);
			glVertex3fv(cube[2]);
			glVertex3fv(cube[7]);
#endif /* SHADED_CUBE */

		glEnd( );
		
		glMatrixMode(GL_MODELVIEW);
		glRotatef(5.0, 1.0, 1.0, 1.0);

		/* Draw 2D logo onto the 3D display */
		if ( logo ) {
			DrawSDLLogo();
		}

		SDL_GL_SwapBuffers( );

		/* Check for error conditions. */
		gl_error = glGetError( );

		if( gl_error != GL_NO_ERROR ) {
			fprintf( stderr, "testgl: OpenGL error: %d\n", gl_error );
		}

		sdl_error = SDL_GetError( );

		if( sdl_error[0] != '\0' ) {
			fprintf(stderr, "testgl: SDL error '%s'\n", sdl_error);
			SDL_ClearError();
		}

		/* Allow the user to see what's happening */
		if ( slowly ) {
			SDL_Delay( 20 );
		}

		/* Check if there's a pending event. */
		while( SDL_PollEvent( &event ) ) {
			done = HandleEvent(&event);
		}
		++frames;
	}

	/* Print out the frames per second */
	this_time = SDL_GetTicks();
	if ( this_time != start_time ) {
		printf("%2.2f FPS\n",
			((float)frames/(this_time-start_time))*1000.0);
	}

	/* Destroy our GL context, etc. */
	SDL_Quit( );
	return(0);
}

int main(int argc, char *argv[])
{
	int i, logo;
	int numtests;
	int bpp = 0;
	int slowly;
	float gamma = 0.0;

	logo = 0;
	slowly = 0;
	numtests = 1;
	for ( i=1; argv[i]; ++i ) {
		if ( strcmp(argv[i], "-twice") == 0 ) {
			++numtests;
		}
		if ( strcmp(argv[i], "-logo") == 0 ) {
			logo = 1;
		}
		if ( strcmp(argv[i], "-slow") == 0 ) {
			slowly = 1;
		}
		if ( strcmp(argv[i], "-bpp") == 0 ) {
 		       bpp = atoi(argv[++i]);
		}
		if ( strcmp(argv[i], "-gamma") == 0 ) {
 		       gamma = (float)atof(argv[++i]);
		}
		if ( strncmp(argv[i], "-h", 2) == 0 ) {
 		       printf(
"Usage: %s [-twice] [-logo] [-slow] [-bpp n] [-gamma n]\n",
 			      argv[0]);
			exit(0);
		}
	}
	for ( i=0; i<numtests; ++i ) {
 	       RunGLTest(argc, argv, logo, slowly, bpp, gamma);
	}
	return 0;
}

#else /* HAVE_OPENGL */

int main(int argc, char *argv[])
{
	printf("No OpenGL support on this system\n");
	return 1;
}

#endif /* HAVE_OPENGL */
