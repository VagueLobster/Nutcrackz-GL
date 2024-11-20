#include "nzpch.hpp"
#include "ProjectSerializer.hpp"
#include "Nutcrackz/Core/Timer.hpp"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "yyjson.h"

namespace Nutcrackz {

	ProjectSerializer::ProjectSerializer(RefPtr<Project> project)
		: m_Project(project)
	{
	}

	bool ProjectSerializer::Serialize(const std::filesystem::path& filepath)
	{
		const auto& config = m_Project->GetConfig();

		YAML::Emitter out;
		{
			out << YAML::BeginMap; // Root
			out << YAML::Key << "Project" << YAML::Value;
			{
				out << YAML::BeginMap; // Project
				out << YAML::Key << "Name" << YAML::Value << config.Name;
				out << YAML::Key << "StartScene" << YAML::Value << (uint64_t)config.StartScene;
				out << YAML::Key << "AssetDirectory" << YAML::Value << config.AssetDirectory.string();
				out << YAML::Key << "AssetRegistryPath" << YAML::Value << config.AssetRegistryPath.string();
				out << YAML::Key << "ScriptModulePath" << YAML::Value << config.ScriptModulePath.string();
				out << YAML::EndMap; // Project
			}
			out << YAML::EndMap; // Root
		}

		std::ofstream fout(filepath);
		fout << out.c_str();

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		auto& config = m_Project->GetConfig();

		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath.string());
		}
		catch (YAML::ParserException e)
		{
			NZ_CORE_ERROR("Failed to load project file '{0}'\n    {1}", filepath, e.what());
			return false;
		}

		auto projectNode = data["Project"];
		if (!projectNode)
			return false;

		config.Name = projectNode["Name"].as<std::string>();
		config.StartScene = projectNode["StartScene"].as<uint64_t>();
		config.AssetDirectory = projectNode["AssetDirectory"].as<std::string>();
		if (projectNode["AssetRegistryPath"])
			config.AssetRegistryPath = projectNode["AssetRegistryPath"].as<std::string>();
		config.ScriptModulePath = projectNode["ScriptModulePath"].as<std::string>();

		return true;
	}

	bool ProjectSerializer::SerializeJSON(const std::filesystem::path& filepath)
	{
		Timer timer;

		const auto& config = m_Project->GetConfig();

		yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
		yyjson_mut_val* root = yyjson_mut_obj(doc);
		yyjson_mut_doc_set_root(doc, root);

		yyjson_mut_obj_add_str(doc, root, "Name", config.Name.c_str());
		yyjson_mut_obj_add_uint(doc, root, "StartScene", (uint64_t)config.StartScene);

		std::string assetDir = config.AssetDirectory.string();
		yyjson_mut_obj_add_str(doc, root, "AssetDirectory", assetDir.c_str());
		std::string assetRegistryPath = config.AssetRegistryPath.string();
		yyjson_mut_obj_add_str(doc, root, "AssetRegistryPath", assetRegistryPath.c_str());
		std::string scriptModulePath = config.ScriptModulePath.string();
		yyjson_mut_obj_add_str(doc, root, "ScriptModulePath", scriptModulePath.c_str());

		// Write the json pretty, escape unicode
		yyjson_write_flag flg = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE;
		yyjson_write_err err;
		yyjson_mut_write_file(filepath.string().c_str(), doc, flg, NULL, &err);

		if (err.code)
			NZ_CORE_WARN("Write error ({0}): {1}", err.code, err.msg);

		// Free the doc
		yyjson_mut_doc_free(doc);

		NZ_CORE_WARN("JSON (Project file) Serialization took {0} seconds", timer.ElapsedMicros());

		return true;
	}

	bool ProjectSerializer::DeserializeJSON(const std::filesystem::path& filepath)
	{
		Timer timer;

		auto& config = m_Project->GetConfig();

		// Read JSON file, allowing comments and trailing commas
		yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS | /*YYJSON_READ_INSITU |*/ YYJSON_READ_STOP_WHEN_DONE;
		yyjson_read_err err;
		yyjson_doc* doc = yyjson_read_file(filepath.string().c_str(), flg, NULL, &err);
		yyjson_val* root = yyjson_doc_get_root(doc);

		uint64_t numberOFEntities = 0;

		// Iterate over the root object
		if (doc)
		{
			yyjson_val* obj = yyjson_doc_get_root(doc);
			yyjson_obj_iter iter;
			yyjson_obj_iter_init(obj, &iter);
			yyjson_val* key, * val;

			{
				key = yyjson_obj_iter_next(&iter);

				if (key)
				{
					val = yyjson_obj_iter_get_val(key);
					config.Name = yyjson_get_str(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					config.StartScene = yyjson_get_uint(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					config.AssetDirectory = yyjson_get_str(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					config.AssetRegistryPath = yyjson_get_str(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					config.ScriptModulePath = yyjson_get_str(val);
				}
			}
		}

		NZ_CORE_WARN("JSON (Project file) Deserialization took {0} seconds", timer.ElapsedMicros());

		return true;
	}

}
