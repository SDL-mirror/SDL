/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 2010 itsnotabigtruck.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "SDL_config.h"

#if SDL_VIDEO_RENDER_OGL_ES2

#ifndef SDL_shaderdata_h_
#define SDL_shaderdata_h_

typedef struct GLES2_ShaderInstance
{
    GLenum type;
    GLenum format;
    int length;
    const void *data;
} GLES2_ShaderInstance;

typedef struct GLES2_Shader
{
    int instance_count;
    const GLES2_ShaderInstance *instances[4];
} GLES2_Shader;

typedef enum
{
    GLES2_SHADER_VERTEX_DEFAULT,
    GLES2_SHADER_FRAGMENT_SOLID_SRC,
    GLES2_SHADER_FRAGMENT_TEXTURE_SRC
} GLES2_ShaderType;

#define GLES2_SOURCE_SHADER (GLenum)-1

const GLES2_Shader *GLES2_GetShader(GLES2_ShaderType type, SDL_BlendMode blendMode);

#endif /* SDL_shaderdata_h_ */

#endif /* SDL_VIDEO_RENDER_OGL_ES2 */

/* vi: set ts=4 sw=4 expandtab: */
