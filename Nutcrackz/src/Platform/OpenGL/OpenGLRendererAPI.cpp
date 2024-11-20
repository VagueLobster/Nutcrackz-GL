#include "nzpch.hpp"
#include "OpenGLRendererAPI.hpp"

#include <glad/glad.h>

namespace Nutcrackz {

	void OpenGLMessageCallback(unsigned source, unsigned type, unsigned id, unsigned severity, int length, const char* message, const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH: NZ_CORE_CRITICAL(message); return;
		case GL_DEBUG_SEVERITY_MEDIUM: NZ_CORE_ERROR(message); return;
		case GL_DEBUG_SEVERITY_LOW: NZ_CORE_WARN(message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: NZ_CORE_TRACE(message); return;
		}

		NZ_CORE_ASSERT(false, "Unknown severity level!");
	}

	void OpenGLRendererAPI::Init()
	{
		//NZ_PROFILE_FUNCTION();

#ifdef NZ_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif
		glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_GEQUAL);
	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::SetClearColor(const rtmcpp::Vec4& color)
	{
		glClearColor(color.X, color.Y, color.Z, color.W);
		glClearDepthf(0.0f);
	}

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount, bool useBillboard)
	{
		NZ_PROFILE_GPU_SCOPE("OpenGLRendererAPI::DrawIndexed");

		if (useBillboard)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}

		vertexArray->Bind();
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);

		if (useBillboard)
		{
			glDisable(GL_CULL_FACE);
		}
	}

	void OpenGLRendererAPI::DrawMeshIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount)
	{
		NZ_PROFILE_GPU_SCOPE("OpenGLRendererAPI::DrawMeshIndexed");

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		vertexArray->Bind();
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);

		glDisable(GL_CULL_FACE);
	}

	void OpenGLRendererAPI::DrawGrid(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount)
	{
		NZ_PROFILE_GPU_SCOPE("OpenGLRendererAPI::DrawGrid");

		vertexArray->Bind();
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawParticles(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount, bool useBillboard)
	{
		NZ_PROFILE_GPU_SCOPE("OpenGLRendererAPI::DrawParticles");

		if (useBillboard)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		vertexArray->Bind();
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (useBillboard)
		{
			glDisable(GL_CULL_FACE);
		}
	}

	void OpenGLRendererAPI::DrawLines(const RefPtr<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		NZ_PROFILE_GPU_SCOPE("OpenGLRendererAPI::DrawLines");

		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void OpenGLRendererAPI::SetLineWidth(float width)
	{
		NZ_PROFILE_GPU_SCOPE("OpenGLRendererAPI::SetLineWidth");

		glLineWidth(width);
	}

}