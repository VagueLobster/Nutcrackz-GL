#pragma once

#include "VertexArray.hpp"

#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL = 1
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetClearColor(const rtmcpp::Vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void DrawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0, bool useBillboard = false) = 0;
		virtual void DrawMeshIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
		virtual void DrawGrid(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
		virtual void DrawParticles(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0, bool useBillboard = false) = 0;
		virtual void DrawLines(const RefPtr<VertexArray>& vertexArray, uint32_t vertexCount) = 0;

		virtual void SetLineWidth(float width) = 0;

		static API GetAPI() { return s_API; }
		static void SetAPI(API api) { s_API = api; }
		static std::unique_ptr<RendererAPI> Create();

	private:
		static API s_API;
	};

}
