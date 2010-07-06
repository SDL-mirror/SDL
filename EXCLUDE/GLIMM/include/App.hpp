#ifndef APP_HPP
#define APP_HPP

#include "Window.hpp"

class App : public Window_Listener
{
public:
	App();
	virtual ~App();

	void Initialize();
	void Finalize();

	void Run();

	virtual void On_Close();
	virtual void On_Key_Down(int Key);
	virtual void On_Key_Up(int Key);
	virtual void On_Char(unsigned int Char);
	virtual void On_Resized(unsigned int Width, unsigned int Height);
	virtual void On_Mouse_Button_Down(Mouse_Button Button);

private:
	void Update();
	void Draw();

	static const int Width = 800;
	static const int Height = 600;
	static const int Bits_Per_Pixel = 32;
	static const bool Fullscreen = true;

	Window my_Window;
	bool my_Done;
};

#endif
