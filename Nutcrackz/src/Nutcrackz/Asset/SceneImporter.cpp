#include "nzpch.hpp"
#include "SceneImporter.hpp"

#include "Nutcrackz/Project/Project.hpp"
#include "Nutcrackz/Scene/SceneSerializer.hpp"
#include "Nutcrackz/Scripting/ScriptEngine.hpp"

//#include <limits>

namespace Nutcrackz {

	RefPtr<Scene> SceneImporter::ImportScene(AssetHandle handle, const AssetMetadata& metadata)
	{
		//NZ_PROFILE_FUNCTION();

		return LoadScene(Project::GetActiveAssetDirectory() / metadata.FilePath);
	}

	RefPtr<Scene> SceneImporter::LoadScene(const std::filesystem::path& path)
	{
		//NZ_PROFILE_FUNCTION();
		
		RefPtr<Scene> scene = RefPtr<Scene>::Create();
		SceneSerializer serializer(scene);
		//serializer.Deserialize(path); // YAML... Cringe!!!
		serializer.DeserializeJSON(path);

		return scene;
	}

	void SceneImporter::SaveScene(RefPtr<Scene> scene, const std::filesystem::path& path)
	{
		SceneSerializer serializer(scene);
		//serializer.Serialize(Project::GetActiveAssetDirectory() / path); // YAML... Still cringe!!!
		serializer.SerializeJSON(Project::GetActiveAssetDirectory() / path);
	}

	RefPtr<Script> SceneImporter::ImportScript(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadScript(Project::GetActiveAssetDirectory() / metadata.FilePath);
	}

	RefPtr<Script> SceneImporter::LoadScript(const std::filesystem::path& path)
	{
		RefPtr<Script> result = RefPtr<Script>::Create();
		return result;
	}

}
