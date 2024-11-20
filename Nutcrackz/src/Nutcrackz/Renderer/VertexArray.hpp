#pragma once

#include "Nutcrackz/Renderer/Buffer.hpp"

namespace Nutcrackz {

	class VertexArray : public RefCounted
	{
	public:
		virtual ~VertexArray() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void AddVertexBuffer(const RefPtr<VertexBuffer>& vertexBuffer) = 0;
		virtual void SetIndexBuffer(const RefPtr<IndexBuffer>& indexBuffer) = 0;

		//virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const = 0;
		virtual const RefPtr<IndexBuffer>& GetIndexBuffer() const = 0;

		static RefPtr<VertexArray> Create();
	};

}