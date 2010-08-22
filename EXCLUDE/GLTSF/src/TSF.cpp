#include "TSF.hpp"
#include <stdexcept>

bool TSF::COM_Initialized = false;
CComPtr<ITfThreadMgr> TSF::Thread_Manager;
TfClientId TSF::Client_Id;
TSF::TSF_Text_Store *TSF::Text_Store = NULL;

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

		if (FAILED(Thread_Manager->Activate(&Client_Id)))
			throw std::runtime_error("ITfThreadMgr::Activate failed");

		Text_Store = new TSF_Text_Store;
		Text_Store->Initialize();
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

STDMETHODIMP TSF::TSF_Text_Store::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;
	if (IID_IUnknown == riid || IID_ITextStoreACP == riid)
		*ppvObject = static_cast<ITextStoreACP *>(this);
	else if (IID_ITfContextOwnerCompositionSink == riid)
		*ppvObject = static_cast<ITfContextOwnerCompositionSink *>(this);

	if (*ppvObject)
	{
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) TSF::TSF_Text_Store::AddRef()
{
	return ++my_Reference_Count;
}

STDMETHODIMP_(ULONG) TSF::TSF_Text_Store::Release()
{
	--my_Reference_Count;
	if (0 != my_Reference_Count)
		return my_Reference_Count;

	delete this;
	return 0;
}

#define CHECK_CONDITION(condition, retval, function, line)						\
	if (!condition)																\
	{																			\
		printf("%s:%d: Condition failure: %s\n", function, line, #condition);	\
	}

#define ENSURE(condition, retval)	CHECK_CONDITION(condition, retval, __FUNCTION__, __LINE__)

STDMETHODIMP TSF::TSF_Text_Store::AdviseSink(REFIID riid, IUnknown *punk, DWORD dwMask)
{
	ENSURE(punk && IID_ITextStoreACP == riid, E_INVALIDARG);

	if (!my_Sink)
	{
		HRESULT hr = punk->QueryInterface(&my_Sink);
		ENSURE(SUCCEEDED(hr) && my_Sink, E_UNEXPECTED);
	}
	else
	{
		CComPtr<IUnknown> Unknown_1, Unknown_2;
		punk->QueryInterface(&Unknown_1);
		my_Sink->QueryInterface(&Unknown_2);
		if (Unknown_1 != Unknown_2)
			return CONNECT_E_ADVISELIMIT;
	}
	my_Sink_Mask = dwMask;
	return S_OK;
}

STDMETHODIMP TSF::TSF_Text_Store::UnadviseSink(IUnknown *punk)
{
	ENSURE(punk, E_INVALIDARG);
	ENSURE(my_Sink, CONNECT_E_NOCONNECTION);

	CComPtr<IUnknown> Unknown_1, Unknown_2;
	punk->QueryInterface(&Unknown_1);
	my_Sink->QueryInterface(&Unknown_2);

	if (Unknown_1 != Unknown_2)
		return CONNECT_E_NOCONNECTION;

	my_Sink = NULL;
	my_Sink_Mask = 0;
	return S_OK;
}

STDMETHODIMP TSF::TSF_Text_Store::RequestLock(DWORD dwLockFlags, HRESULT *phrSession)
{
	ENSURE(my_Sink, E_FAIL);
	ENSURE(phrSession, E_INVALIDARG);
	if (my_Lock)
	{
		if (TS_LF_READ == (my_Lock & TS_LF_READWRITE)
			&& TS_LF_READWRITE == (dwLockFlags & TS_LF_READWRITE)
			&& !(dwLockFlags & TS_LF_SYNC))
		{
			*phrSession = TS_S_ASYNC;
			my_Lock_Queued = dwLockFlags & (~TS_LF_SYNC);
		}
		else
		{
			*phrSession = TS_E_SYNCHRONOUS;
			return E_FAIL;
		}
	}
	else
	{
		my_Lock = dwLockFlags & (~TS_LF_SYNC);
		*phrSession = my_Sink->OnLockGranted(my_Lock);
		while (my_Lock_Queued)
		{
			my_Lock = my_Lock_Queued;
			my_Lock_Queued = 0;
			my_Sink->OnLockGranted(my_Lock);
		}
		my_Lock = 0;
	}
	return S_OK;
}

STDMETHODIMP TSF::TSF_Text_Store::GetStatus(TS_STATUS *pdcs)
{
	ENSURE(pdcs, E_INVALIDARG);
	pdcs->dwDynamicFlags = 0;
	pdcs->dwStaticFlags = TS_SS_NOHIDDENTEXT;
	return S_OK;
}

STDMETHODIMP TSF::TSF_Text_Store::QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG *pacpResultStart, LONG *pacpResultEnd)
{
	ENSURE(0 <= acpTestStart && acpTestStart <= acpTestEnd && pacpResultStart && pacpResultEnd, E_INVALIDARG);

	*pacpResultStart = acpTestStart;
	*pacpResultEnd = acpTestStart + cch;
	return S_OK;
}

STDMETHODIMP TSF::TSF_Text_Store::GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP *pSelection, ULONG *pcFetched)
{
	ENSURE(TS_LF_READ == (my_Lock && TS_LF_READ), TS_E_NOLOCK);
	ENSURE(ulCount && pSelection && pcFetched, E_INVALIDARG);

	*pcFetched = 0;
	ENSURE(TS_DEFAULT_SELECTION == ulIndex || 0 == ulIndex, TS_E_NOSELECTION);
	if (my_Composition_View)
	{
		*pSelection = my_Composition_Selection;
	}
	else
	{
		//TODO
	}
	*pcFetched = 1;
	return S_OK;
}

STDMETHODIMP TSF::TSF_Text_Store::SetSelection(ULONG ulCount, const TS_SELECTION_ACP *pSelection)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::GetText(LONG acpStart, LONG acpEnd, WCHAR *pchPlain, ULONG cchPlainReq, ULONG *pcchPlainRet, TS_RUNINFO *prgRunInfo, ULONG cRunInfoReq, ULONG *pcRunInfoRet, LONG *pacpNext)
{
	ENSURE(TS_LF_READ == (my_Lock & TS_LF_READ), TS_E_NOLOCK);
	ENSURE(pcchPlainRet && (pchPlain || prgRunInfo)
		&& (!cchPlainReq == !pchPlain)
		&& (!cRunInfoReq == !prgRunInfo), E_INVALIDARG);
	ENSURE(0 <= acpStart && -1 <= acpEnd
		&& (-1 == acpEnd || acpStart <= acpEnd), TS_E_INVALIDPOS);

	*pcchPlainRet = 0;
	if (pchPlain && cchPlainReq) *pchPlain = 0;
	if (pcRunInfoRet) *pcRunInfoRet = 0;
	//TODO
	return S_OK;
}

STDMETHODIMP TSF::TSF_Text_Store::SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR *pchText, ULONG cch, TS_TEXTCHANGE *pChange)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject **ppDataObject)
{
	//not needed
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown **ppunk)
{
	//not needed
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::QueryInsertEmbedded(const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable)
{
	if (!pfInsertable)
		return E_INVALIDARG;

	//Not supported
	*pfInsertable = FALSE;
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject *pDataObject, TS_TEXTCHANGE *pChange)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::InsertTextAtSelection(DWORD dwFlags, const WCHAR *pchText, ULONG cch, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject *pDataObject, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange)
{
	//not needed
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs)
{
	//not needed
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags)
{
	//not needed
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags)
{
	//not needed
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags, LONG *pacpNext, BOOL *pfFound, LONG *plFoundOffset)
{
	//not needed
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL *paAttrVals, ULONG *pcFetched)
{
	//not needed
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::GetEndACP(LONG *pacp)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::GetActiveView(TsViewCookie *pvcView)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::GetACPFromPoint(TsViewCookie vcView, const POINT *ptScreen, DWORD dwFlags, LONG *pacp)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::GetScreenExt(TsViewCookie vcView, RECT *prc)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::GetWnd(TsViewCookie vcView, HWND *phwnd)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSF::TSF_Text_Store::OnEndComposition(ITfCompositionView *pComposition)
{
	return E_NOTIMPL;
}

TSF::TSF_Text_Store::TSF_Text_Store() : my_Reference_Count(1),
										my_Edit_Cookie(0),
										my_Lock(0),
										my_Lock_Queued(0)
{

}

TSF::TSF_Text_Store::~TSF_Text_Store()
{

}

void TSF::TSF_Text_Store::Initialize()
{
	if (FAILED(Thread_Manager->CreateDocumentMgr(&my_Document_Manager)))
		throw std::runtime_error("Failed to create document manager");

	if (FAILED(my_Document_Manager->CreateContext(Client_Id, 0, static_cast<ITextStoreACP *>(this), &my_Context, &my_Edit_Cookie)))
		throw std::runtime_error("Failed to create document context");

	if (FAILED(my_Document_Manager->Push(my_Context)))
		throw std::runtime_error("Failed to push context");
}

void TSF::TSF_Text_Store::Finalize()
{

}
