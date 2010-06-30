#include "IMM.hpp"
#include <stdexcept>

IMM::IMM() : my_COM_Initialized(false),
			 my_Thread_Manager(0),
			 my_Window(0),
			 my_Context(0),
			 my_HKL(0),
			 my_Vertical_Candidates(false)
{

}

IMM::~IMM()
{
	Finalize();
}

void IMM::Initialize(HWND Window)
{
	Finalize();

	my_Window = Window;

	if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
	{
		my_COM_Initialized = true;
		if (SUCCEEDED(CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, reinterpret_cast<LPVOID *>(&my_Thread_Manager))))
		{
			ITfDocumentMgr *Document_Manager = 0;
			if (FAILED(my_Thread_Manager->AssociateFocus(Window, NULL, &Document_Manager)))
				printf("Warning: ITfThreadMgr->AssociateFocus failed\n");

			if (Document_Manager)
				Document_Manager->Release();
		}
		else
			printf("Warning: Failed to create ITfThreadMgr instance\n");
	}
	else
		printf("Warning: Failed to initialize COM\n");

	ImmDisableTextFrameService(-1);

	my_Context = ImmGetContext(my_Window);
	if (!ImmReleaseContext(my_Window, my_Context))
		throw std::runtime_error("Error releasing context");

	if (!my_Context)
		throw std::runtime_error("No context");

	Update_Input_Locale();
}

void IMM::Finalize()
{
	if (my_Thread_Manager)
	{
		my_Thread_Manager->Release();
		my_Thread_Manager = 0;
	}
	if (my_COM_Initialized)
	{
		CoUninitialize();
		my_COM_Initialized = false;
	}
}

#define GET_LANG(hkl) LOWORD((hkl))
#define GET_PRIMLANG(hkl) ((WORD)PRIMARYLANGID(GET_LANG((hkl))))
#define GET_SUBLANG(hkl) SUBLANGID(GET_LANG((hkl)))

void IMM::Update_Input_Locale()
{
	static HKL Previous_HKL = 0;
	my_HKL = GetKeyboardLayout(0);
	if (Previous_HKL == my_HKL)
		return;

	Previous_HKL = my_HKL;
	my_Vertical_Candidates = false;
	switch (GET_PRIMLANG(my_HKL))
	{
	case LANG_CHINESE:
		my_Vertical_Candidates = true;
		switch (GET_SUBLANG(my_HKL))
		{
		case SUBLANG_CHINESE_SIMPLIFIED:
			my_Vertical_Candidates = false;
			break;
		}
		break;
	case LANG_JAPANESE:
		my_Vertical_Candidates = true;
		break;
	}
}

LRESULT IMM::Handle_Message(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, bool &Ate)
{
	Ate = false;
	switch (Message)
	{
	case WM_INPUTLANGCHANGE:
		Update_Input_Locale();
		break;
	case WM_IME_SETCONTEXT:
		lParam = 0;
		return DefWindowProcW(my_Window, Message, wParam, lParam);
		break;
	case WM_IME_STARTCOMPOSITION:
		Ate = true;
		break;
	case WM_IME_COMPOSITION:
		{
			Ate = true;
			HIMC Context = ImmGetContext(Window);
			if (!Context)
				break;

			if (lParam & GCS_RESULTSTR)
			{
				LONG Length = ImmGetCompositionStringW(Context, GCS_RESULTSTR, 0, 0);
				std::wstring Composition(Length / sizeof(wchar_t), 0);
				Length = ImmGetCompositionStringW(Context, GCS_RESULTSTR, &Composition[0], Composition.size() * sizeof(Composition[0]));
				printf("GCS_RESULTSTR: ");
				for (LONG i = 0; i < Length / sizeof(wchar_t); ++i)
					printf("U+%04X ", Composition[i]);

				printf("\n");
			}
			if (lParam & GCS_COMPSTR)
			{
				LONG Length = ImmGetCompositionStringW(Context, GCS_COMPSTR, 0, 0);
				std::wstring Composition(Length / sizeof(wchar_t), 0);
				Length = ImmGetCompositionStringW(Context, GCS_COMPSTR, &Composition[0], Composition.size() * sizeof(Composition[0]));
				printf("GCS_COMPSTR: ");
				for (LONG i = 0; i < Length / sizeof(wchar_t); ++i)
					printf("U+%04X ", Composition[i]);

				printf("\n");
			}
			ImmReleaseContext(Window, Context);
		}
		break;
	case WM_IME_ENDCOMPOSITION:
		break;
	case WM_IME_NOTIFY:
		switch (wParam)
		{
		case IMN_SETCONVERSIONMODE:

			break;
		case IMN_SETOPENSTATUS:
			Update_Input_Locale();
			break;
		case IMN_OPENCANDIDATE:
		case IMN_CHANGECANDIDATE:
			Ate = true;
			break;
		}
		break;
	}
	return 0;
}
