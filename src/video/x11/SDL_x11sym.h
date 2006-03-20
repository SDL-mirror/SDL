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

SDL_X11_SYM(1,XClassHint*,XAllocClassHint,(void))
SDL_X11_SYM(1,Status,XAllocColor,(Display*,Colormap,XColor*))
SDL_X11_SYM(1,XSizeHints*,XAllocSizeHints,(void))
SDL_X11_SYM(1,XWMHints*,XAllocWMHints,(void))
SDL_X11_SYM(1,int,XChangePointerControl,(Display*,Bool,Bool,int,int,int))
SDL_X11_SYM(1,int,XChangeProperty,(Display*,Window,Atom,Atom,int,int,_Xconst unsigned char*,int))
SDL_X11_SYM(1,int,XChangeWindowAttributes,(Display*,Window,unsigned long,XSetWindowAttributes*))
SDL_X11_SYM(1,Bool,XCheckTypedEvent,(Display*,int,XEvent*))
SDL_X11_SYM(1,int,XClearWindow,(Display*,Window))
SDL_X11_SYM(1,int,XCloseDisplay,(Display*))
SDL_X11_SYM(1,Colormap,XCreateColormap,(Display*,Window,Visual*,int))
SDL_X11_SYM(1,Cursor,XCreatePixmapCursor,(Display*,Pixmap,Pixmap,XColor*,XColor*,unsigned int,unsigned int))
SDL_X11_SYM(1,GC,XCreateGC,(Display*,Drawable,unsigned long,XGCValues*))
SDL_X11_SYM(1,XImage*,XCreateImage,(Display*,Visual*,unsigned int,int,int,char*,unsigned int,unsigned int,int,int))
SDL_X11_SYM(1,Pixmap,XCreatePixmap,(Display*,Drawable,unsigned int,unsigned int,unsigned int))
SDL_X11_SYM(1,Pixmap,XCreatePixmapFromBitmapData,(Display*,Drawable,char*,unsigned int,unsigned int,unsigned long,unsigned long,unsigned int))
SDL_X11_SYM(1,Window,XCreateSimpleWindow,(Display*,Window,int,int,unsigned int,unsigned int,unsigned int,unsigned long,unsigned long))
SDL_X11_SYM(1,Window,XCreateWindow,(Display*,Window,int,int,unsigned int,unsigned int,unsigned int,int,unsigned int,Visual*,unsigned long,XSetWindowAttributes*))
SDL_X11_SYM(1,int,XDefineCursor,(Display*,Window,Cursor))
SDL_X11_SYM(1,int,XDeleteProperty,(Display*,Window,Atom))
SDL_X11_SYM(1,int,XDestroyWindow,(Display*,Window))
SDL_X11_SYM(1,char*,XDisplayName,(_Xconst char*))
SDL_X11_SYM(1,int,XEventsQueued,(Display*,int))
SDL_X11_SYM(1,Bool,XFilterEvent,(XEvent *event, Window w))
SDL_X11_SYM(1,int,XFlush,(Display*))
SDL_X11_SYM(1,int,XFree,(void*))
SDL_X11_SYM(1,int,XFreeColormap,(Display*,Colormap))
SDL_X11_SYM(1,int,XFreeColors,(Display*,Colormap,unsigned long*,int,unsigned long))
SDL_X11_SYM(1,int,XFreeCursor,(Display*,Cursor))
SDL_X11_SYM(1,int,XFreeGC,(Display*,GC))
SDL_X11_SYM(1,int,XFreeModifiermap,(XModifierKeymap*))
SDL_X11_SYM(1,int,XFreePixmap,(Display*,Pixmap))
SDL_X11_SYM(1,int,XGetErrorDatabaseText,(Display*,_Xconst char*,_Xconst char*,_Xconst char*,char*,int))
SDL_X11_SYM(1,XModifierKeymap*,XGetModifierMapping,(Display*))
SDL_X11_SYM(1,int,XGetPointerControl,(Display*,int*,int*,int*))
SDL_X11_SYM(1,XVisualInfo*,XGetVisualInfo,(Display*,long,XVisualInfo*,int*))
SDL_X11_SYM(1,XWMHints*,XGetWMHints,(Display*,Window))
SDL_X11_SYM(1,Status,XGetTextProperty,(Display*,Window,XTextProperty*,Atom))
SDL_X11_SYM(1,Status,XGetWindowAttributes,(Display*,Window,XWindowAttributes*))
SDL_X11_SYM(1,int,XGrabKeyboard,(Display*,Window,Bool,int,int,Time))
SDL_X11_SYM(1,int,XGrabPointer,(Display*,Window,Bool,unsigned int,int,int,Window,Cursor,Time))
SDL_X11_SYM(1,Status,XIconifyWindow,(Display*,Window,int))
SDL_X11_SYM(1,int,XInstallColormap,(Display*,Colormap))
SDL_X11_SYM(1,KeyCode,XKeysymToKeycode,(Display*,KeySym))
SDL_X11_SYM(1,Atom,XInternAtom,(Display*,_Xconst char*,Bool))
SDL_X11_SYM(1,XPixmapFormatValues*,XListPixmapFormats,(Display*,int*))
SDL_X11_SYM(1,int,XLookupString,(XKeyEvent*,char*,int,KeySym*,XComposeStatus*))
SDL_X11_SYM(1,int,XMapRaised,(Display*,Window))
SDL_X11_SYM(1,int,XMapWindow,(Display*,Window))
SDL_X11_SYM(1,int,XMaskEvent,(Display*,long,XEvent*))
SDL_X11_SYM(1,Status,XMatchVisualInfo,(Display*,int,int,int,XVisualInfo*))
SDL_X11_SYM(1,int,XMissingExtension,(Display*,_Xconst char*))
SDL_X11_SYM(1,int,XMoveResizeWindow,(Display*,Window,int,int,unsigned int,unsigned int))
SDL_X11_SYM(1,int,XMoveWindow,(Display*,Window,int,int))
SDL_X11_SYM(1,int,XNextEvent,(Display*,XEvent*))
SDL_X11_SYM(1,Display*,XOpenDisplay,(_Xconst char*))
SDL_X11_SYM(1,int,XPeekEvent,(Display*,XEvent*))
SDL_X11_SYM(1,int,XPending,(Display*))
SDL_X11_SYM(1,int,XPutImage,(Display*,Drawable,GC,XImage*,int,int,int,int,unsigned int,unsigned int))
SDL_X11_SYM(1,int,XQueryColors,(Display*,Colormap,XColor*,int))
SDL_X11_SYM(1,int,XQueryKeymap,(Display*,char [32]))
SDL_X11_SYM(1,Bool,XQueryPointer,(Display*,Window,Window*,Window*,int*,int*,int*,int*,unsigned int*))
SDL_X11_SYM(1,int,XRaiseWindow,(Display*,Window))
SDL_X11_SYM(1,int,XReparentWindow,(Display*,Window,Window,int,int))
SDL_X11_SYM(1,int,XResizeWindow,(Display*,Window,unsigned int,unsigned int))
SDL_X11_SYM(1,int,XSelectInput,(Display*,Window,long))
SDL_X11_SYM(1,Status,XSendEvent,(Display*,Window,Bool,long,XEvent*))
SDL_X11_SYM(1,int,XSetClassHint,(Display*,Window,XClassHint*))
SDL_X11_SYM(1,XErrorHandler,XSetErrorHandler,(XErrorHandler))
SDL_X11_SYM(1,XIOErrorHandler,XSetIOErrorHandler,(XIOErrorHandler))
SDL_X11_SYM(1,int,XSetTransientForHint,(Display*,Window,Window))
SDL_X11_SYM(1,int,XSetWMHints,(Display*,Window,XWMHints*))
SDL_X11_SYM(1,void,XSetTextProperty,(Display*,Window,XTextProperty*,Atom))
SDL_X11_SYM(1,void,XSetWMNormalHints,(Display*,Window,XSizeHints*))
SDL_X11_SYM(1,Status,XSetWMProtocols,(Display*,Window,Atom*,int))
SDL_X11_SYM(1,int,XSetWindowBackground,(Display*,Window,unsigned long))
SDL_X11_SYM(1,int,XSetWindowBackgroundPixmap,(Display*,Window,Pixmap))
SDL_X11_SYM(1,int,XSetWindowColormap,(Display*,Window,Colormap))
SDL_X11_SYM(1,int,XStoreColors,(Display*,Colormap,XColor*,int))
SDL_X11_SYM(1,Status,XStringListToTextProperty,(char**,int,XTextProperty*))
SDL_X11_SYM(1,int,XSync,(Display*,Bool))
SDL_X11_SYM(1,int,XUngrabKeyboard,(Display*,Time))
SDL_X11_SYM(1,int,XUngrabPointer,(Display*,Time))
SDL_X11_SYM(1,int,XUnmapWindow,(Display*,Window))
SDL_X11_SYM(1,int,XWarpPointer,(Display*,Window,Window,int,int,unsigned int,unsigned int,int,int))
SDL_X11_SYM(1,VisualID,XVisualIDFromVisual,(Visual*))
SDL_X11_SYM(1,XExtDisplayInfo*,XextAddDisplay,(XExtensionInfo*,Display*,char*,XExtensionHooks*,int,XPointer))
SDL_X11_SYM(1,XExtensionInfo*,XextCreateExtension,(void))
SDL_X11_SYM(1,void,XextDestroyExtension,(XExtensionInfo*))
SDL_X11_SYM(1,XExtDisplayInfo*,XextFindDisplay,(XExtensionInfo*,Display*))
SDL_X11_SYM(1,int,XextRemoveDisplay,(XExtensionInfo*,Display*))
SDL_X11_SYM(1,Bool,XQueryExtension,(Display*,_Xconst char*,int*,int*,int*))
SDL_X11_SYM(1,char *,XDisplayString,(Display*))
SDL_X11_SYM(1,int,XGetErrorText,(Display*,int,char*,int))

#ifdef X_HAVE_UTF8_STRING
SDL_X11_SYM(1,int,Xutf8TextListToTextProperty,(Display*,char**,int,XICCEncodingStyle,XTextProperty*))
SDL_X11_SYM(1,int,Xutf8LookupString,(XIC,XKeyPressedEvent*,char*,int,KeySym*,Status*))
SDL_X11_SYM(1,XIC,XCreateIC,(XIM, ...))
SDL_X11_SYM(1,void,XDestroyIC,(XIC))
SDL_X11_SYM(1,void,XSetICFocus,(XIC))
SDL_X11_SYM(1,void,XUnsetICFocus,(XIC))
SDL_X11_SYM(1,XIM,XOpenIM,(Display*,struct _XrmHashBucketRec*,char*,char*))
SDL_X11_SYM(1,Status,XCloseIM,(XIM))
#endif
SDL_X11_SYM(1,void,_XEatData,(Display*,unsigned long))
SDL_X11_SYM(1,void,_XFlush,(Display*))
SDL_X11_SYM(1,void,_XFlushGCCache,(Display*,GC))
SDL_X11_SYM(1,int,_XRead,(Display*,char*,long))
SDL_X11_SYM(1,void,_XReadPad,(Display*,char*,long))
SDL_X11_SYM(1,void,_XSend,(Display*,_Xconst char*,long))
SDL_X11_SYM(1,Status,_XReply,(Display*,xReply*,int,Bool))
SDL_X11_SYM(1,unsigned long,_XSetLastRequestRead,(Display*,xGenericReply*))

/*
 * These don't exist in 32-bit versions and are removed by Xlib macros, but
 *  64+ bit systems will use them.
 */
#if defined(LONG64)
SDL_X11_SYM(1,int,_XData32,(Display *dpy,register long *data,unsigned len))
SDL_X11_SYM(1,void,_XRead32,(Display *dpy,register long *data,long len))
#endif

#if defined(__osf__)
SDL_X11_SYM(1,void,_SmtBufferOverflow,(Display *dpy,register smtDisplayPtr))
SDL_X11_SYM(1,void,_SmtIpError,(Display *dpy,register smtDisplayPtr, int))
SDL_X11_SYM(1,int,ipAllocateData,(ChannelPtr, IPCard, IPDataPtr *))
SDL_X11_SYM(1,int,ipUnallocateAndSendData,(ChannelPtr, IPCard))
#endif

#if NeedWidePrototypes
SDL_X11_SYM(1,KeySym,XKeycodeToKeysym,(Display*,unsigned int,int))
#else
SDL_X11_SYM(1,KeySym,XKeycodeToKeysym,(Display*,KeyCode,int))
#endif

#ifndef NO_SHARED_MEMORY
SDL_X11_SYM(1,Status,XShmAttach,(Display*,XShmSegmentInfo*))
SDL_X11_SYM(1,Status,XShmDetach,(Display*,XShmSegmentInfo*))
SDL_X11_SYM(1,Status,XShmPutImage,(Display*,Drawable,GC,XImage*,int,int,int,int,unsigned int,unsigned int,Bool))
SDL_X11_SYM(1,XImage*,XShmCreateImage,(Display*,Visual*,unsigned int,int,char*,XShmSegmentInfo*,unsigned int,unsigned int))
SDL_X11_SYM(1,Bool,XShmQueryExtension,(Display*))
#endif

SDL_X11_SYM(1,SDL_X11_XSynchronizeRetType,XSynchronize,(Display*,Bool))
SDL_X11_SYM(1,SDL_X11_XESetWireToEventRetType,XESetWireToEvent,(Display*,int,SDL_X11_XESetWireToEventRetType))
SDL_X11_SYM(1,SDL_X11_XESetEventToWireRetType,XESetEventToWire,(Display*,int,SDL_X11_XESetEventToWireRetType))
SDL_X11_SYM(1,SDL_X11_XSetExtensionErrorHandlerType,XSetExtensionErrorHandler,(SDL_X11_XSetExtensionErrorHandlerType))

/* end of SDL_x11sym.h ... */

