#pragma once

#include "Nutcrackz/Project/Project.hpp"
#include "Nutcrackz/Renderer/Texture.hpp"

#include <queue>

namespace Nutcrackz {

	struct ThumbnailImage
	{
		uint64_t Timestamp;
		RefPtr<Texture2D> Image;
	};

	class ThumbnailCache : public RefCounted
	{
	public:
		ThumbnailCache(RefPtr<Project> project);

		RefPtr<Texture2D> GetOrCreateThumbnail(const std::filesystem::path& path);
		void OnUpdate();

	private:
		RefPtr<Project> m_Project;

		uint32_t m_ThumbnailSize = 96;

		std::map<std::filesystem::path, ThumbnailImage> m_CachedImages;

		struct ThumbnailInfo
		{
			std::filesystem::path AbsolutePath;
			std::filesystem::path AssetPath;
			uint64_t Timestamp;
		};

		std::queue<ThumbnailInfo> m_Queue;

		// TEMP (replace with Hazel::Serialization)
		std::filesystem::path m_ThumbnailCachePath;
	};

}