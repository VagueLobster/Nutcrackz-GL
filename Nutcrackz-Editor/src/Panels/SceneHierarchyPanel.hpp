#pragma once

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Scene/Scene.hpp"
#include "Nutcrackz/Scene/Entity.hpp"
#include "Nutcrackz/Asset/Asset.hpp"

#include "Nutcrackz/Events/Event.hpp"
#include "Nutcrackz/Events/KeyEvent.hpp"
#include "Nutcrackz/Events/MouseEvent.hpp"

namespace Nutcrackz {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(RefPtr<Scene>& context);

		void Init();

		void OnImGuiRender();
		void OnEvent(Event& e);

		RefPtr<Scene>& GetContext() { return m_Context; }
		void SetContext(RefPtr<Scene>& context);

		Entity GetSelectedEntity() const { return m_SelectionContext; }
		void SetSelectedEntity(Entity entity);

	public:
		static bool ShowSceneHierarchy;
		static bool ShowProperties;
		static bool PressedEnter;
		static bool MouseDragging;

	private:
		template<typename T>
		void DisplayAddComponentEntry(const std::string& entryName);

		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);

		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);

		//void SetFloatValueFromDegreesToRadians(const char* label, float& value);

	private:
		RefPtr<Scene> m_Context;

		Entity m_SelectionContext;
		bool m_UseLinear = false;
		AssetHandle m_DefaultTexture;

		//rtmcpp::Vec4 m_Vec4Result = rtmcpp::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		//rtmcpp::Vec4 m_Vec4ResultTemp = rtmcpp::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		//rtmcpp::Vec3 m_Vec3Result = rtmcpp::Vec3(0.0f, 0.0f, 0.0f);
		//rtmcpp::Vec3 m_Vec3ResultTemp = rtmcpp::Vec3(0.0f, 0.0f, 0.0f);
		//rtmcpp::Vec2 m_Vec2Result = rtmcpp::Vec2(0.0f, 0.0f);
		//rtmcpp::Vec2 m_Vec2ResultTemp = rtmcpp::Vec2(0.0f, 0.0f);
	};

}