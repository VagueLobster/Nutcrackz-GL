#pragma once

#include "RendererAPI.hpp"

namespace Nutcrackz {

	class RenderCommand
	{
	public:
		static void Init()
		{
			//s_RendererAPI = RendererAPI::Create(); // Fixes early crash in RenderCommand.cpp (#574)
			s_RendererAPI->Init();
		}

		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		static void SetClearColor(const rtmcpp::Vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		static void Clear()
		{
			s_RendererAPI->Clear();
		}

		static void DrawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0, bool useBillboard = false)
		{
			s_RendererAPI->DrawIndexed(vertexArray, indexCount, useBillboard);
		}

		static void DrawMeshIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawMeshIndexed(vertexArray, indexCount);
		}

		static void DrawGrid(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawGrid(vertexArray, indexCount);
		}

		static void DrawParticles(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount = 0, bool useBillboard = false)
		{
			s_RendererAPI->DrawParticles(vertexArray, indexCount, useBillboard);
		}

		static void DrawLines(const RefPtr<VertexArray>& vertexArray, uint32_t vertexCount = 0)
		{
			s_RendererAPI->DrawLines(vertexArray, vertexCount);
		}

		static void SetLineWidth(float width)
		{
			s_RendererAPI->SetLineWidth(width);
		}

	public:
		static std::unique_ptr<RendererAPI> s_RendererAPI;
	};

}
