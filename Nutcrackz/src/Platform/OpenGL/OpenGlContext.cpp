#include "nzpch.hpp"
#include "OpenGLContext.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Nutcrackz {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		NZ_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		//NZ_PROFILE_FUNCTION();

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		NZ_CORE_ASSERT(status, "Failed to initialize Glad!");

		NZ_CORE_INFO("OpenGL Info:");
		NZ_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
		NZ_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		NZ_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));

		NZ_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Nutcrackz requires at least OpenGL version 4.5!");
	}

	void OpenGLContext::SwapBuffers()
	{
		//NZ_PROFILE_FUNCTION();

		glfwSwapBuffers(m_WindowHandle);
	}

}