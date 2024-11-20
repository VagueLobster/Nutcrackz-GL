#pragma once

#include "Nutcrackz/Renderer/RendererAPI.hpp"

#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void SetClearColor(const rtmcpp::Vec4& color) override;
		virtual void Clear() override;

		virtual void DrawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0, bool useBillboard = false) override;
		virtual void DrawMeshIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		virtual void DrawGrid(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		virtual void DrawParticles(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0, bool useBillboard = false) override;
		virtual void DrawLines(const RefPtr<VertexArray>& vertexArray, uint32_t vertexCount) override;

		virtual void SetLineWidth(float width) override;
	};

}
