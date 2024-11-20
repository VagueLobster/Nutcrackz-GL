#pragma once

#include "Project.hpp"

namespace Nutcrackz {

	class ProjectSerializer : public RefCounted
	{
	public:
		ProjectSerializer(RefPtr<Project> project);

		bool Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);

		bool SerializeJSON(const std::filesystem::path& filepath);
		bool DeserializeJSON(const std::filesystem::path& filepath);

	private:
		RefPtr<Project> m_Project;
	};

}
