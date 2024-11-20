#pragma once

#include "Scene.hpp"

namespace YAML {
	class Emitter;
}

struct yyjson_mut_doc;
struct yyjson_mut_val;

namespace Nutcrackz {

	class SceneSerializer : public RefCounted
	{
	public:
		SceneSerializer(const RefPtr<Scene>& scene);

		void Serialize(const std::filesystem::path& filepath);
		void SerializeJSON(const std::filesystem::path& filepath);
		void SerializeRuntime(const std::filesystem::path& filepath);

		static void SerializeEntity(YAML::Emitter& out, Entity entity, RefPtr<Scene> scene);
		static void SerializeEntityJSON(yyjson_mut_doc* doc, yyjson_mut_val* root, Entity entity, RefPtr<Scene> scene);

		bool Deserialize(const std::filesystem::path& filepath);
		bool DeserializeJSON(const std::filesystem::path& filepath);
		bool DeserializeRuntime(const std::filesystem::path& filepath);

	private:
		RefPtr<Scene> m_Scene;
	};

}