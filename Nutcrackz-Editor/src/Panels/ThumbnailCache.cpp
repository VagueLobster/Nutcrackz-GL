#include "ThumbnailCache.hpp"

#include "Nutcrackz/Asset/TextureImporter.hpp"

#include <chrono>

namespace Nutcrackz {

	ThumbnailCache::ThumbnailCache(RefPtr<Project> project)
		: m_Project(project)
	{
		// TODO: revisit this path (move to Cache dir)
		m_ThumbnailCachePath = m_Project->GetAssetDirectory() / "Thumbnail.cache";
	}

	RefPtr<Texture2D> ThumbnailCache::GetOrCreateThumbnail(const std::filesystem::path& assetPath)
	{
		// 1. Read file timestamp
		// 2. Compare hashed timestamp with existing cached image (in memory first, then from cache file)
		// 3. If equal, return associated thumbnail, otherwise load asset from disk and generate thumbnail
		// 4. If generated new thumbnail, store in cache obviously

		auto absolutePath = m_Project->GetAssetAbsolutePath(assetPath);
		std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(absolutePath);
		uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(lastWriteTime.time_since_epoch()).count();

		if (m_CachedImages.find(assetPath) != m_CachedImages.end())
		{
			auto& cachedImage = m_CachedImages.at(assetPath);
			if (cachedImage.Timestamp == timestamp)
				return cachedImage.Image;
		}

		// TODO: PNGs for now
		if (assetPath.extension() != ".png")
			return nullptr;

		m_Queue.push({ absolutePath, assetPath, timestamp });

		return nullptr;
	}

	void ThumbnailCache::OnUpdate()
	{
		while (!m_Queue.empty())
		{
			const auto& thumbnailInfo = m_Queue.front();

			if (m_CachedImages.find(thumbnailInfo.AssetPath) != m_CachedImages.end())
			{
				auto& cachedImage = m_CachedImages.at(thumbnailInfo.AssetPath);
				if (cachedImage.Timestamp == thumbnailInfo.Timestamp)
				{
					m_Queue.pop();
					continue;
				}
			}

			RefPtr<Texture2D> texture = TextureImporter::LoadTexture2D(thumbnailInfo.AbsolutePath);

			if (!texture)
			{
				m_Queue.pop();
				continue;
			}

			float thumbnailHeight = m_ThumbnailSize * ((float)texture->GetHeight() / (float)texture->GetWidth());
			texture->ChangeSize(m_ThumbnailSize, static_cast<uint32_t>(thumbnailHeight));

			auto& cachedImage = m_CachedImages[thumbnailInfo.AssetPath];
			cachedImage.Timestamp = thumbnailInfo.Timestamp;
			cachedImage.Image = texture;
			m_Queue.pop();
			break;
		}
	}

}
