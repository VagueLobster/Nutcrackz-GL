#pragma once

#include <filesystem>

namespace Nutcrackz {

	class EditorSerializer
	{
	public:
		//static bool EditorSerialize(const std::filesystem::path& filepath);
		//static bool EditorDeserialize(const std::filesystem::path& filepath);
		static bool EditorSerializeJSON(const std::filesystem::path& filepath);
		static bool EditorDeserializeJSON(const std::filesystem::path& filepath);
	};

}
