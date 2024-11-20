#include "nzpch.hpp"
#include "Asset.hpp"

namespace Nutcrackz {

	std::string_view AssetTypeToString(AssetType type)
	{
		switch (type)
		{
		case AssetType::None:        return "None";
		case AssetType::Scene:       return "Scene";
		case AssetType::Texture2D:   return "Texture2D";
		case AssetType::Font:        return "Font";
		case AssetType::Video:       return "Video";
		case AssetType::Audio:       return "Audio";
		case AssetType::ObjModel:    return "ObjModel";
		case AssetType::ScriptFile:  return "ScriptFile";
		}

		return "<Invalid>";
	}

	AssetType AssetTypeFromString(std::string_view assetType)
	{
		if (assetType == "None")      return AssetType::None;
		if (assetType == "Scene")     return AssetType::Scene;
		if (assetType == "Texture2D") return AssetType::Texture2D;
		if (assetType == "Font")      return AssetType::Font;
		if (assetType == "Video")     return AssetType::Video;
		if (assetType == "Audio")     return AssetType::Audio;
		if (assetType == "ObjModel")  return AssetType::ObjModel;
		if (assetType == "ScriptFile")  return AssetType::ScriptFile;

		return AssetType::None;
	}

}