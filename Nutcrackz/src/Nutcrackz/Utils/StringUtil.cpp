#include "nzpch.hpp"
#include "StringUtil.hpp"

namespace Nutcrackz {

	namespace Utils::String {

		std::string& ToLower(std::string& string)
		{
			std::transform(string.begin(), string.end(), string.begin(),
				[](const unsigned char c) { return std::tolower(c); });
			return string;
		}

		void Erase(std::string& str, const char* chars)
		{
			for (size_t i = 0; i < strlen(chars); i++)
				str.erase(std::remove(str.begin(), str.end(), chars[i]), str.end());
		}

	}

	std::vector<std::string> StringUtil::SplitString(const std::string& string, char delim)
	{

		std::vector<std::string> result;
		std::istringstream f(string);
		std::string s;

		while (getline(f, s, delim))
		{
			if (!s.empty())
				result.push_back(s);
		}
		return result;
	}

}