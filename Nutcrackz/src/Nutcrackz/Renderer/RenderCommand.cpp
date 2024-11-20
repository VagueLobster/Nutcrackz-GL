#include "nzpch.hpp"
#include "RenderCommand.hpp"

#include "Platform/OpenGL/OpenGLRendererAPI.hpp"

namespace Nutcrackz {

	std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();
	//Scope<RendererAPI> RenderCommand::s_RendererAPI = nullptr; // Fixes early crash in RenderCommand.cpp (#574)

}