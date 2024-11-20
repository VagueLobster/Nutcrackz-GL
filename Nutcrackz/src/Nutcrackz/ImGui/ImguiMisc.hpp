#pragma once

#include "Nutcrackz/Core/Application.hpp"
#include "Nutcrackz/Scene/Components.hpp"
#include "Nutcrackz/Scene/Scene.hpp"
#include "Nutcrackz/Scene/Entity.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <combaseapi.h>
#include <shellapi.h>

namespace Nutcrackz {

	namespace UI {

#pragma region URL

		static bool s_IsHoveringURL = false;
		static int currentUrlID = 0;

		static ImVec4 ConvertColorFromByteToImVec4(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
		{
			return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
		}

		static void OpenURLInBrowser(const std::string& url)
		{
			CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

			SHELLEXECUTEINFOA sei = { sizeof sei };
			sei.lpVerb = "open";
			sei.lpFile = url.c_str();
			ShellExecuteExA(&sei);
		}

		static void AddUnderLine(ImColor color)
		{
			ImVec2 min = ImGui::GetItemRectMin();
			ImVec2 max = ImGui::GetItemRectMax();
			min.y = max.y;
			ImGui::GetWindowDrawList()->AddLine(min, max, color, 1.0f);
		}

		static void ButtonURL(const char* name, const std::string& URL, bool sameLineBefore, bool sameLineAfter)
		{
			if (sameLineBefore)
				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

			ImGui::Button(name);

			if (ImGui::IsItemHovered())
			{
				if (ImGui::IsMouseClicked(0))
				{
					std::string url = "microsoft-edge:" + URL;
					OpenURLInBrowser(url);
				}
			}

			if (sameLineAfter)
				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
		}

		static void TextURL(const char* name, const std::string& URL, bool showToolTip, bool sameLineBefore, bool sameLineAfter, bool& isHovering)
		{
			if (sameLineBefore)
				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

			if (isHovering)
				ImGui::PushStyleColor(ImGuiCol_Text, ConvertColorFromByteToImVec4(93, 197, 5, 255));
			else
				ImGui::PushStyleColor(ImGuiCol_Text, ConvertColorFromByteToImVec4(73, 159, 5, 255));

			ImGui::Text(name);

			ImGui::PopStyleColor();

			if (ImGui::IsItemHovered())
			{
				if (ImGui::IsMouseClicked(0))
				{
					std::string url = "microsoft-edge:" + URL;
					OpenURLInBrowser(url);
				}

				ImColor color = ConvertColorFromByteToImVec4(93, 197, 5, 255);
				AddUnderLine(color);

				if (showToolTip)
					ImGui::SetTooltip("Opens in Microsoft Edge");

				isHovering = true;
			}
			else
			{
				ImColor color = ConvertColorFromByteToImVec4(73, 159, 5, 255);
			
				isHovering = false;
			}

			if (sameLineAfter)
				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
		}

		static void SetCursorForURL(bool shouldChange)
		{
			if (shouldChange)
				Application::Get().GetWindow().SetCursorHand();
			else
				Application::Get().GetWindow().SetCursorNormal();
		}

#pragma endregion

	}

}