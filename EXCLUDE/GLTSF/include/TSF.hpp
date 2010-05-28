#ifndef TSF_HPP
#define TSF_HPP

#include <msctf.h>

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

private:

};

#endif
