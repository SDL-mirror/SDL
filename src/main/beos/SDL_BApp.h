/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#ifndef SDL_BAPP_H
#define SDL_BAPP_H

#include <InterfaceKit.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL_config.h"

#include "SDL_video.h"

/* Local includes */
#include "../../events/SDL_events_c.h"

#ifdef __cplusplus
}

#include <vector>	/* Vector should only be included if we use a C++
					   compiler */

#endif




/* Forward declarations */
class SDL_BWin;

/* Message constants */
enum ToSDL {
	/* Intercepted by BWindow on its way to BView */
	BAPP_MOUSE_MOVED,
	BAPP_MOUSE_BUTTON,
	BAPP_MOUSE_WHEEL,
	BAPP_KEY,
	BAPP_REPAINT,           /* from _UPDATE_ */
	/* From BWindow */
	BAPP_MAXIMIZE,          /* from B_ZOOM */
	BAPP_MINIMIZE,
	BAPP_RESTORE,			/* TODO: IMPLEMENT! */
	BAPP_SHOW,
	BAPP_HIDE,
	BAPP_MOUSE_FOCUS,		/* caused by MOUSE_MOVE */
	BAPP_KEYBOARD_FOCUS,	/* from WINDOW_ACTIVATED */
	BAPP_WINDOW_CLOSE_REQUESTED,
	BAPP_WINDOW_MOVED,
	BAPP_WINDOW_RESIZED,
	BAPP_SCREEN_CHANGED
};



/* Create a descendant of BApplication */
class SDL_BApp : public BApplication {
public:
	SDL_BApp(const char* signature) :
		BApplication(signature) {
#ifndef __cplusplus
		/* Set vector imitation variables */
		_ResizeArray();
		_size = 0;
		_length = 0;
#endif
	}
	virtual ~SDL_BApp() {
#ifndef __cplusplus
		SDL_free(window_map);
#endif
	}
		/* Event-handling functions */
	virtual void MessageReceived(BMessage* message) {
		/* Sort out SDL-related messages */
        switch ( message->what ) {
        case BAPP_MOUSE_MOVED:
        	_HandleMouseMove(message);
        	break;

		case BAPP_MOUSE_BUTTON:
			_HandleMouseButton(message);
			break;
			
		case BAPP_MOUSE_WHEEL:
			_HandleMouseWheel(message);
			break;
			
		case BAPP_KEY:
			_HandleKey(message);
			break;

		case BAPP_REPAINT:
			_HandleBasicWindowEvent(message, SDL_WINDOWEVENT_EXPOSED);
			break;
			
		case BAPP_MAXIMIZE:
			_HandleBasicWindowEvent(message, SDL_WINDOWEVENT_MAXIMIZED);
			break;
			
		case BAPP_MINIMIZE:
			_HandleBasicWindowEvent(message, SDL_WINDOWEVENT_MINIMIZED);
			break;
			
		case BAPP_SHOW:
			_HandleBasicWindowEvent(message, SDL_WINDOWEVENT_SHOWN);
			break;
			
		case BAPP_HIDE:
			_HandleBasicWindowEvent(message, SDL_WINDOWEVENT_HIDDEN);
			break;
			
		case BAPP_MOUSE_FOCUS:
			_HandleMouseFocus(message);
			break;
			
		case BAPP_KEYBOARD_FOCUS:
			_HandleKeyboardFocus(message);
			break;
			
		case BAPP_WINDOW_CLOSE_REQUESTED:
			_HandleBasicWindowEvent(message, SDL_WINDOWEVENT_CLOSE);
			break;
			
		case BAPP_WINDOW_MOVED:
			_HandleWindowMoved(message);
			break;
			
		case BAPP_WINDOW_RESIZED:
			_HandleWindowResized(message);
			break;
			
		case BAPP_SCREEN_CHANGED:
			/* TODO: Handle screen resize or workspace change */
			break;

        default:
           BApplication::MessageReceived(message);
           break;
        }
    }
    
    /* Window creation/destruction methods */
    int32 GetID(SDL_Window *win) {
    	int32 i;
    	for(i = 0; i < _GetNumWindowSlots(); ++i) {
    		if( _GetSDLWindow(i) == NULL ) {
    			_SetSDLWindow(win, i);
    			return i;
    		}
    	}
    	
    	/* Expand the vector if all slots are full */
    	if( i == _GetNumWindowSlots() ) {
    		_PushBackWindow(win);
    		return i;
    	}
    }
    
    /* Modes methods */
    void SetPrevMode(display_mode *prevMode) { saved_mode = prevMode; }
    
    display_mode* GetPrevMode() { return saved_mode; }
    
    /* FIXME: Bad coding practice, but I can't include SDL_BWin.h here.  Is
       there another way to do this? */
    void ClearID(SDL_BWin *bwin); /* Defined in SDL_BeApp.cc */
    
private:
	/* Event management */
	void _HandleBasicWindowEvent(BMessage *msg, int32 sdlEventType) {
		SDL_Window *win;
		int32 winID;
		if(
			!_GetWinID(msg, &winID)
		) {
			return;
		}
		win = _GetSDLWindow(winID);
		SDL_SendWindowEvent(win, sdlEventType, 0, 0);
	}
	
	void _HandleMouseMove(BMessage *msg) {
		SDL_Window *win;
		int32 winID;
		int32 dx, dy;
		if(	
			!_GetWinID(msg, &winID) ||
			msg->FindInt32("dx", &dx) != B_OK || /* x movement */
			msg->FindInt32("dy", &dy) != B_OK    /* y movement */
		) {
			return;
		}
		win = _GetSDLWindow(winID);
		SDL_SendMouseMotion(win, 0, dx, dy);
	}
	
	void _HandleMouseButton(BMessage *msg) {
		SDL_Window *win;
		int32 winID;
		int32 button, state;	/* left/middle/right, pressed/released */
		if(
			!_GetWinID(msg, &winID) ||
			msg->FindInt32("button-id", &button) != B_OK ||
			msg->FindInt32("button-state", &state) != B_OK
		) {
			return;
		}
		win = _GetSDLWindow(winID);
		SDL_SendMouseButton(win, state, button);
	}
	
	void _HandleMouseWheel(BMessage *msg) {
		SDL_Window *win;
		int32 winID;
		int32 xTicks, yTicks;
		if(
			!_GetWinID(msg, &winID) ||
			msg->FindInt32("xticks", &xTicks) != B_OK ||
			msg->FindInt32("yticks", &yTicks) != B_OK
		) {
			return;
		}
		win = _GetSDLWindow(winID);
		SDL_SendMouseWheel(win, xTicks, yTicks);
	}
	
	void _HandleKey(BMessage *msg) {
		int32 scancode, state;	/* scancode, pressed/released */
		if(
			msg->FindInt32("key-state", &state) != B_OK ||
			msg->FindInt32("key-scancode", &scancode) != B_OK
		) {
			return;
		}
		SDL_SendKeyboardKey(state, (SDL_Scancode)scancode);
	}
	
	void _HandleMouseFocus(BMessage *msg) {
		SDL_Window *win;
		int32 winID;
		bool bSetFocus;	/* If false, lose focus */
		if(
			!_GetWinID(msg, &winID) ||
			msg->FindBool("focusGained", &bSetFocus) != B_OK
		) {
			return;
		}
		win = _GetSDLWindow(winID);
		if(bSetFocus) {
			SDL_SetMouseFocus(win);
		} else if(SDL_GetMouseFocus() == win) {
			/* Only lose all focus if this window was the current focus */
			SDL_SetMouseFocus(NULL);
		}
	}
	
	void _HandleKeyboardFocus(BMessage *msg) {
		SDL_Window *win;
		int32 winID;
		bool bSetFocus;	/* If false, lose focus */
		if(
			!_GetWinID(msg, &winID) ||
			msg->FindBool("focusGained", &bSetFocus) != B_OK
		) {
			return;
		}
		win = _GetSDLWindow(winID);
		if(bSetFocus) {
			SDL_SetKeyboardFocus(win);
		} else if(SDL_GetKeyboardFocus() == win) {
			/* Only lose all focus if this window was the current focus */
			SDL_SetKeyboardFocus(NULL);
		}
	}
	
	void _HandleWindowMoved(BMessage *msg) {
		SDL_Window *win;
		int32 winID;
		int32 xPos, yPos;
		/* Get the window id and new x/y position of the window */
		if(
			!_GetWinID(msg, &winID) ||
			msg->FindInt32("window-x", &xPos) != B_OK ||
			msg->FindInt32("window-y", &yPos) != B_OK
		) {
			return;
		}
		win = _GetSDLWindow(winID);
		SDL_SendWindowEvent(win, SDL_WINDOWEVENT_MOVED, xPos, yPos);
	}
	
	void _HandleWindowResized(BMessage *msg) {
		SDL_Window *win;
		int32 winID;
		int32 w, h;
		/* Get the window id ]and new x/y position of the window */
		if(
			!_GetWinID(msg, &winID) ||
			msg->FindInt32("window-w", &w) != B_OK ||
			msg->FindInt32("window-h", &h) != B_OK
		) {
			return;
		}
		win = _GetSDLWindow(winID);
		SDL_SendWindowEvent(win, SDL_WINDOWEVENT_RESIZED, w, h);
	}

	bool _GetWinID(BMessage *msg, int32 *winID) {
		return msg->FindInt32("window-id", winID) == B_OK;
	}



	/* Vector imitators */
	SDL_Window *_GetSDLWindow(int32 winID) {
		return window_map[winID];
	}
	
	void _SetSDLWindow(SDL_Window *win, int32 winID) {
		window_map[winID] = win;
	}
	
	int32 _GetNumWindowSlots() {
#ifdef __cplusplus
		return window_map.size();
#else
		return _size;
#endif
	}
	
	
	void _PopBackWindow() {
#ifdef __cplusplus
		window_map.pop_back();
#else
		--_size;
#endif
	}

	void _PushBackWindow(SDL_Window *win) {
#ifdef __cplusplus
		window_map.push_back(win);
#else
		/* Resize array */
		if(_length == _size) {
			_ResizeArray();
		}

		window_map[_size] = win;
		++_size;
#endif
	}

#ifndef __cplusplus
	_ResizeArray() {
		_length += 4;	/* Increase capacity by some arbitrary number */
		SDL_Window *temp = (SDL_Window*)SDL_calloc(_length, 
							sizeof(SDL_Window*));

		/* Move windows from old list to new list */
		int32 i;
		for(i = 0; i < _size; ++i) {
			temp[i] = window_map[i];
		}
		SDL_free(window_map);
		window_map = temp;
	}
#endif

	/* Members */
#ifdef __cplusplus
	vector<SDL_Window*> window_map; /* Keeps track of SDL_Windows by index-id */
#else
	int32 _size;
	int32 _length;
	SDL_Window *window_map;
#endif

	display_mode *saved_mode;
};

#endif
