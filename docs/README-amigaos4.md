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

A renderer is a subsystem that can do 2D drawing. We have three renderers:
software, OpenGL and compositing.

Software renderer is always available. Pixels are plotted by the CPU so this is
usually the slowest option.

OpenGL renderer uses MiniGL (and Warp3D) for accelerated drawing. Drawing is
done in immediate mode, there is no batching. This should be fairly fast if
textures are static.

Compositing renderer uses AmigaOS 4 graphics.library for accelerated drawing.
However, blended lines and points are not accelerated since compositing doesn't
support them. Compositing renderer currently supports only 32-bit bitmaps. If
(Workbench) screen mode is 16-bit, color format conversion can slow things down.

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
you have to create an OpenGL context, instead of SDL_Renderer.

If you would like to create an OpenGL ES 2.0 context, you need to specify the
version before window creation, for example:

SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

If context version is not specified, or OpenGL ES 2.0 is not supported by the
system, MiniGL context is created instead of.

================================================================================
Tips
================================================================================

If you are already familiar with SDL 1.2, or porting SDL 1.2 code, it's worth
checking the migration guide at:

https://wiki.libsdl.org/MigrationGuide

================================================================================
Bugs
================================================================================

It's best to report bugs (as tickets) on the project page:

https://sourceforge.net/projects/sdl2-amigaos4/

Next best option is to use community forums such as Amigans.net or
AmigaWorld.net.
