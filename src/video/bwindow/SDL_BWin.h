/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

#ifndef _SDL_BWin_h
#define _SDL_BWin_h

#include <stdio.h>
#include <AppKit.h>
#include <InterfaceKit.h>
#include <be/game/DirectWindow.h>
#ifdef HAVE_OPENGL
#include <be/opengl/GLView.h>
#endif

#include "SDL_BeApp.h"
#include "SDL_events.h"
#include "SDL_BView.h"

extern "C" {
#include "SDL_events_c.h"
};

class SDL_BWin : public BDirectWindow
{
public:
	SDL_BWin(BRect bounds) :
			BDirectWindow(bounds, "Untitled", B_TITLED_WINDOW, 0) {
		the_view = NULL;
#ifdef HAVE_OPENGL
		SDL_GLView = NULL;
#endif
		SDL_View = NULL;
		Unlock();
		shown = false;
		inhibit_resize = false;
	}
	virtual ~SDL_BWin() {
		Lock();
		if ( the_view ) {
#ifdef HAVE_OPENGL
			if ( the_view == SDL_GLView ) {
				SDL_GLView->UnlockGL();
			}
#endif
			RemoveChild(the_view);
			the_view = NULL;
		}
		Unlock();
#ifdef HAVE_OPENGL
		if ( SDL_GLView ) {
			delete SDL_GLView;
		}
#endif
		if ( SDL_View ) {
			delete SDL_View;
		}
	}

	/* Override the Show() method so we can tell when we've been shown */
	virtual void Show(void) {
		BWindow::Show();
		shown = true;
	}
	virtual bool Shown(void) {
		return (shown);
	}
	/* If called, the next resize event will not be forwarded to SDL. */
	virtual void InhibitResize(void) {
		inhibit_resize=true;
	}
	/* Handle resizing of the window */
	virtual void FrameResized(float width, float height) {
		if(inhibit_resize)
			inhibit_resize = false;
		else 
			SDL_PrivateResize((int)width, (int)height);
	}
	virtual int CreateView(Uint32 flags) {
		int retval;

		retval = 0;
		Lock();
		if ( flags & SDL_OPENGL ) {
#ifdef HAVE_OPENGL
			if ( SDL_GLView == NULL ) {
				/* FIXME: choose BGL type via user flags */
				SDL_GLView = new BGLView(Bounds(), "SDL GLView",
					 	B_FOLLOW_ALL_SIDES,
                                        	(B_WILL_DRAW|B_FRAME_EVENTS),
						(BGL_RGB|BGL_DOUBLE|BGL_DEPTH));
			}
			if ( the_view != SDL_GLView ) {
				if ( the_view ) {
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
			if ( SDL_View == NULL ) {
				SDL_View = new SDL_BView(Bounds());
			}
			if ( the_view != SDL_View ) {
				if ( the_view ) {
#ifdef HAVE_OPENGL
					if ( the_view == SDL_GLView ) {
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
		return(retval);
	}
	virtual void SetBitmap(BBitmap *bitmap) {
		SDL_View->SetBitmap(bitmap);
	}
	virtual void SetXYOffset(int x, int y) {
#ifdef HAVE_OPENGL
		if ( the_view == SDL_GLView ) {
			return;
		}
#endif
		SDL_View->SetXYOffset(x, y);		
	}
	virtual void GetXYOffset(int &x, int &y) {
#ifdef HAVE_OPENGL
		if ( the_view == SDL_GLView ) {
			x = 0;
			y = 0;
			return;
		}
#endif
		SDL_View->GetXYOffset(x, y);
	}
	virtual bool BeginDraw(void) {
		return(Lock());
	}
	virtual void DrawAsync(BRect updateRect) {
		SDL_View->DrawAsync(updateRect);
	}
	virtual void EndDraw(void) {
		SDL_View->Sync();
		Unlock();
	}
#ifdef HAVE_OPENGL
	virtual void SwapBuffers(void) {
		SDL_GLView->UnlockGL();
		SDL_GLView->LockGL();
		SDL_GLView->SwapBuffers();
	}
#endif
	virtual BView *View(void) {
		return(the_view);
	}

	/* Hook functions -- overridden */
	virtual void Minimize(bool minimize) {
		/* This is only called when mimimized, not when restored */
		//SDL_PrivateAppActive(minimize, SDL_APPACTIVE);
		BWindow::Minimize(minimize);
	}
	virtual void WindowActivated(bool active) {
		SDL_PrivateAppActive(active, SDL_APPINPUTFOCUS);
	}
	virtual bool QuitRequested(void) {
		if ( SDL_BeAppActive > 0 ) {
			SDL_PrivateQuit();
			/* We don't ever actually close the window here because
			   the application should respond to the quit request,
			   or ignore it as desired.
			 */
			return(false);
		}
		return(true);	/* Close the app window */
	}

private:
#ifdef HAVE_OPENGL
	BGLView *SDL_GLView;
#endif
	SDL_BView *SDL_View;
	BView *the_view;

	bool shown;
	bool inhibit_resize;
};

#endif /* _SDL_BWin_h */
