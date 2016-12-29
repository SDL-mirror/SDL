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

#if SDL_VIDEO_DRIVER_AMIGAOS4

#if SDL_VIDEO_OPENGL_ES2

#include <GLES2/gl2.h>
#include <string.h>

/* The OpenGL ES 2.0 API */

static void AmiglActiveTexture (GLenum texture) {
    return glActiveTexture(texture);
}

static void AmiglAttachShader (GLuint program, GLuint shader) {
    return glAttachShader(program, shader);
}

static void AmiglBindAttribLocation (GLuint program, GLuint index, const GLchar *name) {
    return glBindAttribLocation(program, index, name);
}

static void AmiglBindBuffer (GLenum target, GLuint buffer) {
    return glBindBuffer(target, buffer);
}

static void AmiglBindFramebuffer (GLenum target, GLuint framebuffer) {
    return glBindFramebuffer(target, framebuffer);
}

static void AmiglBindRenderbuffer (GLenum target, GLuint renderbuffer) {
    return glBindRenderbuffer(target, renderbuffer);
}

static void AmiglBindTexture (GLenum target, GLuint texture) {
    return glBindTexture(target, texture);
}

static void AmiglBlendColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    return glBlendColor(red, green, blue, alpha);
}

static void AmiglBlendEquation (GLenum mode) {
    return glBlendEquation(mode);
}

static void AmiglBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha) {
    return glBlendEquationSeparate(modeRGB, modeAlpha);
}

static void AmiglBlendFunc (GLenum sfactor, GLenum dfactor) {
    return glBlendFunc(sfactor, dfactor);
}

static void AmiglBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    return glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

static void AmiglBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    return glBufferData(target, size, data, usage);
}

static void AmiglBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    return glBufferSubData(target, offset, size, data);
}

static GLenum AmiglCheckFramebufferStatus (GLenum target) {
    return glCheckFramebufferStatus(target);
}

static void AmiglClear (GLbitfield mask) {
    return glClear(mask);
}

static void AmiglClearColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    return glClearColor(red, green, blue, alpha);
}

static void AmiglClearDepthf (GLfloat d) {
    return glClearDepthf(d);
}

static void AmiglClearStencil (GLint s) {
    return glClearStencil(s);
}

static void AmiglColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    return glColorMask(red, green, blue, alpha);
}

static void AmiglCompileShader (GLuint shader) {
    return glCompileShader(shader);
}

static void AmiglCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
    return glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

static void AmiglCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
    return glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

static void AmiglCopyTexImage2D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    return glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

static void AmiglCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    return glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

static GLuint AmiglCreateProgram (void) {
    return glCreateProgram();
}

static GLuint AmiglCreateShader (GLenum type) {
    return glCreateShader(type);
}

static void AmiglCullFace (GLenum mode) {
    return glCullFace(mode);
}

static void AmiglDeleteBuffers (GLsizei n, const GLuint *buffers) {
    return glDeleteBuffers(n, buffers);
}

static void AmiglDeleteFramebuffers (GLsizei n, const GLuint *framebuffers) {
    return glDeleteFramebuffers(n, framebuffers);
}

static void AmiglDeleteProgram (GLuint program) {
    return glDeleteProgram(program);
}

static void AmiglDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers) {
    return glDeleteRenderbuffers(n, renderbuffers);
}

static void AmiglDeleteShader (GLuint shader) {
    return glDeleteShader(shader);
}

static void AmiglDeleteTextures (GLsizei n, const GLuint *textures) {
    return glDeleteTextures(n, textures);
}

static void AmiglDepthFunc (GLenum func) {
    return glDepthFunc(func);
}

static void AmiglDepthMask (GLboolean flag) {
    return glDepthMask(flag);
}

static void AmiglDepthRangef (GLfloat n, GLfloat f) {
    return glDepthRangef(n, f);
}

static void AmiglDetachShader (GLuint program, GLuint shader) {
    return glDetachShader(program, shader);
}

static void AmiglDisable (GLenum cap) {
    return glDisable(cap);
}

static void AmiglDisableVertexAttribArray (GLuint index) {
    return glDisableVertexAttribArray(index);
}

static void AmiglDrawArrays (GLenum mode, GLint first, GLsizei count) {
    return glDrawArrays(mode, first, count);
}

static void AmiglDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices) {
    return glDrawElements(mode, count, type, indices);
}

static void AmiglEnable (GLenum cap) {
    return glEnable(cap);
}

static void AmiglEnableVertexAttribArray (GLuint index) {
    return glEnableVertexAttribArray(index);
}

static void AmiglFinish (void) {
    return glFinish();
}

static void AmiglFlush (void) {
    return glFlush();
}

static void AmiglFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    return glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

static void AmiglFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    return glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

static void AmiglFrontFace (GLenum mode) {
    return glFrontFace(mode);
}

static void AmiglGenBuffers (GLsizei n, GLuint *buffers) {
    return glGenBuffers(n, buffers);
}

static void AmiglGenerateMipmap (GLenum target) {
    return glGenerateMipmap(target);
}

static void AmiglGenFramebuffers (GLsizei n, GLuint *framebuffers) {
    return glGenFramebuffers(n, framebuffers);
}

static void AmiglGenRenderbuffers (GLsizei n, GLuint *renderbuffers) {
    return glGenRenderbuffers(n, renderbuffers);
}

static void AmiglGenTextures (GLsizei n, GLuint *textures) {
    return glGenTextures(n, textures);
}

static void AmiglGetActiveAttrib (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    return glGetActiveAttrib(program, index, bufSize, length, size, type, name);
}

static void AmiglGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    return glGetActiveUniform(program,  index, bufSize, length, size, type, name);
}

static void AmiglGetAttachedShaders (GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) {
    return glGetAttachedShaders(program, maxCount, count, shaders);
}

static GLint AmiglGetAttribLocation (GLuint program, const GLchar *name) {
    return glGetAttribLocation(program, name);
}

static void AmiglGetBooleanv (GLenum pname, GLboolean *data) {
    return glGetBooleanv(pname, data);
}

static void AmiglGetBufferParameteriv (GLenum target, GLenum pname, GLint *params) {
    return glGetBufferParameteriv(target, pname, params);
}

static GLenum AmiglGetError (void) {
    return glGetError();
}

static void AmiglGetFloatv (GLenum pname, GLfloat *data) {
    return glGetFloatv(pname, data);
}

static void AmiglGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    return glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

static void AmiglGetIntegerv (GLenum pname, GLint *data) {
    return glGetIntegerv(pname, data);
}

static void AmiglGetProgramiv (GLuint program, GLenum pname, GLint *params) {
    return glGetProgramiv(program, pname, params);
}

static void AmiglGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    return glGetProgramInfoLog(program, bufSize, length, infoLog);
}

static void AmiglGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint *params) {
    return glGetRenderbufferParameteriv(target, pname, params);
}

static void AmiglGetShaderiv (GLuint shader, GLenum pname, GLint *params) {
    return glGetShaderiv(shader, pname, params);
}

static void AmiglGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    return glGetShaderInfoLog(shader, bufSize, length, infoLog);
}

static void AmiglGetShaderPrecisionFormat (GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) {
    return glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
}

static void AmiglGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) {
    return glGetShaderSource(shader, bufSize, length, source);
}

static const GLubyte *AmiglGetString (GLenum name) {
    return glGetString(name);
}

static void AmiglGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params) {
    return glGetTexParameterfv(target, pname, params);
}

static void AmiglGetTexParameteriv (GLenum target, GLenum pname, GLint *params) {
    return glGetTexParameteriv(target, pname, params);
}

static void AmiglGetUniformfv (GLuint program, GLint location, GLfloat *params) {
    return glGetUniformfv(program, location, params);
}

static void AmiglGetUniformiv (GLuint program, GLint location, GLint *params) {
    return glGetUniformiv(program, location, params);
}

static GLint AmiglGetUniformLocation (GLuint program, const GLchar *name) {
    return glGetUniformLocation(program, name);
}

static void AmiglGetVertexAttribfv (GLuint index, GLenum pname, GLfloat *params) {
    return glGetVertexAttribfv(index, pname, params);
}

static void AmiglGetVertexAttribiv (GLuint index, GLenum pname, GLint *params) {
    return glGetVertexAttribiv(index, pname, params);
}

static void AmiglGetVertexAttribPointerv (GLuint index, GLenum pname, void **pointer) {
    return glGetVertexAttribPointerv(index, pname, pointer);
}

static void AmiglHint (GLenum target, GLenum mode) {
    return glHint(target, mode);
}

static GLboolean AmiglIsBuffer (GLuint buffer) {
    return glIsBuffer(buffer);
}

static GLboolean AmiglIsEnabled (GLenum cap) {
    return glIsEnabled(cap);
}

static GLboolean AmiglIsFramebuffer (GLuint framebuffer) {
    return glIsFramebuffer(framebuffer);
}

static GLboolean AmiglIsProgram (GLuint program) {
    return glIsProgram(program);
}

static GLboolean AmiglIsRenderbuffer (GLuint renderbuffer) {
    return glIsRenderbuffer(renderbuffer);
}

static GLboolean AmiglIsShader (GLuint shader) {
    return glIsShader(shader);
}

static GLboolean AmiglIsTexture (GLuint texture) {
    return glIsTexture(texture);
}

static void AmiglLineWidth (GLfloat width) {
    return glLineWidth(width);
}

static void AmiglLinkProgram (GLuint program) {
    return glLinkProgram(program);
}

static void AmiglPixelStorei (GLenum pname, GLint param) {
    return glPixelStorei(pname, param);
}

static void AmiglPolygonOffset (GLfloat factor, GLfloat units) {
    return glPolygonOffset(factor, units);
}

static void AmiglReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
    return glReadPixels(x, y, width, height, format, type, pixels);
}

static void AmiglReleaseShaderCompiler (void) {
    return glReleaseShaderCompiler();
}

static void AmiglRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    return glRenderbufferStorage(target, internalformat, width, height);
}

static void AmiglSampleCoverage (GLfloat value, GLboolean invert) {
    return glSampleCoverage(value, invert);
}

static void AmiglScissor (GLint x, GLint y, GLsizei width, GLsizei height) {
    return glScissor(x, y, width, height);
}

static void AmiglShaderBinary (GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length) {
    return glShaderBinary(count, shaders, binaryformat, binary, length);
}

static void AmiglShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {
    return glShaderSource(shader, count, string, length);
}

static void AmiglStencilFunc (GLenum func, GLint ref, GLuint mask) {
    return glStencilFunc(func, ref, mask);
}

static void AmiglStencilFuncSeparate (GLenum face, GLenum func, GLint ref, GLuint mask) {
    return glStencilFuncSeparate(face, func, ref, mask);
}

static void AmiglStencilMask (GLuint mask) {
    return glStencilMask(mask);
}

static void AmiglStencilMaskSeparate (GLenum face, GLuint mask) {
    return glStencilMaskSeparate(face, mask);
}

static void AmiglStencilOp (GLenum fail, GLenum zfail, GLenum zpass) {
    return glStencilOp(fail, zfail, zpass);
}

static void AmiglStencilOpSeparate (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    return glStencilOpSeparate(face, sfail, dpfail, dppass);
}

static void AmiglTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
    return glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

static void AmiglTexParameterf (GLenum target, GLenum pname, GLfloat param) {
    return glTexParameterf(target, pname, param);
}

static void AmiglTexParameterfv (GLenum target, GLenum pname, const GLfloat *params) {
    return glTexParameterfv(target, pname, params);
}

static void AmiglTexParameteri (GLenum target, GLenum pname, GLint param) {
    return glTexParameteri(target, pname, param);
}

static void AmiglTexParameteriv (GLenum target, GLenum pname, const GLint *params) {
    return glTexParameteriv(target, pname, params);
}

static void AmiglTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    return glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

static void AmiglUniform1f (GLint location, GLfloat v0) {
    return glUniform1f(location, v0);
}

static void AmiglUniform1fv (GLint location, GLsizei count, const GLfloat *value) {
    return glUniform1fv(location, count, value);
}

static void AmiglUniform1i (GLint location, GLint v0) {
    return glUniform1i(location, v0);
}

static void AmiglUniform1iv (GLint location, GLsizei count, const GLint *value) {
    return glUniform1iv(location, count, value);
}

static void AmiglUniform2f (GLint location, GLfloat v0, GLfloat v1) {
    return glUniform2f(location, v0, v1);
}

static void AmiglUniform2fv (GLint location, GLsizei count, const GLfloat *value) {
    return glUniform2fv(location, count, value);
}

static void AmiglUniform2i (GLint location, GLint v0, GLint v1) {
    return glUniform2i(location, v0, v1);
}

static void AmiglUniform2iv (GLint location, GLsizei count, const GLint *value) {
    return glUniform2iv(location, count, value);
}

static void AmiglUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    return glUniform3f(location, v0, v1, v2);
}

static void AmiglUniform3fv (GLint location, GLsizei count, const GLfloat *value) {
    return glUniform3fv(location, count, value);
}

static void AmiglUniform3i (GLint location, GLint v0, GLint v1, GLint v2) {
    return glUniform3i(location, v0, v1, v2);
}

static void AmiglUniform3iv (GLint location, GLsizei count, const GLint *value) {
    return glUniform3iv(location, count, value);
}

static void AmiglUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    return glUniform4f(location, v0, v1, v2, v3);
}

static void AmiglUniform4fv (GLint location, GLsizei count, const GLfloat *value) {
    return glUniform4fv(location, count, value);
}

static void AmiglUniform4i (GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    return glUniform4i(location, v0, v1, v2, v3);
}

static void AmiglUniform4iv (GLint location, GLsizei count, const GLint *value) {
    return glUniform4iv(location, count, value);
}

static void AmiglUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    return glUniformMatrix2fv(location, count, transpose, value);
}

static void AmiglUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    return glUniformMatrix3fv(location, count, transpose, value);
}

static void AmiglUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    return glUniformMatrix4fv(location, count, transpose, value);
}

static void AmiglUseProgram (GLuint program) {
    return glUseProgram(program);
}

static void AmiglValidateProgram (GLuint program) {
    return glValidateProgram(program);
}

static void AmiglVertexAttrib1f (GLuint index, GLfloat x) {
    return glVertexAttrib1f(index, x);
}

static void AmiglVertexAttrib1fv (GLuint index, const GLfloat *v) {
    return glVertexAttrib1fv(index, v);
}

static void AmiglVertexAttrib2f (GLuint index, GLfloat x, GLfloat y) {
    return glVertexAttrib2f(index, x, y);
}

static void AmiglVertexAttrib2fv (GLuint index, const GLfloat *v) {
    return glVertexAttrib2fv(index, v);
}

static void AmiglVertexAttrib3f (GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    return glVertexAttrib3f(index, x, y, z);
}

static void AmiglVertexAttrib3fv (GLuint index, const GLfloat *v) {
    return glVertexAttrib3fv(index, v);
}

static void AmiglVertexAttrib4f (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    return glVertexAttrib4f(index, x, y, z, w);
}

static void AmiglVertexAttrib4fv (GLuint index, const GLfloat *v) {
    return glVertexAttrib4fv(index, v);
}

static void AmiglVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    return glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

static void AmiglViewport (GLint x, GLint y, GLsizei width, GLsizei height) {
    return glViewport(x, y, width, height);
}

struct MyGLFunc
{
   CONST_STRPTR name;
   APTR func;
};

#define MY_GL_FUNC(name) {#name, Ami##name},


void *AmiGetGLESProc(const char *proc)
{
   static CONST struct MyGLFunc table[] = {
        MY_GL_FUNC(glActiveTexture)
        MY_GL_FUNC(glAttachShader)
        MY_GL_FUNC(glBindAttribLocation)
        MY_GL_FUNC(glBindBuffer)
        MY_GL_FUNC(glBindFramebuffer)
        MY_GL_FUNC(glBindRenderbuffer)
        MY_GL_FUNC(glBindTexture)
        MY_GL_FUNC(glBlendColor)
        MY_GL_FUNC(glBlendEquation)
        MY_GL_FUNC(glBlendEquationSeparate)
        MY_GL_FUNC(glBlendFunc)
        MY_GL_FUNC(glBlendFuncSeparate)
        MY_GL_FUNC(glBufferData)
        MY_GL_FUNC(glBufferSubData)
        MY_GL_FUNC(glCheckFramebufferStatus)
        MY_GL_FUNC(glClear)
        MY_GL_FUNC(glClearColor)
        MY_GL_FUNC(glClearDepthf)
        MY_GL_FUNC(glClearStencil)
        MY_GL_FUNC(glColorMask)
        MY_GL_FUNC(glCompileShader)
        MY_GL_FUNC(glCompressedTexImage2D)
        MY_GL_FUNC(glCompressedTexSubImage2D)
        MY_GL_FUNC(glCopyTexImage2D)
        MY_GL_FUNC(glCopyTexSubImage2D)
        MY_GL_FUNC(glCreateProgram)
        MY_GL_FUNC(glCreateShader)
        MY_GL_FUNC(glCullFace)
        MY_GL_FUNC(glDeleteBuffers)
        MY_GL_FUNC(glDeleteFramebuffers)
        MY_GL_FUNC(glDeleteProgram)
        MY_GL_FUNC(glDeleteRenderbuffers)
        MY_GL_FUNC(glDeleteShader)
        MY_GL_FUNC(glDeleteTextures)
        MY_GL_FUNC(glDepthFunc)
        MY_GL_FUNC(glDepthMask)
        MY_GL_FUNC(glDepthRangef)
        MY_GL_FUNC(glDetachShader)
        MY_GL_FUNC(glDisable)
        MY_GL_FUNC(glDisableVertexAttribArray)
        MY_GL_FUNC(glDrawArrays)
        MY_GL_FUNC(glDrawElements)
        MY_GL_FUNC(glEnable)
        MY_GL_FUNC(glEnableVertexAttribArray)
        MY_GL_FUNC(glFinish)
        MY_GL_FUNC(glFlush)
        MY_GL_FUNC(glFramebufferRenderbuffer)
        MY_GL_FUNC(glFramebufferTexture2D)
        MY_GL_FUNC(glFrontFace)
        MY_GL_FUNC(glGenBuffers)
        MY_GL_FUNC(glGenerateMipmap)
        MY_GL_FUNC(glGenFramebuffers)
        MY_GL_FUNC(glGenRenderbuffers)
        MY_GL_FUNC(glGenTextures)
        MY_GL_FUNC(glGetActiveAttrib)
        MY_GL_FUNC(glGetActiveUniform)
        MY_GL_FUNC(glGetAttachedShaders)
        MY_GL_FUNC(glGetAttribLocation)
        MY_GL_FUNC(glGetBooleanv)
        MY_GL_FUNC(glGetBufferParameteriv)
        MY_GL_FUNC(glGetError)
        MY_GL_FUNC(glGetFloatv)
        MY_GL_FUNC(glGetFramebufferAttachmentParameteriv)
        MY_GL_FUNC(glGetIntegerv)
        MY_GL_FUNC(glGetProgramiv)
        MY_GL_FUNC(glGetProgramInfoLog)
        MY_GL_FUNC(glGetRenderbufferParameteriv)
        MY_GL_FUNC(glGetShaderiv)
        MY_GL_FUNC(glGetShaderInfoLog)
        MY_GL_FUNC(glGetShaderPrecisionFormat)
        MY_GL_FUNC(glGetShaderSource)
        MY_GL_FUNC(glGetString)
        MY_GL_FUNC(glGetTexParameterfv)
        MY_GL_FUNC(glGetTexParameteriv)
        MY_GL_FUNC(glGetUniformfv)
        MY_GL_FUNC(glGetUniformiv)
        MY_GL_FUNC(glGetUniformLocation)
        MY_GL_FUNC(glGetVertexAttribfv)
        MY_GL_FUNC(glGetVertexAttribiv)
        MY_GL_FUNC(glGetVertexAttribPointerv)
        MY_GL_FUNC(glHint)
        MY_GL_FUNC(glIsBuffer)
        MY_GL_FUNC(glIsEnabled)
        MY_GL_FUNC(glIsFramebuffer)
        MY_GL_FUNC(glIsProgram)
        MY_GL_FUNC(glIsRenderbuffer)
        MY_GL_FUNC(glIsShader)
        MY_GL_FUNC(glIsTexture)
        MY_GL_FUNC(glLineWidth)
        MY_GL_FUNC(glLinkProgram)
        MY_GL_FUNC(glPixelStorei)
        MY_GL_FUNC(glPolygonOffset)
        MY_GL_FUNC(glReadPixels)
        MY_GL_FUNC(glReleaseShaderCompiler)
        MY_GL_FUNC(glRenderbufferStorage)
        MY_GL_FUNC(glSampleCoverage)
        MY_GL_FUNC(glScissor)
        MY_GL_FUNC(glShaderBinary)
        MY_GL_FUNC(glShaderSource)
        MY_GL_FUNC(glStencilFunc)
        MY_GL_FUNC(glStencilFuncSeparate)
        MY_GL_FUNC(glStencilMask)
        MY_GL_FUNC(glStencilMaskSeparate)
        MY_GL_FUNC(glStencilOp)
        MY_GL_FUNC(glStencilOpSeparate)
        MY_GL_FUNC(glTexImage2D)
        MY_GL_FUNC(glTexParameterf)
        MY_GL_FUNC(glTexParameterfv)
        MY_GL_FUNC(glTexParameteri)
        MY_GL_FUNC(glTexParameteriv)
        MY_GL_FUNC(glTexSubImage2D)
        MY_GL_FUNC(glUniform1f)
        MY_GL_FUNC(glUniform1fv)
        MY_GL_FUNC(glUniform1i)
        MY_GL_FUNC(glUniform1iv)
        MY_GL_FUNC(glUniform2f)
        MY_GL_FUNC(glUniform2fv)
        MY_GL_FUNC(glUniform2i)
        MY_GL_FUNC(glUniform2iv)
        MY_GL_FUNC(glUniform3f)
        MY_GL_FUNC(glUniform3fv)
        MY_GL_FUNC(glUniform3i)
        MY_GL_FUNC(glUniform3iv)
        MY_GL_FUNC(glUniform4f)
        MY_GL_FUNC(glUniform4fv)
        MY_GL_FUNC(glUniform4i)
        MY_GL_FUNC(glUniform4iv)
        MY_GL_FUNC(glUniformMatrix2fv)
        MY_GL_FUNC(glUniformMatrix3fv)
        MY_GL_FUNC(glUniformMatrix4fv)
        MY_GL_FUNC(glUseProgram)
        MY_GL_FUNC(glValidateProgram)
        MY_GL_FUNC(glVertexAttrib1f)
        MY_GL_FUNC(glVertexAttrib1fv)
        MY_GL_FUNC(glVertexAttrib2f)
        MY_GL_FUNC(glVertexAttrib2fv)
        MY_GL_FUNC(glVertexAttrib3f)
        MY_GL_FUNC(glVertexAttrib3fv)
        MY_GL_FUNC(glVertexAttrib4f)
        MY_GL_FUNC(glVertexAttrib4fv)
        MY_GL_FUNC(glVertexAttribPointer)
        MY_GL_FUNC(glViewport)
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

#endif /* SDL_VIDEO_OPENGL_ES2 */
#endif /* SDL_VIDEO_DRIVER_AMIGAOS4 */
