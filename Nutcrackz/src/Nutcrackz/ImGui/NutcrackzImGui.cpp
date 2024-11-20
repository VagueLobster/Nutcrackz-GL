#include "nzpch.hpp"
#include "NutcrackzImgui.hpp"

namespace Nutcrackz::UI {

	const char* GenerateLabelID(std::string_view label)
	{
		*fmt::format_to_n(s_LabelIDBuffer, std::size(s_LabelIDBuffer), "{}##{}", label, s_Counter++).out = 0;
		return s_LabelIDBuffer;
	}

}