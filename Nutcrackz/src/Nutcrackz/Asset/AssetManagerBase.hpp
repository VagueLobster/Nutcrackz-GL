#pragma once

#include "Asset.hpp"

#include <map>

namespace Nutcrackz {

	using AssetMap = std::map<AssetHandle, RefPtr<Asset>>;

	class AssetManagerBase
	{
	public:
		virtual RefPtr<Asset> GetAsset(AssetHandle handle) = 0;

		virtual bool IsAssetHandleValid(AssetHandle handle) const = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) const = 0;
		virtual AssetType GetAssetType(AssetHandle handle) const = 0;
	};

}