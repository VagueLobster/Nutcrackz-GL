#include "nzpch.hpp"
#include "SceneHierarchyPanel.hpp"

#include "Nutcrackz/Scene/Components.hpp"
#include "Nutcrackz/Scripting/ScriptFile.hpp"
#include "Nutcrackz/Core/UUID.hpp"
#include "Nutcrackz/Utils/PlatformUtils.hpp"
#include "Nutcrackz/Scene/SceneSerializer.hpp"
#include "Nutcrackz/Core/Input.hpp"
#include "Nutcrackz/Project/Project.hpp"
#include "Nutcrackz/Renderer/Renderer3D.hpp"

#include "Nutcrackz/Asset/AssetImporter.hpp"
#include "Nutcrackz/Asset/TextureImporter.hpp"

#include "Nutcrackz/UndoRedo/CommandHistory.hpp"
#include "Nutcrackz/UndoRedo/ChangeBoolCommand.hpp"
#include "Nutcrackz/UndoRedo/ChangeEnumCommand.hpp"
#include "Nutcrackz/UndoRedo/ChangeInt32Command.hpp"
#include "Nutcrackz/UndoRedo/ChangeUint32Command.hpp"
#include "Nutcrackz/UndoRedo/ChangeStringCommand.hpp"
#include "Nutcrackz/UndoRedo/ChangeAssetHandleCommand.hpp"
#include "Nutcrackz/UndoRedo/ChangeFloatCommand.hpp"
#include "Nutcrackz/UndoRedo/ChangeVec2Command.hpp"
#include "Nutcrackz/UndoRedo/ChangeVec3Command.hpp"
#include "Nutcrackz/UndoRedo/ChangeVec4Command.hpp"

#include "Nutcrackz/Imgui/ImGui.hpp"
#include "Nutcrackz/Imgui/ImGuiMisc.hpp"
#include "Nutcrackz/Imgui/NutcrackzImGui.hpp"
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <imgui/imgui_internal.h>

#include <filesystem>
#include <cstdint>
#include <iostream>

#include <memory>
#include <string>
#include <stdexcept>

// The Microsoft C++ compiler is non-compliant with the C++ standard and needs
// the following definition to disable a security warning on std::strncpy().
#ifdef _MSVC_LANG
#define _CRT_SECURE_NO_WARNINGS
#endif

namespace Nutcrackz {

	bool SceneHierarchyPanel::ShowSceneHierarchy = true;
	bool SceneHierarchyPanel::ShowProperties = false;
	bool SceneHierarchyPanel::PressedEnter = false;
	bool SceneHierarchyPanel::MouseDragging = false;

	SceneHierarchyPanel::SceneHierarchyPanel(RefPtr<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::Init()
	{
		m_DefaultTexture = AssetHandle(13960236995167982461); // AssetHandle points to: "Resources/Images/Checkerboard.png"
	}

	void SceneHierarchyPanel::SetContext(RefPtr<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
		ShowProperties = false;
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		NZ_PROFILE_FUNCTION("SceneHierarchyPanel::OnImGuiRender");

		ImGuiWindowClass imguiWindow;
		imguiWindow.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoCloseButton;
		imguiWindow.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoWindowMenuButton;
		
		if (ShowSceneHierarchy)
		{
			ImGui::SetNextWindowClass(&imguiWindow);
			ImGui::Begin("Scene Hierarchy", &ShowSceneHierarchy);

			// The ImGuiListClipper tip I got from Peter1745,
			// and the link he gave is: https://github.com/ocornut/imgui/issues/662
			//ImGuiListClipper clipper(m_Context->GetEntityWorld().count<IDComponent>() - 1, ImGui::GetTextLineHeightWithSpacing());
			//for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
			//{
				m_Context->GetECS().each([&](flecs::entity entityID, IDComponent& e)
				{
					Entity entity{ entityID };

					DrawEntityNode(entity);
				});
			//}
			//clipper.End();

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			{
				m_SelectionContext = {};
				ShowProperties = false;
			}

			bool entityDeleted = false;
			// Right-click on blank space
			if (ImGui::BeginPopupContextWindow(0, 1))
			{
				if (ImGui::MenuItem("Create Empty Entity"))
					m_Context->CreateEntity("Empty Entity");

				if (m_SelectionContext)
				{
					ImGui::Separator();
				
					if (ImGui::MenuItem("Delete Entity"))
						entityDeleted = true;
				}

				ImGui::EndPopup();
			}

			ImGui::End();

			if (entityDeleted)
			{
				m_Context->DestroyEntity(m_SelectionContext);
				m_SelectionContext = {};
			}
		}

		if (ShowProperties)
		{
			ImGui::SetNextWindowClass(&imguiWindow);
			ImGui::Begin("Properties", &ShowProperties);
			if (m_SelectionContext)
			{
				DrawComponents(m_SelectionContext);
			}

			ImGui::End();
		}
	}

	void SceneHierarchyPanel::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseButtonPressedEvent>(NZ_BIND_EVENT_FN(SceneHierarchyPanel::OnMouseButtonPressed));
		dispatcher.Dispatch<MouseButtonReleasedEvent>(NZ_BIND_EVENT_FN(SceneHierarchyPanel::OnMouseButtonReleased));
		dispatcher.Dispatch<MouseMovedEvent>(NZ_BIND_EVENT_FN(SceneHierarchyPanel::OnMouseMoved));
	}

	bool SceneHierarchyPanel::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		//if (e.GetMouseButton() == Mouse::ButtonLeft)
		//{
		//	MouseDragging = true;
		//	NZ_CORE_WARN("Left Mouse Button Pressed!");
		//}

		return false;
	}

	bool SceneHierarchyPanel::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
	{
		//if (e.GetMouseButton() == Mouse::ButtonLeft)
		//{
		//	MouseDragging = false;
		//	NZ_CORE_WARN("Left Mouse Button Released!");
		//}

		return false;
	}

	bool SceneHierarchyPanel::OnMouseMoved(MouseMovedEvent& e)
	{
		//if (MouseDragging)
		//const ImGuiIO& io = ImGui::GetIO();
		//e.Handled |= e.IsInCategory(EventCategoryApplication) & io.WantCaptureMouse;

		//if (!e.)
		//{
		//	NZ_CORE_WARN("Left Mouse Button Dragged!");
		//}

		return false;
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;

		if (m_SelectionContext)
			ShowProperties = true;
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
		if (ImGui::IsMouseReleased(0))
		{
			if (ImGui::IsItemHovered())
			{
				m_SelectionContext = entity;
				ShowProperties = true;
			}
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Create Empty Entity"))
			{
				m_SelectionContext = m_Context->CreateEntity("Empty Entity");
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Delete Entity"))
			{
				entityDeleted = true;
			}

			ImGui::EndPopup();
		}

		//if (opened)
		//{
		//	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		//	bool opened = ImGui::TreeNodeEx((void*)9817239, flags, tag.c_str());
		//	if (opened)
		//		ImGui::TreePop();
		//	ImGui::TreePop();
		//}

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
			{
				m_SelectionContext = {};
				ShowProperties = false;
			}
		}
	}

	static bool DrawVec3Control(const std::string& label, rtmcpp::Vec3& values, float resetValue = 0.0f, float columnWidth = 135.0f, float minValue = 0.0f, float maxValue = 0.0f)
	{
		bool modified = false;
		bool resetValues = false;
		bool changedValue = false;

		const ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		rtmcpp::Vec3 oldValues = values;

		ImVec2 xLabelSize = ImGui::CalcTextSize("X");
		float lineHeight = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
		{
			oldValues.X = resetValue;
			resetValues = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##X", &oldValues.X, 0.1f, minValue, maxValue, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
		{
			oldValues.Y = resetValue;
			resetValues = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Y", &oldValues.Y, 0.1f, minValue, maxValue, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
		{
			oldValues.Z = resetValue;
			resetValues = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Z", &oldValues.Z, 0.1f, minValue, maxValue, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();
		ImGui::Columns(1);

		ImGui::PopID();

		if (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging || resetValues)
		{
			values = oldValues;
			changedValue = true;
		}

		return (modified || resetValues) && changedValue;
	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImVec2 xLabelSize = ImGui::CalcTextSize("+");
			float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::SameLine(contentRegionAvailable.x - buttonSize * 0.9f);
			if (ImGui::Button("+", ImVec2{ buttonSize, buttonSize }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent)
				entity.RemoveComponent<T>();
		}
	}

	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName)
	{
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}

	std::string ConvertToString(char* a, int size)
	{
		int i;
		std::string s = "";
		for (i = 0; i < size; i++)
		{
			s = s + a[i];
		}
		return s;
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		NZ_PROFILE_FUNCTION("SceneHierarchyPanel::DrawComponents");

		const ImGuiIO& io = ImGui::GetIO();

		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strncpy_s(buffer, sizeof(buffer), tag.c_str(), _TRUNCATE);
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 4.0f, ImGui::GetCursorPosY() + 4.0f));
			ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionMax().x - 184.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });

			if (ImGui::InputText("##Tag", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				std::string result = std::string(buffer);
				CommandHistory::AddAndRemoveCommand(new ChangeStringCommand(entity.GetComponent<TagComponent>().Tag, result));
				tag = result;
			}

			ImGui::PopStyleVar();
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");
		ImGui::PopStyleVar();

		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 4.0f, ImGui::GetCursorPosY() + 0.0f));
		std::string idText = "Entity ID: " + std::to_string(entity.GetEntityHandle());
		ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.3f, 1.0f), idText.c_str());

		if (ImGui::BeginPopup("AddComponent"))
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			DisplayAddComponentEntry<CameraComponent>("Camera");
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			DisplayAddComponentEntry<ScriptComponent>("Script Component");
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			DisplayAddComponentEntry<TextComponent>("Text Component");

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			if (ImGui::BeginMenu("2D Primitives"))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<CircleRendererComponent>("Circle Renderer");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<TriangleRendererComponent>("Triangle Renderer");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<LineRendererComponent>("Line Renderer");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<VideoRendererComponent>("Video Renderer");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<ParticleSystemComponent>("Particle System");

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				if (ImGui::BeginMenu("Physics"))
				{
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					DisplayAddComponentEntry<Rigidbody2DComponent>("Rigidbody 2D");
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider 2D");
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D");
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					DisplayAddComponentEntry<TriangleCollider2DComponent>("Triangle Collider 2D");
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					DisplayAddComponentEntry<CapsuleCollider2DComponent>("Capsule Collider 2D");
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
					DisplayAddComponentEntry<MeshCollider2DComponent>("Mesh Collider 2D");

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			if (ImGui::BeginMenu("3D Primitives"))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<CubeRendererComponent>("Cube Renderer");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<PyramidRendererComponent>("Pyramid Renderer");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<TriangularPrismRendererComponent>("Triangular Prism Renderer");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<PlaneRendererComponent>("Plane Renderer");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<OBJRendererComponent>("Obj Renderer");

				ImGui::EndMenu();
			}

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			if (ImGui::BeginMenu("Audio"))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<AudioSourceComponent>("Audio Source");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<AudioListenerComponent>("Audio Listener");

				ImGui::EndMenu();
			}

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
			if (ImGui::BeginMenu("Widgets"))
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<ButtonWidgetComponent>("Button");
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				DisplayAddComponentEntry<CircleWidgetComponent>("Circle");

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		DrawComponent<TransformComponent>("Transform", entity, [=](TransformComponent& component) mutable
		{
			/*static bool translationModified = false;
			static bool rotationModified = false;
			static bool scalingModified = false;

			rtmcpp::Vec3 translationValues = rtmcpp::Vec3{ component.Translation.X, component.Translation.Y, component.Translation.Z };
			if (DrawVec3Control("Translation", translationValues))
			{
				if (!translationModified)
					m_Vec4ResultTemp = rtmcpp::Vec4{ component.Translation.X, component.Translation.Y, component.Translation.Z, 1.0f };
				
				m_Vec4Result = rtmcpp::Vec4{ translationValues, 1.0f };
				component.Translation = m_Vec4Result;
				translationModified = true;
			}

			if (translationModified && !io.MouseDown[0])
			{
				component.Translation = m_Vec4ResultTemp;
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Translation, m_Vec4Result));
				component.Translation = m_Vec4Result;
				m_Vec4Result = rtmcpp::Vec4{ 0.0f, 0.0f, 0.0f, 1.0f };

				translationModified = false;
			}
			
			rtmcpp::Vec3 rotation = rtmcpp::Vec3{ rtmcpp::Degrees(component.Rotation.X), rtmcpp::Degrees(component.Rotation.Y), rtmcpp::Degrees(component.Rotation.Z) };
			if (DrawVec3Control("Rotation", rotation))
			{
				if (!rotationModified)
					m_Vec3ResultTemp = rtmcpp::Vec3{ rtmcpp::Radians(component.Rotation.X), rtmcpp::Radians(component.Rotation.Y), rtmcpp::Radians(component.Rotation.Z) };;

				m_Vec3Result = rtmcpp::Vec3{ rtmcpp::Radians(rotation.X), rtmcpp::Radians(rotation.Y), rtmcpp::Radians(rotation.Z) };
				component.Rotation = m_Vec3Result;
				rotationModified = true;
			}

			if (rotationModified && !io.MouseDown[0])
			{
				component.Rotation = m_Vec3ResultTemp;
				CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(component.Rotation, m_Vec3Result));
				component.Rotation = m_Vec3Result;
				m_Vec3Result = rtmcpp::Vec3{ 0.0f, 0.0f, 0.0f };

				rotationModified = false;
			}

			rtmcpp::Vec3 scale = component.Scale;
			if (DrawVec3Control("Scale", scale, 1.0f, 135.0f, 0.01f, FLT_MAX))
			{
				if (!scalingModified)
					m_Vec3ResultTemp = component.Scale;

				m_Vec3Result = scale;
				component.Scale = m_Vec3Result;
				scalingModified = true;
			}

			if (scalingModified && !io.MouseDown[0])
			{
				component.Scale = m_Vec3ResultTemp;
				CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(component.Scale, m_Vec3Result));
				component.Scale = m_Vec3Result;
				m_Vec3Result = rtmcpp::Vec3{ 0.0f, 0.0f, 0.0f };

				scalingModified = false;
			}*/

			rtmcpp::Vec3 translationValues = rtmcpp::Vec3{ component.Translation.X, component.Translation.Y, component.Translation.Z };
			if (DrawVec3Control("Translation", translationValues))
			{
				rtmcpp::Vec4 result = rtmcpp::Vec4{ translationValues, 1.0f };
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Translation, result));
				component.Translation = result;
			}

			rtmcpp::Vec3 rotation = rtmcpp::Vec3{ rtmcpp::Degrees(component.Rotation.X), rtmcpp::Degrees(component.Rotation.Y), rtmcpp::Degrees(component.Rotation.Z) };
			if (DrawVec3Control("Rotation", rotation))
			{
				rtmcpp::Vec3 result = rtmcpp::Vec3{ rtmcpp::Radians(rotation.X), rtmcpp::Radians(rotation.Y), rtmcpp::Radians(rotation.Z) };
				CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(component.Rotation, result));
				component.Rotation = result;
			}

			rtmcpp::Vec3 scale = component.Scale;
			if (DrawVec3Control("Scale", scale, 1.0f, 135.0f, 0.01f, FLT_MAX))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(component.Scale, scale));
				component.Scale = scale;
			}

			UI::BeginPropertyGrid();

			bool enabled = component.Enabled;

			ImGui::SetColumnWidth(0, 135.0f);
			if (UI::Property("Render", enabled))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.Enabled, enabled));
				component.Enabled = enabled;
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<ScriptComponent>("Script", entity, [=](ScriptComponent& component) mutable
		{
			UI::BeginPropertyGrid();
		
			ImGui::Text("Script");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
		
			auto& scriptEngine = ScriptEngine::GetMutable();
			bool isError = !scriptEngine.IsValidScript(component.ScriptHandle);
		
			std::string label = "None";
			bool isScriptValid = false;
			if (component.ScriptHandle != 0)
			{		
				if (AssetManager::IsAssetHandleValid(component.ScriptHandle) && AssetManager::GetAssetType(component.ScriptHandle) == AssetType::ScriptFile)
				{
					const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.ScriptHandle);
					label = metadata.FilePath.filename().string();
					isScriptValid = true;
				}
				else
				{
					label = "Invalid";
				}
			}
		
			ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
			buttonLabelSize.x += 20.0f;
			float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x);
		
			ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;
		
					if (AssetManager::GetAssetType(handle) == AssetType::ScriptFile)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.ScriptHandle, handle));
						component.ScriptHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}
		
			if (isScriptValid)
			{
				ImGui::SameLine();
				ImVec2 xLabelSize = ImGui::CalcTextSize("X");
				float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					m_Context->GetScriptStorage().ShutdownEntityStorage(component.ScriptHandle, entity.GetEntityHandle());

					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.ScriptHandle, result));
					component.ScriptHandle = result;
					component.HasInitializedScript = false;
				}
			}
		
			ImGui::PopItemWidth();
			ImGui::NextColumn();
			ImGui::Spacing();

			if (component.ScriptHandle != 0)
			{
				isError = !scriptEngine.IsValidScript(component.ScriptHandle);

				if (!isError && !component.HasInitializedScript)
				{
					m_Context->GetScriptStorage().InitializeEntityStorage(component.ScriptHandle, entity.GetEntityHandle());
					component.HasInitializedScript = true;
				}
				else if (isError && component.HasInitializedScript)
				{
					auto oldScriptHandle = component.ScriptHandle;
					bool wasCleared = component.ScriptHandle == 0;

					if (wasCleared)
						component.ScriptHandle = oldScriptHandle;

					m_Context->GetScriptStorage().ShutdownEntityStorage(component.ScriptHandle, entity.GetEntityHandle());

					if (wasCleared)
						component.ScriptHandle = 0;

					component.HasInitializedScript = false;
				}
			}

			// NOTE(Peter): Editing fields doesn't really work if there's inconsistencies with the script classes...
			if (component.ScriptHandle != 0 && component.HasInitializedScript)
			{		
				auto& entityStorage = m_Context->GetScriptStorage().EntityStorage.at(entity.GetEntityHandle());
		
				for (auto& [fieldID, fieldStorage] : entityStorage.Fields)
				{
					// TODO(Peter): Update field input to display "---" when there's mixed values
					//if (field->IsArray())
					//{
					//	if (UI::DrawFieldArray(m_Context, fieldName, storage.As<ArrayFieldStorage>()))
					//	{
					//		for (auto entityID : entities)
					//		{
					//			/*Entity entity = m_Context->GetEntityWithID(entityID);
					//			const auto& sc = entity.GetComponent<ScriptComponent>();
					//			storage->CopyData(firstComponent.ManagedInstance, sc.ManagedInstance);*/
					//		}
					//	}
					//}
					//else
					//{
					if (UI::DrawFieldValue(fieldStorage.GetName(), fieldStorage))
					{
						/*for (auto entityID : entities)
						{
							Entity entity = m_Context->GetEntityWithID(entityID);
							const auto& sc = entity.GetComponent<ScriptComponent>();
							storage->CopyData(firstComponent.ManagedInstance, sc.ManagedInstance);
						}*/
					}
					//}
				}		
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<CameraComponent>("Camera", entity, [&](CameraComponent& component)
		{
			UI::BeginPropertyGrid();

			if (!component.Camera)
				component.Camera = RefPtr<SceneCamera>::Create();

			bool primary = component.Primary;
			if (UI::Property("Primary", primary))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.Primary, primary));
				component.Primary = primary;
			}

			if (component.Camera)
			{
				const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
				int currentProj = static_cast<int>(component.Camera->m_ProjectionType);
				if (UI::PropertyDropdown("Projection", projectionTypeStrings, 2, &currentProj))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeEnumCommand(component.Camera->m_ProjectionType, (SceneCamera::ProjectionType)currentProj));
					component.Camera->m_ProjectionType = (SceneCamera::ProjectionType)currentProj;
					component.Camera->RecalculateProjection();
				}

				if (component.Camera->m_ProjectionType == SceneCamera::ProjectionType::Perspective)
				{
					// Perspective parameters
					float verticalFOV = rtmcpp::Degrees(component.Camera->m_PerspectiveFOV);
					if (UI::Property("Vertical FOV", verticalFOV) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						float result = rtmcpp::Radians(verticalFOV);
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Camera->m_PerspectiveFOV, result));
						component.Camera->m_PerspectiveFOV = result;
					}
					//SetFloatValueFromDegreesToRadians("Vertical FOV", component.Camera->m_PerspectiveFOV);
					/*static bool fovModified = false;
					static float floatResultTemp = 0.0f;
					static float floatResult = 0.0f;
					if (UI::Property("Vertical FOV", verticalFOV))
					{
						if (!fovModified)
							floatResultTemp = rtmcpp::Radians(rtmcpp::Degrees(component.Camera->m_PerspectiveFOV));

						floatResult = rtmcpp::Radians(verticalFOV);
						component.Camera->m_PerspectiveFOV = floatResult;
						fovModified = true;
					}

					if (fovModified && !io.MouseDown[0])
					{
						component.Camera->m_PerspectiveFOV = floatResultTemp;
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Camera->m_PerspectiveFOV, floatResult));
						component.Camera->m_PerspectiveFOV = floatResult;
						floatResult = 0.0f;

						fovModified = false;
					}*/

					float nearClip = component.Camera->m_PerspectiveNear;
					if (UI::Property("Near Clip", nearClip) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Camera->m_PerspectiveNear, nearClip));
						component.Camera->m_PerspectiveNear = nearClip;
					}

					float farClip = component.Camera->m_PerspectiveFar;
					if (UI::Property("Far Clip", farClip) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Camera->m_PerspectiveFar, farClip));
						component.Camera->m_PerspectiveFar = farClip;
					}
				}
				else if (component.Camera->m_ProjectionType == SceneCamera::ProjectionType::Orthographic)
				{
					// Orthographic parameters
					float orthoSize = component.Camera->m_OrthographicSize;
					if (UI::Property("Size", orthoSize) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Camera->m_OrthographicSize, orthoSize));
						component.Camera->m_OrthographicSize = orthoSize;
					}

					float nearClip = component.Camera->m_OrthographicNear;
					if (UI::Property("Near Clip", nearClip) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Camera->m_OrthographicNear, nearClip));
						component.Camera->m_OrthographicNear = nearClip;
					}

					float farClip = component.Camera->m_OrthographicFar;
					if (UI::Property("Far Clip", farClip) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Camera->m_OrthographicFar, farClip));
						component.Camera->m_OrthographicFar = farClip;
					}

					bool fixedAspectRatio = component.FixedAspectRatio;
					if (UI::Property("Fixed Aspect Ratio", fixedAspectRatio) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.FixedAspectRatio, fixedAspectRatio));
						component.FixedAspectRatio = fixedAspectRatio;
					}
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [this](SpriteRendererComponent& component)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec3 transform = rtmcpp::Vec3{ component.SpriteTranslation.X, component.SpriteTranslation.Y, component.SpriteTranslation.Z };
			if (UI::Property("Position Offset", transform) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				rtmcpp::Vec4 result = rtmcpp::Vec4{ transform, 1.0f };
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.SpriteTranslation, result));
				component.SpriteTranslation = result;
			}

			float saturation = component.Saturation;
			if (UI::Property("Saturation", saturation, 0.01f, 0.0f, 1.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Saturation, saturation));
				component.Saturation = saturation;
			}

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			RefPtr<Texture2D> defaultTexture = AssetManager::GetAsset<Texture2D>(m_DefaultTexture);
			uint32_t buttonTex = defaultTexture->GetRendererID();

			if (component.TextureHandle)
			{
				RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(component.TextureHandle);
				if (texture)
					buttonTex = texture->GetRendererID();
			}

			UI::PropertyButtonImage("Texture", buttonTex, ImVec2{ 100.0f, 100.0f });

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, handle));
						component.TextureHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (component.TextureHandle)
			{
				ImGui::SameLine();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
				if (UI::ButtonProperty("##RemoveTexture", "Remove"))
				{
					buttonTex = defaultTexture->GetRendererID();
					component.Texture = defaultTexture;

					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, result));
					component.TextureHandle = result;
				}
				ImGui::PopStyleVar();

				rtmcpp::Vec2 uv = component.UV;
				if (UI::PropertyDrag("UV Coords", uv, 0.1f, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.UV, uv));
					component.UV = uv;
				}

				bool linear = component.m_AnimationData.UseLinear;
				if (UI::Property("Linear Filtering", linear))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseLinear, linear));
					component.m_AnimationData.UseLinear = linear;
				}

				float animSpeed = component.m_AnimationData.AnimationSpeed;
				if (UI::PropertyDrag("Speed", animSpeed, 0.01f, 0.01f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.m_AnimationData.AnimationSpeed, animSpeed));
					component.m_AnimationData.AnimationSpeed = animSpeed;
				}

				bool useTextureAtlasAnimation = component.m_AnimationData.UseTextureAtlasAnimation;
				if (UI::Property("Use Texture Atlas Animation", useTextureAtlasAnimation))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseTextureAtlasAnimation, useTextureAtlasAnimation));
					component.m_AnimationData.UseTextureAtlasAnimation = useTextureAtlasAnimation;
					component.m_AnimationData.UsePerTextureAnimation = false;
					component.m_AnimationData.UseParallaxScrolling = false;

					if (useTextureAtlasAnimation)
					{
						if (component.m_AnimationData.Textures.size() > 0)
						{
							for (uint32_t i = 0; i < component.m_AnimationData.Textures.size(); i++)
							{
								component.RemoveAnimationTexture(component.m_AnimationData.Textures[i]);
							}
						}

						component.m_AnimationData.ParallaxSpeed.X = 0.0f;
						component.m_AnimationData.ParallaxSpeed.Y = 0.0f;
						component.m_AnimationData.ParallaxDivision = 0.0f;
						component.m_AnimationData.UseCameraParallaxX = false;
						component.m_AnimationData.UseCameraParallaxY = false;
					}
				}

				bool usePerTextureAnimation = component.m_AnimationData.UsePerTextureAnimation;
				if (UI::Property("Use Per Texture Animation", usePerTextureAnimation))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UsePerTextureAnimation, usePerTextureAnimation));
					component.m_AnimationData.UsePerTextureAnimation = usePerTextureAnimation;
					component.m_AnimationData.UseTextureAtlasAnimation = false;
					component.m_AnimationData.UseParallaxScrolling = false;

					if (usePerTextureAnimation)
					{
						component.m_AnimationData.NumberOfTiles = 1;
						component.m_AnimationData.StartIndexX = 0;
						component.m_AnimationData.StartIndexY = 0;
						component.m_AnimationData.Rows = 1;
						component.m_AnimationData.Columns = 1;

						component.m_AnimationData.ParallaxSpeed.X = 0.0f;
						component.m_AnimationData.ParallaxSpeed.Y = 0.0f;
						component.m_AnimationData.ParallaxDivision = 0.0f;
						component.m_AnimationData.UseCameraParallaxX = false;
						component.m_AnimationData.UseCameraParallaxY = false;
					}
				}

				bool useParallaxAnimation = component.m_AnimationData.UseParallaxScrolling;
				if (UI::Property("Use Parallax Animation", useParallaxAnimation))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseParallaxScrolling, useParallaxAnimation));
					component.m_AnimationData.UseParallaxScrolling = useParallaxAnimation;
					component.m_AnimationData.UseTextureAtlasAnimation = false;
					component.m_AnimationData.UsePerTextureAnimation = false;

					if (useParallaxAnimation)
					{
						component.m_AnimationData.NumberOfTiles = 1;
						component.m_AnimationData.StartIndexX = 0;
						component.m_AnimationData.StartIndexY = 0;
						component.m_AnimationData.Rows = 1;
						component.m_AnimationData.Columns = 1;

						if (component.m_AnimationData.Textures.size() > 0)
						{
							for (uint32_t i = 0; i < component.m_AnimationData.Textures.size(); i++)
							{
								component.RemoveAnimationTexture(component.m_AnimationData.Textures[i]);
							}
						}
					}
				}

				if (component.m_AnimationData.UseTextureAtlasAnimation)
				{
					int numTiles = component.m_AnimationData.NumberOfTiles;
					if (UI::Property("Num of Tiles", numTiles, 1, 1, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.NumberOfTiles, numTiles));
						component.m_AnimationData.NumberOfTiles = numTiles;
					}

					int startIndexX = component.m_AnimationData.StartIndexX;

					if ((component.m_AnimationData.Rows - 1) < 1)
						startIndexX = 0;

					if (UI::PropertyU32("Start Index X", startIndexX, 1, 0, (component.m_AnimationData.Rows - 1)) &&
						(SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.StartIndexX, startIndexX));
						component.m_AnimationData.StartIndexX = startIndexX;
					}

					int startIndexY = component.m_AnimationData.StartIndexY;

					if ((component.m_AnimationData.Columns - 1) < 1)
						startIndexY = 0;

					if (UI::PropertyU32("Start Index Y", startIndexY, 1, 0, (component.m_AnimationData.Columns - 1)) &&
						(SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.StartIndexY, startIndexY));
						component.m_AnimationData.StartIndexY = startIndexY;
					}

					int rows = component.m_AnimationData.Rows;
					if (UI::Property("Rows", rows, 1, 1, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.Rows, rows));
						component.m_AnimationData.Rows = rows;
					}

					int columns = component.m_AnimationData.Columns;
					if (UI::Property("Columns", columns, 1, 1, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.Columns, columns));
						component.m_AnimationData.Columns = columns;
					}
				}

				if (component.m_AnimationData.UsePerTextureAnimation)
				{
					if (component.m_AnimationData.Textures.size() == 0)
					{
						component.AddAnimationTexture(component.TextureHandle);
					}

					int startIndex = component.m_AnimationData.StartIndex;
					uint32_t minIndex = 0;
					if (UI::Property("Start Index", startIndex, 1, minIndex, (int)component.m_AnimationData.Textures.size() - 1) &&
						(SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.StartIndex, startIndex));
						component.m_AnimationData.StartIndex = startIndex;
					}

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
					if (UI::ButtonProperty("", "Add Texture"))
					{
						if (component.Texture)
						{
							component.AddAnimationTexture(m_DefaultTexture);
						}
					}
					ImGui::PopStyleVar();

					for (uint32_t i = 0; i < component.m_AnimationData.Textures.size(); i++)
					{
						uint32_t buttonTex0 = defaultTexture->GetRendererID();
						RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(component.m_AnimationData.Textures[i]);

						if (texture)
							buttonTex0 = texture->GetRendererID();

						std::string imageName = "Texture " + std::to_string(i);

						ImGui::PushID(i);

						UI::PropertyButtonImage(imageName.c_str(), buttonTex0, ImVec2{ 100.0f, 100.0f });

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
							{
								AssetHandle handle = *(AssetHandle*)payload->Data;

								if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
								{
									CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.m_AnimationData.Textures[i], handle));
									component.m_AnimationData.Textures[i] = handle;
								}
								else
								{
									NZ_CORE_WARN("Wrong asset type!");
								}
							}
							ImGui::EndDragDropTarget();
						}

						ImGui::SameLine();

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
						if (UI::ButtonProperty("", "Remove"))
						{
							AssetHandle result = 0;
							CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.m_AnimationData.Textures[i], result));

							component.RemoveAnimationTexture(component.m_AnimationData.Textures[i]);

							if (i == 0)
							{
								if (component.m_AnimationData.Textures.size() > 0)
								{
									if (component.TextureHandle != component.m_AnimationData.Textures[0] && component.m_AnimationData.Textures[0])
									{
										component.TextureHandle = component.m_AnimationData.Textures[0];
									}
								}
								else
								{
									component.m_AnimationData.UsePerTextureAnimation = false;
								}
							}
						}
						ImGui::PopStyleVar();

						ImGui::PopID();
					}

					if (component.m_AnimationData.Textures.size() > 0)
					{
						if (component.TextureHandle != component.m_AnimationData.Textures[(uint32_t)component.m_AnimationData.StartIndex])
							component.TextureHandle = component.m_AnimationData.Textures[(uint32_t)component.m_AnimationData.StartIndex];
					}
				}

				if (component.m_AnimationData.UseParallaxScrolling)
				{
					rtmcpp::Vec2 parallaxSpeed = component.m_AnimationData.ParallaxSpeed;
					if (UI::PropertyDrag("Speed", parallaxSpeed, 0.1f, -FLT_MAX, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.m_AnimationData.ParallaxSpeed, parallaxSpeed));
						component.m_AnimationData.ParallaxSpeed = parallaxSpeed;
					}

					float division = component.m_AnimationData.ParallaxDivision;
					if (UI::Property("Division", division, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.m_AnimationData.ParallaxDivision, division));
						component.m_AnimationData.ParallaxDivision = division;
					}

					bool cameraParallaxX = component.m_AnimationData.UseCameraParallaxX;
					if (UI::Property("Camera Parallax X", cameraParallaxX))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseCameraParallaxX, cameraParallaxX));
						component.m_AnimationData.UseCameraParallaxX = cameraParallaxX;
					}

					bool cameraParallaxY = component.m_AnimationData.UseCameraParallaxY;
					if (UI::Property("Camera Parallax Y", cameraParallaxY))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseCameraParallaxY, cameraParallaxY));
						component.m_AnimationData.UseCameraParallaxY = cameraParallaxY;
					}
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<CircleRendererComponent>("Circle Renderer", entity, [=](auto& component) 
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec3 transform = rtmcpp::Vec3{ component.CircleTranslation.X, component.CircleTranslation.Y, component.CircleTranslation.Z };
			if (UI::Property("Position Offset", transform) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				rtmcpp::Vec4 result = rtmcpp::Vec4{ transform, 1.0f };
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.CircleTranslation, result));
				component.CircleTranslation = rtmcpp::Vec4{ transform, 1.0f };
			}

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			bool makeSolid = component.MakeSolid;
			if (UI::Property("Make Solid", makeSolid))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.MakeSolid, makeSolid));
				component.MakeSolid = makeSolid;
			}

			ImGui::BeginDisabled(component.MakeSolid);

			float thickness = component.Thickness;
			if (UI::PropertyDrag("Thickness", thickness, 0.025f, 0.0f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Thickness, thickness));
				component.Thickness = thickness;
				component.OldThickness = component.Thickness;
			}

			ImGui::EndDisabled();

			if (component.MakeSolid && component.Thickness != FLT_MAX)
			{
				component.OldThickness = component.Thickness;
				component.Thickness = FLT_MAX;
			}
			else if (!component.MakeSolid && component.Thickness == FLT_MAX)
			{
				component.Thickness = component.OldThickness;
			}

			float fade = component.Fade;
			if (UI::PropertyDrag("Fade", fade, 0.00025f, 0.0f, 1.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Fade, fade));
				component.Fade = fade;
			}

			ImGui::Text("Texture");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string label = "None";
			bool isTextureValid = false;
			if (component.TextureHandle != 0)
			{
				if (AssetManager::IsAssetHandleValid(component.TextureHandle) && AssetManager::GetAssetType(component.TextureHandle) == AssetType::Texture2D)
				{
					const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.TextureHandle);
					label = metadata.FilePath.filename().string();
					isTextureValid = true;
				}
				else
				{
					label = "Invalid";
				}
			}

			ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
			buttonLabelSize.x += 20.0f;
			float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 6 });
			ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, handle));
						component.TextureHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (isTextureValid)
			{
				ImGui::SameLine();
				ImVec2 xLabelSize = ImGui::CalcTextSize("X");
				float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, result));
					component.TextureHandle = 0;
				}
			}

			ImGui::PopStyleVar();
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if (component.TextureHandle)
			{
				rtmcpp::Vec2 uv = component.UV;
				if (UI::PropertyDrag("UV Coords", uv, 0.1f, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.UV, uv));
					component.UV = uv;
				}

				bool linear = component.m_AnimationData.UseLinear;
				if (UI::Property("Linear Filtering", linear))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseLinear, linear));
					component.m_AnimationData.UseLinear = linear;
				}

				bool textureAtlasAnimation = component.m_AnimationData.UseTextureAtlasAnimation;
				if (UI::Property("Texture Atlas Animation", textureAtlasAnimation))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseTextureAtlasAnimation, textureAtlasAnimation));
					component.m_AnimationData.UseTextureAtlasAnimation = textureAtlasAnimation;
				}

				if (component.m_AnimationData.UseTextureAtlasAnimation)
				{
					float animSpeed = component.m_AnimationData.AnimationSpeed;
					if (UI::PropertyDrag("Speed", animSpeed, 0.01f, 0.01f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.m_AnimationData.AnimationSpeed, animSpeed));
						component.m_AnimationData.AnimationSpeed = animSpeed;
					}

					int numTiles = component.m_AnimationData.NumberOfTiles;
					if (UI::Property("Num of Tiles", numTiles, 1, 1, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.NumberOfTiles, numTiles));
						component.m_AnimationData.NumberOfTiles = numTiles;
					}

					int startIndexX = component.m_AnimationData.StartIndexX;
					if (UI::Property("Start Index X", startIndexX, 1, 0, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.StartIndexX, startIndexX));
						component.m_AnimationData.StartIndexX = startIndexX;
					}

					int startIndexY = component.m_AnimationData.StartIndexY;
					if (UI::Property("Start Index Y", startIndexY, 1, 0, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.StartIndexY, startIndexY));
						component.m_AnimationData.StartIndexY = startIndexY;
					}

					int rows = component.m_AnimationData.Rows;
					if (UI::PropertyDrag("Rows", rows, 1, 1, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.Rows, rows));
						component.m_AnimationData.Rows = rows;
					}

					int columns = component.m_AnimationData.Columns;
					if (UI::PropertyDrag("Columns", columns, 1, 1, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.Columns, columns));
						component.m_AnimationData.Columns = columns;
					}
				}

				bool parallaxScrolling = component.m_AnimationData.UseParallaxScrolling;
				if (UI::Property("Parallax Texture", parallaxScrolling))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseParallaxScrolling, parallaxScrolling));
					component.m_AnimationData.UseParallaxScrolling = parallaxScrolling;
				}

				if (component.m_AnimationData.UseParallaxScrolling)
				{
					rtmcpp::Vec2 parallaxSpeed = component.m_AnimationData.ParallaxSpeed;
					if (UI::PropertyDrag("Speed", parallaxSpeed, 0.1f, -FLT_MAX, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.m_AnimationData.ParallaxSpeed, parallaxSpeed));
						component.m_AnimationData.ParallaxSpeed = parallaxSpeed;
					}

					float division = component.m_AnimationData.ParallaxDivision;
					if (UI::Property("Division", division, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.m_AnimationData.ParallaxDivision, division));
						component.m_AnimationData.ParallaxDivision = division;
					}

					bool cameraParallaxX = component.m_AnimationData.UseCameraParallaxX;
					if (UI::Property("Camera Parallax X", cameraParallaxX))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseCameraParallaxX, cameraParallaxX));
						component.m_AnimationData.UseCameraParallaxX = cameraParallaxX;
					}

					bool cameraParallaxY = component.m_AnimationData.UseCameraParallaxY;
					if (UI::Property("Camera Parallax Y", cameraParallaxY))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseCameraParallaxY, cameraParallaxY));
						component.m_AnimationData.UseCameraParallaxY = cameraParallaxY;
					}
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<TriangleRendererComponent>("Triangle Renderer", entity, [this](TriangleRendererComponent& component)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec3 transform = rtmcpp::Vec3{ component.TriangleTranslation.X, component.TriangleTranslation.Y, component.TriangleTranslation.Z };
			if (UI::Property("Position Offset", transform) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				rtmcpp::Vec4 result = rtmcpp::Vec4{ transform, 1.0f };
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.TriangleTranslation, result));
				component.TriangleTranslation = result;
			}

			float saturation = component.Saturation;
			if (UI::Property("Color Saturation", saturation, 0.01f, 0.0f, 1.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Saturation, saturation));
				component.Saturation = saturation;
			}

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			ImGui::Text("Texture");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string label = "None";
			bool isTextureValid = false;
			if (component.TextureHandle != 0)
			{
				if (AssetManager::IsAssetHandleValid(component.TextureHandle) && AssetManager::GetAssetType(component.TextureHandle) == AssetType::Texture2D)
				{
					const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.TextureHandle);
					label = metadata.FilePath.filename().string();
					isTextureValid = true;
				}
				else
				{
					label = "Invalid";
				}
			}

			ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
			buttonLabelSize.x += 20.0f;
			float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 6 });
			ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, handle));
						component.TextureHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (isTextureValid)
			{
				ImGui::SameLine();
				ImVec2 xLabelSize = ImGui::CalcTextSize("X");
				float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, result));
					component.TextureHandle = result;
				}
			}

			ImGui::PopStyleVar();
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if (component.TextureHandle)
			{
				rtmcpp::Vec2 uv = component.UV;
				if (UI::PropertyDrag("UV Coords", uv, 0.1f, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.UV, uv));
					component.UV = uv;
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<LineRendererComponent>("Line Renderer", entity, [=](LineRendererComponent& component)
		{
			UI::BeginPropertyGrid();

			float lineThickness = component.LineThickness;
			if (UI::Property("Line Thickness", lineThickness, 0.1f, 0.1f, 10.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.LineThickness, lineThickness));
				component.LineThickness = lineThickness;
			}

			if (UI::ButtonProperty("", "Add"))
			{
				component.AddColor(rtmcpp::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
				component.AddTranslation(rtmcpp::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
			}

			for (uint32_t i = 0; i < component.Translations.size(); i++)
			{
				ImGui::PushID(i);

				if (i >= 1)
				{
					std::string colorName = "Color " + std::to_string(i);

					rtmcpp::Vec4 color = component.Colors[i];
					if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Colors[i], color));
						component.Colors[i] = color;
					}
				}

				std::string translationName = "Translation " + std::to_string(i);
				rtmcpp::Vec3 position = rtmcpp::Vec3{ component.Translations[i].X, component.Translations[i].Y, component.Translations[i].Z };
				if (UI::Property(translationName.c_str(), position) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					rtmcpp::Vec4 result = rtmcpp::Vec4{ position, 1.0f };
					CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Translations[i], result));
					component.Translations[i] = result;
				}

				if (UI::ButtonProperty("", "Remove"))
				{
					if (i >= 1)
						component.RemoveColor(i);

					component.RemoveTranslation(i);
				}

				ImGui::PopID();
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<ParticleSystemComponent>("Particle System", entity, [](ParticleSystemComponent& component)
			{
				UI::BeginPropertyGrid();

				bool useBillboard = component.UseBillboard;
				if (UI::Property("Use Billboard", useBillboard))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.UseBillboard, useBillboard));
					component.UseBillboard = useBillboard;
				}

				rtmcpp::Vec3 velocity = component.Velocity;
				if (UI::Property("Velocity", velocity) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(component.Velocity, velocity));
					component.Velocity = velocity;
				}

				rtmcpp::Vec3 velocityVariation = component.VelocityVariation;
				if (UI::Property("Velocity Variation", velocityVariation) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(component.VelocityVariation, velocityVariation));
					component.VelocityVariation = velocityVariation;
				}

				rtmcpp::Vec4 colorBegin = component.ColorBegin;
				if (UI::PropertyColor("Color Begin", colorBegin) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.ColorBegin, colorBegin));
					component.ColorBegin = colorBegin;
				}

				rtmcpp::Vec4 colorEnd = component.ColorEnd;
				if (UI::PropertyColor("Color End", colorEnd) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.ColorEnd, colorEnd));
					component.ColorEnd = colorEnd;
				}

			ImGui::Text("Texture");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string label = "None";
			bool isTextureValid = false;
			if (component.TextureHandle != 0)
			{
				if (AssetManager::IsAssetHandleValid(component.TextureHandle) && AssetManager::GetAssetType(component.TextureHandle) == AssetType::Texture2D)
				{
					const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.TextureHandle);
					label = metadata.FilePath.filename().string();
					isTextureValid = true;
				}
				else
				{
					label = "Invalid";
				}
			}

			ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
			buttonLabelSize.x += 20.0f;
			float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 6 });
			ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, handle));
						component.TextureHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (isTextureValid)
			{
				ImGui::SameLine();
				ImVec2 xLabelSize = ImGui::CalcTextSize("X");
				float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, result));
					component.TextureHandle = result;
				}
			}

			ImGui::PopStyleVar();
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			bool linear = component.UseLinear;
			if (UI::Property("Linear Filtering", linear))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.UseLinear, linear));
				component.UseLinear = linear;
			}

			float sizeBegin = component.SizeBegin;
			if (UI::Property("Size Begin", sizeBegin, 0.01f, -FLT_MAX, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.SizeBegin, sizeBegin));
				component.SizeBegin = sizeBegin;
			}

			float sizeEnd = component.SizeEnd;
			if (UI::Property("Size End", sizeEnd, 0.01f, -FLT_MAX, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.SizeEnd, sizeEnd));
				component.SizeEnd = sizeEnd;
			}

			float sizeVariation = component.SizeVariation;
			if (UI::Property("Size Variation", sizeVariation, 0.01f, -FLT_MAX, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.SizeVariation, sizeVariation));
				component.SizeVariation = sizeVariation;
			}

			float lifeTime = component.LifeTime;
			if (UI::Property("Life Time", lifeTime, 0.01f, -FLT_MAX, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.LifeTime, lifeTime));
				component.LifeTime = lifeTime;
			}

			int particleSize = component.ParticleSize;
			if (UI::Property("Particle Size", particleSize, 1, 1, 1000) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.ParticleSize, particleSize));
				component.ParticleSize = particleSize;
			}

			int maxParticles = component.m_ParticleSystem.s_MaxParticles;
			if (UI::Property("Max Particles", maxParticles, 1, 1, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeUint32Command(component.m_ParticleSystem.s_MaxParticles, maxParticles));
				component.m_ParticleSystem.s_MaxParticles = maxParticles;
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<TextComponent>("Text Component", entity, [&](auto& component)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			ImGui::Text("Font");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string label = "None";
			bool isFontValid = false;
			if (component.FontHandle != 0)
			{
				if (AssetManager::IsAssetHandleValid(component.FontHandle) && AssetManager::GetAssetType(component.FontHandle) == AssetType::Font)
				{
					const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.FontHandle);
					label = metadata.FilePath.filename().string();
					isFontValid = true;
				}
				else
				{
					label = "Invalid";
				}
			}

			ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
			buttonLabelSize.x += 20.0f;
			float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 6 });
			ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Font)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.FontHandle, handle));
						component.FontHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (isFontValid)
			{
				ImGui::SameLine();
				ImVec2 xLabelSize = ImGui::CalcTextSize("X");
				float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.FontHandle, result));
					component.FontHandle = result;
				}
			}
			ImGui::PopStyleVar();
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			std::string textString = component.TextString;
			if (UI::PropertyMultiline("Text String", textString)/* && SceneHierarchyPanel::PressedEnter*/)
			{
				CommandHistory::AddAndRemoveCommand(new ChangeStringCommand(component.TextString, textString));
				component.TextString = textString;
			}

			bool useLinear = component.UseLinear;
			if (UI::Property("Linear Filtering", useLinear))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.UseLinear, useLinear));
				component.UseLinear = useLinear;
			}

			float lineSpacing = component.LineSpacing;
			if (UI::Property("Line Spacing", lineSpacing, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.LineSpacing, lineSpacing));
				component.LineSpacing = lineSpacing;
			}

			float kerning = component.Kerning;
			if (UI::Property("Kerning", kerning, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Kerning, kerning));
				component.Kerning = kerning;
			}


			UI::EndPropertyGrid();
		});

		DrawComponent<VideoRendererComponent>("Video Renderer", entity, [=](auto& component)
		{
			UI::BeginPropertyGrid();

			bool useBillboard = component.m_VideoData.UseBillboard;
			if (UI::Property("Use Billboard", useBillboard))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_VideoData.UseBillboard, useBillboard));
				component.m_VideoData.UseBillboard = useBillboard;
			}

			float saturation = component.Saturation;
			if (UI::Property("Color Saturation", saturation, 0.01f, 0.0f, 1.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Saturation, saturation));
				component.Saturation = saturation;
			}

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			ImGui::Text("Video");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string label = "None";
			bool isVideoValid = false;
			if (component.Video != 0)
			{
				if (AssetManager::IsAssetHandleValid(component.Video) && AssetManager::GetAssetType(component.Video) == AssetType::Video)
				{
					const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.Video);
					label = metadata.FilePath.filename().string();
					isVideoValid = true;
				}
				else
				{
					label = "Invalid";
				}
			}

			ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
			buttonLabelSize.x += 20.0f;
			float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x + 4.0f);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 6 });
			ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Video)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.Video, handle));
						component.Video = handle;
						component.m_VideoData.FramePosition = 0;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (isVideoValid)
			{
				ImGui::SameLine();
				ImVec2 xLabelSize = ImGui::CalcTextSize("X");
				float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.Video, result));
					component.Video = result;
				}
			}

			ImGui::PopStyleVar();
			ImGui::PopItemWidth();
			ImGui::NextColumn();
			ImGui::Spacing();

			if (component.Video)
			{
				float volumeFactor = component.m_VideoData.Volume;
				if (UI::Property("Volume", volumeFactor, 0.1f, 0.0f, 100.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.m_VideoData.Volume, volumeFactor));
					component.m_VideoData.Volume = volumeFactor;
				}

				bool useInternalVideoAudio = component.m_VideoData.UseExternalAudio;
				if (UI::Property("Use External Audio", useInternalVideoAudio))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_VideoData.UseExternalAudio, useInternalVideoAudio));
					component.m_VideoData.UseExternalAudio = useInternalVideoAudio;
				}

				bool repeatVideo = component.m_VideoData.RepeatVideo;
				if (UI::Property("Repeat Video", repeatVideo))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_VideoData.RepeatVideo, repeatVideo));
					component.m_VideoData.RepeatVideo = repeatVideo;
				}

				std::string numberOfFramesText = std::to_string(component.m_VideoData.NumberOfFrames);// +" Frames";
				UI::TextProperty("Total Frames", numberOfFramesText);

				std::string millisecondsFormat(4, '\0');
				auto us = snprintf(&millisecondsFormat[0], millisecondsFormat.size(), "%.3f", (float)component.m_VideoData.Milliseconds);
				millisecondsFormat.resize(us);

				std::stringstream ostr1;
				std::stringstream ostr2;
				std::stringstream ostr3;
				std::stringstream ostr4;
				ostr1 << std::setw(2) << std::setfill('0') << (float)component.m_VideoData.Hours << ":";
				ostr2 << std::setw(2) << std::setfill('0') << (float)component.m_VideoData.Minutes << ":";
				ostr3 << std::setw(2) << std::setfill('0') << (float)component.m_VideoData.Seconds << ",";
				ostr4 << std::setw(3) << std::setfill('0') << (float)atof(millisecondsFormat.c_str());

				std::string videoLengthText = ostr1.str() + ostr2.str() + ostr3.str() + ostr4.str();
				UI::TextProperty("Total Playtime", videoLengthText);

				// Consider if it's even worth having seeking integrated into the SceneHierarchyPanel,
				// or if it's better to ONLY use the VideoEngine's seek function inside the VideoRenderer!
				//int64_t framePosition = component.m_VideoData.FramePosition;
				//if (UI::Property("Current Frame", framePosition, 1, 0, component.m_VideoData.NumberOfFrames))
				//{
				//	component.m_VideoData.FramePosition = framePosition;
				//}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<CubeRendererComponent>("Cube Renderer", entity, [this](CubeRendererComponent& component)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec3 materialSpecular = component.MaterialSpecular;
			if (UI::Property("Material Specular", materialSpecular) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(component.MaterialSpecular, materialSpecular));
				component.MaterialSpecular = materialSpecular;
			}

			float materialShininess = component.MaterialShininess;
			if (UI::Property("Material Shininess", materialShininess) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.MaterialShininess, materialShininess));
				component.MaterialShininess = materialShininess;
			}

			rtmcpp::Vec3 transform = rtmcpp::Vec3{ component.CubeTranslation.X, component.CubeTranslation.Y, component.CubeTranslation.Z };
			if (UI::Property("Position Offset", transform) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				rtmcpp::Vec4 result = rtmcpp::Vec4{ transform, 1.0f };
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.CubeTranslation, result));
				component.CubeTranslation = result;
			}

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			RefPtr<Texture2D> defaultTexture = AssetManager::GetAsset<Texture2D>(m_DefaultTexture);
			uint32_t buttonTex = defaultTexture->GetRendererID();

			if (component.TextureHandle)
			{
				RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(component.TextureHandle);
				buttonTex = texture->GetRendererID();
			}

			UI::PropertyButtonImage("Texture", buttonTex, ImVec2{ 100.0f, 100.0f });

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, handle));
						component.TextureHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (component.TextureHandle)
			{
				ImGui::SameLine();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
				if (UI::ButtonProperty("##RemoveTexture", "Remove"))
				{
					buttonTex = defaultTexture->GetRendererID();

					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, result));
					component.TextureHandle = result;
				}
				ImGui::PopStyleVar();

				rtmcpp::Vec2 uv = component.UV;
				if (UI::PropertyDrag("UV Coords", uv, 0.1f, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.UV, uv));
					component.UV = uv;
				}

				bool linear = component.m_AnimationData.UseLinear;
				if (UI::Property("Linear Filtering", linear))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseLinear, linear));
					component.m_AnimationData.UseLinear = linear;
				}

				float animSpeed = component.m_AnimationData.AnimationSpeed;
				if (UI::PropertyDrag("Speed", animSpeed, 0.01f, 0.01f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.m_AnimationData.AnimationSpeed, animSpeed));
					component.m_AnimationData.AnimationSpeed = animSpeed;
				}

				bool useTextureAtlasAnimation = component.m_AnimationData.UseTextureAtlasAnimation;
				if (UI::Property("Use Texture Atlas Animation", useTextureAtlasAnimation))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseTextureAtlasAnimation, useTextureAtlasAnimation));
					component.m_AnimationData.UseTextureAtlasAnimation = useTextureAtlasAnimation;
					component.m_AnimationData.UsePerTextureAnimation = false;
					component.m_AnimationData.UseParallaxScrolling = false;

					if (useTextureAtlasAnimation)
					{
						if (component.m_AnimationData.Textures.size() > 0)
						{
							for (uint32_t i = 0; i < component.m_AnimationData.Textures.size(); i++)
								component.RemoveAnimationTexture(component.m_AnimationData.Textures[i]);
						}

						component.m_AnimationData.ParallaxSpeed.X = 0.0f;
						component.m_AnimationData.ParallaxSpeed.Y = 0.0f;
						component.m_AnimationData.ParallaxDivision = 0.0f;
						component.m_AnimationData.UseCameraParallaxX = false;
						component.m_AnimationData.UseCameraParallaxY = false;
					}
				}

				bool usePerTextureAnimation = component.m_AnimationData.UsePerTextureAnimation;
				if (UI::Property("Use Per Texture Animation", usePerTextureAnimation))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UsePerTextureAnimation, usePerTextureAnimation));
					component.m_AnimationData.UsePerTextureAnimation = usePerTextureAnimation;
					component.m_AnimationData.UseTextureAtlasAnimation = false;
					component.m_AnimationData.UseParallaxScrolling = false;

					if (usePerTextureAnimation)
					{
						component.m_AnimationData.NumberOfTiles = 1;
						component.m_AnimationData.StartIndexX = 0;
						component.m_AnimationData.StartIndexY = 0;
						component.m_AnimationData.Rows = 1;
						component.m_AnimationData.Columns = 1;

						component.m_AnimationData.ParallaxSpeed.X = 0.0f;
						component.m_AnimationData.ParallaxSpeed.Y = 0.0f;
						component.m_AnimationData.ParallaxDivision = 0.0f;
						component.m_AnimationData.UseCameraParallaxX = false;
						component.m_AnimationData.UseCameraParallaxY = false;
					}
				}

				bool useParallaxAnimation = component.m_AnimationData.UseParallaxScrolling;
				if (UI::Property("Use Parallax Animation", useParallaxAnimation))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseParallaxScrolling, useParallaxAnimation));
					component.m_AnimationData.UseParallaxScrolling = useParallaxAnimation;
					component.m_AnimationData.UseTextureAtlasAnimation = false;
					component.m_AnimationData.UsePerTextureAnimation = false;

					if (useParallaxAnimation)
					{
						component.m_AnimationData.NumberOfTiles = 1;
						component.m_AnimationData.StartIndexX = 0;
						component.m_AnimationData.StartIndexY = 0;
						component.m_AnimationData.Rows = 1;
						component.m_AnimationData.Columns = 1;

						if (component.m_AnimationData.Textures.size() > 0)
						{
							for (uint32_t i = 0; i < component.m_AnimationData.Textures.size(); i++)
								component.RemoveAnimationTexture(component.m_AnimationData.Textures[i]);
						}
					}
				}

				if (component.m_AnimationData.UseTextureAtlasAnimation)
				{
					int numTiles = component.m_AnimationData.NumberOfTiles;
					if (UI::Property("Num of Tiles", numTiles, 1, 1, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.NumberOfTiles, numTiles));
						component.m_AnimationData.NumberOfTiles = numTiles;
					}

					int startIndexX = component.m_AnimationData.StartIndexX;

					if ((component.m_AnimationData.Rows - 1) < 1)
						startIndexX = 0;

					if (UI::PropertyU32("Start Index X", startIndexX, 1, 0, (component.m_AnimationData.Rows - 1)) &&
						(SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.StartIndexX, startIndexX));
						component.m_AnimationData.StartIndexX = startIndexX;
					}

					int startIndexY = component.m_AnimationData.StartIndexY;

					if ((component.m_AnimationData.Columns - 1) < 1)
						startIndexY = 0;

					if (UI::PropertyU32("Start Index Y", startIndexY, 1, 0, (component.m_AnimationData.Columns - 1)) &&
						(SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.StartIndexY, startIndexY));
						component.m_AnimationData.StartIndexY = startIndexY;
					}

					int rows = component.m_AnimationData.Rows;
					if (UI::Property("Rows", rows, 1, 1, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.Rows, rows));
						component.m_AnimationData.Rows = rows;
					}

					int columns = component.m_AnimationData.Columns;
					if (UI::Property("Columns", columns, 1, 1, INT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.Columns, columns));
						component.m_AnimationData.Columns = columns;
					}
				}

				if (component.m_AnimationData.UsePerTextureAnimation)
				{
					if (component.m_AnimationData.Textures.size() == 0)
					{
						component.AddAnimationTexture(component.TextureHandle);
					}

					int startIndex = component.m_AnimationData.StartIndex;
					uint32_t minIndex = 0;
					if (UI::Property("Start Index", startIndex, 1, minIndex, (int)component.m_AnimationData.Textures.size() - 1) &&
						(SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeInt32Command(component.m_AnimationData.StartIndex, startIndex));
						component.m_AnimationData.StartIndex = startIndex;
					}

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
					if (UI::ButtonProperty("", "Add Texture"))
					{
						component.AddAnimationTexture(m_DefaultTexture);
					}
					ImGui::PopStyleVar();

					for (uint32_t i = 0; i < component.m_AnimationData.Textures.size(); i++)
					{
						uint32_t buttonTex0 = defaultTexture->GetRendererID();
						RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(component.m_AnimationData.Textures[i]);

						if (texture)
							buttonTex0 = texture->GetRendererID();

						std::string imageName = "Texture " + std::to_string(i);

						ImGui::PushID(i);

						UI::PropertyButtonImage(imageName.c_str(), buttonTex0, ImVec2{ 100.0f, 100.0f });

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
							{
								AssetHandle handle = *(AssetHandle*)payload->Data;

								if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
								{
									CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.m_AnimationData.Textures[i], handle));
									component.m_AnimationData.Textures[i] = handle;
								}
								else
								{
									NZ_CORE_WARN("Wrong asset type!");
								}
							}
							ImGui::EndDragDropTarget();
						}

						ImGui::SameLine();

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
						if (UI::ButtonProperty("", "Remove"))
						{
							AssetHandle result = 0;
							CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.m_AnimationData.Textures[i], result));
							component.RemoveAnimationTexture(component.m_AnimationData.Textures[i]);

							if (i == 0)
							{
								if (component.m_AnimationData.Textures.size() > 0)
								{
									if (component.TextureHandle != component.m_AnimationData.Textures[0] && component.m_AnimationData.Textures[0])
									{
										component.TextureHandle = component.m_AnimationData.Textures[0];
									}
								}
								else
								{
									component.m_AnimationData.UsePerTextureAnimation = false;
								}
							}
						}
						ImGui::PopStyleVar();

						ImGui::PopID();
					}

					if (component.m_AnimationData.Textures.size() > 0)
					{
						if (component.TextureHandle != component.m_AnimationData.Textures[(uint32_t)component.m_AnimationData.StartIndex])
							component.TextureHandle = component.m_AnimationData.Textures[(uint32_t)component.m_AnimationData.StartIndex];
					}
				}

				if (component.m_AnimationData.UseParallaxScrolling)
				{
					rtmcpp::Vec2 parallaxSpeed = component.m_AnimationData.ParallaxSpeed;
					if (UI::PropertyDrag("Speed", parallaxSpeed, 0.1f, -FLT_MAX, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.m_AnimationData.ParallaxSpeed, parallaxSpeed));
						component.m_AnimationData.ParallaxSpeed = parallaxSpeed;
					}

					float division = component.m_AnimationData.ParallaxDivision;
					if (UI::Property("Division", division, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.m_AnimationData.ParallaxDivision, division));
						component.m_AnimationData.ParallaxDivision = division;
					}

					bool cameraParallaxX = component.m_AnimationData.UseCameraParallaxX;
					if (UI::Property("Camera Parallax X", cameraParallaxX))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseCameraParallaxX, cameraParallaxX));
						component.m_AnimationData.UseCameraParallaxX = cameraParallaxX;
					}

					bool cameraParallaxY = component.m_AnimationData.UseCameraParallaxY;
					if (UI::Property("Camera Parallax Y", cameraParallaxY))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.m_AnimationData.UseCameraParallaxY, cameraParallaxY));
						component.m_AnimationData.UseCameraParallaxY = cameraParallaxY;
					}
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<PyramidRendererComponent>("Pyramid Renderer", entity, [this](PyramidRendererComponent& component)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec3 materialSpecular = component.MaterialSpecular;
			if (UI::Property("Material Specular", materialSpecular) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(component.MaterialSpecular, materialSpecular));
				component.MaterialSpecular = materialSpecular;
			}

			float materialShininess = component.MaterialShininess;
			if (UI::Property("Material Shininess", materialShininess) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.MaterialShininess, materialShininess));
				component.MaterialShininess = materialShininess;
			}

			rtmcpp::Vec3 transform = rtmcpp::Vec3{ component.PyramidTranslation.X, component.PyramidTranslation.Y, component.PyramidTranslation.Z };
			if (UI::Property("Position Offset", transform) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				rtmcpp::Vec4 result = rtmcpp::Vec4{ transform, 1.0f };
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.PyramidTranslation, result));
				component.PyramidTranslation = result;
			}

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			RefPtr<Texture2D> defaultTexture = AssetManager::GetAsset<Texture2D>(m_DefaultTexture);
			uint32_t buttonTex = defaultTexture->GetRendererID();

			if (component.TextureHandle)
			{
				RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(component.TextureHandle);
				buttonTex = texture->GetRendererID();
			}

			UI::PropertyButtonImage("Texture", buttonTex, ImVec2{ 100.0f, 100.0f });

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, handle));
						component.TextureHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (component.TextureHandle)
			{
				ImGui::SameLine();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
				if (UI::ButtonProperty("##RemoveTexture", "Remove"))
				{
					buttonTex = defaultTexture->GetRendererID();
					
					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, result));
					component.TextureHandle = result;
				}
				ImGui::PopStyleVar();

				rtmcpp::Vec2 uv = component.UV;
				if (UI::PropertyDrag("UV Coords", uv, 0.1f, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.UV, uv));
					component.UV = uv;
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<TriangularPrismRendererComponent>("Triangular Prism Renderer", entity, [this](TriangularPrismRendererComponent& component)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec3 materialSpecular = component.MaterialSpecular;
			if (UI::Property("Material Specular", materialSpecular) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(component.MaterialSpecular, materialSpecular));
				component.MaterialSpecular = materialSpecular;
			}

			float materialShininess = component.MaterialShininess;
			if (UI::Property("Material Shininess", materialShininess) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.MaterialShininess, materialShininess));
				component.MaterialShininess = materialShininess;
			}

			rtmcpp::Vec3 transform = rtmcpp::Vec3{ component.TriangleTranslation.X, component.TriangleTranslation.Y, component.TriangleTranslation.Z };
			if (UI::Property("Position Offset", transform) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				rtmcpp::Vec4 result = rtmcpp::Vec4{ transform, 1.0f };
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.TriangleTranslation, result));
				component.TriangleTranslation = result;
			}

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			RefPtr<Texture2D> defaultTexture = AssetManager::GetAsset<Texture2D>(m_DefaultTexture);
			uint32_t buttonTex = defaultTexture->GetRendererID();

			if (component.TextureHandle)
			{
				RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(component.TextureHandle);
				buttonTex = texture->GetRendererID();
			}

			UI::PropertyButtonImage("Texture", buttonTex, ImVec2{ 100.0f, 100.0f });

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, handle));
						component.TextureHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (component.TextureHandle)
			{
				ImGui::SameLine();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
				if (UI::ButtonProperty("##RemoveTexture", "Remove"))
				{
					buttonTex = defaultTexture->GetRendererID();

					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, result));
					component.TextureHandle = result;
				}
				ImGui::PopStyleVar();

				rtmcpp::Vec2 uv = component.UV;
				if (UI::PropertyDrag("UV Coords", uv, 0.1f, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.UV, uv));
					component.UV = uv;
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<PlaneRendererComponent>("Plane Renderer", entity, [this](PlaneRendererComponent& component)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec3 materialSpecular = component.MaterialSpecular;
			if (UI::Property("Material Specular", materialSpecular) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(component.MaterialSpecular, materialSpecular));
				component.MaterialSpecular = materialSpecular;
			}

			float materialShininess = component.MaterialShininess;
			if (UI::Property("Material Shininess", materialShininess) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.MaterialShininess, materialShininess));
				component.MaterialShininess = materialShininess;
			}

			rtmcpp::Vec3 transform = rtmcpp::Vec3{ component.PlaneTranslation.X, component.PlaneTranslation.Y, component.PlaneTranslation.Z };
			if (UI::Property("Position Offset", transform) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				rtmcpp::Vec4 result = rtmcpp::Vec4{ transform, 1.0f };
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.PlaneTranslation, result));
				component.PlaneTranslation = result;
			}

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			RefPtr<Texture2D> defaultTexture = AssetManager::GetAsset<Texture2D>(m_DefaultTexture);
			uint32_t buttonTex = defaultTexture->GetRendererID();

			if (component.TextureHandle)
			{
				RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(component.TextureHandle);
				buttonTex = texture->GetRendererID();
			}

			UI::PropertyButtonImage("Texture", buttonTex, ImVec2{ 100.0f, 100.0f });

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, handle));
						component.TextureHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (component.TextureHandle)
			{
				ImGui::SameLine();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
				if (UI::ButtonProperty("##RemoveTexture", "Remove"))
				{
					buttonTex = defaultTexture->GetRendererID();

					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, result));
					component.TextureHandle = result;
				}
				ImGui::PopStyleVar();

				rtmcpp::Vec2 uv = component.UV;
				if (UI::PropertyDrag("UV Coords", uv, 0.1f, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.UV, uv));
					component.UV = uv;
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<OBJRendererComponent>("Obj Renderer", entity, [this](OBJRendererComponent& component)
		{
			UI::BeginPropertyGrid();

			if (component.ModelHandle != 0)
			{
				RefPtr<ObjModel> model = AssetManager::GetAsset<ObjModel>(component.ModelHandle);
				if (model)
				{
					rtmcpp::Vec3 materialSpecular = model->m_MaterialProperties.MaterialSpecular;
					if (UI::Property("Material Specular", materialSpecular) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeVec3Command(model->m_MaterialProperties.MaterialSpecular, materialSpecular));
						model->m_MaterialProperties.MaterialSpecular = materialSpecular;
					}

					float materialShininess = model->m_MaterialProperties.MaterialShininess;
					if (UI::Property("Material Shininess", materialShininess) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(model->m_MaterialProperties.MaterialShininess, materialShininess));
						model->m_MaterialProperties.MaterialShininess = materialShininess;
					}
				}
			}

			rtmcpp::Vec3 transform = rtmcpp::Vec3{ component.ObjTranslation.X, component.ObjTranslation.Y, component.ObjTranslation.Z };
			if (UI::Property("Position Offset", transform) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				rtmcpp::Vec4 result = rtmcpp::Vec4{ transform, 1.0f };
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.ObjTranslation, result));
				component.ObjTranslation = result;
			}

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			ImGui::Text("Model");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string label = "None";
			bool isModelValid = false;

			if (component.ModelHandle != 0)
			{
				if (AssetManager::IsAssetHandleValid(component.ModelHandle) && AssetManager::GetAssetType(component.ModelHandle) == AssetType::ObjModel)
				{
					const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.ModelHandle);
					label = metadata.FilePath.filename().string();
					isModelValid = true;
				}
				else
				{
					label = "Invalid";
				}
			}

			ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
			buttonLabelSize.x += 20.0f;
			float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 6 });
			ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::ObjModel)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.ModelHandle, handle));
						component.ModelHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (isModelValid)
			{
				ImGui::SameLine();
				ImVec2 xLabelSize = ImGui::CalcTextSize("X");
				float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.ModelHandle, result));
					component.ModelHandle = result;
				}
			}

			ImGui::PopStyleVar();
			ImGui::PopItemWidth();
			ImGui::NextColumn();
			ImGui::Spacing();

			if (component.ModelHandle)
			{
				ImGui::PushID("textureFilePath");

				RefPtr<Texture2D> defaultTexture = AssetManager::GetAsset<Texture2D>(m_DefaultTexture);
				uint32_t buttonTex = defaultTexture->GetRendererID();

				RefPtr<ObjModel> modelTexture = AssetManager::GetAsset<ObjModel>(component.ModelHandle);
				if (modelTexture)
				{
					if (AssetManager::IsAssetHandleValid(modelTexture->GetTextureHandle()))
					{
						RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(modelTexture->GetTextureHandle());
						buttonTex = texture->GetRendererID();
					}

					UI::PropertyButtonImage("Texture", buttonTex, ImVec2{ 100.0f, 100.0f });

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							AssetHandle handle = *(AssetHandle*)payload->Data;

							if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
							{
								CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(modelTexture->m_TextureHandle, handle));
								modelTexture->m_TextureHandle = handle;
							}
							else
							{
								NZ_CORE_WARN("Wrong asset type!");
							}
						}
						ImGui::EndDragDropTarget();
					}

					if (modelTexture->GetTextureHandle())
					{
						ImGui::SameLine();

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
						if (UI::ButtonProperty("##RemoveTexture", "Remove"))
						{
							buttonTex = defaultTexture->GetRendererID();
							AssetHandle result = 0;
							CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(modelTexture->m_TextureHandle, result));
							modelTexture->m_TextureHandle = result;
						}
						ImGui::PopStyleVar();

						rtmcpp::Vec2 uv = component.UV;
						if (UI::PropertyDrag("UV Coords", uv, 0.1f, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
						{
							CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.UV, uv));
							component.UV = uv;
						}
					}
				}

				ImGui::PopID();
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<ButtonWidgetComponent>("Button Layout", entity, [=](ButtonWidgetComponent& component)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			float radius = component.Radius;
			if (UI::PropertyDrag("Corner Radius", radius, 0.1f, 0.0f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Radius, radius));
				component.Radius = radius;
			}

			rtmcpp::Vec2 dimensions = component.Dimensions;
			if (UI::PropertyDrag("Corner Dimensions", dimensions, 0.1f, 0.0f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.Dimensions, dimensions));
				component.Dimensions = dimensions;
			}

			bool invertCorners = component.InvertCorners;
			if (UI::Property("Invert Corners", invertCorners))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.InvertCorners, invertCorners));
				component.InvertCorners = invertCorners;
			}

			ImGui::Text("Texture");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string label = "None";
			bool isTextureValid = false;
			if (component.TextureHandle != 0)
			{
				if (AssetManager::IsAssetHandleValid(component.TextureHandle) && AssetManager::GetAssetType(component.TextureHandle) == AssetType::Texture2D)
				{
					const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.TextureHandle);
					label = metadata.FilePath.filename().string();
					isTextureValid = true;
				}
				else
				{
					label = "Invalid";
				}
			}

			ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
			buttonLabelSize.x += 20.0f;
			float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 6 });
			ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, handle));
						component.TextureHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (isTextureValid)
			{
				ImGui::SameLine();
				ImVec2 xLabelSize = ImGui::CalcTextSize("X");
				float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, result));
					component.TextureHandle = result;
				}
			}

			ImGui::PopStyleVar();
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if (component.TextureHandle)
			{
				rtmcpp::Vec2 uv = component.UV;
				if (UI::PropertyDrag("UV Coords", uv, 0.1f, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.UV, uv));
					component.UV = uv;
				}

				bool linear = component.UseLinear;
				if (UI::Property("Linear Filtering", linear))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.UseLinear, linear));
					component.UseLinear = linear;
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<CircleWidgetComponent>("Circle Layout", entity, [=](CircleWidgetComponent& component)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec4 color = component.Color;
			if (UI::PropertyColor("Color", color) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec4Command(component.Color, color));
				component.Color = color;
			}

			float fade = component.Fade;
			if (UI::PropertyDrag("Fade", fade, 0.00025f, 0.0f, 1.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(component.Fade, fade));
				component.Fade = fade;
			}

			ImGui::Text("Texture");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string label = "None";
			bool isTextureValid = false;
			if (component.TextureHandle != 0)
			{
				if (AssetManager::IsAssetHandleValid(component.TextureHandle) && AssetManager::GetAssetType(component.TextureHandle) == AssetType::Texture2D)
				{
					const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.TextureHandle);
					label = metadata.FilePath.filename().string();
					isTextureValid = true;
				}
				else
				{
					label = "Invalid";
				}
			}

			ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
			buttonLabelSize.x += 20.0f;
			float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 6 });
			ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
					{
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, handle));
						component.TextureHandle = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (isTextureValid)
			{
				ImGui::SameLine();
				ImVec2 xLabelSize = ImGui::CalcTextSize("X");
				float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.TextureHandle, result));
					component.TextureHandle = result;
				}
			}

			ImGui::PopStyleVar();
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if (component.TextureHandle)
			{
				rtmcpp::Vec2 uv = component.UV;
				if (UI::PropertyDrag("UV Coords", uv, 0.1f, 0.1f, FLT_MAX) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(component.UV, uv));
					component.UV = uv;
				}

				bool linear = component.UseLinear;
				if (UI::Property("Linear Filtering", linear))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.UseLinear, linear));
					component.UseLinear = linear;
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<Rigidbody2DComponent>("Rigidbody 2D", entity, [](Rigidbody2DComponent& rb2dc)
		{
			UI::BeginPropertyGrid();

			// Rigidbody2D Type
			const char* rb2dTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
			Rigidbody2DComponent::BodyType bodyType = rb2dc.Type;
			if (UI::PropertyDropdown("Type", rb2dTypeStrings, 3, (int*)&bodyType))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeEnumCommand(rb2dc.Type, bodyType));
			}

			if (rb2dc.Type == Rigidbody2DComponent::BodyType::Dynamic)
			{
				bool fixedRotation = rb2dc.FixedRotation;
				if (UI::Property("Fixed Rotation", fixedRotation))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(rb2dc.FixedRotation, fixedRotation));
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& bc2dc)
		{
			UI::BeginPropertyGrid();

			// This needs to be updated so the user can change these values in C# scripting!
			rtmcpp::Vec2 offset = bc2dc.Offset;
			if (UI::Property("Offset", offset, 0.01f, 0.0f, 0.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(bc2dc.Offset, offset));
				bc2dc.Offset = offset;
			}

			rtmcpp::Vec2 size = bc2dc.Size;
			if (UI::Property("Size", size, 0.01f, 0.01f, 0.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(bc2dc.Size, size));
				bc2dc.Size = size;
			}
			
			float density = bc2dc.Density;
			if (UI::Property("Density", density, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(bc2dc.Density, density));
				bc2dc.Density = density;
			}
			
			float friction = bc2dc.Friction;
			if (UI::Property("Friction", friction, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(bc2dc.Friction, friction));
				bc2dc.Friction = friction;
			}
			
			float restitution = bc2dc.Restitution;
			if (UI::Property("Restitution", restitution, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(bc2dc.Restitution, restitution));
				bc2dc.Restitution = restitution;
			}
			
			float restitutionThreshold = bc2dc.RestitutionThreshold;
			if (UI::Property("RestitutionThreshold", restitutionThreshold, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(bc2dc.RestitutionThreshold, restitutionThreshold));
				bc2dc.RestitutionThreshold = restitutionThreshold;
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](CircleCollider2DComponent& cc2dc)
		{
			UI::BeginPropertyGrid();

			// This needs to be updated so the user can change these values in C# scripting!
			rtmcpp::Vec2 offset = cc2dc.Offset;
			if (UI::Property("Offset", offset, 0.01f, 0.0f, 0.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(cc2dc.Offset, offset));
				cc2dc.Offset = offset;
			}

			float radius = cc2dc.Radius;
			if (UI::Property("Radius", radius, 0.01f, 0.01f, 0.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(cc2dc.Radius, radius));
				cc2dc.Radius = radius;
			}
			
			float density = cc2dc.Density;
			if (UI::Property("Density", density, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(cc2dc.Density, density));
				cc2dc.Density = density;
			}
			
			float friction = cc2dc.Friction;
			if (UI::Property("Friction", friction, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(cc2dc.Friction, friction));
				cc2dc.Friction = friction;
			}
			
			float restitution = cc2dc.Restitution;			
			if (UI::Property("Restitution", restitution, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(cc2dc.Restitution, restitution));
				cc2dc.Restitution = restitution;
			}
			
			float restitutionThreshold = cc2dc.RestitutionThreshold;
			if (UI::Property("RestitutionThreshold", restitutionThreshold, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(cc2dc.RestitutionThreshold, restitutionThreshold));
				cc2dc.RestitutionThreshold = restitutionThreshold;
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<TriangleCollider2DComponent>("Triangle Collider 2D", entity, [](TriangleCollider2DComponent& tc2dc)
		{
			UI::BeginPropertyGrid();

			// This needs to be updated so the user can change these values in C# scripting!
			rtmcpp::Vec2 offset = tc2dc.Offset;
			if (UI::Property("Offset", offset, 0.01f, 0.0f, 0.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(tc2dc.Offset, offset));
				tc2dc.Offset = offset;
			}
			
			rtmcpp::Vec2 size = tc2dc.Size;
			if (UI::Property("Size", size, 0.01f, 0.01f, 0.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(tc2dc.Size, size));
				tc2dc.Size = size;
			}
			
			float density = tc2dc.Density;
			if (UI::Property("Density", density, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(tc2dc.Density, density));
				tc2dc.Density = density;
			}
			
			float friction = tc2dc.Friction;
			if (UI::Property("Friction", friction, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(tc2dc.Friction, friction));
				tc2dc.Friction = friction;
			}
			
			float restitution = tc2dc.Restitution;
			if (UI::Property("Restitution", restitution, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(tc2dc.Restitution, restitution));
				tc2dc.Restitution = restitution;
			}
			
			float restitutionThreshold = tc2dc.RestitutionThreshold;
			if (UI::Property("RestitutionThreshold", restitutionThreshold, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(tc2dc.RestitutionThreshold, restitutionThreshold));
				tc2dc.RestitutionThreshold = restitutionThreshold;
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<CapsuleCollider2DComponent>("Capsule Collider 2D", entity, [](CapsuleCollider2DComponent& cc2dc)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec2 offset = cc2dc.Offset;
			if (UI::Property("Offset", offset, 0.01f, 0.0f, 0.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(cc2dc.Offset, offset));
				cc2dc.Offset = offset;
			}
			
			rtmcpp::Vec2 size = cc2dc.Size;
			if (UI::Property("Size", size, 0.01f, 0.01f, 0.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(cc2dc.Size, size));
				cc2dc.Size = size;
			}
			
			float density = cc2dc.Density;
			if (UI::Property("Density", density, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(cc2dc.Density, density));
				cc2dc.Density = density;
			}
			
			float friction = cc2dc.Friction;
			if (UI::Property("Friction", friction, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(cc2dc.Friction, friction));
				cc2dc.Friction = friction;
			}
			
			float restitution = cc2dc.Restitution;
			if (UI::Property("Restitution", restitution, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(cc2dc.Restitution, restitution));
				cc2dc.Restitution = restitution;
			}
			
			float restitutionThreshold = cc2dc.RestitutionThreshold;
			if (UI::Property("RestitutionThreshold", restitutionThreshold, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(cc2dc.RestitutionThreshold, restitutionThreshold));
				cc2dc.RestitutionThreshold = restitutionThreshold;
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<MeshCollider2DComponent>("Mesh Collider 2D", entity, [](MeshCollider2DComponent& mc2dc)
		{
			UI::BeginPropertyGrid();

			rtmcpp::Vec2 offset = mc2dc.Offset;
			if (UI::Property("Offset", offset, 0.01f, 0.0f, 0.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(mc2dc.Offset, offset));
				mc2dc.Offset = offset;
			}
			
			rtmcpp::Vec2 size = mc2dc.Size;
			if (UI::Property("Size", size, 0.01f, 0.01f, 0.0f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(mc2dc.Size, size));
				mc2dc.Size = size;
			}
			
			float density = mc2dc.Density;
			if (UI::Property("Density", density, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(mc2dc.Density, density));
				mc2dc.Density = density;
			}
			
			float friction = mc2dc.Friction;
			if (UI::Property("Friction", friction, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(mc2dc.Friction, friction));
				mc2dc.Friction = friction;
			}
			
			float restitution = mc2dc.Restitution;
			if (UI::Property("Restitution", restitution, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(mc2dc.Restitution, restitution));
				mc2dc.Restitution = restitution;
			}
			
			float restitutionThreshold = mc2dc.RestitutionThreshold;
			if (UI::Property("RestitutionThreshold", restitutionThreshold, 0.01f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(mc2dc.RestitutionThreshold, restitutionThreshold));
				mc2dc.RestitutionThreshold = restitutionThreshold;
			}

			if (mc2dc.Positions.size() <= 7)
			{
				if (UI::ButtonProperty("Add Position", "Add"))
					mc2dc.AddPosition(rtmcpp::Vec2(0.0f, 0.0f));
			}

			if (mc2dc.Positions.size() > 0)
			{
				for (uint32_t i = 0; i < mc2dc.Positions.size(); i++)
				{
					ImGui::PushID(i);

					std::string positionName = "Position " + std::to_string(i);
					rtmcpp::Vec2 position = mc2dc.Positions[i];
					if (UI::Property(positionName.c_str(), position, 0.1f) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeVec2Command(mc2dc.Positions[i], position));
						mc2dc.Positions[i] = position;
					}

					std::string removeName = "Remove Position " + std::to_string(i);
					if (UI::ButtonProperty(removeName.c_str(), "Remove"))
						mc2dc.RemovePosition(i);

					ImGui::PopID();
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<AudioSourceComponent>("Audio Source", entity, [&entity](AudioSourceComponent& component)
		{
			UI::BeginPropertyGrid();
			auto& config = component.Config;

			ImGui::Text("Audio");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string label = "None";
			bool isAudioValid = false;
			if (component.Audio != 0)
			{
				if (AssetManager::IsAssetHandleValid(component.Audio) && AssetManager::GetAssetType(component.Audio) == AssetType::Audio)
				{
					const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.Audio);
					label = metadata.FilePath.filename().string();
					isAudioValid = true;
				}
				else
				{
					label = "Invalid";
				}
			}

			ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
			buttonLabelSize.x += 20.0f;
			float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x + 4.0f);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 6 });
			ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::GetAssetType(handle) == AssetType::Audio)
					{
						if (component.AudioSourceData.Playlist.size() > 0)
						{
							CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.AudioSourceData.Playlist[0], handle));
							component.AudioSourceData.Playlist[0] = handle;
						}

						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.Audio, handle));
						component.Audio = handle;
					}
					else
					{
						NZ_CORE_WARN("Wrong asset type!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (isAudioValid)
			{
				ImGui::SameLine();
				ImVec2 xLabelSize = ImGui::CalcTextSize("X");
				float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
				
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					AssetHandle result = 0;
					CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.Audio, result));
					component.Audio = result;
				}
			}

			ImGui::PopStyleVar();
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if (component.Audio != 0)
			{
				RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);

				float volumeMultiplier = config.VolumeMultiplier;
				if (UI::Property("Volume Multiplier", volumeMultiplier) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.VolumeMultiplier, volumeMultiplier));
					config.VolumeMultiplier = volumeMultiplier;
				}

				float pitchMultiplier = config.PitchMultiplier;
				if (UI::Property("Pitch Multiplier", pitchMultiplier) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.PitchMultiplier, pitchMultiplier));
					config.PitchMultiplier = pitchMultiplier;
				}

				bool playOnAwake = config.PlayOnAwake;
				if (UI::Property("Play On Awake", playOnAwake))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(config.PlayOnAwake, playOnAwake));
					config.PlayOnAwake = playOnAwake;
				}

				if (component.AudioSourceData.UsePlaylist)
				{
					bool looping = false;
					audioSource->SetLooping(looping);
					config.Looping = looping;

					ImGui::BeginDisabled();
					UI::Property("Looping", looping);
					ImGui::EndDisabled();
				}
				else
				{
					bool looping = config.Looping;
					if (UI::Property("Looping", looping))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(config.Looping, looping));
						audioSource->SetLooping(looping);
						config.Looping = looping;
					}
				}

				bool spatialization = config.Spatialization;
				if (UI::Property("Spatialization", spatialization))
				{
					CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(config.Spatialization, spatialization));
					audioSource->SetSpatialization(spatialization);
					config.Spatialization = spatialization;
				}

				if (config.Looping)
				{
					bool usePlaylist = false;
					component.AudioSourceData.UsePlaylist = usePlaylist;
					ImGui::BeginDisabled();
					UI::Property("Use Playlist", usePlaylist);
					ImGui::EndDisabled();
				}
				else
				{
					bool usePlaylist = component.AudioSourceData.UsePlaylist;
					if (UI::Property("Use Playlist", usePlaylist))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.AudioSourceData.UsePlaylist, usePlaylist));
						component.AudioSourceData.UsePlaylist = usePlaylist;
					}
				}

				if (component.AudioSourceData.UsePlaylist)
				{
					bool repeatPlaylist = component.AudioSourceData.RepeatPlaylist;
					if (UI::Property("Repeat Playlist", repeatPlaylist))
					{
						if (component.AudioSourceData.RepeatAfterSpecificTrackPlays)
							component.AudioSourceData.RepeatAfterSpecificTrackPlays = false;

						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.AudioSourceData.RepeatPlaylist, repeatPlaylist));

						component.AudioSourceData.RepeatPlaylist = repeatPlaylist;
					}

					bool repeatAfterSpecificTrackPlays = component.AudioSourceData.RepeatAfterSpecificTrackPlays;
					if (UI::Property("Loop When Start Index Plays", repeatAfterSpecificTrackPlays))
					{
						if (component.AudioSourceData.RepeatPlaylist)
							component.AudioSourceData.RepeatPlaylist = false;

						CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.AudioSourceData.RepeatAfterSpecificTrackPlays, repeatAfterSpecificTrackPlays));

						component.AudioSourceData.RepeatAfterSpecificTrackPlays = repeatAfterSpecificTrackPlays;
					}

					uint32_t startIndex = component.AudioSourceData.StartIndex;
					uint32_t* maxValue = (uint32_t*)(component.AudioSourceData.Playlist.size() - 1);
					if (UI::PropertyScalar("Playlist Start Index", startIndex, 1.0f, 0, maxValue) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeUint32Command(component.AudioSourceData.StartIndex, startIndex));
						component.AudioSourceData.StartIndex = startIndex;
					}
				}

				if (config.Spatialization)
				{
					const char* attenuationTypeStrings[] = { "None", "Inverse", "Linear", "Exponential" };
					int attenuationType = static_cast<int>(config.AttenuationModel);
					if (UI::Property("Attenuation Model", attenuationType, attenuationTypeStrings, 4))
					{
						AttenuationModelType result = static_cast<AttenuationModelType>(attenuationType);
						CommandHistory::AddAndRemoveCommand(new ChangeEnumCommand(config.AttenuationModel, result));
						config.AttenuationModel = result;
					}

					float rollOff = config.RollOff;
					if (UI::Property("Roll Off", rollOff) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.RollOff, rollOff));
					}

					float minGain = config.MinGain;
					if (UI::Property("Min Gain", minGain) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.MinGain, minGain));
					}

					float maxGain = config.MaxGain;
					if (UI::Property("Max Gain", maxGain) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.MaxGain, maxGain));
					}

					float minDistance = config.MinDistance;
					if (UI::Property("Min Distance", minDistance) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.MinDistance, minDistance));
					}

					float maxDistance = config.MaxDistance;
					if (UI::Property("Max Distance", maxDistance) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.MaxDistance, maxDistance));
					}

					float innerAngle = rtmcpp::Degrees(config.ConeInnerAngle);
					if (UI::Property("Cone Inner Angle", innerAngle) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						float result = rtmcpp::Radians(innerAngle);
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.ConeInnerAngle, result));
						config.ConeInnerAngle = result;
					}
					
					float outerAngle = rtmcpp::Degrees(config.ConeOuterAngle);
					if (UI::Property("Cone Outer Angle", outerAngle) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						float result = rtmcpp::Radians(outerAngle);
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.ConeOuterAngle, result));
						config.ConeOuterAngle = result;
					}
					
					float outerGain = config.ConeOuterGain;
					if (UI::Property("Cone Outer Gain", outerGain) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.ConeOuterGain, outerGain));
					}

					float dopplerFactor = config.DopplerFactor;
					if (UI::Property("Doppler Factor", dopplerFactor) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
					{
						CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.DopplerFactor, dopplerFactor));
					}

				}

				if (component.Audio)
				{
					const rtmcpp::Mat4 inverted = rtmcpp::Inverse(entity.GetComponent<TransformComponent>().GetTransform());
					const rtmcpp::Vec3 forward = rtm::vector_normalize3(inverted.Value.z_axis);
					audioSource->SetConfig(config);
					audioSource->SetPosition(entity.GetComponent<TransformComponent>().Translation);
					audioSource->SetDirection(rtmcpp::Vec3{ -forward.X, -forward.Y, -forward.Z });
				}
			}

			if (component.AudioSourceData.UsePlaylist)
			{
				if (component.AudioSourceData.Playlist.size() == 0)
					component.AddAudioSource(component.Audio);

				if (component.AudioSourceData.PlaylistCopy.size() > 0 && component.AudioSourceData.Playlist.size() != component.AudioSourceData.PlaylistCopy.size()/* && component.AudioSourceData.ChangedUsingTextureAnimation*/)
				{
					if (component.AudioSourceData.Playlist.size() != component.AudioSourceData.PlaylistCopy.size())
						component.AudioSourceData.Playlist.resize(component.AudioSourceData.PlaylistCopy.size());

					for (uint32_t i = 0; i < component.AudioSourceData.PlaylistCopy.size(); i++)
					{
						if (component.AudioSourceData.Playlist[i] != component.AudioSourceData.PlaylistCopy[i])
							component.AudioSourceData.Playlist[i] = component.AudioSourceData.PlaylistCopy[i];
					}
				}

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 12, 8 });
				if (UI::ButtonProperty("", "Add Audio"))
				{
					if (component.Audio)
					{
						AssetHandle tempHandle = 0;
						component.AddAudioSource(tempHandle);
					}
				}
				ImGui::PopStyleVar();

				if (component.AudioSourceData.PlaylistCopy.size() != component.AudioSourceData.Playlist.size())
					component.AudioSourceData.PlaylistCopy.resize(component.AudioSourceData.Playlist.size());

				for (uint32_t i = 0; i < component.AudioSourceData.Playlist.size(); i++)
				{
					if (component.AudioSourceData.PlaylistCopy[i] != component.AudioSourceData.Playlist[i])
						component.AudioSourceData.PlaylistCopy[i] = component.AudioSourceData.Playlist[i];

					ImGui::PushID(i);
					ImGui::Text("Audio");
					ImGui::NextColumn();
					ImGui::PushItemWidth(-1);

					std::string label = "None";
					bool isAudioValid = false;
					if (component.AudioSourceData.Playlist[i] != 0)
					{
						if (AssetManager::IsAssetHandleValid(component.AudioSourceData.Playlist[i]) && AssetManager::GetAssetType(component.AudioSourceData.Playlist[i]) == AssetType::Audio)
						{
							const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.AudioSourceData.Playlist[i]);
							label = metadata.FilePath.filename().string();
							isAudioValid = true;
						}
						else
						{
							label = "Invalid";
						}
					}

					ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
					buttonLabelSize.x += 20.0f;
					float buttonLabelWidth = std::max<float>(100.0f, buttonLabelSize.x + 4.0f);

					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 6, 6 });
					ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							AssetHandle handle = *(AssetHandle*)payload->Data;

							if (AssetManager::GetAssetType(handle) == AssetType::Audio)
							{
								if (i == 0)
								{
									CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.Audio, handle));
									component.Audio = handle;
								}

								CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.AudioSourceData.Playlist[i], handle));
								component.AudioSourceData.Playlist[i] = handle;
								component.AudioSourceData.PlaylistCopy[i] = handle;
							}
							else
							{
								NZ_CORE_WARN("Wrong asset type!");
							}
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::SameLine();
					ImVec2 xLabelSize = ImGui::CalcTextSize("X");
					float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;

					if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
					{
						AssetHandle result = 0;
						CommandHistory::AddAndRemoveCommand(new ChangeAssetHandleCommand(component.AudioSourceData.Playlist[i], result));

						if (component.AudioSourceData.PlaylistCopy.size() > 0)
						{
							uint32_t index = 0;

							for (uint32_t j = 0; j < component.AudioSourceData.PlaylistCopy.size(); j++)
							{
								if (component.AudioSourceData.PlaylistCopy[j] == component.AudioSourceData.Playlist[i])
									index = j;
							}

							component.AudioSourceData.PlaylistCopy.erase(component.AudioSourceData.PlaylistCopy.begin() + index);
							component.AudioSourceData.PlaylistCopy.shrink_to_fit();
						}

						component.RemoveAudioSource(component.AudioSourceData.Playlist[i]);

						if (i == 0)
						{
							if (component.AudioSourceData.Playlist.size() > 0)
							{
								if (component.Audio != component.AudioSourceData.Playlist[0] && component.AudioSourceData.Playlist[0] != 0)
								{
									component.Audio = component.AudioSourceData.Playlist[0];
								}
							}

							if (component.AudioSourceData.Playlist.size() == 0)
								component.AudioSourceData.UsePlaylist = false;
						}
					}

					ImGui::PopStyleVar();
					ImGui::PopItemWidth();
					ImGui::NextColumn();
					ImGui::PopID();
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<AudioListenerComponent>("Audio Listener", entity, [](AudioListenerComponent& component)
		{
			UI::BeginPropertyGrid();
			auto& config = component.Config;

			bool active = component.Active;
			if (UI::Property("Active", active))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeBoolCommand(component.Active, active));
			}

			float innerAngle = rtmcpp::Degrees(config.ConeInnerAngle);
			if (UI::Property("Cone Inner Angle", innerAngle) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				float result = rtmcpp::Radians(innerAngle);
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.ConeOuterGain, result));
				config.ConeInnerAngle = result;
			}
			
			float outerAngle = rtmcpp::Degrees(config.ConeOuterAngle);
			if (UI::Property("Cone Outer Angle", outerAngle) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				float result = rtmcpp::Radians(outerAngle);
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.ConeOuterGain, result));
				config.ConeOuterAngle = result;
			}
			
			float outerGain = config.ConeOuterGain;
			if (UI::Property("Cone Outer Gain", outerGain) && (SceneHierarchyPanel::PressedEnter || SceneHierarchyPanel::MouseDragging))
			{
				CommandHistory::AddAndRemoveCommand(new ChangeFloatCommand(config.ConeOuterGain, outerGain));
			}
			
			UI::EndPropertyGrid();
		});
	}

}
