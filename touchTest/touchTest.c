#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <SDL_touch.h>



#define PI 3.1415926535897
#define WIDTH 640
#define HEIGHT 480
#define BPP 4
#define DEPTH 32

#define MAXFINGERS 5

int mousx,mousy;
int keystat[512];
int bstatus;




typedef struct {
  float x,y;
} Point;

typedef struct {
  Point p;
  float pressure;
} Finger;


Finger finger[MAXFINGERS];

void handler (int sig)
{
  printf ("exiting...(%d)\n", sig);
  exit (0);
}

void perror_exit (char *error)
{
  perror (error);
  handler (9);
}


void setpix(SDL_Surface *screen, int x, int y, int col)
{
  Uint32 *pixmem32;
  Uint32 colour;
  
  if((unsigned)x > screen->w) return;
  if((unsigned)y > screen->h) return;

  colour = SDL_MapRGB( screen->format, (col>>16)&0xFF, (col>>8)&0xFF, col&0xFF);
  
  pixmem32 = (Uint32*) screen->pixels  + y*screen->pitch/screen->format->BytesPerPixel + x; //TODO : Check this. May cause crash.
  *pixmem32 = colour;
}

void drawCircle(SDL_Surface* screen,int x,int y,int r,int c)
{

  float a;
  int tx;
  for(a=0;a<PI/2;a+=1.f/(float)abs(r))
  {
    if(r > 0) { //r<0 ==> unfilled circle
      for(tx=x-r*cos(a);tx<x+r*cos(a);tx++) {
	setpix(screen,tx,(int)(y+r*sin(a)),c);
	setpix(screen,tx,(int)(y-r*sin(a)),c);
      }
    }
    
    //Always Draw Outline
    setpix(screen,(int)(x+r*cos(a)),(int)(y+r*sin(a)),c);
    setpix(screen,(int)(x-r*cos(a)),(int)(y+r*sin(a)),c);
    setpix(screen,(int)(x+r*cos(a)),(int)(y-r*sin(a)),c);
    setpix(screen,(int)(x-r*cos(a)),(int)(y-r*sin(a)),c);
  }
}

void DrawScreen(SDL_Surface* screen, int h)
{
  int x, y, xm,ym,c,i;
  if(SDL_MUSTLOCK(screen))
    {                                              
      if(SDL_LockSurface(screen) < 0) return;
    }
  for(y = 0; y < screen->h; y++ )
    {
      for( x = 0; x < screen->w; x++ )
        {
	  //setpixel(screen, x, y, (x*x)/256+3*y+h, (y*y)/256+x+h, h);
	  //xm = (x+h)%screen->w;
	  //ym = (y+h)%screen->w;
	  //c = sin(h/256*2*PI)*x*y/screen->w/screen->h;
	  //setpix(screen,x,y,255*sin(xm/screen->w*2*PI),sin(h/255*2*PI)*255*y/screen->h,c);
	  setpix(screen,x,y,((x%255)<<16) + ((y%255)<<8) + (x+y)%255);
        }
    }

  drawCircle(screen,mousx,mousy,-30,0xFFFFFF);
  
  
  for(i=0;i<MAXFINGERS;i++)
    if(finger[i].p.x >= 0 && finger[i].p.y >= 0)
      if(finger[i].pressure > 0)
	drawCircle(screen,finger[i].p.x*screen->w,finger[i].p.y*screen->h
		   ,20,0xFF*finger[i].pressure);
      else
	drawCircle(screen,finger[i].p.x*screen->w,finger[i].p.y*screen->h
		   ,20,0xFF);
  
  if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
  
  SDL_Flip(screen);
}

SDL_Surface* initScreen(int width,int height)
{
  return SDL_SetVideoMode(width, height, DEPTH,
			  SDL_HWSURFACE | SDL_RESIZABLE);
}

int main(int argc, char* argv[])
{  
  SDL_Surface *screen;
  SDL_Event event;
  
  int keypress = 0;
  int h=0,s=1,i,j;

  memset(keystat,0,512*sizeof(keystat[0]));
  if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;
  screen = initScreen(WIDTH,HEIGHT);
  if (!screen)
    {
      SDL_Quit();
      return 1;
    }

  while(!keystat[27]) {
    //Poll SDL
    while(SDL_PollEvent(&event)) 
      {
	switch (event.type) 
	  {
	  case SDL_QUIT:
	    keystat[27] = 1;
	    break;
	  case SDL_KEYDOWN:
	    //printf("%i\n",event.key.keysym.sym);
	    keystat[event.key.keysym.sym] = 1;
	    //keypress = 1;
	    break;
	  case SDL_KEYUP:
	      //printf("%i\n",event.key.keysym.sym);
	    keystat[event.key.keysym.sym] = 0;
	    //keypress = 1;
	    break;
	  case SDL_VIDEORESIZE:
	    if (!(screen = initScreen(event.resize.w,
				      event.resize.h)))
	      {
		SDL_Quit();
		return 1;
	      }
	    break;
	  case SDL_MOUSEMOTION:
	    mousx = event.motion.x;
	    mousy = event.motion.y; 
	    break;
	  case SDL_MOUSEBUTTONDOWN:
	    bstatus |=  (1<<(event.button.button-1));
	    break;
	  case SDL_MOUSEBUTTONUP:
	    bstatus &= ~(1<<(event.button.button-1));
	    break;
	  case SDL_FINGERMOTION:    
	    ;
	    //printf("Finger: %i,x: %i, y: %i\n",event.tfinger.fingerId,
	    //	   event.tfinger.x,event.tfinger.y);
	    //SDL_Touch *inTouch = SDL_GetTouch(event.tfinger.touchId);
	    //SDL_Finger *inFinger = SDL_GetFinger(inTouch,event.tfinger.fingerId);
	    /*
	    finger[event.tfinger.fingerId].p.x = ((float)event.tfinger.x)/
	      inTouch->xres;
	    finger[event.tfinger.fingerId].p.y = ((float)event.tfinger.y)/
	      inTouch->yres;

	    finger[event.tfinger.fingerId].pressure = 
	      ((float)event.tfinger.pressure)/inTouch->pressureres;*/
		/*
	    printf("Finger: %i, Pressure: %f Pressureres: %i\n",
		   event.tfinger.fingerId,
		   finger[event.tfinger.fingerId].pressure,
		   inTouch->pressureres);
		   */
	    //printf("Finger: %i, pressure: %f\n",event.tfinger.fingerId,
	    //   finger[event.tfinger.fingerId].pressure);

	    
	    break;	    
	  case SDL_FINGERDOWN:
	    printf("Figner: %i down - x: %i, y: %i\n",event.tfinger.fingerId,
		   event.tfinger.x,event.tfinger.y);
	    finger[event.tfinger.fingerId].p.x = event.tfinger.x;
	    finger[event.tfinger.fingerId].p.y = event.tfinger.y;
	    break;
	  case SDL_FINGERUP:
	    printf("Figner: %i up - x: %i, y: %i\n",event.tfinger.fingerId,
		   event.tfinger.x,event.tfinger.y);
	    finger[event.tfinger.fingerId].p.x = -1;
	    finger[event.tfinger.fingerId].p.y = -1;
	    break;
	  }
      }
    //And draw
    DrawScreen(screen,h);
	printf("Things\n");
    /*
      for(i=0;i<512;i++) 
      if(keystat[i]) printf("%i\n",i);
      printf("Buttons:%i\n",bstatus);
    */
  }  
  SDL_Quit();
      
  return 0;
}
