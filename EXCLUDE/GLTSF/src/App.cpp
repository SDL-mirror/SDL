#include "App.hpp"

App::App() : my_Done(false)
{

}

App::~App()
{
	Finalize();
}

void App::Initialize()
{
	Finalize();

	my_Window.Initialize(L"GLTSF", Video_Mode(800, 600, 32), false);
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
