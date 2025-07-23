#include <Windows.h>
#include <crtdbg.h>
#include "Debugging/Logger.h"
#include "GameApp.h"

#pragma warning(disable: 28251) // Disable warning about inconsistent SAL annotations

GameApp g_gameApp;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
#ifdef _DEBUG
    int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;					

	_CrtSetDbgFlag(tmpDbgFlag);
#endif

    if (g_pApp->Initialize())
        g_pApp->Run();

    g_pApp->Uninitialize();

    return 0;
}
