================================================================================
SDL2 requirements
================================================================================

AmigaOS 4.1 Final Update
MiniGL (optional from SDL2 point of view, but OpenGL context might still be
        required by the SDL2 application)


================================================================================
Building SDL2 library
================================================================================

sh configure --disable-altivec --prefix=/sdk/local/newlib/
make

After building, "make install" should work.


================================================================================
Using SDL2 in your projects
================================================================================

#include "SDL2/SDL.h"
...do magical SDL2 things...


gcc helloworld.c -use-dynld -lSDL2 -lpthread -ldl


================================================================================
About renderers
================================================================================

A renderer is a subsystem that can do 2D drawing. We have three renderers:
software, opengl and compositing.

Software renderer is always available. Pixels are plotted by the CPU so this is
usually the slowest option.

OpenGL renderer uses MiniGL (and Warp3D) for accelerated drawing. Drawing is
done in immediate mode, there is no batching. This should be fairly fast if
textures are static.

Compositing renderer uses AmigaOS 4 graphics.library for accelerated drawing.
However, blended lines and points are not accelerated since compositing doesn't
support them. Compositing renderer currently supports only 32-bit bitmaps. If
(Workbench) screenmode is 16-bit, color format conversion can slow things down.

It's possible to select the preferred renderer, like this:

SDL_SetHint(SDL_HINT_RENDER_DRIVER, name);

where name is "software", "opengl" or "compositing"

It's possible to enable VSYNC with:

SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

There is a benchmark tool called sdl2benchmark which was written to test
available renderers.


================================================================================
Tips
================================================================================

If you are already familiar with SDL1, or porting SDL1 code, it's worth checking
the migration guide at:

https://wiki.libsdl.org/MigrationGuide

================================================================================
Bugs
================================================================================

It's best to report bugs on the project page:

https://sourceforge.net/projects/sdl2-amigaos4/

Next best option is to use community forums such as Amigans.net or
AmigaWorld.net.

