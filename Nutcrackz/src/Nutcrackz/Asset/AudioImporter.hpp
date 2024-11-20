#pragma once

#include "Asset.hpp"
#include "AssetMetadata.hpp"

#include "Nutcrackz/Core/Audio/AudioSource.hpp"

namespace Nutcrackz {

	class AudioImporter : RefCounted
	{
	public:
		// AssetMetadata filepath is relative to project asset directory
		static RefPtr<AudioSource> ImportAudio(AssetHandle handle, const AssetMetadata& metadata);
		
		// Load from filepath
		static RefPtr<AudioSource> LoadAudio(const std::filesystem::path& path);
	};

}