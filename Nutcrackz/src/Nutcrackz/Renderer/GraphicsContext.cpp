#include "nzpch.hpp"
#include "Nutcrackz/Renderer/GraphicsContext.hpp"

#include "Nutcrackz/Renderer/Renderer.hpp"
#include "Platform/OpenGL/OpenGLContext.hpp"

namespace Nutcrackz {

	std::unique_ptr<GraphicsContext> GraphicsContext::Create(void* window)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    NZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_unique<OpenGLContext>(static_cast<GLFWwindow*>(window));
		}

		NZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}