/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

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

#include "SDL_config.h"

/* wrapper functions for MiniGL */

#if SDL_VIDEO_DRIVER_AMIGAOS4

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#define NOT_IMPLEMENTED_FUNCS

#include <string.h>

/* The GL API */

static void AmiglActiveTexture(GLenum unit) {
    return glActiveTexture(unit);
}

static void AmiglClientActiveTexture(GLenum texture) {
    return glClientActiveTexture(texture);
}

static void AmiglColorTable(GLenum target, GLenum internalformat, GLint width, GLenum format, GLenum type, const GLvoid* data) {
    return glColorTable(target, internalformat, width, format, type, data);
}

static void AmiglColorTableEXT(GLenum target, GLenum internalformat, GLint width, GLenum format, GLenum type, const GLvoid* data) {
    return glColorTableEXT(target, internalformat, width, format, type, data);
}

static void AmiglClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
    return glClearColor(red, green, blue, alpha);
}

static void AmiglClear( GLbitfield mask ) {
    return glClear(mask);
}

static void AmiglColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
    return glColorMask(red, green, blue, alpha);
}

static void AmiglAlphaFunc( GLenum func, GLclampf ref ) {
    return glAlphaFunc(func, ref);
}

static void AmiglBlendFunc( GLenum sfactor, GLenum dfactor ) {
    return glBlendFunc(sfactor, dfactor);
}

static void AmiglLogicOp( GLenum opcode ) {
    return glLogicOp(opcode);
}

static void AmiglCullFace( GLenum mode ) {
    return glCullFace(mode);
}

static void AmiglFrontFace( GLenum mode ) {
    return glFrontFace(mode);
}

static void AmiglPointSize( GLfloat size ) {
    return glPointSize(size);
}

static void AmiglLineWidth( GLfloat width ) {
    return glLineWidth(width);
}

static void AmiglLineStipple( GLint factor, GLushort pattern ) {
    return glLineStipple(factor, pattern);
}

static void AmiglPolygonMode( GLenum face, GLenum mode ) {
    return glPolygonMode(face, mode);
}

static void AmiglPolygonOffset( GLfloat factor, GLfloat units ) {
    return glPolygonOffset(factor, units);
}

static void AmiglPolygonStipple( GLubyte *mask ) {
    return glPolygonStipple(mask);
}

static void AmiglEdgeFlag( GLboolean flag ) {
    return glEdgeFlag(flag);
}

static void AmiglScissor( GLint x, GLint y, GLsizei width, GLsizei height) {
    return glScissor(x, y, width, height);
}

static void AmiglClipPlane( GLenum plane, const GLdouble *equation ) {
    return glClipPlane(plane, (GLdouble *)equation);
}

static void AmiglGetClipPlane( GLenum plane, GLdouble *equation ) {
    return glGetClipPlane(plane, equation);
}

static void AmiglDrawBuffer( GLenum mode ) {
    return glDrawBuffer(mode);
}

static void AmiglReadBuffer( GLenum mode ) {
    return glReadBuffer(mode);
}

static void AmiglEnable( GLenum cap ) {
    return glEnable(cap);
}

static void AmiglDisable( GLenum cap ) {
    return glDisable(cap);
}

static GLboolean AmiglIsEnabled( GLenum cap ) {
    return glIsEnabled(cap);
}

static void AmiglEnableClientState( GLenum cap ) {  /* 1.1 */
    return glEnableClientState(cap);
}

static void AmiglDisableClientState( GLenum cap ) {  /* 1.1 */
    return glDisableClientState(cap);
}

static void AmiglGetBooleanv( GLenum pname, GLboolean *params ) {
    return glGetBooleanv(pname, params);
}

static void AmiglGetDoublev( GLenum pname, GLdouble *params ) {
    return glGetDoublev(pname, params);
}

static void AmiglGetFloatv( GLenum pname, GLfloat *params ) {
    return glGetFloatv(pname, params);
}

static void AmiglGetIntegerv( GLenum pname, GLint *params ) {
    return glGetIntegerv(pname, params);
}

static void AmiglPushAttrib( GLbitfield mask ) {
    return glPushAttrib(mask);
}

static void AmiglPopAttrib( void ) {
    return glPopAttrib();
}

static void AmiglPushClientAttrib( GLbitfield mask ) {  /* 1.1 */
    return glPushClientAttrib(mask);
}

static void AmiglPopClientAttrib( void ) {  /* 1.1 */
    return glPopClientAttrib();
}

static GLint AmiglRenderMode( GLenum mode ) {
    return glRenderMode(mode);
}

static GLenum AmiglGetError( void ) {
    return glGetError();
}

static const GLubyte* AmiglGetString( GLenum name ) {
    return glGetString(name);
}

static void AmiglFinish( void ) {
    return glFinish();
}

static void AmiglFlush( void ) {
    return glFlush();
}

static void AmiglHint( GLenum target, GLenum mode ) {
    return glHint(target, mode);
}

static void AmiglClearDepth( GLclampd depth ) {
    return glClearDepth(depth);
}

static void AmiglDepthFunc( GLenum func ) {
    return glDepthFunc(func);
}

static void AmiglDepthMask( GLboolean flag ) {
    return glDepthMask(flag);
}

static void AmiglDepthRange( GLclampd near_val, GLclampd far_val ) {
    return glDepthRange(near_val, far_val);
}

static void AmiglMatrixMode( GLenum mode ) {
    return glMatrixMode(mode);
}

static void AmiglOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val ) {
    return glOrtho(left, right, bottom, top, near_val, far_val);
}

static void AmiglFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val ) {
    return glFrustum(left, right, bottom, top, near_val, far_val);
}

static void AmiglViewport( GLint x, GLint y, GLsizei width, GLsizei height ) {
    return glViewport(x, y, width, height);
}

static void AmiglPushMatrix( void ) {
    return glPushMatrix();
}

static void AmiglPopMatrix( void ) {
    return glPopMatrix();
}

static void AmiglLoadIdentity( void ) {
    return glLoadIdentity();
}

static void AmiglLoadMatrixd( const GLdouble *m ) {
    return glLoadMatrixd(m);
}

static void AmiglLoadMatrixf( const GLfloat *m ) {
    return glLoadMatrixf(m);
}

static void AmiglMultMatrixd( const GLdouble *m ) {
    return glMultMatrixd(m);
}

static void AmiglMultMatrixf( const GLfloat *m ) {
    return glMultMatrixf(m);
}

static void AmiglRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) {
    return glRotated(angle, x, y, z);
}

static void AmiglRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
    return glRotatef(angle, x, y, z);
}

static void AmiglScaled( GLdouble x, GLdouble y, GLdouble z ) {
    return glScaled(x, y, z);
}

static void AmiglScalef( GLfloat x, GLfloat y, GLfloat z ) {
    return glScalef(x, y, z);
}

static void AmiglTranslated( GLdouble x, GLdouble y, GLdouble z ) {
    return glTranslated(x, y, z);
}

static void AmiglTranslatef( GLfloat x, GLfloat y, GLfloat z ) {
    return glTranslatef(x, y, z);
}

static GLboolean AmiglIsList( GLuint list ) {
    return glIsList(list);
}

static void AmiglDeleteLists( GLuint list, GLsizei range ) {
    return glDeleteLists(list, range);
}

static GLuint AmiglGenLists( GLsizei range ) {
    return glGenLists(range);
}

static void AmiglNewList( GLuint list, GLenum mode ) {
    return glNewList(list, mode);
}

static void AmiglEndList( void ) {
    return glEndList();
}

static void AmiglCallList( GLuint list ) {
    return glCallList(list);
}

static void AmiglCallLists( GLsizei n, GLenum type, GLvoid *lists ) {
    return glCallLists(n, type, lists);
}

static void AmiglListBase( GLuint base ) {
    return glListBase(base);
}

static void AmiglBegin( GLenum mode ) {
    return glBegin(mode);
}

static void AmiglEnd( void ) {
    return glEnd();
}

static void AmiglVertex2d( GLdouble x, GLdouble y ) {
    return glVertex2d(x, y);
}

static void AmiglVertex2f( GLfloat x, GLfloat y ) {
    return glVertex2f(x, y);
}

static void AmiglVertex2i( GLint x, GLint y ) {
    return glVertex2i(x, y);
}

static void AmiglVertex2s( GLshort x, GLshort y ) {
    return glVertex2s(x, y);
}

static void AmiglVertex3d( GLdouble x, GLdouble y, GLdouble z ) {
    return glVertex3d(x, y, z);
}

static void AmiglVertex3f( GLfloat x, GLfloat y, GLfloat z ) {
    return glVertex3f(x, y, z);
}

static void AmiglVertex3i( GLint x, GLint y, GLint z ) {
    return glVertex3i(x, y, z);
}

static void AmiglVertex3s( GLshort x, GLshort y, GLshort z ) {
    return glVertex3s(x, y, z);
}

static void AmiglVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
    return glVertex4d(x, y, z, w);
}

static void AmiglVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
    return glVertex4f(x, y, z, w);
}

static void AmiglVertex4i( GLint x, GLint y, GLint z, GLint w ) {
    return glVertex4i(x, y, z, w);
}

static void AmiglVertex4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
    return glVertex4s(x, y, z, w);
}

static void AmiglVertex2dv( GLdouble *v ) {
   return glVertex2dv(v);
}

static void AmiglVertex2fv( GLfloat *v ) {
    return glVertex2fv(v);
}

static void AmiglVertex2iv( GLint *v ) {
    return glVertex2iv(v);
}

static void AmiglVertex2sv( GLshort *v ) {
    return glVertex2sv(v);
}

static void AmiglVertex3dv( GLdouble *v ) {
    return glVertex3dv(v);
}

static void AmiglVertex3fv( GLfloat *v ) {
    return glVertex3fv(v);
}

static void AmiglVertex3iv( GLint *v ) {
    return glVertex3iv(v);
}

static void AmiglVertex3sv( GLshort *v ) {
    return glVertex3sv(v);
}

static void AmiglVertex4dv( GLdouble *v ) {
    return glVertex4dv(v);
}

static void AmiglVertex4fv( GLfloat *v ) {
    return glVertex4fv(v);
}

static void AmiglVertex4iv( GLint *v ) {
    return glVertex4iv(v);
}

static void AmiglVertex4sv( GLshort *v ) {
    return glVertex4sv(v);
}

static void AmiglNormal3b( GLbyte nx, GLbyte ny, GLbyte nz ) {
    return glNormal3b(nx, ny, nz);
}

static void AmiglNormal3d( GLdouble nx, GLdouble ny, GLdouble nz ) {
    return glNormal3d(nx, ny, nz);
}

static void AmiglNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) {
    return glNormal3f(nx, ny, nz);
}

static void AmiglNormal3i( GLint nx, GLint ny, GLint nz ) {
    return glNormal3i(nx, ny, nz);
}

static void AmiglNormal3s( GLshort nx, GLshort ny, GLshort nz ) {
    return glNormal3s(nx, ny, nz);
}

static void AmiglNormal3bv( const GLbyte *v ) {
    return glNormal3bv(v);
}

static void AmiglNormal3dv( const GLdouble *v ) {
    return glNormal3dv(v);
}

static void AmiglNormal3fv( GLfloat *v ) {
    return glNormal3fv(v);
}

static void AmiglNormal3iv( const GLint *v ) {
    return glNormal3iv(v);
}

static void AmiglNormal3sv( const GLshort *v ) {
    return glNormal3sv(v);
}

static void AmiglColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
    return glColor3b(red, green, blue);
}

static void AmiglColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
    return glColor3d(red, green, blue);
}

static void AmiglColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
    return glColor3f(red, green, blue);
}

static void AmiglColor3i( GLint red, GLint green, GLint blue ) {
    return glColor3i(red, green, blue);
}

static void AmiglColor3s( GLshort red, GLshort green, GLshort blue ) {
    return glColor3s(red, green, blue);
}

static void AmiglColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
    return glColor3ub(red, green, blue);
}

static void AmiglColor3ui( GLuint red, GLuint green, GLuint blue ) {
    return glColor3ui(red, green, blue);
}

static void AmiglColor3us( GLushort red, GLushort green, GLushort blue ) {
    return glColor3us(red, green, blue);
}

static void AmiglColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) {
    return glColor4b(red, green, blue, alpha);
}

static void AmiglColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) {
    return glColor4d(red, green, blue, alpha);
}

static void AmiglColor4f( GLfloat red, GLfloat green,GLfloat blue, GLfloat alpha ) {
    return glColor4f(red, green, blue, alpha);
}

static void AmiglColor4i( GLint red, GLint green, GLint blue, GLint alpha ) {
    return glColor4i(red, green, blue, alpha);
}

static void AmiglColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha ) {
    return glColor4s(red, green, blue, alpha);
}

static void AmiglColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) {
    return glColor4ub(red, green, blue, alpha);
}

static void AmiglColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha ) {
    return glColor4ui(red, green, blue, alpha);
}

static void AmiglColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha ) {
    return glColor4us(red, green, blue, alpha);
}

static void AmiglColor3bv( const GLbyte *v ) {
    return glColor3bv(v);
}

static void AmiglColor3dv( const GLdouble *v ) {
    return glColor3dv(v);
}

static void AmiglColor3fv( GLfloat *v ) {
    return glColor3fv(v);
}

static void AmiglColor3iv( const GLint *v ) {
    return glColor3iv(v);
}

static void AmiglColor3sv( const GLshort *v ) {
    return glColor3sv(v);
}

static void AmiglColor3ubv( GLubyte *v ) {
    return glColor3ubv(v);
}

static void AmiglColor3uiv( const GLuint *v ) {
    return glColor3uiv(v);
}

static void AmiglColor3usv( const GLushort *v ) {
    glColor3usv(v);
}

static void AmiglColor4bv( const GLbyte *v ) {
    return glColor4bv(v);
}

static void AmiglColor4dv( const GLdouble *v ) {
    return glColor4dv(v);
}

static void AmiglColor4fv( GLfloat *v ) {
    return glColor4fv(v);
}

static void AmiglColor4iv( const GLint *v ) {
    return glColor4iv(v);
}

static void AmiglColor4sv( const GLshort *v ) {
    return glColor4sv(v);
}

static void AmiglColor4ubv( GLubyte *v ) {
    return glColor4ubv(v);
}

static void AmiglColor4uiv( const GLuint *v ) {
    return glColor4uiv(v);
}

static void AmiglColor4usv( const GLushort *v ) {
    return glColor4usv(v);
}

static void AmiglTexCoord1d( GLdouble s ) {
    return glTexCoord1d(s);
}

static void AmiglTexCoord1f( GLfloat s ) {
    return glTexCoord1f(s);
}

static void AmiglTexCoord1i( GLint s ) {
    return glTexCoord1i(s);
}

static void AmiglTexCoord2d( GLdouble s, GLdouble t ) {
    return glTexCoord2d(s, t);
}

static void AmiglTexCoord2f( GLfloat s, GLfloat t ) {
    return glTexCoord2f(s, t);
}

static void AmiglTexCoord2i( GLint s, GLint t ) {
    return glTexCoord2i(s, t);
}

static void AmiglTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) {
    return glTexCoord3f(s, t, r);
}

static void AmiglTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
    return glTexCoord4f(s, t, r, q);
}

static void AmiglTexCoord2fv( GLfloat *v ) {
    return glTexCoord2fv(v);
}

static void AmiglTexCoord2iv( const GLint *v ) {
    return glTexCoord2iv(v);
}

static void AmiglTexCoord3fv( GLfloat *v ) {
    return glTexCoord3fv(v);
}

static void AmiglTexCoord4fv( GLfloat *v ) {
    return glTexCoord4fv(v);
}

static void AmiglClientActiveTextureARB(GLenum texture) {
    return glClientActiveTextureARB(texture);
}

static void AmiglActiveTextureARB(GLenum unit) {
    return glActiveTextureARB(unit);
}

static void AmiglMultiTexCoord2f(GLenum unit, GLfloat s, GLfloat t) {
    return glMultiTexCoord2f(unit, s, t);
}

static void AmiglMultiTexCoord2fv(GLenum unit, GLfloat *v) {
    return glMultiTexCoord2fv(unit, v);
}

static void AmiglMultiTexCoord4f(GLenum unit, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    return glMultiTexCoord4f(unit, s, t, r, q);
}

static void AmiglMultiTexCoord4fv(GLenum unit, GLfloat *v) {
    return glMultiTexCoord4fv(unit, v);
}

static void AmiglMultiTexCoord2fARB(GLenum unit, GLfloat s, GLfloat t) {
    return glMultiTexCoord2fARB(unit, s, t);
}

static void AmiglMultiTexCoord2fvARB(GLenum unit, GLfloat *v) {
    return glMultiTexCoord2fvARB(unit, v);
}

static void AmiglMultiTexCoord4fARB(GLenum unit, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    return glMultiTexCoord4fARB(unit, s, t, r, q);
}

static void AmiglMultiTexCoord4fvARB(GLenum unit, GLfloat *v) {
    return glMultiTexCoord4fvARB(unit, v);
}

static void AmiglRasterPos2d( GLdouble x, GLdouble y ) {
    return glRasterPos2d(x, y);
}

static void AmiglRasterPos2f( GLfloat x, GLfloat y ) {
    return glRasterPos2f(x, y);
}

static void AmiglRasterPos2i( GLint x, GLint y ) {
    return glRasterPos2i(x, y);
}

static void AmiglRasterPos2s( GLshort x, GLshort y ) {
    return glRasterPos2s(x, y);
}

static void AmiglRasterPos3d( GLdouble x, GLdouble y, GLdouble z ) {
    return glRasterPos3d(x, y, z);
}

static void AmiglRasterPos3f( GLfloat x, GLfloat y, GLfloat z ) {
    return glRasterPos3f(x, y, z);
}

static void AmiglRasterPos3i( GLint x, GLint y, GLint z ) {
    return glRasterPos3i(x, y, z);
}

static void AmiglRasterPos3s( GLshort x, GLshort y, GLshort z ) {
    return glRasterPos3s(x, y, z);
}

static void AmiglRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
    return glRasterPos4d(x, y, z, w);
}

static void AmiglRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
    return glRasterPos4f(x, y, z, w);
}

static void AmiglRasterPos4i( GLint x, GLint y, GLint z, GLint w ) {
    return glRasterPos4i(x, y, z, w);
}

static void AmiglRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
    return glRasterPos4s(x, y, z, w);
}

static void AmiglRasterPos2dv( GLdouble *v ) {
    return glRasterPos2dv(v);
}

static void AmiglRasterPos2fv( GLfloat *v ) {
    return glRasterPos2fv((GLfloat *)v);
}

static void AmiglRasterPos2iv( GLint *v ) {
    return glRasterPos2iv(v);
}

static void AmiglRasterPos2sv( GLshort *v ) {
    return glRasterPos2sv(v);
}

static void AmiglRasterPos3dv( GLdouble *v ) {
    return glRasterPos3dv(v);
}

static void AmiglRasterPos3fv( GLfloat *v ) {
    return glRasterPos3fv((GLfloat *)v);
}

static void AmiglRasterPos3iv( GLint *v ) {
    return glRasterPos3iv(v);
}

static void AmiglRasterPos3sv( GLshort *v ) {
    return glRasterPos3sv(v);
}

static void AmiglRasterPos4dv( GLdouble *v ) {
    return glRasterPos4dv(v);
}

static void AmiglRasterPos4fv( GLfloat *v ) {
    return glRasterPos4fv(v);
}

static void AmiglRasterPos4iv( GLint *v ) {
    return glRasterPos4iv(v);
}

static void AmiglRasterPos4sv( GLshort *v ) {
    return glRasterPos4sv(v);
}

static void AmiglRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) {
    return glRectd(x1, y1, x2, y2);
}

static void AmiglRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) {
    return glRectf(x1, y1, x2, y2);
}

static void AmiglRecti( GLint x1, GLint y1, GLint x2, GLint y2 ) {
    return glRecti(x1, y1, x2, y2);
}

static void AmiglRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) {
    return glRects(x1, y1, x2, y2);
}

static void AmiglRectdv( GLdouble *v1, GLdouble *v2 ) {
    return glRectdv(v1, v2);
}

static void AmiglRectfv( GLfloat *v1,  GLfloat *v2 ) {
    return glRectfv(v1, v2);
}

static void AmiglRectiv( GLint *v1, GLint *v2 ) {
    return glRectiv(v1, v2);
}

static void AmiglRectsv( GLshort *v1, GLshort *v2 ) {
    return glRectsv(v1, v2);
}

static void AmiglVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr ) {
    return glVertexPointer(size, type, stride, ptr);
}

static void AmiglNormalPointer( GLenum type, GLsizei stride, const GLvoid *ptr ) {
    return glNormalPointer(type, stride, ptr);
}

static void AmiglColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr ) {
    return glColorPointer(size, type, stride, ptr);
}

static void AmiglTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr ) {
    return glTexCoordPointer(size, type, stride, ptr);
}

static void AmiglArrayElement( GLint i ) {
    return glArrayElement(i);
}

static void AmiglDrawArrays( GLenum mode, GLint first, GLsizei count ) {
    return glDrawArrays(mode, first, count);
}

static void AmiglDrawElements( GLenum mode, GLsizei count, GLenum type, GLvoid *indices ) {
    return glDrawElements(mode, count, type, indices);
}

static void AmiglInterleavedArrays( GLenum format, GLsizei stride, const GLvoid *pointer ) {
    return glInterleavedArrays(format, stride, pointer);
}

static void AmiglShadeModel( GLenum mode ) {
    return glShadeModel(mode);
}

static void AmiglLightf( GLenum light, GLenum pname, GLfloat param ) {
    return glLightf(light, pname, param);
}

static void AmiglLighti( GLenum light, GLenum pname, GLint param ) {
    return glLighti(light, pname, param);
}

static void AmiglLightfv( GLenum light, GLenum pname, GLfloat *params ) {
    return glLightfv(light, pname, params);
}

static void AmiglLightiv( GLenum light, GLenum pname, const GLint *params ) {
    return glLightiv(light, pname, params);
}

static void AmiglGetLightfv( GLenum light, GLenum pname, GLfloat *params ) {
    return glGetLightfv(light, pname, params);
}

static void AmiglGetLightiv( GLenum light, GLenum pname, GLint *params ) {
    return glGetLightiv(light, pname, params);
}

static void AmiglLightModelf( GLenum pname, GLfloat param ) {
    return glLightModelf(pname, param);
}

static void AmiglLightModeli( GLenum pname, GLint param ) {
    return glLightModeli(pname, param);
}

static void AmiglLightModelfv( GLenum pname, GLfloat *params ) {
    return glLightModelfv(pname, params);
}

static void AmiglLightModeliv( GLenum pname, const GLint *params ) {
    return glLightModeliv(pname, params);
}

static void AmiglMaterialf( GLenum face, GLenum pname, GLfloat param ) {
    return glMaterialf(face, pname, param);
}

static void AmiglMateriali( GLenum face, GLenum pname, GLint param ) {
    return glMateriali(face, pname, param);
}

static void AmiglMaterialfv( GLenum face, GLenum pname, GLfloat *params ) {
    return glMaterialfv(face, pname, params);
}

static void AmiglMaterialiv( GLenum face, GLenum pname, const GLint *params ) {
    return glMaterialiv(face, pname, params);
}

static void AmiglGetMaterialfv( GLenum face, GLenum pname, GLfloat *params ) {
    return glGetMaterialfv(face, pname, params);
}

static void AmiglGetMaterialiv( GLenum face, GLenum pname, GLint *params ) {
    return glGetMaterialiv(face, pname, params);
}

static void AmiglColorMaterial( GLenum face, GLenum mode ) {
    return glColorMaterial(face, mode);
}

static void AmiglPixelZoom( GLfloat xfactor, GLfloat yfactor ) {
    return glPixelZoom(xfactor, yfactor);
}

static void AmiglPixelStoref( GLenum pname, GLfloat param ) {
    return glPixelStoref(pname, param);
}

static void AmiglPixelStorei( GLenum pname, GLint param ) {
    return glPixelStorei(pname, param);
}

static void AmiglPixelTransferf( GLenum pname, GLfloat param ) {
    return glPixelTransferf(pname, param);
}

static void AmiglPixelTransferi( GLenum pname, GLint param ) {
    return glPixelTransferi(pname, param);
}

static void AmiglBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap ) {
    return glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
}

static void AmiglReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels ) {
    return glReadPixels(x, y, width, height, format, type, pixels);
}

static void AmiglDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ) {
    return glDrawPixels(width, height, format, type, pixels);
}

static void AmiglCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) {
    return glCopyPixels(x, y, width, height, type);
}

static void AmiglStencilFunc( GLenum func, GLint ref, GLuint mask ) {
    return glStencilFunc(func, ref, mask);
}

static void AmiglStencilMask( GLuint mask ) {
    return glStencilMask(mask);
}

static void AmiglStencilOp( GLenum fail, GLenum zfail, GLenum zpass ) {
    return glStencilOp(fail, zfail, zpass);
}

static void AmiglClearStencil( GLint s ) {
    return glClearStencil(s);
}

static void AmiglTexGeni( GLenum coord, GLenum pname, GLint param ) {
    return glTexGeni(coord, pname, param);
}

static void AmiglTexGenfv( GLenum coord, GLenum pname, GLfloat *params ) {
    return glTexGenfv(coord, pname, params);
}

static void AmiglTexEnvf( GLenum target, GLenum pname, GLfloat param ) {
    return glTexEnvf(target, pname, param);
}

static void AmiglTexEnvi( GLenum target, GLenum pname, GLint param ) {
    return glTexEnvi(target, pname, param);
}

static void AmiglTexEnvfv( GLenum target, GLenum pname, const GLfloat *params ) {
    return glTexEnvfv(target, pname, params);
}

static void AmiglTexEnviv( GLenum target, GLenum pname, GLint *params ) {
    return glTexEnviv(target, pname, params);
}

static void AmiglGetTexEnviv( GLenum target, GLenum pname, GLint *params ) {
    return glGetTexEnviv(target, pname, params);
}

static void AmiglTexParameterf( GLenum target, GLenum pname, GLfloat param ) {
    return glTexParameterf(target, pname, param);
}

static void AmiglTexParameteri( GLenum target, GLenum pname, GLint param ) {
    return glTexParameteri(target, pname, param);
}

static void AmiglTexParameterfv( GLenum target, GLenum pname, GLfloat *params ) {
    return glTexParameterfv(target, pname, params);
}

static void AmiglTexParameteriv( GLenum target, GLenum pname, GLint *params ) {
    return glTexParameteriv(target, pname, params);
}

static void AmiglGetTexParameterfv( GLenum target, GLenum pname, GLfloat *params) {
    return glGetTexParameterfv(target, pname, params);
}

static void AmiglGetTexParameteriv( GLenum target, GLenum pname, GLint *params ) {
    return glGetTexParameteriv(target, pname, params);
}

static void AmiglGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat *params ) {
    return glGetTexLevelParameterfv(target, level, pname, params);
}

static void AmiglGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint *params ) {
    return glGetTexLevelParameteriv(target, level, pname, params);
}

static void AmiglTexImage1D( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
    return glTexImage1D(target, level, internalFormat, width, border, format, type, pixels);
}

static void AmiglTexImage2D( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLvoid *pixels ) {
    return glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
}

static void AmiglGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels ) {
    return glGetTexImage(target, level, format, type, pixels);
}

static void AmiglGenTextures( GLsizei n, GLuint *textures ) {
    return glGenTextures(n, textures);
}

static void AmiglDeleteTextures( GLsizei n, const GLuint *textures) {
    return glDeleteTextures(n, textures);
}

static void AmiglBindTexture( GLenum target, GLuint texture ) {
    return glBindTexture(target, texture);
}

static void AmiglPrioritizeTextures( GLsizei n, const GLuint *textures, const GLclampf *priorities )  {
    return glPrioritizeTextures(n, textures, priorities);
}

static GLboolean AmiglAreTexturesResident( GLsizei n, GLuint *textures, GLboolean *residences ) {
    return glAreTexturesResident(n, textures, residences);
}

static GLboolean AmiglIsTexture( GLuint texture ) {
    return glIsTexture(texture);
}

static void AmiglTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels ) {
    return glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
}

static void AmiglTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels ) {
    return glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

static void AmiglCopyTexImage1D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
    return glCopyTexImage1D(target, level, internalformat, x, y, width, border);
}

static void AmiglCopyTexImage2D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
    return glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

static void AmiglCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
    return glCopyTexSubImage1D(target, level, xoffset, x, y, width);
}

static void AmiglCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
    return glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

static void AmiglMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points ) {
    return glMap1d(target, u1, u2, stride, order, points);
}

static void AmiglMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points ) {
    return glMap1f(target, u1, u2, stride, order, points);
}

static void AmiglMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,  const GLdouble *points ) {
    return glMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

static void AmiglMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points ) {
    return glMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
 }

static void AmiglGetMapdv( GLenum target, GLenum query, GLdouble *v ) {
    return glGetMapdv(target, query, v);
}

static void AmiglGetMapfv( GLenum target, GLenum query, GLfloat *v ) {
    return glGetMapfv(target, query, v);
}

static void AmiglGetMapiv( GLenum target, GLenum query, GLint *v ) {
    return glGetMapiv(target, query, v);
}

static void AmiglEvalCoord1d( GLdouble u ) {
    return glEvalCoord1d(u);
}

static void AmiglEvalCoord1f( GLfloat u ) {
    return glEvalCoord1f(u);
}

static void AmiglEvalCoord1dv( GLdouble *u ) {
    return glEvalCoord1dv(u);
}

static void AmiglEvalCoord1fv( GLfloat *u ) {
    return glEvalCoord1fv(u);
}

static void AmiglEvalCoord2d( GLdouble u, GLdouble v ) {
    return glEvalCoord2d(u, v);
}

static void AmiglEvalCoord2f( GLfloat u, GLfloat v ) {
    return glEvalCoord2f(u, v);
}

static void AmiglEvalCoord2dv( GLdouble *u ) {
    return glEvalCoord2dv(u);
}

static void AmiglEvalCoord2fv( GLfloat *u ) {
    return glEvalCoord2fv(u);
}

static void AmiglMapGrid1d( GLint un, GLdouble u1, GLdouble u2 ) {
    return glMapGrid1d(un, u1, u2);
}

static void AmiglMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) {
    return glMapGrid1f(un, u1, u2);
}

static void AmiglMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) {
    return glMapGrid2d(un, u1, u2, vn, v1, v2);
}

static void AmiglMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) {
    return glMapGrid2f(un, u1, u2, vn, v1, v2);
}

static void AmiglEvalPoint1( GLint i ) {
    return glEvalPoint1(i);
}

static void AmiglEvalPoint2( GLint i, GLint j ) {
    return glEvalPoint2(i, j);
}

static void AmiglEvalMesh1( GLenum mode, GLint i1, GLint i2 ) {
    return glEvalMesh1(mode, i1, i2);
}

static void AmiglEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) {
    return glEvalMesh2(mode, i1, i2, j1, j2);
}

static void AmiglFogf( GLenum pname, GLfloat param ) {
    return glFogf(pname, param);
}

static void AmiglFogi( GLenum pname, GLint param ) {
    return glFogi(pname, param);
}

static void AmiglFogfv( GLenum pname, GLfloat *params ) {
    return glFogfv(pname, params);
}

static void AmiglSelectBuffer( GLsizei size, GLuint *buffer ) {
    return glSelectBuffer(size, buffer);
}

static void AmiglInitNames( void ) {
    return glInitNames();
}

static void AmiglLoadName( GLuint name ) {
    return glLoadName(name);
}

static void AmiglPushName( GLuint name ) {
    return glPushName(name);
}

static void AmiglPopName( void ) {
    return glPopName();
}

static void AmiglDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLvoid *indices ) {
    return glDrawRangeElements(mode, start, end, count, type, indices);
}

static void AmiglLockArraysEXT( GLint first, GLint count ) {
    return glLockArraysEXT(first, count);
}

static void AmiglUnlockArraysEXT( void ) {
    return glUnlockArraysEXT();
}

/* The GLU API */

/*
 *
 * GLU
 *
 */

static void AmigluLookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy, GLdouble upz ) {
    return gluLookAt(eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz);
}

static void AmigluOrtho2D( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top ) {
    return gluOrtho2D(left, right, bottom, top);
}

static void AmigluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar ) {
    return gluPerspective(fovy, aspect, zNear, zFar);
}

static void AmigluPickMatrix( GLdouble x, GLdouble y, GLdouble width, GLdouble height, const GLint viewport[4] ) {
    return gluPickMatrix(x, y, width, height, (GLint *)viewport);
}

static GLint AmigluProject( GLdouble objx, GLdouble objy, GLdouble objz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *winx, GLdouble *winy, GLdouble *winz ) {
    return gluProject(objx, objy, objz, modelMatrix, projMatrix, viewport, winx, winy, winz);
}

static GLint AmigluUnProject( GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *objx, GLdouble *objy, GLdouble *objz ) {
    return gluUnProject(winx, winy, winz, modelMatrix, projMatrix, viewport, objx, objy, objz);
}

static const GLubyte* AmigluErrorString( GLenum errorCode ) {
    return gluErrorString(errorCode);
}

static GLint AmigluScaleImage( GLenum format, GLint widthin, GLint heightin, GLenum typein, const void *datain, GLint widthout, GLint heightout, GLenum typeout, void *dataout ) {
    return gluScaleImage(format, widthin, heightin, typein, datain, widthout, heightout, typeout, dataout);
}

#if 0
static GLint AmigluBuild1DMipmaps( GLenum target, GLint components, GLint width, GLenum format, GLenum type, const void *data ) {
#ifndef __amigaos4__
    return gluBuild1DMipmaps(target, components, width, format, type, data);
#else
    return 0;
#endif
}
#endif

static GLint AmigluBuild2DMipmaps( GLenum target, GLint components, GLint width, GLint height, GLenum format, GLenum type, void *data ) {
    return gluBuild2DMipmaps(target, components, width, height, format, type, data);
}

static GLUquadricObj* AmigluNewQuadric( void ) {
    return gluNewQuadric();
}

static void AmigluDeleteQuadric( GLUquadricObj *state ) {
    return gluDeleteQuadric(state);
}

static void AmigluQuadricDrawStyle( GLUquadricObj *quadObject, GLenum drawStyle ) {
    return gluQuadricDrawStyle(quadObject, drawStyle);
}

static void AmigluQuadricOrientation( GLUquadricObj *quadObject, GLenum orientation ) {
    return gluQuadricOrientation(quadObject, orientation);
}

static void AmigluQuadricNormals( GLUquadricObj *quadObject, GLenum normals ) {
    return gluQuadricNormals(quadObject, normals);
}

static void AmigluQuadricTexture( GLUquadricObj *quadObject, GLboolean textureCoords ) {
    return gluQuadricTexture(quadObject, textureCoords);
}

static void AmigluQuadricCallback( GLUquadricObj *qobj, GLenum which, void *fn) {
    return gluQuadricCallback(qobj, which, fn);
}

static void AmigluCylinder( GLUquadricObj *qobj, GLdouble baseRadius, GLdouble topRadius, GLdouble height, GLint slices, GLint stacks ) {
    return gluCylinder(qobj, baseRadius, topRadius, height, slices, stacks);
}

static void AmigluSphere( GLUquadricObj *qobj, GLdouble radius, GLint slices, GLint stacks ) {
    return gluSphere(qobj, radius, slices, stacks);
}

static void AmigluDisk( GLUquadricObj *qobj, GLdouble innerRadius, GLdouble outerRadius, GLint slices, GLint loops ) {
    return gluDisk(qobj, innerRadius, outerRadius, slices, loops);
}

static void AmigluPartialDisk( GLUquadricObj *qobj, GLdouble innerRadius, GLdouble outerRadius, GLint slices, GLint loops, GLdouble startAngle, GLdouble sweepAngle ) {
    return gluPartialDisk(qobj, innerRadius, outerRadius, slices, loops, startAngle, sweepAngle);
}

static GLUnurbsObj* AmigluNewNurbsRenderer( void ) {
    return gluNewNurbsRenderer();
}

static void AmigluDeleteNurbsRenderer( GLUnurbsObj *nobj ) {
    return gluDeleteNurbsRenderer(nobj);
}

static void AmigluLoadSamplingMatrices( GLUnurbsObj *nobj, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4] ) {
    return gluLoadSamplingMatrices(nobj, modelMatrix, projMatrix, viewport);
}

static void AmigluNurbsProperty( GLUnurbsObj *nobj, GLenum property, GLfloat value ) {
    return gluNurbsProperty(nobj, property, value);
}

static void AmigluGetNurbsProperty( GLUnurbsObj *nobj, GLenum property, GLfloat *value ) {
    return gluGetNurbsProperty(nobj, property, value);
}

static void AmigluBeginCurve( GLUnurbsObj *nobj ) {
    return gluBeginCurve(nobj);
}

static void AmigluEndCurve( GLUnurbsObj * nobj ) {
    return gluEndCurve(nobj);
}

static void AmigluNurbsCurve( GLUnurbsObj *nobj, GLint nknots, GLfloat *knot, GLint stride, GLfloat *ctlarray, GLint order, GLenum type ) {
    return gluNurbsCurve(nobj, nknots, knot, stride, ctlarray, order, type);
}

static void AmigluBeginSurface( GLUnurbsObj *nobj ) {
    return gluBeginSurface(nobj);
}

static void AmigluEndSurface( GLUnurbsObj * nobj ) {
    return gluEndSurface(nobj);
}

static void AmigluNurbsSurface( GLUnurbsObj *nobj, GLint sknot_count, GLfloat *sknot, GLint tknot_count, GLfloat *tknot, GLint s_stride, GLint t_stride, GLfloat *ctlarray, GLint sorder, GLint torder, GLenum type ) {
    return gluNurbsSurface(nobj, sknot_count, sknot, tknot_count, tknot, s_stride, t_stride, ctlarray, sorder, torder, type);
}

static void AmigluBeginTrim( GLUnurbsObj *nobj ) {
    return gluBeginTrim(nobj);
}

static void AmigluEndTrim( GLUnurbsObj *nobj ) {
    return gluEndTrim(nobj);
}

static void AmigluPwlCurve( GLUnurbsObj *nobj, GLint count, GLfloat *array, GLint stride, GLenum type ) {
    return gluPwlCurve(nobj, count, array, stride, type);
}

static void AmigluNurbsCallback( GLUnurbsObj *nobj, GLenum which, void *fn ) {
    return gluNurbsCallback(nobj, which, fn);
}

static GLUtriangulatorObj* AmigluNewTess( void ) {
    return gluNewTess();
}

static void AmigluTessCallback( GLUtriangulatorObj *tobj, GLenum which, void *fn ) {
    return gluTessCallback(tobj, which, fn);
}

static void AmigluDeleteTess( GLUtriangulatorObj *tobj ) {
    return gluDeleteTess(tobj);
}

static void AmigluBeginPolygon( GLUtriangulatorObj *tobj ) {
    return gluBeginPolygon(tobj);
}

static void AmigluEndPolygon( GLUtriangulatorObj *tobj ) {
    return gluEndPolygon(tobj);
}

static void AmigluNextContour( GLUtriangulatorObj *tobj, GLenum type ) {
    return gluNextContour(tobj, type);
}

static void AmigluTessVertex( GLUtriangulatorObj *tobj, GLdouble v[3], void *data ) {
    return gluTessVertex(tobj, v, data);
}

static void AmigluTessProperty(GLUtesselator* tess, GLenum which, GLdouble data) {
    return gluTessProperty(tess, which, data);
}

static void AmigluTessNormal(GLUtesselator* tess, GLdouble X, GLdouble Y, GLdouble Z) {
    return gluTessNormal(tess, X, Y, Z);
}

static void AmigluGetTessProperty(GLUtesselator* tess, GLenum which, GLdouble* data) {
    return gluGetTessProperty(tess, which, data);
}

static void AmigluTessBeginPolygon(GLUtesselator* tess, GLvoid* data) {
    return gluTessBeginPolygon(tess, data);
}

static void AmigluTessEndPolygon(GLUtesselator* tess) {
    return gluTessEndPolygon(tess);
}

static void AmigluTessBeginContour(GLUtesselator* tess) {
    return gluTessBeginContour(tess);
}

static void AmigluTessEndContour(GLUtesselator* tess) {
    return gluTessEndContour(tess);
}

static GLint AmigluBuild2DMipmapLevels(GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint level, GLint base, GLint max, const void *data) {
    return gluBuild2DMipmapLevels(target, internalFormat, width, height, format, type, level, base, max, data);
}

static GLint AmigluUnProject4(GLdouble winX, GLdouble winY, GLdouble winZ, GLdouble clipW, const GLdouble *model, const GLdouble *proj, const GLint *view, GLdouble nearVal, GLdouble farVal, GLdouble* objX, GLdouble* objY, GLdouble* objZ, GLdouble* objW) {
    return gluUnProject4(winX, winY, winZ, clipW, model, proj, view, nearVal, farVal, objX, objY, objZ, objW);
}

static GLboolean AmigluCheckExtension(const GLubyte *extName, const GLubyte *extString) {
    return gluCheckExtension(extName, extString);
}

static const GLubyte* AmigluGetString( GLenum name ) {
    return gluGetString(name);
}

static void AmigluNurbsCallbackData (GLUnurbs* r, void* userData) {
    return gluNurbsCallbackData (r, userData);
}

static void AmigluNurbsCallbackDataEXT(GLUnurbs* nurb, GLvoid* userData) {
    return gluNurbsCallbackDataEXT(nurb, userData);
}

#ifdef NOT_IMPLEMENTED_FUNCS
    #include "SDL_os4_notimplemented_funcs.t"
#endif

struct MyGLFunc
{
   CONST_STRPTR name;
   APTR func;
};

void *AmiGetGLProc(const char *proc)
{
   static CONST struct MyGLFunc table[] = {
        {"glClipPlane", AmiglClipPlane},
        {"glPolygonOffset", AmiglPolygonOffset},
        {"glTexEnviv", AmiglTexEnviv},
        {"glTexEnvfv", AmiglTexEnvfv},
        {"glGetTexEnviv", AmiglGetTexEnviv},
        {"glGetBooleanv", AmiglGetBooleanv},
        {"glGetIntegerv", AmiglGetIntegerv},
        {"glIsEnabled", AmiglIsEnabled},
        {"glAlphaFunc", AmiglAlphaFunc},
        {"glBegin", AmiglBegin},
        {"glBindTexture", AmiglBindTexture},
        {"glBlendFunc", AmiglBlendFunc},
        {"glClear", AmiglClear},
        {"glClearColor", AmiglClearColor},
        {"glClearDepth", AmiglClearDepth},
        {"glColor3b", AmiglColor3b},
        {"glColor3ub", AmiglColor3ub},
        {"glColor3bv", AmiglColor3bv},
        {"glColor3ubv", AmiglColor3ubv},
        {"glColor3s", AmiglColor3s},
        {"glColor3us", AmiglColor3us},
        {"glColor3sv", AmiglColor3sv},
        {"glColor3usv", AmiglColor3usv},
        {"glColor3i", AmiglColor3i},
        {"glColor3ui", AmiglColor3ui},
        {"glColor3iv", AmiglColor3iv},
        {"glColor3uiv", AmiglColor3uiv},
        {"glColor3f", AmiglColor3f},
        {"glColor3fv", AmiglColor3fv},
        {"glColor3d", AmiglColor3d},
        {"glColor3dv", AmiglColor3dv},
        {"glColor4b", AmiglColor4b},
        {"glColor4ub", AmiglColor4ub},
        {"glColor4bv", AmiglColor4bv},
        {"glColor4ubv", AmiglColor4ubv},
        {"glColor4s", AmiglColor4s},
        {"glColor4us", AmiglColor4us},
        {"glColor4sv", AmiglColor4sv},
        {"glColor4usv", AmiglColor4usv},
        {"glColor4i", AmiglColor4i},
        {"glColor4ui", AmiglColor4ui},
        {"glColor4iv", AmiglColor4iv},
        {"glColor4uiv", AmiglColor4uiv},
        {"glColor4f", AmiglColor4f},
        {"glColor4fv", AmiglColor4fv},
        {"glColor4d", AmiglColor4d},
        {"glColor4dv", AmiglColor4dv},
        {"glCullFace", AmiglCullFace},
        {"glVertex2s", AmiglVertex2s},
        {"glVertex2i", AmiglVertex2i},
        {"glVertex2f", AmiglVertex2f},
        {"glVertex2d", AmiglVertex2d},
        {"glVertex3s", AmiglVertex3s},
        {"glVertex3i", AmiglVertex3i},
        {"glVertex3f", AmiglVertex3f},
        {"glVertex3d", AmiglVertex3d},
        {"glVertex4s", AmiglVertex4s},
        {"glVertex4i", AmiglVertex4i},
        {"glVertex4f", AmiglVertex4f},
        {"glVertex4d", AmiglVertex4d},
        {"glVertex2sv", AmiglVertex2sv},
        {"glVertex2iv", AmiglVertex2iv},
        {"glVertex2fv", AmiglVertex2fv},
        {"glVertex2dv", AmiglVertex2dv},
        {"glVertex3sv", AmiglVertex3sv},
        {"glVertex3iv", AmiglVertex3iv},
        {"glVertex3fv", AmiglVertex3fv},
        {"glVertex3dv", AmiglVertex3dv},
        {"glVertex4sv", AmiglVertex4sv},
        {"glVertex4iv", AmiglVertex4iv},
        {"glVertex4fv", AmiglVertex4fv},
        {"glVertex4dv", AmiglVertex4dv},
        {"glDeleteTextures", AmiglDeleteTextures},
        {"glDepthFunc", AmiglDepthFunc},
        {"glDepthMask", AmiglDepthMask},
        {"glDepthRange", AmiglDepthRange},
        {"glDisable", AmiglDisable},
        {"glDrawBuffer", AmiglDrawBuffer},
        {"glEnable", AmiglEnable},
        {"glEnd", AmiglEnd},
        {"glFinish", AmiglFinish},
        {"glFlush", AmiglFlush},
        {"glFogf", AmiglFogf},
        {"glFogi", AmiglFogi},
        {"glFogfv", AmiglFogfv},
        {"glFrontFace", AmiglFrontFace},
        {"glFrustum", AmiglFrustum},
        {"glGenTextures", AmiglGenTextures},
        {"glGetError", AmiglGetError},
        {"glGetFloatv", AmiglGetFloatv},
        {"glGetString", AmiglGetString},
        {"glHint", AmiglHint},
        {"glLoadIdentity", AmiglLoadIdentity},
        {"glLoadMatrixd", AmiglLoadMatrixd},
        {"glLoadMatrixf", AmiglLoadMatrixf},
        {"glMatrixMode", AmiglMatrixMode},
        {"glMultMatrixd", AmiglMultMatrixd},
        {"glMultMatrixf", AmiglMultMatrixf},
        {"glNormal3f", AmiglNormal3f},
        {"glNormal3d", AmiglNormal3d},
        {"glNormal3i", AmiglNormal3i},
        {"glNormal3s", AmiglNormal3s},
        {"glNormal3b", AmiglNormal3b},
        {"glNormal3fv", AmiglNormal3fv},
        {"glNormal3dv", AmiglNormal3dv},
        {"glNormal3iv", AmiglNormal3iv},
        {"glNormal3sv", AmiglNormal3sv},
        {"glNormal3bv", AmiglNormal3bv},
        {"glOrtho", AmiglOrtho},
        {"glPixelStorei", AmiglPixelStorei},
        {"glPixelStoref", AmiglPixelStoref},
        {"glPolygonMode", AmiglPolygonMode},
        {"glPopMatrix", AmiglPopMatrix},
        {"glPushMatrix", AmiglPushMatrix},
        {"glReadPixels", AmiglReadPixels},
        {"glRectf", AmiglRectf},
        {"glRects", AmiglRects},
        {"glRecti", AmiglRecti},
        {"glRectd", AmiglRectd},
        {"glRectsv", AmiglRectsv},
        {"glRectiv", AmiglRectiv},
        {"glRectfv", AmiglRectfv},
        {"glRectdv", AmiglRectdv},
        {"glRotatef", AmiglRotatef},
        {"glRotated", AmiglRotated},
        {"glScalef", AmiglScalef},
        {"glScaled", AmiglScaled},
        {"glScissor", AmiglScissor},
        {"glShadeModel", AmiglShadeModel},
        {"glTexCoord1f", AmiglTexCoord1f},
        {"glTexCoord1d", AmiglTexCoord1d},
        {"glTexCoord1i", AmiglTexCoord1i},
        {"glTexCoord2f", AmiglTexCoord2f},
        {"glTexCoord2d", AmiglTexCoord2d},
        {"glTexCoord2i", AmiglTexCoord2i},
        {"glTexCoord2fv", AmiglTexCoord2fv},
        {"glTexCoord2iv", AmiglTexCoord2iv},
        {"glTexCoord3f", AmiglTexCoord3f},
        {"glTexCoord3fv", AmiglTexCoord3fv},
        {"glTexCoord4f", AmiglTexCoord4f},
        {"glTexCoord4fv", AmiglTexCoord4fv},
        {"glTexEnvf", AmiglTexEnvf},
        {"glTexEnvi", AmiglTexEnvi},
        {"glTexGeni", AmiglTexGeni},
        {"glTexGenfv", AmiglTexGenfv},
        {"glTexImage1D", AmiglTexImage1D},
        {"glTexImage2D", AmiglTexImage2D},
        {"glTexParameteri", AmiglTexParameteri},
        {"glTexParameterf", AmiglTexParameterf},
        {"glTexParameterfv", AmiglTexParameterfv},
        {"glTexParameteriv", AmiglTexParameteriv},
        {"glTexSubImage1D", AmiglTexSubImage1D},
        {"glTexSubImage2D", AmiglTexSubImage2D},
        {"glTranslated", AmiglTranslated},
        {"glTranslatef", AmiglTranslatef},
        {"glViewport", AmiglViewport},
        {"glColorTable", AmiglColorTable},
        {"glColorTableEXT", AmiglColorTableEXT},
        {"glEnableClientState", AmiglEnableClientState},
        {"glDisableClientState", AmiglDisableClientState},
        {"glClientActiveTexture", AmiglClientActiveTexture},
        {"glClientActiveTextureARB", AmiglClientActiveTextureARB},
        {"glTexCoordPointer", AmiglTexCoordPointer},
        {"glColorPointer", AmiglColorPointer},
        {"glNormalPointer", AmiglNormalPointer},
        {"glVertexPointer", AmiglVertexPointer},
        {"glInterleavedArrays", AmiglInterleavedArrays},
        {"glDrawElements", AmiglDrawElements},
        {"glDrawArrays", AmiglDrawArrays},
        {"glArrayElement", AmiglArrayElement},
        {"glLockArraysEXT", AmiglLockArraysEXT},
        {"glUnlockArraysEXT", AmiglUnlockArraysEXT},
        {"glPushAttrib", AmiglPushAttrib},
        {"glPopAttrib", AmiglPopAttrib},
        {"glActiveTextureARB", AmiglActiveTextureARB},
        {"glMultiTexCoord2fARB", AmiglMultiTexCoord2fARB},
        {"glMultiTexCoord2fvARB", AmiglMultiTexCoord2fvARB},
        {"glMultiTexCoord4fARB", AmiglMultiTexCoord4fARB},
        {"glMultiTexCoord4fvARB", AmiglMultiTexCoord4fvARB},
        {"glActiveTexture", AmiglActiveTexture},
        {"glMultiTexCoord2f", AmiglMultiTexCoord2f},
        {"glMultiTexCoord2fv", AmiglMultiTexCoord2fv},
        {"glMultiTexCoord4f", AmiglMultiTexCoord4f},
        {"glMultiTexCoord4fv", AmiglMultiTexCoord4fv},
        {"glMaterialf", AmiglMaterialf},
        {"glMateriali", AmiglMateriali},
        {"glMaterialfv", AmiglMaterialfv},
        {"glMaterialiv", AmiglMaterialiv},
        {"glGetMaterialiv", AmiglGetMaterialiv},
        {"glGetMaterialfv", AmiglGetMaterialfv},
        {"glLightf", AmiglLightf},
        {"glLighti", AmiglLighti},
        {"glLightfv", AmiglLightfv},
        {"glLightiv", AmiglLightiv},
        {"glLightModelf", AmiglLightModelf},
        {"glLightModeli", AmiglLightModeli},
        {"glLightModelfv", AmiglLightModelfv},
        {"glLightModeliv", AmiglLightModeliv},
        {"glColorMaterial", AmiglColorMaterial},
        {"glGetLightfv", AmiglGetLightfv},
        {"glGetLightiv", AmiglGetLightiv},
        {"glStencilOp", AmiglStencilOp},
        {"glStencilFunc", AmiglStencilFunc},
        {"glClearStencil", AmiglClearStencil},
        {"glStencilMask", AmiglStencilMask},
        {"glColorMask", AmiglColorMask},
        {"glLineWidth", AmiglLineWidth},
        {"glPointSize", AmiglPointSize},
        {"glBitmap", AmiglBitmap},
        {"glLineStipple", AmiglLineStipple},
        {"glPolygonStipple", AmiglPolygonStipple},
        {"glRasterPos2s", AmiglRasterPos2s},
        {"glRasterPos2i", AmiglRasterPos2i},
        {"glRasterPos2f", AmiglRasterPos2f},
        {"glRasterPos2d", AmiglRasterPos2d},
        {"glRasterPos3s", AmiglRasterPos3s},
        {"glRasterPos3i", AmiglRasterPos3i},
        {"glRasterPos3f", AmiglRasterPos3f},
        {"glRasterPos3d", AmiglRasterPos3d},
        {"glRasterPos4s", AmiglRasterPos4s},
        {"glRasterPos4i", AmiglRasterPos4i},
        {"glRasterPos4f", AmiglRasterPos4f},
        {"glRasterPos4d", AmiglRasterPos4d},
        {"glRasterPos2sv", AmiglRasterPos2sv},
        {"glRasterPos2iv", AmiglRasterPos2iv},
        {"glRasterPos2fv", AmiglRasterPos2fv},
        {"glRasterPos2dv", AmiglRasterPos2dv},
        {"glRasterPos3sv", AmiglRasterPos3sv},
        {"glRasterPos3iv", AmiglRasterPos3iv},
        {"glRasterPos3fv", AmiglRasterPos3fv},
        {"glRasterPos3dv", AmiglRasterPos3dv},
        {"glRasterPos4sv", AmiglRasterPos4sv},
        {"glRasterPos4iv", AmiglRasterPos4iv},
        {"glRasterPos4fv", AmiglRasterPos4fv},
        {"glRasterPos4dv", AmiglRasterPos4dv},
        {"glDrawPixels", AmiglDrawPixels},
        {"glCallList", AmiglCallList},
        {"glCallLists", AmiglCallLists},
        {"glDeleteLists", AmiglDeleteLists},
        {"glGenLists", AmiglGenLists},
        {"glNewList", AmiglNewList},
        {"glEndList", AmiglEndList},
        {"glIsList", AmiglIsList},
        {"glListBase", AmiglListBase},
        {"glGetDoublev", AmiglGetDoublev},
        {"glIsTexture", AmiglIsTexture},
        {"glAreTexturesResident", AmiglAreTexturesResident},
        {"glInitNames", AmiglInitNames},
        {"glLoadName", AmiglLoadName},
        {"glPushName", AmiglPushName},
        {"glPopName", AmiglPopName},
        {"glSelectBuffer", AmiglSelectBuffer},
        {"glRenderMode", AmiglRenderMode},
        {"glGetTexLevelParameterfv", AmiglGetTexLevelParameterfv},
        {"glGetTexLevelParameteriv", AmiglGetTexLevelParameteriv},
        {"glMap1f", AmiglMap1f},
        {"glMap1d", AmiglMap1d},
        {"glEvalCoord1f", AmiglEvalCoord1f},
        {"glEvalCoord1d", AmiglEvalCoord1d},
        {"glEvalCoord1fv", AmiglEvalCoord1fv},
        {"glEvalCoord1dv", AmiglEvalCoord1dv},
        {"glMapGrid1f", AmiglMapGrid1f},
        {"glMapGrid1d", AmiglMapGrid1d},
        {"glEvalMesh1", AmiglEvalMesh1},
        {"glEvalPoint1", AmiglEvalPoint1},
        {"glMap2f", AmiglMap2f},
        {"glMap2d", AmiglMap2d},
        {"glEvalCoord2f", AmiglEvalCoord2f},
        {"glEvalCoord2d", AmiglEvalCoord2d},
        {"glEvalCoord2fv", AmiglEvalCoord2fv},
        {"glEvalCoord2dv", AmiglEvalCoord2dv},
        {"glMapGrid2f", AmiglMapGrid2f},
        {"glMapGrid2d", AmiglMapGrid2d},
        {"glEvalMesh2", AmiglEvalMesh2},
        {"glEvalPoint2", AmiglEvalPoint2},
        {"glGetMapfv", AmiglGetMapfv},
        {"glGetMapdv", AmiglGetMapdv},
        {"glGetMapiv", AmiglGetMapiv},
        {"glPushClientAttrib", AmiglPushClientAttrib},
        {"glPopClientAttrib", AmiglPopClientAttrib},
        {"glPixelTransferi", AmiglPixelTransferi},
        {"glPixelTransferf", AmiglPixelTransferf},
        {"glGetTexImage", AmiglGetTexImage},
        {"glCopyTexImage1D", AmiglCopyTexImage1D},
        {"glCopyTexImage2D", AmiglCopyTexImage2D},
        {"glCopyTexSubImage1D", AmiglCopyTexSubImage1D},
        {"glCopyTexSubImage2D", AmiglCopyTexSubImage2D},
        {"glEdgeFlag", AmiglEdgeFlag},
        {"glReadBuffer", AmiglReadBuffer},
        {"glPrioritizeTextures", AmiglPrioritizeTextures},
        {"glDrawRangeElements", AmiglDrawRangeElements},
        {"glGetClipPlane", AmiglGetClipPlane},
        {"gluNewTess", AmigluNewTess},
        {"gluDeleteTess", AmigluDeleteTess},
        {"gluTessProperty", AmigluTessProperty},
        {"gluGetTessProperty", AmigluGetTessProperty},
        {"gluTessNormal", AmigluTessNormal},
        {"gluTessCallback", AmigluTessCallback},
        {"gluTessVertex", AmigluTessVertex},
        {"gluTessBeginPolygon", AmigluTessBeginPolygon},
        {"gluTessBeginContour", AmigluTessBeginContour},
        {"gluTessEndContour", AmigluTessEndContour},
        {"gluTessEndPolygon", AmigluTessEndPolygon},
        {"gluBeginPolygon", AmigluBeginPolygon},
        {"gluNextContour", AmigluNextContour},
        {"gluEndPolygon", AmigluEndPolygon},
        {"gluErrorString", AmigluErrorString},
        {"gluScaleImage", AmigluScaleImage},
        {"gluBuild2DMipmapLevels", AmigluBuild2DMipmapLevels},
        {"gluBuild2DMipmaps", AmigluBuild2DMipmaps},
        {"gluOrtho2D", AmigluOrtho2D},
        {"gluPerspective", AmigluPerspective},
        {"gluLookAt", AmigluLookAt},
        {"gluProject", AmigluProject},
        {"gluUnProject", AmigluUnProject},
        {"gluUnProject4", AmigluUnProject4},
        {"gluPickMatrix", AmigluPickMatrix},
        {"gluNewQuadric", AmigluNewQuadric},
        {"gluDeleteQuadric", AmigluDeleteQuadric},
        {"gluQuadricCallback", AmigluQuadricCallback},
        {"gluQuadricNormals", AmigluQuadricNormals},
        {"gluQuadricTexture", AmigluQuadricTexture},
        {"gluQuadricOrientation", AmigluQuadricOrientation},
        {"gluQuadricDrawStyle", AmigluQuadricDrawStyle},
        {"gluCylinder", AmigluCylinder},
        {"gluDisk", AmigluDisk},
        {"gluPartialDisk", AmigluPartialDisk},
        {"gluSphere", AmigluSphere},
        {"gluGetString", AmigluGetString},
        {"gluCheckExtension", AmigluCheckExtension},
        {"gluNurbsCallbackData", AmigluNurbsCallbackData},
        {"gluBeginCurve", AmigluBeginCurve},
        {"gluBeginSurface", AmigluBeginSurface},
        {"gluBeginTrim", AmigluBeginTrim},
        {"gluDeleteNurbsRenderer", AmigluDeleteNurbsRenderer},
        {"gluEndCurve", AmigluEndCurve},
        {"gluEndSurface", AmigluEndSurface},
        {"gluEndTrim", AmigluEndTrim},
        {"gluGetNurbsProperty", AmigluGetNurbsProperty},
        {"gluLoadSamplingMatrices", AmigluLoadSamplingMatrices},
        {"gluNewNurbsRenderer", AmigluNewNurbsRenderer},
        {"gluNurbsCallback", AmigluNurbsCallback},
        {"gluNurbsCallbackDataEXT", AmigluNurbsCallbackDataEXT},
        {"gluNurbsCurve", AmigluNurbsCurve},
        {"gluNurbsProperty", AmigluNurbsProperty},
        {"gluNurbsSurface", AmigluNurbsSurface},
        {"gluPwlCurve", AmigluPwlCurve},
        {"glGetTexParameteriv", AmiglGetTexParameteriv},
        {"glGetTexParameterfv", AmiglGetTexParameterfv},
        {"glPixelZoom", AmiglPixelZoom},
        {"glLogicOp", AmiglLogicOp},
        {"glCopyPixels", AmiglCopyPixels},
#ifdef NOT_IMPLEMENTED_FUNCS
        #include "SDL_os4_notimplemented_table.t"
#endif
        { NULL, NULL }
   };

   CONST struct MyGLFunc *tb = table;

   do {
      if (!strcmp(tb->name, proc)) {
         return tb->func;
      }
      tb++;
   } while (tb->name);

   return NULL;
}

#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */
