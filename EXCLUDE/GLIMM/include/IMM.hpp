#ifndef IMM_HPP
#define IMM_HPP

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <msctf.h>

class IMM
{
public:
	IMM();
	~IMM();

	void Initialize(HWND Window);
	void Finalize();

	LRESULT Handle_Message(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, bool &Ate);

private:
	void Update_Input_Locale();

	bool my_COM_Initialized;
	ITfThreadMgr *my_Thread_Manager;
	HWND my_Window;
	HIMC my_Context;
	HKL my_HKL;
	bool my_Vertical_Candidates;
};

#endif
