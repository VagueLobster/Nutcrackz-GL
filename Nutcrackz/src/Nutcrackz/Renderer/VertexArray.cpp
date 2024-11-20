#include "nzpch.hpp"
#include "VertexArray.hpp"

#include "Renderer.hpp"
#include "Platform/OpenGL/OpenGLVertexArray.hpp"

namespace Nutcrackz {

	RefPtr<VertexArray> VertexArray::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    NZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return RefPtr<OpenGLVertexArray>::Create();
		}

		NZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}