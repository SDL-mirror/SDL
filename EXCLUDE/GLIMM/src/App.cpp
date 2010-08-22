#include "App.hpp"
#include <GL/gl.h>
#include <GL/glu.h>

#pragma comment(lib, "glu32.lib")

GLfloat Rotation = 0.0f;

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

	my_Window.Initialize(L"GLIMM", Video_Mode(Width, Height, Bits_Per_Pixel), Fullscreen);
	my_Window.Set_Listener(this);
	my_Window.Show();
	my_Window.Hide_Cursor();
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
		my_Window.Handle_Events();

		Update();
		Draw();
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

void App::On_Resized(unsigned int Width, unsigned int Height)
{
	glViewport(0, 0, Width, Height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void App::On_Mouse_Button_Down(Mouse_Button Button)
{
	switch (Button)
	{
	case Mouse_Button_Left:
		my_Window.Get_IMM().Toggle();
		break;
	}
}

void App::Update()
{
	Rotation += 0.2f;
}

void App::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glLoadIdentity();
	glRotatef(Rotation, 0.0f, 0.0f, -1.0f);

	glBegin(GL_TRIANGLES);
		glColor3f(0.7f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.5f, 0.0f);
		glColor3f(0.0f, 0.7f, 0.0f);
		glVertex3f(-0.5f, -0.5f, 0.0f);
		glColor3f(0.0f, 0.0f, 0.7f);
		glVertex3f(0.5f, -0.5f, 0.0f);
	glEnd();
}
