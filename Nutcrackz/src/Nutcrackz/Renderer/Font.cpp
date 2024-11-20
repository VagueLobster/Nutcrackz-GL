#include "nzpch.hpp"
#include "Font.hpp"

#include "MSDFData.hpp"

#include "Nutcrackz/Asset/FontImporter.hpp"
#include "Nutcrackz/Project/Project.hpp"

namespace Nutcrackz {

	Font::Font()
	{
		m_Data = new MSDFData();
	}

	Font::~Font()
	{
		if (m_Data)
			delete m_Data;
	}

	RefPtr<Font> Font::SetLinearFiltering(AssetHandle& handle, bool useLinear)
	{
		return FontImporter::ReloadFont(Project::GetActive()->GetEditorAssetManager()->GetMetadata(handle), m_Data, m_AtlasTexture, useLinear);
	}

	RefPtr<Font> Font::GetDefault()
	{
		static RefPtr<Font> DefaultFont;
		if (!DefaultFont)
			DefaultFont = FontImporter::LoadFont("assets/fonts/opensans/OpenSans-Regular.ttf");

		return DefaultFont;
	}

}