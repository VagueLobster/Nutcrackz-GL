#include "nzpch.hpp"
#include "ConsoleLog.hpp"

#include <filesystem>

namespace Nutcrackz {

	RefPtr<ConsoleLog> ConsoleLog::s_CoreLoggerConsole;
	RefPtr<ConsoleLog> ConsoleLog::s_ClientLoggerConsole;
	std::vector<std::string> ConsoleLog::s_Buffer;
	std::vector<std::pair<Severity, std::string>> ConsoleLog::s_Messages;

	bool ConsoleLog::s_LogToFile = true;
	bool ConsoleLog::s_LogToConsole = true;
	bool ConsoleLog::s_LogToEditorConsole = true;

	const char* ConsoleLog::s_PreviousFile = "Logs/NutcrackzPrevLog.tlog";
	const char* ConsoleLog::s_CurrentFile = "Logs/NutcrackzLog.tlog";

	ConsoleLog::ConsoleLog(const char* name)
		: m_Name(name)
	{
	}

	void ConsoleLog::LogMsg(Severity severity, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		ConsoleLog::LogMsg(m_Name, severity, format, args);
		va_end(args);
	}

	void ConsoleLog::LogTrace(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		ConsoleLog::LogMsg(m_Name, Severity::Trace, format, args);
		va_end(args);
	}

	void ConsoleLog::LogInfo(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		ConsoleLog::LogMsg(m_Name, Severity::Info, format, args);
		va_end(args);
	}

	void ConsoleLog::LogWarning(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		ConsoleLog::LogMsg(m_Name, Severity::Warning, format, args);
		va_end(args);
	}

	void ConsoleLog::LogError(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		ConsoleLog::LogMsg(m_Name, Severity::Error, format, args);
		va_end(args);
	}

	void ConsoleLog::LogCritical(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		ConsoleLog::LogMsg(m_Name, Severity::Critical, format, args);
		va_end(args);
	}

	void ConsoleLog::Init()
	{
		s_CoreLoggerConsole = RefPtr<ConsoleLog>::Create("NUTCRACKZ");
		s_ClientLoggerConsole = RefPtr<ConsoleLog>::Create("APP");

		if (std::filesystem::exists(ConsoleLog::s_CurrentFile))
		{
			if (std::filesystem::exists(ConsoleLog::s_PreviousFile))
				std::filesystem::remove(ConsoleLog::s_PreviousFile);

			if (rename(ConsoleLog::s_CurrentFile, ConsoleLog::s_PreviousFile))
				ConsoleLog("Logger").LogMsg(Severity::Warning, "Failed to rename log file %s to %s", ConsoleLog::s_CurrentFile, ConsoleLog::s_PreviousFile);
		}
	}

	void ConsoleLog::LogMsg(const char* name, Severity severity, const char* format, va_list args)
	{
		uint32_t length = vsnprintf(nullptr, 0, format, args) + 1;
		char* buf = new char[length];
		vsnprintf(buf, length, format, args);

		std::string message(buf);
		delete[] buf;

		std::vector<std::string> messages;

		uint32_t lastIndex = 0;
		for (uint32_t i = 0; i < message.length(); i++)
		{
			if (message[i] == '\n')
			{
				messages.push_back(message.substr(lastIndex, i - lastIndex));
				lastIndex = i + 1;
			}
			else if (i == message.length() - 1)
				messages.push_back(message.substr(lastIndex));
		}

		for (std::string msg : messages)
		{
			std::string systemConsoleMsg = "";
			std::string editorConsoleMsg = "";

			constexpr uint32_t timeBufferSize = 16;
			std::time_t    currentTime = std::time(nullptr);
			char           timeBuffer[timeBufferSize];

			if (ConsoleLog::s_LogToConsole)
				systemConsoleMsg += std::string(ConsoleLog::GetSeverityConsoleColor(severity)) + "[" + std::string(name) + "]";

			if (std::strftime(timeBuffer, timeBufferSize, "[%H:%M:%S]", std::localtime(&currentTime)))
			{
				if (ConsoleLog::s_LogToConsole)
					systemConsoleMsg += timeBuffer;
				if (ConsoleLog::s_LogToEditorConsole)
					editorConsoleMsg += timeBuffer;
			}

			if (ConsoleLog::s_LogToConsole)
				systemConsoleMsg += " " + std::string(ConsoleLog::GetSeverityID(severity)) + ": " + msg + "\033[0m " + "\n";

			if (ConsoleLog::s_LogToConsole)
				printf("%s", systemConsoleMsg.c_str());

			//ImGui Console
			if (ConsoleLog::s_LogToEditorConsole)
			{
				editorConsoleMsg += " " + std::string(ConsoleLog::GetSeverityID(severity)) + ": " + msg;
				s_Messages.emplace_back(std::pair<Severity, std::string>(severity, editorConsoleMsg));
			}

		}

		if (ConsoleLog::s_LogToFile)
			if (ConsoleLog::s_Buffer.size() > ConsoleLog::GetSeverityMaxBufferCount(severity))
				Flush();
	}

	void ConsoleLog::Shutdown()
	{
		Flush();
	}

	void ConsoleLog::Flush()
	{
		if (!ConsoleLog::s_LogToFile)
			return;

		std::filesystem::path filepath{ ConsoleLog::s_CurrentFile };
		std::filesystem::create_directories(filepath.parent_path());

		FILE* file = fopen(ConsoleLog::s_CurrentFile, "a");
		if (file)
		{
			for (auto message : ConsoleLog::s_Buffer)
				fwrite(message.c_str(), sizeof(char), message.length(), file);
			fclose(file);
			ConsoleLog::s_Buffer.clear();
		}
		else
			ConsoleLog::s_LogToFile = false;
	}

	void ConsoleLog::Clear()
	{
		if (ConsoleLog::s_Messages.size() > 0)
		{
			for (size_t i = 0; i < ConsoleLog::s_Messages.size(); i++)
				ConsoleLog::s_Messages.erase(ConsoleLog::s_Messages.begin(), ConsoleLog::s_Messages.end());
		}
	}

	void ConsoleLog::AutoClear()
	{
		if (ConsoleLog::s_Messages.size() > 19)
		{
			for (size_t i = 0; i < ConsoleLog::s_Messages.size() - 20; i++)
				ConsoleLog::s_Messages.erase(ConsoleLog::s_Messages.begin() + i);
		}
	}

	size_t ConsoleLog::SizeOfMessages()
	{
		return ConsoleLog::s_Messages.size();
	}

	uint32_t ConsoleLog::GetSeverityMaxBufferCount(Severity severity)
	{
		switch (severity)
		{
		case Severity::Trace:
			return 100;
		case Severity::Info:
			return 100;
		case Severity::Warning:
			return 10;
		case Severity::Error:
			return 0;
		case Severity::Critical:
			return 0;
		}
		return 0;
	}

	const char* ConsoleLog::GetSeverityID(Severity severity)
	{
		switch (severity)
		{
		case Severity::Trace:
			return "TRACE";
		case Severity::Info:
			return "INFO";
		case Severity::Warning:
			return "WARNING";
		case Severity::Error:
			return "ERROR";
		case  Severity::Critical:
			return "CRITICAL";
		}

		return "Unknown Severity";
	}

	const char* ConsoleLog::GetSeverityConsoleColor(Severity severity)
	{
		/*
		* Console Colors https://stackoverflow.com/questions/4053837
		* Name            FG  BG
		* Black           30  40
		* Red             31  41
		* Green           32  42
		* Yellow          33  43
		* Blue            34  44
		* Magenta         35  45
		* Cyan            36  46
		* White           37  47
		* Bright Black    90  100
		* Bright Red      91  101
		* Bright Green    92  102
		* Bright Yellow   93  103
		* Bright Blue     94  104
		* Bright Magenta  95  105
		* Bright Cyan     96  106
		* Bright White    97  107
		*/
		switch (severity)
		{
		case Severity::Trace:
			return "\033[0;97m";
		case Severity::Info:
			return "\033[0;92m";
		case Severity::Warning:
			return "\033[0;93m";
		case Severity::Error:
			return "\033[0;91m";
		case Severity::Critical:
			return "\033[0;35m";
		}
		return "\033[0;97m";
	}

 }