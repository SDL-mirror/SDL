
#include "testnative.h"

#ifdef TEST_NATIVE_WIN32

static void *CreateWindowWin32(int w, int h);
static void DestroyWindowWin32(void *window);

NativeWindowFactory Win32WindowFactory = {
    "win32",
    CreateWindowWin32,
    DestroyWindowWin32
};

static Display *dpy;

static void *
CreateWindowWin32(int w, int h)
{
    return NULL;
}

static void
DestroyWindowWin32(void *window)
{
}

#endif
