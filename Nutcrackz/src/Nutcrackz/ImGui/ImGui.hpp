#pragma once

#include "Nutcrackz/Renderer/Texture.hpp"

#include <Coral/String.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include "ImGuiUtilities.hpp"

#include <string>
#include <vector>

#include <Windows.h>
#include <codecvt>

//#include <glm/glm.hpp>
//#include <glm/gtc/type_ptr.hpp>
#include "rtmcpp/Common.hpp"
//#include "rtmcpp/Transforms.hpp"

// The Microsoft C++ compiler is non-compliant with the C++ standard and needs
// the following definition to disable a security warning on std::strncpy().
//#ifdef _MSVC_LANG
//#define _CRT_SECURE_NO_WARNINGS
//#endif

// To experiment with editor theme live you can change these constexpr into static
// members of a static "Theme" class and add a quick ImGui window to adjust the colour values
namespace Colors::Theme
{
	constexpr auto accent = IM_COL32(236, 158, 36, 255);
	constexpr auto highlight = IM_COL32(39, 185, 242, 255);
	constexpr auto niceBlue = IM_COL32(83, 232, 254, 255);
	constexpr auto compliment = IM_COL32(78, 151, 166, 255);
	constexpr auto background = IM_COL32(36, 36, 36, 255);
	constexpr auto backgroundDark = IM_COL32(26, 26, 26, 255);
	constexpr auto titlebar = IM_COL32(21, 21, 21, 255);
	constexpr auto propertyField = IM_COL32(15, 15, 15, 255);
	constexpr auto text = IM_COL32(192, 192, 192, 255);
	constexpr auto textBrighter = IM_COL32(210, 210, 210, 255);
	constexpr auto textDarker = IM_COL32(128, 128, 128, 255);
	constexpr auto textError = IM_COL32(230, 51, 51, 255);
	constexpr auto muted = IM_COL32(77, 77, 77, 255);
	constexpr auto groupHeader = IM_COL32(47, 47, 47, 255);
	//constexpr auto selection				= IM_COL32(191, 177, 155, 255);
	//constexpr auto selectionMuted			= IM_COL32(59, 57, 45, 255);
	constexpr auto selection = IM_COL32(237, 192, 119, 255);
	constexpr auto selectionMuted = IM_COL32(237, 201, 142, 23);

	//constexpr auto backgroundPopup			= IM_COL32(63, 73, 77, 255); // in between
	//constexpr auto backgroundPopup			= IM_COL32(63, 77, 76, 255); // most green
	constexpr auto backgroundPopup = IM_COL32(50, 50, 50, 255); // most blue
}

namespace Nutcrackz::UI {

	static int s_UIContextID = 0;
	static uint32_t s_Counter = 0;
	static uint32_t s_OldCounter = 0;
	static char s_IDBuffer[16];
	static std::string s_Label = "";
	static char* s_MultilineBuffer = nullptr;

	class ScopedStyle
	{
	public:
		ScopedStyle(const ScopedStyle&) = delete;
		ScopedStyle operator=(const ScopedStyle&) = delete;
		template<typename T>
		ScopedStyle(ImGuiStyleVar styleVar, T value) { ImGui::PushStyleVar(styleVar, value); }
		~ScopedStyle() { ImGui::PopStyleVar(); }
	};

	class ScopedColour
	{
	public:
		ScopedColour(const ScopedColour&) = delete;
		ScopedColour operator=(const ScopedColour&) = delete;
		template<typename T>
		ScopedColour(ImGuiCol colourId, T colour) { ImGui::PushStyleColor(colourId, ImColor(colour).Value); }
		~ScopedColour() { ImGui::PopStyleColor(); }
	};

	static const char* GenerateID()
	{
		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);

		return &s_IDBuffer[0];
	}

	static void PushID()
	{
		ImGui::PushID(s_UIContextID++);
		s_Counter = 0;
	}

	static void PopID()
	{
		ImGui::PopID();
		--s_UIContextID;
	}

	static void BeginProperty(const char* label, const char* tooltip, bool rightAlignNextColumn = false)
	{
		PushID();

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGui::PushID(label);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y * 0.5f);
		ImGui::TextUnformatted(label);
		if (tooltip && ImGui::IsItemHovered(/*ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay*/))
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(tooltip);
			ImGui::EndTooltip();
		}

		ImGui::TableNextColumn();

		if (rightAlignNextColumn)
			ImGui::SetNextItemWidth(-FLT_MIN);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		++s_Counter;
		std::string buffer = fmt::format("##{}", s_Counter);
		std::memcpy(&s_IDBuffer, buffer.data(), 16);
	}

	static void EndProperty()
	{
		ImGui::PopID();
		PopID();
	}

	static bool IsMouseEnabled()
	{
		return ImGui::GetIO().ConfigFlags & ~ImGuiConfigFlags_NoMouse;
	}

	static void SetMouseEnabled(const bool enable)
	{
		if (enable)
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		else
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
	}

	static void BeginPropertyGrid()
	{
		PushID();
		ImGui::Columns(2);
	}

	//static bool InputProperty(const char* label, std::string& value, const std::string& valueHint, bool hint = false, bool error = false)
	static bool InputProperty(const char* label, Coral::String& value, const std::string& valueHint, bool hint = false, bool error = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char buffer[256];
		std::string changedValue = value;

		strcpy_s(buffer, changedValue.c_str());

		s_Label = "##" + std::to_string(s_Counter++);

		if (error)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
		if (hint)
		{
			if (ImGui::InputTextWithHint(s_Label.c_str(), valueHint.c_str(), buffer, 256))
			{
				changedValue = buffer;
				value = Coral::String::New(changedValue);
				modified = true;
			}
		}
		else
		{
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
			ImGuiInputTextCallback callback = NULL;
			void* user_data = NULL;
			//modified = InputText(s_Label.c_str(), &value, flags, callback, user_data);
			if (InputText(s_Label.c_str(), &changedValue, flags, callback, user_data))
			{
				value = Coral::String::New(changedValue);
				modified = true;
			}
			//if (ImGui::InputText(s_Label.c_str(), buffer, 256))
			//{
			//	value = buffer;
			//	modified = true;
			//}
		}
		if (error)
			ImGui::PopStyleColor();
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, std::string& value, bool error, bool readOnly)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char buffer[256];
		strcpy_s(buffer, value.c_str());

		s_Label = "##" + std::to_string(s_Counter++);

		ImGuiInputTextFlags flags = readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;

		if (error)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
		if (ImGui::InputText(s_Label.c_str(), buffer, 256, flags))
		{
			if (!readOnly)
			{
				value = buffer;
				modified = true;
			}
		}
		if (error)
			ImGui::PopStyleColor();
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyWithButton(const char* label, const char* buttonLabel, std::string& value, bool error, bool readOnly, rtmcpp::Vec2 size = rtmcpp::Vec2(0.0f, 0.0f))
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		//ImGui::PushItemWidth(-1);

		char buffer[256];
		strcpy_s(buffer, value.c_str());

		s_Label = "##" + std::to_string(s_Counter++);

		ImGuiInputTextFlags flags = readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;

		if (error)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
		if (ImGui::InputText(s_Label.c_str(), buffer, 256, flags))
		{
			value = buffer;

			if (!readOnly)
				modified = true;
		}
		if (error)
			ImGui::PopStyleColor();
		ImGui::SameLine();
		if (ImGui::Button(buttonLabel, ImVec2(size.X, size.Y)))
		{
			modified = true;
		}
		//ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool MultiLineProperty(const char* label, std::string& value, bool multiLine = true, bool error = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char buffer[256];
		strcpy_s(buffer, value.c_str());

		s_Label = "##" + std::to_string(s_Counter++);

		if (error)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
		if (multiLine)
		{
			if (ImGui::InputTextMultiline(s_Label.c_str(), buffer, 256, ImVec2(0, 0), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				value = buffer;
				modified = true;
			}
		}
		else
		{
			if (ImGui::InputText(s_Label.c_str(), buffer, 256, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				value = buffer;
				modified = true;
			}
		}
		if (error)
			ImGui::PopStyleColor();
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, const char* value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char buffer[256];
		strcpy_s(buffer, value);

		s_Label = "##" + std::to_string(s_Counter++);

		//ImGui::InputText(s_Label.c_str(), (char*)value, 256, ImGuiInputTextFlags_ReadOnly);
		if (ImGui::InputText(s_Label.c_str(), buffer, 256, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			value = buffer;
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, bool& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::Checkbox(s_Label.c_str(), &value))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool ButtonProperty(const char* label, const char* buttonLabel, rtmcpp::Vec2 size = rtmcpp::Vec2(0.0f, 0.0f))
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		if (ImGui::Button(buttonLabel, ImVec2(size.X, size.Y)))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, int& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragInt(s_Label.c_str(), &value))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, int8_t& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		int valueToChange = value;

		//if (ImGui::DragInt(s_Label.c_str(), reinterpret_cast<int*>(&value), 1.0f))
		if (ImGui::DragInt(s_Label.c_str(), &valueToChange))
		{
			value = (int8_t)valueToChange;
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, int16_t& value, bool isBoolean = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		int valueToChange = value;

		if (!isBoolean)
		{
			//if (ImGui::DragInt(s_Label.c_str(), reinterpret_cast<int*>(&value), 1.0f))
			if (ImGui::DragInt(s_Label.c_str(), &valueToChange))
			{
				value = (int16_t)valueToChange;
				modified = true;
			}
		}
		else
		{
			bool boolValue = (bool)value;
			if (ImGui::Checkbox(s_Label.c_str(), &boolValue))
				modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, int& value, const char** dropdownStrings, int count, const char* tooltip = "")
	{
		//BeginProperty(label, tooltip);

		bool modified = false;
		const char* current = dropdownStrings[value];

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		if (ImGui::BeginCombo(s_IDBuffer, current))
		{
			for (int i = 0; i < count; i++)
			{
				bool isSelected = current == dropdownStrings[i];
				if (ImGui::Selectable(dropdownStrings[i], isSelected))
				{
					current = dropdownStrings[i];
					value = i;
					modified = true;
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		//EndProperty();

		return modified;
	}

	static void TextProperty(const char* label, const std::string& text)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		ImGui::Text(text.c_str());

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	static bool Property(const char* label, int64_t& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragInt(s_Label.c_str(), reinterpret_cast<int*>(&value)))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, int64_t& value, int64_t delta, int min, int max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragInt(s_Label.c_str(), reinterpret_cast<int*>(&value), (float)delta, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, uint64_t& value, uint64_t delta, uint64_t min, uint64_t max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragScalar(s_Label.c_str(), ImGuiDataType_U64, reinterpret_cast<int*>(&value), (float)delta, reinterpret_cast<int*>(&min), reinterpret_cast<int*>(&max)))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool SliderProperty(const char* label, uint64_t& value, uint64_t min, uint64_t max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::SliderScalar(s_Label.c_str(), ImGuiDataType_U64, reinterpret_cast<int*>(&value), reinterpret_cast<int*>(&min), reinterpret_cast<int*>(&max)))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, uint8_t& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		int valueToChange = value;

		//if (ImGui::DragInt(s_Label.c_str(), reinterpret_cast<int*>(&value), 1.0f))
		if (ImGui::DragScalar(s_Label.c_str(), ImGuiDataType_U8, &value))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, uint16_t& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		int valueToChange = value;

		//if (ImGui::DragInt(s_Label.c_str(), reinterpret_cast<int*>(&value), 1.0f))
		if (ImGui::DragScalar(s_Label.c_str(), ImGuiDataType_U16, &value))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyScalar(const char* label, uint32_t& value, float speed = 1.0f, uint32_t* min = 0, uint32_t* max = 0)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		//if (ImGui::DragInt(s_Label.c_str(), reinterpret_cast<int*>(&value), 1.0f))
		//if (ImGui::DragInt(s_Label.c_str(), &valueToChange, 1.0f, 0))
		if (ImGui::DragScalar(s_Label.c_str(), ImGuiDataType_U32, &value, speed, &min, &max))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, uint64_t& value, bool readOnly = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragScalar(s_Label.c_str(), ImGuiDataType_U64, &value))
		{
			if (!readOnly)
				modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, UUID& value, bool readOnly = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragScalar(s_Label.c_str(), ImGuiDataType_U64, &value))
		{
			if (!readOnly)
				modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, long long& value, float delta, bool readOnly = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		//if (ImGui::DragScalar(s_Label.c_str(), ImGuiDataType_U64, &value))
		if (ImGui::DragInt(s_Label.c_str(), reinterpret_cast<int*>(&value), delta, 0, 0, "%32d"))
		{
			if (!readOnly)
				modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, char* value, size_t length)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::InputText(s_Label.c_str(), value, length))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, int& value, int delta, int min, int max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragInt(s_Label.c_str(), &value, (float)delta, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, int& value, int delta, int min, int max, bool disabled)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (disabled)
			ImGui::BeginDisabled(true);

		if (ImGui::DragInt(s_Label.c_str(), &value, (float)delta, min, max))
			modified = true;

		if (disabled)
			ImGui::EndDisabled();

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool UintDragProperty(const char* label, uint32_t& value, int delta, int min, int max, bool disabled)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (disabled)
			ImGui::BeginDisabled(true);

		if (ImGui::DragScalar(s_Label.c_str(), ImGuiDataType_U32, &value, (float)delta, &min, &max, "%d", ImGuiSliderFlags_AlwaysClamp))
			modified = true;

		if (disabled)
			ImGui::EndDisabled();

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyU32(const char* label, int& value, int delta, int min, int max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragScalar(s_Label.c_str(), ImGuiDataType_U32, &value, (float)delta, &min, &max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertySlider(const char* label, int& value, int min, int max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::SliderInt(s_Label.c_str(), &value, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertySlider(const char* label, float& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::SliderFloat(s_Label.c_str(), &value, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertySlider(const char* label, rtmcpp::Vec2& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::SliderFloat2(s_Label.c_str(), &value.X, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertySlider(const char* label, rtmcpp::Vec3& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::SliderFloat3(s_Label.c_str(), &value.X, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertySlider(const char* label, rtmcpp::Vec4& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::SliderFloat4(s_Label.c_str(), &value.X, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDrag(const char* label, int& value, int delta, int min, int max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragInt(s_Label.c_str(), (int*)&value, (float)delta, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDrag(const char* label, rtmcpp::Vec2& value, int delta, int min, int max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragInt2(s_Label.c_str(), (int*)&value.X, (float)delta, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyCustomDrag(const char* label, rtmcpp::Vec2& value, int delta, int min, int max, const char* labelX = "", const char* labelY = "", float sizeX = -1.0f, float sizeY = -1.0f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		ImGui::Text(labelX);
		ImGui::SameLine();

		ImGui::PushID(0);
		ImGui::PushItemWidth(sizeX);
		if (ImGui::DragInt(s_Label.c_str(), (int*)&value.X, (float)delta, min, max))
			modified = true;
		ImGui::PopID();

		ImGui::SameLine();
		ImGui::Text(labelY);
		ImGui::SameLine();

		ImGui::PushID(1);
		ImGui::PushItemWidth(sizeY);
		if (ImGui::DragInt(s_Label.c_str(), (int*)&value.Y, (float)delta, min, max))
			modified = true;
		ImGui::PopID();

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, rtmcpp::Vec2& value, int delta = 1)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		//int inValue[2] = { static_cast<int>(value.X), static_cast<int>(value.Y) };
		//if (ImGui::DragInt2(s_Label.c_str(), glm::value_ptr(value), (float)delta))
		if (ImGui::DragInt2(s_Label.c_str(), (int*)&value.X, (float)delta))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, const char* buttonLabel, rtmcpp::Vec2& value, bool& buttonValue, ImVec2 size = ImVec2(0.0f, 0.0f), int delta = 1)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		//int inValue[2] = { static_cast<int>(value.X), static_cast<int>(value.Y) };
		//if (ImGui::DragInt2(s_Label.c_str(), glm::value_ptr(value), (float)delta))
		if (ImGui::DragInt2(s_Label.c_str(), (int*)&value.X, (float)delta))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDrag(const char* label, float& value, float delta = 0.1f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat(s_Label.c_str(), &value, delta, 0.0f, 0.0f))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDrag(const char* label, float& value, float delta, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat(s_Label.c_str(), &value, delta, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDrag(const char* label, rtmcpp::Vec2& value, float delta = 0.1f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat2(s_Label.c_str(), &value.X, delta, 0.0f, 0.0f))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDrag(const char* label, rtmcpp::Vec2& value, float delta, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat2(s_Label.c_str(), &value.X, delta, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDrag(const char* label, rtmcpp::Vec3& value, float delta = 0.1f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat3(s_Label.c_str(), &value.X, delta, 0.0f, 0.0f))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDrag(const char* label, rtmcpp::Vec4& value, float delta = 0.1f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat4(s_Label.c_str(), &value.X, delta, 0.0f, 0.0f))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDrag(const char* label, rtmcpp::Vec3& value, float delta, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat3(s_Label.c_str(), &value.X, delta, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDrag(const char* label, rtmcpp::Vec4& value, float delta, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat4(s_Label.c_str(), &value.X, delta, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, float& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f, bool readOnly = false, const char* format = "%.3f")
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (!readOnly)
		{
			if (ImGui::DragFloat(s_Label.c_str(), &value, delta, min, max, format))
				modified = true;
		}
		else
		{
			ImGui::InputFloat(s_Label.c_str(), &value, 0.0F, 0.0F, format, ImGuiInputTextFlags_ReadOnly);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, double& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f, bool readOnly = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (!readOnly)
		{
			if (ImGui::DragFloat(s_Label.c_str(), reinterpret_cast<float*>(&value), delta, min, max, "%.6f"))
				modified = true;
		}
		else
		{
			ImGui::InputFloat(s_Label.c_str(), reinterpret_cast<float*>(&value), 0.0F, 0.0F, "%.6f", ImGuiInputTextFlags_ReadOnly);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, rtmcpp::Vec2& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat2(s_Label.c_str(), &value.X, delta, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyColor(const char* label, rtmcpp::Vec3& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::ColorEdit3(s_Label.c_str(), &value.X))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyColor(const char* label, rtmcpp::Vec4& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::ColorEdit4(s_Label.c_str(), &value.X))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, rtmcpp::Vec3& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat3(s_Label.c_str(), &value.X, delta, min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, rtmcpp::Vec4& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::DragFloat4(s_Label.c_str(), &value.X, delta, min, max))
			modified = true;
		
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyDropdown(const char* label, const char** options, int32_t optionCount, int32_t* selected)
	{
		const char* current = options[*selected];
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		bool changed = false;

		std::string id = "##" + std::string(label);
		if (ImGui::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; i++)
			{
				bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i], is_selected))
				{
					current = options[i];
					*selected = i;
					changed = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return changed;
	}

	static bool PropertyDropdown(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected)
	{
		const char* current = options[*selected].c_str();
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		bool changed = false;

		std::string id = "##" + std::string(label);
		if (ImGui::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; i++)
			{
				bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i].c_str(), is_selected))
				{
					current = options[i].c_str();
					*selected = i;
					changed = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return changed;
	}

	static bool PropertyButtonImage(const char* label, uint32_t value, const ImVec2& size, const ImVec2& uv0 = ImVec2{ 0, 1 }, const ImVec2& uv1 = ImVec2{ 1, 0 })
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_Label = "##" + std::to_string(s_Counter++);
		
		if (ImGui::ImageButton((ImTextureID)(uint64_t)value, size, uv0, uv1, 0))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyMultiline(const char* label, std::string& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		if (!s_MultilineBuffer)
		{
			s_MultilineBuffer = new char[1024 * 1024]; // 1KB
			memset(s_MultilineBuffer, 0, 1024 * 1024);
		}

		/*if (std::find(value.begin(), value.end(), '\t') != value.end())
		{

		}*/

		//std::string endValue = Utils::ReplaceAll(value, std::string("\t"), std::string("    "));

		strcpy_s(s_MultilineBuffer, 1024 * 1024, value.c_str());

		if (ImGui::InputTextMultiline(GenerateID(), s_MultilineBuffer, 1024 * 1024, ImVec2(0, 0)))
		{
			value = s_MultilineBuffer;
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static void EndPropertyGrid()
	{
		ImGui::Columns(1);
		PopID();
	}

	static bool BeginTreeNode(const char* name, bool defaultOpen = true)
	{
		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
		if (defaultOpen)
			treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

		return ImGui::TreeNodeEx(name, treeNodeFlags);
	}

	static void EndTreeNode()
	{
		ImGui::TreePop();
	}

	static int s_CheckboxCount = 0;

	static void BeginCheckboxGroup(const char* label)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
	}

	static bool PropertyCheckboxGroup(const char* label, bool& value)
	{
		bool modified = false;

		if (++s_CheckboxCount > 1)
			ImGui::SameLine();

		ImGui::Text(label);
		ImGui::SameLine();

		s_Label = "##" + std::to_string(s_Counter++);

		if (ImGui::Checkbox(s_Label.c_str(), &value))
			modified = true;

		return modified;
	}

	static void EndCheckboxGroup()
	{
		ImGui::PopItemWidth();
		ImGui::NextColumn();
		s_CheckboxCount = 0;
	}

	// Rectangle
	static inline ImRect GetItemRect()
	{
		return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	static inline ImRect RectExpanded(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x -= x;
		result.Min.y -= y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	static inline ImRect RectOffset(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x += x;
		result.Min.y += y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	static inline ImRect RectOffset(const ImRect& rect, ImVec2 xy)
	{
		return RectOffset(rect, xy.x, xy.y);
	}

	static void DrawItemActivityOutline(float rounding = 0.0f, bool drawWhenInactive = false, ImColor colourWhenActive = ImColor(80, 80, 80))
	{
		auto* drawList = ImGui::GetWindowDrawList();
		const ImRect rect = RectExpanded(GetItemRect(), 1.0f, 1.0f);
		if (ImGui::IsItemHovered() && !ImGui::IsItemActive())
		{
			drawList->AddRect(rect.Min, rect.Max,
				ImColor(60, 60, 60), rounding, 0, 1.5f);
		}
		if (ImGui::IsItemActive())
		{
			drawList->AddRect(rect.Min, rect.Max,
				colourWhenActive, rounding, 0, 1.0f);
		}
		else if (!ImGui::IsItemHovered() && drawWhenInactive)
		{
			drawList->AddRect(rect.Min, rect.Max,
				ImColor(50, 50, 50), rounding, 0, 1.0f);
		}
	};

	// The delay won't work on texts, because the timer isn't tracked for them.
	static bool IsItemHovered(float delayInSeconds = 0.1f, ImGuiHoveredFlags flags = 0)
	{
		return ImGui::IsItemHovered() && GImGui->HoveredIdTimer > delayInSeconds; /*HoveredIdNotActiveTimer*/
	}

	static void SetTooltip(std::string_view text, float delayInSeconds = 0.1f, bool allowWhenDisabled = true, ImVec2 padding = ImVec2(5, 5))
	{
		if (IsItemHovered(delayInSeconds, allowWhenDisabled ? ImGuiHoveredFlags_AllowWhenDisabled : 0))
		{
			UI::ScopedStyle tooltipPadding(ImGuiStyleVar_WindowPadding, padding);
			UI::ScopedColour textCol(ImGuiCol_Text, Colors::Theme::textBrighter);
			ImGui::SetTooltip(text.data());
		}
	}

	// Cursor
	static void ShiftCursorX(float distance)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + distance);
	}

	static void ShiftCursorY(float distance)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + distance);
	}

	static void ShiftCursor(float x, float y)
	{
		const ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(cursor.x + x, cursor.y + y));
	}

	// Button Image
	static void DrawButtonImage(const RefPtr<Texture2D>& imageNormal, const RefPtr<Texture2D>& imageHovered, const RefPtr<Texture2D>& imagePressed,
		ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax)
	{
		auto* drawList = ImGui::GetWindowDrawList();
		if (ImGui::IsItemActive())
			drawList->AddImage((ImTextureID)(uint64_t)imagePressed->GetRendererID(), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintPressed);
		else if (ImGui::IsItemHovered())
			drawList->AddImage((ImTextureID)(uint64_t)imageHovered->GetRendererID(), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintHovered);
		else
			drawList->AddImage((ImTextureID)(uint64_t)imageNormal->GetRendererID(), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintNormal);
	};

	static void DrawButtonImage(const RefPtr<Texture2D>& imageNormal, const RefPtr<Texture2D>& imageHovered, const RefPtr<Texture2D>& imagePressed,
		ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle)
	{
		DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
	};

	static void DrawButtonImage(const RefPtr<Texture2D>& image, ImU32 tintNormal,
		ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax)
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectMin, rectMax);
	};

	static void DrawButtonImage(const RefPtr<Texture2D>& image, ImU32 tintNormal,
		ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle)
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
	};

	static void DrawButtonImage(const RefPtr<Texture2D>& imageNormal, const RefPtr<Texture2D>& imageHovered, const RefPtr<Texture2D>& imagePressed,
		ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	};

	static void DrawButtonImage(const RefPtr<Texture2D>& image,
		ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	};

	static bool BeginMenubar(const ImRect& barRectangle)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		/*if (!(window->Flags & ImGuiWindowFlags_MenuBar))
			return false;*/

		IM_ASSERT(!window->DC.MenuBarAppending);
		ImGui::BeginGroup(); // Backup position on layer 0 // FIXME: Misleading to use a group for that backup/restore
		ImGui::PushID("##menubar");

		const ImVec2 padding = window->WindowPadding;

		// We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
		// We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
		ImRect bar_rect = UI::RectOffset(barRectangle, 0.0f, padding.y);// window->MenuBarRect();
		ImRect clip_rect(IM_ROUND(ImMax(window->Pos.x, bar_rect.Min.x + window->WindowBorderSize + window->Pos.x - 10.0f)), IM_ROUND(bar_rect.Min.y + window->WindowBorderSize + window->Pos.y),
			IM_ROUND(ImMax(bar_rect.Min.x + window->Pos.x, bar_rect.Max.x - ImMax(window->WindowRounding, window->WindowBorderSize))), IM_ROUND(bar_rect.Max.y + window->Pos.y));

		clip_rect.ClipWith(window->OuterRectClipped);
		ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

		// We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar() would need something analogous here, maybe a BeginGroupEx() with flags).
		window->DC.CursorPos = window->DC.CursorMaxPos = ImVec2(bar_rect.Min.x + window->Pos.x, bar_rect.Min.y + window->Pos.y);
		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
		window->DC.MenuBarAppending = true;
		ImGui::AlignTextToFramePadding();
		return true;
	};

	static void EndMenubar()
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;
		ImGuiContext& g = *GImGui;

		// Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
		if (ImGui::NavMoveRequestButNoResultYet() && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) && (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
		{
			// Try to find out if the request is for one of our child menu
			ImGuiWindow* nav_earliest_child = g.NavWindow;
			while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
				nav_earliest_child = nav_earliest_child->ParentWindow;
			if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal && (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded) == 0)
			{
				// To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
				// This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth bothering)
				const ImGuiNavLayer layer = ImGuiNavLayer_Menu;
				IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer)); // Sanity check
				ImGui::FocusWindow(window);
				ImGui::SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
				g.NavDisableHighlight = true; // Hide highlight for the current frame so we don't see the intermediary selection.
				g.NavDisableMouseHover = g.NavMousePosDirty = true;
				ImGui::NavMoveRequestForward(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags, g.NavMoveScrollFlags); // Repeat
			}
		}

		IM_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
		// IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar); // NOTE(Yan): Needs to be commented out because Jay
		IM_ASSERT(window->DC.MenuBarAppending);
		ImGui::PopClipRect();
		ImGui::PopID();
		window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x; // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.
		g.GroupStack.back().EmitItem = false;
		ImGui::EndGroup(); // Restore position on layer 0
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
		window->DC.MenuBarAppending = false;
	};

	struct PropertyAssetReferenceSettings
	{
		bool AdvanceToNextColumn = true;
		bool NoItemSpacing = false; // After label
		float WidthOffset = 0.0f;
		bool AllowMemoryOnlyAssets = false;
		ImVec4 ButtonLabelColor = ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);
		ImVec4 ButtonLabelColorError = ImGui::ColorConvertU32ToFloat4(Colors::Theme::textError);
		bool ShowFullFilePath = false;
	};

}
