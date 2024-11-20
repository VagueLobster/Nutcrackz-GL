#pragma once

#include "Nutcrackz/Core/Ref.hpp"

#include <string>
#include <vector>

namespace Nutcrackz {

	enum class Severity
	{
		Trace,
		Info,
		Warning,
		Error,
		Critical
	};

	class ConsoleLog : public RefCounted
	{
	public:
		ConsoleLog(const char* name);

		void LogMsg(Severity severity, const char* format, ...);
		void LogTrace(const char* format, ...);
		void LogInfo(const char* format, ...);
		void LogWarning(const char* format, ...);
		void LogError(const char* format, ...);
		void LogCritical(const char* format, ...);
		
		static RefPtr<ConsoleLog>& GetCoreLoggerConsole() { return s_CoreLoggerConsole; }
		static RefPtr<ConsoleLog>& GetClientLoggerConsole() { return s_ClientLoggerConsole; }

		static void Init();
		static void Shutdown();
		static void Flush();
		static void Clear();
		static void AutoClear();
		static size_t SizeOfMessages();

	private:
		static const char* GetSeverityID(Severity severity);
		static const char* GetSeverityConsoleColor(Severity severity);
		static uint32_t GetSeverityMaxBufferCount(Severity severity);
		static void LogMsg(const char* name, Severity severity, const char* format, va_list args);

	private:
		const char* m_Name;

		static std::vector<std::string> s_Buffer;

		static std::vector<std::pair<Severity, std::string>> s_Messages;

		static bool s_LogToFile;
		static bool s_LogToConsole;
		static bool s_LogToEditorConsole;

		static const char* s_PreviousFile;
		static const char* s_CurrentFile;

		static RefPtr<ConsoleLog> s_CoreLoggerConsole;
		static RefPtr<ConsoleLog> s_ClientLoggerConsole;

		friend class LogPanel;
	};

}

//Core log macros
#define NZ_CONSOLE_LOG_CORE_TRACE(...)    ::Nutcrackz::ConsoleLog::GetCoreLoggerConsole()->LogTrace(__VA_ARGS__)
#define NZ_CONSOLE_LOG_CORE_INFO(...)     ::Nutcrackz::ConsoleLog::GetCoreLoggerConsole()->LogInfo(__VA_ARGS__)
#define NZ_CONSOLE_LOG_CORE_WARN(...)     ::Nutcrackz::ConsoleLog::GetCoreLoggerConsole()->LogWarning(__VA_ARGS__)
#define NZ_CONSOLE_LOG_CORE_ERROR(...)    ::Nutcrackz::ConsoleLog::GetCoreLoggerConsole()->LogError(__VA_ARGS__)
#define NZ_CONSOLE_LOG_CORE_CRITICAL(...) ::Nutcrackz::ConsoleLog::GetCoreLoggerConsole()->LogCritical(__VA_ARGS__)

//Client log macros
#define NZ_CONSOLE_LOG_TRACE(...)         ::Nutcrackz::ConsoleLog::GetClientLoggerConsole()->LogTrace(__VA_ARGS__)
#define NZ_CONSOLE_LOG_INFO(...)          ::Nutcrackz::ConsoleLog::GetClientLoggerConsole()->LogInfo(__VA_ARGS__)
#define NZ_CONSOLE_LOG_WARN(...)          ::Nutcrackz::ConsoleLog::GetClientLoggerConsole()->LogWarning(__VA_ARGS__)
#define NZ_CONSOLE_LOG_ERROR(...)         ::Nutcrackz::ConsoleLog::GetClientLoggerConsole()->LogError(__VA_ARGS__)
#define NZ_CONSOLE_LOG_CRITICAL(...)      ::Nutcrackz::ConsoleLog::GetClientLoggerConsole()->LogCritical(__VA_ARGS__)