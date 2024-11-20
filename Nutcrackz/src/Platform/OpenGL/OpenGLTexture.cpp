#include "nzpch.hpp"
#include "Platform/OpenGL/OpenGLTexture.hpp"

#include "stb_image.h"

#include <GLFW/glfw3.h>

namespace Nutcrackz {

	namespace Utils {

		static GLenum NutcrackzImageFormatToGLDataFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RGB8:  return GL_RGB;
			case ImageFormat::RGBA8: return GL_RGBA;
			}

			NZ_CORE_ASSERT(false);
			return 0;
		}

		static GLenum NutcrackzImageFormatToGLInternalFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RGB8:  return GL_RGB8;
			case ImageFormat::RGBA8: return GL_RGBA8;
			}

			NZ_CORE_ASSERT(false);
			return 0;
		}

	}

	OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification, Buffer data)
		: m_Specification(specification), m_Width(m_Specification.Width), m_Height(m_Specification.Height)
	{
		//NZ_PROFILE_FUNCTION();

		m_InternalFormat = Utils::NutcrackzImageFormatToGLInternalFormat(m_Specification.Format);
		m_DataFormat = Utils::NutcrackzImageFormatToGLDataFormat(m_Specification.Format);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		if (!m_Specification.UseLinear)
		{
			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		else
		{
			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (data)
		{
			SetData(data);
		}
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		//NZ_PROFILE_FUNCTION();

		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::ChangeSize(uint32_t newWidth, uint32_t newHeight)
	{
		// Create new texture
		uint32_t newTextureID;
		glCreateTextures(GL_TEXTURE_2D, 1, &newTextureID);
		glTextureStorage2D(newTextureID, 1, m_InternalFormat, newWidth, newHeight);

		glTextureParameteri(newTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(newTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(newTextureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(newTextureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		GLuint framebufferRendererIDs[2] = { 0 };
		glGenFramebuffers(2, framebufferRendererIDs);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferRendererIDs[0]);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RendererID, 0);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferRendererIDs[1]);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, newTextureID, 0);

		glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, newWidth, newHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glDeleteTextures(1, &m_RendererID);
		glDeleteFramebuffers(2, framebufferRendererIDs);

		m_RendererID = newTextureID;
		m_Width = newWidth;
		m_Height = newHeight;
	}

	void OpenGLTexture2D::SetData(Buffer data)
	{
		//NZ_PROFILE_FUNCTION();

		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		NZ_CORE_ASSERT(data.Size == m_Width * m_Height * bpp, "Data must be entire texture!");
		
		if (!m_Specification.UseLinear)
		{
			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		else
		{
			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data.Data);
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		//NZ_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_RendererID);
	}

}