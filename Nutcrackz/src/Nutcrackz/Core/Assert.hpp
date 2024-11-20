#pragma once

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Core/Log.hpp"
#include <filesystem>

#ifdef NZ_ENABLE_ASSERTS

// Alternatively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
#define NZ_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { NZ##type##ERROR(msg, __VA_ARGS__); NZ_DEBUGBREAK(); } }
#define NZ_INTERNAL_ASSERT_WITH_MSG(type, check, ...) NZ_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
#define NZ_INTERNAL_ASSERT_NO_MSG(type, check) NZ_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", NZ_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

#define NZ_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define NZ_INTERNAL_ASSERT_GET_MACRO(...) NZ_EXPAND_MACRO( NZ_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, NZ_INTERNAL_ASSERT_WITH_MSG, NZ_INTERNAL_ASSERT_NO_MSG) )

// Currently accepts at least the condition and one additional parameter (the message) being optional
#define NZ_ASSERT(...) NZ_EXPAND_MACRO( NZ_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
#define NZ_CORE_ASSERT(...) NZ_EXPAND_MACRO( NZ_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
#define NZ_ASSERT(...)
#define NZ_CORE_ASSERT(...)
#endif

#ifdef NZ_ENABLE_VERIFY

// Alternatively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
#define NZ_INTERNAL_VERIFY_IMPL(type, check, msg, ...) { if(!(check)) { NZ##type##ERROR(msg, __VA_ARGS__); NZ_DEBUGBREAK(); } }
#define NZ_INTERNAL_VERIFY_WITH_MSG(type, check, ...) NZ_INTERNAL_VERIFY_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
#define NZ_INTERNAL_VERIFY_NO_MSG(type, check) NZ_INTERNAL_VERIFY_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", NZ_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

#define NZ_INTERNAL_VERIFY_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define NZ_INTERNAL_VERIFY_GET_MACRO(...) NZ_EXPAND_MACRO( NZ_INTERNAL_VERIFY_GET_MACRO_NAME(__VA_ARGS__, NZ_INTERNAL_VERIFY_WITH_MSG, NZ_INTERNAL_VERIFY_NO_MSG) )

// Currently accepts at least the condition and one additional parameter (the message) being optional
#define NZ_VERIFY(...) NZ_EXPAND_MACRO( NZ_INTERNAL_VERIFY_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
#define NZ_CORE_VERIFY(...) NZ_EXPAND_MACRO( NZ_INTERNAL_VERIFY_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
#define NZ_VERIFY(...)
#define NZ_CORE_VERIFY(...)
#endif
