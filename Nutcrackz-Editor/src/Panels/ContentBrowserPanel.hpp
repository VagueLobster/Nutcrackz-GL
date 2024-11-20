#pragma once

#include "Nutcrackz/Renderer/Texture.hpp"
#include "Nutcrackz/Scene/Scene.hpp"
#include "Nutcrackz/Scene/Entity.hpp"
#include "Nutcrackz/Project/Project.hpp"
//#include "ThumbnailCache.hpp"

#include <map>
#include <set>
#include <filesystem>
#include <unordered_map>

namespace Nutcrackz {

	enum class Mode
	{
		Asset = 0, FileSystem = 1
	};

	class ContentBrowserPanel : public RefCounted
	{
	public:
		//ContentBrowserPanel() = default;
		ContentBrowserPanel(/*Ref<Project> project*/);

		void OnImGuiRender();

	public:
		inline static bool ShowContentBrowserPanel = true;

	private:
		void RefreshAssetTree();

	private:
		//Ref<Project> m_Project;
		//Ref<ThumbnailCache> m_ThumbnailCache;

		std::filesystem::path m_BaseDirectory;
		std::filesystem::path m_CurrentDirectory;
		RefPtr<Texture2D> m_DirectoryIcon;
		RefPtr<Texture2D> m_FileIcon;
		RefPtr<Texture2D> m_BackIcon;

		struct TreeNode
		{
			std::filesystem::path Path;
			AssetHandle Handle = 0;

			uint32_t Parent = (uint32_t)-1;
			std::map<std::filesystem::path, uint32_t> Children;

			TreeNode(const std::filesystem::path& path, AssetHandle handle)
				: Path(path), Handle(handle) {}
		};

		std::vector<TreeNode> m_TreeNodes;

		std::map<std::filesystem::path, std::vector<std::filesystem::path>> m_AssetTree;
	};

}
