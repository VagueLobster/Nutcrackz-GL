#include "nzpch.hpp"
#include "Buffer.hpp"

#include "Renderer.hpp"

#include "Platform/OpenGL/OpenGLBuffer.hpp"

namespace Nutcrackz {

	RefPtr<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    NZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return RefPtr<OpenGLVertexBuffer>::Create(size);
		}

		NZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	RefPtr<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    NZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return RefPtr<OpenGLVertexBuffer>::Create(vertices, size);
		}

		NZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	RefPtr<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    NZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return RefPtr<OpenGLIndexBuffer>::Create(indices, size);
		}

		NZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
	
}