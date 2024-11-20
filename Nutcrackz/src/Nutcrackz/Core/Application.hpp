#pragma once

#include "Nutcrackz/Core/Core.hpp"

#include "Nutcrackz/Core/Window.hpp"
#include "Nutcrackz/Core/LayerStack.hpp"
#include "Nutcrackz/Events/Event.hpp"
#include "Nutcrackz/Events/ApplicationEvent.hpp"

#include "Nutcrackz/Input/InputSystem.hpp"
#include "Nutcrackz/Input/InputAction.hpp"
#include "Nutcrackz/Input/InputCodes.hpp"

#include "Nutcrackz/Core/Timestep.hpp"

#include "Nutcrackz/ImGui/ImGuiLayer.hpp"

int main(int argc, char** argv);

namespace Nutcrackz {

	struct ApplicationCommandLineArgs
	{
		int Count;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
			NZ_CORE_ASSERT(index < Count);
			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		std::string Name = "Nutcrackz Application";
		ApplicationCommandLineArgs CommandLineArgs;
		std::string WorkingDirectory = "";
		bool MaximizedOnLaunch = false;
		bool Fullscreen = false;
	};

	class ScriptEngine;

	class Application
	{
	public:
		Application(const ApplicationSpecification& specification, bool maximizedOnLaunch, uint32_t width, uint32_t height, bool fullscreen);
		virtual ~Application();

		void Run();
		void Update();
		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Window& GetWindow() { return *m_Window; }

		void Close();

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		static Application& Get() { return *s_Instance; }

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		InputSystem GetInputSystem() { return m_InputSystem; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		ApplicationSpecification m_Specification;
		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;

		InputSystem m_InputSystem;

	private:
		static Application* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	// To be defined in CLIENT
	Application* CreateApplication(ApplicationCommandLineArgs args);

}
