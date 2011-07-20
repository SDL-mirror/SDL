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

#ifndef _SDL_BWin_h
#define _SDL_BWin_h

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL_config.h"
#include "SDL.h"
#include "SDL_syswm.h"

#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include <AppKit.h>
#include <InterfaceKit.h>
#include <be/game/DirectWindow.h>
#if SDL_VIDEO_OPENGL
#include <be/opengl/GLView.h>
#endif
#include "SDL_events.h"
#include "SDL_BView.h"
#include "../../main/beos/SDL_BApp.h"

enum WinCommands {
	BWIN_MOVE_WINDOW,
	BWIN_RESIZE_WINDOW,
	BWIN_SHOW_WINDOW,
	BWIN_HIDE_WINDOW,
	BWIN_MAXIMIZE_WINDOW,
	BWIN_MINIMIZE_WINDOW,
	BWIN_RESTORE_WINDOW,	/* TODO: IMPLEMENT THIS! */
	BWIN_SET_TITLE,
	BWIN_FULLSCREEN
};


class SDL_BWin:public BDirectWindow
{
  public:
  	/* Constructor/Destructor */
    SDL_BWin(BRect bounds):BDirectWindow(bounds, "Untitled",
                                         B_TITLED_WINDOW, 0)
    {
        last_buttons = 0;
printf("SDL_BWin.h: 69\n");
        the_view = NULL;
#if SDL_VIDEO_OPENGL
        SDL_GLView = NULL;
#endif
        SDL_View = NULL;
        _shown = false;
        inhibit_resize = false;
        mouse_focused = false;
        prev_frame = NULL; printf("SDL_BWin.h: 79\n");
    }

    virtual ~ SDL_BWin()
    {
        Lock();
        if (the_view) {
#if SDL_VIDEO_OPENGL
            if (the_view == SDL_GLView) {
                SDL_GLView->UnlockGL();
            }
#endif
            RemoveChild(the_view);
            the_view = NULL;
        }
        Unlock();
#if SDL_VIDEO_OPENGL
        if (SDL_GLView) {
            delete SDL_GLView;
        }
#endif
        if (SDL_View) {
            delete SDL_View;
        }
    }
    
    
    /* Other construction */
    virtual int CreateView(Uint32 flags, Uint32 gl_flags)
    {
        int retval;

        retval = 0;
        Lock();
        if (flags & SDL_OPENGL/*SDL_INTERNALOPENGL*/) {
#if SDL_VIDEO_OPENGL
            if (SDL_GLView == NULL) {
                SDL_GLView = new BGLView(Bounds(), "SDL GLView",
                                         B_FOLLOW_ALL_SIDES,
                                         (B_WILL_DRAW | B_FRAME_EVENTS),
                                         gl_flags);
            }
            if (the_view != SDL_GLView) {
                if (the_view) {
                    RemoveChild(the_view);
                }
                AddChild(SDL_GLView);
                SDL_GLView->LockGL();
                the_view = SDL_GLView;
            }
#else
            SDL_SetError("OpenGL support not enabled");
            retval = -1;
#endif
        } else {
            if (SDL_View == NULL) {
                SDL_View = new SDL_BView(Bounds());
            }
            if (the_view != SDL_View) {
                if (the_view) {
#if SDL_VIDEO_OPENGL
                    if (the_view == SDL_GLView) {
                        SDL_GLView->UnlockGL();
                    }
#endif
                    RemoveChild(the_view);
                }
                AddChild(SDL_View);
                the_view = SDL_View;
            }
        }
        Unlock();
        return (retval);
    }
    
    
    /* * * * * Framebuffering* * * * */
    virtual void DirectConnected(direct_buffer_info *info) {
    }
    
    
    /* * * * * Event sending * * * * */
    /* Hook functions */
    virtual void FrameMoved(BPoint origin) {
    	/* Post a message to the BApp so that it can handle the window event */
    	BMessage msg(BAPP_WINDOW_MOVED);
		msg.AddInt32("window-x", (int)origin.x);
		msg.AddInt32("window-y", (int)origin.y);
    	_PostWindowEvent(msg);
		
		/* Perform normal hook operations */
    	BDirectWindow::FrameMoved(origin);
    }
    
    virtual void FrameResized(float width, float height) {
    	/* Post a message to the BApp so that it can handle the window event */
    	BMessage msg(BAPP_WINDOW_RESIZED);
		msg.AddInt32("window-w", (int)width) + 1;	/* TODO: Check that +1 is needed */
		msg.AddInt32("window-h", (int)height) + 1;
    	_PostWindowEvent(msg);
		
		/* Perform normal hook operations */
    	BDirectWindow::FrameResized(width, height);
    }

	virtual bool QuitRequested() {
    	BMessage msg(BAPP_WINDOW_CLOSE_REQUESTED);
    	_PostWindowEvent(msg);
    	
    	/* We won't allow a quit unless asked by DestroyWindow() */
    	return false;	
    }
    
    virtual void WindowActivated(bool active) {
    	BMessage msg(BAPP_KEYBOARD_FOCUS);	/* Mouse focus sold separately */
    	_PostWindowEvent(msg);
    }
    
    virtual void Zoom(BPoint origin,
				float width,
				float height) {
		BMessage msg(BAPP_MAXIMIZE);	/* Closest thing to maximization Haiku has */
    	_PostWindowEvent(msg);
    	
    	/* Before the window zooms, record its size */
    	if( !prev_frame )
    		prev_frame = new BRect(Frame());

    	/* Perform normal hook operations */
    	BDirectWindow::Zoom(origin, width, height);
    }
    
    /* Member functions */
    virtual void Show() {
    	BDirectWindow::Show();
    	_shown = true;
    	
    	BMessage msg(BAPP_SHOW);
    	_PostWindowEvent(msg);
    }
    
    virtual void Hide() {
    	/* FIXME: Multiple hides require multiple shows to undo. Should
    	   this be altered to prevent this from happening? */
    	BDirectWindow::Hide();
    	_shown = false;
    	
    	BMessage msg(BAPP_HIDE);
    	_PostWindowEvent(msg);
    }

    virtual void Minimize(bool minimize) {
    	BDirectWindow::Minimize(minimize);
    	int32 minState = (minimize ? BAPP_MINIMIZE : BAPP_RESTORE);
    	
    	BMessage msg(minState);
    	_PostWindowEvent(msg);
    }
    
    
    /* BView message interruption */
    virtual void DispatchMessage(BMessage * msg, BHandler * target)
    {
    	
        BPoint where;	/* Used by mouse moved */
        int32 buttons;	/* Used for mouse button events */
        int32 key;		/* Used for key events */
        
        switch (msg->what) {
        case B_MOUSE_MOVED:
            where;
            int32 transit;
            if (msg->FindPoint("where", &where) == B_OK
                && msg->FindInt32("be:transit", &transit) == B_OK) {
            	_MouseMotionEvent(where, transit);
            }
            
            /* FIXME: Apparently a button press/release event might be dropped
               if made before before a different button is released.  Does
               B_MOUSE_MOVED have the data needed to check if a mouse button
               state has changed? */
            if (msg->FindInt32("buttons", &buttons) == B_OK) {
				_MouseButtonEvent(buttons);
            }
            break;

        case B_MOUSE_DOWN:
        case B_MOUSE_UP:
            /* _MouseButtonEvent() detects any and all buttons that may have
               changed state, as well as that button's new state */
            if (msg->FindInt32("buttons", &buttons) == B_OK) {
				_MouseButtonEvent(buttons);
            }
            break;

        case B_MOUSE_WHEEL_CHANGED:
        	float x, y;
        	if (msg->FindFloat("be:wheel_delta_x", &x) == B_OK
        		&& msg->FindFloat("be:wheel_delta_y", &y) == B_OK) {
        			_MouseWheelEvent((int)x, (int)y);
        	}
        	break;

        case B_KEY_DOWN:
        case B_UNMAPPED_KEY_DOWN:      /* modifier keys are unmapped */
        	if (msg->FindInt32("key", &key) == B_OK) {
        		_KeyEvent((SDL_Scancode)key, SDL_PRESSED);
        	}
        	break;

        case B_KEY_UP:
        case B_UNMAPPED_KEY_UP:        /* modifier keys are unmapped */
        	if (msg->FindInt32("key", &key) == B_OK) {
            	_KeyEvent(key, SDL_RELEASED);
            }
            break;
                
        case _UPDATE_:
        case _UPDATE_IF_NEEDED_:	/* Hopefully one doesn't call the other */
        	_RepaintEvent();
        	break;
        	
        default:
            /* move it after switch{} so it's always handled
               that way we keep BeOS feautures like:
               - CTRL+Q to close window (and other shortcuts)
               - PrintScreen to make screenshot into /boot/home
               - etc.. */
            //BDirectWindow::DispatchMessage(msg, target);
            break;
        }
        BDirectWindow::DispatchMessage(msg, target);
    }
    
    /* Handle command messages */
    virtual void MessageReceived(BMessage* message) {
    	switch (message->what) {
    		/* Handle commands from SDL */
    		case BWIN_SET_TITLE:
    			_SetTitle(message);
    			break;
    		case BWIN_MOVE_WINDOW: 
    			_MoveTo(message);
    			break;
    		case BWIN_RESIZE_WINDOW:
    			_ResizeTo(message);
    			break;
    		case BWIN_SHOW_WINDOW:
    			Show();
    			break;
    		case BWIN_HIDE_WINDOW:
    			Hide();
    			break;
    		case BWIN_MAXIMIZE_WINDOW:
    			BWindow::Zoom();
    			break;
			case BWIN_MINIMIZE_WINDOW:
				Minimize(true);
    			break;
    		case BWIN_RESTORE_WINDOW:
    			_Restore();
    			break;
    		case BWIN_FULLSCREEN:
    			_SetFullScreen(message);
    			break;
    		default:
    			/* Perform normal message handling */
    			BDirectWindow::MessageReceived(message);
    			break;
    	}

    }
    
    

	/* Accessor methods */
	bool IsShown() { return _shown; }
	int32 GetID() { return _id; }
	
	/* Setter methods */
	void SetID(int32 id) { _id = id; }









	/* FIXME: Methods copied directly; do we need them? */
#if 0	/* Disabled until its purpose is determined */
    virtual void SetXYOffset(int x, int y)
    {
#if SDL_VIDEO_OPENGL
        if (the_view == SDL_GLView) {
            return;
        }
#endif
        SDL_View->SetXYOffset(x, y);
    }
    virtual void GetXYOffset(int &x, int &y)
    {
#if SDL_VIDEO_OPENGL
        if (the_view == SDL_GLView) {
            x = 0;
            y = 0;
            return;
        }
#endif
        SDL_View->GetXYOffset(x, y);
    }
#endif
    virtual bool BeginDraw(void)
    {
        return (Lock());
    }
    virtual void DrawAsync(BRect updateRect)
    {
        SDL_View->DrawAsync(updateRect);
    }
    virtual void EndDraw(void)
    {
        SDL_View->Sync();
        Unlock();
    }
#if SDL_VIDEO_OPENGL
    virtual void SwapBuffers(void)
    {
        SDL_GLView->UnlockGL();
        SDL_GLView->LockGL();
        SDL_GLView->SwapBuffers();
    }
#endif
    virtual BView *View(void)
    {
        return (the_view);
    }

	
	
	
	
	
	
private:
	/* Event redirection */
    void _MouseMotionEvent(BPoint &where, int32 transit) {
    	if(transit == B_EXITED_VIEW) {
    		/* Change mouse focus */
    		if(mouse_focused) {
    			_MouseFocusEvent(false);
    		}
    	} else {
    		static int x = 0, y = 0;
    		/* Change mouse focus */
    		if (!mouse_focused) {
    			_MouseFocusEvent(true);
    		}
//    		GetXYOffset(x, y);	//FIXME: What is this doing? (from SDL 1.2)
    		BMessage msg(BAPP_MOUSE_MOVED);
    		msg.AddInt32("dx", where.x - x);
    		msg.AddInt32("dy", where.y - y);
    		x = (int) where.x;
    		y = (int) where.y;
    		_PostWindowEvent(msg);
    	}
    }
    
    void _MouseFocusEvent(bool focusGained) {
    	mouse_focused = focusGained;
    	BMessage msg(BAPP_MOUSE_FOCUS);
    	msg.AddBool("focusGained", focusGained);
    	_PostWindowEvent(msg);
    	
//FIXME: Why were these here?
// if false: be_app->SetCursor(B_HAND_CURSOR);
// if true:  SDL_SetCursor(NULL);
    }
    
    void _MouseButtonEvent(int32 buttons) {
    	int32 buttonStateChange = buttons ^ last_buttons;
    	
    	/* Make sure at least one button has changed state */ 
    	if( !(buttonStateChange) ) {
    		return;
    	}
    	
    	/* Add any mouse button events */
    	if(buttonStateChange & B_PRIMARY_MOUSE_BUTTON) {
    		_SendMouseButton(SDL_BUTTON_LEFT, buttons &
    			B_PRIMARY_MOUSE_BUTTON);
    	}
    	if(buttonStateChange & B_SECONDARY_MOUSE_BUTTON) {
    		_SendMouseButton(SDL_BUTTON_RIGHT, buttons &
    			B_PRIMARY_MOUSE_BUTTON);
    	}
    	if(buttonStateChange & B_TERTIARY_MOUSE_BUTTON) {
    		_SendMouseButton(SDL_BUTTON_MIDDLE, buttons &
    			B_PRIMARY_MOUSE_BUTTON);
    	}
    	
    	last_buttons = buttons;
    }
    
    void _SendMouseButton(int32 button, int32 state) {
    	BMessage msg(BAPP_MOUSE_BUTTON);
    	msg.AddInt32("button-id", button);
    	msg.AddInt32("button-state", state);
    	_PostWindowEvent(msg);
    }
    
    void _MouseWheelEvent(int32 x, int32 y) {
    	/* Create a message to pass along to the BeApp thread */
    	BMessage msg(BAPP_MOUSE_WHEEL);
    	msg.AddInt32("xticks", x);
    	msg.AddInt32("yticks", y);
    	_PostWindowEvent(msg);
    }
    
    void _KeyEvent(int32 keyCode, int32 keyState) {
    	/* Create a message to pass along to the BeApp thread */
    	BMessage msg(BAPP_KEY);
    	msg.AddInt32("key-state", keyState);
    	msg.AddInt32("key-scancode", keyCode);
    	be_app->PostMessage(&msg);
    	/* Apparently SDL only uses the scancode */
    }
    
    void _RepaintEvent() {
    	/* Force a repaint: Call the SDL exposed event */
    	BMessage msg(BAPP_REPAINT);
    	_PostWindowEvent(msg);
    }
    void _PostWindowEvent(BMessage &msg) {
    	msg.AddInt32("window-id", _id);
    	be_app->PostMessage(&msg);
    }
    
	/* Command methods (functions called upon by SDL) */
    void _SetTitle(BMessage *msg) {
    	const char *title;
    	if(
			msg->FindString("window-title", &title) != B_OK
		) {
			return;
		}
		SetTitle(title);
    }
    
    void _MoveTo(BMessage *msg) {
    	int32 x, y;
    	if(
			msg->FindInt32("window-x", &x) != B_OK ||
			msg->FindInt32("window-y", &y) != B_OK
		) {
			return;
		}
		MoveTo(x, y);
    }
    
    void _ResizeTo(BMessage *msg) {
    	int32 w, h;
    	if(
			msg->FindInt32("window-w", &w) != B_OK ||
			msg->FindInt32("window-h", &h) != B_OK
		) {
			return;
		}
    	ResizeTo(w, h);
    }
    
    void _Restore() {
    	if(IsMinimized()) {
    		Minimize(false);
    	} else if(IsHidden()) {
    		Show();
    	} else if(prev_frame != NULL) {	/* Zoomed */
    		MoveTo(prev_frame->left, prev_frame->top);
    		ResizeTo(prev_frame->Width(), prev_frame->Height());
    	}
    }

    void _SetFullScreen(BMessage *msg) {
    	bool fullscreen;
    	if(
			msg->FindBool("fullscreen", &fullscreen) != B_OK
		) {
			return;
    	}
    	SetFullScreen(fullscreen);
    }
    
    /* Members */
#if SDL_VIDEO_OPENGL
    BGLView * SDL_GLView;
#endif
    SDL_BView *SDL_View;
    BView *the_view;
    
    int32 last_buttons;
    int32 _id;	/* Window id used by SDL_BApp */
    bool mouse_focused;		/* Does this window have mouse focus? */
    bool _shown;
    bool inhibit_resize;
    
    BRect *prev_frame;	/* Previous position and size of the window */
};

#endif
