#pragma once

#include "AssetMetadata.hpp"

namespace Nutcrackz {

	class AssetImporter
	{
	public:
		static RefPtr<Asset> ImportAsset(AssetHandle handle, const AssetMetadata& metadata);
	};

}