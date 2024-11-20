#pragma once

#include "Nutcrackz/Asset/Asset.hpp"

namespace Nutcrackz {

	class Script : public Asset
	{
	public:
		Script() = default;
		~Script() = default;

		static AssetType GetStaticType() { return AssetType::ScriptFile; }
		virtual AssetType GetType() const override { return GetStaticType(); }
	};

}