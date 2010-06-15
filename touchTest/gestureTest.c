#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <SDL_touch.h>

#define PI 3.1415926535897
#define WIDTH 640
#define HEIGHT 480
#define BPP 4
#define DEPTH 32

#define MAXFINGERS 3




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

typedef struct { //dt + s
  Point d,s; //direction, start
  int points;
} Line;


Finger finger[MAXFINGERS];

Finger gestureLast[MAXFINGERS];
Line gestureLine[MAXFINGERS];

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
  /*
  for(a=0;a<PI/2;a+=1.f/(float)abs(r))
  {
    if(r > 0) { //r > 0 ==> filled circle
      for(tx=x-r*cos(a);tx<x+r*cos(a);tx++) {
	setpix(screen,tx,(int)(y+r*sin(a)),c);
	setpix(screen,tx,(int)(y-r*sin(a)),c);
      }
    }
    else {
      //Draw Outline
      setpix(screen,(int)(x+r*cos(a)),(int)(y+r*sin(a)),c);
      setpix(screen,(int)(x-r*cos(a)),(int)(y+r*sin(a)),c);
      setpix(screen,(int)(x+r*cos(a)),(int)(y-r*sin(a)),c);
      setpix(screen,(int)(x-r*cos(a)),(int)(y-r*sin(a)),c);
    }
    }*/
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
  for(i = 0;i < MAXFINGERS;i++) {
    gestureLast[i].id = -1;
    gestureLine[i].points = 0;
  }
  for(i = SDL_max(0,eventWrite - EVENT_BUF_SIZE);i != eventWrite;i++) {
    SDL_Event event = events[i&(EVENT_BUF_SIZE-1)];
    int age = eventWrite - i - 1;
    if(event.type == SDL_FINGERMOTION || 
       event.type == SDL_FINGERDOWN ||
       event.type == SDL_FINGERUP) {
      SDL_Touch* inTouch = SDL_GetTouch(event.tfinger.touchId);
      //SDL_Finger* inFinger = SDL_GetFinger(inTouch,event.tfinger.fingerId);
	    
      float x = ((float)event.tfinger.x)/inTouch->xres;
      float y = ((float)event.tfinger.y)/inTouch->yres;
      int j,empty = -1;
      for(j = 0;j<MAXFINGERS;j++) {
	if(gestureLast[j].id == event.tfinger.fingerId) {
	  if(event.type == SDL_FINGERUP) {
	    if(gestureLine[j].points > 0)
	    drawLine(screen,
		     gestureLine[j].s.x*screen->w,
		     gestureLine[j].s.y*screen->h,
		     (gestureLine[j].s.x +50*gestureLine[j].d.x)*screen->w,
		     (gestureLine[j].s.y +50*gestureLine[j].d.y)*screen->h,
		     0xFF00);

	    gestureLine[j].points = 0;
	    gestureLast[j].id = -1;
	    break;
	  }
	  else {
	    if(gestureLine[j].points == 1) {
	      gestureLine[j].d.x = x - gestureLine[j].s.x;
	      gestureLine[j].d.y = y - gestureLine[j].s.y;
	    }

	    gestureLine[j].s.x = gestureLine[j].s.x*gestureLine[j].points+x;
	    gestureLine[j].s.y = gestureLine[j].s.y*gestureLine[j].points+y;

	    gestureLine[j].d.x = gestureLine[j].d.x*gestureLine[j].points+
	      x - gestureLast[j].p.x;
	    gestureLine[j].d.y = gestureLine[j].d.y*gestureLine[j].points+
	      y - gestureLast[j].p.y;;


	    gestureLine[j].points++;
	    
	    gestureLine[j].s.x /= gestureLine[j].points;
	    gestureLine[j].s.y /= gestureLine[j].points;

	    gestureLine[j].d.x /= gestureLine[j].points;
	    gestureLine[j].d.y /= gestureLine[j].points;

	    
	    

	    gestureLast[j].p.x = x;
	    gestureLast[j].p.y = y;
	    break;
	    //pressure?
	  }	  
	}
	else if(gestureLast[j].id == -1 && empty == -1) {
	  empty = j;
	}
      }

      if(j >= MAXFINGERS && empty >= 0) {
	printf("Finger Down!!!\n");
	j = empty; //important that j is the index of the added finger
	gestureLast[j].id = event.tfinger.fingerId;
	gestureLast[j].p.x  = x;
	gestureLast[j].p.y  = y;

	gestureLine[j].s.x = x;
	gestureLine[j].s.y = y;
	gestureLine[j].points = 1;
      }

      //draw the touch && each centroid:

      if(gestureLast[j].id < 0) continue; //Finger up. Or some error...
      int k;
      for(k = 0; k < MAXFINGERS;k++) {

	if(gestureLast[k].id < 0) continue;
	//colors have no alpha, so shouldn't overflow
	unsigned int c = (colors[gestureLast[j].id%7] + 
			  colors[gestureLast[k].id%7])/2; 
	unsigned int col = 
	  ((unsigned int)(c*(.1+.85))) |
	  ((unsigned int)((0xFF*(1-((float)age)/EVENT_BUF_SIZE))) & 0xFF)<<24;
	x = (gestureLast[j].p.x + gestureLast[k].p.x)/2;
	y = (gestureLast[j].p.y + gestureLast[k].p.y)/2;
	if(event.type == SDL_FINGERMOTION)
	  drawCircle(screen,x*screen->w,y*screen->h,5,col);
	else if(event.type == SDL_FINGERDOWN)
	  drawCircle(screen,x*screen->w,y*screen->h,-10,col);
      }      
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
	      
	      printf("Finger: %i, Pressure: %f Pressureres: %i\n",
		     event.tfinger.fingerId,
		     finger[i].pressure,
		     inTouch->pressureres);
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
	  }
      }
    //And draw
    DrawScreen(screen,h);
    /*
      for(i=0;i<512;i++) 
      if(keystat[i]) printf("%i\n",i);
      printf("Buttons:%i\n",bstatus);
    */
  }  
  SDL_Quit();
      
  return 0;
}
