#pragma once

#include "Nutcrackz/UndoRedo/CommandHistory.hpp"
#include "Nutcrackz/UndoRedo/ChangeBoolCommand.hpp"
#include "Nutcrackz/UndoRedo/ChangeInt32Command.hpp"
#include "Nutcrackz/UndoRedo/ChangeUint32Command.hpp"
#include "Nutcrackz/UndoRedo/ChangeStringCommand.hpp"
#include "Nutcrackz/UndoRedo/ChangeFloatCommand.hpp"
#include "Nutcrackz/UndoRedo/ChangeVec2Command.hpp"
#include "Nutcrackz/UndoRedo/ChangeVec3Command.hpp"
#include "Nutcrackz/UndoRedo/ChangeVec4Command.hpp"

#include "Imgui.hpp"
#include "Nutcrackz/ImGUI/ImGuiWidgets.hpp"
#include "Nutcrackz/Scripting/ScriptEngine.hpp"

#include "rtmcpp/Common.hpp"

namespace Nutcrackz::UI {

	static char s_LabelIDBuffer[1024 + 1];

	// TODO: move most of the functions in this header into the Draw namespace
	namespace Draw {
		//=========================================================================================
		/// Lines
		inline void Underline(bool fullWidth = false, float offsetX = 0.0f, float offsetY = -1.0f)
		{
			if (fullWidth)
			{
				if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
					ImGui::PushColumnsBackground();
				else if (ImGui::GetCurrentTable() != nullptr)
					ImGui::TablePushBackgroundChannel();
			}

			const float width = fullWidth ? ImGui::GetWindowWidth() : ImGui::GetContentRegionAvail().x;
			const ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddLine(ImVec2(cursor.x + offsetX, cursor.y + offsetY),
				ImVec2(cursor.x + width, cursor.y + offsetY),
				Colors::Theme::backgroundDark, 1.0f);

			if (fullWidth)
			{
				if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
					ImGui::PopColumnsBackground();
				else if (ImGui::GetCurrentTable() != nullptr)
					ImGui::TablePopBackgroundChannel();
			}
		}
	}

	const char* GenerateLabelID(std::string_view label);

	static bool DrawFieldValue(std::string_view fieldName, FieldStorage& storage)
	{
		ImGui::PushID(fieldName.data());

		bool result = false;

		switch (storage.GetType())
		{
		case DataType::Bool:
		{
			bool value = storage.GetValue<bool>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::SByte:
		{
			int8_t value = storage.GetValue<int8_t>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::Short:
		{
			int16_t value = storage.GetValue<int16_t>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::Int:
		{
			int32_t value = storage.GetValue<int32_t>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::Long:
		{
			int64_t value = storage.GetValue<int64_t>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::Byte:
		{
			uint8_t value = storage.GetValue<uint8_t>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::UShort:
		{
			uint16_t value = storage.GetValue<uint16_t>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::UInt:
		{
			uint32_t value = storage.GetValue<uint32_t>();
			if (PropertyScalar(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::ULong:
		{
			uint64_t value = storage.GetValue<uint64_t>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::Float:
		{
			float value = storage.GetValue<float>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::Double:
		{
			double value = storage.GetValue<double>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::String:
		{
			Coral::String value = storage.GetValue<Coral::String>();
			std::string hint = "";
			if (InputProperty(fieldName.data(), value, hint))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::AssetHandle:
		{
			uint64_t value = storage.GetValue<uint64_t>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;

					if (AssetManager::IsAssetHandleValid(handle))
					{
						value = handle;
						storage.SetValue(value);
					}
					else
					{
						NZ_CORE_WARN("Not a valid asset handle!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			break;
		}
		//case DataType::Entity:
		//{
		//	uint64_t value = storage.GetValue<uint64_t>();
		//	if (Property(fieldName.data(), value))
		//	{
		//		storage.SetValue(value);
		//		result = true;
		//	}
		//
		//	if (ImGui::BeginDragDropTarget())
		//	{
		//		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		//		{
		//			AssetHandle handle = *(AssetHandle*)payload->Data;
		//
		//			if (AssetManager::IsAssetHandleValid(handle))
		//			{
		//				value = handle;
		//				storage.SetValue(value);
		//			}
		//			else
		//			{
		//				NZ_CORE_WARN("Not a valid asset handle!");
		//			}
		//		}
		//		ImGui::EndDragDropTarget();
		//	}
		//
		//	break;
		//}
		case DataType::Bool32:
		{
			bool value = (bool)storage.GetValue<uint32_t>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue((uint32_t)value);
				result = true;
			}
			break;
		}
		case DataType::Vector2:
		{
			rtmcpp::Vec2 value = storage.GetValue<rtmcpp::Vec2>();
			if (Property(fieldName.data(), value, 0.01f))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::Vector3:
		{
			rtmcpp::Vec3 value = storage.GetValue<rtmcpp::Vec3>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		case DataType::Vector4:
		{
			rtmcpp::Vec4 value = storage.GetValue<rtmcpp::Vec4>();
			if (Property(fieldName.data(), value))
			{
				storage.SetValue(value);
				result = true;
			}
			break;
		}
		/*case DataType::Prefab:
		{
			AssetHandle handle = storage->GetValue<AssetHandle>();
			if (PropertyAssetReference<Prefab>(fieldName.c_str(), handle))
			{
				storage->SetValue(handle);
				result = true;
			}
			break;
		}
		case DataType::Entity:
		{
			UUID uuid = storage->GetValue<UUID>();
			if (PropertyEntityReference(fieldName.c_str(), uuid, sceneContext))
			{
				storage->SetValue(uuid);
				result = true;
			}
			break;
		}
		case DataType::Texture2D:
		{
			AssetHandle handle = storage->GetValue<AssetHandle>();
			if (PropertyAssetReference<Texture2D>(fieldName.c_str(), handle))
				storage->SetValue(handle);
			break;
		}
		case DataType::Scene:
		{
			AssetHandle handle = storage->GetValue<AssetHandle>();
			if (PropertyAssetReference<Scene>(fieldName.c_str(), handle))
				storage->SetValue(handle);
			break;
		}*/
		}

		ImGui::PopID();

		return result;
	}

}
