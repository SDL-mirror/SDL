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

private:
	Window my_Window;
	bool my_Done;
};

#endif
