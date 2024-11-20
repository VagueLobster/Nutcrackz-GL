#pragma once

#include "Nutcrackz/Core/Layer.hpp"

#include "Nutcrackz/Events/ApplicationEvent.hpp"
#include "Nutcrackz/Events/KeyEvent.hpp"
#include "Nutcrackz/Events/MouseEvent.hpp"

struct ImVec4;

namespace Nutcrackz {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();
		void ResizeFont(float dpiSize);

		void BlockEvents(bool block) { m_BlockEvents = block; }

		uint32_t GetActiveWidgetID() const;

		static void SetDarkThemeColorsGreen();
		static void SetDarkThemeColorsOrange();
		static void SetGoldDarkThemeColors();
		static void SetChocolateThemeColors();
		static void SetLightThemeColors();

		static ImVec4 ConvertColorFromByteToFloats(uint32_t r, uint32_t g, uint32_t b, uint32_t a);
		static uint32_t ConvertColorFromFloatToByte(float value);

	private:
		bool m_BlockEvents = true;
		float m_Time = 0.0f;

		bool m_HasResized = true;
		float m_DPISize;
	};

}
