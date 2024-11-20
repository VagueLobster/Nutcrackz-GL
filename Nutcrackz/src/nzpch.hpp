#pragma once

//#ifndef NZ_PLATFORM_WINDOWS
//#define NZ_PLATFORM_WINDOWS
//#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Core/Buffer.hpp"

#include "Nutcrackz/Core/Log.hpp"
#include "Nutcrackz/Core/ConsoleLog.hpp"

//#include "Nutcrackz/Debug/Instrumentor.hpp"
#include "Nutcrackz/Debug/Profiler.hpp"
