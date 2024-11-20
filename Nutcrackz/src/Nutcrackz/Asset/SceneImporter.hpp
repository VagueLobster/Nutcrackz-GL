#pragma once

#include "Asset.hpp"
#include "AssetMetadata.hpp"

#include "Nutcrackz/Scene/Scene.hpp"
#include "Nutcrackz/Scripting/ScriptFile.hpp"

namespace Nutcrackz {

	class SceneImporter : RefCounted
	{
	public:
		// AssetMetadata filepath is relative to project asset directory
		static RefPtr<Scene> ImportScene(AssetHandle handle, const AssetMetadata& metadata);
		
		// Load from filepath
		static RefPtr<Scene> LoadScene(const std::filesystem::path& path);

		static void SaveScene(RefPtr<Scene> scene, const std::filesystem::path& path);

		static RefPtr<Script> ImportScript(AssetHandle handle, const AssetMetadata& metadata);

		static RefPtr<Script> LoadScript(const std::filesystem::path& path);
	};

}