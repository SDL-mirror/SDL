#ifndef WINDOW_LISTENER_HPP
#define WINDOW_LISTENER_HPP

class Window_Listener
{
public:
	virtual void On_Close(){}
	virtual void On_Key_Down(int Key){}
	virtual void On_Key_Up(int Key){}
	virtual void On_Char(unsigned int Char){}
	virtual void On_Resized(unsigned int Width, unsigned int Height){}
};

#endif
