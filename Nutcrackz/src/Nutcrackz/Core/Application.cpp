#include "nzpch.hpp"
#include "Application.hpp"

#include "Nutcrackz/Core/Log.hpp"
#include "Nutcrackz/Renderer/Renderer.hpp"
#include "Nutcrackz/Scripting/ScriptEngine.hpp"
#include "Nutcrackz/Debug/Profiler.hpp"

#include "Nutcrackz/Input/InputSystemImpl.hpp"

#include "Nutcrackz/Core/Input.hpp"

#include "Input.hpp"
#include "Nutcrackz/Utils/PlatformUtils.hpp"

#include <filesystem>

namespace Nutcrackz {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification, bool maximizedOnLaunch, uint32_t width, uint32_t height, bool fullscreen)
		: m_Specification(specification)
	{
		//NZ_PROFILE_FUNCTION();

		NZ_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		// Set working directory here
		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		m_Window = Window::Create(WindowProps(m_Specification.Name, m_Specification.MaximizedOnLaunch, width, height, fullscreen));

		m_Window->SetEventCallback(NZ_BIND_EVENT_FN(Application::OnEvent));
		//m_Window->SetVSync(false);
		//m_Window->SetVSync(true);

		m_InputSystem = { new InputSystem::Impl() };
		m_InputSystem->Init();

		Renderer::Init();
		ScriptEngine::GetMutable().InitializeHost();

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);

	}

	Application::~Application()
	{
		//NZ_PROFILE_FUNCTION();

		ScriptEngine::GetMutable().ShutdownHost();
		Renderer::Shutdown();
	}

	void Application::PushLayer(Layer* layer)
	{
		//NZ_PROFILE_FUNCTION();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		//NZ_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& e)
	{
		NZ_PROFILE_FUNCTION("Application::OnEvent");
		//NZ_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(NZ_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(NZ_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run()
	{
		//NZ_PROFILE_FUNCTION("Application::Run");

		while (m_Running)
		{
			m_InputSystem->Update();

			Update();
			NZ_PROFILE_MARK_FRAME;
		}

		m_InputSystem->Shutdown();
	}

	void Application::Update()
	{
		//NZ_PROFILE_FUNCTION_COLOR("Application::Update", 0xFF72FA);
		NZ_PROFILE_FUNCTION("Application::Update");

		{
			//NZ_PROFILE_SCOPE_COLOR("Application::Update Scope", 0xFF7200);
			//NZ_PROFILE_SCOPE("Application::Update Scope");

			float time = Time::GetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			Input::TransitionPressedButtons();
			
			if (!m_Minimized)
			{
				{
					//NZ_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(timestep);
				}

				m_ImGuiLayer->ResizeFont(m_Window->getDPISize());

				m_ImGuiLayer->Begin();
				{
					//NZ_PROFILE_SCOPE("LayerStack OnImGuiRender");

					for (Layer* layer : m_LayerStack)
						layer->OnImGuiRender();
				}
				m_ImGuiLayer->End();
			}

			m_Window->OnUpdate();

			Input::ClearReleasedKeys();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		//NZ_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		Update();

		return false;
	}

}
