#include "nzpch.hpp"
#include "AssetImporter.hpp"

#include "TextureImporter.hpp"
#include "SceneImporter.hpp"
#include "FontImporter.hpp"
#include "AudioImporter.hpp"
#include "ObjModelImporter.hpp"
#include "Nutcrackz/Video/VideoTexture.hpp"

#include <map>

namespace Nutcrackz {

	using AssetImportFunction = std::function<RefPtr<Asset>(AssetHandle, const AssetMetadata&)>;
	static std::map<AssetType, AssetImportFunction> s_AssetImportFunctions = {
		{ AssetType::Scene, SceneImporter::ImportScene },
		{ AssetType::Texture2D, TextureImporter::ImportTexture2D },
		{ AssetType::Font, FontImporter::ImportFont },
		{ AssetType::Video, TextureImporter::ImportVideoTexture },
		{ AssetType::Audio, AudioImporter::ImportAudio },
		{ AssetType::ObjModel, ObjModelImporter::ImportObjModel },
		{ AssetType::ScriptFile, SceneImporter::ImportScript }
	};

	RefPtr<Asset> AssetImporter::ImportAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		NZ_PROFILE_FUNCTION_COLOR("AssetImporter::ImportAsset", 0xF2FA8A);

		{
			NZ_PROFILE_SCOPE_COLOR("AssetImporter::ImportAsset Scope", 0x27628A);

			if (s_AssetImportFunctions.find(metadata.Type) == s_AssetImportFunctions.end())
			{
				NZ_CORE_ERROR("No importer available for asset type: {}", (uint16_t)metadata.Type);
				return nullptr;
			}
		}

		auto& result = s_AssetImportFunctions.at(metadata.Type);//(metadata.Type)(handle, metadata);
		
		{
			NZ_PROFILE_SCOPE_COLOR("AssetImporter::ImportAsset 2 Scope", 0xD1C48A);

			return result(handle, metadata);
		}

	}

}
