#pragma once

#include <vector>

#undef INFINITE
#include "msdf-atlas-gen.h"

namespace Nutcrackz {

	struct MSDFData
	{
		msdf_atlas::FontGeometry FontGeometry;
		std::vector<msdf_atlas::GlyphGeometry> Glyphs;
	};

}