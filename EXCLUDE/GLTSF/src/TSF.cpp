#include "TSF.hpp"
#include <stdexcept>

bool TSF::COM_Initialized = false;
CComPtr<ITfThreadMgr> TSF::Thread_Manager;

void TSF::Initialize()
{
	if (!COM_Initialized)
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		if (S_OK != hr && S_FALSE != hr)
			throw std::runtime_error("Failed to initialize COM");

		COM_Initialized = true;
	}
	if (!Thread_Manager)
	{
		if (FAILED(CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, reinterpret_cast<void **>(&Thread_Manager))))
			throw std::runtime_error("Failed to create ITfThreadMgr instance");

		TfClientId ClientId;
		if (FAILED(Thread_Manager->Activate(&ClientId)))
			throw std::runtime_error("ITfThreadMgr::Activate failed");
	}
}

void TSF::Finalize()
{
	if (Thread_Manager)
	{
		Thread_Manager->Deactivate();
		Thread_Manager = NULL;
	}
	if (COM_Initialized)
	{
		CoUninitialize();
		COM_Initialized = false;
	}
}
