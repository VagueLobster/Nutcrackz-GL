#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <vector>
#include <string>

namespace Nutcrackz::UI {

	inline bool InputText(const char* label, std::string* value, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL)
	{
		bool changed = ImGui::InputText(label, value, flags, callback, user_data);
		return changed;
	}

}