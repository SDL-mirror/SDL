#ifndef _SDL_msctf_h
#define _SDL_msctf_h

#define CONST_VTBL
#include <basetyps.h>
#include <unknwn.h>

EXTERN_C const IID IID_ITfInputProcessorProfileActivationSink;
EXTERN_C const IID IID_ITfUIElementSink;
EXTERN_C const IID IID_ITfSource;
EXTERN_C const IID IID_ITfUIElementMgr;
EXTERN_C const IID IID_ITfReadingInformationUIElement;
EXTERN_C const IID IID_ITfThreadMgr;
EXTERN_C const IID IID_ITfThreadMgrEx;

EXTERN_C const CLSID CLSID_TF_ThreadMgr;
EXTERN_C const GUID GUID_TFCAT_TIP_KEYBOARD;

#define     TF_INVALID_COOKIE               (0xffffffff)
#define     TF_IPSINK_FLAG_ACTIVE           0x0001
#define     TF_TMAE_UIELEMENTENABLEDONLY    0x00000004

typedef _COM_interface ITfThreadMgr ITfThreadMgr;
typedef _COM_interface ITfDocumentMgr ITfDocumentMgr;
typedef _COM_interface ITfClientId ITfClientId;

typedef _COM_interface IEnumTfDocumentMgrs IEnumTfDocumentMgrs;
typedef _COM_interface IEnumTfFunctionProviders IEnumTfFunctionProviders;
typedef _COM_interface ITfFunctionProvider ITfFunctionProvider;
typedef _COM_interface ITfCompartmentMgr ITfCompartmentMgr;
typedef _COM_interface ITfContext ITfContext;
typedef _COM_interface IEnumTfContexts IEnumTfContexts;
typedef _COM_interface ITfUIElementSink ITfUIElementSink;
typedef _COM_interface ITfUIElement ITfUIElement;
typedef _COM_interface ITfUIElementMgr ITfUIElementMgr;
typedef _COM_interface IEnumTfUIElements IEnumTfUIElements;
typedef _COM_interface ITfThreadMgrEx ITfThreadMgrEx;
typedef _COM_interface ITfReadingInformationUIElement ITfReadingInformationUIElement;
typedef _COM_interface ITfInputProcessorProfileActivationSink ITfInputProcessorProfileActivationSink;
typedef _COM_interface ITfSource ITfSource;

typedef DWORD TfClientId;
typedef DWORD TfEditCookie;

typedef struct ITfThreadMgrVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITfThreadMgr *, REFIID, void **);
    ULONG (STDMETHODCALLTYPE *AddRef)(ITfThreadMgr *);
    ULONG (STDMETHODCALLTYPE *Release)(ITfThreadMgr *);
    HRESULT (STDMETHODCALLTYPE *Activate)(ITfThreadMgr *, TfClientId *);
    HRESULT (STDMETHODCALLTYPE *Deactivate)(ITfThreadMgr *);
    HRESULT (STDMETHODCALLTYPE *CreateDocumentMgr)(ITfThreadMgr *);
    HRESULT (STDMETHODCALLTYPE *EnumDocumentMgrs)(ITfThreadMgr *, IEnumTfDocumentMgrs **);
    HRESULT (STDMETHODCALLTYPE *GetFocus)(ITfThreadMgr *, ITfDocumentMgr **);
    HRESULT (STDMETHODCALLTYPE *SetFocus)(ITfThreadMgr *, ITfDocumentMgr *);
    HRESULT (STDMETHODCALLTYPE *AssociateFocus)(ITfThreadMgr *, HWND, ITfDocumentMgr *, ITfDocumentMgr **);
    HRESULT (STDMETHODCALLTYPE *IsThreadFocus)(ITfThreadMgr *, BOOL *);
    HRESULT (STDMETHODCALLTYPE *GetFunctionProvider)(ITfThreadMgr *, REFCLSID, ITfFunctionProvider **);
    HRESULT (STDMETHODCALLTYPE *EnumFunctionProviders)(ITfThreadMgr *, IEnumTfFunctionProviders **);
    HRESULT (STDMETHODCALLTYPE *GetGlobalCompartment)(ITfThreadMgr *, ITfCompartmentMgr **);
} ITfThreadMgrVtbl;

_COM_interface ITfThreadMgr
{
    CONST_VTBL struct ITfThreadMgrVtbl *lpVtbl;
};

typedef struct ITfThreadMgrExVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITfThreadMgrEx *, REFIID, void **);
    ULONG (STDMETHODCALLTYPE *AddRef)(ITfThreadMgrEx *);
    ULONG (STDMETHODCALLTYPE *Release)(ITfThreadMgrEx *);
    HRESULT (STDMETHODCALLTYPE *Activate)(ITfThreadMgrEx *, TfClientId *);
    HRESULT (STDMETHODCALLTYPE *Deactivate)(ITfThreadMgrEx *);
    HRESULT (STDMETHODCALLTYPE *CreateDocumentMgr)(ITfThreadMgrEx *, ITfDocumentMgr **);
    HRESULT (STDMETHODCALLTYPE *EnumDocumentMgrs)(ITfThreadMgrEx *, IEnumTfDocumentMgrs **);
    HRESULT (STDMETHODCALLTYPE *GetFocus)(ITfThreadMgrEx *, ITfDocumentMgr **);
    HRESULT (STDMETHODCALLTYPE *SetFocus)(ITfThreadMgrEx *, ITfDocumentMgr *);
    HRESULT (STDMETHODCALLTYPE *AssociateFocus)(ITfThreadMgrEx *, ITfDocumentMgr *, ITfDocumentMgr **);
    HRESULT (STDMETHODCALLTYPE *IsThreadFocus)(ITfThreadMgrEx *, BOOL *);
    HRESULT (STDMETHODCALLTYPE *GetFunctionProvider)(ITfThreadMgrEx *, REFCLSID, ITfFunctionProvider **);
    HRESULT (STDMETHODCALLTYPE *EnumFunctionProviders)(ITfThreadMgrEx *, IEnumTfFunctionProviders **);
    HRESULT (STDMETHODCALLTYPE *GetGlobalCompartment)(ITfThreadMgrEx *, ITfCompartmentMgr **);
    HRESULT (STDMETHODCALLTYPE *ActivateEx)(ITfThreadMgrEx *, TfClientId *, DWORD);
    HRESULT (STDMETHODCALLTYPE *GetActiveFlags)(ITfThreadMgrEx *, DWORD *);
} ITfThreadMgrExVtbl;

_COM_interface ITfThreadMgrEx
{
    CONST_VTBL struct ITfThreadMgrExVtbl *lpVtbl;
};

typedef struct ITfDocumentMgrVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITfDocumentMgr *, REFIID, void **);
    ULONG (STDMETHODCALLTYPE *AddRef)(ITfDocumentMgr *);
    ULONG (STDMETHODCALLTYPE *Release)(ITfDocumentMgr *);
    HRESULT (STDMETHODCALLTYPE *CreateContext)(ITfDocumentMgr *, TfClientId, DWORD, IUnknown *, ITfContext **, TfEditCookie *);
    HRESULT (STDMETHODCALLTYPE *Push)(ITfDocumentMgr *, ITfContext *);
    HRESULT (STDMETHODCALLTYPE *Pop)(ITfDocumentMgr *);
    HRESULT (STDMETHODCALLTYPE *GetTop)(ITfDocumentMgr *, ITfContext **);
    HRESULT (STDMETHODCALLTYPE *GetBase)(ITfDocumentMgr *, ITfContext **);
    HRESULT (STDMETHODCALLTYPE *EnumContexts)(ITfDocumentMgr *, IEnumTfContexts **);
} ITfDocumentMgrVtbl;

_COM_interface ITfDocumentMgr
{
    CONST_VTBL struct ITfDocumentMgrVtbl *lpVtbl;
};

typedef struct ITfUIElementSinkVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITfUIElementSink *, REFIID, void **);
    ULONG (STDMETHODCALLTYPE *AddRef)(ITfUIElementSink *);
    ULONG (STDMETHODCALLTYPE *Release)(ITfUIElementSink *);
    HRESULT (STDMETHODCALLTYPE *BeginUIElement)(ITfUIElementSink *, DWORD, BOOL *);
    HRESULT (STDMETHODCALLTYPE *UpdateUIElement)(ITfUIElementSink *, DWORD);
    HRESULT (STDMETHODCALLTYPE *EndUIElement)(ITfUIElementSink *, DWORD);
} ITfUIElementSinkVtbl;

_COM_interface ITfUIElementSink
{
    CONST_VTBL struct ITfUIElementSinkVtbl *lpVtbl;
};

typedef struct ITfUIElementMgrVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITfUIElementMgr *, REFIID, void **);
    ULONG (STDMETHODCALLTYPE *AddRef)(ITfUIElementMgr *);
    ULONG (STDMETHODCALLTYPE *Release)(ITfUIElementMgr *);
    HRESULT (STDMETHODCALLTYPE *BeginUIElement)(ITfUIElementMgr *, ITfUIElement *, BOOL *, DWORD *);
    HRESULT (STDMETHODCALLTYPE *UpdateUIElement)(ITfUIElementMgr *, DWORD);
    HRESULT (STDMETHODCALLTYPE *EndUIElement)(ITfUIElementMgr *, DWORD);
    HRESULT (STDMETHODCALLTYPE *GetUIElement)(ITfUIElementMgr *, DWORD, ITfUIElement **);
    HRESULT (STDMETHODCALLTYPE *EnumUIElements)(ITfUIElementMgr *, IEnumTfUIElements **);
} ITfUIElementMgrVtbl;

_COM_interface ITfUIElementMgr
{
    CONST_VTBL struct ITfUIElementMgrVtbl *lpVtbl;
};

typedef struct ITfReadingInformationUIElementVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITfReadingInformationUIElement *, REFIID, void **);
    ULONG (STDMETHODCALLTYPE *AddRef)(ITfReadingInformationUIElement *);
    ULONG (STDMETHODCALLTYPE *Release)(ITfReadingInformationUIElement *);
    HRESULT (STDMETHODCALLTYPE *GetDescription)(ITfReadingInformationUIElement *, BSTR *);
    HRESULT (STDMETHODCALLTYPE *GetGUID)(ITfReadingInformationUIElement *, GUID *);
    HRESULT (STDMETHODCALLTYPE *Show)(ITfReadingInformationUIElement *, BOOL);
    HRESULT (STDMETHODCALLTYPE *IsShown)(ITfReadingInformationUIElement *, BOOL *);
    HRESULT (STDMETHODCALLTYPE *GetUpdatedFlags)(ITfReadingInformationUIElement *, DWORD *);
    HRESULT (STDMETHODCALLTYPE *GetContext)(ITfReadingInformationUIElement *, ITfContext **);
    HRESULT (STDMETHODCALLTYPE *GetString)(ITfReadingInformationUIElement *, BSTR *);
    HRESULT (STDMETHODCALLTYPE *GetMaxReadingStringLength)(ITfReadingInformationUIElement *, UINT *);
    HRESULT (STDMETHODCALLTYPE *GetErrorIndex)(ITfReadingInformationUIElement *, UINT *);
    HRESULT (STDMETHODCALLTYPE *IsVerticalOrderPreferred)(ITfReadingInformationUIElement *, BOOL *);
} ITfReadingInformationUIElementVtbl;

_COM_interface ITfReadingInformationUIElement
{
    CONST_VTBL struct ITfReadingInformationUIElementVtbl *lpVtbl;
};

typedef struct ITfUIElementVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITfUIElement *, REFIID, void **);
    ULONG (STDMETHODCALLTYPE *AddRef)(ITfUIElement *);
    ULONG (STDMETHODCALLTYPE *Release)(ITfUIElement *);
    HRESULT (STDMETHODCALLTYPE *GetDescription)(ITfUIElement *, BSTR *);
    HRESULT (STDMETHODCALLTYPE *GetGUID)(ITfUIElement *, GUID *);
    HRESULT (STDMETHODCALLTYPE *Show)(ITfUIElement *, BOOL);
    HRESULT (STDMETHODCALLTYPE *IsShown)(ITfUIElement *, BOOL *);
} ITfUIElementVtbl;

_COM_interface ITfUIElement
{
    CONST_VTBL struct ITfUIElementVtbl *lpVtbl;
};

typedef struct ITfInputProcessorProfileActivationSinkVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITfInputProcessorProfileActivationSink *, REFIID, void **);
    ULONG (STDMETHODCALLTYPE *AddRef)(ITfInputProcessorProfileActivationSink *);
    ULONG (STDMETHODCALLTYPE *Release)(ITfInputProcessorProfileActivationSink *);
    HRESULT (STDMETHODCALLTYPE *OnActivated)(ITfInputProcessorProfileActivationSink *, DWORD, LANGID, REFCLSID, REFGUID, REFGUID, HKL, DWORD);

} ITfInputProcessorProfileActivationSinkVtbl;

_COM_interface ITfInputProcessorProfileActivationSink
{
    CONST_VTBL struct ITfInputProcessorProfileActivationSinkVtbl *lpVtbl;
};

typedef struct ITfSourceVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITfSource *, REFIID, void **);
    ULONG (STDMETHODCALLTYPE *AddRef)(ITfSource *);
    ULONG (STDMETHODCALLTYPE *Release)(ITfSource *);
    HRESULT (STDMETHODCALLTYPE *AdviseSink)(ITfSource *, REFIID, IUnknown *, DWORD *);
    HRESULT (STDMETHODCALLTYPE *UnadviseSink)(ITfSource *, DWORD);
} ITfSourceVtbl;

_COM_interface ITfSource
{
    CONST_VTBL struct ITfSourceVtbl *lpVtbl;
};

#endif /* _SDL_msctf_h */
