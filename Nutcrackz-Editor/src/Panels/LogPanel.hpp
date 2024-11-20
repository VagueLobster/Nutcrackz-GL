#pragma once

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Core/Ref.hpp"
#include "Nutcrackz/Core/ConsoleLog.hpp"

#include "imgui/imgui.h"

namespace Nutcrackz {

	class LogPanel : public RefCounted
	{
	public:
		LogPanel() = default;

		static RefPtr<LogPanel>& Get() { return s_Console; }

		void OnImGuiRender();
		void Clear();
		void AutoClear();

	public:
		inline static bool ShowLogPanel = true;

	private:
		static RefPtr<LogPanel> s_Console;
		
		bool m_AutoClear = false;
		bool m_ScrollLock = true;
		bool m_ShowInfoMsgs = true;
		bool m_ShowTraceMsgs = true;
		bool m_ShowWarningMsgs = true;
		bool m_ShowErrorMsgs = true;
		bool m_ShowCriticalMsgs = true;

		//Colors
		ImVec4 m_TraceColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		ImVec4 m_InfoColor = { 0.1f, 0.9f, 0.1f, 1.0f };
		ImVec4 m_WarnColor = { 1.0f, 0.9f, 0.0f, 1.0f };
		ImVec4 m_ErrorColor = { 1.0f, 0.2f, 0.1f, 1.0f };
		ImVec4 m_CriticalColor = { 0.5f, 0.0f, 0.7f, 1.0f };
	};
}
