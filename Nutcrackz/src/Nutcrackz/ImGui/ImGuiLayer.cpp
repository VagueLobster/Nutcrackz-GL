#include "nzpch.hpp"
#include "ImGuiLayer.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "Nutcrackz/Core/Application.hpp"

// TEMPORARY
#include <GLFW/glfw3.h>
//#include <glad/glad.h>

#include "ImGuizmo.h"
#include "StyleManager.hpp"

#define IMGUI_UNLIMITED_FRAME_RATE

namespace Nutcrackz {

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
	}

	void ImGuiLayer::OnAttach()
	{
		//NZ_PROFILE_FUNCTION();

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
		io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		//io.FontDefault = io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Regular.ttf", Window::s_HighDPIScaleFactor * 18.0f);
		//io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Bold.ttf", Window::s_HighDPIScaleFactor * 18.0f);
		//io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/consola.ttf", Window::s_HighDPIScaleFactor * 13.333333f);
		//io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", Window::s_HighDPIScaleFactor * 16.0f);
		//io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Bold.ttf", Window::s_HighDPIScaleFactor * 22.0f);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Regular.ttf", 18.0f);
		io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Bold.ttf", 18.0f);
		io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/consola.ttf", 13.333333f);
		io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 16.0f);
		io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Bold.ttf", 22.0f);

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical
		ImGuiStyle& style = ImGui::GetStyle();

		//style.WindowMenuButtonPosition = ImGuiDir_None; // Comment this out, to get the windows' (hide) arrows back!
		style.TabRounding = 0.0f; // Comment this out, to get rounded tabs back!
		//style.ScaleAllSizes(Window::s_HighDPIScaleFactor);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		//SetDarkThemeColorsGreen();
		SetDarkThemeColorsOrange();
		
		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void ImGuiLayer::OnDetach()
	{
		//NZ_PROFILE_FUNCTION();

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& e)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::Begin()
	{
		//NZ_PROFILE_FUNCTION();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::End()
	{
		//NZ_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();

		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::ResizeFont(float dpiSize)
	{
		if (m_DPISize != dpiSize)
		{
			m_HasResized = false;
			m_DPISize = dpiSize;
		}

		if (!m_HasResized)
		{
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();

			OnAttach();

			m_HasResized = true;
		}
	}

	uint32_t ImGuiLayer::GetActiveWidgetID() const
	{
		return GImGui->ActiveId;
	}

	void ImGuiLayer::SetDarkThemeColorsGreen()
	{
		Hazard::ImUI::Style colorStyle = Hazard::ImUI::Style();

		colorStyle.BackgroundColor = ImVec4{ 0.1f,  0.105f,  0.105f,  1.0f };
		colorStyle.Window.PopupBgColor = ImVec4{ 0.05f,  0.0505f,  0.051f,  1.0f };
		colorStyle.DragDropTarget = ConvertColorFromByteToFloats(93, 197, 5, 112);

		// Misc.
		colorStyle.Window.SliderGrab = ConvertColorFromByteToFloats(93, 197, 5, 112);
		colorStyle.Window.SliderGrabActive = ConvertColorFromByteToFloats(73, 159, 5, 255);

		// Headers
		colorStyle.Window.Header = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Window.HeaderHovered = ConvertColorFromByteToFloats(93, 197, 5, 112);
		colorStyle.Window.HeaderActive = ConvertColorFromByteToFloats(93, 197, 5, 168);

		// Buttons
		colorStyle.Button.Button = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Button.ButtonHovered = ConvertColorFromByteToFloats(93, 197, 5, 112);
		colorStyle.Button.ButtonActive = ConvertColorFromByteToFloats(73, 159, 5, 255);

		// Frame BG
		colorStyle.Frame.FrameColor = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Frame.FrameHovered = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Frame.FrameActive = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };

		// Tabs
		colorStyle.Tab.Tab = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colorStyle.Tab.TabHovered = ConvertColorFromByteToFloats(93, 197, 5, 197);
		colorStyle.Tab.TabActiveFocus = ConvertColorFromByteToFloats(93, 197, 5, 112);
		colorStyle.Tab.TabUnfocus = ImVec4{ 0.13f, 0.13005f, 0.1301f, 1.0f };

		Hazard::ImUI::StyleManager::LoadStyle(colorStyle);
		//Hazard::ImUI::StyleManager::LoadStyle(Hazard::ImUI::Style());
	}

	void ImGuiLayer::SetDarkThemeColorsOrange()
	{
		/*auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };*/

		Hazard::ImUI::Style colorStyle = Hazard::ImUI::Style();

		colorStyle.BackgroundColor = ImVec4{ 0.1f,  0.105f,  0.105f,  1.0f };
		//colorStyle.ModalBackgroundColor = ConvertColorFromByteToFloats(10, 10, 10, 128);
		//colorStyle.ChildBackgroundColor = ConvertColorFromByteToFloats(38, 38, 38, 255);
		//colorStyle.MenuBarBackground = ConvertColorFromByteToFloats(24, 24, 22, 255);
		colorStyle.Window.PopupBgColor = ImVec4{ 0.05f,  0.0505f,  0.051f,  1.0f };
		colorStyle.Window.Plotlines = ConvertColorFromByteToFloats(255, 140, 0, 112);
		colorStyle.DragDropTarget = ConvertColorFromByteToFloats(255, 140, 0, 112);
		colorStyle.NavHighlight = ConvertColorFromByteToFloats(255, 140, 0, 112);

		colorStyle.Separator.SeparatorHovered = ConvertColorFromByteToFloats(255, 140, 0, 112);
		colorStyle.Separator.SeparatorActive = ConvertColorFromByteToFloats(255, 140, 0, 168);

		// Resize Grip
		colorStyle.Window.ResizeGripHovered = ConvertColorFromByteToFloats(255, 140, 0, 112);
		colorStyle.Window.ResizeGripActive = ConvertColorFromByteToFloats(255, 140, 0, 168);

		// Misc.
		colorStyle.Window.SliderGrab = ConvertColorFromByteToFloats(255, 140, 0, 112);
		colorStyle.Window.SliderGrabActive = ConvertColorFromByteToFloats(255, 140, 0, 168);

		// Headers
		colorStyle.Window.Header = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Window.HeaderHovered = ConvertColorFromByteToFloats(255, 140, 0, 112);
		colorStyle.Window.HeaderActive = ConvertColorFromByteToFloats(255, 140, 0, 168);

		// Buttons
		colorStyle.Button.Button = ImVec4{ 0.19f,  0.1905f,  0.191f,  1.0f };
		colorStyle.Button.ButtonHovered = ConvertColorFromByteToFloats(255, 140, 0, 112);
		colorStyle.Button.ButtonActive = ConvertColorFromByteToFloats(255, 145, 35, 168);

		// Frame BG
		colorStyle.Frame.FrameColor = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Frame.FrameHovered = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Frame.FrameActive = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };

		// Scrollbars
		colorStyle.ScrollBar.ScrollbarGrab = ConvertColorFromByteToFloats(255, 140, 0, 112);
		colorStyle.ScrollBar.ScrollbarGrabHovered = ConvertColorFromByteToFloats(255, 140, 0, 112);
		colorStyle.ScrollBar.ScrollbarGrabActive = ConvertColorFromByteToFloats(255, 145, 35, 168);

		// Tabs
		colorStyle.Tab.Tab = ImVec4{ 0.15f, 0.1505f, 0.1501f, 1.0f };
		colorStyle.Tab.TabHovered = ConvertColorFromByteToFloats(255, 140, 0, 138);
		colorStyle.Tab.TabActiveFocus = ConvertColorFromByteToFloats(255, 140, 0, 112);
		colorStyle.Tab.TabUnfocus = ImVec4{ 0.12f, 0.1205f, 0.1201f, 1.0f };
		colorStyle.Tab.TabActive = ImVec4{ 0.15f, 0.1505f, 0.1501f, 1.0f };

		// Title
		colorStyle.Window.TitleBgColor = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };
		colorStyle.Window.TitleBgActive = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		Hazard::ImUI::StyleManager::LoadStyle(colorStyle);
		//Hazard::ImUI::StyleManager::LoadStyle(Hazard::ImUI::Style());
	}

	void ImGuiLayer::SetGoldDarkThemeColors()
	{
		Hazard::ImUI::Style colorStyle = Hazard::ImUI::Style();

		colorStyle.BackgroundColor = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };
		colorStyle.Window.PopupBgColor = ImVec4{ 0.1f,  0.105f,  0.11f,  0.95f };
		colorStyle.ChildBackgroundColor = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };
		colorStyle.Window.Border = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colorStyle.Window.Text = ImVec4{ 0.8f, 0.805f, 0.81f, 1.0f };
		colorStyle.Window.TextSelectedBg = ImVec4{ 0.48f, 0.4805f, 0.381f, 1.0f };
		colorStyle.Window.TextDisabled = ImVec4{ 0.48f, 0.4805f, 0.381f, 1.0f };
		colorStyle.DragDropTarget = ImVec4{ 0.48f, 0.4805f, 0.381f, 1.0f };
		colorStyle.MenuBarBackground = ImVec4{ 0.1f,  0.105f,  0.11f,  1.0f };

		// Misc.
		colorStyle.Window.Checkmark = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.Window.SliderGrab = ImVec4{ 0.25f, 0.2505f, 0.151f, 1.0f };
		colorStyle.Window.SliderGrabActive = ImVec4{ 0.38f, 0.3805f, 0.281f, 1.0f };

		// Separator
		colorStyle.Separator.Separator = ImVec4{ 0.45f, 0.455f, 0.21f, 1.0f };
		colorStyle.Separator.SeparatorHovered = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.Separator.SeparatorActive = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };

		// Scrollbar
		colorStyle.ScrollBar.ScrollbarGrab = ImVec4{ 0.45f, 0.455f, 0.21f, 1.0f };
		colorStyle.ScrollBar.ScrollbarGrabHovered = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.ScrollBar.ScrollbarGrabActive = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };

		// Headers
		colorStyle.Window.Header = ImVec4{ 0.45f, 0.455f, 0.21f, 1.0f };
		colorStyle.Window.HeaderHovered = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.Window.HeaderActive = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };

		// Buttons
		colorStyle.Button.Button = ImVec4{ 0.45f, 0.455f, 0.21f, 1.0f };
		colorStyle.Button.ButtonHovered = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.Button.ButtonActive = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };

		// Frame BG
		colorStyle.Frame.FrameColor = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colorStyle.Frame.FrameHovered = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colorStyle.Frame.FrameActive = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colorStyle.Tab.Tab = ImVec4{ 0.45f, 0.455f, 0.21f, 1.0f };
		colorStyle.Tab.TabHovered = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.Tab.TabActive = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };
		colorStyle.Tab.TabActiveFocus = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };
		colorStyle.Tab.TabUnfocus = ImVec4{ 0.25f, 0.2505f, 0.151f, 1.0f };

		// Title
		colorStyle.Window.TitleBgColor = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colorStyle.Window.TitleBgActive = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		//Hazard::ImUI::StyleManager::LoadStyle(Hazard::ImUI::Style());
		Hazard::ImUI::StyleManager::LoadStyle(colorStyle);
	}

	void ImGuiLayer::SetChocolateThemeColors()
	{
		Hazard::ImUI::Style colorStyle = Hazard::ImUI::Style();
		
		colorStyle.BackgroundColor = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.Window.PopupBgColor = ConvertColorFromByteToFloats(42, 33, 28, 243);
		colorStyle.ChildBackgroundColor = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.Window.Border = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.Window.Text = ConvertColorFromByteToFloats(189, 174, 157, 255);
		colorStyle.Window.TextSelectedBg = ConvertColorFromByteToFloats(75, 60, 52, 255);
		colorStyle.Window.TextDisabled = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.DragDropTarget = ConvertColorFromByteToFloats(75, 60, 52, 255);
		colorStyle.MenuBarBackground = ConvertColorFromByteToFloats(42, 33, 28, 255);

		// Misc.
		colorStyle.Window.Checkmark = ConvertColorFromByteToFloats(189, 174, 157, 255);
		colorStyle.Window.SliderGrab = ConvertColorFromByteToFloats(189, 174, 157, 255);
		colorStyle.Window.SliderGrabActive = ConvertColorFromByteToFloats(42, 33, 28, 255);

		// Separator
		colorStyle.Separator.Separator = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Separator.SeparatorHovered = ConvertColorFromByteToFloats(158, 126, 109, 255);
		colorStyle.Separator.SeparatorActive = ConvertColorFromByteToFloats(75, 60, 52, 255);

		// Scrollbar
		colorStyle.ScrollBar.ScrollbarGrab = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.ScrollBar.ScrollbarGrabHovered = ConvertColorFromByteToFloats(158, 126, 109, 255);
		colorStyle.ScrollBar.ScrollbarGrabActive = ConvertColorFromByteToFloats(75, 60, 52, 255);

		// Headers
		colorStyle.Window.Header = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Window.HeaderHovered = ConvertColorFromByteToFloats(158, 126, 109, 255);
		colorStyle.Window.HeaderActive = ConvertColorFromByteToFloats(75, 60, 52, 255);

		// Buttons
		colorStyle.Button.Button = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Button.ButtonHovered = ConvertColorFromByteToFloats(158, 126, 109, 255);
		colorStyle.Button.ButtonActive = ConvertColorFromByteToFloats(75, 60, 52, 255);

		// Frame BG
		colorStyle.Frame.FrameColor = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Frame.FrameHovered = ConvertColorFromByteToFloats(158, 126, 109, 255);
		colorStyle.Frame.FrameActive = ConvertColorFromByteToFloats(75, 60, 52, 255);

		// Tabs
		colorStyle.Tab.Tab = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Tab.TabHovered = ConvertColorFromByteToFloats(149, 117, 100, 255);
		colorStyle.Tab.TabActive = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.Tab.TabActiveFocus = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.Tab.TabUnfocus = ConvertColorFromByteToFloats(119, 92, 79, 255);

		// Title
		colorStyle.Window.TitleBgColor = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Window.TitleBgActive = ConvertColorFromByteToFloats(119, 92, 79, 255);

		//Hazard::ImUI::StyleManager::LoadStyle(Hazard::ImUI::Style());
		Hazard::ImUI::StyleManager::LoadStyle(colorStyle);
	}

	void ImGuiLayer::SetLightThemeColors()
	{
		Hazard::ImUI::Style colorStyle = Hazard::ImUI::Style();

		colorStyle.BackgroundColor = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colorStyle.ChildBackgroundColor = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colorStyle.Window.PopupBgColor = ImVec4{ 0.65f, 0.6505f, 0.651f, 0.95f };
		colorStyle.Window.Border = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };
		colorStyle.DragDropTarget = ImVec4{ 0.88f, 0.8805f, 0.881f, 1.0f };
		colorStyle.MenuBarBackground = ImVec4{ 0.8f, 0.805f, 0.81f, 1.0f };
		colorStyle.ModalBackgroundColor = ImVec4{ 0.1f, 0.105f, 0.11f, 0.65f };

		// Text
		colorStyle.Window.Text = ImVec4{ 0.18f, 0.1805f, 0.181f, 1.0f };
		colorStyle.Window.TextSelectedBg = ImVec4{ 0.85f, 0.8505f, 0.851f, 1.0f };
		colorStyle.Window.TextDisabled = ImVec4{ 0.08f, 0.0805f, 0.081f, 1.0f };

		// Misc.
		colorStyle.Window.Checkmark = ImVec4{ 0.88f, 0.8805f, 0.881f, 1.0f };
		colorStyle.Window.SliderGrab = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };
		colorStyle.Window.SliderGrabActive = ImVec4{ 0.68f, 0.6805f, 0.681f, 1.0f };
		
		// Separator
		colorStyle.Separator.Separator = ImVec4{ 0.45f, 0.455f, 0.46f, 1.0f };
		colorStyle.Separator.SeparatorHovered = ImVec4{ 0.55f, 0.555f, 0.56f, 1.0f };
		colorStyle.Separator.SeparatorActive = ImVec4{ 0.3f, 0.3005f, 0.301f, 1.0f };

		// Scrollbar
		colorStyle.ScrollBar.ScrollbarGrab = ImVec4{ 0.45f, 0.455f, 0.46f, 1.0f };
		colorStyle.ScrollBar.ScrollbarGrabHovered = ImVec4{ 0.55f, 0.555f, 0.56f, 1.0f };
		colorStyle.ScrollBar.ScrollbarGrabActive = ImVec4{ 0.3f, 0.3005f, 0.301f, 1.0f };

		// Headers
		colorStyle.Window.Header = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };
		colorStyle.Window.HeaderHovered = ImVec4{ 0.85f, 0.8505f, 0.851f, 1.0f };
		colorStyle.Window.HeaderActive = ImVec4{ 0.8f, 0.805f, 0.81f, 1.0f };

		// Buttons
		colorStyle.Button.Button = ImVec4{ 0.45f, 0.455f, 0.46f, 1.0f };
		colorStyle.Button.ButtonHovered = ImVec4{ 0.55f, 0.555f, 0.56f, 1.0f };
		colorStyle.Button.ButtonActive = ImVec4{ 0.8f, 0.805f, 0.81f, 1.0f };

		// Frame BG
		colorStyle.Frame.FrameColor = ImVec4{ 0.6f, 0.605f, 0.61f, 1.0f };
		colorStyle.Frame.FrameHovered = ImVec4{ 0.7f, 0.705f, 0.71f, 1.0f };
		colorStyle.Frame.FrameActive = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };

		// Tabs
		colorStyle.Tab.Tab = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };
		colorStyle.Tab.TabHovered = ImVec4{ 0.78f, 0.7805f, 0.781f, 1.0f };
		colorStyle.Tab.TabActive = ImVec4{ 0.68f, 0.6805f, 0.681f, 1.0f };
		colorStyle.Tab.TabActiveFocus = ImVec4{ 0.68f, 0.6805f, 0.681f, 1.0f };
		colorStyle.Tab.TabUnfocus = ImVec4{ 0.65f, 0.6505f, 0.651f, 1.0f };

		// Title
		colorStyle.Window.TitleBgColor = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };
		colorStyle.Window.TitleBgActive = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };

		//Hazard::ImUI::StyleManager::LoadStyle(Hazard::ImUI::Style());
		Hazard::ImUI::StyleManager::LoadStyle(colorStyle);
	}

	ImVec4 ImGuiLayer::ConvertColorFromByteToFloats(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
	{
		return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
	}

	uint32_t ImGuiLayer::ConvertColorFromFloatToByte(float value)
	{
		return (value >= 1.0f ? 255 : (value <= 0.0f ? 0 : (int)floor(value * 256.0f)));
	}

}
