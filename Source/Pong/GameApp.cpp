#include "GameApp.h"
#include "Debugging/Logger.h"

GameApp* g_pApp = nullptr;

GameApp::GameApp()
{
	g_pApp		= this;

	m_hInst		= GetModuleHandle(nullptr);

	SetRect(&m_rcclient, 0, 0, 640, 480);
	m_hwnd		= nullptr;
}

bool GameApp::Initialize()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MsgProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInst;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"PongWindowClass";
	if (!RegisterClass(&wc))
	{
		return false;
	}

	AdjustWindowRect(&m_rcclient, WS_OVERLAPPEDWINDOW, FALSE);
	m_hwnd = CreateWindow(L"PongWindowClass", L"Pong", WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, CW_USEDEFAULT, m_rcclient.right - m_rcclient.left, m_rcclient.bottom - m_rcclient.top, 
		nullptr, nullptr, m_hInst, nullptr);
	if (m_hwnd == nullptr)
	{
		return false;
	}

	return true;
}

void GameApp::Run()
{
	if (!IsWindowVisible(m_hwnd))
	{
		ShowWindow(m_hwnd, SW_SHOW);
	}

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

LRESULT GameApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}
