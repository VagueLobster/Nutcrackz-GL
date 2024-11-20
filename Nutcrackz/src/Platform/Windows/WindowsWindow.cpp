#include "nzpch.hpp"
#include "Platform/Windows/WindowsWindow.hpp"

#include "Nutcrackz/Core/Application.hpp"

#include "Nutcrackz/Core/Input.hpp"

#include "Nutcrackz/Events/ApplicationEvent.hpp"
#include "Nutcrackz/Events/MouseEvent.hpp"
#include "Nutcrackz/Events/KeyEvent.hpp"

#include "Nutcrackz/Project/Project.hpp"

#include "Nutcrackz/Renderer/Renderer.hpp"
#include "Nutcrackz/Renderer/Texture.hpp"

#include "Platform/OpenGL/OpenGLContext.hpp"

#include <stb_image.h>
#include "imgui.h"

namespace Nutcrackz {

	static uint8_t s_GLFWWindowCount = 0;
	static bool m_HasResizedWindow = false;

	static void GLFWErrorCallback(int error, const char* description)
	{
		// This line below always gives errors when pressing on one of the media keys!
		//NZ_CORE_ERROR("GLFW Error ({}): {}", error, description);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		//NZ_PROFILE_FUNCTION();

		Init(props);
	}

	WindowsWindow::~WindowsWindow()
	{
		//NZ_PROFILE_FUNCTION();

		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		//NZ_PROFILE_FUNCTION();

		m_Data.Title = props.Title;

		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		NZ_CORE_INFO("Creating window: {0} ({1}, {2})", props.Title.c_str(), props.Width, props.Height);

		if (s_GLFWWindowCount == 0)
		{
			//NZ_PROFILE_SCOPE("glfwInit");
			int success = glfwInit();
			NZ_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		{			
			if (props.Maximized)
			{
				glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
			}

			//NZ_PROFILE_SCOPE("glfwCreateWindow");

			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			float xscale, yscale;
			glfwGetMonitorContentScale(monitor, &xscale, &yscale);

			NZ_CORE_INFO("X DPI Scale: {0}, Y DPI Scale: {1}", xscale, yscale);

			if (xscale >= 1 || yscale >= 1)
			{
				if (m_XScale != xscale)
				{
					m_HasResizedDPI = false;
					m_XScale = xscale;
				}

				if (!m_HasResizedDPI)
				{
					s_HighDPIScaleFactor = xscale;
					s_HighDPIScaleFactorY = yscale;
					glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

					m_HasResizedDPI = true;
				}
			}
			else
			{
				Shutdown();
			}

#if defined(NZ_DEBUG)
			if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

			if (props.Fullscreen)
				m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), glfwGetPrimaryMonitor(), nullptr); // Borderless fullscreen
			else
				m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr); // Windowed mode

			//SetWindowIcon("Resources/Icons/Editor/Icon.png");

			++s_GLFWWindowCount;
		}

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();

		TracyGpuContext;

		glfwSetWindowUserPointer(m_Window, &m_Data);
		
		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			if (m_HasResizedWindow)
				m_HasResizedWindow = false;

			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);

			if (!m_HasResizedWindow)
				m_HasResizedWindow = true;
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				KeyPressedEvent event(key, 0);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				KeyReleasedEvent event(key);
				data.EventCallback(event);
				break;
			}
			case GLFW_REPEAT:
			{
				KeyPressedEvent event(key, true);
				data.EventCallback(event);
				break;
			}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(keycode);
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				MouseButtonPressedEvent event(button);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				MouseButtonReleasedEvent event(button);
				data.EventCallback(event);
				break;
			}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});

		glfwSetDropCallback(m_Window, [](GLFWwindow* window, int pathCount, const char* paths[])
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			std::vector<std::filesystem::path> filepaths(pathCount);
			for (int i = 0; i < pathCount; i++)
				filepaths[i] = paths[i];

			WindowDropEvent event(std::move(filepaths));
			data.EventCallback(event);
		});

		//SetVSync(true);
		SetVSync(false);
	}

	void WindowsWindow::Shutdown()
	{
		//NZ_PROFILE_FUNCTION();

		glfwDestroyWindow(m_Window);
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void WindowsWindow::OnUpdate()
	{
		//NZ_PROFILE_FUNCTION();

		if (!m_HasResizedGLFWWindow && m_HasResizedWindow)
		{
			ResizeWindow();
			m_HasResizedGLFWWindow = m_HasResizedWindow;
		}

		glfwPollEvents();
		Input::Update();
		m_Context->SwapBuffers();
		TracyGpuCollect;
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		//NZ_PROFILE_FUNCTION();

		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}

	void WindowsWindow::ResizeWindow()
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();

		float xscale, yscale;
		glfwGetMonitorContentScale(monitor, &xscale, &yscale);
		
		if (xscale >= 1 || yscale >= 1)
		{
			if (m_XScale != xscale)
			{
				m_HasResizedDPI = false;
				m_XScale = xscale;
			}

			if (!m_HasResizedDPI)
			{
				s_HighDPIScaleFactor = xscale;
				s_HighDPIScaleFactorY = yscale;
				glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

				m_HasResizedDPI = true;
			}
		}
	}

	void WindowsWindow::ChangeScreenSizeAndRefreshRate(int width, int height, int refreshRate)
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		
		glfwSetWindowSize(m_Window, width, height);

		float xscale, yscale;
		glfwGetMonitorContentScale(monitor, &xscale, &yscale);

		NZ_CORE_ERROR("X Scale DPI: {0}, Y Scale DPI: {1}", xscale, yscale);

		if (xscale >= 1 || yscale >= 1)
		{
			if (m_XScale != xscale)
			{
				m_HasResizedDPI = false;
				m_XScale = xscale;
			}

			if (!m_HasResizedDPI)
			{
				s_HighDPIScaleFactor = xscale;
				s_HighDPIScaleFactorY = yscale;
				glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

				m_HasResizedDPI = true;
			}
		}

		NZ_CORE_ERROR("High DPI Scale: {0}", xscale, yscale);
	}

	std::string& WindowsWindow::GetTitle()
	{
		return m_Data.Title;
	}

	void WindowsWindow::SetTitle(const std::string& title)
	{
		m_Data.Title = title;
		glfwSetWindowTitle(m_Window, title.c_str());
	}

	std::string& WindowsWindow::GetCursor()
	{
		return m_CustomCursorPath;
	}

	void WindowsWindow::SetCursor(const std::string& path, const rtmcpp::Vec2& hotSpot)
	{
		std::filesystem::path cursorPath = Project::GetActiveProjectDirectory() / path.c_str();

		GLFWimage image;

		stbi_set_flip_vertically_on_load(0);
		image.pixels = stbi_load(cursorPath.string().c_str(), &image.width, &image.height, 0, STBI_rgb_alpha);

		if (image.pixels)
		{
			m_CustomCursorPath = cursorPath.string().c_str();
			GLFWcursor* cursor = glfwCreateCursor(&image, static_cast<int>(hotSpot.X), static_cast<int>(hotSpot.Y));
			glfwSetCursor(m_Window, cursor);
			m_CursorIsStandardArrow = false;
			m_CursorIsStandardHand = false;

			m_CursorIsCustom = true;
		}

		stbi_image_free(image.pixels);
	}

	void WindowsWindow::SetCursorNormal()
	{
		if (m_CursorIsStandardHand || m_CursorIsCustom)
		{
			m_CustomCursorPath = "";
			glfwSetCursor(m_Window, NULL);
			m_CursorIsStandardHand = false;
			m_CursorIsCustom = false;

			m_CursorIsStandardArrow = true;
		}
	}

	void WindowsWindow::SetCursorHand()
	{
		if (m_CursorIsStandardArrow || m_CursorIsCustom)
		{
			m_CustomCursorPath = "";
			GLFWcursor* cursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
			glfwSetCursor(m_Window, cursor);
			m_CursorIsStandardArrow = false;
			m_CursorIsCustom = false;

			m_CursorIsStandardHand = true;
		}
	}

	void WindowsWindow::ResetCursor()
	{
		m_CustomCursorPath = "";
		glfwSetCursor(m_Window, NULL);

		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
	}

	void WindowsWindow::SetWindowIcon(const std::string& path)
	{
		// This code is inspired by Kenny Tutorials' code on StackOverflow:
		// https://stackoverflow.com/questions/58631465/c-glfw-setwindowicon

		GLFWimage imageData[2];

		//NZ_PROFILE_SCOPE("stbi_load - WindowsWindow::SetWindowIcon(const std::string&)");
		imageData[0].pixels = stbi_load(path.c_str(), &imageData[0].width, &imageData[0].height, 0, STBI_rgb_alpha);
		imageData[1].pixels = stbi_load(path.c_str(), &imageData[1].width, &imageData[1].height, 0, STBI_rgb_alpha);

		glfwSetWindowIcon(m_Window, 1, imageData);

		NZ_CORE_ASSERT(imageData[0].pixels, "Failed to load image 1!");
		NZ_CORE_ASSERT(imageData[1].pixels, "Failed to load image 2!");

		stbi_image_free(imageData[0].pixels);
		stbi_image_free(imageData[1].pixels);
	}

}
