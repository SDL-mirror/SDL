#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <SDL_touch.h>
#include <SDL_gesture.h>

#define PI 3.1415926535897
#define PHI ((sqrt(5)-1)/2)
#define WIDTH 640
#define HEIGHT 480
#define BPP 4
#define DEPTH 32

#define MAXFINGERS 5

#define DOLLARNPOINTS 64
#define DOLLARSIZE 256

//MUST BE A POWER OF 2!
#define EVENT_BUF_SIZE 256

SDL_Event events[EVENT_BUF_SIZE];
int eventWrite;

int mousx,mousy;
int keystat[512];
int bstatus;

int colors[7] = {0xFF,0xFF00,0xFF0000,0xFFFF00,0x00FFFF,0xFF00FF,0xFFFFFF};

int index2fingerid[MAXFINGERS];
int fingersDown;

typedef struct {
  float x,y;
} Point;

typedef struct {
  Point p;
  float pressure;
  int id;
} Finger;

typedef struct {
  Finger f;
  Point cv;
  float dtheta,dDist;
} TouchPoint;

 
typedef struct { //dt + s
  Point d,s; //direction, start
  int points;
} Line;


typedef struct {
  float length;
  
  int numPoints;
  Point p[EVENT_BUF_SIZE]; //To be safe
} DollarPath;

typedef struct {
  float ang,r;
  Point p;
} Knob;

Knob knob;

Finger finger[MAXFINGERS];


DollarPath dollarPath[MAXFINGERS];

#define MAXTEMPLATES 4

Point dollarTemplate[MAXTEMPLATES][DOLLARNPOINTS];
int numDollarTemplates = 0;
#ifdef DRAW_VECTOR_EST
Line gestureLine[MAXFINGERS];
#endif

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

  SDL_GetRGB(colour,screen->format,&r,&g,&b); //Always returns 0xFFFFFF?
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

  float a;
  int tx;
  
  int ty;
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
  //printf("Knob: x = %f, y = %f, r = %f, a = %f\n",k.p.x,k.p.y,k.r,k.ang);
 
  drawCircle(screen,k.p.x*screen->w,k.p.y*screen->h,k.r*screen->w,0xFFFFFF);
  
  drawCircle(screen,(k.p.x+k.r/2*cos(k.ang))*screen->w,
  	            (k.p.y+k.r/2*sin(k.ang))*screen->h,k.r/4*screen->w,0);
  
}

void DrawScreen(SDL_Surface* screen, int h)
{
  int x, y, xm,ym,c;
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
	  //setpix(screen,x,y,0); //Inefficient, but that's okay...
        }
    }
  drawCircle(screen,mousx,mousy,-30,0xFFFFFF);
  drawLine(screen,0,0,screen->w,screen->h,0xFFFFFF);

  int i;
//draw Touch History
  TouchPoint gestureLast[MAXFINGERS];
  //printf("------------------Start History------------------\n");
  for(i = 0;i < MAXFINGERS;i++) {
    gestureLast[i].f.id = -1;
  }
  int numDownFingers = 0;
  Point centroid;
  float gdtheta,gdDist;


  for(i = SDL_max(0,eventWrite - EVENT_BUF_SIZE);i < eventWrite;i++) {
    SDL_Event event = events[i&(EVENT_BUF_SIZE-1)];
    int age = eventWrite - i - 1;
    if(event.type == SDL_FINGERMOTION || 
       event.type == SDL_FINGERDOWN ||
       event.type == SDL_FINGERUP) {
      SDL_Touch* inTouch = SDL_GetTouch(event.tfinger.touchId);
      //SDL_Finger* inFinger = SDL_GetFinger(inTouch,event.tfinger.fingerId);
	    
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
      /*      
      //if there is a centroid, draw it
      if(numDownFingers > 1) {
	unsigned int col = 
	  ((unsigned int)(0xFFFFFF)) |
	  ((unsigned int)((0xFF*(1-((float)age)/EVENT_BUF_SIZE))) & 0xFF)<<24;
	drawCircle(screen,centroid.x*screen->w,centroid.y*screen->h,5,col);
      }
      */
    }
  }
  

  for(i=0;i<MAXFINGERS;i++)
    if(finger[i].p.x >= 0 && finger[i].p.y >= 0)
      if(finger[i].pressure > 0)
	drawCircle(screen,finger[i].p.x*screen->w,finger[i].p.y*screen->h
		   ,20,0xFF*finger[i].pressure);
      else
	drawCircle(screen,finger[i].p.x*screen->w,finger[i].p.y*screen->h
		   ,20,0xFF);


  
  keystat[32] = 0;
  
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
  
  int keypress = 0;
  int h=0,s=1,i,j;

  //gesture variables
  int numDownFingers = 0;
  float gdtheta = 0,gdDist = 0;
  Point centroid;
  knob.r = .1;
  knob.ang = 0;
  TouchPoint gestureLast[MAXFINGERS];
  


  memset(keystat,0,512*sizeof(keystat[0]));
  if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;
  
  if (!(screen = initScreen(WIDTH,HEIGHT)))
    {
      SDL_Quit();
      return 1;
    }

  while(!keystat[27]) {
    //Poll SDL
    while(SDL_PollEvent(&event)) 
      {
	//Record _all_ events
	events[eventWrite & (EVENT_BUF_SIZE-1)] = event;
	eventWrite++;
	
	switch (event.type) 
	  {
	  case SDL_QUIT:
	    keystat[27] = 1;
	    break;
	  case SDL_KEYDOWN:
	    //printf("%i\n",event.key.keysym.sym);
	    keystat[event.key.keysym.sym] = 1;
	    if(event.key.keysym.sym == 32) {
	      SDL_RecordGesture(-1);
	    }
	    else if(event.key.keysym.sym == 115) {
	      FILE *fp;
	      fp = fopen("gestureSave","w");
	      SDL_SaveAllDollarTemplates(fp);
	      fclose(fp);
	    }
	    else if(event.key.keysym.sym == 108) {
	      FILE *fp;
	      fp = fopen("gestureSave","r");
	      printf("Loaded: %i\n",SDL_LoadDollarTemplates(-1,fp));
	      fclose(fp);
	    }
	    
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
	    SDL_Touch* inTouch = SDL_GetTouch(event.tfinger.touchId);
	    SDL_Finger* inFinger = SDL_GetFinger(inTouch,event.tfinger.fingerId);

	    for(i = 0;i<MAXFINGERS;i++) 
	      if(index2fingerid[i] == event.tfinger.fingerId) 	      
		break;
	    if(i == MAXFINGERS) break;  
	    if(inTouch > 0) {
	      finger[i].p.x = ((float)event.tfinger.x)/
		inTouch->xres;
	      finger[i].p.y = ((float)event.tfinger.y)/
		inTouch->yres;
	      
	      finger[i].pressure = 
		((float)event.tfinger.pressure)/inTouch->pressureres;
	      /*
	      printf("Finger: %i, Pressure: %f Pressureres: %i\n",
		     event.tfinger.fingerId,
		     finger[i].pressure,
		     inTouch->pressureres);
	      */
	      //printf("Finger: %i, pressure: %f\n",event.tfinger.fingerId,
	      //   finger[event.tfinger.fingerId].pressure);
	    }
	    
	    break;	    
	  case SDL_FINGERDOWN:
	    printf("Finger: %i down - x: %i, y: %i\n",event.tfinger.fingerId,
		   event.tfinger.x,event.tfinger.y);

	    for(i = 0;i<MAXFINGERS;i++) 
	      if(index2fingerid[i] == -1) {
		index2fingerid[i] = event.tfinger.fingerId;
		break;
	      }
	    finger[i].p.x = event.tfinger.x;
	    finger[i].p.y = event.tfinger.y;
	    break;
	  case SDL_FINGERUP:
	    printf("Figner: %i up - x: %i, y: %i\n",event.tfinger.fingerId,
	           event.tfinger.x,event.tfinger.y);
	    for(i = 0;i<MAXFINGERS;i++) 
	      if(index2fingerid[i] == event.tfinger.fingerId) {
		index2fingerid[i] = -1;
		break;
	      }
	    finger[i].p.x = -1;
	    finger[i].p.y = -1;
	    break;
	  case SDL_MULTIGESTURE:
	    knob.p.x = event.mgesture.x;
	    knob.p.y = event.mgesture.y;
	    knob.ang += event.mgesture.dTheta;
	    knob.r += event.mgesture.dDist;
	    break;
	  case SDL_DOLLARGESTURE:
	    printf("Gesture %lu performed, error: %f\n",
		   event.dgesture.gestureId,
		   event.dgesture.error);
	    break;
	  case SDL_DOLLARRECORD:
	    printf("Recorded gesture: %lu\n",event.dgesture.gestureId);
	    break;
	  }
      }
    DrawScreen(screen,h);    
    for(i = 0; i < 256; i++) 
      if(keystat[i]) 
	printf("Key %i down\n",i);
  }  
  SDL_Quit();
  
  return 0;
}

