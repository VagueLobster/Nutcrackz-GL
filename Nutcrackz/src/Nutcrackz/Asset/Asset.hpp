#pragma once

#include "Nutcrackz/Core/Ref.hpp"
#include "Nutcrackz/Core/UUID.hpp"

#include <string_view>

namespace Nutcrackz {

	using AssetHandle = UUID;

	enum class AssetType : uint16_t
	{
		None = 0,
		Scene,
		Texture2D,
		Font,
		Video,
		Audio,
		ObjModel,
		ScriptFile,
	};

	std::string_view AssetTypeToString(AssetType type);
	AssetType AssetTypeFromString(std::string_view assetType);

	class Asset : public RefCounted
	{
	public:
		AssetHandle Handle; // Generate handle

		virtual AssetType GetType() const = 0;
	};

}
