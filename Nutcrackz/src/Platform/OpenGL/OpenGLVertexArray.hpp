#pragma once

#include "Nutcrackz/Renderer/VertexArray.hpp"

namespace Nutcrackz {

	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		virtual ~OpenGLVertexArray();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void AddVertexBuffer(const RefPtr<VertexBuffer>& vertexBuffer) override;
		virtual void SetIndexBuffer(const RefPtr<IndexBuffer>& indexBuffer) override;

		//virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const { return m_VertexBuffers; }
		virtual const RefPtr<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
	private:
		uint32_t m_RendererID;
		uint32_t m_VertexBufferIndex = 0;
		//std::vector<Ref<VertexBuffer>> m_VertexBuffers;
		RefPtr<IndexBuffer> m_IndexBuffer;
	};

}