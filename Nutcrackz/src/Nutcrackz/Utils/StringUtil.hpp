#pragma once

#include <vector>
#include <string>

namespace Nutcrackz {

	namespace Utils::String
	{
		std::string& ToLower(std::string& string);
		void Erase(std::string& str, const char* chars);
	}

	class StringUtil
	{
	public:
		static std::vector<std::string> SplitString(const std::string& string, char delim);
	};

}