#include "nzpch.hpp"
#include "UniformBuffer.hpp"

#include "Renderer.hpp"
#include "Platform/OpenGL/OpenGLUniformBuffer.hpp"

namespace Nutcrackz {

	RefPtr<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    NZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return RefPtr<OpenGLUniformBuffer>::Create(size, binding);
		}

		NZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}