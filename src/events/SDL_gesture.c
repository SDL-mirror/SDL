/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software    Founation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* General mouse handling code for SDL */

#include "SDL_events.h"
#include "SDL_events_c.h"
#include "SDL_gesture_c.h"

//TODO: Replace with malloc
#define MAXFINGERS 5
#define MAXTOUCHES 2
#define MAXTEMPLATES 4
#define MAXPATHSIZE 1024

#define DOLLARNPOINTS 64
#define DOLLARSIZE 256

//PHI = ((sqrt(5)-1)/2)
#define PHI 0.618033989 

typedef struct {
  float x,y;
} Point;


typedef struct {
  Point p;
  float pressure;
  SDL_FingerID id;
} Finger;


typedef struct {
  float length;
  
  int numPoints;
  Point p[MAXPATHSIZE];
} DollarPath;


typedef struct {
  Finger f;
  Point cv;
  float dtheta,dDist;
  DollarPath dollarPath;
} TouchPoint;

typedef struct {
  Point path[DOLLARNPOINTS];
  unsigned long hash;
} DollarTemplate;

typedef struct {
  SDL_GestureID id;
  Point res;
  Point centroid;
  TouchPoint gestureLast[MAXFINGERS];
  int numDownFingers;

  int numDollarTemplates;
  DollarTemplate dollarTemplate[MAXTEMPLATES];

  SDL_bool recording;
} GestureTouch;

GestureTouch gestureTouch[MAXTOUCHES];
int numGestureTouches = 0;
SDL_bool recordAll;

int SDL_RecordGesture(SDL_TouchID touchId) {
  int i;
  if(touchId < 0) recordAll = SDL_TRUE;
  for(i = 0;i < numGestureTouches; i++) {
    if((touchId < 0) || (gestureTouch[i].id == touchId)) {
      gestureTouch[i].recording = SDL_TRUE;
      if(touchId >= 0)
	return 1;
    }      
  }
  return (touchId < 0);
}

unsigned long SDL_HashDollar(Point* points) {
  unsigned long hash = 5381;
  int i;
  for(i = 0;i < DOLLARNPOINTS; i++) { 
    hash = ((hash<<5) + hash) + points[i].x;
    hash = ((hash<<5) + hash) + points[i].y;
  }
  return hash;
}


static int SaveTemplate(DollarTemplate *templ, SDL_RWops * src) {
  if(src == NULL) return 0;

  int i;
  
  //No Longer storing the Hash, rehash on load
  //if(SDL_RWops.write(src,&(templ->hash),sizeof(templ->hash),1) != 1) return 0;

  if(SDL_RWwrite(src,templ->path,
		 sizeof(templ->path[0]),DOLLARNPOINTS) != DOLLARNPOINTS) 
    return 0;

  return 1;
}


int SDL_SaveAllDollarTemplates(SDL_RWops *src) {  
  int i,j,rtrn = 0;
  for(i = 0; i < numGestureTouches; i++) {
    GestureTouch* touch = &gestureTouch[i];
    for(j = 0;j < touch->numDollarTemplates; j++) {
	rtrn += SaveTemplate(&touch->dollarTemplate[i],src);
    }
  }
  return rtrn;  
}

int SDL_SaveDollarTemplate(SDL_GestureID gestureId, SDL_RWops *src) {
  int i,j;
  for(i = 0; i < numGestureTouches; i++) {
    GestureTouch* touch = &gestureTouch[i];
    for(j = 0;j < touch->numDollarTemplates; j++) {
      if(touch->dollarTemplate[i].hash == gestureId) {
	return SaveTemplate(&touch->dollarTemplate[i],src);
      }
    }
  }
  SDL_SetError("Unknown gestureId");
  return -1;
}

//path is an already sampled set of points
//Returns the index of the gesture on success, or -1
static int SDL_AddDollarGesture(GestureTouch* inTouch,Point* path) {
  if(inTouch == NULL) {
    if(numGestureTouches == 0) return -1;
    int i = 0;
    for(i = 0;i < numGestureTouches; i++) {
      inTouch = &gestureTouch[i];
      if(inTouch->numDollarTemplates < MAXTEMPLATES) {
	DollarTemplate *templ = 
	  &inTouch->dollarTemplate[inTouch->numDollarTemplates];
	memcpy(templ->path,path,DOLLARNPOINTS*sizeof(Point));
	templ->hash = SDL_HashDollar(templ->path);
	inTouch->numDollarTemplates++;
      }
    }
    return inTouch->numDollarTemplates - 1;
  }else if(inTouch->numDollarTemplates < MAXTEMPLATES) {
    DollarTemplate *templ = 
      &inTouch->dollarTemplate[inTouch->numDollarTemplates];
    memcpy(templ->path,path,DOLLARNPOINTS*sizeof(Point));
    templ->hash = SDL_HashDollar(templ->path);
    inTouch->numDollarTemplates++;
    return inTouch->numDollarTemplates - 1;
  }
  return -1;
}

int SDL_LoadDollarTemplates(SDL_TouchID touchId, SDL_RWops *src) {
  if(src == NULL) return 0;
  int i,loaded = 0;
  GestureTouch *touch = NULL;
  if(touchId >= 0) {
    for(i = 0;i < numGestureTouches; i++)
      if(gestureTouch[i].id == touchId)
	touch = &gestureTouch[i];
    if(touch == NULL) return -1;
  }

  while(1) {
    DollarTemplate templ;
    //fscanf(fp,"%lu ",&templ.hash);
    /*
    for(i = 0;i < DOLLARNPOINTS; i++) {		
      int x,y;
      if(fscanf(fp,"%i %i ",&x,&y) != 2) break;
      templ.path[i].x = x;
      templ.path[i].y = y;
    }
    fscanf(fp,"\n");
    */
    if(SDL_RWread(src,templ.path,sizeof(templ.path[0]),DOLLARNPOINTS) < DOLLARNPOINTS) break;

    if(touchId >= 0) {
      printf("Adding loaded gesture to 1 touch\n");
      if(SDL_AddDollarGesture(touch,templ.path)) loaded++;
    }
    else {
      printf("Adding to: %i touches\n",numGestureTouches);
      for(i = 0;i < numGestureTouches; i++) {
	touch = &gestureTouch[i];
	printf("Adding loaded gesture to + touches\n");
	//TODO: What if this fails?
	SDL_AddDollarGesture(touch,templ.path);	
      }
      loaded++;
    }
  }

  return loaded; 
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
  float ta = -M_PI/4;
  float tb = M_PI/4;
  float dt = M_PI/90;
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

float dollarRecognize(DollarPath path,int *bestTempl,GestureTouch* touch) {
	
	Point points[DOLLARNPOINTS];
	int numPoints = dollarNormalize(path,points);
	int i;
	
	int bestDiff = 10000;
	*bestTempl = -1;
	for(i = 0;i < touch->numDollarTemplates;i++) {
		int diff = bestDollarDifference(points,touch->dollarTemplate[i].path);
		if(diff < bestDiff) {bestDiff = diff; *bestTempl = i;}
	}
	return bestDiff;
}

int SDL_GestureAddTouch(SDL_Touch* touch) { 
  if(numGestureTouches >= MAXTOUCHES) return -1;
  
  gestureTouch[numGestureTouches].res.x = touch->xres;
  gestureTouch[numGestureTouches].res.y = touch->yres;
  gestureTouch[numGestureTouches].numDownFingers = 0;

  gestureTouch[numGestureTouches].res.x = touch->xres;
  gestureTouch[numGestureTouches].id = touch->id;

  gestureTouch[numGestureTouches].numDollarTemplates = 0;

  gestureTouch[numGestureTouches].recording = SDL_FALSE;

  numGestureTouches++;
  return 0;
}

GestureTouch * SDL_GetGestureTouch(SDL_TouchID id) {
  int i;
  for(i = 0;i < numGestureTouches; i++) {
    //printf("%i ?= %i\n",gestureTouch[i].id,id);
    if(gestureTouch[i].id == id) return &gestureTouch[i];
  }
  return NULL;
}

int SDL_SendGestureMulti(GestureTouch* touch,float dTheta,float dDist) {
  SDL_Event event;
  event.mgesture.type = SDL_MULTIGESTURE;
  event.mgesture.touchId = touch->id;
  event.mgesture.x = touch->centroid.x;
  event.mgesture.y = touch->centroid.y;
  event.mgesture.dTheta = dTheta;
  event.mgesture.dDist = dDist;  
  return SDL_PushEvent(&event) > 0;
}

int SDL_SendGestureDollar(GestureTouch* touch,
			  SDL_GestureID gestureId,float error) {
  SDL_Event event;
  event.dgesture.type = SDL_DOLLARGESTURE;
  event.dgesture.touchId = touch->id;
  /*
    //TODO: Add this to give location of gesture?
  event.mgesture.x = touch->centroid.x;
  event.mgesture.y = touch->centroid.y;
  */
  event.dgesture.gestureId = gestureId;
  event.dgesture.error = error;  
  return SDL_PushEvent(&event) > 0;
}


int SDL_SendDollarRecord(GestureTouch* touch,SDL_GestureID gestureId) {
  SDL_Event event;
  event.dgesture.type = SDL_DOLLARRECORD;
  event.dgesture.touchId = touch->id;
  event.dgesture.gestureId = gestureId;

  return SDL_PushEvent(&event) > 0;
}


void SDL_GestureProcessEvent(SDL_Event* event)
{
  if(event->type == SDL_FINGERMOTION || 
     event->type == SDL_FINGERDOWN ||
     event->type == SDL_FINGERUP) {
    GestureTouch* inTouch = SDL_GetGestureTouch(event->tfinger.touchId);

    //Shouldn't be possible
    if(inTouch == NULL) return;
    
    
    float x = ((float)event->tfinger.x)/inTouch->res.x;
    float y = ((float)event->tfinger.y)/inTouch->res.y;
    int j,empty = -1;
    
    for(j = 0;j<inTouch->numDownFingers;j++) {
      if(inTouch->gestureLast[j].f.id != event->tfinger.fingerId) continue;
      //Finger Up
      if(event->type == SDL_FINGERUP) {
	inTouch->numDownFingers--;

	if(inTouch->recording) {
	  inTouch->recording = SDL_FALSE;
	  Point path[DOLLARNPOINTS];
	  dollarNormalize(inTouch->gestureLast[j].dollarPath,path);
	  int index;
	  if(recordAll) {
	    index = SDL_AddDollarGesture(NULL,path);
	    int i;
	    for(i = 0;i < numGestureTouches; i++)
	      gestureTouch[i].recording = SDL_FALSE;
	  }
	  else {
	    index = SDL_AddDollarGesture(inTouch,path);
	  }
	  
	  if(index >= 0) {
	    SDL_SendDollarRecord(inTouch,inTouch->dollarTemplate[index].hash);
	  }
	  else {
	    SDL_SendDollarRecord(inTouch,-1);
	  }
	}
	else {	
	  int bestTempl;
	  float error;
	  error = dollarRecognize(inTouch->gestureLast[j].dollarPath,
				  &bestTempl,inTouch);
	  if(bestTempl >= 0){
	    //Send Event
	    unsigned long gestureId = inTouch->dollarTemplate[bestTempl].hash;
	    SDL_SendGestureDollar(inTouch,gestureId,error);
	    printf("Dollar error: %f\n",error);
	  }
	} 
	inTouch->gestureLast[j] = inTouch->gestureLast[inTouch->numDownFingers];
	j = -1;
	break;
      }
      else if(event->type == SDL_FINGERMOTION) {
	float dx = x - inTouch->gestureLast[j].f.p.x;
	float dy = y - inTouch->gestureLast[j].f.p.y;
	DollarPath* path = &inTouch->gestureLast[j].dollarPath;
	if(path->numPoints < MAXPATHSIZE) {
	  path->p[path->numPoints].x = x;
	  path->p[path->numPoints].y = y;
	  path->length += sqrt(dx*dx + dy*dy);
	  path->numPoints++;
	}


	inTouch->centroid.x += dx/inTouch->numDownFingers;
	inTouch->centroid.y += dy/inTouch->numDownFingers;    
	if(inTouch->numDownFingers > 1) {
	  Point lv; //Vector from centroid to last x,y position
	  Point v; //Vector from centroid to current x,y position
	  lv = inTouch->gestureLast[j].cv;
	  float lDist = sqrt(lv.x*lv.x + lv.y*lv.y);
	  //printf("lDist = %f\n",lDist);
	  v.x = x - inTouch->centroid.x;
	  v.y = y - inTouch->centroid.y;
	  inTouch->gestureLast[j].cv = v;
	  float Dist = sqrt(v.x*v.x+v.y*v.y);
	  // cos(dTheta) = (v . lv)/(|v| * |lv|)
	  
	  //Normalize Vectors to simplify angle calculation
	  lv.x/=lDist;
	  lv.y/=lDist;
	  v.x/=Dist;
	  v.y/=Dist;
	  float dtheta = atan2(lv.x*v.y - lv.y*v.x,lv.x*v.x + lv.y*v.y);
	  
	  float dDist = (Dist - lDist);
	  if(lDist == 0) {dDist = 0;dtheta = 0;} //To avoid impossible values
	  inTouch->gestureLast[j].dDist = dDist;
	  inTouch->gestureLast[j].dtheta = dtheta;
	  
	  //printf("dDist = %f, dTheta = %f\n",dDist,dtheta);
	  //gdtheta = gdtheta*.9 + dtheta*.1;
	  //gdDist  =  gdDist*.9 +  dDist*.1
	  //knob.r += dDist/numDownFingers;
	  //knob.ang += dtheta;
	  //printf("thetaSum = %f, distSum = %f\n",gdtheta,gdDist);
	  //printf("id: %i dTheta = %f, dDist = %f\n",j,dtheta,dDist);
	  SDL_SendGestureMulti(inTouch,dtheta,dDist);
	}
	else {
	  inTouch->gestureLast[j].dDist = 0;
	  inTouch->gestureLast[j].dtheta = 0;
	  inTouch->gestureLast[j].cv.x = 0;
	  inTouch->gestureLast[j].cv.y = 0;
	}
	inTouch->gestureLast[j].f.p.x = x;
	inTouch->gestureLast[j].f.p.y = y;
	break;
	//pressure?
      }      
    }
    
    if(j == inTouch->numDownFingers) {
      //printf("Finger Down!!!\n");
      inTouch->numDownFingers++;
      inTouch->centroid.x = (inTouch->centroid.x*(inTouch->numDownFingers - 1)+ 
			     x)/inTouch->numDownFingers;
      inTouch->centroid.y = (inTouch->centroid.y*(inTouch->numDownFingers - 1)+
			     y)/inTouch->numDownFingers;
      
      inTouch->gestureLast[j].f.id = event->tfinger.fingerId;
      inTouch->gestureLast[j].f.p.x  = x;
      inTouch->gestureLast[j].f.p.y  = y;	
      inTouch->gestureLast[j].cv.x = 0;
      inTouch->gestureLast[j].cv.y = 0;

      inTouch->gestureLast[j].dollarPath.length = 0;
      inTouch->gestureLast[j].dollarPath.p[0].x = x;
      inTouch->gestureLast[j].dollarPath.p[0].y = y;
      inTouch->gestureLast[j].dollarPath.numPoints = 1;
    }
  }
}  
  
  /* vi: set ts=4 sw=4 expandtab: */
  
