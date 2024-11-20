#include "nzpch.hpp"
#include "ImGuiWidgets.hpp"

#include "Nutcrackz/Asset/AssetManager.hpp"

#include "Nutcrackz/Scene/Components.hpp"
#include "Nutcrackz/Scripting/ScriptEngine.hpp"

#include "Nutcrackz/ImGui/ScopedVar.hpp"

namespace Nutcrackz::UI {

	bool BeginPopup(const char* str_id, ImGuiWindowFlags flags)
	{
		bool opened = false;
		if (ImGui::BeginPopup(str_id, flags))
		{
			opened = true;
			// Fill background with nice gradient
			const float padding = ImGui::GetStyle().WindowBorderSize;
			const ImRect windowRect = UI::RectExpanded(ImGui::GetCurrentWindow()->Rect(), -padding, -padding);
			ImGui::PushClipRect(windowRect.Min, windowRect.Max, false);
			const ImColor col1 = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);// Colors::Theme::backgroundPopup;
			const ImColor col2 = UI::ColourWithMultipliedValue(col1, 0.8f);
			ImGui::GetWindowDrawList()->AddRectFilledMultiColor(windowRect.Min, windowRect.Max, col1, col1, col2, col2);
			ImGui::GetWindowDrawList()->AddRect(windowRect.Min, windowRect.Max, UI::ColourWithMultipliedValue(col1, 1.1f));
			ImGui::PopClipRect();

			// Popped in EndPopup()
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 80));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));
		}

		return opened;
	}

	void EndPopup()
	{
		ImGui::PopStyleVar(); // WindowPadding;
		ImGui::PopStyleColor(); // HeaderHovered;
		ImGui::EndPopup();
	}

}