#pragma once

#include <Windows.h>
#include <wrl.h>
#include <dxgi.h>
#include <d3d11.h>

class GameApp
{
    HINSTANCE   m_hInst;

    RECT        m_rcclient;
    HWND        m_hwnd;

public:
    GameApp();

    bool Initialize();
    void Run();

    static LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

extern GameApp* g_pApp;
