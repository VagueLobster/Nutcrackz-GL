#pragma once

#include "Nutcrackz/Renderer/Texture.hpp"

#include <glad/glad.h>

namespace Nutcrackz {

	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(const TextureSpecification& specification, Buffer data = Buffer());

		virtual ~OpenGLTexture2D();

		virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }
		virtual TextureSpecification& GetSpecification() override { return m_Specification; }

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual uint64_t GetEstimatedSize() const override { return m_Width * m_Height * 4; }

		virtual void ChangeSize(uint32_t newWidth, uint32_t newHeight) override;

		virtual void SetLinear(bool value) { m_Specification.UseLinear = value; }

		virtual void SetData(Buffer data) override;

		virtual void Bind(uint32_t slot = 0) const override;

		virtual bool operator==(const Texture& other) const override
		{
			return m_RendererID == other.GetRendererID();
		}

	private:
		TextureSpecification m_Specification;

		uint32_t m_Width, m_Height;
		uint32_t m_RendererID = 0;
		GLenum m_InternalFormat, m_DataFormat;
	};

}