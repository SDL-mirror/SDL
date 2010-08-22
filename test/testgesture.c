/*  Usage:
 *  Spacebar to begin recording a gesture on all touches.
 *  s to save all touches into "./gestureSave"
 *  l to load all touches from "./gestureSave"
 */

#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <SDL_touch.h>
#include <SDL_gesture.h>


/* Make sure we have good macros for printing 32 and 64 bit values */
#ifndef PRIs32
#define PRIs32 "d"
#endif
#ifndef PRIu32
#define PRIu32 "u"
#endif
#ifndef PRIs64
#ifdef __WIN32__
#define PRIs64 "I64"
#else
#define PRIs64 "lld"
#endif
#endif
#ifndef PRIu64
#ifdef __WIN32__
#define PRIu64 "I64u"
#else
#define PRIu64 "llu"
#endif
#endif

#define WIDTH 640
#define HEIGHT 480
#define BPP 4
#define DEPTH 32

//MUST BE A POWER OF 2!
#define EVENT_BUF_SIZE 256


#define VERBOSE SDL_FALSE

SDL_Event events[EVENT_BUF_SIZE];
int eventWrite;

int colors[7] = {0xFF,0xFF00,0xFF0000,0xFFFF00,0x00FFFF,0xFF00FF,0xFFFFFF};

typedef struct {
  float x,y;
} Point;

typedef struct {
  float ang,r;
  Point p;
} Knob;

Knob knob;

void handler (int sig)
{
  printf ("\exiting...(%d)\n", sig);
  exit (0);
}

void perror_exit (char *error)
{
  perror (error);
  handler (9);
}

void setpix(SDL_Surface *screen, int x, int y, unsigned int col)
{
  Uint32 *pixmem32;
  Uint32 colour;
  
  if((unsigned)x > screen->w) return;
  if((unsigned)y > screen->h) return;

  pixmem32 = (Uint32*) screen->pixels  + y*screen->pitch/BPP + x;
  
  Uint8 r,g,b;
  float a;
  
  memcpy(&colour,pixmem32,screen->format->BytesPerPixel);

  SDL_GetRGB(colour,screen->format,&r,&g,&b);
  //r = 0;g = 0; b = 0;
  a = (col>>24)&0xFF;
  if(a == 0) a = 0xFF; //Hack, to make things easier.
  a /= 0xFF;
  r = r*(1-a) + ((col>>16)&0xFF)*(a);
  g = g*(1-a) + ((col>> 8)&0xFF)*(a);
  b = b*(1-a) + ((col>> 0)&0xFF)*(a);
  colour = SDL_MapRGB( screen->format,r, g, b);
  

  *pixmem32 = colour;
}

void drawLine(SDL_Surface *screen,int x0,int y0,int x1,int y1,unsigned int col) {
  float t;
  for(t=0;t<1;t+=1.f/SDL_max(abs(x0-x1),abs(y0-y1)))
    setpix(screen,x1+t*(x0-x1),y1+t*(y0-y1),col);
}

void drawCircle(SDL_Surface* screen,int x,int y,int r,unsigned int c)
{
  int tx,ty;
  float xr;
  for(ty = -abs(r);ty <= abs(r);ty++) {
    xr = sqrt(r*r - ty*ty);
    if(r > 0) { //r > 0 ==> filled circle
      for(tx=-xr+.5;tx<=xr-.5;tx++) {
	setpix(screen,x+tx,y+ty,c);
      }
    }
    else {
      setpix(screen,x-xr+.5,y+ty,c);
      setpix(screen,x+xr-.5,y+ty,c);
    }
  }
}

void drawKnob(SDL_Surface* screen,Knob k) {
  drawCircle(screen,k.p.x*screen->w,k.p.y*screen->h,k.r*screen->w,0xFFFFFF);  
  drawCircle(screen,(k.p.x+k.r/2*cos(k.ang))*screen->w,
  	            (k.p.y+k.r/2*sin(k.ang))*screen->h,k.r/4*screen->w,0);
}

void DrawScreen(SDL_Surface* screen)
{
  int x, y;
  if(SDL_MUSTLOCK(screen))
    {                                              
      if(SDL_LockSurface(screen) < 0) return;
    }
  for(y = 0;y < screen->h;y++)
    for(x = 0;x < screen->w;x++)
	setpix(screen,x,y,((x%255)<<16) + ((y%255)<<8) + (x+y)%255);

  int i;
  //draw Touch History
  for(i = SDL_max(0,eventWrite - EVENT_BUF_SIZE);i < eventWrite;i++) {
    SDL_Event event = events[i&(EVENT_BUF_SIZE-1)];
    int age = eventWrite - i - 1;
    if(event.type == SDL_FINGERMOTION || 
       event.type == SDL_FINGERDOWN ||
       event.type == SDL_FINGERUP) {
      SDL_Touch* inTouch = SDL_GetTouch(event.tfinger.touchId);
      if(inTouch == NULL) continue;

      float x = ((float)event.tfinger.x)/inTouch->xres;
      float y = ((float)event.tfinger.y)/inTouch->yres;      
      
      //draw the touch:      
      unsigned int c = colors[event.tfinger.touchId%7]; 
      unsigned int col = 
	((unsigned int)(c*(.1+.85))) |
	((unsigned int)((0xFF*(1-((float)age)/EVENT_BUF_SIZE))) & 0xFF)<<24;

      if(event.type == SDL_FINGERMOTION)
	drawCircle(screen,x*screen->w,y*screen->h,5,col);
      else if(event.type == SDL_FINGERDOWN)
	drawCircle(screen,x*screen->w,y*screen->h,-10,col);     
    }
  }
  
  if(knob.p.x > 0)
    drawKnob(screen,knob);
  
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

  //gesture variables
  knob.r = .1;
  knob.ang = 0;

  
  SDL_bool quitting = SDL_FALSE;
  SDL_RWops *src;

  if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;
  
  if (!(screen = initScreen(WIDTH,HEIGHT)))
    {
      SDL_Quit();
      return 1;
    }

  while(!quitting) {
    while(SDL_PollEvent(&event)) 
      {
	//Record _all_ events
	events[eventWrite & (EVENT_BUF_SIZE-1)] = event;
	eventWrite++;
	
	switch (event.type) 
	  {
	  case SDL_QUIT:
	    quitting = SDL_TRUE;
	    break;
	  case SDL_KEYDOWN:
	    switch (event.key.keysym.sym)
	      {
	      case SDLK_SPACE:
		SDL_RecordGesture(-1);
		break;
	      case SDLK_s:
		src = SDL_RWFromFile("gestureSave","w");
		printf("Wrote %i templates\n",SDL_SaveAllDollarTemplates(src));
		SDL_RWclose(src);
		break;
	      case SDLK_l:
		src = SDL_RWFromFile("gestureSave","r");
		printf("Loaded: %i\n",SDL_LoadDollarTemplates(-1,src));
		SDL_RWclose(src);
		break;
	      case SDLK_ESCAPE:
		quitting = SDL_TRUE;
		break;
	    }
	    break;
	  case SDL_VIDEORESIZE:
	    if (!(screen = initScreen(event.resize.w,
				      event.resize.h)))
	      {
		SDL_Quit();
		return 1;
	      }
	    break;
	  case SDL_FINGERMOTION:    
	    ;
#if VERBOSE
	    printf("Finger: %i,x: %i, y: %i\n",event.tfinger.fingerId,
	    	   event.tfinger.x,event.tfinger.y);
#endif
	    SDL_Touch* inTouch = SDL_GetTouch(event.tfinger.touchId);
	    SDL_Finger* inFinger = SDL_GetFinger(inTouch,event.tfinger.fingerId);
	    break;	    
	  case SDL_FINGERDOWN:
#if VERBOSE
	    printf("Finger: %"PRIs64" down - x: %i, y: %i\n",
		   event.tfinger.fingerId,event.tfinger.x,event.tfinger.y);
#endif
	    break;
	  case SDL_FINGERUP:
#if VERBOSE
	    printf("Finger: %"PRIs64" up - x: %i, y: %i\n",
	    	   event.tfinger.fingerId,event.tfinger.x,event.tfinger.y);
#endif
	    break;
	  case SDL_MULTIGESTURE:
#if VERBOSE	    
	    printf("Multi Gesture: x = %f, y = %f, dAng = %f, dR = %f\n",
		   event.mgesture.x,
		   event.mgesture.y,
		   event.mgesture.dTheta,
		   event.mgesture.dDist);
	    printf("MG: numDownTouch = %i\n",event.mgesture.numFingers);
#endif
	    knob.p.x = event.mgesture.x;
	    knob.p.y = event.mgesture.y;
	    knob.ang += event.mgesture.dTheta;
	    knob.r += event.mgesture.dDist;
	    break;
	  case SDL_DOLLARGESTURE:
	    printf("Gesture %"PRIs64" performed, error: %f\n",
		   event.dgesture.gestureId,
		   event.dgesture.error);
	    break;
	  case SDL_DOLLARRECORD:
	    printf("Recorded gesture: %"PRIs64"\n",event.dgesture.gestureId);
	    break;
	  }
      }
    DrawScreen(screen);
  }  
  SDL_Quit();  
  return 0;
}

