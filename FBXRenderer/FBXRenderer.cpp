﻿
// FBXRenderer.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//


#include "framework.h"
#include "FBXRenderer.h"

#include<shellapi.h>
#include <shobjidl.h>

#include "imgui.h"

#include "SSRenderer/SSRenderer.h"

#include "SSEngineDefault/SSDebugLogger.h"
#include "SSEngineDefault/SSInput.h"


#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE g_hInst;                                // 현재 인스턴스입니다.
HWND g_hWnd;
RECT g_WndRect{0,0,1920,1080};
SSRenderer g_Renderer;
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.


HRESULT					InitWindow(HINSTANCE, int, RECT);
LRESULT CALLBACK		WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK		About(HWND, UINT, WPARAM, LPARAM);
void					CheckRemainingObjects();

void					AnalyzeCommandLineArguements();
// HINSTANCE는 해당 어플리케이션에 해당하는 값. ("프로그램"에 대응, 똑같은 프로그램을 두 개 띄워도 HINSTANCE임)
// HWND는 해당 어플리케이션의 하나의 "윈도우"에 해당하는 값 ("윈도우"에 대흥, 똑같은 프로그램을 두 개 띄우면 두 HWND는 다름)

HRESULT FindFilePathWithOpenDialog(SS::FixedStringW<PATH_LEN_MAX>& OutFilePath)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;

		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			hr = pFileOpen->Show(NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

					// Display the file name to the user.
					if (SUCCEEDED(hr))
					{
						OutFilePath = pszFilePath;
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}
	return hr;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
	SS_LOG("========================== Select file to Open ==========================");
	SS::FixedStringW<PATH_LEN_MAX> FbxFilePathToLoad;
	FindFilePathWithOpenDialog(FbxFilePathToLoad);
	if(FbxFilePathToLoad.GetLen() == 0)
	{
		return 0;
	}

	AnalyzeCommandLineArguements();

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif

	// 전역 문자열을 초기화합니다.
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_FBXRENDERER, szWindowClass, MAX_LOADSTRING);

	// 애플리케이션 초기화를 수행합니다:
	if (FAILED(InitWindow(hInstance, nCmdShow, g_WndRect)))
	{
		__debugbreak();
		return FALSE;
	}
	g_hInst = hInstance;


	g_Renderer.BindFbxFilePathToImport(FbxFilePathToLoad);
	HRESULT hr = g_Renderer.Init(g_hInst, g_hWnd);
	if (FAILED(hr)) {
		__debugbreak();
		g_Renderer.CleanUp();
		return FALSE;
	}


	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FBXRENDERER));


	MSG msg = { 0 };

	SSInput::Get();
	SSFrameInfo::Get();
	g_Renderer.BeginFrame();

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			SSFrameInfo::Get()->ProcessPerFrameBeginEventInternal();
			g_Renderer.PerFrame();
			SSInput::Get()->ProcessInputEndOfFrame();
		}
	}


	g_Renderer.CleanUp();

	SSFrameInfo::Release();
	SSInput::Release();


#ifdef _DEBUG
	CheckRemainingObjects();
	assert(_CrtDumpMemoryLeaks() == false);
	assert(_CrtCheckMemory());
#endif
	return (int)msg.wParam;
}


HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow, RECT WindowSize)
{

	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FBXRENDERER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_FBXRENDERER); // MFC로 창 만들어서 등록
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	// RegisterClass를 통해 화면에 어떤 객체를 올릴지 클래스를 "등록"
	if (!RegisterClassExW(&wcex)) {
		__debugbreak();
		return E_FAIL;
	}


	g_hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	AdjustWindowRect(&WindowSize, WS_OVERLAPPEDWINDOW, FALSE);

	int32 Width = WindowSize.right - WindowSize.left;
	int32 Height = WindowSize.bottom - WindowSize.top;

	// 위에서 RegisterClass를 통해 등록한 클래스를 "생성"해서 윈도우 창 생성
	HWND hWnd = CreateWindow(szWindowClass, szTitle,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, Width, Height
		, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		__debugbreak();
		return E_FAIL;
	}


	ShowWindow(hWnd, nCmdShow);
	// nCmdShow: 윈도우 및 컨트롤의 표시/숨김 여부 설정과 최대화, 최소화 여부 설정


	UpdateWindow(hWnd);

	g_hWnd = hWnd;


	return S_OK;
}




extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;


	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 메뉴 선택을 구문 분석합니다:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_KILLFOCUS:
	case WM_MOUSELEAVE:
	case WM_NCMOUSELEAVE:
		SSInput::Get()->ClearCurInputState();
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		SSFrameInfo::Get()->ProcessWindowResizeEventInternal(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:

		SSInput::Get()->ProcessInputEventForWindowsInternal(hWnd, message, wParam, lParam);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


#ifdef _DEBUG
void CheckRemainingObjects() {
	HMODULE dxgidebugdll = GetModuleHandleW(L"dxgidebug.dll");
	decltype(&DXGIGetDebugInterface) GetDebugInterface = reinterpret_cast<decltype(&DXGIGetDebugInterface)>(GetProcAddress(dxgidebugdll, "DXGIGetDebugInterface"));

	IDXGIDebug* debug;

	GetDebugInterface(IID_PPV_ARGS(&debug));

	OutputDebugStringW(L"Starting Live Direct3D Object Dump:\r\n");
	debug->ReportLiveObjects(DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_DETAIL);
	OutputDebugStringW(L"Completed Live Direct3D Object Dump.\r\n");

	debug->Release();
}
#endif


void AnalyzeCommandLineArguements()
{
	int32 argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);

	for (int32 i = 1; i < argc; i++)
	{
		SS::FixedStringW<100> Key;
		SS::FixedStringW<100> Value;


		utf16* splitStart = wcschr(argv[i], L'=');
		if (splitStart != nullptr)
		{
			uint32 newStrlen = splitStart - argv[i];

			Key = argv[i];
			Key.CutOut(newStrlen);
			Value = (splitStart + 1);
		}
		else
		{
			Key = argv[i];
		}


		if (wcscmp(Key.C_Str(), L"g_exportSSMaterial") == 0)
		{
			if (wcscmp(Value.C_Str(), L"true") == 0)
			{
				SSFBXImporter::g_exportSSMaterial = true;
			}
		}
		else if (wcscmp(Key.C_Str(), L"g_WndRect") == 0)
		{
			swscanf(Value.C_Str(), L"{%d,%d,%d,%d}",
				&g_WndRect.left, &g_WndRect.top, &g_WndRect.right, &g_WndRect.bottom);
		}

	}

}