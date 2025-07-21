#include <Windows.h>

#include "Debugging/Logger.h"

#pragma warning(disable: 28251) // Disable warning about inconsistent SAL annotations

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	LOG("Main", Verbose, "Hello, World! %i %f %s", 1, 2.3f, "4");

	return 0;
}
