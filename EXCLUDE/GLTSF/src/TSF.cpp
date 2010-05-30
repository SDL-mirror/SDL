#include "TSF.hpp"
#include <stdexcept>

TSF::TSF() : my_COM_Initialized(false),
			 my_UI_Sink(0),
			 my_UI_Element_Sink_Cookie(TF_INVALID_COOKIE),
			 my_IPPA_Sink_Cookie(TF_INVALID_COOKIE)
{

}

TSF::~TSF()
{
	Finalize();
}

void TSF::Initialize()
{
	Finalize();

	if (S_OK != CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))
		throw std::runtime_error("Failed to initialize COM");

	my_COM_Initialized = true;
	if (S_OK != CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, __uuidof(ITfThreadMgrEx), (void **)&my_Thread_Manager))
		throw std::runtime_error("Failed to create ITfThreadMgrEx instance");

	TfClientId Client_Id = 0;
	if (FAILED(my_Thread_Manager->ActivateEx(&Client_Id, TF_TMAE_UIELEMENTENABLEDONLY)))
		throw std::runtime_error("ITfThreadMgrEx::ActivateEx failed");

	my_UI_Sink = new UI_Sink();
	ITfSource *Source = NULL;
	if (FAILED(my_Thread_Manager->QueryInterface(__uuidof(ITfSource), (void **)&Source)))
		throw std::runtime_error("QueryInterface failed");

	if (FAILED(Source->AdviseSink(__uuidof(ITfUIElementSink), (ITfUIElementSink *)my_UI_Sink, &my_UI_Element_Sink_Cookie)))
		throw std::runtime_error("AdviseSink failed");

	if (FAILED(Source->AdviseSink(__uuidof(ITfInputProcessorProfileActivationSink), (ITfInputProcessorProfileActivationSink *)my_UI_Sink, &my_IPPA_Sink_Cookie)))
		throw std::runtime_error("AdviseSink failed");
}

void TSF::Finalize()
{
	if (my_UI_Sink)
	{
		ITfSource *Source = NULL;
		if (SUCCEEDED(my_Thread_Manager->QueryInterface(__uuidof(ITfSource), (void **)&Source)))
		{
			Source->UnadviseSink(my_IPPA_Sink_Cookie);
			Source->UnadviseSink(my_UI_Element_Sink_Cookie);
			Source->Release();
		}
		if (my_Thread_Manager)
			my_Thread_Manager->Deactivate();

		my_UI_Sink->Release();
		delete my_UI_Sink;
		my_UI_Sink = NULL;
	}
	my_Thread_Manager = NULL;
	if (my_COM_Initialized)
	{
		CoUninitialize();
		my_COM_Initialized = false;
	}
}

TSF::UI_Sink::UI_Sink()
{
	my_Reference_Count = 1;
}

TSF::UI_Sink::~UI_Sink()
{

}

STDMETHODIMP TSF::UI_Sink::QueryInterface(REFIID riid, void **ppvObj)
{
	if (NULL == ppvObj)
		return E_INVALIDARG;

	*ppvObj = NULL;
	if (IsEqualIID(riid, IID_IUnknown))
		*ppvObj = reinterpret_cast<IUnknown *>(this);
	else if (IsEqualIID(riid, __uuidof(ITfUIElementSink)))
		*ppvObj = reinterpret_cast<ITfUIElementSink *>(this);
	else if (IsEqualIID(riid, __uuidof(ITfInputProcessorProfileActivationSink)))
		*ppvObj = reinterpret_cast<ITfInputProcessorProfileActivationSink *>(this);
	else if (IsEqualIID(riid, __uuidof(ITfCompartmentEventSink)))
		*ppvObj = reinterpret_cast<ITfCompartmentEventSink *>(this);

	if (*ppvObj)
	{
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG TSF::UI_Sink::AddRef(void)
{
	return ++my_Reference_Count;
}

ULONG TSF::UI_Sink::Release(void)
{
	LONG Count = --my_Reference_Count;
	if (0 == Count)
		delete this;

	return Count;
}

STDMETHODIMP TSF::UI_Sink::BeginUIElement(DWORD dwUIElementId, BOOL *pbShow)
{
	return S_OK;
}

STDMETHODIMP TSF::UI_Sink::UpdateUIElement(DWORD dwUIElementId)
{
	return S_OK;
}

STDMETHODIMP TSF::UI_Sink::EndUIElement(DWORD dwUIElementId)
{
	return S_OK;
}

STDMETHODIMP TSF::UI_Sink::OnActivated(DWORD dwProfileType, LANGID langid, REFCLSID clsid, REFGUID catid, REFGUID guidProfile, HKL hkl, DWORD dwFlags)
{
	return S_OK;
}

STDMETHODIMP TSF::UI_Sink::OnChange(REFGUID rguid)
{
	return S_OK;
}
