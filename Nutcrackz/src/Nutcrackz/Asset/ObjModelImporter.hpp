#pragma once

#include "Asset.hpp"
#include "AssetMetadata.hpp"

#include "Nutcrackz/Renderer/Meshes/ObjModel.hpp"

namespace Nutcrackz {

	class ObjModelImporter : RefCounted
	{
	public:
		// AssetMetadata filepath is relative to project asset directory
		static RefPtr<ObjModel> ImportObjModel(AssetHandle handle, const AssetMetadata& metadata);
		
		// Load from filepath
		static RefPtr<ObjModel> LoadTinyObjModel(const std::filesystem::path& path);
	};

}