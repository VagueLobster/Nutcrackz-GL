#pragma once

#include <imgui.h>

namespace Hazard::ImUI
{
	static ImVec4 ColorToImVec4(float r, float g, float b, float a)
	{
		return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
	}

	struct WindowStyle
	{
		ImVec2 Padding = { 2, 2 };
		float BorderSize = 0.0f;
		float PopupBorderSize = 0.0f;

		ImVec2 TitleAlign = { 0.5f, 0.5f };
		ImGuiDir MenuButtonPosition = ImGuiDir_None;
		ImVec2 DisplaySafeArea = { 0, 6 };
		float Rounding = 2;

		ImVec4 TitleBgColor = ColorToImVec4(24.0f, 24.0f, 22.0f, 255.0f);
		ImVec4 PopupBgColor = ColorToImVec4(24.0f, 24.0f, 22.0f, 255.0f);
		ImVec4 TitleBgActive = ColorToImVec4(24.0f, 24.0f, 22.0f, 255.0f);
		ImVec4 Header = ColorToImVec4(50.0f, 50.0f, 52.0f, 255.0f);
		ImVec4 HeaderHighlighted = ColorToImVec4(64.0f, 64.0f, 66.0f, 255.0f);
		ImVec4 HeaderHovered = ColorToImVec4(46.0f, 46.0f, 48.0f, 255.0f);
		ImVec4 HeaderActive = ColorToImVec4(46.0f, 99.0f, 3.0f, 255.0f);

		ImVec4 ResizeGrip = ColorToImVec4(24.0f, 24.0f, 22.0f, 255.0f);
		ImVec4 ResizeGripHovered = ColorToImVec4(93.0f, 197.0f, 5.0f, 112.0f);
		ImVec4 ResizeGripActive = ColorToImVec4(83.0f, 179.0f, 5.0f, 255.0f);

		ImVec4 Text = ColorToImVec4(185.0f, 185.0f, 185.0f, 255.0f);
		ImVec4 TextDark = ColorToImVec4(80.0f, 80.0f, 80.0f, 255.0f);
		ImVec4 TextSelectedBg = ColorToImVec4(93.0f, 197.0f, 5.0f, 75.0f);
		ImVec4 TextDisabled = ColorToImVec4(144.0f, 144.0f, 144.0f, 255.0f);

		ImVec4 Plotlines = ColorToImVec4(83.0f, 179.0f, 5.0f, 255.0f);
		ImVec4 Border = ColorToImVec4(48.0f, 48.0f, 48.0f, 255.0f);

		ImVec4 Checkmark = ColorToImVec4(93.0f, 197.0f, 5.0f, 255.0f);
		ImVec4 SliderGrab = ColorToImVec4(48.0f, 48.0f, 48.0f, 255.0f);
		ImVec4 SliderGrabActive = ColorToImVec4(80.0f, 80.0f, 80.0f, 255.0f);

	};
	struct FrameStyle
	{
		ImVec2 Padding = { 0, 8 };
		float BorderSize = 0;
		float Rounding = 2;

		ImVec4 FrameColor = ColorToImVec4(13.0f, 13.0f, 11.0f, 255.0f);
		ImVec4 FrameHovered = ColorToImVec4(11.0f, 11.0f, 9.0f, 255.0f);
		ImVec4 FrameActive = ColorToImVec4(11.0f, 11.0f, 9.0f, 255.0f);
	};
	struct TabStyle 
	{
		float TabRounding = 2;

		ImVec4 Tab = ColorToImVec4(32.0f, 32.0f, 32.0f, 255.0f);
		ImVec4 TabActiveFocus = ColorToImVec4(50.0f, 50.0f, 48.0f, 255.0f);
		ImVec4 TabActive = ColorToImVec4(36.0f, 36.0f, 34.0f, 255.0f);
		ImVec4 TabUnfocus = ColorToImVec4(22.0f, 22.0f, 20.0f, 255.0f);
		ImVec4 TabHovered = ColorToImVec4(93.0f, 197.0f, 5.0f, 148.0f);
	};

	struct DockspaceStyle 
	{
		ImVec4 DockingPreview = ColorToImVec4(93.0f, 197.0f, 5.0f, 42.0f);
	};
	
	struct ScrollbarStyle 
	{
		float ScrollbarSize = 16;
		float ScrollBarRounding = 6;
		float GrabRounding = 2;

		ImVec4 ScrollbarGrab = ColorToImVec4(46.0f, 99.0f, 3.0f, 255.0f);
		ImVec4 ScrollbarGrabHovered = ColorToImVec4(74.0f, 159.0f, 4.0f, 255.0f);
		ImVec4 ScrollbarGrabActive = ColorToImVec4(65.0f, 139.0f, 4.0f, 255.0f);
	};
	struct ButtonStyle 
	{
		ImVec4 Button = ColorToImVec4(34.0f, 34.0f, 34.0f, 255.0f);
		ImVec4 ButtonHovered = ColorToImVec4(24.0f, 24.0f, 24.0f, 255.0f);
		ImVec4 ButtonActive = ColorToImVec4(34.0f, 34.0f, 34.0f, 255.0f);
	};
	struct SeparatorStyle 
	{
		ImVec4 Separator = ColorToImVec4(24.0f, 24.0f, 22.0f, 255.0f);
		ImVec4 SeparatorHovered = ColorToImVec4(93.0f, 197.0f, 5.0f, 112.0f);
		ImVec4 SeparatorActive = ColorToImVec4(93.0f, 197.0f, 5.0f, 255.0f);
	};
	struct StylePrimaryColors {
		ImVec4 AxisX = ColorToImVec4(219.0f, 55.0f, 33.0f, 255.0f);
		ImVec4 AxisY = ColorToImVec4(83.0f, 179.0f, 5.0f, 255.0f);
		ImVec4 AxisZ = ColorToImVec4(22.0f, 81.0f, 243.0f, 255.0f);

		ImVec4 Warning = ColorToImVec4(255.0f, 230.0f, 0.0f, 255.0f);

	};
	struct Style
	{
		WindowStyle Window;
		FrameStyle Frame;
		TabStyle Tab;
		DockspaceStyle Dockspace;
		ScrollbarStyle ScrollBar;
		ButtonStyle Button;
		SeparatorStyle Separator;
		StylePrimaryColors Colors;

		ImVec2 ItemInnerSpacing = { 6, 4 };

		float IndentSpacing = 16;

		ImVec4 BackgroundColor = ColorToImVec4(32.0f, 32.0f, 32.0f, 255.0f);
		ImVec4 ModalBackgroundColor = ColorToImVec4(10.0f, 10.0f, 10.0f, 128.0f);
		ImVec4 ChildBackgroundColor = ColorToImVec4(38.0f, 38.0f, 38.0f, 255.0f);
		ImVec4 MenuBarBackground = ColorToImVec4(24.0f, 24.0f, 22.0f, 255.0f);
		ImVec4 NavHighlight = ColorToImVec4(93.0f, 197.0f, 5.0f, 144.0f);
		ImVec4 DragDropTarget = ColorToImVec4(65.0f, 139.0f, 4.0f, 255.0f);
	};
}