#pragma once

#include <string>

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Core/Buffer.hpp"

#include "Nutcrackz/Asset/Asset.hpp"

namespace Nutcrackz {

	enum class ImageFormat
	{
		None = 0,
		R8,
		RGB8,
		RGBA8,
		RGBA16F,
		RGBA32F
	};

	struct TextureSpecification
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = true;
		bool UseLinear = false;
	};

	class Texture : public Asset
	{
	public:
		virtual ~Texture() = default;

		virtual const TextureSpecification& GetSpecification() const = 0;
		virtual TextureSpecification& GetSpecification() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual uint64_t GetEstimatedSize() const = 0;

		virtual void SetLinear(bool value) = 0;

		virtual void SetData(Buffer data) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static RefPtr<Texture2D> Create(const TextureSpecification& specification, Buffer data = Buffer());

		virtual void ChangeSize(uint32_t newWidth, uint32_t newHeight) = 0;

		static AssetType GetStaticType() { return AssetType::Texture2D; }
		virtual AssetType GetType() const { return GetStaticType(); }
	};

}