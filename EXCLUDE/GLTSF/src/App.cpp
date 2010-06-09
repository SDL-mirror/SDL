#include "App.hpp"
#include "TSF.hpp"

App::App() : my_Done(false)
{
	TSF::Initialize();
}

App::~App()
{
	Finalize();
	TSF::Finalize();
}

void App::Initialize()
{
	Finalize();

	my_Window.Initialize(L"GLTSF", Video_Mode(Width, Height, Bits_Per_Pixel), Fullscreen);
	my_Window.Set_Listener(this);
	my_Window.Show();
}

void App::Finalize()
{
	my_Window.Finalize();
}

void App::Run()
{
	Initialize();
	while (!my_Done)
	{
		my_Window.Update();
		my_Window.Clear();
		my_Window.Display();
	}
}

void App::On_Close()
{
	my_Done = true;
	my_Window.Hide();
}

void App::On_Key_Down(int Key)
{
	switch (Key)
	{
	case VK_ESCAPE:
		On_Close();
		break;
	}
}

void App::On_Key_Up(int Key)
{

}

void App::On_Char(unsigned int Char)
{
	printf("Char: U+%04X\n", Char);
}
