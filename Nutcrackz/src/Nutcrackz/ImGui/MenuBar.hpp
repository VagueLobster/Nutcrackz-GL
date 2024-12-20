#pragma once

#include "Nutcrackz/Utils/StringUtil.hpp"
#include "StyleManager.hpp"
#include "UILibrary.hpp"
#include "ScopedVar.hpp"
#include <functional>

#include "imgui.h"

namespace Hazard::ImUI
{
	struct MenuItemElement {
		std::string Name;
		std::function<void()> OnClicked;

		std::vector<MenuItemElement> m_SubMenus;

		MenuItemElement& GetSubMenu(const std::string& name) {
			for (auto& item : m_SubMenus) {
				if (item.Name == name) return item;
			}
			//Does not exist yet
			MenuItemElement& item = m_SubMenus.emplace_back();
			item.Name = name;
			item.OnClicked = nullptr;
			return item;
		}
		MenuItemElement& GetSubMenuFromPath(std::vector<std::string> path) {
			for (auto& item : m_SubMenus) {
				if (item.Name == path[0]) {
					if (path.size() <= 1) return item;
					else return item.GetSubMenuFromPath({ path.begin() + 1, path.end() });
				}
			}
			MenuItemElement& item = m_SubMenus.emplace_back();
			item.Name = path[0];

			if (path.size() <= 1) return item;
			else return item.GetSubMenuFromPath({ path.begin() + 1, path.end() });
		}
	};


	class MenuBar {
	public:

		MenuBar()
		{
			m_MenuItems = std::vector<MenuItemElement>();
		}

		virtual void Init() = 0;

		void Render()
		{
			ImGui::BeginMainMenuBar();
			{
				ScopedStyleStack vars(ImGuiStyleVar_WindowPadding, ImVec2(4, 2), ImGuiStyleVar_ChildBorderSize, 0);

				for (auto& item : m_MenuItems)
				{
					if (ImGui::BeginMenu(item.Name.c_str(), true))
					{
						RenderSubMenu(item);
						ImGui::EndMenu();
					}
				}
			}
			ImGui::Separator();
			ImGui::EndMainMenuBar();
		};
		void AddMenuItem(const std::string& name, const std::function<void()> onClick = nullptr)
		{
			if (onClick == nullptr) return;

			std::vector<std::string> path = Nutcrackz::StringUtil::SplitString(name, '/');
			MenuItemElement& target = GetMenu(name);
			target.OnClicked = onClick;
		}
		void ClearMenuBar()
		{
			m_MenuItems.clear();
		}

		void Reset()
		{
			ClearMenuBar();
			Init();
		}

		MenuItemElement& GetMenu(const std::string& name)
		{

			std::vector<std::string> path = Nutcrackz::StringUtil::SplitString(name, '/');

			MenuItemElement& heading = FindMenu(path[0]);
			if (path.size() <= 1) return heading;

			return heading.GetSubMenuFromPath({ path.begin() + 1, path.end() });
		}
	private:

		MenuItemElement& FindMenu(const std::string& name)
		{
			for (auto& item : m_MenuItems)
				if (item.Name == name)
					return item;

			//Does not exist yet
			MenuItemElement& item = m_MenuItems.emplace_back();
			item.Name = name;
			item.OnClicked = nullptr;
			return item;

		}

		void RenderSubMenu(const MenuItemElement& item)
		{
			ScopedStyleVar spacing(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));

			for (auto& subItem : item.m_SubMenus)
			{
				if (subItem.m_SubMenus.size())
				{
					if (ImGui::BeginMenu(subItem.Name.c_str()))
					{
						RenderSubMenu(subItem);
						ImGui::EndMenu();
					}
					continue;
				}
				if (ImGui::MenuItem(subItem.Name.c_str()))
				{
					if (subItem.OnClicked)
						subItem.OnClicked();
				}
			}
		}

	private:
		std::vector<MenuItemElement> m_MenuItems;

	};
}
