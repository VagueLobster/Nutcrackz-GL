#pragma once

#include "Nutcrackz.hpp"

#include "Panels/SceneHierarchyPanel.hpp"
#include "Panels/ContentBrowserPanel.hpp"
#include "Panels/LogPanel.hpp"
#include "Nutcrackz/Core/Timer.hpp"

#include "Nutcrackz/Renderer/EditorCamera.hpp"
#include "Nutcrackz/Scene/Entity.hpp"

#include "Nutcrackz/Input/InputCodes.hpp"

#include "Nutcrackz/Events/Event.hpp"
#include "Nutcrackz/Events/KeyEvent.hpp"
#include "Nutcrackz/Events/MouseEvent.hpp"

#include <set>

namespace Nutcrackz {

	//struct Resolution
	//{
	//	int Width;
	//	int Height;
	//	int RefreshRate;
	//
	//	bool operator<(const Resolution& other) const
	//	{
	//		return Width > other.Width || Height > other.Height || RefreshRate > other.RefreshRate;
	//	}
	//};

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;

	private:
		std::vector<std::function<void()>> m_PostSceneUpdateQueue;

		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void OnOverlayRender();

		void NewProject();
		bool OpenProject();
		void OpenProject(const std::filesystem::path& path);
		void SaveProject();
		void SaveProjectAs();
		void SerializeProject(RefPtr<Project> project, const std::filesystem::path& path);
		std::string FindAssetsFolder();
		std::string FindFilepath(const char* filter);

		void NewScene();
		void OpenScene(AssetHandle handle);
		void SaveScene();
		void SaveSceneAs();

		void SerializeScene(RefPtr<Scene> scene, const std::filesystem::path& path);

		void OnSceneTransition(AssetHandle handle);

		void OnScenePlay();
		void OnSceneSimulate();
		void OnSceneStop();

		void ReloadCSharp();

		void OnDuplicateEntity();
		void OnCreateEmptyEntity();

		void SetEditorScenePath(const std::filesystem::path& path);
		void SyncViewportTitle();

		// UI Panels
		void UI_CentralToolbar();
		void UI_AboutPopup();
		void UI_NewProjectPopup();
		void UI_EditProjectSettingsPopup();
		void UI_EditGravityPopup();

		void QueueSceneTransition(AssetHandle handle);

	public:
		inline static bool ShowEditorViewport = true, ShowSettingsViewport = true, Show2DStatisticsViewport = true;

		inline static bool ShowAboutPopupWindow = false;
		inline static bool ShowNewProjectPopupWindow = false;
		inline static bool ShowEditProjectPopupWindow = false;
		inline static bool ShowEditGravityPopupWindow = false;

		inline static bool AnimateTexturesInEdit = false;
		inline static bool ShowingAboutPopupWindow = false;

		// Only for themes!
		inline static bool UseGreenDarkTheme = false;
		inline static bool UseOrangeDarkTheme = true;
		inline static bool UseLightTheme = false;
		inline static bool UseGoldDarkTheme = false;
		inline static bool UseChocolateTheme = false;

		// Step frames
		inline static int FramesToStep = 1;

	private:
		// Temp
		RefPtr<Framebuffer> m_FramebufferEditor;

		RefPtr<Scene> m_ActiveScene;
		RefPtr<Scene> m_EditorScene;
		std::filesystem::path m_EditorScenePath;
		std::filesystem::path m_EditorProjectPath;

		std::filesystem::path m_SceneName;

		// For selection
		Entity m_HoveredEntity;
		Entity m_SelectedEntity;
		Entity m_MainSceneCameraEntity;

		RefPtr<EditorCamera> m_EditorCamera;
		
		bool m_EditorViewportHovered = false;
		bool m_EditorViewportFocused = false;
		bool m_ViewportToolbarHovered = false;
		rtmcpp::Vec2 m_EditorViewportSize = { 0.0f, 0.0f };
		rtmcpp::Vec2 m_EditorViewportBounds[2];

		int m_GizmoType = -1;

		bool m_ShowPhysicsColliders = false;

		enum class SceneState
		{
			Edit = 0, Play = 1, Simulate = 2
		};
		SceneState m_SceneState = SceneState::Edit;

		// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		RefPtr<ContentBrowserPanel> m_ContentBrowserPanel;
		LogPanel m_LogPanel;

		//Editor resources
		RefPtr<Texture2D> m_IconPlay, m_IconStep, m_IconPause, m_IconSimulate, m_IconStop,
			m_IconNoGizmo, m_IconTranslate, m_IconRotate, m_IconScale, m_Logo;

		// Visualization
		float m_PhysicsDebugLineThickness = 2.0f;

		// ImGui
		ImFont* m_BoldFont = nullptr;
		ImFont* m_LargeFont = nullptr;
		ImFont* m_LargerFont = nullptr;

		bool m_HoveringChernoUnplugged = false;
		bool m_HoveringStudioChernosGithub = false;
		bool m_HoveringToastmasternsGithub = false;
		bool m_HoveringToniPlaysGithub = false;

		rtmcpp::Vec2 m_Gravity = { 0.0f, -9.81f };

		// Retrieving desktop resolutions
		//std::set<Resolution> m_Resolutions;

		InputContext m_EditorLayerContext;
		InputContext m_ShiftContext;
		InputContext m_ControlContext;
		InputContext m_ControlShiftContext;

		// Projects
		std::string m_ProjectName = "";
		std::string m_AssetsFolder = "";
		std::string m_ScriptModulePath = "";
	};

}
