/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2003  Sam Lantinga

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

#include "SDL_QuartzVideo.h"


@implementation NSOpenGLContext (CGLContextAccess)
- (CGLContextObj) cglContext;
{
    return _contextAuxiliary;
}
@end

/* OpenGL helper functions (used internally) */

int QZ_SetupOpenGL (_THIS, int bpp, Uint32 flags) {

    NSOpenGLPixelFormatAttribute attr[32];
    NSOpenGLPixelFormat *fmt;
    int i = 0;
    int colorBits = bpp;

    if ( flags & SDL_FULLSCREEN ) {

        attr[i++] = NSOpenGLPFAFullScreen;
    }
    /* In windowed mode, the OpenGL pixel depth must match device pixel depth */
    else if ( colorBits != device_bpp ) {

        colorBits = device_bpp;
    }

    attr[i++] = NSOpenGLPFAColorSize;
    attr[i++] = colorBits;

    attr[i++] = NSOpenGLPFADepthSize;
    attr[i++] = this->gl_config.depth_size;

    if ( this->gl_config.double_buffer ) {
        attr[i++] = NSOpenGLPFADoubleBuffer;
    }

    if ( this->gl_config.stereo ) {
        attr[i++] = NSOpenGLPFAStereo;
    }

    if ( this->gl_config.stencil_size != 0 ) {
        attr[i++] = NSOpenGLPFAStencilSize;
        attr[i++] = this->gl_config.stencil_size;
    }

#if NSOPENGL_CURRENT_VERSION > 1  /* What version should this be? */
    if ( this->gl_config.multisamplebuffers != 0 ) {
        attr[i++] = NSOpenGLPFASampleBuffers;
        attr[i++] = this->gl_config.multisamplebuffers;
    }

    if ( this->gl_config.multisamplesamples != 0 ) {
        attr[i++] = NSOpenGLPFASamples;
        attr[i++] = this->gl_config.multisamplesamples;
    }
#endif

    attr[i++] = NSOpenGLPFAScreenMask;
    attr[i++] = CGDisplayIDToOpenGLDisplayMask (display_id);
    attr[i] = 0;

    fmt = [ [ NSOpenGLPixelFormat alloc ] initWithAttributes:attr ];
    if (fmt == nil) {
        SDL_SetError ("Failed creating OpenGL pixel format");
        return 0;
    }

    gl_context = [ [ NSOpenGLContext alloc ] initWithFormat:fmt
                                               shareContext:nil];

    if (gl_context == nil) {
        SDL_SetError ("Failed creating OpenGL context");
        return 0;
    }

    /*
     * Wisdom from Apple engineer in reference to UT2003's OpenGL performance:
     *  "You are blowing a couple of the internal OpenGL function caches. This
     *  appears to be happening in the VAO case.  You can tell OpenGL to up
     *  the cache size by issuing the following calls right after you create
     *  the OpenGL context.  The default cache size is 16."    --ryan.
     */

    #ifndef GLI_ARRAY_FUNC_CACHE_MAX
    #define GLI_ARRAY_FUNC_CACHE_MAX 284
    #endif

    #ifndef GLI_SUBMIT_FUNC_CACHE_MAX
    #define GLI_SUBMIT_FUNC_CACHE_MAX 280
    #endif

    {
        long cache_max = 64;
        CGLContextObj ctx = [ gl_context cglContext ];
        CGLSetParameter (ctx, GLI_SUBMIT_FUNC_CACHE_MAX, &cache_max);
        CGLSetParameter (ctx, GLI_ARRAY_FUNC_CACHE_MAX, &cache_max);
    }

    /* End Wisdom from Apple Engineer section. --ryan. */

    /* Convince SDL that the GL "driver" is loaded */
    this->gl_config.driver_loaded = 1;

    [ fmt release ];

    return 1;
}

void QZ_TearDownOpenGL (_THIS) {

    [ NSOpenGLContext clearCurrentContext ];
    [ gl_context clearDrawable ];
    [ gl_context release ];
}


/* SDL OpenGL functions */

int    QZ_GL_LoadLibrary    (_THIS, const char *location) {
    this->gl_config.driver_loaded = 1;
    return 1;
}

void*  QZ_GL_GetProcAddress (_THIS, const char *proc) {

    /* We may want to cache the bundleRef at some point */
    CFBundleRef bundle;
    CFURLRef bundleURL = CFURLCreateWithFileSystemPath (kCFAllocatorDefault,
                                                        CFSTR("/System/Library/Frameworks/OpenGL.framework"), kCFURLPOSIXPathStyle, true);

    CFStringRef functionName = CFStringCreateWithCString
        (kCFAllocatorDefault, proc, kCFStringEncodingASCII);

    void *function;

    bundle = CFBundleCreate (kCFAllocatorDefault, bundleURL);
    assert (bundle != NULL);

    function = CFBundleGetFunctionPointerForName (bundle, functionName);

    CFRelease ( bundleURL );
    CFRelease ( functionName );
    CFRelease ( bundle );

    return function;
}

int    QZ_GL_GetAttribute   (_THIS, SDL_GLattr attrib, int* value) {

    GLenum attr = 0;

    QZ_GL_MakeCurrent (this);

    switch (attrib) {
        case SDL_GL_RED_SIZE: attr = GL_RED_BITS;   break;
        case SDL_GL_BLUE_SIZE: attr = GL_BLUE_BITS;  break;
        case SDL_GL_GREEN_SIZE: attr = GL_GREEN_BITS; break;
        case SDL_GL_ALPHA_SIZE: attr = GL_ALPHA_BITS; break;
        case SDL_GL_DOUBLEBUFFER: attr = GL_DOUBLEBUFFER; break;
        case SDL_GL_DEPTH_SIZE: attr = GL_DEPTH_BITS;  break;
        case SDL_GL_STENCIL_SIZE: attr = GL_STENCIL_BITS; break;
        case SDL_GL_ACCUM_RED_SIZE: attr = GL_ACCUM_RED_BITS; break;
        case SDL_GL_ACCUM_GREEN_SIZE: attr = GL_ACCUM_GREEN_BITS; break;
        case SDL_GL_ACCUM_BLUE_SIZE: attr = GL_ACCUM_BLUE_BITS; break;
        case SDL_GL_ACCUM_ALPHA_SIZE: attr = GL_ACCUM_ALPHA_BITS; break;
        case SDL_GL_STEREO: attr = GL_STEREO; break;
        case SDL_GL_MULTISAMPLEBUFFERS: attr = GL_SAMPLE_BUFFERS_ARB; break;
        case SDL_GL_MULTISAMPLESAMPLES: attr = GL_SAMPLES_ARB; break;
        case SDL_GL_BUFFER_SIZE:
        {
            GLint bits = 0;
            GLint component;

            /* there doesn't seem to be a single flag in OpenGL for this! */
            glGetIntegerv (GL_RED_BITS, &component);   bits += component;
            glGetIntegerv (GL_GREEN_BITS,&component);  bits += component;
            glGetIntegerv (GL_BLUE_BITS, &component);  bits += component;
            glGetIntegerv (GL_ALPHA_BITS, &component); bits += component;

            *value = bits;
        }
        return 0;
    }

    glGetIntegerv (attr, (GLint *)value);
    return 0;
}

int    QZ_GL_MakeCurrent    (_THIS) {
    [ gl_context makeCurrentContext ];
    return 0;
}

void   QZ_GL_SwapBuffers    (_THIS) {
    [ gl_context flushBuffer ];
}
