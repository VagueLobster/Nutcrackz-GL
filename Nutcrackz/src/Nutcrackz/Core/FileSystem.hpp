#pragma once

#include "Nutcrackz/Core/Buffer.hpp"

namespace Nutcrackz {

	class FileSystem
	{
	public:
		// TODO: move to FileSystem class
		static Buffer ReadFileBinary(const std::filesystem::path& filepath);
	};

}