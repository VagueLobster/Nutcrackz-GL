#pragma once

//#define NZ_ENABLE_PROFILING !NZ_DIST
#define NZ_ENABLE_PROFILING 1

#if NZ_ENABLE_PROFILING
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"
//#include "tracy/TracyVulkan.hpp"
#endif

#if NZ_ENABLE_PROFILING
#define NZ_PROFILE_MARK_FRAME					FrameMark
#define NZ_PROFILE_FUNCTION(...)				ZoneScopedN(__VA_ARGS__)
#define NZ_PROFILE_FUNCTION_COLOR(name, ...)	ZoneScopedNC(name, __VA_ARGS__) // Color is in hexadecimal
#define NZ_PROFILE_SCOPE(...)					NZ_PROFILE_FUNCTION(__VA_ARGS__)
#define NZ_PROFILE_SCOPE_COLOR(name, ...)		NZ_PROFILE_FUNCTION_COLOR(name, __VA_ARGS__)
#define NZ_PROFILE_GPU_SCOPE(...)				TracyGpuZone(__VA_ARGS__)

//#define NZ_PROFILE_ALLOC(p, size)		TracyCAllocS(p, size, 12)
//#define NZ_PROFILE_FREE(p)			TracyCFreeS(p, 12)
#else
#define NZ_PROFILE_MARK_FRAME(...)
#define NZ_PROFILE_FUNCTION(...)
#define NZ_PROFILE_FUNCTION_COLOR(name, ...)
#define NZ_PROFILE_SCOPE(...)
#define NZ_PROFILE_SCOPE_COLOR(name, ...)
#define NZ_PROFILE_GPU_SCOPE(...)
#endif
