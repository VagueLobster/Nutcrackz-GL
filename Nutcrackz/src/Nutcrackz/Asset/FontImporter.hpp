#pragma once

#include "Asset.hpp"
#include "AssetMetadata.hpp"

#include "Nutcrackz/Renderer/Texture.hpp"

namespace Nutcrackz {

	class Font;
	struct MSDFData;

	class FontImporter : RefCounted
	{
	public:
		// AssetMetadata filepath is relative to project asset directory
		static RefPtr<Font> ImportFont(AssetHandle handle, const AssetMetadata& metadata);
		
		// Load from filepath
		static RefPtr<Font> LoadFont(const std::filesystem::path& path);

		// Refresh font
		static RefPtr<Font> ReloadFont(const AssetMetadata& metadata, MSDFData* data, RefPtr<Texture2D>& atlasTexture, bool useLinear);
	};

}