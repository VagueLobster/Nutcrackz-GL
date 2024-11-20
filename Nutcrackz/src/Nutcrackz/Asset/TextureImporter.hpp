#pragma once

#include "Asset.hpp"
#include "AssetMetadata.hpp"

#include "Nutcrackz/Renderer/Texture.hpp"
#include "Nutcrackz/Video/VideoTexture.hpp"

namespace Nutcrackz {

	class TextureImporter : RefCounted
	{
	public:
		// AssetMetadata filepath is relative to project asset directory
		static RefPtr<Texture2D> ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata);

		// Reads file directly from filesystem
		// (i.e. path has to be relative / absolute to working directory)
		static RefPtr<Texture2D> LoadTexture2D(const std::filesystem::path& path);

		static RefPtr<VideoTexture> ImportVideoTexture(AssetHandle handle, const AssetMetadata& metadata);
	};

}