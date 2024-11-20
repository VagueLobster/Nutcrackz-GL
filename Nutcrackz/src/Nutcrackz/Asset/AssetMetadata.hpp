#pragma once

#include "Asset.hpp"

#include <filesystem>

namespace Nutcrackz {

	struct AssetMetadata
	{
		AssetType Type = AssetType::None;
		std::filesystem::path FilePath = "";

		operator bool() const { return Type != AssetType::None; }
	};

}