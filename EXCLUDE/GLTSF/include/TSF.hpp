#ifndef TSF_HPP
#define TSF_HPP

#include <msctf.h>
#include <atlbase.h>

class TSF
{
public:

protected:
	class UI_Sink : public ITfUIElementSink, public ITfInputProcessorProfileActivationSink
	{
	public:
		UI_Sink();
		~UI_Sink();

		// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
		STDMETHODIMP_(ULONG) AddRef(void);
		STDMETHODIMP_(ULONG) Release(void);

		// ITfUIElementSink
		STDMETHODIMP BeginUIElement(DWORD dwUIElementId, BOOL *pbShow);
		STDMETHODIMP UpdateUIElement(DWORD dwUIElementId);
		STDMETHODIMP EndUIElement(DWORD dwUIElementId);

		// ITfInputProcessorProfileActivationSink
		STDMETHODIMP OnActivated(DWORD dwProfileType, LANGID langid,
								 REFCLSID clsid, REFGUID catid,
								 REFGUID guidProfile, HKL hkl,
								 DWORD dwFlags);

		// ITfCompartmentEventSink
		STDMETHODIMP OnChange(REFGUID rguid);

	private:
		LONG my_Reference_Count;
	};

	TSF();
	~TSF();

	void Initialize();
	void Finalize();

private:
	bool my_COM_Initialized;

	CComPtr<ITfThreadMgrEx> my_Thread_Manager;
	UI_Sink *my_UI_Sink;

	DWORD my_UI_Element_Sink_Cookie;
	DWORD my_IPPA_Sink_Cookie;
};

#endif
