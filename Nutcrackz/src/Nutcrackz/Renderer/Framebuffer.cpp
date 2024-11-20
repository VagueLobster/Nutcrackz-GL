#include "nzpch.hpp"
#include "Nutcrackz/Renderer/Framebuffer.hpp"
#include "Nutcrackz/Renderer/Renderer.hpp"

#include "Platform/OpenGL/OpenGLFramebuffer.hpp"

namespace Nutcrackz {

	RefPtr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    NZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return RefPtr<OpenGLFramebuffer>::Create(spec);
		}

		NZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
