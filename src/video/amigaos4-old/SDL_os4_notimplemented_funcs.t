#define FUNC_NOT_IMPLEMENTED printf("function (%s) not implemented on file (%s),%d\n",__FUNCTION__, __FILE__, __LINE__);

static void AmiglClearIndex( GLfloat c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glClearIndex(c);
#endif
}

static void AmiglIndexMask( GLuint mask ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexMask(mask);
#endif
}

static void AmiglGetPolygonStipple( GLubyte *mask ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetPolygonStipple(mask);
#endif
}

static void AmiglEdgeFlagv( const GLboolean *flag ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glEdgeFlagv(flag);
#endif
}

static void AmiglClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glClearAccum(red, green, blue, alpha);
#endif
}

static void AmiglAccum( GLenum op, GLfloat value ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glAccum(op, value);
#endif
}

static void AmiglIndexd( GLdouble c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexd(c);
#endif
}

static void AmiglIndexf( GLfloat c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexf(c);
#endif
}

static void AmiglIndexi( GLint c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexi(c);
#endif
}

static void AmiglIndexs( GLshort c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexs(c);
#endif
}

static void AmiglIndexub( GLubyte c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexub(c);
#endif
}

static void AmiglIndexdv( const GLdouble *c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexdv(c);
#endif
}

static void AmiglIndexfv( const GLfloat *c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexfv(c);
#endif
}

static void AmiglIndexiv( const GLint *c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexiv(c);
#endif
}

static void AmiglIndexsv( const GLshort *c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexsv(c);
#endif
}

static void AmiglIndexubv( const GLubyte *c ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexubv(c);
#endif
}

static void AmiglTexCoord1s( GLshort s ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord1s(s);
#endif
}

static void AmiglTexCoord2s( GLshort s, GLshort t ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord2s(s, t);
#endif
}

static void AmiglTexCoord3d( GLdouble s, GLdouble t, GLdouble r ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord3d(s, t, r);
#endif
}

static void AmiglTexCoord3i( GLint s, GLint t, GLint r ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord3i(s, t, r);
#endif
 }

static void AmiglTexCoord3s( GLshort s, GLshort t, GLshort r ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord3s(s, t, r);
#endif
 }

static void AmiglTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord4d(s, t, r, q);
#endif
}

static void AmiglTexCoord4i( GLint s, GLint t, GLint r, GLint q ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord4i(s, t, r, q);
#endif
}

static void AmiglTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord4s(s, t, r, q);
#endif
}

static void AmiglTexCoord1dv( const GLdouble *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord1dv(v);
#endif
}

static void AmiglTexCoord1fv( const GLfloat *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord1fv(v);
#endif
}

static void AmiglTexCoord1iv( const GLint *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord1iv(v);
#endif
}

static void AmiglTexCoord1sv( const GLshort *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord1sv(v);
#endif
}

static void AmiglTexCoord2dv( const GLdouble *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord2dv(v);
#endif
}

static void AmiglTexCoord2sv( const GLshort *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord2sv(v);
#endif
}

static void AmiglTexCoord3dv( const GLdouble *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord3dv(v);
#endif
}

static void AmiglTexCoord3iv( const GLint *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord3iv(v);
#endif
}

static void AmiglTexCoord3sv( const GLshort *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord3sv(v);
#endif
}

static void AmiglTexCoord4dv( const GLdouble *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord4dv(v);
#endif
}

static void AmiglTexCoord4iv( const GLint *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord4iv(v);
#endif
}

static void AmiglTexCoord4sv( const GLshort *v ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoord4sv(v);
#endif
}

static void AmiglIndexPointer( GLenum type, GLsizei stride, const GLvoid *ptr ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexPointer(type, stride, ptr);
#endif
}

static void AmiglEdgeFlagPointer( GLsizei stride, const GLboolean *ptr ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glEdgeFlagPointer(stride, ptr);
#endif
}

static void AmiglGetPointerv( GLenum pname, void **params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetPointerv(pname, params);
#endif
}

static void AmiglPixelMapfv( GLenum map, GLint mapsize, const GLfloat *values ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glPixelMapfv(map, mapsize, values);
#endif
}

static void AmiglPixelMapuiv( GLenum map, GLint mapsize, const GLuint *values ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glPixelMapuiv(map, mapsize, values);
#endif
}

static void AmiglPixelMapusv( GLenum map, GLint mapsize, const GLushort *values ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glPixelMapusv(map, mapsize, values);
#endif
}

static void AmiglGetPixelMapfv( GLenum map, GLfloat *values ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetPixelMapfv(map, values);
#endif
}

static void AmiglGetPixelMapuiv( GLenum map, GLuint *values ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetPixelMapuiv(map, values);
#endif
}

static void AmiglGetPixelMapusv( GLenum map, GLushort *values ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetPixelMapusv(map, values);
#endif
}

static void AmiglTexGend( GLenum coord, GLenum pname, GLdouble param ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexGend(coord, pname, param);
#endif
}

static void AmiglTexGenf( GLenum coord, GLenum pname, GLfloat param ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexGenf(coord, pname, param);
#endif
}

static void AmiglTexGendv( GLenum coord, GLenum pname, const GLdouble *params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexGendv(coord, pname, params);
#endif
}

static void AmiglTexGeniv( GLenum coord, GLenum pname, const GLint *params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexGeniv(coord, pname, params);
#endif
}

static void AmiglGetTexGendv( GLenum coord, GLenum pname, GLdouble *params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetTexGendv(coord, pname, params);
#endif
}

static void AmiglGetTexGenfv( GLenum coord, GLenum pname, GLfloat *params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetTexGenfv(coord, pname, params);
#endif
}

static void AmiglGetTexGeniv( GLenum coord, GLenum pname, GLint *params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetTexGeniv(coord, pname, params);
#endif
}

static void AmiglGetTexEnvfv( GLenum target, GLenum pname, GLfloat *params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetTexEnvfv(target, pname, params);
#endif
}

static void AmiglFogiv( GLenum pname, const GLint *params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glFogiv(pname, params);
#endif
 }

static void AmiglFeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glFeedbackBuffer(size, type, buffer);
#endif
}

static void AmiglPassThrough( GLfloat token ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glPassThrough(token);
#endif
}

static void AmiglBlendEquationEXT( GLenum mode ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glBlendEquationEXT(mode);
#endif
}

static void AmiglBlendColorEXT( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glBlendColorEXT(red, green, blue, alpha);
#endif
}

static void AmiglPolygonOffsetEXT( GLfloat factor, GLfloat bias ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glPolygonOffsetEXT(factor, bias);
#endif
}

static void AmiglVertexPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glVertexPointerEXT(size, type, stride, count, ptr);
#endif
}

static void AmiglNormalPointerEXT( GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glNormalPointerEXT(type, stride, count, ptr);
#endif
}

static void AmiglColorPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glColorPointerEXT(size, type, stride, count, ptr);
#endif
}

static void AmiglIndexPointerEXT( GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIndexPointerEXT(type, stride, count, ptr);
#endif
}

static void AmiglTexCoordPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexCoordPointerEXT(size, type, stride, count, ptr);
#endif
}

static void AmiglEdgeFlagPointerEXT( GLsizei stride, GLsizei count, const GLboolean *ptr ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glEdgeFlagPointerEXT(stride, count, ptr);
#endif
}

static void AmiglGetPointervEXT( GLenum pname, void **params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetPointervEXT(pname, params);
#endif
 }

static void AmiglArrayElementEXT( GLint i ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glArrayElementEXT(i);
#endif
}

static void AmiglDrawArraysEXT( GLenum mode, GLint first, GLsizei count ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glDrawArraysEXT(mode, first, count);
#endif
}

static void AmiglGenTexturesEXT( GLsizei n, GLuint *textures ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGenTexturesEXT(n, textures);
#endif
}

static void AmiglDeleteTexturesEXT( GLsizei n, const GLuint *textures) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glDeleteTexturesEXT(n, textures);
#endif
}

static void AmiglBindTextureEXT( GLenum target, GLuint texture ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glBindTextureEXT(target, texture);
#endif
}

static void AmiglPrioritizeTexturesEXT( GLsizei n, const GLuint *textures, const GLclampf *priorities ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glPrioritizeTexturesEXT(n, textures, priorities);
#endif
}

static GLboolean AmiglAreTexturesResidentEXT( GLsizei n, const GLuint *textures, GLboolean *residences ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glAreTexturesResidentEXT(n, textures, residences);
#else
	return 0;
#endif
}

static GLboolean AmiglIsTextureEXT( GLuint texture ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glIsTextureEXT(texture);
#else
	return 0;
#endif
}

static void AmiglTexImage3DEXT( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexImage3DEXT(target, level, internalFormat, width, height, depth, border, format, type, pixels);
#endif
}

static void AmiglTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
#endif
}

static void AmiglCopyTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height);
#endif
}

static void AmiglColorSubTableEXT( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glColorSubTableEXT(target, start, count, format, type, data);
#endif
}

static void AmiglGetColorTableEXT( GLenum target, GLenum format, GLenum type, GLvoid *table ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetColorTableEXT(target, format, type, table);
#endif
}

static void AmiglGetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat *params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetColorTableParameterfvEXT(target, pname, params);
#endif
}

static void AmiglGetColorTableParameterivEXT( GLenum target, GLenum pname, GLint *params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glGetColorTableParameterivEXT(target, pname, params);
#endif
}

static void AmiglMultiTexCoord1dSGIS(GLenum target, GLdouble s) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1dSGIS(target, s);
#endif
}

static void AmiglMultiTexCoord1dvSGIS(GLenum target, const GLdouble *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1dvSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord1fSGIS(GLenum target, GLfloat s) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1fSGIS(target, s);
#endif
}
static void AmiglMultiTexCoord1fvSGIS(GLenum target, const GLfloat *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1fvSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord1iSGIS(GLenum target, GLint s) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1iSGIS(target, s);
#endif
}

static void AmiglMultiTexCoord1ivSGIS(GLenum target, const GLint *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1ivSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord1sSGIS(GLenum target, GLshort s) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1sSGIS(target, s);
#endif
}

static void AmiglMultiTexCoord1svSGIS(GLenum target, const GLshort *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1svSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord2dSGIS(GLenum target, GLdouble s, GLdouble t) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2dSGIS(target, s, t);
#endif
}

static void AmiglMultiTexCoord2dvSGIS(GLenum target, const GLdouble *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2dvSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord2fSGIS(GLenum target, GLfloat s, GLfloat t) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2fSGIS(target, s, t);
#endif
}

static void AmiglMultiTexCoord2fvSGIS(GLenum target, const GLfloat *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2fvSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord2iSGIS(GLenum target, GLint s, GLint t) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2iSGIS(target, s, t);
#endif
}

static void AmiglMultiTexCoord2ivSGIS(GLenum target, const GLint *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2ivSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord2sSGIS(GLenum target, GLshort s, GLshort t) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2sSGIS(target, s, t);
#endif
}

static void AmiglMultiTexCoord2svSGIS(GLenum target, const GLshort *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2svSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord3dSGIS(GLenum target, GLdouble s, GLdouble t, GLdouble r) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3dSGIS(target, s, t, r);
#endif
}

static void AmiglMultiTexCoord3dvSGIS(GLenum target, const GLdouble *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3dvSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord3fSGIS(GLenum target, GLfloat s, GLfloat t, GLfloat r) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3fSGIS(target, s, t, r);
#endif
}

static void AmiglMultiTexCoord3fvSGIS(GLenum target, const GLfloat *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3fvSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord3iSGIS(GLenum target, GLint s, GLint t, GLint r) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3iSGIS(target, s, t, r);
#endif
}

static void AmiglMultiTexCoord3ivSGIS(GLenum target, const GLint *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3ivSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord3sSGIS(GLenum target, GLshort s, GLshort t, GLshort r) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3sSGIS(target, s, t, r);
#endif
}

static void AmiglMultiTexCoord3svSGIS(GLenum target, const GLshort *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3svSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord4dSGIS(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4dSGIS(target, s, t, r, q);
#endif
}

static void AmiglMultiTexCoord4dvSGIS(GLenum target, const GLdouble *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4dvSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord4fSGIS(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4fSGIS(target, s, t, r, q);
#endif
}

static void AmiglMultiTexCoord4fvSGIS(GLenum target, const GLfloat *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4fvSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord4iSGIS(GLenum target, GLint s, GLint t, GLint r, GLint q) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4iSGIS(target, s, t, r, q);
#endif
}

static void AmiglMultiTexCoord4ivSGIS(GLenum target, const GLint *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4ivSGIS(target, v);
#endif
}

static void AmiglMultiTexCoord4sSGIS(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4sSGIS(target, s, t, r, q);
#endif
}

static void AmiglMultiTexCoord4svSGIS(GLenum target, const GLshort *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4svSGIS(target, v);
#endif
}

static void AmiglMultiTexCoordPointerSGIS(GLenum target, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoordPointerSGIS(target, size, type, stride, pointer);
#endif
}

static void AmiglSelectTextureSGIS(GLenum target) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glSelectTextureSGIS(target);
#endif
}

static void AmiglSelectTextureCoordSetSGIS(GLenum target) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glSelectTextureCoordSetSGIS(target);
#endif
}

static void AmiglMultiTexCoord1dEXT(GLenum target, GLdouble s) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1dEXT(target, s);
#endif
}

static void AmiglMultiTexCoord1dvEXT(GLenum target, const GLdouble *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1dvEXT(target, v);
#endif
}

static void AmiglMultiTexCoord1fEXT(GLenum target, GLfloat s) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1fEXT(target, s);
#endif
}

static void AmiglMultiTexCoord1fvEXT(GLenum target, const GLfloat *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1fvEXT(target, v);
#endif
}

static void AmiglMultiTexCoord1iEXT(GLenum target, GLint s) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1iEXT(target, s);
#endif
}

static void AmiglMultiTexCoord1ivEXT(GLenum target, const GLint *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1ivEXT(target, v);
#endif
}

static void AmiglMultiTexCoord1sEXT(GLenum target, GLshort s) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1sEXT(target, s);
#endif
}

static void AmiglMultiTexCoord1svEXT(GLenum target, const GLshort *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord1svEXT(target, v);
#endif
}

static void AmiglMultiTexCoord2dEXT(GLenum target, GLdouble s, GLdouble t) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2dEXT(target, s, t);
#endif
}

static void AmiglMultiTexCoord2dvEXT(GLenum target, const GLdouble *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2dvEXT(target, v);
#endif
}

static void AmiglMultiTexCoord2fEXT(GLenum target, GLfloat s, GLfloat t) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2fEXT(target, s, t);
#endif
}

static void AmiglMultiTexCoord2fvEXT(GLenum target, const GLfloat *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2fvEXT(target, v);
#endif
}

static void AmiglMultiTexCoord2iEXT(GLenum target, GLint s, GLint t) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2iEXT(target, s, t);
#endif
}

static void AmiglMultiTexCoord2ivEXT(GLenum target, const GLint *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2ivEXT(target, v);
#endif
}

static void AmiglMultiTexCoord2sEXT(GLenum target, GLshort s, GLshort t) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2sEXT(target, s, t);
#endif
}
static void AmiglMultiTexCoord2svEXT(GLenum target, const GLshort *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord2svEXT(target, v);
#endif
}

static void AmiglMultiTexCoord3dEXT(GLenum target, GLdouble s, GLdouble t, GLdouble r) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3dEXT(target, s, t, r);
#endif
}

static void AmiglMultiTexCoord3dvEXT(GLenum target, const GLdouble *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3dvEXT(target, v);
#endif
}

static void AmiglMultiTexCoord3fEXT(GLenum target, GLfloat s, GLfloat t, GLfloat r) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3fEXT(target, s, t, r);
#endif
}

static void AmiglMultiTexCoord3fvEXT(GLenum target, const GLfloat *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3fvEXT(target, v);
#endif
}

static void AmiglMultiTexCoord3iEXT(GLenum target, GLint s, GLint t, GLint r) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3iEXT(target, s, t, r);
#endif
}

static void AmiglMultiTexCoord3ivEXT(GLenum target, const GLint *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3ivEXT(target, v);
#endif
}

static void AmiglMultiTexCoord3sEXT(GLenum target, GLshort s, GLshort t, GLshort r) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3sEXT(target, s, t, r);
#endif
}

static void AmiglMultiTexCoord3svEXT(GLenum target, const GLshort *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord3svEXT(target, v);
#endif
}

static void AmiglMultiTexCoord4dEXT(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4dEXT(target, s, t, r, q);
#endif
}

static void AmiglMultiTexCoord4dvEXT(GLenum target, const GLdouble *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4dvEXT(target, v);
#endif
}

static void AmiglMultiTexCoord4fEXT(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4fEXT(target, s, t, r, q);
#endif
}

static void AmiglMultiTexCoord4fvEXT(GLenum target, const GLfloat *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4fvEXT(target, v);
#endif
}

static void AmiglMultiTexCoord4iEXT(GLenum target, GLint s, GLint t, GLint r, GLint q) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4iEXT(target, s, t, r, q);
#endif
}

static void AmiglMultiTexCoord4ivEXT(GLenum target, const GLint *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4ivEXT(target, v);
#endif
}

static void AmiglMultiTexCoord4sEXT(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4sEXT(target, s, t, r, q);
#endif
}

static void AmiglMultiTexCoord4svEXT(GLenum target, const GLshort *v) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glMultiTexCoord4svEXT(target, v);
#endif
}

static void AmiglInterleavedTextureCoordSetsEXT( GLint factor ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glInterleavedTextureCoordSetsEXT(factor);
#endif
}

static void AmiglSelectTextureEXT( GLenum target ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glSelectTextureEXT(target);
#endif
}

static void AmiglSelectTextureCoordSetEXT( GLenum target ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glSelectTextureCoordSetEXT(target);
#endif
}

static void AmiglSelectTextureTransformEXT( GLenum target ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glSelectTextureTransformEXT(target);
#endif
}

static void AmiglPointParameterfEXT( GLenum pname, GLfloat param ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glPointParameterfEXT(pname, param);
#endif
}

static void AmiglPointParameterfvEXT( GLenum pname, GLfloat *params ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glPointParameterfvEXT(pname, params);
#endif
}

static void AmiglWindowPos2iMESA( GLint x, GLint y ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos2iMESA(x, y);
#endif
}

static void AmiglWindowPos2sMESA( GLshort x, GLshort y ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos2sMESA(x, y);
#endif
}

static void AmiglWindowPos2fMESA( GLfloat x, GLfloat y ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos2fMESA(x, y);
#endif
}

static void AmiglWindowPos2dMESA( GLdouble x, GLdouble y ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos2dMESA(x, y);
#endif
}

static void AmiglWindowPos2ivMESA( const GLint *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos2ivMESA(p);
#endif
}

static void AmiglWindowPos2svMESA( const GLshort *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos2svMESA(p);
#endif
}

static void AmiglWindowPos2fvMESA( const GLfloat *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos2fvMESA(p);
#endif
}

static void AmiglWindowPos2dvMESA( const GLdouble *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos2dvMESA(p);
#endif
}

static void AmiglWindowPos3iMESA( GLint x, GLint y, GLint z ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos3iMESA(x, y, z);
#endif
}

static void AmiglWindowPos3sMESA( GLshort x, GLshort y, GLshort z ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos3sMESA(x, y, z);
#endif
}

static void AmiglWindowPos3fMESA( GLfloat x, GLfloat y, GLfloat z ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos3fMESA(x, y, z);
#endif
}

static void AmiglWindowPos3dMESA( GLdouble x, GLdouble y, GLdouble z ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos3dMESA(x, y, z);
#endif
}

static void AmiglWindowPos3ivMESA( const GLint *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos3ivMESA(p);
#endif
}

static void AmiglWindowPos3svMESA( const GLshort *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos3svMESA(p);
#endif
}

static void AmiglWindowPos3fvMESA( const GLfloat *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos3fvMESA(p);
#endif
}

static void AmiglWindowPos3dvMESA( const GLdouble *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos3dvMESA(p);
#endif
}

static void AmiglWindowPos4iMESA( GLint x, GLint y, GLint z, GLint w ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos4iMESA(x, y, z, w);
#endif
}

static void AmiglWindowPos4sMESA( GLshort x, GLshort y, GLshort z, GLshort w ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos4sMESA(x, y, z, w);
#endif
}

static void AmiglWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos4fMESA(x, y, z, w);
#endif
}

static void AmiglWindowPos4dMESA( GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos4dMESA(x, y, z, w);
#endif
}

static void AmiglWindowPos4ivMESA( const GLint *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos4ivMESA(p);
#endif
}

static void AmiglWindowPos4svMESA( const GLshort *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos4svMESA(p);
#endif
}

static void AmiglWindowPos4fvMESA( const GLfloat *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos4fvMESA(p);
#endif
}

static void AmiglWindowPos4dvMESA( const GLdouble *p ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glWindowPos4dvMESA(p);
#endif
}

static void AmiglResizeBuffersMESA( void ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glResizeBuffersMESA();
#endif
}

static void AmiglTexImage3D( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);
#endif
}

static void AmiglTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
#endif
}

static void AmiglCopyTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
FUNC_NOT_IMPLEMENTED
#ifndef __amigaos4__
	return glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
#endif
}



