#include "SDLmain_WinRT_common.h"
#include "SDL_WinRTApp.h"

extern "C" {
#include "SDL_assert.h"
#include "SDL_stdinc.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_keyboard_c.h"
#include "SDL_events.h"
#include "SDL_log.h"
}

// TODO, WinRT: Remove reference(s) to BasicTimer.h
//#include "BasicTimer.h"

// HACK, DLudwig: The C-style main() will get loaded via the app's
// WinRT-styled main(), which is part of SDLmain_for_WinRT.cpp.
// This seems wrong on some level, but does seem to work.
typedef int (*SDL_WinRT_MainFunction)(int, char **);
static SDL_WinRT_MainFunction SDL_WinRT_main = nullptr;

// HACK, DLudwig: record a reference to the global, Windows RT 'app'/view.
// SDL/WinRT will use this throughout its code.
//
// TODO, WinRT: consider replacing SDL_WinRTGlobalApp with something
// non-global, such as something created inside
// SDL_InitSubSystem(SDL_INIT_VIDEO), or something inside
// SDL_CreateWindow().
SDL_WinRTApp ^ SDL_WinRTGlobalApp = nullptr;


using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Devices::Input;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace concurrency;

SDL_WinRTApp::SDL_WinRTApp() :
	m_windowClosed(false),
	m_windowVisible(true),
    m_sdlWindowData(NULL),
    m_useRelativeMouseMode(false)
{
}

void SDL_WinRTApp::Initialize(CoreApplicationView^ applicationView)
{
	applicationView->Activated +=
        ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &SDL_WinRTApp::OnActivated);

	CoreApplication::Suspending +=
        ref new EventHandler<SuspendingEventArgs^>(this, &SDL_WinRTApp::OnSuspending);

	CoreApplication::Resuming +=
        ref new EventHandler<Platform::Object^>(this, &SDL_WinRTApp::OnResuming);

	m_renderer = ref new SDL_winrtrenderer();
}

void SDL_WinRTApp::SetWindow(CoreWindow^ window)
{
	window->SizeChanged += 
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &SDL_WinRTApp::OnWindowSizeChanged);

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &SDL_WinRTApp::OnVisibilityChanged);

	window->Closed += 
        ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &SDL_WinRTApp::OnWindowClosed);

	window->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);

	window->PointerPressed +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerPressed);

    window->PointerReleased +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerReleased);

	window->PointerMoved +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerMoved);

    // Retrieves relative-only mouse movements:
    Windows::Devices::Input::MouseDevice::GetForCurrentView()->MouseMoved +=
        ref new TypedEventHandler<MouseDevice^, MouseEventArgs^>(this, &SDL_WinRTApp::OnMouseMoved);

    window->KeyDown +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &SDL_WinRTApp::OnKeyDown);

	window->KeyUp +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &SDL_WinRTApp::OnKeyUp);


	m_renderer->Initialize(CoreWindow::GetForCurrentThread());
}

void SDL_WinRTApp::Load(Platform::String^ entryPoint)
{
}

void SDL_WinRTApp::Run()
{
    if (SDL_WinRT_main)
    {
        // TODO, WinRT: pass the C-style main() a reasonably realistic
        // representation of command line arguments.
        int argc = 0;
        char **argv = NULL;
        SDL_WinRT_main(argc, argv);
    }
}

void SDL_WinRTApp::PumpEvents()
{
	if (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

void SDL_WinRTApp::UpdateWindowFramebuffer(SDL_Surface * surface, SDL_Rect * rects, int numrects)
{
    if (!m_windowClosed && m_windowVisible)
	{
		m_renderer->Render(surface, rects, numrects);
		m_renderer->Present(); // This call is synchronized to the display frame rate.
	}
}

void SDL_WinRTApp::Uninitialize()
{
}

void SDL_WinRTApp::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	m_renderer->UpdateForWindowSizeChange();
}

void SDL_WinRTApp::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void SDL_WinRTApp::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

void SDL_WinRTApp::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
    if (m_sdlWindowData)
    {
    	SDL_SendMouseButton(m_sdlWindowData->sdlWindow, SDL_PRESSED, SDL_BUTTON_LEFT);
    }
}

void SDL_WinRTApp::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
    if (m_sdlWindowData)
    {
    	SDL_SendMouseButton(m_sdlWindowData->sdlWindow, SDL_RELEASED, SDL_BUTTON_LEFT);
    }
}

void SDL_WinRTApp::OnMouseMoved(MouseDevice^ mouseDevice, MouseEventArgs^ args)
{
    if (m_sdlWindowData && m_useRelativeMouseMode) {
        // DLudwig, 2012-12-28: On some systems, namely Visual Studio's Windows
        // Simulator, as well as Windows 8 in a Parallels 8 VM, MouseEventArgs'
        // MouseDelta field often reports very large values.  More information
        // on this can be found at the following pages on MSDN:
        //  - http://social.msdn.microsoft.com/Forums/en-US/winappswithnativecode/thread/a3c789fa-f1c5-49c4-9c0a-7db88d0f90f8
        //  - https://connect.microsoft.com/VisualStudio/Feedback/details/756515
        //
        // The values do not appear to be as large when running on some systems,
        // most notably a Surface RT.  Furthermore, the values returned by
        // CoreWindow's PointerMoved event, and sent to this class' OnPointerMoved
        // method, do not ever appear to be large, even when MouseEventArgs'
        // MouseDelta is reporting to the contrary.
        //
        // On systems with the large-values behavior, it appears that the values
        // get reported as if the screen's size is 65536 units in both the X and Y
        // dimensions.  This can be viewed by using Windows' now-private, "Raw Input"
        // APIs.  (GetRawInputData, RegisterRawInputDevices, WM_INPUT, etc.)
        //
        // MSDN's documentation on MouseEventArgs' MouseDelta field (at
        // http://msdn.microsoft.com/en-us/library/windows/apps/windows.devices.input.mouseeventargs.mousedelta ),
        // does not seem to indicate (to me) that its values should be so large.  It
        // says that its values should be a "change in screen location".  I could
        // be misinterpreting this, however a post on MSDN from a Microsoft engineer (see: 
        // http://social.msdn.microsoft.com/Forums/en-US/winappswithnativecode/thread/09a9868e-95bb-4858-ba1a-cb4d2c298d62 ),
        // indicates that these values are in DIPs, which is the same unit used
        // by CoreWindow's PointerMoved events (via the Position field in its CurrentPoint
        // property.  See http://msdn.microsoft.com/en-us/library/windows/apps/windows.ui.input.pointerpoint.position.aspx
        // for details.)
        //
        // To note, PointerMoved events are sent a 'RawPosition' value (via the
        // CurrentPoint property in MouseEventArgs), however these do not seem
        // to exhibit the same large-value behavior.
        //
        // The values passed via PointerMoved events can't always be used for relative
        // mouse motion, unfortunately.  Its values are bound to the cursor's position,
        // which stops when it hits one of the screen's edges.  This can be a problem in
        // first person shooters, whereby it is normal for mouse motion to travel far
        // along any one axis for a period of time.  MouseMoved events do not have the
        // screen-bounding limitation, and can be used regardless of where the system's
        // cursor is.
        //
        // One possible workaround would be to programmatically set the cursor's
        // position to the screen's center (when SDL's relative mouse mode is enabled),
        // however Windows RT does not yet seem to have the ability to set the cursor's
        // position via a public API.  Win32 did this via an API call, SetCursorPos,
        // however WinRT makes this function be private.  Apps that use it won't get
        // approved for distribution in the Windows Store.  I've yet to be able to find
        // a suitable, store-friendly counterpart for WinRT.
        //
        // There may be some room for a workaround whereby OnPointerMoved's values
        // are compared to the values from OnMouseMoved in order to detect
        // when this bug is active.  A suitable transformation could then be made to
        // OnMouseMoved's values.  For now, however, the system-reported values are sent
        // without transformation.
        //
        SDL_SendMouseMotion(m_sdlWindowData->sdlWindow, 1, args->MouseDelta.X, args->MouseDelta.Y);
    }
}

// Applies necessary geometric transformations to raw cursor positions:
Point SDL_WinRTApp::TransformCursor(Point rawPosition)
{
    if ( ! m_sdlWindowData || ! m_sdlWindowData->sdlWindow ) {
        return rawPosition;
    }
    CoreWindow ^ nativeWindow = CoreWindow::GetForCurrentThread();
    Point outputPosition;
    outputPosition.X = rawPosition.X * (((float32)m_sdlWindowData->sdlWindow->w) / nativeWindow->Bounds.Width);
    outputPosition.Y = rawPosition.Y * (((float32)m_sdlWindowData->sdlWindow->h) / nativeWindow->Bounds.Height);
    return outputPosition;
}

void SDL_WinRTApp::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
    if (m_sdlWindowData && ! m_useRelativeMouseMode)
    {
        Point transformedPoint = TransformCursor(args->CurrentPoint->Position);
        SDL_SendMouseMotion(m_sdlWindowData->sdlWindow, 0, (int)transformedPoint.X, (int)transformedPoint.Y);
    }
}

static SDL_Scancode WinRT_Keycodes[] = {
    SDL_SCANCODE_UNKNOWN, // VirtualKey.None -- 0
    SDL_SCANCODE_UNKNOWN, // VirtualKey.LeftButton -- 1
    SDL_SCANCODE_UNKNOWN, // VirtualKey.RightButton -- 2
    SDL_SCANCODE_CANCEL, // VirtualKey.Cancel -- 3
    SDL_SCANCODE_UNKNOWN, // VirtualKey.MiddleButton -- 4
    SDL_SCANCODE_UNKNOWN, // VirtualKey.XButton1 -- 5
    SDL_SCANCODE_UNKNOWN, // VirtualKey.XButton2 -- 6
    SDL_SCANCODE_UNKNOWN, // -- 7
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Back -- 8  (maybe SDL_SCANCODE_AC_BACK ?)
    SDL_SCANCODE_TAB, // VirtualKey.Tab -- 9
    SDL_SCANCODE_UNKNOWN, // -- 10
    SDL_SCANCODE_UNKNOWN, // -- 11
    SDL_SCANCODE_CLEAR, // VirtualKey.Clear -- 12
    SDL_SCANCODE_RETURN, // VirtualKey.Enter -- 13
    SDL_SCANCODE_UNKNOWN, // -- 14
    SDL_SCANCODE_UNKNOWN, // -- 15
    SDL_SCANCODE_LSHIFT, // VirtualKey.Shift -- 16
    SDL_SCANCODE_LCTRL, // VirtualKey.Control -- 17
    SDL_SCANCODE_MENU, // VirtualKey.Menu -- 18
    SDL_SCANCODE_PAUSE, // VirtualKey.Pause -- 19
    SDL_SCANCODE_CAPSLOCK, // VirtualKey.CapitalLock -- 20
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Kana or VirtualKey.Hangul -- 21
    SDL_SCANCODE_UNKNOWN, // -- 22
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Junja -- 23
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Final -- 24
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Hanja or VirtualKey.Kanji -- 25
    SDL_SCANCODE_UNKNOWN, // -- 26
    SDL_SCANCODE_ESCAPE, // VirtualKey.Escape -- 27
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Convert -- 28
    SDL_SCANCODE_UNKNOWN, // VirtualKey.NonConvert -- 29
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Accept -- 30
    SDL_SCANCODE_UNKNOWN, // VirtualKey.ModeChange -- 31  (maybe SDL_SCANCODE_MODE ?)
    SDL_SCANCODE_SPACE, // VirtualKey.Space -- 32
    SDL_SCANCODE_PAGEUP, // VirtualKey.PageUp -- 33
    SDL_SCANCODE_PAGEDOWN, // VirtualKey.PageDown -- 34
    SDL_SCANCODE_END, // VirtualKey.End -- 35
    SDL_SCANCODE_HOME, // VirtualKey.Home -- 36
    SDL_SCANCODE_LEFT, // VirtualKey.Left -- 37
    SDL_SCANCODE_UP, // VirtualKey.Up -- 38
    SDL_SCANCODE_RIGHT, // VirtualKey.Right -- 39
    SDL_SCANCODE_DOWN, // VirtualKey.Down -- 40
    SDL_SCANCODE_SELECT, // VirtualKey.Select -- 41
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Print -- 42  (maybe SDL_SCANCODE_PRINTSCREEN ?)
    SDL_SCANCODE_EXECUTE, // VirtualKey.Execute -- 43
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Snapshot -- 44
    SDL_SCANCODE_INSERT, // VirtualKey.Insert -- 45
    SDL_SCANCODE_DELETE, // VirtualKey.Delete -- 46
    SDL_SCANCODE_HELP, // VirtualKey.Help -- 47
    SDL_SCANCODE_0, // VirtualKey.Number0 -- 48
    SDL_SCANCODE_1, // VirtualKey.Number1 -- 49
    SDL_SCANCODE_2, // VirtualKey.Number2 -- 50
    SDL_SCANCODE_3, // VirtualKey.Number3 -- 51
    SDL_SCANCODE_4, // VirtualKey.Number4 -- 52
    SDL_SCANCODE_5, // VirtualKey.Number5 -- 53
    SDL_SCANCODE_6, // VirtualKey.Number6 -- 54
    SDL_SCANCODE_7, // VirtualKey.Number7 -- 55
    SDL_SCANCODE_8, // VirtualKey.Number8 -- 56
    SDL_SCANCODE_9, // VirtualKey.Number9 -- 57
    SDL_SCANCODE_UNKNOWN, // -- 58
    SDL_SCANCODE_UNKNOWN, // -- 59
    SDL_SCANCODE_UNKNOWN, // -- 60
    SDL_SCANCODE_UNKNOWN, // -- 61
    SDL_SCANCODE_UNKNOWN, // -- 62
    SDL_SCANCODE_UNKNOWN, // -- 63
    SDL_SCANCODE_UNKNOWN, // -- 64
    SDL_SCANCODE_A, // VirtualKey.A -- 65
    SDL_SCANCODE_B, // VirtualKey.B -- 66
    SDL_SCANCODE_C, // VirtualKey.C -- 67
    SDL_SCANCODE_D, // VirtualKey.D -- 68
    SDL_SCANCODE_E, // VirtualKey.E -- 69
    SDL_SCANCODE_F, // VirtualKey.F -- 70
    SDL_SCANCODE_G, // VirtualKey.G -- 71
    SDL_SCANCODE_H, // VirtualKey.H -- 72
    SDL_SCANCODE_I, // VirtualKey.I -- 73
    SDL_SCANCODE_J, // VirtualKey.J -- 74
    SDL_SCANCODE_K, // VirtualKey.K -- 75
    SDL_SCANCODE_L, // VirtualKey.L -- 76
    SDL_SCANCODE_M, // VirtualKey.M -- 77
    SDL_SCANCODE_N, // VirtualKey.N -- 78
    SDL_SCANCODE_O, // VirtualKey.O -- 79
    SDL_SCANCODE_P, // VirtualKey.P -- 80
    SDL_SCANCODE_Q, // VirtualKey.Q -- 81
    SDL_SCANCODE_R, // VirtualKey.R -- 82
    SDL_SCANCODE_S, // VirtualKey.S -- 83
    SDL_SCANCODE_T, // VirtualKey.T -- 84
    SDL_SCANCODE_U, // VirtualKey.U -- 85
    SDL_SCANCODE_V, // VirtualKey.V -- 86
    SDL_SCANCODE_W, // VirtualKey.W -- 87
    SDL_SCANCODE_X, // VirtualKey.X -- 88
    SDL_SCANCODE_Y, // VirtualKey.Y -- 89
    SDL_SCANCODE_Z, // VirtualKey.Z -- 90
    SDL_SCANCODE_UNKNOWN, // VirtualKey.LeftWindows -- 91  (maybe SDL_SCANCODE_APPLICATION or SDL_SCANCODE_LGUI ?)
    SDL_SCANCODE_UNKNOWN, // VirtualKey.RightWindows -- 92  (maybe SDL_SCANCODE_APPLICATION or SDL_SCANCODE_RGUI ?)
    SDL_SCANCODE_APPLICATION, // VirtualKey.Application -- 93
    SDL_SCANCODE_UNKNOWN, // -- 94
    SDL_SCANCODE_SLEEP, // VirtualKey.Sleep -- 95
    SDL_SCANCODE_KP_0, // VirtualKey.NumberPad0 -- 96
    SDL_SCANCODE_KP_1, // VirtualKey.NumberPad1 -- 97
    SDL_SCANCODE_KP_2, // VirtualKey.NumberPad2 -- 98
    SDL_SCANCODE_KP_3, // VirtualKey.NumberPad3 -- 99
    SDL_SCANCODE_KP_4, // VirtualKey.NumberPad4 -- 100
    SDL_SCANCODE_KP_5, // VirtualKey.NumberPad5 -- 101
    SDL_SCANCODE_KP_6, // VirtualKey.NumberPad6 -- 102
    SDL_SCANCODE_KP_7, // VirtualKey.NumberPad7 -- 103
    SDL_SCANCODE_KP_8, // VirtualKey.NumberPad8 -- 104
    SDL_SCANCODE_KP_9, // VirtualKey.NumberPad9 -- 105
    SDL_SCANCODE_KP_MULTIPLY, // VirtualKey.Multiply -- 106
    SDL_SCANCODE_KP_PLUS, // VirtualKey.Add -- 107
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Separator -- 108
    SDL_SCANCODE_KP_MINUS, // VirtualKey.Subtract -- 109
    SDL_SCANCODE_UNKNOWN, // VirtualKey.Decimal -- 110  (maybe SDL_SCANCODE_DECIMALSEPARATOR, SDL_SCANCODE_KP_DECIMAL, or SDL_SCANCODE_KP_PERIOD ?)
    SDL_SCANCODE_KP_DIVIDE, // VirtualKey.Divide -- 111
    SDL_SCANCODE_F1, // VirtualKey.F1 -- 112
    SDL_SCANCODE_F2, // VirtualKey.F2 -- 113
    SDL_SCANCODE_F3, // VirtualKey.F3 -- 114
    SDL_SCANCODE_F4, // VirtualKey.F4 -- 115
    SDL_SCANCODE_F5, // VirtualKey.F5 -- 116
    SDL_SCANCODE_F6, // VirtualKey.F6 -- 117
    SDL_SCANCODE_F7, // VirtualKey.F7 -- 118
    SDL_SCANCODE_F8, // VirtualKey.F8 -- 119
    SDL_SCANCODE_F9, // VirtualKey.F9 -- 120
    SDL_SCANCODE_F10, // VirtualKey.F10 -- 121
    SDL_SCANCODE_F11, // VirtualKey.F11 -- 122
    SDL_SCANCODE_F12, // VirtualKey.F12 -- 123
    SDL_SCANCODE_F13, // VirtualKey.F13 -- 124
    SDL_SCANCODE_F14, // VirtualKey.F14 -- 125
    SDL_SCANCODE_F15, // VirtualKey.F15 -- 126
    SDL_SCANCODE_F16, // VirtualKey.F16 -- 127
    SDL_SCANCODE_F17, // VirtualKey.F17 -- 128
    SDL_SCANCODE_F18, // VirtualKey.F18 -- 129
    SDL_SCANCODE_F19, // VirtualKey.F19 -- 130
    SDL_SCANCODE_F20, // VirtualKey.F20 -- 131
    SDL_SCANCODE_F21, // VirtualKey.F21 -- 132
    SDL_SCANCODE_F22, // VirtualKey.F22 -- 133
    SDL_SCANCODE_F23, // VirtualKey.F23 -- 134
    SDL_SCANCODE_F24, // VirtualKey.F24 -- 135
    SDL_SCANCODE_UNKNOWN, // -- 136
    SDL_SCANCODE_UNKNOWN, // -- 137
    SDL_SCANCODE_UNKNOWN, // -- 138
    SDL_SCANCODE_UNKNOWN, // -- 139
    SDL_SCANCODE_UNKNOWN, // -- 140
    SDL_SCANCODE_UNKNOWN, // -- 141
    SDL_SCANCODE_UNKNOWN, // -- 142
    SDL_SCANCODE_UNKNOWN, // -- 143
    SDL_SCANCODE_NUMLOCKCLEAR, // VirtualKey.NumberKeyLock -- 144
    SDL_SCANCODE_SCROLLLOCK, // VirtualKey.Scroll -- 145
    SDL_SCANCODE_UNKNOWN, // -- 146
    SDL_SCANCODE_UNKNOWN, // -- 147
    SDL_SCANCODE_UNKNOWN, // -- 148
    SDL_SCANCODE_UNKNOWN, // -- 149
    SDL_SCANCODE_UNKNOWN, // -- 150
    SDL_SCANCODE_UNKNOWN, // -- 151
    SDL_SCANCODE_UNKNOWN, // -- 152
    SDL_SCANCODE_UNKNOWN, // -- 153
    SDL_SCANCODE_UNKNOWN, // -- 154
    SDL_SCANCODE_UNKNOWN, // -- 155
    SDL_SCANCODE_UNKNOWN, // -- 156
    SDL_SCANCODE_UNKNOWN, // -- 157
    SDL_SCANCODE_UNKNOWN, // -- 158
    SDL_SCANCODE_UNKNOWN, // -- 159
    SDL_SCANCODE_LSHIFT, // VirtualKey.LeftShift -- 160
    SDL_SCANCODE_RSHIFT, // VirtualKey.RightShift -- 161
    SDL_SCANCODE_LCTRL, // VirtualKey.LeftControl -- 162
    SDL_SCANCODE_RCTRL, // VirtualKey.RightControl -- 163
    SDL_SCANCODE_MENU, // VirtualKey.LeftMenu -- 164
    SDL_SCANCODE_MENU, // VirtualKey.RightMenu -- 165
};

static SDL_Scancode
TranslateKeycode(int keycode)
{
    SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
    if (keycode < SDL_arraysize(WinRT_Keycodes)) {
        scancode = WinRT_Keycodes[keycode];
    }
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        SDL_Log("WinRT TranslateKeycode, unknown keycode=%d\n", (int)keycode);
    }
    return scancode;
}

void SDL_WinRTApp::OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
#if 0
    SDL_Log("key down, handled=%s, ext?=%s, released?=%s, menu key down?=%s, repeat count=%d, scan code=%d, was down?=%s, vkey=%d\n",
        (args->Handled ? "1" : "0"),
        (args->KeyStatus.IsExtendedKey ? "1" : "0"),
        (args->KeyStatus.IsKeyReleased ? "1" : "0"),
        (args->KeyStatus.IsMenuKeyDown ? "1" : "0"),
        args->KeyStatus.RepeatCount,
        args->KeyStatus.ScanCode,
        (args->KeyStatus.WasKeyDown ? "1" : "0"),
        args->VirtualKey);
    //args->Handled = true;
    //VirtualKey vkey = args->VirtualKey;
#endif
    SDL_SendKeyboardKey(SDL_PRESSED, TranslateKeycode((int)args->VirtualKey));
}

void SDL_WinRTApp::OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
#if 0
    SDL_Log("key up, handled=%s, ext?=%s, released?=%s, menu key down?=%s, repeat count=%d, scan code=%d, was down?=%s, vkey=%d\n",
        (args->Handled ? "1" : "0"),
        (args->KeyStatus.IsExtendedKey ? "1" : "0"),
        (args->KeyStatus.IsKeyReleased ? "1" : "0"),
        (args->KeyStatus.IsMenuKeyDown ? "1" : "0"),
        args->KeyStatus.RepeatCount,
        args->KeyStatus.ScanCode,
        (args->KeyStatus.WasKeyDown ? "1" : "0"),
        args->VirtualKey);
    //args->Handled = true;
#endif
    SDL_SendKeyboardKey(SDL_RELEASED, TranslateKeycode((int)args->VirtualKey));
}

void SDL_WinRTApp::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	CoreWindow::GetForCurrentThread()->Activate();
}

void SDL_WinRTApp::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	create_task([this, deferral]()
	{
		// Insert your code here.

		deferral->Complete();
	});
}
 
void SDL_WinRTApp::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.
}

SDL_DisplayMode SDL_WinRTApp::GetMainDisplayMode()
{
    SDL_DisplayMode mode;
    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB888;
    mode.w = (int) CoreWindow::GetForCurrentThread()->Bounds.Width;
    mode.h = (int) CoreWindow::GetForCurrentThread()->Bounds.Height;
    mode.refresh_rate = 0;  // TODO, WinRT: see if refresh rate data is available, or relevant (for WinRT apps)
    mode.driverdata = NULL;
    return mode;
}

const SDL_WindowData * SDL_WinRTApp::GetSDLWindowData() const
{
    return m_sdlWindowData;
}

bool SDL_WinRTApp::HasSDLWindowData() const
{
    return (m_sdlWindowData != NULL);
}

void SDL_WinRTApp::SetRelativeMouseMode(bool enable)
{
    m_useRelativeMouseMode = enable;
}

void SDL_WinRTApp::SetSDLWindowData(const SDL_WindowData* windowData)
{
    m_sdlWindowData = windowData;
}

void SDL_WinRTApp::ResizeMainTexture(int w, int h)
{
    m_renderer->ResizeMainTexture(w, h);
}

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
    // TODO, WinRT: see if this function (CreateView) can ever get called
    // more than once.  For now, just prevent it from ever assigning
    // SDL_WinRTGlobalApp more than once.
    SDL_assert(!SDL_WinRTGlobalApp);
    SDL_WinRTApp ^ app = ref new SDL_WinRTApp();
    if (!SDL_WinRTGlobalApp)
    {
        SDL_WinRTGlobalApp = app;
    }
    return app;
}

__declspec(dllexport) int SDL_WinRT_RunApplication(SDL_WinRT_MainFunction mainFunction)
{
    SDL_WinRT_main = mainFunction;
    auto direct3DApplicationSource = ref new Direct3DApplicationSource();
	CoreApplication::Run(direct3DApplicationSource);
	return 0;
}
