#pragma once

#include "Core.hpp"

//#define GLM_ENABLE_EXPERIMENTAL
//#include "glm/gtx/string_cast.hpp"

#include "rtmcpp/Common.hpp"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

#include <map>

#define NZ_ASSERT_MESSAGE_BOX (!NZ_DIST && NZ_PLATFORM_WINDOWS)

#ifdef NZ_ASSERT_MESSAGE_BOX
#ifdef NZ_PLATFORM_WINDOWS
#include <Windows.h>
#endif
#endif

namespace Nutcrackz {

	class Log
	{
	public:
		enum class Type : uint8_t
		{
			Core = 0, Client = 1
		};
		enum class Level : uint8_t
		{
			Trace = 0, Info, Warn, Error, Fatal
		};
		struct TagDetails
		{
			bool Enabled = true;
			Level LevelFilter = Level::Trace;
		};

	public:
		static void Init();
		
		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

		static bool HasTag(const std::string& tag) { return s_EnabledTags.find(tag) != s_EnabledTags.end(); }
		static std::map<std::string, TagDetails>& EnabledTags() { return s_EnabledTags; }

		template<typename... Args>
		static void PrintMessage(Log::Type type, Log::Level level, std::string_view tag, Args&&... args);

		template<typename... Args>
		static void PrintAssertMessage(Log::Type type, std::string_view prefix, Args&&... args);

	public:
		// Enum utils
		static const char* LevelToString(Level level)
		{
			switch (level)
			{
			case Level::Trace: return "Trace";
			case Level::Info:  return "Info";
			case Level::Warn:  return "Warn";
			case Level::Error: return "Error";
			case Level::Fatal: return "Fatal";
			}
			return "";
		}
		static Level LevelFromString(std::string_view string)
		{
			if (string == "Trace") return Level::Trace;
			if (string == "Info")  return Level::Info;
			if (string == "Warn")  return Level::Warn;
			if (string == "Error") return Level::Error;
			if (string == "Fatal") return Level::Fatal;

			return Level::Trace;
		}

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

		inline static std::map<std::string, TagDetails> s_EnabledTags;
	};

}

/*template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}
template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}*/

/*template<typename OStream>
inline OStream& operator<<(OStream& os, const rtmcpp::Vec2& vector)
{
	return os << glm::to_string(vector);
}
template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}*/

// Core logging
#define NZ_CORE_TRACE_TAG(tag, ...) ::Nutcrackz::Log::PrintMessage(::Nutcrackz::Log::Type::Core, ::Nutcrackz::Log::Level::Trace, tag, __VA_ARGS__)
#define NZ_CORE_INFO_TAG(tag, ...)  ::Nutcrackz::Log::PrintMessage(::Nutcrackz::Log::Type::Core, ::Nutcrackz::Log::Level::Info, tag, __VA_ARGS__)
#define NZ_CORE_WARN_TAG(tag, ...)  ::Nutcrackz::Log::PrintMessage(::Nutcrackz::Log::Type::Core, ::Nutcrackz::Log::Level::Warn, tag, __VA_ARGS__)
#define NZ_CORE_ERROR_TAG(tag, ...) ::Nutcrackz::Log::PrintMessage(::Nutcrackz::Log::Type::Core, ::Nutcrackz::Log::Level::Error, tag, __VA_ARGS__)
#define NZ_CORE_FATAL_TAG(tag, ...) ::Nutcrackz::Log::PrintMessage(::Nutcrackz::Log::Type::Core, ::Nutcrackz::Log::Level::Fatal, tag, __VA_ARGS__)

// Client logging
#define NZ_TRACE_TAG(tag, ...) ::Nutcrackz::Log::PrintMessage(::Nutcrackz::Log::Type::Client, ::Nutcrackz::Log::Level::Trace, tag, __VA_ARGS__)
#define NZ_INFO_TAG(tag, ...)  ::Nutcrackz::Log::PrintMessage(::Nutcrackz::Log::Type::Client, ::Nutcrackz::Log::Level::Info, tag, __VA_ARGS__)
#define NZ_WARN_TAG(tag, ...)  ::Nutcrackz::Log::PrintMessage(::Nutcrackz::Log::Type::Client, ::Nutcrackz::Log::Level::Warn, tag, __VA_ARGS__)
#define NZ_ERROR_TAG(tag, ...) ::Nutcrackz::Log::PrintMessage(::Nutcrackz::Log::Type::Client, ::Nutcrackz::Log::Level::Error, tag, __VA_ARGS__)
#define NZ_FATAL_TAG(tag, ...) ::Nutcrackz::Log::PrintMessage(::Nutcrackz::Log::Type::Client, ::Nutcrackz::Log::Level::Fatal, tag, __VA_ARGS__)

//Core log macros
#define NZ_CORE_TRACE(...)    ::Nutcrackz::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define NZ_CORE_INFO(...)     ::Nutcrackz::Log::GetCoreLogger()->info(__VA_ARGS__)
#define NZ_CORE_WARN(...)     ::Nutcrackz::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define NZ_CORE_ERROR(...)    ::Nutcrackz::Log::GetCoreLogger()->error(__VA_ARGS__)
#define NZ_CORE_CRITICAL(...) ::Nutcrackz::Log::GetCoreLogger()->critical(__VA_ARGS__)

//Client log macros
#define NZ_TRACE(...)         ::Nutcrackz::Log::GetClientLogger()->trace(__VA_ARGS__)
#define NZ_INFO(...)          ::Nutcrackz::Log::GetClientLogger()->info(__VA_ARGS__)
#define NZ_WARN(...)          ::Nutcrackz::Log::GetClientLogger()->warn(__VA_ARGS__)
#define NZ_ERROR(...)         ::Nutcrackz::Log::GetClientLogger()->error(__VA_ARGS__)
#define NZ_CRITICAL(...)      ::Nutcrackz::Log::GetClientLogger()->critical(__VA_ARGS__)

namespace Nutcrackz {

	template<typename... Args>
	void Log::PrintMessage(Log::Type type, Log::Level level, std::string_view tag, Args&&... args)
	{
		auto detail = s_EnabledTags[std::string(tag)];
		if (detail.Enabled && detail.LevelFilter <= level)
		{
			auto logger = (type == Type::Core) ? GetCoreLogger() : GetClientLogger();
			std::string logString = tag.empty() ? "{0}{1}" : "[{0}] {1}";
			switch (level)
			{
			case Level::Trace:
				logger->trace(logString, tag, fmt::format(std::forward<Args>(args)...));
				break;
			case Level::Info:
				logger->info(logString, tag, fmt::format(std::forward<Args>(args)...));
				break;
			case Level::Warn:
				logger->warn(logString, tag, fmt::format(std::forward<Args>(args)...));
				break;
			case Level::Error:
				logger->error(logString, tag, fmt::format(std::forward<Args>(args)...));
				break;
			case Level::Fatal:
				logger->critical(logString, tag, fmt::format(std::forward<Args>(args)...));
				break;
			}
		}
	}


	template<typename... Args>
	void Log::PrintAssertMessage(Log::Type type, std::string_view prefix, Args&&... args)
	{
		auto logger = (type == Type::Core) ? GetCoreLogger() : GetClientLogger();
		logger->error("{0}: {1}", prefix, fmt::format(std::forward<Args>(args)...));

#ifdef NZ_ASSERT_MESSAGE_BOX
		std::string message = fmt::format(std::forward<Args>(args)...);
		MessageBoxA(nullptr, message.c_str(), "Nutcrackz Assert", MB_OK | MB_ICONERROR);
#endif
	}

	template<>
	inline void Log::PrintAssertMessage(Log::Type type, std::string_view prefix)
	{
		auto logger = (type == Type::Core) ? GetCoreLogger() : GetClientLogger();
		logger->error("{0}", prefix);
#ifdef NZ_ASSERT_MESSAGE_BOX
		MessageBoxA(nullptr, "No message :(", "Nutcrackz Assert", MB_OK | MB_ICONERROR);
#endif
	}
}