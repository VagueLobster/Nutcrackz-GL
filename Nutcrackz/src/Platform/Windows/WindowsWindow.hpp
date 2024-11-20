#pragma once

#include "Nutcrackz/Core/Window.hpp"
#include "Nutcrackz/Renderer/GraphicsContext.hpp"

#include <GLFW/glfw3.h>

namespace Nutcrackz {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void OnUpdate() override;

		unsigned int GetWidth() const override { return m_Data.Width; }
		unsigned int GetHeight() const override { return m_Data.Height; }

		// Window attributes
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;
		void ResizeWindow() override;
		bool HasResizedDPIScale() override { return m_HasResizedDPI; }
		bool HasResizedWindow() override { return m_HasResizedGLFWWindow; }
		float getDPISize() override { return m_XScale; }

		std::string& GetTitle() override;
		void SetTitle(const std::string& title) override;

		std::string& GetCursor() override;
		void SetCursor(const std::string& path, const rtmcpp::Vec2& hotSpot) override;
		void SetCursorNormal() override;
		void SetCursorHand() override;
		void ResetCursor() override;

		virtual void* GetNativeWindow() const { return m_Window; }
		rtmcpp::Vec2 GetWindowPosition() override
		{
			int width = 0;
			int height = 0;

			glfwGetWindowPos(m_Window, &width, &height);
			return rtmcpp::Vec2(static_cast<float>(width), static_cast<float>(height));
		}
		void ChangeScreenSizeAndRefreshRate(int width, int height, int refreshRate) override;

	private:
		void SetWindowIcon(const std::string& path);

		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;
		std::unique_ptr<GraphicsContext> m_Context;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;

		std::string m_CustomCursorPath = "";

		bool m_HasResizedDPI = true;
		bool m_HasResizedGLFWWindow = false;
		float m_XScale;

		bool m_CursorIsStandardArrow = true;
		bool m_CursorIsStandardHand = false;
		bool m_CursorIsCustom = false;
	};

}
