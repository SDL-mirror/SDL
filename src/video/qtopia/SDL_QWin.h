/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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

#ifndef _SDL_QWin_h
#define _SDL_QWin_h
#include <stdio.h>

#include <qimage.h>
#include <qpixmap.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qdirectpainter_qws.h>

#include "SDL_events.h"
//#include "SDL_BView.h"

extern "C" {
#include "SDL_events_c.h"
};

class SDL_QWin : public QWidget
{
  void QueueKey(QKeyEvent *e, int pressed);
 public:
  SDL_QWin(const QSize& size);
  virtual ~SDL_QWin();
  virtual bool shown(void) {
    return isVisible();
  }
  /* If called, the next resize event will not be forwarded to SDL. */
  virtual void inhibitResize(void) {
    my_inhibit_resize = true;
  }
  void setImage(QImage *image);
  void setOffset(int x, int y) {
    my_offset = QPoint(x, y);
  }
  void GetXYOffset(int &x, int &y) {
    x = my_offset.x();
    y = my_offset.y();
  }
  bool beginDraw(void) {
    return true;
  }
  void endDraw(void) {
  }
  QImage *image(void) {
    return my_image;
  }
  
  void setWFlags(WFlags flags) {
    QWidget::setWFlags(flags);
    my_flags = flags;
  }
  const QPoint& mousePos() const { return my_mouse_pos; }
  void setMousePos(const QPoint& newpos) { my_mouse_pos = newpos; }
  void setFullscreen(bool);

  void lockScreen() {
    if(!my_painter) {
      my_painter = new QDirectPainter(this);
    }
    my_locked++; // Increate lock refcount
  }
  void unlockScreen() {
    if(my_locked > 0) {
      my_locked--; // decrease lock refcount;
    }
    if(!my_locked && my_painter) {
      my_painter->end();
      delete my_painter;
      my_painter = 0;
    }
  }
  void repaintRect(const QRect& rect);
 protected:
  /* Handle resizing of the window */
  virtual void resizeEvent(QResizeEvent *e);
  void focusInEvent(QFocusEvent *);
  void focusOutEvent(QFocusEvent *);
  void closeEvent(QCloseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);
  void mousePressEvent(QMouseEvent *e);
  void mouseReleaseEvent(QMouseEvent *e);
  void paintEvent(QPaintEvent *ev);
  void keyPressEvent(QKeyEvent *e) {
    QueueKey(e, 1);
  }
  void keyReleaseEvent(QKeyEvent *e) {
    QueueKey(e, 0);
  }
 private:
  void enableFullscreen();
  QDirectPainter *my_painter;
  QImage *my_image;
  bool my_inhibit_resize;
  QPoint my_offset;
  QPoint my_mouse_pos;
  WFlags my_flags;
  WFlags my_has_fullscreen;
  unsigned int my_locked;
};

#endif /* _SDL_QWin_h */
