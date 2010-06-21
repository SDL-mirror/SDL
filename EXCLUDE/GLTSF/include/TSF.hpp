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
	class TSF_Text_Store : public ITextStoreACP, public ITfContextOwnerCompositionSink
	{
	public:
		//IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		//ITextStoreACP
		STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD dwMask);
		STDMETHODIMP UnadviseSink(IUnknown *punk);
		STDMETHODIMP RequestLock(DWORD dwLockFlags, HRESULT *phrSession);
		STDMETHODIMP GetStatus(TS_STATUS *pdcs);
		STDMETHODIMP QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG *pacpResultStart, LONG *pacpResultEnd);
		STDMETHODIMP GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP *pSelection, ULONG *pcFetched);
		STDMETHODIMP SetSelection(ULONG ulCount, const TS_SELECTION_ACP *pSelection);
		STDMETHODIMP GetText(LONG acpStart, LONG acpEnd, WCHAR *pchPlain, ULONG cchPlainReq, ULONG *pcchPlainRet, TS_RUNINFO *prgRunInfo, ULONG cRunInfoReq, ULONG *pcRunInfoRet, LONG *pacpNext);
		STDMETHODIMP SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR *pchText, ULONG cch, TS_TEXTCHANGE *pChange);
		STDMETHODIMP GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject **ppDataObject);
		STDMETHODIMP GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown **ppunk);
		STDMETHODIMP QueryInsertEmbedded(const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable);
		STDMETHODIMP InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject *pDataObject, TS_TEXTCHANGE *pChange);
		STDMETHODIMP InsertTextAtSelection(DWORD dwFlags, const WCHAR *pchText, ULONG cch, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange);
		STDMETHODIMP InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject *pDataObject, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange);
		STDMETHODIMP RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs);
		STDMETHODIMP RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags);
		STDMETHODIMP RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags);
		STDMETHODIMP FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags, LONG *pacpNext, BOOL *pfFound, LONG *plFoundOffset);
		STDMETHODIMP RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL *paAttrVals, ULONG *pcFetched);
		STDMETHODIMP GetEndACP(LONG *pacp);
		STDMETHODIMP GetActiveView(TsViewCookie *pvcView);
		STDMETHODIMP GetACPFromPoint(TsViewCookie vcView, const POINT *ptScreen, DWORD dwFlags, LONG *pacp);
		STDMETHODIMP GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped);
		STDMETHODIMP GetScreenExt(TsViewCookie vcView, RECT *prc);
		STDMETHODIMP GetWnd(TsViewCookie vcView, HWND *phwnd);

		//ITfOwnerCompositionSink
		STDMETHODIMP OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk);
		STDMETHODIMP OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew);
		STDMETHODIMP OnEndComposition(ITfCompositionView *pComposition);

		void Initialize();
		void Finalize();

		TSF_Text_Store();
		~TSF_Text_Store();

	private:
		ULONG my_Reference_Count;
		CComPtr<ITfDocumentMgr> my_Document_Manager;
		CComPtr<ITfContext> my_Context;
		DWORD my_Edit_Cookie;
		CComPtr<ITextStoreACPSink> my_Sink;
		DWORD my_Sink_Mask;
		DWORD my_Lock;
		DWORD my_Lock_Queued;
		CComPtr<ITfCompositionView> my_Composition_View;
		TS_SELECTION_ACP my_Composition_Selection;
	};

	TSF();

	static bool COM_Initialized;

	static CComPtr<ITfThreadMgr> Thread_Manager;
	static TfClientId Client_Id;
	static TSF_Text_Store *Text_Store;
};

#endif
