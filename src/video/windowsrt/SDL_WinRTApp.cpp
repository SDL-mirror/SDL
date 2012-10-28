#include "SDLmain_WinRT_common.h"
#include "SDL_WinRTApp.h"
#include "BasicTimer.h"

extern "C" {
#include "SDL_assert.h"
#include "SDL_stdinc.h"
#include "../SDL_sysvideo.h"
}

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
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace concurrency;

SDL_WinRTApp::SDL_WinRTApp() :
	m_windowClosed(false),
	m_windowVisible(true)
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

	m_renderer = ref new CubeRenderer();
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

	window->PointerMoved +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerMoved);

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

	BasicTimer^ timer = ref new BasicTimer();

	while (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			timer->Update();
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			m_renderer->Update(timer->Total, timer->Delta);
			m_renderer->Render();
			m_renderer->Present(); // This call is synchronized to the display frame rate.
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
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
	// Insert your code here.
}

void SDL_WinRTApp::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
	// Insert your code here.
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
