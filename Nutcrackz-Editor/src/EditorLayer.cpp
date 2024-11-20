#include "EditorLayer.hpp"

#include "Serialization/EditorSerializer.hpp"

#include "Nutcrackz/Scene/SceneSerializer.hpp"
#include "Nutcrackz/Utils/PlatformUtils.hpp"
#include "Nutcrackz/Math/Math.hpp"
#include "Nutcrackz/Scripting/ScriptGlue.hpp"
#include "Nutcrackz/Renderer/Font.hpp"

#include "Nutcrackz/Asset/AssetManager.hpp"
#include "Nutcrackz/Asset/TextureImporter.hpp"
#include "Nutcrackz/Asset/SceneImporter.hpp"

#include "Nutcrackz/Input/InputDevice.hpp"

#include "Nutcrackz/UndoRedo/CommandHistory.hpp"

#include <imgui/imgui.h>
#include "ImGuizmo.h"

#include "Nutcrackz/ImGUI/ImGuiLayer.hpp"
#include "Nutcrackz/ImGUI/ImGui.hpp"
#include "Nutcrackz/ImGUI/ImGuiMisc.hpp"

#include "Platform/Windows/WindowsWindow.hpp"

#include "rtmcpp/MatrixOps.hpp"
#include "rtm/qvvf.h"

#include <fstream>
#include <shellapi.h>

// The Microsoft C++ compiler is non-compliant with the C++ standard and needs
// the following definition to disable a security warning on std::strncpy().
#ifdef _MSVC_LANG
	#define _CRT_SECURE_NO_WARNINGS
#endif

namespace Nutcrackz {

	// Used in New Project Popup
	std::string g_ProjectName = "Untitled";
	std::string g_AssetsFolder = "";
	std::string g_ScriptModulePath = "";

	// Used in Edit Project Settings Popup
	std::string g_EditProjectName = "";
	std::string g_EditStartScene = "";
	std::string g_EditAssetsFolder = "";
	std::string g_EditScriptModulePath = "";

	static RefPtr<Font> s_Font;

	ImVec4 ConvertColorFromByteToFloats(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
	{
		return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
	}

	static bool s_PressedShift = false;
	static bool s_CanUndo = false;
	static bool s_CanRedo = false;

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
		s_Font = Font::GetDefault();
	}

	void EditorLayer::OnAttach()
	{
		//NZ_PROFILE_FUNCTION();

/*#ifdef NZ_DEBUG
		// Don't retrieve screen resolution for now (it currently won't work!)
#else
		int count;
		const GLFWvidmode* videoModes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &count);

		for (int i = 0; i < count; i++)
			m_Resolutions.insert({ videoModes[i].width, videoModes[i].height, videoModes[i].refreshRate });

		for (const auto& res : m_Resolutions)
		{
			std::cout << res.Width << ", " << res.Height << " @ " << res.RefreshRate << std::endl;
		}
#endif*/

#ifdef NZ_DEBUG
		Application::Get().GetWindow().SetTitle("Nutcrackz Editor - Debug | Untitled");
#elif NZ_RELEASE
		Application::Get().GetWindow().SetTitle("Nutcrackz Editor - Release | Untitled");
#endif

		m_IconPlay = TextureImporter::LoadTexture2D("Resources/Icons/Editor/PlayButton.png");
		m_IconPause = TextureImporter::LoadTexture2D("Resources/Icons/Editor/PauseButton.png");
		m_IconSimulate = TextureImporter::LoadTexture2D("Resources/Icons/Editor/SimulateButton.png");
		m_IconStep = TextureImporter::LoadTexture2D("Resources/Icons/Editor/StepButton.png");
		m_IconStop = TextureImporter::LoadTexture2D("Resources/Icons/Editor/StopButton.png");
		m_IconNoGizmo = TextureImporter::LoadTexture2D("Resources/Icons/Editor/DisableTool.png");
		m_IconTranslate = TextureImporter::LoadTexture2D("Resources/Icons/Editor/MoveTool.png");
		m_IconRotate = TextureImporter::LoadTexture2D("Resources/Icons/Editor/RotateTool.png");
		m_IconScale = TextureImporter::LoadTexture2D("Resources/Icons/Editor/ScaleTool.png");
		m_Logo = TextureImporter::LoadTexture2D("Resources/Icons/Editor/Icon.png");

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		m_FramebufferEditor = Framebuffer::Create(fbSpec);

		m_EditorScene = RefPtr<Scene>::Create();
		m_ActiveScene = m_EditorScene;
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		m_SceneHierarchyPanel.Init();

		auto commandLineArgs = Application::Get().GetSpecification().CommandLineArgs;
		if (commandLineArgs.Count > 1)
		{
			auto projectFilePath = commandLineArgs[1];
			OpenProject(projectFilePath);
		}
		else
		{
			NewProject();
		}

		m_EditorCamera = RefPtr<EditorCamera>::Create(30.0f, 1.778f, 0.1f, 1000.0f);
		m_EditorCamera->SetPitch(0.3f);
		m_EditorCamera->IsControllable = true;

		//Application::Get().GetWindow().SetVSync(false);

		//if (!EditorSerializer::EditorDeserialize("EditorSettings.nzset"))
		if (!EditorSerializer::EditorDeserializeJSON("EditorSettings.nzset"))
		{
			NZ_CORE_ERROR("Settings file does not exist!");
		}

		//m_ContentBrowserPanel = CreateScope<ContentBrowserPanel>(Project::GetActive());

		using enum TriggerEventType;

		/* Used keycodes
		KeyCode::N
		KeyCode::O
		KeyCode::S
		KeyCode::D
		KeyCode::Q
		KeyCode::W
		KeyCode::E
		KeyCode::R
		KeyCode::Delete
		KeyCode::F5
		KeyCode::F7
		*/

		m_EditorLayerContext = Application::Get().GetInputSystem().CreateContext();
		m_ShiftContext = Application::Get().GetInputSystem().CreateContext();
		m_ControlContext = Application::Get().GetInputSystem().CreateContext();
		m_ControlShiftContext = Application::Get().GetInputSystem().CreateContext();

		auto activateMouseAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericMouse, MouseCode::ButtonLeft, OnPressed | OnHeld }, 1.0f },
						{ { GenericMouse, MouseCode::ButtonLeft, OnReleased }, 0.0f }
					}
				},
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::Enter, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::Enter, OnReleased }, 0.0f }
					}
				}
			},
			.ConsumeInputs = true
		});

		m_EditorLayerContext.BindAction(activateMouseAction, [&](const InputReading& reading)
		{
			auto [mouseButton, enterKey] = reading.Read<2>();

			if (mouseButton > 0.0f)
				SceneHierarchyPanel::MouseDragging = true;
			else if (mouseButton <= 0.0f)
				SceneHierarchyPanel::MouseDragging = false;

			if (enterKey > 0.0f)
				SceneHierarchyPanel::PressedEnter = true;
			else if (enterKey <= 0.0f)
				SceneHierarchyPanel::PressedEnter = false;
		});

		auto activateControlAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::LeftCtrl, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::LeftCtrl, OnReleased }, 0.0f },
						{ { GenericKeyboard, KeyCode::RightCtrl, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::RightCtrl, OnReleased }, 0.0f }
					}
				}
			},
			.ConsumeInputs = true
		});

		m_EditorLayerContext.BindAction(activateControlAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f)
				m_ControlContext.Activate();
			else
				m_ControlContext.Deactivate();
		});

		auto activateShiftAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::LeftShift, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::LeftShift, OnReleased }, 0.0f },
						{ { GenericKeyboard, KeyCode::RightShift, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::RightShift, OnReleased }, 0.0f }
					}
				}
			},
			.ConsumeInputs = true
		});

		m_EditorLayerContext.BindAction(activateShiftAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f)
			{
				s_PressedShift = true;
				m_ShiftContext.Activate();
			}
			else
			{
				s_PressedShift = false;
				m_ShiftContext.Deactivate();
			}
		});

		auto activateControlShiftAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::LeftShift, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::LeftShift, OnReleased }, 0.0f },
						{ { GenericKeyboard, KeyCode::RightShift, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::RightShift, OnReleased }, 0.0f }
					}
				}
			},
			.ConsumeInputs = true
		});

		m_ControlContext.BindAction(activateControlShiftAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f)
			{
				s_PressedShift = true;
				m_ControlShiftContext.Activate();
			}
			else
			{
				s_PressedShift = false;
				m_ControlShiftContext.Deactivate();
			}
		});

		auto fileNewAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::N, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::N, OnReleased }, 0.0f }
					},
				}
			},
			.ConsumeInputs = true
		});

		m_ControlContext.BindAction(fileNewAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f)
				NewScene();
		});

		auto fileOpenAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::O, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::O, OnReleased }, 0.0f }
					},
				}
			},
			.ConsumeInputs = true
		});

		m_ControlContext.BindAction(fileOpenAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f)
				OpenProject();
		});

		auto fileSaveAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::S, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::S, OnReleased }, 0.0f }
					},
				}
			},
			.ConsumeInputs = true
		});

		m_ControlContext.BindAction(fileSaveAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f)
				SaveScene();
		});

		m_ControlShiftContext.BindAction(fileSaveAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f)
				SaveSceneAs();
		});

		auto undoRedoAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::Z, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::Z, OnReleased }, 0.0f }
					},
				}
			},
			.ConsumeInputs = true
		});

		m_ControlContext.BindAction(undoRedoAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f && !s_PressedShift)
				CommandHistory::Undo();
		});

		m_ControlShiftContext.BindAction(undoRedoAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f)
				CommandHistory::Redo();
		});

		auto playPauseMediaAction = Application::Get().GetInputSystem().RegisterAction({
		.AxisBindings = {
			{
				.Bindings = {
					{ { GenericKeyboard, KeyCode::PlayPauseMedia , OnPressed }, 1.0f },
					{ { GenericKeyboard, KeyCode::PlayPauseMedia, OnReleased }, 0.0f }
				},
			}
		},
		.ConsumeInputs = true
		});

		m_ControlContext.BindAction(playPauseMediaAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();
		});

		/*auto numpadMinusAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::NumpadSubtract, OnPressed }, 1.0f },
						{ { GenericKeyboard, KeyCode::NumpadSubtract, OnReleased }, 0.0f }
					},
				}
			},
			.ConsumeInputs = true
		});

		m_EditorLayerContext.BindAction(numpadMinusAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f)
				NewScene();
		});*/

		m_EditorLayerContext.Activate();
	}

	void EditorLayer::OnDetach()
	{
		//NZ_PROFILE_FUNCTION();

		//if (!EditorSerializer::EditorSerialize("EditorSettings.nzset"))
		if (!EditorSerializer::EditorSerializeJSON("EditorSettings.nzset"))
		{
			NZ_CORE_ERROR("Could not serialize settings data to file!");
		}
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		NZ_PROFILE_FUNCTION("EditorLayer::OnUpdate");

		// Editor rendering
		if (ShowEditorViewport)
		{
			m_ActiveScene->OnViewportResize((uint32_t)m_EditorViewportSize.X, (uint32_t)m_EditorViewportSize.Y);

			// Resize Editor
			if (FramebufferSpecification spec = m_FramebufferEditor->GetSpecification();
				m_EditorViewportSize.X > 0.0f && m_EditorViewportSize.Y > 0.0f &&
				(spec.Width != m_EditorViewportSize.X || spec.Height != m_EditorViewportSize.Y))
			{
				m_FramebufferEditor->Resize((uint32_t)m_EditorViewportSize.X, (uint32_t)m_EditorViewportSize.Y);
				m_EditorCamera->SetViewportSize(m_EditorViewportSize.X, m_EditorViewportSize.Y);
			}

			// Render
			Renderer2D::ResetStats();
			Renderer3D::Reset3DStats();

			m_FramebufferEditor->Bind();

			// Enable these to switch the clear color from almost black to fit the other themes more "properly"...
			{
				if (UseGreenDarkTheme || UseOrangeDarkTheme || UseGoldDarkTheme)
					RenderCommand::SetClearColor({ 0.075f, 0.075f, 0.075f, 1 });
				else if (UseLightTheme)
					RenderCommand::SetClearColor({ 0.25f, 0.25f, 0.25f, 1 });
				else if (UseChocolateTheme)
					RenderCommand::SetClearColor({ 42.0f / 255.0f, 33.0f / 255.0f, 28.0f / 255.0f, 1.0f });
			}

			RenderCommand::Clear();

			// Clear our entity ID attachment to -1
			m_FramebufferEditor->ClearAttachment(1, -1);

			switch (m_SceneState)
			{
			case SceneState::Edit:
			{
				if (!CommandHistory::CanUndo())
					s_CanUndo = false;
				else
					s_CanUndo = true;

				if (!CommandHistory::CanRedo())
					s_CanRedo = false;
				else
					s_CanRedo = true;

				m_EditorCamera->OnUpdate(ts);
				m_ActiveScene->OnUpdateEditor(ts, *m_EditorCamera.Raw());

				break;
			}
			case SceneState::Simulate:
			{
				m_EditorCamera->OnUpdate(ts);
				m_ActiveScene->OnUpdateSimulation(ts, *m_EditorCamera.Raw());

				break;
			}
			case SceneState::Play:
			{
				m_ActiveScene->OnUpdateRuntime(ts);

				if (m_Gravity.X != m_ActiveScene->GetPhysics2DGravity().X || m_Gravity.Y != m_ActiveScene->GetPhysics2DGravity().Y)
					m_Gravity = m_ActiveScene->GetPhysics2DGravity();

				auto sceneUpdateQueue = m_PostSceneUpdateQueue;
				m_PostSceneUpdateQueue.clear();
				for (auto& fn : sceneUpdateQueue)
					fn();

				break;
			}
			}

			//if (ShowingAboutPopupWindow)
			//{
			//	if (m_HoveringChernoUnplugged || m_HoveringStudioChernosGithub || m_HoveringToastmasternsGithub || m_HoveringToniPlaysGithub)
			//		Application::Get().GetWindow().SetCursorHand();
			//	else if (!m_HoveringChernoUnplugged && !m_HoveringStudioChernosGithub && !m_HoveringToastmasternsGithub && !m_HoveringToniPlaysGithub)
			//		Application::Get().GetWindow().SetCursorNormal();
			//}

			// Custom mouse cursor
			if (!ShowingAboutPopupWindow)
			{
				if (!ScriptGlue::s_SetCursorPath.empty())
				{
					if (m_EditorViewportHovered || m_ViewportToolbarHovered)
					{
						// Set custom cursor via scripting (not *.cur cursors!)
						if (m_SceneState == SceneState::Play)
						{
							if (!ScriptGlue::s_SetCursorPath.empty())
							{
								if (!ScriptGlue::s_CalledSetCursor || (ScriptGlue::s_ChangedCursor && Application::Get().GetWindow().GetCursor() != ScriptGlue::s_SetCursorPath))
								{
									Application::Get().GetWindow().SetCursor(ScriptGlue::s_SetCursorPath, ScriptGlue::s_CursorHotSpot);
									ScriptGlue::s_CalledSetCursor = true;
									ScriptGlue::s_ChangedCursor = false;
								}
							}
						}

						// Revert custom cursor back to original
						if (m_SceneState == SceneState::Edit)
						{
							if (ScriptGlue::s_CalledSetCursor)
							{
								Application::Get().GetWindow().ResetCursor();
								ScriptGlue::s_CalledSetCursor = false;
							}
							else
							{
								Application::Get().GetWindow().SetCursorNormal();
							}
						}
					}
					else if (!m_EditorViewportHovered && !m_ViewportToolbarHovered)
					{
						if (ScriptGlue::s_CalledSetCursor)
						{
							Application::Get().GetWindow().ResetCursor();
							ScriptGlue::s_CalledSetCursor = false;
						}
						else
						{
							Application::Get().GetWindow().SetCursorNormal();
						}
					}
				}
			}
			else if (ShowingAboutPopupWindow)
			{
				if (m_HoveringChernoUnplugged || m_HoveringStudioChernosGithub || m_HoveringToastmasternsGithub || m_HoveringToniPlaysGithub)
					Application::Get().GetWindow().SetCursorHand();
				else if (!m_HoveringChernoUnplugged && !m_HoveringStudioChernosGithub && !m_HoveringToastmasternsGithub && !m_HoveringToniPlaysGithub)
					Application::Get().GetWindow().SetCursorNormal();
			}

			auto [mx, my] = ImGui::GetMousePos();

			Input::s_MousePos = rtmcpp::Vec2(mx, my);
			Input::s_ViewportBounds = m_EditorViewportBounds[0];
			Input::s_ViewportSize = m_EditorViewportSize;

			mx -= m_EditorViewportBounds[0].X;
			my -= m_EditorViewportBounds[0].Y;
			rtmcpp::Vec2 viewportSize = m_EditorViewportSize;
			my = viewportSize.Y - my;
			int mouseX = (int)mx;
			int mouseY = (int)my;

			if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.X && mouseY < (int)viewportSize.Y)
			{
				int pixelData = m_FramebufferEditor->ReadPixel(1, mouseX, mouseY);

				if (pixelData >= -2)
				{
					if (pixelData != -2 && !Input::IsKeyPressed(Key::LeftAlt))
					{
						m_HoveredEntity = pixelData == -1 ? Entity() : Entity(m_ActiveScene->GetECS().entity(pixelData));
					}
				}
			}

			if (m_ActiveScene != nullptr)
			{
				if (m_HoveredEntity != Entity())
				{
					ScriptGlue::SetHoveredEntity(m_HoveredEntity);
					//NZ_CORE_CRITICAL("Hovered entity is NOT Null!");
				}
				else
				{
					ScriptGlue::SetHoveredEntity(Entity());
					//NZ_CORE_CRITICAL("Hovered entity is Null!");
				}

				if (m_SelectedEntity != Entity())
				{
					ScriptGlue::SetSelectedEntity(m_SelectedEntity);
				}
				else
				{
					ScriptGlue::SetSelectedEntity(Entity());
				}
			}

			//if (m_EditorViewportHovered && m_ActiveScene != nullptr)
			//{
			//	if (m_HoveredEntity != Entity())
			//		ScriptGlue::SetHoveredEntity(m_HoveredEntity);
			//	else
			//		ScriptGlue::SetHoveredEntity(Entity());
			//
			//	if (m_SelectedEntity != Entity())
			//		ScriptGlue::SetSelectedEntity(m_SelectedEntity);
			//	else
			//		ScriptGlue::SetSelectedEntity(Entity());
			//}
			//else if (!m_EditorViewportHovered && m_ActiveScene != nullptr)
			//{
			//	ScriptGlue::SetHoveredEntity(Entity());
			//
			//	NZ_CORE_CRITICAL("Hovered entity is Null!");
			//}

			OnOverlayRender();

			m_FramebufferEditor->Unbind();
		}
	}

	void EditorLayer::OnImGuiRender()
	{
		NZ_PROFILE_FUNCTION("EditorLayer::OnImGuiRender");

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f, 8.0f));
		Hazard::ImUI::Dockspace::BeginDockspace("DockSpace Demo", ImGuiDockNodeFlags_NoSplit);
		Hazard::ImUI::Dockspace::EndDockspace("MyDockSpace");
		ImGui::PopStyleVar();

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		style.WindowMinSize.x = minWinSizeX;
		
		// Begin Menu bar
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("New Project"))
					ShowNewProjectPopupWindow = true;

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
					OpenProject();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Save Project"))
					SaveProject();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Save Project As..."))
					SaveProjectAs();

				ImGui::Separator();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
					NewScene();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
					SaveScene();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					SaveSceneAs();

				ImGui::Separator();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Exit"))
					Application::Get().Close();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (!s_CanUndo)
					ImGui::BeginDisabled();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Undo", "Ctrl+Z"))
					CommandHistory::Undo();
				
				if (!s_CanUndo)
					ImGui::EndDisabled();

				if (!s_CanRedo)
					ImGui::BeginDisabled();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z"))
					CommandHistory::Redo();

				if (!s_CanRedo)
					ImGui::EndDisabled();

				ImGui::Separator();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Create Empty Entity"))
					OnCreateEmptyEntity();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Duplicate Entity", "Ctrl+D"))
					OnDuplicateEntity();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Script"))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Reload assembly", "Ctrl+R"))
					ReloadCSharp();
			
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Settings"))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Edit Project Settings"))
					ShowEditProjectPopupWindow = true;

				ImGui::Separator();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Change Gravity..."))
					ShowEditGravityPopupWindow = true;

				ImGui::Separator();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::BeginMenu("Editor Settings"))
				{
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					if (ImGui::MenuItem("Change To 1 Frame Step"))
						FramesToStep = 1;

					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					if (ImGui::MenuItem("Change To 5 Frame Steps"))
						FramesToStep = 5;

					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					if (ImGui::MenuItem("Change To 10 Frame Steps"))
						FramesToStep = 10;

					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					if (ImGui::MenuItem("Change To 50 Frame Steps"))
						FramesToStep = 50;

					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					if (ImGui::MenuItem("Change To 100 Frame Steps"))
						FramesToStep = 100;

					ImGui::EndMenu();
				}

				ImGui::Separator();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::MenuItem("Animate In Edit Mode...", "", &AnimateTexturesInEdit);
				if (AnimateTexturesInEdit)
				{
					Renderer2D::s_AnimateInEdit = true;
					Renderer3D::s_AnimateInEdit = true;
				}
				else
				{
					Renderer2D::s_AnimateInEdit = false;
					Renderer3D::s_AnimateInEdit = false;
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Window"))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::MenuItem("Editor Viewport", nullptr, &ShowEditorViewport);

				//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				//ImGui::MenuItem("Game Viewport", nullptr,  &ShowGameViewport);

				ImGui::Separator();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::MenuItem("Show Scene Hierarchy Window", nullptr, &SceneHierarchyPanel::ShowSceneHierarchy);

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::MenuItem("Show Property Window", nullptr, &SceneHierarchyPanel::ShowProperties);

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::MenuItem("Show Content Browser Window", nullptr, &ContentBrowserPanel::ShowContentBrowserPanel);

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::MenuItem("Show Log Window", nullptr, &LogPanel::ShowLogPanel);

				ImGui::Separator();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::MenuItem("Show Statistics Window", nullptr, &Show2DStatisticsViewport);

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::MenuItem("Show Settings Window", nullptr, &ShowSettingsViewport);

				ImGui::Separator();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::BeginMenu("Themes"))
				{
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					if (ImGui::MenuItem("Use Green Dark Theme", nullptr, &UseGreenDarkTheme))
					{
						UseOrangeDarkTheme = false;
						UseLightTheme = false;
						UseGoldDarkTheme = false;
						UseChocolateTheme = false;

						NZ_CORE_WARN("Using Green Dark Theme!");
						ImGuiLayer::SetDarkThemeColorsGreen();
					}

					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					if (ImGui::MenuItem("Use Orange Dark Theme", nullptr, &UseOrangeDarkTheme))
					{
						UseGreenDarkTheme = false;
						UseLightTheme = false;
						UseGoldDarkTheme = false;
						UseChocolateTheme = false;

						NZ_CORE_WARN("Using Orange Dark Theme!");
						ImGuiLayer::SetDarkThemeColorsOrange();
					}

					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					if (ImGui::MenuItem("Use Light Theme", nullptr, &UseLightTheme))
					{
						UseGreenDarkTheme = false;
						UseOrangeDarkTheme = false;
						UseGoldDarkTheme = false;
						UseChocolateTheme = false;

						NZ_CORE_WARN("Using Light Theme!");
						ImGuiLayer::SetLightThemeColors();
					}

					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					if (ImGui::MenuItem("Use Gold Dark Theme", nullptr, &UseGoldDarkTheme))
					{
						UseGreenDarkTheme = false;
						UseOrangeDarkTheme = false;
						UseLightTheme = false;
						UseChocolateTheme = false;

						NZ_CORE_WARN("Using Gold Dark Theme!");
						ImGuiLayer::SetGoldDarkThemeColors();
					}

					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					if (ImGui::MenuItem("Use Chocolate Theme", nullptr, &UseChocolateTheme))
					{
						UseGreenDarkTheme = false;
						UseOrangeDarkTheme = false;
						UseLightTheme = false;
						UseGoldDarkTheme = false;

						NZ_CORE_WARN("Using Chocolate Theme!");
						ImGuiLayer::SetChocolateThemeColors();
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("About..."))
					ShowAboutPopupWindow = true;

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		ImGuiWindowClass imguiWindow;
		imguiWindow.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoCloseButton;
		imguiWindow.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoWindowMenuButton;

		// Statistics panel
		if (Show2DStatisticsViewport)
		{
			ImGui::SetNextWindowClass(&imguiWindow);
			if (ImGui::Begin("2D Statistics", &Show2DStatisticsViewport))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::Text("Current Scene Name:");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::Text("%s", m_ActiveScene->GetName().c_str());
				ImGui::Separator();

				auto stats = Renderer2D::GetStats();

				//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				//ImGui::Text("Current FPS: %.3f", Application::Get().GetFPS());
				//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				//ImGui::Text("Current Frame Time: %.6fms", Application::Get().GetFrameTime());
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::Text("Current FPS: %.3f", m_ActiveScene->GetFPS());
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::Text("Current Frame Time: %.6fms", m_ActiveScene->GetFrameTime());
				//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				//ImGui::Text("Min. Frame Time: %.6fms", Application::Get().GetMinFrameTime());
				//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				//ImGui::Text("Max. Frame Time: %.6fms", Application::Get().GetMaxFrameTime());

				ImGui::Separator();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });

				if (ImGui::Button("Reset Min. Frame Time"))
					m_ActiveScene->SetMinFrameTime(FLT_MAX);

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);

				if (ImGui::Button("Reset Max. Frame Time"))
					m_ActiveScene->SetMaxFrameTime(0.0f);

				ImGui::PopStyleVar();

				ImGui::Separator();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::Text("Draw Calls: %d", stats.DrawCalls);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::Text("Quads: %d", stats.QuadCount);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

				ImGui::End();
			}
		}

		// Settings panel
		if (ShowSettingsViewport)
		{
			ImGui::SetNextWindowClass(&imguiWindow);
			if (ImGui::Begin("Settings", &ShowSettingsViewport))
			{
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 4.0f, ImGui::GetCursorPosY() + 4.0f));
				ImGui::Checkbox("Show Physics Colliders", &m_ShowPhysicsColliders);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				ImGui::SliderFloat("Debug Line Thickness", &m_PhysicsDebugLineThickness, 0.1f, 10.0f);

				ImGui::End();
			}
		}

		if (ShowEditorViewport)
		{
			ImGui::SetNextWindowClass(&imguiWindow);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			if (ImGui::Begin("Viewport", &ShowEditorViewport))
			{
				auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
				auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
				auto viewportOffset = ImGui::GetWindowPos();
				m_EditorViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
				m_EditorViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

				m_EditorViewportFocused = ImGui::IsWindowFocused();
				m_EditorViewportHovered = ImGui::IsWindowHovered();

				Application::Get().GetImGuiLayer()->BlockEvents(!m_EditorViewportHovered && !m_ViewportToolbarHovered);

				m_EditorCamera->ViewportFocused = m_EditorViewportFocused;
				m_EditorCamera->ViewportHovered = m_EditorViewportHovered;

				ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
				m_EditorViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

				uint64_t textureID = 0;
				textureID = m_FramebufferEditor->GetColorAttachmentRendererID();

				ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_EditorViewportSize.X, m_EditorViewportSize.Y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						AssetHandle handle = *(AssetHandle*)payload->Data;

						if (AssetManager::GetAssetType(handle) == AssetType::Scene)
						{
							if (m_SceneState != SceneState::Edit)
								OnSceneStop();

							OpenScene(handle);
						}
						else if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
						{
							if (m_HoveredEntity && m_HoveredEntity.HasComponent<SpriteRendererComponent>())
							{
								m_HoveredEntity.GetComponent<SpriteRendererComponent>().TextureHandle = handle;
							}
							else if (m_HoveredEntity && m_HoveredEntity.HasComponent<CircleRendererComponent>())
							{
								m_HoveredEntity.GetComponent<CircleRendererComponent>().TextureHandle = handle;
							}
							else if (m_HoveredEntity && m_HoveredEntity.HasComponent<ParticleSystemComponent>())
							{
								m_HoveredEntity.GetComponent<ParticleSystemComponent>().TextureHandle = handle;
							}
							else if (m_HoveredEntity && m_HoveredEntity.HasComponent<ButtonWidgetComponent>())
							{
								m_HoveredEntity.GetComponent<ButtonWidgetComponent>().TextureHandle = handle;
							}
							else if (m_HoveredEntity && m_HoveredEntity.HasComponent<CircleWidgetComponent>())
							{
								m_HoveredEntity.GetComponent<CircleWidgetComponent>().TextureHandle = handle;
							}
							else if (!m_HoveredEntity)
							{
								Entity entity = m_ActiveScene->CreateEntity("Sprite Entity");
								entity.AddComponent<SpriteRendererComponent>();
								entity.GetComponent<SpriteRendererComponent>().TextureHandle = handle;
							}
						}
						else if (AssetManager::GetAssetType(handle) == AssetType::Video)
						{
							if (m_HoveredEntity && m_HoveredEntity.HasComponent<VideoRendererComponent>())
							{
								m_HoveredEntity.GetComponent<VideoRendererComponent>().Video = handle;
							}
							else if (!m_HoveredEntity)
							{
								Entity entity = m_ActiveScene->CreateEntity("Video Entity");
								entity.AddComponent<VideoRendererComponent>();
								entity.GetComponent<VideoRendererComponent>().Video = handle;
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				// Gizmos
				if (m_SceneState == SceneState::Edit)
				{
					m_SelectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();

					if (m_SelectedEntity && m_GizmoType != -1)
					{
						ImGuizmo::SetOrthographic(false);
						ImGuizmo::SetDrawlist();
						ImGuizmo::SetRect(m_EditorViewportBounds[0].X, m_EditorViewportBounds[0].Y, m_EditorViewportBounds[1].X - m_EditorViewportBounds[0].X, m_EditorViewportBounds[1].Y - m_EditorViewportBounds[0].Y);

						// Editor camera
						//const rtmcpp::Mat4 cameraProjection = rtmcpp::Transpose(m_EditorCamera->GetProjection());
						//rtmcpp::Mat4 cameraView = rtmcpp::Transpose(m_EditorCamera->GetViewMatrix());
						const rtmcpp::Mat4 cameraProjection = m_EditorCamera->GetProjection();
						rtmcpp::Mat4 cameraView = m_EditorCamera->GetViewMatrix();

						// Entity Transform
						auto& tc = m_SelectedEntity.GetComponent<TransformComponent>();
						rtmcpp::Mat4 transform = tc.GetTransform();

						// Snapping
						bool snap = Input::IsKeyPressed(Key::LeftControl);
						float snapValue = 0.5f; // Snap to 0.5m for translation/scale

						// Snap to 45 degrees for rotation
						if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
							snapValue = 45.0f;

						float snapValues[3] = { snapValue, snapValue, snapValue };

						ImGuizmo::Manipulate(&(cameraView.Value.x_axis.m128_f32[0]), &(cameraProjection.Value.x_axis.m128_f32[0]),//glm::value_ptr(cameraProjection),
							(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, &(transform.Value.x_axis.m128_f32[0]),//glm::value_ptr(transform),
							nullptr, snap ? snapValues : nullptr);

						if (ImGuizmo::IsUsing() && !Input::IsKeyPressed(Key::LeftAlt))
						{
							//rtmcpp::Vec3 translation, rotation, scale;
							rtm::qvvf qvvf = rtm::qvv_from_matrix(rtm::matrix_cast(transform.Value));
							//Math::DecomposeTransform(transform, translation, rotation, scale);
							
							//rtmcpp::Vec3 deltaRotation = rtm::vector_sub(rotation.Value, tc.Rotation.Value);
							rtmcpp::Vec3 deltaRotation = rtm::vector_sub(qvvf.rotation, tc.Rotation.Value);
							//tc.Translation = rtmcpp::Vec4{ translation, 1.0f };
							//tc.Translation = rtmcpp::Vec4{ 0.0f, 2.0f, 0.0f, 1.0f };
							tc.Translation = rtmcpp::Vec4{ qvvf.translation, 1.0f };
							tc.Rotation += deltaRotation;
							//tc.Scale = scale;
							//tc.Scale = rtmcpp::Vec3{ 1.0f, 1.0f, 1.0f };
							tc.Scale = qvvf.scale;
						}
					}
				}

				ImGui::SetNextWindowPos(ImVec2(m_EditorViewportBounds[0].X, m_EditorViewportBounds[0].Y));
				ImGui::SetNextWindowSize(ImVec2(m_EditorViewportSize.X, m_EditorViewportSize.Y));

				ImGui::SetNextWindowBgAlpha(0.0f);

				float iconSize = 21.333333f;
				bool open = false;
				if (ImGui::Begin("Canvas", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
				{
					ImGui::SetCursorScreenPos(ImVec2(m_EditorViewportBounds[0].X + 18.0f, m_EditorViewportBounds[0].Y + 10.0f));

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

					if (m_GizmoType == -1)
						ImGui::ImageButton((ImTextureID)(uint64_t)m_IconNoGizmo->GetRendererID(), ImVec2(iconSize * Window::s_HighDPIScaleFactor, iconSize * Window::s_HighDPIScaleFactor), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ConvertColorFromByteToFloats(83, 179, 5, 255));
					else
						ImGui::ImageButton((ImTextureID)(uint64_t)m_IconNoGizmo->GetRendererID(), ImVec2(iconSize * Window::s_HighDPIScaleFactor, iconSize * Window::s_HighDPIScaleFactor), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

					ImGui::PopStyleColor();

					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

					if (m_GizmoType == ImGuizmo::OPERATION::TRANSLATE)
						ImGui::ImageButton((ImTextureID)(uint64_t)m_IconTranslate->GetRendererID(), ImVec2(iconSize * Window::s_HighDPIScaleFactor, iconSize * Window::s_HighDPIScaleFactor), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ConvertColorFromByteToFloats(83, 179, 5, 255));
					else
						ImGui::ImageButton((ImTextureID)(uint64_t)m_IconTranslate->GetRendererID(), ImVec2(iconSize * Window::s_HighDPIScaleFactor, iconSize * Window::s_HighDPIScaleFactor), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

					ImGui::PopStyleColor();

					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

					if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
						ImGui::ImageButton((ImTextureID)(uint64_t)m_IconRotate->GetRendererID(), ImVec2(iconSize * Window::s_HighDPIScaleFactor, iconSize * Window::s_HighDPIScaleFactor), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ConvertColorFromByteToFloats(83, 179, 5, 255));
					else
						ImGui::ImageButton((ImTextureID)(uint64_t)m_IconRotate->GetRendererID(), ImVec2(iconSize * Window::s_HighDPIScaleFactor, iconSize * Window::s_HighDPIScaleFactor), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

					ImGui::PopStyleColor();

					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

					if (m_GizmoType == ImGuizmo::OPERATION::SCALE)
						ImGui::ImageButton((ImTextureID)(uint64_t)m_IconScale->GetRendererID(), ImVec2(iconSize * Window::s_HighDPIScaleFactor, iconSize * Window::s_HighDPIScaleFactor), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ConvertColorFromByteToFloats(83, 179, 5, 255));
					else
						ImGui::ImageButton((ImTextureID)(uint64_t)m_IconScale->GetRendererID(), ImVec2(iconSize * Window::s_HighDPIScaleFactor, iconSize * Window::s_HighDPIScaleFactor), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

					ImGui::PopStyleColor();
					ImGui::End();
				}

				UI_CentralToolbar();
			}
			ImGui::End();
			ImGui::PopStyleVar();
		}

		m_SceneHierarchyPanel.OnImGuiRender();
		m_LogPanel.OnImGuiRender();
		m_ContentBrowserPanel->OnImGuiRender();

		// Set fonts to something so they can be used with About window!
		if (io.Fonts->Fonts[0] != nullptr && m_BoldFont != io.Fonts->Fonts[0])
			m_BoldFont = io.Fonts->Fonts[0];

		if (io.Fonts->Fonts[1] != nullptr && m_LargeFont != io.Fonts->Fonts[1])
			m_LargeFont = io.Fonts->Fonts[1];

		if (io.Fonts->Fonts[4] != nullptr && m_LargerFont != io.Fonts->Fonts[4])
			m_LargerFont = io.Fonts->Fonts[4];

		UI_AboutPopup();
		UI_NewProjectPopup();
		UI_EditProjectSettingsPopup();
		UI_EditGravityPopup();
	}

	// The original code for the Toolbar has been "borrowed" from Hazel Dev and has been modified to fit my needs!
	void EditorLayer::UI_CentralToolbar()
	{
		UI::PushID();

		UI::ScopedStyle disableSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		UI::ScopedStyle disableWindowBorder(ImGuiStyleVar_WindowBorderSize, 0.0f);
		UI::ScopedStyle windowRounding(ImGuiStyleVar_WindowRounding, 4.0f);
		UI::ScopedStyle disablePadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		const float buttonSize = 18.0f + 5.0f;
		const float edgeOffset = 4.0f;
		const float windowOffset = 0.0f;
		const float windowHeight = 44.0f; // annoying limitation of ImGui, window can't be smaller than 32 pixels // Originally 32.0f!
		const float numberOfButtons = 4.0f;
		const float backgroundWidth = edgeOffset * 6.0f + buttonSize * numberOfButtons + edgeOffset * (numberOfButtons - 1.0f) * 2.0f;

		float toolbarX = (m_EditorViewportBounds[0].X + m_EditorViewportBounds[1].X) / 2.0f;

		ImGui::SetNextWindowPos(ImVec2(toolbarX - (backgroundWidth / 2.0f), m_EditorViewportBounds[0].Y + windowOffset));

		ImGui::SetNextWindowSize(ImVec2(backgroundWidth, windowHeight));
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::Begin("##viewport_central_toolbar", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking);

		m_ViewportToolbarHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!m_EditorViewportHovered && !m_ViewportToolbarHovered);

		// A hack to make icon panel appear smaller than minimum allowed by ImGui size
		// Filling the background for the desired 26px height
		const float desiredHeight = 39.0f + 5.0f; // Originally 26.0f + 5.0f!
		ImRect background = UI::RectExpanded(ImGui::GetCurrentWindow()->Rect(), 0.0f, -(windowHeight - desiredHeight) / 2.0f);
		ImGui::GetWindowDrawList()->AddRectFilled(background.Min, background.Max, IM_COL32(15, 15, 15, 127), 4.0f);

		ImGui::BeginVertical("##viewport_central_toolbarV", { backgroundWidth, ImGui::GetContentRegionAvail().y });
		ImGui::Spring();
		ImGui::BeginHorizontal("##viewport_central_toolbarH", { backgroundWidth, ImGui::GetContentRegionAvail().y });
		ImGui::Spring();
		{
			bool isPaused = m_ActiveScene->IsPaused();
			bool hasStopped = false;

			UI::ScopedStyle enableSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(edgeOffset * 2.0f, 0));

			const ImColor c_ButtonTint = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
			const ImColor c_ButtonHoveredTint = ConvertColorFromByteToFloats(93, 197, 5, 255);
			const ImColor c_ButtonPressedTint = ConvertColorFromByteToFloats(73, 159, 5, 255);
			const ImColor c_ButtonDisabledTint = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);

			auto toolbarButton = [buttonSize](const RefPtr<Texture2D>& icon, const ImColor& tint, const ImColor& hoveredTint, const ImColor& pressedTint, float paddingY = 0.0f)
			{
				const float height = (std::min)((float)icon->GetHeight(), buttonSize) - paddingY * 2.0f;
				const float width = (float)icon->GetWidth() / (float)icon->GetHeight() * height;
				const bool clicked = ImGui::InvisibleButton(UI::GenerateID(), ImVec2(width, height));
				UI::DrawButtonImage(icon, tint, hoveredTint, pressedTint, UI::RectOffset(UI::GetItemRect(), 0.0f, paddingY));

				return clicked;
			};

			RefPtr<Texture2D> buttonTex = m_IconPlay;

			if (m_SceneState == SceneState::Play && !isPaused)
				buttonTex = m_IconPause;
			else if (isPaused)
				buttonTex = m_IconPlay;

			RefPtr<Texture2D> buttonSimTex = m_SceneState == SceneState::Simulate ? m_IconStop : m_IconSimulate;

			if (m_SceneState == SceneState::Simulate && !isPaused)
				buttonSimTex = m_IconPause;
			else if (isPaused)
				buttonSimTex = m_IconSimulate;

			if (toolbarButton(buttonTex, c_ButtonTint, c_ButtonHoveredTint, c_ButtonPressedTint))
			{
				if (m_SelectedEntity)
				{
					m_HoveredEntity = {};
					m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
					m_SelectedEntity = {};
				}

				if (ScriptGlue::GetSelectedEntity())
				{
					ScriptGlue::SetHoveredEntity({});
					m_SceneHierarchyPanel.SetSelectedEntity(ScriptGlue::GetHoveredEntity());
					ScriptGlue::SetSelectedEntity({});
				}

				if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate)
				{
					OnScenePlay();
					hasStopped = false;
				}
				else if (m_SceneState == SceneState::Play)
				{
					m_ActiveScene->SetPaused(!isPaused);
				}
			}
			if (m_SceneState == SceneState::Edit)
				UI::SetTooltip("Play");
			else if (isPaused)
				UI::SetTooltip("Resume");
			else if (m_SceneState == SceneState::Play && !isPaused)
				UI::SetTooltip("Pause");

			if (m_SceneState == SceneState::Edit)
				ImGui::BeginDisabled();

			if (toolbarButton(m_IconStop, m_SceneState == SceneState::Edit ? c_ButtonDisabledTint : c_ButtonTint, c_ButtonHoveredTint, c_ButtonPressedTint))
			{
				if (m_SceneState != SceneState::Edit)
					hasStopped = true;
			}

			UI::SetTooltip("Stop");

			if (m_SceneState == SceneState::Edit)
				ImGui::EndDisabled();

			if (hasStopped)
			{
				OnSceneStop();
				hasStopped = false;
			}

			if (toolbarButton(buttonSimTex, c_ButtonTint, c_ButtonHoveredTint, c_ButtonPressedTint))
			{
				if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play)
					OnSceneSimulate();
				else if (m_SceneState == SceneState::Simulate)
					m_ActiveScene->SetPaused(!isPaused);
			}

			if (m_SceneState == SceneState::Edit)
				UI::SetTooltip("Simulate Physics");
			else if (isPaused)
				UI::SetTooltip("Resume Simulation");
			else if (m_SceneState == SceneState::Simulate && !isPaused)
				UI::SetTooltip("Pause Simulation");

			std::string NumStepHint = "Step";

			if (FramesToStep == 1)
				NumStepHint = "Step " + std::to_string(FramesToStep) + " Frame";
			else
				NumStepHint = "Step " + std::to_string(FramesToStep) + " Frames";

			if (m_SceneState == SceneState::Edit)
				ImGui::BeginDisabled();

			if (toolbarButton(m_IconStep, m_SceneState == SceneState::Edit ? c_ButtonDisabledTint : c_ButtonTint, c_ButtonHoveredTint, c_ButtonPressedTint))
			{
				if (isPaused)
					m_ActiveScene->Step(FramesToStep);
			}

			UI::SetTooltip(m_SceneState == SceneState::Edit ? "Step" : NumStepHint.c_str());

			if (m_SceneState == SceneState::Edit)
				ImGui::EndDisabled();
		}
		ImGui::Spring();
		ImGui::EndHorizontal();
		ImGui::Spring();
		ImGui::EndVertical();

		ImGui::End();

		UI::PopID();
	}

	void EditorLayer::UI_AboutPopup()
	{
		if (ShowAboutPopupWindow)
		{
			ImGui::OpenPopup("About...##AboutPopup");
			ShowAboutPopupWindow = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2{ 675,0 });
		if (ImGui::BeginPopupModal("About...##AboutPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (!ShowingAboutPopupWindow)
				ShowingAboutPopupWindow = true;

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
			ImGui::Image((ImTextureID)(uint64_t)m_Logo->GetRendererID(), ImVec2(64.0f * Window::s_HighDPIScaleFactor, 64.0f * Window::s_HighDPIScaleFactor), ImVec2(0, 1), ImVec2(1, 0));

			ImGui::PushFont(m_LargeFont);
			ImGui::Text("Nutcrackz v0.1");
			ImGui::PopFont();

			ImGui::Separator();
			ImGui::TextWrapped("Nutcrackz is a 2D game engine for Windows.");
			
			ImGui::Separator();			
			ImGui::PushFont(m_BoldFont);
			ImGui::Text("Mainly coded by: VagueLobster (2019 - ).");
			ImGui::PopFont();
			
			ImGui::Text("");
			ImGui::Text("A lot of the code from TheCherno's");
			UI::TextURL("Game-Engine-Series", "https://www.youtube.com/c/ChernoUnplugged", true, true, false, m_HoveringChernoUnplugged);
			ImGui::Text("on YouTube has been used as a startup, and some of it modified.");
			ImGui::Text("Also, a portion of TheCherno's Hazel-Dev's code has been");
			ImGui::Text("borrowed and edited, mainly the toolbar inside the viewport");
			ImGui::Text("+ some other stuff. I try to limit the borrowing.");
			ImGui::Text("");
			ImGui::Text("The script engine has been borrowed from");
			
			ImGui::SameLine();
			UI::TextURL("StudioCherno", "https://github.com/StudioCherno/Coral", true, true, true, m_HoveringStudioChernosGithub);
			ImGui::Text("on Github.");
			ImGui::Text("The Log panel has been borrowed from");
			
			ImGui::SameLine();
			UI::TextURL("Toastmastern", "https://github.com/Toastmastern87/Toast", true, true, true, m_HoveringToastmasternsGithub);
			ImGui::Text("on Github, though, it may be replaced at some point.");
			ImGui::Text("The UI has been borrowed from");
			
			ImGui::SameLine();
			UI::TextURL("ToniPlays' Hazard", "https://github.com/ToniPlays/Hazard", true, true, true, m_HoveringToniPlaysGithub);
			ImGui::Text("on Github.");
			ImGui::Separator();
			
			ImGui::SetCursorPosX(static_cast<float>(ImGui::GetWindowWidth() - 130.0f));
			if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
			{
				ShowingAboutPopupWindow = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void EditorLayer::UI_NewProjectPopup()
	{
		if (ShowNewProjectPopupWindow)
		{
			ImGui::OpenPopup("New Project...##NewProjectPopup");
			ShowNewProjectPopupWindow = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2{ 675,0 });
		if (ImGui::BeginPopupModal("New Project...##NewProjectPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::PushFont(m_LargerFont);
			ImGui::Text("Create A New Project...");
			ImGui::PopFont();

			char projectNameBuffer[256];
			strcpy_s(projectNameBuffer, g_ProjectName.c_str());
			char assetsBuffer[256];
			strcpy_s(assetsBuffer, g_AssetsFolder.c_str());
			char assemblyBuffer[256];
			strcpy_s(assemblyBuffer, g_ScriptModulePath.c_str());

			ImGui::Separator();
			ImGui::PushFont(m_LargeFont);
			ImGui::Text("Set Project Name:");
			ImGui::PopFont();
			
			ImGui::PushFont(m_BoldFont);
			
			if (ImGui::InputText("##InputProjectName", projectNameBuffer, 256))
				g_ProjectName = projectNameBuffer;
			
			ImGui::PopFont();
			
			ImGui::PushFont(m_LargeFont);
			ImGui::Text("Set Assets Root Folder:");
			ImGui::PopFont();
			
			ImGui::PushFont(m_BoldFont);
			
			if (ImGui::InputText("##InputAssetsFolder", assetsBuffer, 256))
				g_AssetsFolder = assetsBuffer;
			
			ImGui::SameLine();
			if (ImGui::Button("Browse##ButtonBrowseAssetsFolder"))
			{
				g_AssetsFolder = FindAssetsFolder();
				strcpy_s(assetsBuffer, g_AssetsFolder.c_str());
			}
			ImGui::PopFont();
			
			ImGui::PushFont(m_LargeFont);
			ImGui::Text("Set Script Module Path:");
			ImGui::PopFont();
			
			ImGui::PushFont(m_BoldFont);
			
			if (ImGui::InputText("##InputScriptModulePath", assemblyBuffer, 256))
				g_ScriptModulePath = assemblyBuffer;
			
			ImGui::SameLine();
			if (ImGui::Button("Browse##ButtonBrowseScriptModulePath"))
			{
				g_ScriptModulePath = FindFilepath("Nutcrackz Script Modules (*.dll)\0*.dll\0");
				strcpy_s(assemblyBuffer, g_ScriptModulePath.c_str());
			}
			ImGui::PopFont();
			
			ImGui::Separator();
			ImGui::PushFont(m_BoldFont);
			ImGui::SetCursorPosX(static_cast<float>(ImGui::GetWindowWidth() - 382.0f));
			if (ImGui::Button("Create New Project", ImVec2(240.0f, 0.0f)))
			{
				if (!g_ProjectName.empty() && !g_AssetsFolder.empty() && !g_ScriptModulePath.empty())
				{
					m_ProjectName = g_ProjectName;
					m_AssetsFolder = g_AssetsFolder;
					m_ScriptModulePath = g_ScriptModulePath;
					NewProject();
				}

				g_ProjectName = "";
				g_AssetsFolder = "";
				g_ScriptModulePath = "";

				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				g_ProjectName = "";
				g_AssetsFolder = "";
				g_ScriptModulePath = "";

				ImGui::CloseCurrentPopup();
			}
			ImGui::PopFont();

			ImGui::EndPopup();
		}
	}

	void EditorLayer::UI_EditProjectSettingsPopup()
	{
		if (ShowEditProjectPopupWindow)
		{
			if (m_ProjectName.empty())
				m_ProjectName = Project::GetActive()->GetConfig().Name;

			if (m_AssetsFolder.empty())
				m_AssetsFolder = Project::GetActive()->GetConfig().AssetDirectory.string();

			if (m_ScriptModulePath.empty())
				m_ScriptModulePath = Project::GetActive()->GetConfig().ScriptModulePath.string();

			if (g_EditProjectName != m_ProjectName)
				g_EditProjectName = m_ProjectName;

			if (g_EditAssetsFolder != m_AssetsFolder)
				g_EditAssetsFolder = m_AssetsFolder;

			if (g_EditScriptModulePath != m_ScriptModulePath)
				g_EditScriptModulePath = m_ScriptModulePath;

			ImGui::OpenPopup("Edit Project...##EditProjectPopup");
			ShowEditProjectPopupWindow = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2{ 537, 475 });
		if (ImGui::BeginPopupModal("Edit Project...##EditProjectPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::PushFont(m_LargerFont);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			ImGui::Text("Edit Current Project...");
			ImGui::PopFont();

			char projectNameBuffer[256];
			strcpy_s(projectNameBuffer, g_EditProjectName.c_str());
			char startSceneBuffer[256];
			strcpy_s(startSceneBuffer, g_EditStartScene.c_str());
			char assetsBuffer[256];
			strcpy_s(assetsBuffer, g_EditAssetsFolder.c_str());
			char assemblyBuffer[256];
			strcpy_s(assemblyBuffer, g_EditScriptModulePath.c_str());

			ImGui::Separator();
			ImGui::PushFont(m_LargeFont);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			ImGui::Text("Set Project Name:");
			ImGui::PopFont();
			
			ImGui::PushFont(m_BoldFont);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });

			if (ImGui::InputText("##InputEditProjectName", projectNameBuffer, 256))
				g_EditProjectName = projectNameBuffer;

			ImGui::PopStyleVar();
			ImGui::PopFont();
			
			ImGui::PushFont(m_LargeFont);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			ImGui::Text("Set Start Scene:");
			ImGui::PopFont();
			
			ImGui::PushFont(m_BoldFont);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });

			if (ImGui::InputText("##InputEditStartScene", startSceneBuffer, 256))
				g_EditStartScene = startSceneBuffer;

			ImGui::PopStyleVar();
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });
			if (ImGui::Button("Browse##ButtonBrowseEditStartScene"))
			{
				g_EditStartScene = FindFilepath("Nutcrackz Scene (*.ncz)\0*.ncz\0Hazel Scene (*.hazel)\0*.hazel\0");
				strcpy_s(assetsBuffer, g_EditStartScene.c_str());
			}
			ImGui::PopStyleVar();
			ImGui::PopFont();
			
			ImGui::PushFont(m_LargeFont);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			ImGui::Text("Set Assets Root Folder:");
			ImGui::PopFont();
			
			ImGui::PushFont(m_BoldFont);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });
			
			if (ImGui::InputText("##InputEditAssetsFolder", assetsBuffer, 256))
				g_EditAssetsFolder = assetsBuffer;
			
			ImGui::PopStyleVar();
			
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });
			if (ImGui::Button("Browse##ButtonBrowseEditAssetsFolder"))
			{
				g_EditAssetsFolder = FindAssetsFolder();
				strcpy_s(assetsBuffer, g_EditAssetsFolder.c_str());
			}
			ImGui::PopStyleVar();
			ImGui::PopFont();
			
			ImGui::PushFont(m_LargeFont);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			ImGui::Text("Set Script Module Path:");
			ImGui::PopFont();
			
			ImGui::PushFont(m_BoldFont);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });
			if (ImGui::InputText("##InputEditScriptModulePath", assemblyBuffer, 256))
				g_EditScriptModulePath = assemblyBuffer;
			ImGui::PopStyleVar();
			
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });
			if (ImGui::Button("Browse##ButtonBrowseEditScriptModulePath"))
			{
				g_EditScriptModulePath = FindFilepath("Nutcrackz Script Modules (*.dll)\0*.dll\0");
				strcpy_s(assemblyBuffer, g_EditScriptModulePath.c_str());
			}
			ImGui::PopStyleVar();
			ImGui::PopFont();
			
			ImGui::Separator();
			ImGui::PushFont(m_BoldFont);
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowContentRegionMax().x - 391.0f, ImGui::GetWindowContentRegionMin().y + 380.0f));
			if (ImGui::Button("Update Project Settings", ImVec2(260.0f, 0.0f)))
			{
				if (!g_EditProjectName.empty())
				{
					m_ProjectName = g_EditProjectName;
					Project::GetActive()->GetConfig().Name = g_EditProjectName;
				}

				m_AssetsFolder = g_EditAssetsFolder;
				Project::GetActive()->GetConfig().AssetDirectory = g_EditAssetsFolder;

				if (!m_AssetsFolder.empty())
					m_ContentBrowserPanel = RefPtr<ContentBrowserPanel>::Create();

				m_ScriptModulePath = g_EditScriptModulePath;
				Project::GetActive()->GetConfig().ScriptModulePath = g_EditScriptModulePath;

				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowContentRegionMax().x - 125.0f, ImGui::GetWindowContentRegionMin().y + 380.0f));
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				g_EditProjectName = "";
				g_EditStartScene = "";
				g_EditAssetsFolder = "";
				g_EditScriptModulePath = "";

				ImGui::CloseCurrentPopup();
			}
			ImGui::PopFont();

			ImGui::EndPopup();
		}
	}

	void EditorLayer::UI_EditGravityPopup()
	{
		if (ShowEditGravityPopupWindow)
		{
			ImGui::OpenPopup("Change Gravity...##ChangeGravityPopup");
			ShowEditGravityPopupWindow = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2{ 444, 228 });
		if (ImGui::BeginPopupModal("Change Gravity...##ChangeGravityPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::PushFont(m_LargerFont);
			ImGui::Text("Change 2D Gravity...");
			ImGui::PopFont();

			ImGui::Separator();
			ImGui::PushFont(m_LargeFont);
			ImGui::Text("Set Gravity:");
			ImGui::PopFont();
			ImGui::PushFont(m_BoldFont);

			rtmcpp::Vec2 endGravity = m_Gravity;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);			
			if (ImGui::DragFloat2("##Gravity2D", &(endGravity.X), 0.01f, 0.0f, 0.0f))
				m_Gravity = endGravity;

			ImGui::PopFont();

			ImGui::Separator();

			ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 250.0f);
			if (ImGui::Button("Apply", ImVec2(120.0f, 0.0f)))
			{
				m_ActiveScene->SetPhysics2DGravity(m_Gravity);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 124.0f);
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				m_Gravity = m_ActiveScene->GetPhysics2DGravity();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void EditorLayer::QueueSceneTransition(AssetHandle handle)
	{
		m_PostSceneUpdateQueue.emplace_back([this, handle]() { OnSceneTransition(handle); });
	}

	void EditorLayer::OnEvent(Event& e)
	{
		//if (m_SceneState == SceneState::Edit)
		//	m_EditorCamera->OnEvent(e);

		m_SceneHierarchyPanel.OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(NZ_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(NZ_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
		//dispatcher.Dispatch<WindowDropEvent>(NZ_BIND_EVENT_FN(EditorLayer::OnWindowDrop));

	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// Shortcuts
		if (e.IsRepeat())
			return false;

		bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

		switch (e.GetKeyCode())
		{
		/*case Key::N:
		{
			if (control)
				NewScene();

			break;
		}
		case Key::O:
		{
			if (control)
				OpenProject();

			break;
		}
		case Key::S:
		{
			if (control)
			{
				if (shift)
					SaveSceneAs();
				else
					SaveScene();
			}

			break;
		}*/
		case Key::D:
		{
			if (control)
				OnDuplicateEntity();

			break;
		}

		// Gizmos
		case Key::Q:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = -1;
			break;
		}
		case Key::W:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		}
		case Key::E:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::ROTATE;
			break;
		}
		case Key::R:
		{
			if (control)
			{
				ReloadCSharp();
			}
			else
			{
				if (!ImGuizmo::IsUsing())
					m_GizmoType = ImGuizmo::OPERATION::SCALE;
			}
			break;
		}
		case Key::Delete:
		{
			if (Application::Get().GetImGuiLayer()->GetActiveWidgetID() == 0)
			{
				Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
				if (selectedEntity)
				{
					m_SceneHierarchyPanel.SetSelectedEntity({});
					m_ActiveScene->DestroyEntity(selectedEntity);
				}
			}

			break;
		}
		case Key::F5:
		{
			if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate)
				OnScenePlay();
			else if (m_SceneState == SceneState::Play)
				OnSceneStop();

			break;
		}
		case Key::F7:
		{
			if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play)
				OnSceneSimulate();
			else if (m_SceneState == SceneState::Simulate)
				OnSceneStop();

			break;
		}
		}

		return true;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() == Mouse::ButtonLeft)
		{
			if (!m_ViewportToolbarHovered)
			{
				if (m_EditorViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt) && m_SceneState == SceneState::Edit)
				{
					m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
				}
				else if (m_SceneState == SceneState::Play)
				{
					ScriptGlue::s_IsCursorInViewport = true;

					if (ScriptGlue::GetHoveredEntity() && ScriptGlue::GetHoveredEntity() != Entity())
					{
						m_SceneHierarchyPanel.SetSelectedEntity(ScriptGlue::GetHoveredEntity());
						ScriptGlue::SetSelectedEntity(ScriptGlue::GetHoveredEntity());
					}
					else if (ScriptGlue::GetHoveredEntity() == Entity())
					{
						m_SceneHierarchyPanel.SetSelectedEntity(Entity());
					}
				}
			}
			else if (m_EditorViewportHovered || m_ViewportToolbarHovered)
			{
				if (m_SceneState == SceneState::Play)
					ScriptGlue::s_IsCursorInViewport = false;
			}
		}

		return false;
	}

	void EditorLayer::OnOverlayRender()
	{
		if (m_SceneState == SceneState::Play)
		{
			Entity camera = m_ActiveScene->GetPrimaryCameraEntity();

			if (!camera)
				return;

			Renderer2D::BeginScene(*camera.GetComponent<CameraComponent>().Camera.Raw(), camera.GetComponent<TransformComponent>().GetTransform());
		}
		else if (m_SceneState != SceneState::Play)
		{
			Renderer2D::BeginScene(*m_EditorCamera.Raw());
		}

		// Calculate z index for translation
		float zIndex = 0.001f;
		rtmcpp::Vec3 cameraForwardDirection = m_EditorCamera->GetForwardDirection();

		// Draw selected entity outline 
		if (m_SceneState == SceneState::Edit)
		{
			if (m_SceneHierarchyPanel.GetContext() == m_EditorScene)
			{
				if (m_SelectedEntity && m_SceneHierarchyPanel.GetSelectedEntity() == m_SelectedEntity)
				{
					rtmcpp::Vec3 projectionCollider = rtm::vector_mul(rtmcpp::Vec3(zIndex, zIndex, zIndex).Value, cameraForwardDirection.Value);

					if (m_SelectedEntity.HasComponent<TransformComponent>())
					{
						auto& tc = m_SelectedEntity.GetComponent<TransformComponent>();
						float circleLineThickness = 0.01f * m_PhysicsDebugLineThickness;

						if (m_SelectedEntity.HasComponent<SpriteRendererComponent>() ||
							m_SelectedEntity.HasComponent<ParticleSystemComponent>() ||
							m_SelectedEntity.HasComponent<ButtonWidgetComponent>() ||
							m_SelectedEntity.HasComponent<VideoRendererComponent>())
						{
							rtmcpp::Vec3 translation = rtmcpp::Vec3{ tc.Translation.X, tc.Translation.Y, tc.Translation.Z + -projectionCollider.Z };
							rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(rtmcpp::Vec3{ tc.Rotation.Y, tc.Rotation.Z, tc.Rotation.X }));

							rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(tc.Scale))
								* rotation
								* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ translation.X, translation.Y, translation.Z }));

							Renderer2D::SetLineWidth(m_PhysicsDebugLineThickness);
							Renderer2D::DrawRect(transform, rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
						}
						else if (m_SelectedEntity.HasComponent<TextComponent>())
						{
							auto entity = m_SelectedEntity.GetComponent<TextComponent>();

							float minQuadY = entity.GetTextQuadMin().Y;
							rtmcpp::Vec2 maxQuad = entity.GetTextQuadMax();

							rtmcpp::Vec3 translation = rtmcpp::Vec3{ tc.Translation.X, tc.Translation.Y, tc.Translation.Z + -projectionCollider.Z };
							rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(rtmcpp::Vec3{ tc.Rotation.Y, tc.Rotation.Z, tc.Rotation.X }));

							rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(tc.Scale))
								* rotation
								* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ translation.X, translation.Y, translation.Z }));

							Renderer2D::SetLineWidth(m_PhysicsDebugLineThickness);

							Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.0f, minQuadY, 0.0f, 1.0f), rtmcpp::Vec4(maxQuad.X, minQuadY, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f)); // Bottom line
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(maxQuad.X, minQuadY, 0.0f, 1.0f), rtmcpp::Vec4(maxQuad.X, maxQuad.Y, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f)); // Right line
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(maxQuad.X, maxQuad.Y, 0.0f, 1.0f), rtmcpp::Vec4(0.0f, maxQuad.Y, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f)); // Top line
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.0f, maxQuad.Y, 0.0f, 1.0f), rtmcpp::Vec4(0.0f, minQuadY, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f)); // Left line
						}
						else if (m_SelectedEntity.HasComponent<CircleRendererComponent>() ||
							m_SelectedEntity.HasComponent<CircleWidgetComponent>())
						{
							if (circleLineThickness <= 0.01f)
								circleLineThickness = 0.01f;
							else if (circleLineThickness > 0.099f)
								circleLineThickness = 0.1f;
							else if (circleLineThickness > 0.01f && circleLineThickness < 0.099f)
								circleLineThickness += 0.01f;

							rtmcpp::Vec3 translation = rtmcpp::Vec3{ tc.Translation.X, tc.Translation.Y, tc.Translation.Z + -projectionCollider.Z };
							rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(rtmcpp::Vec3{ tc.Rotation.Y, tc.Rotation.Z, tc.Rotation.X }));

							rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(tc.Scale))
								* rotation
								* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ translation.X, translation.Y, translation.Z }));

							Renderer2D::DrawCircleLine(transform, rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f), circleLineThickness, 0.0f);
						}
						else if (m_SelectedEntity.HasComponent<TriangleRendererComponent>())
						{
							rtmcpp::Vec3 translation = rtmcpp::Vec3{ tc.Translation.X, tc.Translation.Y, tc.Translation.Z + -projectionCollider.Z };
							rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(rtmcpp::Vec3{ tc.Rotation.Y, tc.Rotation.Z, tc.Rotation.X }));

							rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(tc.Scale))
								* rotation
								* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ translation.X, translation.Y, translation.Z }));

							Renderer2D::SetLineWidth(m_PhysicsDebugLineThickness);

							Renderer2D::DrawLine(transform, rtmcpp::Vec4(-0.5f, -0.5f, 0.0f, 1.0f), rtmcpp::Vec4(0.5f, -0.5f, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.5f, -0.5f, 0.0f, 1.0f), rtmcpp::Vec4(0.0f, 0.5f, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.0f, 0.5f, 0.0f, 1.0f), rtmcpp::Vec4(-0.5f, -0.5f, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
						}
						else if (m_SelectedEntity.HasComponent<CameraComponent>())
						{
							rtmcpp::Vec3 translation = rtmcpp::Vec3{ tc.Translation.X, tc.Translation.Y, tc.Translation.Z + -projectionCollider.Z };
							rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(rtmcpp::Vec3{ tc.Rotation.Y, tc.Rotation.Z, tc.Rotation.X }));

							rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(tc.Scale))
								* rotation
								* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ translation.X, translation.Y, translation.Z }));

							Renderer2D::SetLineWidth(m_PhysicsDebugLineThickness);
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(-0.15f, -0.15f, 0.0f, 1.0f), rtmcpp::Vec4(0.15f, -0.15f, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.15f, -0.15f, 0.0f, 1.0f), rtmcpp::Vec4(0.15f, 0.15f, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.15f, 0.15f, 0.0f, 1.0f), rtmcpp::Vec4(-0.15f, 0.15f, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(-0.15f, 0.15f, 0.0f, 1.0f), rtmcpp::Vec4(-0.15f, -0.15f, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));

							Renderer2D::DrawLine(transform, rtmcpp::Vec4(-0.15f, -0.15f, 0.0f, 1.0f), rtmcpp::Vec4(-3.75f, -2.5f, -4.27f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(-0.15f, 0.15f, 0.0f, 1.0f), rtmcpp::Vec4(-3.75f, 2.5f, -4.27f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.15f, -0.15f, 0.0f, 1.0f), rtmcpp::Vec4(3.75f, -2.5f, -4.27f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.15f, 0.15f, 0.0f, 1.0f), rtmcpp::Vec4(3.75f, 2.5f, -4.27f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));

							Renderer2D::DrawLine(transform, rtmcpp::Vec4(-3.75f, -2.5f, -4.27f, 1.0f), rtmcpp::Vec4(3.75f, -2.5f, -4.27f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(3.75f, -2.5f, -4.27f, 1.0f), rtmcpp::Vec4(3.75f, 2.5f, -4.27f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(3.75f, 2.5f, -4.27f, 1.0f), rtmcpp::Vec4(-3.75f, 2.5f, -4.27f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
							Renderer2D::DrawLine(transform, rtmcpp::Vec4(-3.75f, 2.5f, -4.27f, 1.0f), rtmcpp::Vec4(-3.75f, -2.5f, -4.27f, 1.0f), rtmcpp::Vec4(1.0f, 0.5f, 0.0f, 1.0f));
						}
					}
				}
			}
		}

		if (m_ShowPhysicsColliders)
		{
			// Calculate z index for translation
			rtmcpp::Vec3 projectionCollider = cameraForwardDirection * rtmcpp::Vec3(zIndex, zIndex, zIndex);

			// Box Colliders
			{
				Renderer2D::SetLineWidth(m_PhysicsDebugLineThickness);

				auto filter = m_ActiveScene->GetECS().filter<TransformComponent, BoxCollider2DComponent>();
				filter.each([&](TransformComponent& tc, BoxCollider2DComponent& bc2d)
				{
					rtmcpp::Vec3 scale = tc.Scale * rtmcpp::Vec3(bc2d.Size.X * 2.0f, bc2d.Size.Y * 2.0f, 1.0f);
					rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(scale))
						* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3(bc2d.Offset.X, bc2d.Offset.Y, -projectionCollider.Z)))
						* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(tc.Rotation.Z, rtmcpp::Vec3(0.0f, 0.0f, 1.0f)))
						* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3(tc.Translation.X, tc.Translation.Y, tc.Translation.Z)));

					Renderer2D::DrawRect(transform, rtmcpp::Vec4(0, 1, 0, 1));
				});
			}

			// Circle Colliders
			{
				auto filter = m_ActiveScene->GetECS().filter<TransformComponent, CircleCollider2DComponent>();

				float circleLineThickness = 0.01f * m_PhysicsDebugLineThickness;

				if (circleLineThickness <= 0.01f)
					circleLineThickness = 0.01f;
				else if (circleLineThickness > 0.099f)
					circleLineThickness = 0.1f;
				else if (circleLineThickness > 0.01f && circleLineThickness < 0.099f)
					circleLineThickness += 0.01f;

				filter.each([&](TransformComponent& tc, CircleCollider2DComponent& cc2d)
				{
					rtmcpp::Vec3 scale = rtm::vector_mul(tc.Scale.Value, rtmcpp::Vec3(cc2d.Radius * 2.0f, cc2d.Radius * 2.0f, cc2d.Radius * 2.0f).Value);
					rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(scale))
						* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3(cc2d.Offset.X, cc2d.Offset.Y, -projectionCollider.Z)))
						* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(tc.Rotation.Z, rtmcpp::Vec3(0.0f, 0.0f, 1.0f)))
						* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3(tc.Translation.X, tc.Translation.Y, tc.Translation.Z)));

					Renderer2D::DrawCircleLine(transform, rtmcpp::Vec4(0.0f, 1.0f, 0.0f, 1.0f), circleLineThickness, 0.0f);
				});
			}

			// Triangle Colliders
			{
				Renderer2D::SetLineWidth(m_PhysicsDebugLineThickness);

				auto filter = m_ActiveScene->GetECS().filter<TransformComponent, TriangleCollider2DComponent>();
				filter.each([&](TransformComponent& tc, TriangleCollider2DComponent& tc2d)
				{
					rtmcpp::Vec3 scale = rtm::vector_mul(tc.Scale.Value, rtmcpp::Vec3(tc2d.Size.X * 2.0f, tc2d.Size.Y * 2.0f, 1.0f).Value);
					rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(scale))
						* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3(tc2d.Offset.X, tc2d.Offset.Y, -projectionCollider.Z)))
						* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(tc.Rotation.Z, rtmcpp::Vec3(0.0f, 0.0f, 1.0f)))
						* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3(tc.Translation.X, tc.Translation.Y, tc.Translation.Z)));

					Renderer2D::DrawLine(transform, rtmcpp::Vec4(-0.5f, -0.5f, 0.0f, 1.0f), rtmcpp::Vec4(0.5f, -0.5f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
					Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.5f, -0.5f, 0.0f, 1.0f), rtmcpp::Vec4(0.0f, 0.5f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
					Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.0f, 0.5f, 0.0f, 1.0f), rtmcpp::Vec4(-0.5f, -0.5f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
				});
			}

			// Capsule Colliders
			{
				Renderer2D::SetLineWidth(m_PhysicsDebugLineThickness);

				auto filter = m_ActiveScene->GetECS().filter<TransformComponent, CapsuleCollider2DComponent>();
				filter.each([&](TransformComponent& tc, CapsuleCollider2DComponent& bc2d)
				{
					rtmcpp::Vec3 scale = rtm::vector_mul(tc.Scale.Value, rtmcpp::Vec3(bc2d.Size.X, bc2d.Size.Y, 1.0f).Value);
					rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(scale))
						* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(bc2d.Rotation, rtmcpp::Vec3(0.0f, 0.0f, 1.0f)))
						* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3(bc2d.Offset.X, bc2d.Offset.Y, -projectionCollider.Z)))
						* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(tc.Rotation.Z, rtmcpp::Vec3(0.0f, 0.0f, 1.0f)))
						* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3(tc.Translation.X, tc.Translation.Y, tc.Translation.Z)));

					Renderer2D::DrawLine(transform, rtmcpp::Vec4(-0.35f, 1.0f, 0.0f, 1.0f), rtmcpp::Vec4(0.35f, 1.0f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
					Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.35f, 1.0f, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, 0.65f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
					Renderer2D::DrawLine(transform, rtmcpp::Vec4(1.0f, 0.65f, 0.0f, 1.0f), rtmcpp::Vec4(1.0f, -0.65f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
					Renderer2D::DrawLine(transform, rtmcpp::Vec4(1.0f, -0.65f, 0.0f, 1.0f), rtmcpp::Vec4(0.35f, -1.0f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
					Renderer2D::DrawLine(transform, rtmcpp::Vec4(0.35f, -1.0f, 0.0f, 1.0f), rtmcpp::Vec4(-0.35f, -1.0f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
					Renderer2D::DrawLine(transform, rtmcpp::Vec4(-0.35f, -1.0f, 0.0f, 1.0f), rtmcpp::Vec4(-1.0f, -0.65f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
					Renderer2D::DrawLine(transform, rtmcpp::Vec4(-1.0f, -0.65f, 0.0f, 1.0f), rtmcpp::Vec4(-1.0f, 0.65f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
					Renderer2D::DrawLine(transform, rtmcpp::Vec4(-1.0f, 0.65f, 0.0f, 1.0f), rtmcpp::Vec4(-0.35f, 1.0f, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
				});
			}

			// Mesh Colliders
			{
				Renderer2D::SetLineWidth(m_PhysicsDebugLineThickness);

				auto filter = m_ActiveScene->GetECS().filter<TransformComponent, MeshCollider2DComponent>();
				filter.each([&](TransformComponent& tc, MeshCollider2DComponent& mc2d)
				{
					rtmcpp::Vec3 scale = rtm::vector_mul(tc.Scale.Value, rtmcpp::Vec3(mc2d.Size.X, mc2d.Size.Y, 1.0f).Value);
					rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(scale))
						* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(mc2d.Rotation, rtmcpp::Vec3(0.0f, 0.0f, 1.0f)))
						* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3(mc2d.Offset.X, mc2d.Offset.Y, -projectionCollider.Z)))
						* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(tc.Rotation.Z, rtmcpp::Vec3(0.0f, 0.0f, 1.0f)))
						* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3(tc.Translation.X, tc.Translation.Y, tc.Translation.Z)));

					for (uint32_t i = 0; i < mc2d.Positions.size(); i++)
					{
						if (mc2d.Positions.size() >= 3)
						{
							if (i == mc2d.Positions.size() - 1)
								Renderer2D::DrawLine(transform, rtmcpp::Vec4(mc2d.Positions[i].X, mc2d.Positions[i].Y, 0.0f, 1.0f), rtmcpp::Vec4(mc2d.Positions[0].X, mc2d.Positions[0].Y, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));

							if ((i + 1) < mc2d.Positions.size())
								Renderer2D::DrawLine(transform, rtmcpp::Vec4(mc2d.Positions[i].X, mc2d.Positions[i].Y, 0.0f, 1.0f), rtmcpp::Vec4(mc2d.Positions[i + 1].X, mc2d.Positions[i + 1].Y, 0.0f, 1.0f), rtmcpp::Vec4(0, 1, 0, 1));
						}
					}
				});
			}
		}

		Renderer2D::EndScene();
	}

	void EditorLayer::NewProject()
	{
		Project::New();

		if (!m_ProjectName.empty())
			Project::GetActive()->GetConfig().Name = m_ProjectName;

		if (!m_AssetsFolder.empty())
		{
			Project::GetActive()->GetConfig().AssetDirectory = m_AssetsFolder;
			m_ContentBrowserPanel = RefPtr<ContentBrowserPanel>::Create();
		}

		if (!m_ScriptModulePath.empty())
			Project::GetActive()->GetConfig().ScriptModulePath = m_ScriptModulePath;

		NewScene();
		m_EditorProjectPath = "";
	}

	bool EditorLayer::OpenProject()
	{
		std::string filepath = FileDialogs::OpenFile("Nutcrackz Project (*.nproj)\0*.nproj\0Hazel Project (*.hproj)\0*.hproj\0");
		if (filepath.empty())
			return false;

		OpenProject(filepath);
		return true;
	}

	void EditorLayer::OpenProject(const std::filesystem::path& path)
	{
		if (Project::Load(path))
		{
			AssetHandle startScene = Project::GetActive()->GetConfig().StartScene;
			if (startScene)
				OpenScene(startScene);

			m_EditorProjectPath = path;
			m_ContentBrowserPanel = RefPtr<ContentBrowserPanel>::Create(/*Project::GetActive()*/);
		}
	}

	void EditorLayer::SaveProject()
	{
		if (!m_EditorProjectPath.empty())
			SerializeProject(Project::GetActive(), m_EditorProjectPath);
		else
			SaveProjectAs();
	}

	void EditorLayer::SaveProjectAs()
	{
		std::string filepath = FileDialogs::SaveFile("Nutcrackz Project (*.nproj)\0*.nproj\0Hazel Project (*.hproj)\0*.hproj\0");
		if (!filepath.empty())
			SerializeProject(Project::GetActive(), filepath);
	}

	void EditorLayer::SerializeProject(RefPtr<Project> project, const std::filesystem::path& path)
	{
		NZ_CORE_ASSERT(!path.empty());

		if (path.extension().string() == ".hproj" || path.extension().string() == ".nproj")
		{
			std::string finalPath = "";

			if (!m_EditorScenePath.empty())
			{
				std::filesystem::path serializeFilepath = Project::GetActiveProjectDirectory().parent_path() / m_EditorScenePath;
				finalPath = serializeFilepath.string();
				std::replace(finalPath.begin(), finalPath.end(), '\\', '/');

				NZ_CORE_WARN("Serialized Filepath: {0}", finalPath);
			}

			Project::SaveActive(path);
			m_EditorProjectPath = path;
		}
	}

	std::string EditorLayer::FindAssetsFolder()
	{
		std::string filepath = FileDialogs::OpenDirectory();
		if (!filepath.empty())
		{
			std::string finalPath = filepath;
			std::replace(finalPath.begin(), finalPath.end(), '\\', '/');
			return finalPath;
		}

		return std::string();
	}

	std::string EditorLayer::FindFilepath(const char* filter)
	{
		std::string filepath = FileDialogs::OpenFile(filter);
		if (!filepath.empty())
		{
			std::string finalPath = filepath;
			std::replace(finalPath.begin(), finalPath.end(), '\\', '/');
			return finalPath;
		}

		return std::string();
	}

	void EditorLayer::NewScene()
	{
		if (m_SceneState != SceneState::Edit)
			OnSceneStop();

		RefPtr<Scene> newScene = RefPtr<Scene>::Create();
		m_EditorScene = newScene;
		m_SceneHierarchyPanel.SetContext(m_EditorScene);

		m_Gravity = rtmcpp::Vec2(0.0f, -9.81f);
		m_EditorScene->SetPhysics2DGravity(m_Gravity);

		m_ActiveScene = m_EditorScene;
		m_EditorScenePath = "";
		SetEditorScenePath(std::filesystem::path());
		m_MainSceneCameraEntity = {};
	}

	void EditorLayer::OpenScene(AssetHandle handle)
	{
		NZ_CORE_ASSERT(handle);

		if (m_SceneState != SceneState::Edit)
			OnSceneStop();

		RefPtr<Scene> readOnlyScene = AssetManager::GetAsset<Scene>(handle);
		RefPtr<Scene> newScene = Scene::Copy(readOnlyScene);
		readOnlyScene->GetScriptStorage().CopyTo(newScene->GetScriptStorage());

		m_EditorScene = newScene;
		m_EditorScene->SetAssetHandle(handle);
		m_SceneHierarchyPanel.SetContext(m_EditorScene);
		
		m_ActiveScene = m_EditorScene;
		m_EditorScenePath = Project::GetActive()->GetEditorAssetManager()->GetFilePath(handle);
		SetEditorScenePath(Project::GetActive()->GetEditorAssetManager()->GetFilePath(handle));
		Project::GetActive()->UpdateEditorAssetManager();

		m_Gravity = m_ActiveScene->GetPhysics2DGravity();
		m_MainSceneCameraEntity = {};
	}

	void EditorLayer::SaveScene()
	{
		if (!m_EditorScenePath.empty())
			SerializeScene(m_EditorScene, m_EditorScenePath);
		else
			SaveSceneAs();
	}

	void EditorLayer::SerializeScene(RefPtr<Scene> scene, const std::filesystem::path& path)
	{
		SceneImporter::SaveScene(scene, path);
		SetEditorScenePath(path);
	}

	void EditorLayer::SaveSceneAs()
	{
		std::string filepath = FileDialogs::SaveFile("Nutcrackz Scene (*.ncz)\0*.ncz\0Hazel Scene (*.hazel)\0*.hazel\0");
		if (!filepath.empty())
		{
			SerializeScene(m_EditorScene, filepath);
			m_EditorScenePath = filepath;
			SetEditorScenePath(std::filesystem::path(filepath));
			Project::GetActive()->UpdateEditorAssetManager();
		}
	}

	void EditorLayer::OnScenePlay()
	{
		if (m_SceneState == SceneState::Simulate)
			OnSceneStop();

		m_SceneState = SceneState::Play;

		RefPtr<Scene> readOnlyScene = AssetManager::GetAsset<Scene>(m_EditorScene->GetAssetHandle());
		RefPtr<Scene> newScene = Scene::Copy(readOnlyScene);
		readOnlyScene->GetScriptStorage().CopyTo(newScene->GetScriptStorage());
		
		m_ActiveScene = newScene;
		m_ActiveScene->SetAssetHandle(m_EditorScene->GetAssetHandle());
		m_ActiveScene->SetName(m_SceneName.filename().string());

		Project::GetActive()->UpdateEditorAssetManager();

		m_ActiveScene->SetSceneTransitionCallback([this](AssetHandle handle) { QueueSceneTransition(handle); });
		m_ActiveScene->OnRuntimeStart();
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::OnSceneSimulate()
	{
		if (m_SceneState == SceneState::Play)
			OnSceneStop();

		m_SceneState = SceneState::Simulate;

		m_ActiveScene = Scene::Copy(m_EditorScene);

		m_ActiveScene->OnSimulationStart();
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::OnSceneStop()
	{
		NZ_CORE_ASSERT(m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate);

		if (m_SelectedEntity)
		{
			m_HoveredEntity = Entity();
			m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
		}

		if (ScriptGlue::GetSelectedEntity())
		{
			ScriptGlue::SetHoveredEntity(Entity());
			m_SceneHierarchyPanel.SetSelectedEntity(ScriptGlue::GetHoveredEntity());
		}

		if (m_SceneState == SceneState::Play)
			m_ActiveScene->OnRuntimeStop();
		else if (m_SceneState == SceneState::Simulate)
			m_ActiveScene->OnSimulationStop();

		m_SceneState = SceneState::Edit;

		m_ActiveScene = m_EditorScene;
		m_ActiveScene->SetAssetHandle(m_EditorScene->GetAssetHandle());
		m_ActiveScene->SetName(m_SceneName.filename().string());
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::ReloadCSharp()
	{
		ScriptStorage tempStorage;

		auto& scriptStorage = m_ActiveScene->GetScriptStorage();
		scriptStorage.CopyTo(tempStorage);
		scriptStorage.Clear();

		Project::GetActive()->ReloadScriptEngine();

		tempStorage.CopyTo(scriptStorage);
		tempStorage.Clear();

		scriptStorage.SynchronizeStorage();
	}

	void EditorLayer::OnDuplicateEntity()
	{
		if (m_SceneState != SceneState::Edit)
			return;

		Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
		if (selectedEntity)
		{
			Entity newEntity = m_EditorScene->DuplicateEntity(selectedEntity);
			m_SceneHierarchyPanel.SetSelectedEntity(newEntity);
		}
	}

	void EditorLayer::OnCreateEmptyEntity()
	{
		if (m_SceneState != SceneState::Edit)
			return;

		m_SelectedEntity = m_EditorScene->CreateEntity("Empty Entity");

		if (m_SceneHierarchyPanel.GetSelectedEntity() != m_SelectedEntity)
			m_SceneHierarchyPanel.SetSelectedEntity(m_SelectedEntity);
	}

	void EditorLayer::OnSceneTransition(AssetHandle handle)
	{
		//NZ_CORE_ASSERT(handle);

		m_ActiveScene->OnRuntimeStop();

		RefPtr<Scene> readOnlyScene = AssetManager::GetAsset<Scene>(handle);
		RefPtr<Scene> newScene = Scene::Copy(readOnlyScene);
		readOnlyScene->GetScriptStorage().CopyTo(newScene->GetScriptStorage());
		m_ActiveScene = newScene;
		m_ActiveScene->SetAssetHandle(handle);
		m_ActiveScene->SetName((Project::GetActiveAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetFilePath(handle)).filename().string());
		m_ActiveScene->SetSceneTransitionCallback([this](AssetHandle handle) { QueueSceneTransition(handle); });
		m_ActiveScene->OnRuntimeStart();
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		m_Gravity = m_ActiveScene->GetPhysics2DGravity();
	}

	void EditorLayer::SetEditorScenePath(const std::filesystem::path& path)
	{
		std::string name = path.filename().string();
		m_SceneName = path.filename().string();

		m_EditorScene->SetName(name);
		SyncViewportTitle();
	}

	void EditorLayer::SyncViewportTitle()
	{
		std::string title = "Nutcrackz Editor";

#ifdef NZ_DEBUG
		title += " - Debug";
#else
		title += " - Release";
#endif

		title += " | " + m_ActiveScene->GetName();

		if (Application::Get().GetWindow().GetTitle() != title)
			Application::Get().GetWindow().SetTitle(title);
	}

}
