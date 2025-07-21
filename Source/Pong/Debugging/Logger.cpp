#include <Windows.h>
#include <cstdio>
#include <cstdarg>
#include "Logger.h"

namespace Logger
{
	void Log(const char* category, LogVerbosityLevel verbosityLevel, const char* message, ...)
	{
		char buffer[2048];
		int i;

		i = sprintf_s(buffer, 2048, "%s: ", category);

		const char* logVerbosityLevels[] = { "Fatal", "Error", "Warning", "Info", "Verbose" };
		i += sprintf_s(buffer + i, 2048 - i, "%s: ", logVerbosityLevels[static_cast<int>(verbosityLevel)]);

		va_list args;
		va_start(args, message);
		i += vsprintf_s(buffer + i, 2048 - i, message, args);
		va_end(args);

		i += sprintf_s(buffer + i, 2048 - i, "\n");

		OutputDebugStringA(buffer);
	}
};
