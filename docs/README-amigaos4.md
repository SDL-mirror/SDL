================================================================================
SDL 2.0 requirements
================================================================================

AmigaOS 4.1 Final Edition
MiniGL (optional from SDL2 point of view, but OpenGL context might still be
        required by the SDL2 application)
OpenGL ES 2.0 (optional)

================================================================================
Building SDL 2.0 library
================================================================================

    sh configure --disable-altivec --prefix=/SDK/local/newlib/
    make

After building, "make install" should work. Optionally you can also build tests:

    cd test
    sh configure
    make

================================================================================
Using SDL 2.0 in your projects
================================================================================

    #include "SDL2/SDL.h"
    ...do magical SDL2 things...


    gcc helloworld.c -use-dynld -lSDL2 -lpthread

================================================================================
About SDL_Renderers
================================================================================

A renderer is a subsystem that can do 2D drawing. We have 4 renderers:
software, OpenGL, OpenGL ES 2.0 and compositing.

Software renderer is always available. Pixels are plotted by the CPU so this is
usually a slow option.

OpenGL renderer uses MiniGL (and Warp3D) for accelerated drawing. Drawing is
done in immediate mode, there is no batching. This should be fairly fast if
textures are static.

OpenGL ES 2.0 renderer uses ogles2.library (and Warp3D Nova).

Compositing renderer uses AmigaOS 4 graphics.library for accelerated drawing.
However, blended lines and points are not accelerated since compositing doesn't
support them. Compositing renderer supports only 32-bit bitmaps. If (Workbench)
screen mode is 16-bit, color format conversion can slow things down.

It's possible to select the preferred renderer before its creation, like this:

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, name);

where name is "software", "opengl" or "compositing".

It's possible to enable VSYNC with:

    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

There is a benchmark tool called sdl2benchmark which was written to test
available renderers.

================================================================================
About OpenGL
================================================================================

If you want to draw accelerated 3D graphics or use explicitly OpenGL functions,
you have to create an OpenGL context, instead of a SDL_Renderer.

If you would like to create an OpenGL ES 2.0 context, you need to specify the
version before window creation, for example:

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

MiniGL context can be created using major version 1 and minor version 3. This is
also the default setup.

================================================================================
WinUAE
================================================================================

Because WinUAE doesn't support hardware-accelerated compositing or 3D, you need
to install the following software:

- http://os4depot.net/index.php?function=showfile&file=graphics/misc/patchcompositetags.lha
- http://os4depot.net/index.php?function=showfile&file=library/graphics/wazp3d.lha

================================================================================
Tips
================================================================================

If you are already familiar with SDL 1.2, or porting SDL 1.2 code, it's worth
checking the migration guide at:

https://wiki.libsdl.org/MigrationGuide

Always check the return values of functions and in error case you can get more
information using SDL_GetError() function!

================================================================================
Limitations
================================================================================

Altivec support is disabled. It should be possible to enable in private builds
but it hasn't been tested so far.

Unsupported subsystems include Haptic and Power. There is no Vulkan backend for
AmigaOS either.

OpenGL renderer doesn't support render targets and blend modes "ADD" or "MOD".
This is due to missing features in MiniGL.

================================================================================
Bugs
================================================================================

Old bug tracker (ramping down): https://sourceforge.net/projects/sdl2-amigaos4/
New bug tracker: https://github.com/AmigaPorts/sdl2-amigaos4/issues
