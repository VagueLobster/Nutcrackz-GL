#pragma once

#include <filesystem>

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Renderer/Texture.hpp"

#include "Nutcrackz/Asset/Asset.hpp"

namespace Nutcrackz {

	struct MSDFData;

	class Font : public Asset
	{
	public:
		Font();
		~Font();

		virtual AssetType GetType() const { return AssetType::Font; }

		MSDFData* GetMSDFData() { return m_Data; }
		void SetMSDFData(MSDFData* data) { m_Data = data; }
		
		RefPtr<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
		void SetAtlasTexture(const RefPtr<Texture2D>& atlasTexture) { m_AtlasTexture = atlasTexture; }
		RefPtr<Font> SetLinearFiltering(AssetHandle& handle, bool value);

		static RefPtr<Font> GetDefault();

	public:
		RefPtr<Texture2D> m_AtlasTexture;

	private:
		MSDFData* m_Data;
	};

}