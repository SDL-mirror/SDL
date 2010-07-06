#ifndef WINDOW_LISTENER_HPP
#define WINDOW_LISTENER_HPP

enum Mouse_Button
{
	Mouse_Button_Left,
	Mouse_Button_Right
};

class Window_Listener
{
public:
	virtual void On_Close(){}
	virtual void On_Key_Down(int Key){}
	virtual void On_Key_Up(int Key){}
	virtual void On_Char(unsigned int Char){}
	virtual void On_Resized(unsigned int Width, unsigned int Height){}
	virtual void On_Mouse_Button_Down(Mouse_Button Button){}
	virtual void On_Mouse_Button_Up(Mouse_Button Button){}

};

#endif
