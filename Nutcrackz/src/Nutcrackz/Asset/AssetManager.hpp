#pragma once

#include "AssetManagerBase.hpp"

#include "Nutcrackz/Project/Project.hpp"

namespace Nutcrackz {

	class AssetManager : public RefCounted
	{
	public:
		template<typename T>
		static RefPtr<T> GetAsset(AssetHandle handle)
		{
			//NZ_PROFILE_FUNCTION_COLOR("AssetManager::GetAsset", 0x8CCBFF);

			RefPtr<Asset> asset = Project::GetActive()->GetAssetManager()->GetAsset(handle);
			return asset.As<T>();
		}

		static bool IsAssetHandleValid(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->IsAssetHandleValid(handle);
		}

		static bool IsAssetLoaded(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->IsAssetLoaded(handle);
		}

		static AssetType GetAssetType(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->GetAssetType(handle);
		}
	};

}
