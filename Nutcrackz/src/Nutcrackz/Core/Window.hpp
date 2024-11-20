#pragma once

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Events/Event.hpp"

//#include <glm/glm.hpp>
#include "rtmcpp/Common.hpp"
//#include "rtmcpp/Transforms.hpp"

struct GLFWwindow;

namespace Nutcrackz {

	struct WindowProps
	{
		std::string Title;
		bool Maximized;
		bool Fullscreen;
		uint32_t Width;
		uint32_t Height;

		WindowProps(const std::string& title = "Nutcrackz Editor",
			bool maximizedOnLaunch = false,
			uint32_t width = 1600,
			uint32_t height = 900,
			bool fullscreen = false)
			: Title(title), Maximized(maximizedOnLaunch), Width(width), Height(height), Fullscreen(fullscreen)
		{
		}
	};

	// Interface representing a desktop system based Window
	class Window
	{
	public:
		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;
		virtual void ResizeWindow() = 0;
		virtual bool HasResizedDPIScale() = 0;
		virtual bool HasResizedWindow() = 0;
		virtual float getDPISize() = 0;

		virtual std::string& GetTitle() = 0;
		virtual void SetTitle(const std::string& title) = 0;

		virtual std::string& GetCursor() = 0;
		virtual void SetCursor(const std::string& path, const rtmcpp::Vec2& hotSpot) = 0;
		virtual void SetCursorNormal() = 0;
		virtual void SetCursorHand() = 0;
		virtual void ResetCursor() = 0;

		virtual void* GetNativeWindow() const = 0;
		virtual rtmcpp::Vec2 GetWindowPosition() = 0;
		virtual void ChangeScreenSizeAndRefreshRate(int width, int height, int refreshRate) = 0;

		static std::unique_ptr<Window> Create(const WindowProps& props = WindowProps());

	public:
		static float s_HighDPIScaleFactor;
		static float s_HighDPIScaleFactorY;
	};

}
