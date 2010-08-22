#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Video_Mode.hpp"
#include "Window_Listener.hpp"
#include "TSF.hpp"

class Window
{
public:
	Window();
	~Window();

	void Initialize(const std::wstring &Title, const Video_Mode &Mode, bool Fullscreen);
	void Finalize();

	void Set_Listener(Window_Listener *Listener);

	void Show();
	void Hide();

	void Handle_Events();
	void Display();

	void Show_Cursor();
	void Hide_Cursor();

private:
	static const wchar_t *Window_Class_Name;

	void Register_Class();
	void Unregister_Class();

	void Create_Window(const std::wstring &Title, const Video_Mode &Mode, bool Fullscreen);
	void Destroy_Window();

	void Create_Context(const Video_Mode &Mode);
	void Destroy_Context();

	void Switch_To_Fullscreen(const Video_Mode &Mode);

	LRESULT Handle_Message(HWND Handle, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK Window_Procedure(HWND Handle, UINT Message, WPARAM wParam, LPARAM lParam);

	HWND my_Handle;
	Video_Mode my_Video_Mode;
	bool my_Fullscreen;
	HDC my_Device_Context;
	HGLRC my_GL_Context;
	bool my_Class_Registered;
	Window_Listener *my_Listener;
};

#endif
