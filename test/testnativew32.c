
#include "testnative.h"

#ifdef TEST_NATIVE_WIN32

static void *CreateWindowWin32(int w, int h);
static void DestroyWindowWin32(void *window);

NativeWindowFactory Win32WindowFactory = {
    "win32",
    CreateWindowWin32,
    DestroyWindowWin32
};

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

static void *
CreateWindowWin32(int w, int h)
{
    HWND hwnd;
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "SDL Test";

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd =
        CreateWindow("SDL Test", "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                     CW_USEDEFAULT, w, h, NULL, NULL, GetModuleHandle(NULL),
                     NULL);
    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);

    return hwnd;
}

static void
DestroyWindowWin32(void *window)
{
    DestroyWindow((HWND) window);
}

#endif
