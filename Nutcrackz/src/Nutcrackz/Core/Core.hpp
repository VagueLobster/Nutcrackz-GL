#pragma once

#include <memory>

#ifndef NZ_PLATFORM_WINDOWS
#define NZ_PLATFORM_WINDOWS
#endif

#ifdef NZ_PLATFORM_WINDOWS
#define NZ_DEBUGBREAK() __debugbreak()
#endif

#ifdef NZ_DEBUG
	#define NZ_ENABLE_ASSERTS
#endif

#ifndef NZ_DIST
	#define NZ_ENABLE_VERIFY
#endif

#define NZ_EXPAND_MACRO(x) x
#define NZ_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define NZ_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#define NutcrackzUnused(x) (void)x

namespace Nutcrackz {

	template<typename T>
	constexpr T AlignUp2(T value, uint64_t align)
	{
		return reinterpret_cast<T>((reinterpret_cast<uint64_t>(value) + align - 1) & ~(align - 1));
	}

	template<typename T>
	struct ScopeExit
	{
		T Func;

		ScopeExit(T&& func) noexcept
			: Func(std::move(func))
		{
		}

		ScopeExit(const ScopeExit&) = delete;
		ScopeExit& operator=(const ScopeExit&) = delete;
		ScopeExit(ScopeExit&&) noexcept = delete;
		ScopeExit& operator=(ScopeExit&&) noexcept = delete;

		~ScopeExit()
		{
			Func();
		}
	};

	using byte = uint8_t;

	template<typename From, typename To>
	concept CastableTo = requires { static_cast<To>(std::declval<From>()); };

	using float32_t = float;
	using float64_t = double;

	constexpr float32_t operator""_f32(long double value)
	{
		return static_cast<float32_t>(value);
	}

	constexpr float64_t operator""_f64(long double value)
	{
		return static_cast<float64_t>(value);
	}

}

#include "Nutcrackz/Core/Log.hpp"
#include "Nutcrackz/Core/Assert.hpp"
