#pragma once

#include <vector>
#include <string>

#include <imgui_internal.h>

namespace Hazard::ImUI
{
	class Dropdown
	{
	public:
		Dropdown() = default;
		Dropdown(const std::string& title, float titleWidth = 125.0f) : m_Title(title), m_Width(titleWidth) {}
		~Dropdown() {}

		void Render();
		
		void SetOptions(const std::vector<std::string>& options) { m_Options = options; }
		void SetOptions(const std::initializer_list<std::string>& options) { m_Options = options; }
		uint64_t GetSelected() { return m_Selected; }
		void SetSelected(uint32_t selected) { m_Selected = selected; }

		void SetMixed(bool mixed = true) { m_Mixed = mixed; }
		bool DidChange() { return m_DidChange; }
		const std::string& GetSelectedValue() { return m_Options[m_Selected]; }

	private:
		std::string m_Title;
		float m_Width;
		uint64_t m_Selected;
		std::vector<std::string> m_Options;
		bool m_DidChange = false;
		bool m_Mixed = false;
	};
}