#include "Window.hpp"
#include <gl/GL.h>

#pragma comment(lib, "opengl32.lib")

const wchar_t *Window::Window_Class_Name = L"GLTSF";

Window::Window() : my_Handle(0),
				   my_Device_Context(0),
				   my_GL_Context(0),
				   my_Class_Registered(false),
				   my_Listener(0)
{

}

Window::~Window()
{
	Finalize();
	Show_Cursor();
}

void Window::Initialize(const std::wstring &Title, const Video_Mode &Mode, bool Fullscreen)
{
	Finalize();

	my_Video_Mode = Mode;
	if (!my_Video_Mode.Is_Valid())
		throw std::runtime_error("Invalid video mode");

	my_Fullscreen = Fullscreen;
	Register_Class();
	Create_Window(Title, Mode, Fullscreen);
	Show();
	my_IMM.Initialize(my_Handle);
}

void Window::Finalize()
{
	my_IMM.Finalize();
	Destroy_Window();
	Unregister_Class();
}

void Window::Set_Listener(Window_Listener *Listener)
{
	my_Listener = Listener;
}

void Window::Show()
{
	if (my_Handle)
		ShowWindow(my_Handle, SW_SHOW);
}

void Window::Hide()
{
	if (my_Handle)
		ShowWindow(my_Handle, SW_HIDE);
}

void Window::Handle_Events()
{
	MSG Message = {0};
	while (PeekMessageW(&Message, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&Message);
		DispatchMessageW(&Message);
	}
}

void Window::Display()
{
	if (my_Device_Context && my_GL_Context)
		SwapBuffers(my_Device_Context);
}

void Window::Show_Cursor()
{
	ShowCursor(TRUE);
}

void Window::Hide_Cursor()
{
	ShowCursor(FALSE);
}

HWND Window::Get_Handle()
{
	return my_Handle;
}

IMM & Window::Get_IMM()
{
	return my_IMM;
}

void Window::Register_Class()
{
	WNDCLASSEXW Window_Class = {0};
	Window_Class.cbSize = sizeof(Window_Class);
	Window_Class.style = 0;
	Window_Class.lpfnWndProc = &Window::Window_Procedure;
	Window_Class.cbClsExtra = 0;
	Window_Class.cbWndExtra = 0;
	Window_Class.hInstance = GetModuleHandle(NULL);
	Window_Class.hIcon = NULL;
	Window_Class.hCursor = NULL;
	Window_Class.hbrBackground = NULL;
	Window_Class.lpszMenuName = NULL;
	Window_Class.lpszClassName = Window_Class_Name;
	Window_Class.hIconSm = NULL;
	if (0 == RegisterClassExW(&Window_Class))
		throw std::runtime_error("Failed to register window class");

	my_Class_Registered = true;
}

void Window::Unregister_Class()
{
	if (my_Class_Registered)
	{
		if (0 == UnregisterClassW(Window_Class_Name, GetModuleHandle(NULL)))
			printf("Warning: Failed to unregister window class\n");

		my_Class_Registered = false;
	}
}

void Window::Create_Window(const std::wstring &Title, const Video_Mode &Mode, bool Fullscreen)
{
	HDC Screen_DC = GetDC(NULL);
	int Left = (GetDeviceCaps(Screen_DC, HORZRES) - my_Video_Mode.Width) / 2;
	int Top = (GetDeviceCaps(Screen_DC, VERTRES) - my_Video_Mode.Height) / 2;
	int Width = my_Video_Mode.Width;
	int Height = my_Video_Mode.Height;
	ReleaseDC(NULL, Screen_DC);

	DWORD Style = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
	if (!my_Fullscreen)
	{
		RECT Rect = {0, 0, Width, Height};
		AdjustWindowRect(&Rect, Style, false);
		Width = Rect.right - Rect.left;
		Height = Rect.bottom - Rect.top;
	}
	my_Handle = CreateWindowW(Window_Class_Name, Title.c_str(), Style, Left, Top, Width, Height, NULL, NULL, GetModuleHandle(NULL), this);
	if (!my_Handle)
		throw std::runtime_error("Failed to create window");

	if (Fullscreen)
		Switch_To_Fullscreen(Mode);

	Create_Context(Mode);

	RECT Rect = {0};
	GetClientRect(my_Handle, &Rect);
	//TODO: ...
}

void Window::Destroy_Window()
{
	Destroy_Context();
	if (my_Handle)
	{
		DestroyWindow(my_Handle);
		my_Handle = 0;

		if (my_Fullscreen)
			ChangeDisplaySettings(NULL, 0);
	}
}

void Window::Create_Context(const Video_Mode &Mode)
{
	my_Device_Context = GetDC(my_Handle);
	if (!my_Device_Context)
		throw std::runtime_error("Failed to get device context");

	PIXELFORMATDESCRIPTOR Pixel_Descriptor = {0};
	Pixel_Descriptor.nSize = sizeof(Pixel_Descriptor);
	Pixel_Descriptor.nVersion = 1;
	Pixel_Descriptor.iLayerType = PFD_MAIN_PLANE;
	Pixel_Descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	Pixel_Descriptor.iPixelType = PFD_TYPE_RGBA;
	Pixel_Descriptor.cColorBits = static_cast<BYTE>(Mode.Bits_Per_Pixel);
	Pixel_Descriptor.cDepthBits = 24;
	Pixel_Descriptor.cStencilBits = 8;
	Pixel_Descriptor.cAlphaBits = Mode.Bits_Per_Pixel == 32 ? 8 : 0;

	int Best_Format = ChoosePixelFormat(my_Device_Context, &Pixel_Descriptor);
	if (0 == Best_Format)
		throw std::runtime_error("Failed to find suitable pixel format");

	PIXELFORMATDESCRIPTOR Actual_Format = {0};
	Actual_Format.nSize = sizeof(Actual_Format);
	Actual_Format.nVersion = 1;
	DescribePixelFormat(my_Device_Context, Best_Format, sizeof(Actual_Format), &Actual_Format);
	if (!SetPixelFormat(my_Device_Context, Best_Format, &Actual_Format))
		throw std::runtime_error("Failed to set device pixel format");

	my_GL_Context = wglCreateContext(my_Device_Context);
	if (!my_GL_Context)
		throw std::runtime_error("Failed to create OpenGL context");

	wglMakeCurrent(my_Device_Context, my_GL_Context);
}

void Window::Destroy_Context()
{
	if (my_GL_Context)
	{
		wglDeleteContext(my_GL_Context);
		my_GL_Context = 0;
	}
	if (my_Device_Context)
	{
		ReleaseDC(my_Handle, my_Device_Context);
		my_Device_Context = 0;
	}
}

void Window::Switch_To_Fullscreen(const Video_Mode &Mode)
{
	DEVMODE Device_Mode = {0};
	Device_Mode.dmSize = sizeof(Device_Mode);
	Device_Mode.dmPelsWidth = Mode.Width;
	Device_Mode.dmPelsHeight = Mode.Height;
	Device_Mode.dmBitsPerPel = Mode.Bits_Per_Pixel;
	Device_Mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

	if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettings(&Device_Mode, CDS_FULLSCREEN))
		throw std::runtime_error("Failed to change to fullscreen mode");

	SetWindowLong(my_Handle, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	SetWindowLong(my_Handle, GWL_EXSTYLE, WS_EX_APPWINDOW);

	SetWindowPos(my_Handle, HWND_TOP, 0, 0, Mode.Width, Mode.Height, SWP_FRAMECHANGED);
}

LRESULT CALLBACK Window::Window_Procedure(HWND Handle, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_CREATE:
		{
			LONG This = reinterpret_cast<LONG>(reinterpret_cast<CREATESTRUCT *>(lParam)->lpCreateParams);
			SetWindowLongPtr(Handle, GWLP_USERDATA, This);
			return 0;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		break;
	default:
		{
			Window* Win = reinterpret_cast<Window *>(GetWindowLongPtr(Handle, GWLP_USERDATA));
			if (Win)
				return Win->Handle_Message(Handle, Message, wParam, lParam);
		}
		break;
	}
	return DefWindowProcW(Handle, Message, wParam, lParam);
}

#define Call_Listener(x)\
	if (my_Listener) my_Listener->x

LRESULT Window::Handle_Message(HWND Handle, UINT Message, WPARAM wParam, LPARAM lParam)
{
	bool IMM_Message = false;
	LRESULT Result = my_IMM.Handle_Message(Handle, Message, wParam, lParam, IMM_Message);
	if (IMM_Message)
		return Result;

	switch (Message)
	{
	case WM_SIZE:
		Call_Listener(On_Resized(LOWORD(lParam), HIWORD(lParam)));
		break;
	case WM_CLOSE:
		Call_Listener(On_Close());
		break;
	case WM_KEYDOWN:
		Call_Listener(On_Key_Down(wParam));
		break;
	case WM_KEYUP:
		Call_Listener(On_Key_Up(wParam));
		break;
	case WM_CHAR:
		Call_Listener(On_Char(wParam));
		break;
	case WM_SETFOCUS:
			my_IMM.Focus_Gained();
		break;
	case WM_KILLFOCUS:
			my_IMM.Focus_Lost();
		break;
	case WM_LBUTTONDOWN:
		Call_Listener(On_Mouse_Button_Down(Mouse_Button_Left));
		break;
	case WM_LBUTTONUP:
		Call_Listener(On_Mouse_Button_Up(Mouse_Button_Left));
		break;
	case WM_RBUTTONDOWN:
		Call_Listener(On_Mouse_Button_Down(Mouse_Button_Right));
		break;
	case WM_RBUTTONUP:
		Call_Listener(On_Mouse_Button_Up(Mouse_Button_Right));
		break;
	default:
		return DefWindowProcW(Handle, Message, wParam, lParam);
		break;
	}
	return 0;
}
