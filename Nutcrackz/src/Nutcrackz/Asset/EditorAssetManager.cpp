#include "nzpch.hpp"
#include "EditorAssetManager.hpp"

#include "AssetImporter.hpp"

#include "Nutcrackz/Project/Project.hpp"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Nutcrackz {

	static std::map<std::filesystem::path, AssetType> s_AssetExtensionMap = {
		{ ".ncz", AssetType::Scene },
		{ ".hazel", AssetType::Scene },
		{ ".png", AssetType::Texture2D },
		{ ".jpg", AssetType::Texture2D },
		{ ".jpeg", AssetType::Texture2D },
		{ ".ttf", AssetType::Font },
		{ ".TTF", AssetType::Font },
		{ ".otf", AssetType::Font },
		{ ".mkv", AssetType::Video },
		{ ".mov", AssetType::Video },
		{ ".mp4", AssetType::Video },
		{ ".mp3", AssetType::Audio },
		{ ".wav", AssetType::Audio },
		{ ".ogg", AssetType::Audio },
		{ ".obj", AssetType::ObjModel },
		{ ".cs", AssetType::ScriptFile },
	};

	static AssetType GetAssetTypeFromFileExtension(const std::filesystem::path& extension)
	{
		if (s_AssetExtensionMap.find(extension) == s_AssetExtensionMap.end())
		{
			NZ_CORE_WARN("Could not find AssetType for {}", extension);
			return AssetType::None;
		}

		return s_AssetExtensionMap.at(extension);
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const std::string_view& v)
	{
		out << std::string(v.data(), v.size());
		return out;
	}

	bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle) const
	{
		return handle != 0 && m_AssetRegistry.find(handle) != m_AssetRegistry.end();
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle handle) const
	{
		if (!IsAssetHandleValid(handle))
			return AssetType::None;

		return m_AssetRegistry.at(handle).Type;
	}

	void EditorAssetManager::ImportAsset(const std::filesystem::path& filepath)
	{
		AssetHandle handle; // generate new handle
		AssetMetadata metadata;
		metadata.FilePath = filepath;
		metadata.Type = GetAssetTypeFromFileExtension(filepath.extension());
		NZ_CORE_ASSERT(metadata.Type != AssetType::None);
		RefPtr<Asset> asset = AssetImporter::ImportAsset(handle, metadata);
		if (asset)
		{
			asset->Handle = handle;
			m_LoadedAssets[handle] = asset;
			m_AssetRegistry[handle] = metadata;
			SerializeAssetRegistry();
		}
	}

	void EditorAssetManager::ImportScriptAsset(const std::filesystem::path& filepath, uint64_t uuid)
	{
		AssetHandle handle = uuid; // generate new handle
		AssetMetadata metadata;
		metadata.FilePath = filepath;
		metadata.Type = GetAssetTypeFromFileExtension(filepath.extension());
		NZ_CORE_ASSERT(metadata.Type != AssetType::None);
		RefPtr<Asset> asset = AssetImporter::ImportAsset(handle, metadata);
		if (asset)
		{
			asset->Handle = handle;
			m_LoadedAssets[handle] = asset;
			m_AssetRegistry[handle] = metadata;
			SerializeAssetRegistry();
		}
	}

	const AssetMetadata& EditorAssetManager::GetMetadata(AssetHandle handle) const
	{
		NZ_PROFILE_FUNCTION_COLOR("EditorAssetManager::GetMetadata", 0xF2A58A);

		static AssetMetadata s_NullMetadata;
		auto it = m_AssetRegistry.find(handle);
		if (it == m_AssetRegistry.end())
			return s_NullMetadata;

		return it->second;
	}

	const std::filesystem::path& EditorAssetManager::GetFilePath(AssetHandle handle) const
	{
		return GetMetadata(handle).FilePath;
	}

	RefPtr<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
	{
		NZ_PROFILE_FUNCTION_COLOR("EditorAssetManager::GetAsset", 0xA3FFA4);

		// 1. check if handle is valid
		if (!IsAssetHandleValid(handle))
			return nullptr;

		// 2. check if asset needs load (and if so, load)
		RefPtr<Asset> asset;
		if (IsAssetLoaded(handle))
		{
			NZ_PROFILE_SCOPE_COLOR("EditorAssetManager::GetAsset Scope", 0xFF7200);

			asset = m_LoadedAssets.at(handle);
		}
		else
		{
			//NZ_PROFILE_SCOPE_COLOR("EditorAssetManager::GetAsset 2 Scope", 0xA331F3);

			// load asset
			const AssetMetadata& metadata = GetMetadata(handle);
			asset = AssetImporter::ImportAsset(handle, metadata);
			if (!asset)
			{
				// import failed
				NZ_CORE_ERROR("EditorAssetManager::GetAsset - asset import failed!");
			}

			m_LoadedAssets[handle] = asset;
		}

		// 3. return asset
		return asset;
	}

	void EditorAssetManager::SerializeAssetRegistry()
	{
		auto path = Project::GetActiveAssetRegistryPath();

		YAML::Emitter out;
		{
			out << YAML::BeginMap; // Root
			out << YAML::Key << "AssetRegistry" << YAML::Value;

			out << YAML::BeginSeq;
			for (const auto& [handle, metadata] : m_AssetRegistry)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << handle;
				std::string filepathStr = metadata.FilePath.generic_string();
				out << YAML::Key << "FilePath" << YAML::Value << filepathStr;
				out << YAML::Key << "Type" << YAML::Value << AssetTypeToString(metadata.Type);
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
			out << YAML::EndMap; // Root
		}

		std::ofstream fout(path);
		fout << out.c_str();

	}

	bool EditorAssetManager::DeserializeAssetRegistry()
	{
		auto path = Project::GetActiveAssetRegistryPath();
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(path.string());
		}
		catch (YAML::ParserException e)
		{
			NZ_CORE_ERROR("Failed to load project file '{0}'\n     {1}", path, e.what());
			return false;
		}

		auto rootNode = data["AssetRegistry"];
		if (!rootNode)
			return false;

		for (const auto& node : rootNode)
		{
			AssetHandle handle = node["Handle"].as<uint64_t>();
			auto& metadata = m_AssetRegistry[handle];
			metadata.FilePath = node["FilePath"].as<std::string>();
			metadata.Type = AssetTypeFromString(node["Type"].as<std::string>());
		}

		return true;
	}

}