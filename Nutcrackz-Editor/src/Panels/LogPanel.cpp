#include "LogPanel.hpp"

#include <imgui/imgui_internal.h>

namespace Nutcrackz {

	RefPtr<LogPanel> LogPanel::s_Console = RefPtr<LogPanel>::Create();

	void LogPanel::OnImGuiRender()
	{
		ImGuiWindowClass imguiWindow;
		imguiWindow.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoCloseButton;
		imguiWindow.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoWindowMenuButton;

		if (ShowLogPanel)
		{
			ImGui::SetNextWindowClass(&imguiWindow);
			ImGui::Begin("Log", &ShowLogPanel);


			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 4.0f, ImGui::GetCursorPosY() + 4.0f));
			ImGui::PushStyleColor(0, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
			ImGui::Checkbox("Info", &m_ShowInfoMsgs);
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::PushStyleColor(0, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
			ImGui::Checkbox("Trace", &m_ShowTraceMsgs);
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::PushStyleColor(0, ImVec4(0.7f, 0.7f, 0.0f, 1.0f));
			ImGui::Checkbox("Warning", &m_ShowWarningMsgs);
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::PushStyleColor(0, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
			ImGui::Checkbox("Error", &m_ShowErrorMsgs);
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::PushStyleColor(0, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
			ImGui::Checkbox("Critical", &m_ShowCriticalMsgs);
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::Checkbox("Auto Scroll", &m_ScrollLock);
			ImGui::SameLine();
			if (ConsoleLog::SizeOfMessages() > 19)
			{
				ImGui::Checkbox("Auto Resize", &m_AutoClear);
			}
			else
			{
				ImGui::BeginDisabled();
				ImGui::Checkbox("Auto Resize", &m_AutoClear);
				ImGui::EndDisabled();
			}

			ImGui::SameLine();

			if (ConsoleLog::SizeOfMessages() > 0)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });
				if (ImGui::Button("Clear Log"))
				{
					Clear();
				}
				ImGui::PopStyleVar();
			}
			else
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8, 8 });
				ImGui::BeginDisabled();
				ImGui::Button("Clear Log");
				ImGui::EndDisabled();
				ImGui::PopStyleVar();
			}

			ImGuiIO& io = ImGui::GetIO();
			auto consolasFont = io.Fonts->Fonts[1];

			if (m_AutoClear)
				AutoClear();

			ImGui::BeginChild("Log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
			for (auto itr = ConsoleLog::s_Messages.begin(); itr != ConsoleLog::s_Messages.end(); ++itr)
			{
				switch (itr->first)
				{
				case Severity::Trace:
				{
					if (m_ShowTraceMsgs)
					{
						ImGui::PushFont(consolasFont);
						ImGui::TextColored(m_TraceColor, itr->second.c_str());
						ImGui::PopFont();
					}
					break;
				}
				case Severity::Info:
				{
					if (m_ShowInfoMsgs)
					{
						ImGui::PushFont(consolasFont);
						ImGui::TextColored(m_InfoColor, itr->second.c_str());
						ImGui::PopFont();
					}
					break;
				}
				case Severity::Warning:
				{
					if (m_ShowWarningMsgs)
					{
						ImGui::PushFont(consolasFont);
						ImGui::TextColored(m_WarnColor, itr->second.c_str());
						ImGui::PopFont();
					}
					break;
				}
				case Severity::Error:
				{
					if (m_ShowErrorMsgs)
					{
						ImGui::PushFont(consolasFont);
						ImGui::TextColored(m_ErrorColor, itr->second.c_str());
						ImGui::PopFont();
					}
					break;
				}
				case Severity::Critical:
				{
					if (m_ShowCriticalMsgs)
					{
						ImGui::PushFont(consolasFont);
						ImGui::TextColored(m_CriticalColor, itr->second.c_str());
						ImGui::PopFont();
					}
					break;
				}
				}
			}

			if (m_ScrollLock)
				ImGui::SetScrollHereY(1.0f);

			ImGui::EndChild();
			ImGui::End();
		}
	}

	void LogPanel::Clear()
	{
		ConsoleLog::Clear();
	}

	void LogPanel::AutoClear()
	{
		ConsoleLog::AutoClear();
	}

}