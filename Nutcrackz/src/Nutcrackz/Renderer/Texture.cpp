#include "nzpch.hpp"
#include "Texture.hpp"

#include "Renderer.hpp"
#include "Platform/OpenGL/OpenGLTexture.hpp"

namespace Nutcrackz {

	RefPtr<Texture2D> Texture2D::Create(const TextureSpecification& specification, Buffer data)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    NZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return RefPtr<OpenGLTexture2D>::Create(specification, data);
		}

		NZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}