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
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* General mouse handling code for SDL */

#include "SDL_events.h"
#include "SDL_events_c.h"
#include "SDL_gesture_c.h"

//TODO: Replace with malloc
#define MAXFINGERS 3
#define MAXTOUCHES 2

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


typedef struct {
  int id;
  Point res;
  Point centroid;
  TouchPoint gestureLast[MAXFINGERS];
  int numDownFingers;
} GestureTouch;

GestureTouch gestureTouch[MAXTOUCHES];
int numGestureTouches = 0;
int SDL_GestureAddTouch(SDL_Touch* touch) { 
  if(numGestureTouches >= MAXTOUCHES) return -1;
  
  gestureTouch[numGestureTouches].res.x = touch->xres;
  gestureTouch[numGestureTouches].res.y = touch->yres;
  gestureTouch[numGestureTouches].numDownFingers = 0;

  gestureTouch[numGestureTouches].res.x = touch->xres;
  gestureTouch[numGestureTouches].id = touch->id;

  numGestureTouches++;
  return 0;
}

GestureTouch * SDL_GetGestureTouch(int id) {
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

      if(event->type == SDL_FINGERUP) {
	inTouch->numDownFingers--;
	inTouch->gestureLast[j] = inTouch->gestureLast[inTouch->numDownFingers];
	break;
      }
      else {
	float dx = x - inTouch->gestureLast[j].f.p.x;
	float dy = y - inTouch->gestureLast[j].f.p.y;
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
    }
  }
}  
  
  /* vi: set ts=4 sw=4 expandtab: */
  
