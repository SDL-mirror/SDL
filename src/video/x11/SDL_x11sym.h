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

SDL_X11_SYM(XClassHint*,XAllocClassHint,(void))
SDL_X11_SYM(Status,XAllocColor,(Display*,Colormap,XColor*))
SDL_X11_SYM(XSizeHints*,XAllocSizeHints,(void))
SDL_X11_SYM(XWMHints*,XAllocWMHints,(void))
SDL_X11_SYM(int,XChangePointerControl,(Display*,Bool,Bool,int,int,int))
SDL_X11_SYM(int,XChangeProperty,(Display*,Window,Atom,Atom,int,int,_Xconst unsigned char*,int))
SDL_X11_SYM(int,XChangeWindowAttributes,(Display*,Window,unsigned long,XSetWindowAttributes*))
SDL_X11_SYM(Bool,XCheckTypedEvent,(Display*,int,XEvent*))
SDL_X11_SYM(int,XClearWindow,(Display*,Window))
SDL_X11_SYM(int,XCloseDisplay,(Display*))
SDL_X11_SYM(Colormap,XCreateColormap,(Display*,Window,Visual*,int))
SDL_X11_SYM(Cursor,XCreatePixmapCursor,(Display*,Pixmap,Pixmap,XColor*,XColor*,unsigned int,unsigned int))
SDL_X11_SYM(GC,XCreateGC,(Display*,Drawable,unsigned long,XGCValues*))
SDL_X11_SYM(XImage*,XCreateImage,(Display*,Visual*,unsigned int,int,int,char*,unsigned int,unsigned int,int,int))
SDL_X11_SYM(Pixmap,XCreatePixmap,(Display*,Drawable,unsigned int,unsigned int,unsigned int))
SDL_X11_SYM(Pixmap,XCreatePixmapFromBitmapData,(Display*,Drawable,char*,unsigned int,unsigned int,unsigned long,unsigned long,unsigned int))
SDL_X11_SYM(Window,XCreateSimpleWindow,(Display*,Window,int,int,unsigned int,unsigned int,unsigned int,unsigned long,unsigned long))
SDL_X11_SYM(Window,XCreateWindow,(Display*,Window,int,int,unsigned int,unsigned int,unsigned int,int,unsigned int,Visual*,unsigned long,XSetWindowAttributes*))
SDL_X11_SYM(int,XDefineCursor,(Display*,Window,Cursor))
SDL_X11_SYM(int,XDeleteProperty,(Display*,Window,Atom))
SDL_X11_SYM(int,XDestroyWindow,(Display*,Window))
SDL_X11_SYM(char*,XDisplayName,(_Xconst char*))
SDL_X11_SYM(int,XEventsQueued,(Display*,int))
SDL_X11_SYM(int,XFlush,(Display*))
SDL_X11_SYM(int,XFree,(void*))
SDL_X11_SYM(int,XFreeColormap,(Display*,Colormap))
SDL_X11_SYM(int,XFreeColors,(Display*,Colormap,unsigned long*,int,unsigned long))
SDL_X11_SYM(int,XFreeCursor,(Display*,Cursor))
SDL_X11_SYM(int,XFreeGC,(Display*,GC))
SDL_X11_SYM(int,XFreeModifiermap,(XModifierKeymap*))
SDL_X11_SYM(int,XFreePixmap,(Display*,Pixmap))
SDL_X11_SYM(int,XGetErrorDatabaseText,(Display*,_Xconst char*,_Xconst char*,_Xconst char*,char*,int))
SDL_X11_SYM(XModifierKeymap*,XGetModifierMapping,(Display*))
SDL_X11_SYM(int,XGetPointerControl,(Display*,int*,int*,int*))
SDL_X11_SYM(XVisualInfo*,XGetVisualInfo,(Display*,long,XVisualInfo*,int*))
SDL_X11_SYM(XWMHints*,XGetWMHints,(Display*,Window))
SDL_X11_SYM(Status,XGetWMIconName,(Display*,Window,XTextProperty*))
SDL_X11_SYM(Status,XGetWMName,(Display*,Window,XTextProperty*))
SDL_X11_SYM(Status,XGetWindowAttributes,(Display*,Window,XWindowAttributes*))
SDL_X11_SYM(int,XGrabKeyboard,(Display*,Window,Bool,int,int,Time))
SDL_X11_SYM(int,XGrabPointer,(Display*,Window,Bool,unsigned int,int,int,Window,Cursor,Time))
SDL_X11_SYM(Status,XIconifyWindow,(Display*,Window,int))
SDL_X11_SYM(int,XInstallColormap,(Display*,Colormap))
SDL_X11_SYM(KeyCode,XKeysymToKeycode,(Display*,KeySym))
SDL_X11_SYM(Atom,XInternAtom,(Display*,_Xconst char*,Bool))
SDL_X11_SYM(XPixmapFormatValues*,XListPixmapFormats,(Display*,int*))
SDL_X11_SYM(int,XLookupString,(XKeyEvent*,char*,int,KeySym*,XComposeStatus*))
SDL_X11_SYM(int,XMapRaised,(Display*,Window))
SDL_X11_SYM(int,XMapWindow,(Display*,Window))
SDL_X11_SYM(int,XMaskEvent,(Display*,long,XEvent*))
SDL_X11_SYM(Status,XMatchVisualInfo,(Display*,int,int,int,XVisualInfo*))
SDL_X11_SYM(int,XMissingExtension,(Display*,_Xconst char*))
SDL_X11_SYM(int,XMoveResizeWindow,(Display*,Window,int,int,unsigned int,unsigned int))
SDL_X11_SYM(int,XMoveWindow,(Display*,Window,int,int))
SDL_X11_SYM(int,XNextEvent,(Display*,XEvent*))
SDL_X11_SYM(Display*,XOpenDisplay,(_Xconst char*))
SDL_X11_SYM(int,XPeekEvent,(Display*,XEvent*))
SDL_X11_SYM(int,XPending,(Display*))
SDL_X11_SYM(int,XPutImage,(Display*,Drawable,GC,XImage*,int,int,int,int,unsigned int,unsigned int))
SDL_X11_SYM(int,XQueryColors,(Display*,Colormap,XColor*,int))
SDL_X11_SYM(int,XQueryKeymap,(Display*,char [32]))
SDL_X11_SYM(Bool,XQueryPointer,(Display*,Window,Window*,Window*,int*,int*,int*,int*,unsigned int*))
SDL_X11_SYM(int,XRaiseWindow,(Display*,Window))
SDL_X11_SYM(int,XReparentWindow,(Display*,Window,Window,int,int))
SDL_X11_SYM(int,XResizeWindow,(Display*,Window,unsigned int,unsigned int))
SDL_X11_SYM(int,XSelectInput,(Display*,Window,long))
SDL_X11_SYM(Status,XSendEvent,(Display*,Window,Bool,long,XEvent*))
SDL_X11_SYM(int,XSetClassHint,(Display*,Window,XClassHint*))
SDL_X11_SYM(XErrorHandler,XSetErrorHandler,(XErrorHandler))
SDL_X11_SYM(XIOErrorHandler,XSetIOErrorHandler,(XIOErrorHandler))
SDL_X11_SYM(int,XSetTransientForHint,(Display*,Window,Window))
SDL_X11_SYM(int,XSetWMHints,(Display*,Window,XWMHints*))
SDL_X11_SYM(void,XSetWMIconName,(Display*,Window,XTextProperty*))
SDL_X11_SYM(void,XSetWMName,(Display*,Window,XTextProperty*))
SDL_X11_SYM(void,XSetWMNormalHints,(Display*,Window,XSizeHints*))
SDL_X11_SYM(Status,XSetWMProtocols,(Display*,Window,Atom*,int))
SDL_X11_SYM(int,XSetWindowBackground,(Display*,Window,unsigned long))
SDL_X11_SYM(int,XSetWindowBackgroundPixmap,(Display*,Window,Pixmap))
SDL_X11_SYM(int,XSetWindowColormap,(Display*,Window,Colormap))
SDL_X11_SYM(int,XStoreColors,(Display*,Colormap,XColor*,int))
SDL_X11_SYM(Status,XStringListToTextProperty,(char**,int,XTextProperty*))
SDL_X11_SYM(int,XSync,(Display*,Bool))
SDL_X11_SYM(int,XUngrabKeyboard,(Display*,Time))
SDL_X11_SYM(int,XUngrabPointer,(Display*,Time))
SDL_X11_SYM(int,XUnmapWindow,(Display*,Window))
SDL_X11_SYM(int,XWarpPointer,(Display*,Window,Window,int,int,unsigned int,unsigned int,int,int))
SDL_X11_SYM(VisualID,XVisualIDFromVisual,(Visual*))
SDL_X11_SYM(XExtDisplayInfo*,XextAddDisplay,(XExtensionInfo*,Display*,char*,XExtensionHooks*,int,XPointer))
SDL_X11_SYM(XExtensionInfo*,XextCreateExtension,(void))
SDL_X11_SYM(void,XextDestroyExtension,(XExtensionInfo*))
SDL_X11_SYM(XExtDisplayInfo*,XextFindDisplay,(XExtensionInfo*,Display*))
SDL_X11_SYM(int,XextRemoveDisplay,(XExtensionInfo*,Display*))
#ifdef X_HAVE_UTF8_STRING
SDL_X11_SYM(int,Xutf8TextListToTextProperty,(Display*,char**,int,XICCEncodingStyle,XTextProperty*))
#endif
SDL_X11_SYM(void,_XEatData,(Display*,unsigned long))
SDL_X11_SYM(void,_XFlush,(Display*))
SDL_X11_SYM(void,_XFlushGCCache,(Display*,GC))
SDL_X11_SYM(int,_XRead,(Display*,char*,long))
SDL_X11_SYM(void,_XReadPad,(Display*,char*,long))
SDL_X11_SYM(void,_XSend,(Display*,_Xconst char*,long))
SDL_X11_SYM(Status,_XReply,(Display*,xReply*,int,Bool))
SDL_X11_SYM(unsigned long,_XSetLastRequestRead,(Display*,xGenericReply*))

#if NeedWidePrototypes
SDL_X11_SYM(KeySym,XKeycodeToKeysym,(Display*,unsigned int,int))
#else
SDL_X11_SYM(KeySym,XKeycodeToKeysym,(Display*,KeyCode,int))
#endif

#ifndef NO_SHARED_MEMORY
SDL_X11_SYM(Status,XShmAttach,(Display*,XShmSegmentInfo*))
SDL_X11_SYM(Status,XShmDetach,(Display*,XShmSegmentInfo*))
SDL_X11_SYM(Status,XShmPutImage,(Display*,Drawable,GC,XImage*,int,int,int,int,unsigned int,unsigned int,Bool))
SDL_X11_SYM(XImage*,XShmCreateImage,(Display*,Visual*,unsigned int,int,char*,XShmSegmentInfo*,unsigned int,unsigned int))
SDL_X11_SYM(Bool,XShmQueryExtension,(Display*))
#endif

SDL_X11_SYM(SDL_X11_XSynchronizeRetType,XSynchronize,(Display*,Bool))
SDL_X11_SYM(SDL_X11_XESetWireToEventRetType,XESetWireToEvent,(Display*,int,SDL_X11_XESetWireToEventRetType))
SDL_X11_SYM(SDL_X11_XESetEventToWireRetType,XESetEventToWire,(Display*,int,SDL_X11_XESetEventToWireRetType))

/* end of SDL_x11sym.h ... */

