#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <SDL_touch.h>

#define PI 3.1415926535897
#define PHI ((sqrt(5)-1)/2)
#define WIDTH 640
#define HEIGHT 480
#define BPP 4
#define DEPTH 32

#define MAXFINGERS 3

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

typedef struct { //dt + s
  Point d,s; //direction, start
  int points;
} Line;


typedef struct {
  float length;
  
  int numPoints;
  Point p[EVENT_BUF_SIZE]; //To be safe
} DollarPath;


Finger finger[MAXFINGERS];

Finger gestureLast[MAXFINGERS];
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
  /*
  for(a=0;a<PI/2;a+=1.f/(float)abs(r))
  {
    if(r > 0) { //r > 0 ==> filled circle
      for(tx=x-r*cos(a);tx<x+r*cos(a);tx++) {
	setpix(screen,tx,(int)(y+r*sin(a)),c);
	setpix(screen,tx,(int)(y-r*sin(a)),c);
      }
    }    else {
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

void drawDollarPath(SDL_Surface* screen,Point* points,int numPoints,
		    int rad,unsigned int col){
  int i;
  for(i=0;i<numPoints;i++) {
    drawCircle(screen,points[i].x+screen->w/2,
	       points[i].y+screen->h/2,
	       rad,col);
  }
}

float dollarDifference(Point* points,Point* templ,float ang) {
  //  Point p[DOLLARNPOINTS];
  float dist = 0;
  Point p;
  int i;
  for(i = 0; i < DOLLARNPOINTS; i++) {
    p.x = points[i].x * cos(ang) - points[i].y * sin(ang);
    p.y = points[i].x * sin(ang) + points[i].y * cos(ang);
    dist += sqrt((p.x-templ[i].x)*(p.x-templ[i].x)+
		 (p.y-templ[i].y)*(p.y-templ[i].y));
  }
  return dist/DOLLARNPOINTS;
  
}

float bestDollarDifference(Point* points,Point* templ) {
  //------------BEGIN DOLLAR BLACKBOX----------------//
  //-TRANSLATED DIRECTLY FROM PSUDEO-CODE AVAILABLE AT-//
  //-"http://depts.washington.edu/aimgroup/proj/dollar/"-//
  float ta = -PI/4;
  float tb = PI/4;
  float dt = PI/90;
  float x1 = PHI*ta + (1-PHI)*tb;
  float f1 = dollarDifference(points,templ,x1);
  float x2 = (1-PHI)*ta + PHI*tb;
  float f2 = dollarDifference(points,templ,x2);
  while(abs(ta-tb) > dt) {
    if(f1 < f2) {
      tb = x2;
      x2 = x1;
      f2 = f1;
      x1 = PHI*ta + (1-PHI)*tb;
      f1 = dollarDifference(points,templ,x1);
    }
    else {
      ta = x1;
      x1 = x2;
      f1 = f2;
      x2 = (1-PHI)*ta + PHI*tb;
      f2 = dollarDifference(points,templ,x2);
    }
  }
  return SDL_min(f1,f2);
  
}

float dollarRecognize(SDL_Surface* screen, DollarPath path,int *bestTempl) {

  Point points[DOLLARNPOINTS];
  int numPoints = dollarNormalize(path,points);
  int i;
  
  int k;
  for(k = 0;k<DOLLARNPOINTS;k++) {
    printf("(%f,%f)\n",points[k].x,
	   points[k].y);
  }

  drawDollarPath(screen,points,numPoints,-15,0xFF6600);

  int bestDiff = 10000;
  *bestTempl = -1;
  for(i = 0;i < numDollarTemplates;i++) {
    int diff = bestDollarDifference(points,dollarTemplate[i]);
    if(diff < bestDiff) {bestDiff = diff; *bestTempl = i;}
  }
  return bestDiff;
}

//DollarPath contains raw points, plus (possibly) the calculated length
int dollarNormalize(DollarPath path,Point *points) {
  int i;
  //Calculate length if it hasn't already been done
  if(path.length <= 0) {
    for(i=1;i<path.numPoints;i++) {
      float dx = path.p[i  ].x - 
	         path.p[i-1].x;
      float dy = path.p[i  ].y - 
	         path.p[i-1].y;
      path.length += sqrt(dx*dx+dy*dy);
    }
  }


  //Resample
  float interval = path.length/(DOLLARNPOINTS - 1);
  float dist = 0;

  int numPoints = 0;
  Point centroid; centroid.x = 0;centroid.y = 0;
  //printf("(%f,%f)\n",path.p[path.numPoints-1].x,path.p[path.numPoints-1].y);
  for(i = 1;i < path.numPoints;i++) {
    float d = sqrt((path.p[i-1].x-path.p[i].x)*(path.p[i-1].x-path.p[i].x)+
		   (path.p[i-1].y-path.p[i].y)*(path.p[i-1].y-path.p[i].y));
    //printf("d = %f dist = %f/%f\n",d,dist,interval);
    while(dist + d > interval) {
      points[numPoints].x = path.p[i-1].x + 
	((interval-dist)/d)*(path.p[i].x-path.p[i-1].x);
      points[numPoints].y = path.p[i-1].y + 
	((interval-dist)/d)*(path.p[i].y-path.p[i-1].y);
      centroid.x += points[numPoints].x;
      centroid.y += points[numPoints].y;
      numPoints++;

      dist -= interval;
    }
    dist += d;
  }
  if(numPoints < 1) return 0;
  centroid.x /= numPoints;
  centroid.y /= numPoints;
 
  //printf("Centroid (%f,%f)",centroid.x,centroid.y);
  //Rotate Points so point 0 is left of centroid and solve for the bounding box
  float xmin,xmax,ymin,ymax;
  xmin = centroid.x;
  xmax = centroid.x;
  ymin = centroid.y;
  ymax = centroid.y;
  
  float ang = atan2(centroid.y - points[0].y,
		    centroid.x - points[0].x);

  for(i = 0;i<numPoints;i++) {					       
    float px = points[i].x;
    float py = points[i].y;
    points[i].x = (px - centroid.x)*cos(ang) - 
                  (py - centroid.y)*sin(ang) + centroid.x;
    points[i].y = (px - centroid.x)*sin(ang) + 
                  (py - centroid.y)*cos(ang) + centroid.y;


    if(points[i].x < xmin) xmin = points[i].x;
    if(points[i].x > xmax) xmax = points[i].x; 
    if(points[i].y < ymin) ymin = points[i].y;
    if(points[i].y > ymax) ymax = points[i].y;
  }

  //Scale points to DOLLARSIZE, and translate to the origin
  float w = xmax-xmin;
  float h = ymax-ymin;

  for(i=0;i<numPoints;i++) {
    points[i].x = (points[i].x - centroid.x)*DOLLARSIZE/w;
    points[i].y = (points[i].y - centroid.y)*DOLLARSIZE/h;
  }  
  return numPoints;
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
#ifdef DRAW_VECTOR_EST
    gestureLine[i].points = 0;
#endif
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
#ifdef DRAW_VECTOR_EST
	    if(gestureLine[j].points > 0)
	      drawLine(screen,
		     gestureLine[j].s.x*screen->w,
		     gestureLine[j].s.y*screen->h,
		     (gestureLine[j].s.x +50*gestureLine[j].d.x)*screen->w,
		     (gestureLine[j].s.y +50*gestureLine[j].d.y)*screen->h,
		     0xFF00);

	    gestureLine[j].points = 0;
#endif
	    //ignore last point - probably invalid
	    dollarPath[j].numPoints--;
	    
	    
	    float dx = dollarPath[j].p[dollarPath[j].numPoints].x - 
	      dollarPath[j].p[dollarPath[j].numPoints - 1].x;
	    float dy = dollarPath[j].p[dollarPath[j].numPoints].y - 
	      dollarPath[j].p[dollarPath[j].numPoints - 1].y;
	    dollarPath[j].length -= sqrt(dx*dx+dy*dy);

	    if(!keystat[32]){ //spacebar
	      int bestTempl;
	      float error = dollarRecognize(screen,dollarPath[j],&bestTempl);
	      printf("%i\n",bestTempl);
	      if(bestTempl >= 0){
		drawDollarPath(screen,dollarTemplate[bestTempl]
			       ,DOLLARNPOINTS,-15,0x0066FF);\
		
		printf("ERROR: %f\n",error);
	      }
	      
	    }
	    else if(numDollarTemplates < MAXTEMPLATES) {
	      
	      dollarNormalize(dollarPath[j],
			      dollarTemplate[numDollarTemplates]);
	      int k;
	      /*
	      for(k = 0;k<DOLLARNPOINTS;k++) {
		printf("(%f,%f)\n",dollarTemplate[numDollarTemplates][i].x,
		       dollarTemplate[numDollarTemplates][i].y);
		       }*/
	      numDollarTemplates++;	      
	    }

	    gestureLast[j].id = -1;
	    break;
	  }
	  else {
#ifdef DRAW_VECTOR_EST
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
#endif

	    dollarPath[j].p[dollarPath[j].numPoints].x = x;
	    dollarPath[j].p[dollarPath[j].numPoints].y = y;
	    float dx = (dollarPath[j].p[dollarPath[j].numPoints-1].x-
		      dollarPath[j].p[dollarPath[j].numPoints  ].x);
	    float dy = (dollarPath[j].p[dollarPath[j].numPoints-1].y-
		      dollarPath[j].p[dollarPath[j].numPoints  ].y);
	    dollarPath[j].length += sqrt(dx*dx + dy*dy);

	    dollarPath[j].numPoints++;
	    

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
	//	printf("Finger Down!!!\n");
	j = empty; //important that j is the index of the added finger
	gestureLast[j].id = event.tfinger.fingerId;
	gestureLast[j].p.x  = x;
	gestureLast[j].p.y  = y;
#ifdef DRAW_VECTOR_EST
	gestureLine[j].s.x = x;
	gestureLine[j].s.y = y;
	gestureLine[j].points = 1;
#endif

	dollarPath[j].length = 0;
	dollarPath[j].p[0].x = x;
	dollarPath[j].p[0].y = y;
	dollarPath[j].numPoints = 1;
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

  
  keystat[32] = 0;	      

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
    
    //for(i=0;i<512;i++) 
    // if(keystat[i]) printf("%i\n",i);
    
    
  }  
  SDL_Quit();
      
  return 0;
}
