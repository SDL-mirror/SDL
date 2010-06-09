#ifndef TSF_HPP
#define TSF_HPP

#include <msctf.h>
#include <atlbase.h>

class TSF
{
public:
	static void Initialize();
	static void Finalize();

private:
	TSF();

	static bool COM_Initialized;

	static CComPtr<ITfThreadMgr> Thread_Manager;
};

#endif
