#pragma once

#include <imgui_internal.h>

namespace Hazard::ImUI
{
	class TextField
	{
	public:
		TextField() = default;
		TextField(const std::string& value) : m_Value(value) {};
		~TextField() {}

		void Render();
		void Clear() { m_Value = ""; }
		void SetIcon(const std::string& icon) { m_Icon = icon; };
		void SetIcon(const char* icon) { m_Icon = icon; };
		void SetHint(const std::string& hint) { m_Hint = hint; };
		bool DidChange() { return m_DidChange; }
		const std::string& GetValue() { return m_Value; }

	private:
		std::string m_Icon;
		std::string m_Value;
		std::string m_Hint;
		uint32_t m_Flags = 0;
		bool m_DidChange = false;
	};
}