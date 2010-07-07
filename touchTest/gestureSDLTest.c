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
  /*
  if(f1 <= f2)
    printf("Min angle (x1): %f\n",x1);
  else if(f1 >  f2)
    printf("Min angle (x2): %f\n",x2);
  */
  return SDL_min(f1,f2);  
}

float dollarRecognize(SDL_Surface* screen, DollarPath path,int *bestTempl) {

  Point points[DOLLARNPOINTS];
  int numPoints = dollarNormalize(path,points);
  int i;
  
  int k;
  /*
  for(k = 0;k<DOLLARNPOINTS;k++) {
    printf("(%f,%f)\n",points[k].x,
	   points[k].y);
  }
  */
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
      int j,empty = -1;
      
      for(j = 0;j<MAXFINGERS;j++) {
	if(gestureLast[j].f.id == event.tfinger.fingerId) {
	  if(event.type == SDL_FINGERUP) {
	    numDownFingers--;
	    if(numDownFingers <= 1) {
	      gdtheta = 0;
	      gdDist = 0;
	    }
	    if(!keystat[32]){ //spacebar
	      int bestTempl;
	      float error = dollarRecognize(screen,dollarPath[j],&bestTempl);
	      if(bestTempl >= 0){
		drawDollarPath(screen,dollarTemplate[bestTempl]
			       ,DOLLARNPOINTS,-15,0x0066FF);		
		printf("Dollar error: %f\n",error);
	      }
	      
	    }
	    else if(numDollarTemplates < MAXTEMPLATES) {
	      
	      dollarNormalize(dollarPath[j],
			      dollarTemplate[numDollarTemplates]);
	      /*
	      int k;	      
	      for(k = 0;k<DOLLARNPOINTS;k++) {
		printf("(%f,%f)\n",dollarTemplate[numDollarTemplates][i].x,
		       dollarTemplate[numDollarTemplates][i].y);
		       }*/
	      numDollarTemplates++;	      
	    }

	    gestureLast[j].f.id = -1;
	    break;
	  }
	  else {
	    dollarPath[j].p[dollarPath[j].numPoints].x = x;
	    dollarPath[j].p[dollarPath[j].numPoints].y = y;
	    float dx = (dollarPath[j].p[dollarPath[j].numPoints  ].x-
			dollarPath[j].p[dollarPath[j].numPoints-1].x);
	    float dy = (dollarPath[j].p[dollarPath[j].numPoints  ].y-
			dollarPath[j].p[dollarPath[j].numPoints-1].y);
	    dollarPath[j].length += sqrt(dx*dx + dy*dy);

	    dollarPath[j].numPoints++;

	    centroid.x = centroid.x + dx/numDownFingers;
	    centroid.y = centroid.y + dy/numDownFingers;    
	    if(numDownFingers > 1) {
	      Point lv; //Vector from centroid to last x,y position
	      Point v; //Vector from centroid to current x,y position
	      lv.x = gestureLast[j].cv.x;
	      lv.y = gestureLast[j].cv.y;
	      float lDist = sqrt(lv.x*lv.x + lv.y*lv.y);
	      
	      v.x = x - centroid.x;
	      v.y = y - centroid.y;
	      gestureLast[j].cv = v;
	      float Dist = sqrt(v.x*v.x+v.y*v.y);
	      // cos(dTheta) = (v . lv)/(|v| * |lv|)
	      
	      lv.x/=lDist;
	      lv.y/=lDist;
	      v.x/=Dist;
	      v.y/=Dist;
	      float dtheta = atan2(lv.x*v.y - lv.y*v.x,lv.x*v.x + lv.y*v.y);
	      
	      float dDist = (lDist - Dist);	      
	      
	      gestureLast[j].dDist = dDist;
	      gestureLast[j].dtheta = dtheta;

	      //gdtheta = gdtheta*.9 + dtheta*.1;
	      //gdDist  =  gdDist*.9 +  dDist*.1
	      gdtheta += dtheta;
	      gdDist += dDist;

	      //printf("thetaSum = %f, distSum = %f\n",gdtheta,gdDist);
	      //printf("id: %i dTheta = %f, dDist = %f\n",j,dtheta,dDist);
	    }
	    else {
	      gestureLast[j].dDist = 0;
	      gestureLast[j].dtheta = 0;
	      gestureLast[j].cv.x = 0;
	      gestureLast[j].cv.y = 0;
	    }
	    gestureLast[j].f.p.x = x;
	    gestureLast[j].f.p.y = y;
	    break;
	    //pressure?
	  }	  
	}
	else if(gestureLast[j].f.id == -1 && empty == -1) {
	  empty = j;
	}
      }
      
      if(j >= MAXFINGERS && empty >= 0) {
	//	printf("Finger Down!!!\n");
	numDownFingers++;
	centroid.x = (centroid.x*(numDownFingers - 1) + x)/numDownFingers;
	centroid.y = (centroid.y*(numDownFingers - 1) + y)/numDownFingers;
	
	j = empty;
	gestureLast[j].f.id = event.tfinger.fingerId;
	gestureLast[j].f.p.x  = x;
	gestureLast[j].f.p.y  = y;
	
	
	dollarPath[j].length = 0;
	dollarPath[j].p[0].x = x;
	dollarPath[j].p[0].y = y;
	dollarPath[j].numPoints = 1;
      }
      
      //draw the touch:
      
      if(gestureLast[j].f.id < 0) continue; //Finger up. Or some error...
      
      unsigned int c = colors[gestureLast[j].f.id%7]; 
      unsigned int col = 
	((unsigned int)(c*(.1+.85))) |
	((unsigned int)((0xFF*(1-((float)age)/EVENT_BUF_SIZE))) & 0xFF)<<24;
      x = gestureLast[j].f.p.x;
      y = gestureLast[j].f.p.y;
      if(event.type == SDL_FINGERMOTION)
	drawCircle(screen,x*screen->w,y*screen->h,5,col);
      else if(event.type == SDL_FINGERDOWN)
	drawCircle(screen,x*screen->w,y*screen->h,-10,col);     
      
      //if there is a centroid, draw it
      if(numDownFingers > 1) {
	unsigned int col = 
	  ((unsigned int)(0xFFFFFF)) |
	  ((unsigned int)((0xFF*(1-((float)age)/EVENT_BUF_SIZE))) & 0xFF)<<24;
	drawCircle(screen,centroid.x*screen->w,centroid.y*screen->h,5,col);
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
	  }
      
    
	
	//And draw
	
      }
    DrawScreen(screen,h);
    //printf("c: (%f,%f)\n",centroid.x,centroid.y);
    //printf("numDownFingers: %i\n",numDownFingers);
    //for(i=0;i<512;i++) 
    // if(keystat[i]) printf("%i\n",i);
      
    
  }  
  SDL_Quit();
  
  return 0;
}

