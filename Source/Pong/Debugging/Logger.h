#pragma once

namespace Logger
{
	enum class LogVerbosityLevel
	{
		Fatal,
		Error,
		Warning,
		Info,
		Verbose
	};

	void Log(const char* category, LogVerbosityLevel verbosityLevel, const char* message, ...);
};

#define LOG(category, verbosityLevel, message, ...) \
	::Logger::Log(category, ::Logger::LogVerbosityLevel::verbosityLevel, message, ##__VA_ARGS__)
