#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <linux/input.h>
#include <fcntl.h>


#define PI 3.1415926535897
#define WIDTH 640
#define HEIGHT 480
#define BPP 4
#define DEPTH 32


#define MAX_FINGERS 5

int mousx,mousy;
int keystat[512];
int bstatus;




typedef struct {
  int x,y;
} Point;

Point finger[MAX_FINGERS];

void handler (int sig)
{
  printf ("\nexiting...(%d)\n", sig);
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
  
  pixmem32 = (Uint32*) screen->pixels  + y*screen->pitch/BPP + x;
  *pixmem32 = colour;
}

void drawCircle(SDL_Surface* screen,int x,int y,int r,int c)
{

  float a;
  for(a=0;a<2*PI;a+=1.f/(float)r)
  {
    setpix(screen,(int)(x+r*cos(a)),(int)(y+r*sin(a)),c);
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
        }
    }
  drawCircle(screen,mousx,mousy,30,0xFFFFFF);
  int i;
  for(i=0;i<MAX_FINGERS;i++)
    drawCircle(screen,finger[i].x,finger[i].y,30,0xFFFFFF);
  
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
  struct input_event ev[64];
  int fd, rd, value, size = sizeof (struct input_event);
  char name[256] = "Unknown";
  char *device = NULL;

  
  
  //Setup check
  if (argv[1] == NULL){
    printf("Please specify (on the command line) the path to the dev event interface device\n");
    exit (0);
  }
  
  if ((getuid ()) != 0)
    printf ("You are not root! This may not work...\n");
  
  if (argc > 1)
    device = argv[1];
  
  //Open Device
  if ((fd = open (device, O_RDONLY | O_NONBLOCK)) == -1)
    printf ("%s is not a vaild device.\n", device);
  
  //Print Device Name
  ioctl (fd, EVIOCGNAME (sizeof (name)), name);
  printf ("Reading From : %s (%s)\n", device, name);
  
  
  
  
  
  SDL_Surface *screen;
  SDL_Event event;
  
  int keypress = 0;
  int h=0,s=1,i,j;
  int tx,ty,curf=0;

  memset(keystat,0,512*sizeof(keystat[0]));
  if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;
  
  if (!(screen = initScreen(WIDTH,HEIGHT)))
    {
      SDL_Quit();
      return 1;
    }

  while(!keystat[27]) 
    {
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
	      printf("Holy SH!T\n");
	      break;
		     
	    }
	}

      //poll for Touch <- Goal: make this a case:      
      

      /*if ((rd = read (fd, ev, size * 64)) < size)
	perror_exit ("read()");          */
      //printf("time: %i\n type: %X\n code: %X\n value: %i\n ",
      //     ev->time,ev->type,ev->code,ev->value);
      if((rd = read (fd, ev, size * 64)) >= size)
	 for (i = 0; i < rd / sizeof(struct input_event); i++)
	   switch (ev[i].type)	
	     {
	     case EV_ABS:
	       if(ev[i].code == ABS_X)
		 tx = ev[i].value;
	       else if (ev[i].code == ABS_Y)
		 ty = ev[i].value;
	       else if (ev[i].code == ABS_MISC)
		 {	     
		   //printf("Misc:type: %X\n     code: %X\n     value: %i\n ",
		   //      ev[i].type,ev[i].code,ev[i].value);
		 }
	       break;
	     case EV_MSC:
	       if(ev[i].code == MSC_SERIAL)
		 curf = ev[i].value;
	       break;
	     case EV_SYN:
	       curf -= 1;
	       if(tx >= 0)
		 finger[curf].x = tx;
	       if(ty >= 0)
		 finger[curf].y = ty;
	       
	       //printf("Synched: %i tx: %i, ty: %i\n",curf,finger[curf].x,finger[curf].y);
	       tx = -1;
	       ty = -1;
	       break;    
	       
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
