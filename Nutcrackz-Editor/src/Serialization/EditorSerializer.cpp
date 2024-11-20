#include "EditorSerializer.hpp"

#include "../EditorLayer.hpp"

#include <fstream>
//#include <yaml-cpp/yaml.h>

#include "yyjson.h"

namespace Nutcrackz {

	/*bool EditorSerializer::EditorSerialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		{
			out << YAML::BeginMap; // Root
			out << YAML::Key << "Editor" << YAML::Value;
			{
				out << YAML::BeginMap; // Project
				out << YAML::Key << "FramesToStep" << YAML::Value << EditorLayer::FramesToStep;
				out << YAML::Key << "ShowEditorViewport" << YAML::Value << EditorLayer::ShowEditorViewport;
				//out << YAML::Key << "ShowGameViewport" << YAML::Value << EditorLayer::ShowGameViewport;
				out << YAML::Key << "ShowSettingsViewport" << YAML::Value << EditorLayer::ShowSettingsViewport;
				out << YAML::Key << "Show2DStatisticsViewport" << YAML::Value << EditorLayer::Show2DStatisticsViewport;
				out << YAML::Key << "ShowAboutPopupWindow" << YAML::Value << EditorLayer::ShowAboutPopupWindow;
				out << YAML::Key << "ShowNewProjectPopupWindow" << YAML::Value << EditorLayer::ShowNewProjectPopupWindow;
				out << YAML::Key << "ShowEditProjectPopupWindow" << YAML::Value << EditorLayer::ShowEditProjectPopupWindow;
				out << YAML::Key << "ShowEditGravityPopupWindow" << YAML::Value << EditorLayer::ShowEditGravityPopupWindow;
				out << YAML::Key << "ShowingAboutPopupWindow" << YAML::Value << EditorLayer::ShowingAboutPopupWindow;
				out << YAML::Key << "ShowSceneHierarchy" << YAML::Value << SceneHierarchyPanel::ShowSceneHierarchy;
				out << YAML::Key << "ShowProperties" << YAML::Value << SceneHierarchyPanel::ShowProperties;
				out << YAML::Key << "ShowContentBrowserPanel" << YAML::Value << ContentBrowserPanel::ShowContentBrowserPanel;
				out << YAML::Key << "ShowLogPanel" << YAML::Value << LogPanel::ShowLogPanel;

				out << YAML::Key << "AnimateTexturesInEdit" << YAML::Value << EditorLayer::AnimateTexturesInEdit;
				out << YAML::Key << "UseDarkTheme" << YAML::Value << EditorLayer::UseDarkTheme;
				out << YAML::Key << "UseLightTheme" << YAML::Value << EditorLayer::UseLightTheme;
				out << YAML::Key << "UseGoldDarkTheme" << YAML::Value << EditorLayer::UseGoldDarkTheme;
				out << YAML::Key << "UseChocolateTheme" << YAML::Value << EditorLayer::UseChocolateTheme;
				out << YAML::EndMap; // Project
			}
			out << YAML::EndMap; // Root
		}

		std::ofstream fout(filepath);
		fout << out.c_str();

		return true;
	}

	bool EditorSerializer::EditorDeserialize(const std::filesystem::path& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath.string());
		}
		catch (YAML::ParserException e)
		{
			NZ_CORE_ERROR("Failed to load editor data file '{0}'\n    {1}", filepath, e.what());
			return false;
		}

		auto editorNode = data["Editor"];
		if (!editorNode)
			return false;

		EditorLayer::FramesToStep = editorNode["FramesToStep"].as<int>();
		EditorLayer::ShowEditorViewport = editorNode["ShowEditorViewport"].as<bool>();
		//EditorLayer::ShowGameViewport = editorNode["ShowGameViewport"].as<bool>();
		EditorLayer::ShowSettingsViewport = editorNode["ShowSettingsViewport"].as<bool>();
		EditorLayer::Show2DStatisticsViewport = editorNode["Show2DStatisticsViewport"].as<bool>();
		EditorLayer::ShowAboutPopupWindow = editorNode["ShowAboutPopupWindow"].as<bool>();
		EditorLayer::ShowNewProjectPopupWindow = editorNode["ShowNewProjectPopupWindow"].as<bool>();
		EditorLayer::ShowEditProjectPopupWindow = editorNode["ShowEditProjectPopupWindow"].as<bool>();
		EditorLayer::ShowEditGravityPopupWindow = editorNode["ShowEditGravityPopupWindow"].as<bool>();
		EditorLayer::ShowingAboutPopupWindow = editorNode["ShowingAboutPopupWindow"].as<bool>();
		SceneHierarchyPanel::ShowSceneHierarchy = editorNode["ShowSceneHierarchy"].as<bool>();
		SceneHierarchyPanel::ShowProperties = editorNode["ShowProperties"].as<bool>();
		ContentBrowserPanel::ShowContentBrowserPanel = editorNode["ShowContentBrowserPanel"].as<bool>();
		LogPanel::ShowLogPanel = editorNode["ShowLogPanel"].as<bool>();

		EditorLayer::AnimateTexturesInEdit = editorNode["AnimateTexturesInEdit"].as<bool>();
		EditorLayer::UseDarkTheme = editorNode["UseDarkTheme"].as<bool>();
		EditorLayer::UseLightTheme = editorNode["UseLightTheme"].as<bool>();
		EditorLayer::UseGoldDarkTheme = editorNode["UseGoldDarkTheme"].as<bool>();
		EditorLayer::UseChocolateTheme = editorNode["UseChocolateTheme"].as<bool>();

		return true;
	}*/

	bool EditorSerializer::EditorSerializeJSON(const std::filesystem::path& filepath)
	{
		Timer timer;

		yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
		yyjson_mut_val* root = yyjson_mut_obj(doc);
		yyjson_mut_doc_set_root(doc, root);

		yyjson_mut_obj_add_int(doc, root, "FramesToStep", EditorLayer::FramesToStep);
		yyjson_mut_obj_add_bool(doc, root, "ShowEditorViewport", EditorLayer::ShowEditorViewport);
		//yyjson_mut_obj_add_bool(doc, root, "ShowGameViewport", EditorLayer::ShowGameViewport);
		yyjson_mut_obj_add_bool(doc, root, "ShowSettingsViewport", EditorLayer::ShowSettingsViewport);
		yyjson_mut_obj_add_bool(doc, root, "Show2DStatisticsViewport", EditorLayer::Show2DStatisticsViewport);
		yyjson_mut_obj_add_bool(doc, root, "ShowAboutPopupWindow", EditorLayer::ShowAboutPopupWindow);
		yyjson_mut_obj_add_bool(doc, root, "ShowNewProjectPopupWindow", EditorLayer::ShowNewProjectPopupWindow);
		yyjson_mut_obj_add_bool(doc, root, "ShowEditProjectPopupWindow", EditorLayer::ShowEditProjectPopupWindow);
		yyjson_mut_obj_add_bool(doc, root, "ShowEditGravityPopupWindow", EditorLayer::ShowEditGravityPopupWindow);
		yyjson_mut_obj_add_bool(doc, root, "ShowingAboutPopupWindow", EditorLayer::ShowingAboutPopupWindow);
		yyjson_mut_obj_add_bool(doc, root, "ShowSceneHierarchy", SceneHierarchyPanel::ShowSceneHierarchy);
		yyjson_mut_obj_add_bool(doc, root, "ShowProperties", SceneHierarchyPanel::ShowProperties);
		yyjson_mut_obj_add_bool(doc, root, "ShowContentBrowserPanel", ContentBrowserPanel::ShowContentBrowserPanel);
		yyjson_mut_obj_add_bool(doc, root, "ShowLogPanel", LogPanel::ShowLogPanel);

		yyjson_mut_obj_add_bool(doc, root, "AnimateTexturesInEdit", EditorLayer::AnimateTexturesInEdit);
		yyjson_mut_obj_add_bool(doc, root, "UseGreenDarkTheme", EditorLayer::UseGreenDarkTheme);
		yyjson_mut_obj_add_bool(doc, root, "UseOrangeDarkTheme", EditorLayer::UseOrangeDarkTheme);
		yyjson_mut_obj_add_bool(doc, root, "UseLightTheme", EditorLayer::UseLightTheme);
		yyjson_mut_obj_add_bool(doc, root, "UseGoldDarkTheme", EditorLayer::UseGoldDarkTheme);
		yyjson_mut_obj_add_bool(doc, root, "UseChocolateTheme", EditorLayer::UseChocolateTheme);

		// Write the json pretty, escape unicode
		yyjson_write_flag flg = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE;
		yyjson_write_err err;
		yyjson_mut_write_file(filepath.string().c_str(), doc, flg, NULL, &err);

		if (err.code)
			NZ_CORE_WARN("Write error ({0}): {1}", err.code, err.msg);

		// Free the doc
		yyjson_mut_doc_free(doc);

		NZ_CORE_WARN("JSON (Editor settings) Serialization took {0} seconds", timer.ElapsedMicros());

		return true;
	}

	bool EditorSerializer::EditorDeserializeJSON(const std::filesystem::path& filepath)
	{
		Timer timer;

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

					EditorLayer::FramesToStep = yyjson_get_int(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::ShowEditorViewport = yyjson_get_bool(val);

					//key = yyjson_obj_iter_next(&iter);
					//val = yyjson_obj_iter_get_val(key);
					//EditorLayer::ShowGameViewport = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::ShowSettingsViewport = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::Show2DStatisticsViewport = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::ShowAboutPopupWindow = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::ShowNewProjectPopupWindow = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::ShowEditProjectPopupWindow = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::ShowEditGravityPopupWindow = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::ShowingAboutPopupWindow = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					SceneHierarchyPanel::ShowSceneHierarchy = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					SceneHierarchyPanel::ShowProperties = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					ContentBrowserPanel::ShowContentBrowserPanel = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					LogPanel::ShowLogPanel = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::AnimateTexturesInEdit = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::UseGreenDarkTheme = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::UseOrangeDarkTheme = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::UseLightTheme = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::UseGoldDarkTheme = yyjson_get_bool(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					EditorLayer::UseChocolateTheme = yyjson_get_bool(val);
				}
			}
		}

		NZ_CORE_WARN("JSON (Editor settings) Deserialization took {0} seconds", timer.ElapsedMicros());

		return true;
	}

}
