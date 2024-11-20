#include "nzpch.hpp"
#include "Nutcrackz/Core/Window.hpp"

#ifdef NZ_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.hpp"
#endif

namespace Nutcrackz
{
	float Window::s_HighDPIScaleFactor = 1.0f;
	float Window::s_HighDPIScaleFactorY = 1.0f;

	std::unique_ptr<Window> Window::Create(const WindowProps& props)
	{
#ifdef NZ_PLATFORM_WINDOWS
		return std::make_unique<WindowsWindow>(props);
#else
		NZ_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
#endif
	}

}
