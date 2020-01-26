/*

This is not an official SDL2 test. It's a collection of random tests
for doing ad-hoc testing related to AmigaOS 4 port.

*/

#include <stdio.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"

#if SDL_BYTEORDER != SDL_BIG_ENDIAN
#warning "wrong endian?"
#endif

static SDL_bool eventLoopInner(void)
{
    SDL_bool running = SDL_TRUE;
    SDL_Event e;

    while(SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                puts("Quit");
                running = SDL_FALSE;
                break;

            case SDL_WINDOWEVENT:
                {
                    SDL_WindowEvent * we = (SDL_WindowEvent *)&e;
                    printf("Window event %d window %d\n", we->event, we->windowID);
                }
                break;

            case SDL_SYSWMEVENT:
                printf("Sys WM event\n");
                break;

            case SDL_KEYDOWN:
                {
                    SDL_KeyboardEvent * ke = (SDL_KeyboardEvent *)&e;
                    printf("Key down scancode %d (%s), keycode %d (%s)\n",
                        ke->keysym.scancode, SDL_GetScancodeName(ke->keysym.scancode),
                        ke->keysym.sym, SDL_GetKeyName(ke->keysym.sym));
                }
                break;

            case SDL_KEYUP:
                {
                    SDL_KeyboardEvent * ke = (SDL_KeyboardEvent *)&e;
                    printf("Key up %d\n", ke->keysym.scancode);
                }
                break;

            case SDL_TEXTEDITING:
                puts("Text editing");
                break;

            case SDL_TEXTINPUT:
                {
                    SDL_TextEditingEvent * te = (SDL_TextEditingEvent *)&e;
                    SDL_Window * w = SDL_GetWindowFromID(te->windowID);

                    printf("Text input '%s'\n", te->text);

                    if (strcmp("q", te->text) == 0) {
                        running = SDL_FALSE;
                    } else if (strcmp("w", te->text) == 0) {
                        SDL_WarpMouseInWindow(w, 50, 50);
                    } else if (strcmp("f", te->text) == 0) {
                        static int t = 1;

                        printf("Toggle fullscreen %d\n", t);
                        SDL_SetWindowFullscreen(w, t ? SDL_WINDOW_FULLSCREEN : 0);

                        t ^= 1;
                    } else if (strcmp("t", te->text) == 0) {
                        puts("Change size...");
                        SDL_SetWindowSize(w, rand() % 1000, rand() % 1000);
                    } else if (strcmp("c", te->text) == 0) {
                        static int c = 1;
                        SDL_ShowCursor(c);
                        c ^= 1;
                    }
                }
                break;

            case SDL_MOUSEMOTION:
                {
                    SDL_MouseMotionEvent * me = (SDL_MouseMotionEvent *)&e;
                    printf("Mouse motion x=%d, y=%d, xrel=%d, yrel=%d\n", me->x, me->y, me->xrel, me->yrel);
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                {
                    SDL_MouseButtonEvent * me = (SDL_MouseButtonEvent *)&me;
                    printf("Mouse button down %d, state %d\n", (int)me->button, (int)me->state);
                }
                break;

            case SDL_MOUSEBUTTONUP:
                {
                    SDL_MouseButtonEvent * me = (SDL_MouseButtonEvent *)&me;
                    printf("Mouse button up %d, state %d\n", (int)me->button, (int)me->state);
                }
                break;

            case SDL_MOUSEWHEEL:
                {
                    SDL_MouseWheelEvent * me = (SDL_MouseWheelEvent *)&me;
                    printf("Mouse wheel x=%d, y=%d\n", me->x, me->y);
                }
                break;

            default:
                printf("Unhandled event %d\n", e.type);
                break;
        }
    }

    return running;
}

static void eventLoop()
{
    while (eventLoopInner()) {
        SDL_Delay(1);
    }
}

static void testPath(void)
{
    char * bp = SDL_GetBasePath();
    printf("'%s'\n", bp);
    SDL_free(bp);

   char* pp = SDL_GetPrefPath("foo", "bar");
   printf("'%s'\n", pp);
   SDL_free(pp);
}

static void testWindow()
{
    SDL_Window * w = SDL_CreateWindow("blah", 100, 100, 100, 100, SDL_WINDOW_RESIZABLE);

    if (w) {
        SDL_SetWindowMinimumSize(w, 50, 50);
        SDL_SetWindowMaximumSize(w, 200, 200);

        SDL_MinimizeWindow(w);
        SDL_Delay(1000);

        SDL_RestoreWindow(w);
        SDL_Delay(1000);

        SDL_MinimizeWindow(w);
        SDL_Delay(1000);

        SDL_MaximizeWindow(w);

        eventLoop();

        SDL_DestroyWindow(w);
    }
}

static void testManyWindows()
{
    SDL_Window * w = SDL_CreateWindow("blah", 100, 100, 100, 100, 0);
    SDL_Window * w2 = SDL_CreateWindow("blah2", 200, 100, 100, 100, 0/*SDL_WINDOW_FULLSCREEN*/);

    if (w && w2) {
        SDL_SetWindowGrab(w, SDL_TRUE);

        eventLoop();

        SDL_DestroyWindow(w);
        SDL_DestroyWindow(w2);
    }
}

static void testRelativeMouse()
{
    SDL_Window * w = SDL_CreateWindow("relative", 100, 100, 100, 100, 0);

    if (w) {
        //SDL_SetRelativeMouseMode(SDL_FALSE);
        //SDL_SetRelativeMouseMode(SDL_TRUE);

        eventLoop();

        SDL_DestroyWindow(w);
    }
}

static void testFullscreen()
{
    SDL_Window * w = SDL_CreateWindow("Fullscreen",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_FULLSCREEN);

    if (w) {
        //SDL_SetWindowSize(w, 1280, 960);
        //SDL_SetWindowSize(w, 640, 480);
        //SDL_SetWindowSize(w, 800, 600);

        //SDL_SetWindowFullscreen(w, SDL_WINDOW_FULLSCREEN);
        //SDL_SetWindowFullscreen(w, SDL_WINDOW_FULLSCREEN);
        //SDL_SetWindowFullscreen(w, 0);
        //SDL_SetWindowFullscreen(w, 0);

        SDL_Delay(3000);

#if 0
        SDL_DisplayMode dm;
        dm.format = SDL_PIXELFORMAT_ARGB8888;
        dm.w = 1280;
        dm.h = 960;
        dm.refresh_rate = 0;
        dm.driverdata = NULL;

        SDL_SetWindowDisplayMode(w, &dm);
        SDL_SetWindowFullscreen(w, SDL_WINDOW_FULLSCREEN);
#endif
        eventLoop();

        SDL_DestroyWindow(w);
    }
}

static void testFullscreenDesktop()
{
    SDL_Window * w = SDL_CreateWindow("Desktop mode",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640, 400,
        SDL_WINDOW_FULLSCREEN_DESKTOP);

    if (w) {
        eventLoop();

        SDL_DestroyWindow(w);
    }
}

// TODO: should load GL functions
// NOTE: will not work on OGLES2 (try testgles2.c instead of)
static void drawUsingFixedFunctionPipeline(SDL_Window *w)
{
    if (w) {
        SDL_GLContext c = SDL_GL_CreateContext(w);

        if (c) {

            //SDL_GL_SetSwapInterval(1);

            Uint32 start = SDL_GetTicks();
            Uint32 frames = 0;

            while (eventLoopInner()) {
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();

                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();

                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                glBegin(GL_TRIANGLES);

                glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
                glVertex3f(-0.5f, -0.5f, 0.0f);

                glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
                glVertex3f(0.5f, -0.5f, 0.0f);

                glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
                glVertex3f(0.0f, 0.5f, 0.0f);

                glEnd();

                SDL_GL_SwapWindow(w);
                frames++;
            }

            Uint32 end = SDL_GetTicks();
            printf("%u frames in %u ms - %f\n", frames, end - start, 1000.f * frames / (end - start));


            SDL_GL_DeleteContext(c);
        }

        SDL_DestroyWindow(w);
    }

}

static SDL_Window* createWindow(const char *name)
{
    return SDL_CreateWindow(name,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        400,
        300,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
}

static void testDeleteContext()
{
    SDL_Window* w = createWindow("Context deletion");
    if (w) {
        SDL_GLContext c = SDL_GL_CreateContext(w);

        if (c) {
            SDL_GL_DeleteContext(c);

            printf("Context after deletion: %p\n", SDL_GL_GetCurrentContext());

            SDL_SetWindowSize(w, 101, 101);

            c = SDL_GL_CreateContext(w);

            if (c) {
                SDL_GL_DeleteContext(c);
            }
        }
        SDL_DestroyWindow(w);
    }
}

static void testOpenGL()
{
    SDL_Window * w = createWindow("Centered & Resizable OpenGL window");

    drawUsingFixedFunctionPipeline(w);
}

static void testOpenGLES2()
{
    int mask, major, minor;

    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &mask);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

    printf("Current GL mask %d, major version %d, minor version %d\n", mask, major, minor);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_Window * w = createWindow("Centered & Resizable OGLES2 window");

    if (w)
    {
        SDL_GLContext c = SDL_GL_CreateContext(w);

        if (c) {
            puts("OGLES2 context created, now exiting");
            SDL_GL_DeleteContext(c);
        } else {
            puts("Failed to create OGLES2 context");
        }

        SDL_DestroyWindow(w);
    } else {
        puts("Failed to create OGLES2 window");
    }

    // Restore flags
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, mask);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
}


static void testOpenGLSwitching()
{
    SDL_Window* w = createWindow("Centered & Resizable OpenGL window");

    if (w) {
        SDL_DestroyWindow(w);
    }

    // Switch to OGLES2
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    w = createWindow("Centered & Resizable OGLES2 window");

    if (w) {
        SDL_DestroyWindow(w);
    } else {
        printf("%s\n", SDL_GetError());
    }

    // Switch back to MiniGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    w = createWindow("Centered & Resizable OpenGL window");

    drawUsingFixedFunctionPipeline(w);
}

static void testFullscreenOpenGL()
{
    SDL_Window * w = SDL_CreateWindow("Fullscreen",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1200,
        900,
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);

    drawUsingFixedFunctionPipeline(w);
}

static void testOpenGLVersion()
{
    int mask, major, minor;

    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &mask);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

    printf("Current GL mask %d, major version %d, minor version %d\n", mask, major, minor);
}

static void testRenderer()
{
    SDL_Window * r = SDL_CreateWindow("Red", 0, 0, 100, 100, SDL_WINDOW_RESIZABLE);
    SDL_Window * g = SDL_CreateWindow("Green", 200, 200, 100, 100, SDL_WINDOW_RESIZABLE);
    SDL_Window * b = SDL_CreateWindow("Blue", 400, 400, 100, 100, SDL_WINDOW_RESIZABLE);

    SDL_Renderer * rr = SDL_CreateRenderer(r, -1, SDL_RENDERER_SOFTWARE);
    SDL_Renderer * gr = SDL_CreateRenderer(g, -1, SDL_RENDERER_SOFTWARE);
    SDL_Renderer * br = SDL_CreateRenderer(b, -1, SDL_RENDERER_ACCELERATED);

    if (r && g && b && rr && gr && br) {
        while (eventLoopInner()) {
            SDL_SetRenderDrawColor(rr, 255, 0, 0, 255);
            SDL_RenderClear(rr);
            SDL_RenderPresent(rr);

            SDL_SetRenderDrawColor(gr, 0, 255, 0, 255);
            SDL_RenderClear(gr);
            SDL_RenderPresent(gr);

            SDL_SetRenderDrawColor(br, 0, 0, 255, 255);
            SDL_RenderClear(br);
            SDL_RenderPresent(br);
        }

        SDL_DestroyRenderer(rr);
        SDL_DestroyRenderer(gr);
        SDL_DestroyRenderer(br);

        SDL_DestroyWindow(r);
        SDL_DestroyWindow(g);
        SDL_DestroyWindow(b);
    } else {
        printf("%s\n", SDL_GetError());
    }
}

static void testDraw()
{
    SDL_Window * w = SDL_CreateWindow("Draw", 100, 100, 200, 200, SDL_WINDOW_RESIZABLE);

    SDL_Renderer * r = SDL_CreateRenderer(w, -1, /*SDL_RENDERER_SOFTWARE*/ SDL_RENDERER_ACCELERATED);

    if (w && r) {
        while (eventLoopInner()) {
            SDL_Rect rect;

            SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
            SDL_RenderClear(r);

            SDL_SetRenderDrawColor(r, 200, 200, 200, 255);

            rect.x = 10;
            rect.y = 10;
            rect.w = 100;
            rect.h = 100;

            SDL_RenderFillRect(r, &rect);

            SDL_RenderPresent(r);
        }

        SDL_DestroyRenderer(r);
        SDL_DestroyWindow(w);
    }
}

static void testRenderVsync()
{
    SDL_Window * w = SDL_CreateWindow("Draw", 100, 100, 200, 200, SDL_WINDOW_RESIZABLE);

    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    SDL_Renderer * r = SDL_CreateRenderer(w, -1, /*SDL_RENDERER_SOFTWARE*/ SDL_RENDERER_ACCELERATED);

    if (w && r) {
        while (eventLoopInner()) {
            SDL_Rect rect;

            SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
            SDL_RenderClear(r);

            SDL_SetRenderDrawColor(r, 200, 200, 200, 255);

            rect.x = 10;
            rect.y = 10;
            rect.w = 100;
            rect.h = 100;

            SDL_RenderFillRect(r, &rect);

            SDL_RenderPresent(r);
        }

        SDL_DestroyRenderer(r);
        SDL_DestroyWindow(w);
    }
}

static void testBmp()
{
    SDL_Surface *s = SDL_LoadBMP("sample.bmp");

    if (s) {
        SDL_FreeSurface(s);
    } else {
        printf("%s\n", SDL_GetError());
    }
}

static void testMessageBox()
{
    int button = 0;
    int result;

    const SDL_MessageBoxButtonData buttons[] = {
        {0, 1, "button 1"}, // flags, id, text
        {0, 2, "button 2"} // flags, id, text
    };

    const SDL_MessageBoxData mb = {
        0, // flags
        0, // window
        "Title",
        "Message",
        SDL_arraysize(buttons), // numbuttons
        buttons,
        NULL
    };

    result = SDL_ShowMessageBox(&mb, &button);

    printf("MB returned %d, button %d\n", result, button);
}

static void testAltivec()
{
    printf("AltiVec: %d\n", SDL_HasAltiVec());
}

static void testPC()
{
    Uint64 pc1 = SDL_GetPerformanceCounter();

    SDL_Delay(1000);

    Uint64 pc2 = SDL_GetPerformanceCounter();

    Uint64 f = SDL_GetPerformanceFrequency();

    double result = (pc2 - pc1) / (double)f;

    printf("%f s\n", result);
}

static void testSystemCursors()
{
    SDL_Window * w = SDL_CreateWindow("blah", 100, 100, 100, 100, 0);

    if (w) {
        int c = 0;

        while (eventLoopInner()) {

            char buf[32];
            snprintf(buf, sizeof(buf), "Cursor %d", c);

            SDL_SetWindowTitle(w, buf);

            SDL_SetCursor( SDL_CreateSystemCursor(c) );
            SDL_ShowCursor(1);

            SDL_Delay(1000);

            if (++c == SDL_NUM_SYSTEM_CURSORS) {
                c = 0;
                SDL_ShowCursor(0);
                SDL_SetWindowTitle(w, "Hidden");
                SDL_Delay(1000);
            }
        }

        SDL_DestroyWindow(w);
    }
}

static void testCustomCursor()
{
    int w = 64;
    int h = 64;

    SDL_Surface *surface = SDL_CreateRGBSurface(0, w, h, 32,
                                   0x00FF0000,
                                   0x0000FF00,
                                   0x000000FF,
                                   0xFF000000);

    if (surface) {

        int x, y;

        //printf("pitch %d\n", surface->pitch);

        for (y = 0; y < h; y++) {

            Uint32 *p = (Uint32 *)((Uint8 *)surface->pixels + y * surface->pitch);

            Uint32 color = 0xFF000000 | rand();

            for (x = 0; x < w; x++) {
                p[x] = color;
            }
        }

        //SDL_FillRect(surface, NULL, 0xFFFFFFFF);

        SDL_Window * w = SDL_CreateWindow("Custom cursor", 100, 100, 100, 100, 0);

        if (w) {

            SDL_SetCursor( SDL_CreateColorCursor(surface, 0, 0));

            while (eventLoopInner()) {

                SDL_Delay(1000);
            }

            SDL_DestroyWindow(w);
        }

        SDL_FreeSurface(surface);
    }
}


static void testHiddenCursor()
{
    //SDL_ShowCursor(0);

    SDL_Window * w = SDL_CreateWindow("Hidden cursor", 0, 0, 640, 480, SDL_WINDOW_FULLSCREEN);

    if (w) {
        while (eventLoopInner()) {
            SDL_Delay(1000);
        }

        SDL_DestroyWindow(w);
    }
}


static void testClipboard()
{
    SDL_Window * w = SDL_CreateWindow("Clipboard", 100, 100, 100, 100, 0);

    if (w) {

        while (eventLoopInner()) {

            if (SDL_HasClipboardText()) {
                printf("Text '%s' found\n", SDL_GetClipboardText());
            }

            SDL_Delay(1000);
        }

        SDL_DestroyWindow(w);
     }

    puts("Leaving message to clipboard");

    SDL_SetClipboardText("Amiga rules!");
}

static void testHint()
{
    char *result1 = (char *)SDL_GetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS);
    char *result2 = (char *)SDL_GetHint("SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS");

    printf("%s, %s\n", result1, result2);
}

static void testGlobalMouseState()
{
    SDL_Window * w = SDL_CreateWindow("Global Mouse State", 100, 100, 100, 100, 0);

    if (w) {

        while (eventLoopInner()) {

            int x = 0;
            int y = 0;

            printf("State 0x%x (%d, %d)\n", SDL_GetGlobalMouseState(&x, &y), x, y);
            SDL_Delay(1000);
        }

        SDL_DestroyWindow(w);
     }
}

static void testGlobalMouseWarp()
{
    SDL_Window * w = SDL_CreateWindow("Global Mouse Warp", 100, 100, 100, 100, 0);

    if (w) {

        while (eventLoopInner()) {

            int x = rand() % 800;
            int y = rand() % 600;

            printf("Warping to %d, %d\n", x, y);
            SDL_WarpMouseGlobal(x, y);

            SDL_Delay(1000);
        }

        SDL_DestroyWindow(w);
     }
}

static void testOpaqueWindow()
{
    SDL_Window * w = SDL_CreateWindow("Opaque window", 100, 100, 100, 100, 0);

    if (w) {

        while (eventLoopInner()) {
            float opacity = 0.0f;

            while (opacity <= 1.1f) {
                printf("Opacity %f\n", opacity);
                SDL_SetWindowOpacity(w, opacity);
                opacity += 0.1f;
                SDL_Delay(100);
            }
        }

        SDL_DestroyWindow(w);
     }
}

static void testWindowBordersSize()
{
    SDL_Window * w = SDL_CreateWindow("BorderSizes window", 100, 100, 100, 100, SDL_WINDOW_RESIZABLE);

    if (w) {

        int top, left, bottom, right;

        SDL_GetWindowBordersSize(w, &top, &left, &bottom, &right);

        printf("top %d, left %d, bottom %d, right %d\n", top, left, bottom, right);

        while (eventLoopInner()) {
            SDL_Delay(1000);
        }

        SDL_DestroyWindow(w);
     }
}

static void testHiddenWindow()
{
    SDL_Window * w = SDL_CreateWindow("Hidden window", 100, 100, 100, 100, SDL_WINDOW_HIDDEN);

    if (w) {
        int i = 0;

        while (eventLoopInner()) {

            SDL_Delay(1000);

            ((i++ % 2) == 0) ? SDL_ShowWindow(w) : SDL_HideWindow(w);
        }

        SDL_DestroyWindow(w);
     }
}

static void testInitEverything()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
        puts("I works?");
    } else {
        puts("Expected failure, there is no force feedback (haptic) here");
    }

    SDL_Quit();
}

int main(void)
{
    if (0) testInitEverything();

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) == 0) {
        if (1) testPath();
        if (0) testWindow();
        if (0) testManyWindows();
        if (0) testFullscreen();
        if (0) testFullscreenOpenGL();
        if (0) testDeleteContext();
        if (0) testOpenGL();
        if (0) testOpenGLES2();
        if (0) testOpenGLSwitching();
        if (0) testOpenGLVersion();
        if (0) testRenderer();
        if (0) testDraw();
        if (0) testMessageBox();
        if (0) testBmp();
        if (0) testAltivec();
        if (0) testFullscreenDesktop();
        if (0) testRenderVsync();
        if (0) testPC();
        if (0) testPC();
        if (0) testSystemCursors();
        if (0) testCustomCursor();
        if (0) testHiddenCursor();
        if (0) testClipboard();
        if (0) testHint();
        if (0) testGlobalMouseState();
        if (0) testGlobalMouseWarp();
        if (0) testOpaqueWindow();
        if (0) testWindowBordersSize();
        if (0) testHiddenWindow();
        if (0) testRelativeMouse();
    } else {
        printf("%s\n", SDL_GetError());
    }

    SDL_Quit();

    return 0;
}
