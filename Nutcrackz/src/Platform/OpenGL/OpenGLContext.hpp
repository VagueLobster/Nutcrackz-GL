#pragma once

#include "Nutcrackz/Renderer/GraphicsContext.hpp"

struct GLFWwindow;

namespace Nutcrackz {

	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
	private:
		GLFWwindow* m_WindowHandle;
	};

}