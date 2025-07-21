#include <Windows.h>

#pragma warning(disable: 28251) // Disable warning about inconsistent SAL annotations

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	return MessageBox(nullptr, L"Hello, World!", L" ", MB_OK);
}
