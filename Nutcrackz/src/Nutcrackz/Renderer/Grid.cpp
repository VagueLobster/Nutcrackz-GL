#include "nzpch.hpp"
#include "Grid.hpp"

#include "VertexArray.hpp"
#include "Shader.hpp"
#include "RenderCommand.hpp"
#include "UniformBuffer.hpp"

#include "rtmcpp/Transforms.hpp"
#include "rtmcpp/PackedVector.hpp"
#include "rtmcpp/VectorOps.hpp"
#include "rtmcpp/Scalar.hpp"

namespace Nutcrackz {

	// Editor only!
	struct GridVertex
	{
		rtmcpp::PackedVec4 Position;

		int EntityID;

		float Near;
		float Far;

		rtmcpp::Mat4 View;
		rtmcpp::Mat4 Proj;
	};

	struct GridData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;

		RefPtr<VertexArray> GridVertexArray;
		RefPtr<VertexBuffer> GridVertexBuffer;
		RefPtr<Shader> GridShader;

		uint32_t GridIndexCount = 0;
		GridVertex* GridVertexBufferBase = nullptr;
		GridVertex* GridVertexBufferPtr = nullptr;

		rtmcpp::Vec4 GridVertexPositions[4];

		// Editor only!
		struct ViewProjData
		{
			rtmcpp::Mat4 View;
			rtmcpp::Mat4 Proj;
		};
		ViewProjData ViewProjBuffer;
	};

	static GridData s_GridData;


	void Grid::Init()
	{
		//NZ_PROFILE_FUNCTION();

		// Grid
		s_GridData.GridVertexArray = VertexArray::Create();

		s_GridData.GridVertexBuffer = VertexBuffer::Create(s_GridData.MaxVertices * sizeof(GridVertex));
		s_GridData.GridVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"    },
			{ ShaderDataType::Int,    "a_EntityID"    },
			{ ShaderDataType::Float,  "a_Near"        },
			{ ShaderDataType::Float,  "a_Far"         },
			{ ShaderDataType::Mat4,   "a_View"        },
			{ ShaderDataType::Mat4,   "a_Proj"        }
		});
		s_GridData.GridVertexArray->AddVertexBuffer(s_GridData.GridVertexBuffer);

		s_GridData.GridVertexBufferBase = new GridVertex[s_GridData.MaxVertices];

		uint32_t* quadIndices = new uint32_t[s_GridData.MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_GridData.MaxVertices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		RefPtr<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_GridData.MaxIndices);
		s_GridData.GridVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		s_GridData.GridShader = Shader::Create("assets/shaders/Renderer2D_Grid.glsl");

		s_GridData.GridVertexPositions[0] = { -1.0f, -1.0f, 0.0f, 1.0f };
		s_GridData.GridVertexPositions[1] = {  1.0f, -1.0f, 0.0f, 1.0f };
		s_GridData.GridVertexPositions[2] = {  1.0f,  1.0f, 0.0f, 1.0f };
		s_GridData.GridVertexPositions[3] = { -1.0f,  1.0f, 0.0f, 1.0f };
	}

	void Grid::Shutdown()
	{
		//NZ_PROFILE_FUNCTION();

		delete[] s_GridData.GridVertexBufferBase;
	}

	void Grid::BeginScene(const Camera& camera, const rtmcpp::Mat4& transform)
	{
		//NZ_PROFILE_FUNCTION();

		s_GridData.ViewProjBuffer.View = rtmcpp::Inverse(transform);
		s_GridData.ViewProjBuffer.Proj = camera.GetProjection();

		StartBatch();
	}

	void Grid::BeginScene(const EditorCamera& camera)
	{
		//NZ_PROFILE_FUNCTION();

		s_GridData.ViewProjBuffer.View = camera.GetViewMatrix();
		s_GridData.ViewProjBuffer.Proj = camera.GetProjection();

		StartBatch();
	}

	void Grid::EndScene()
	{
		//NZ_PROFILE_FUNCTION();

		Flush();
	}

	void Grid::StartBatch()
	{
		s_GridData.GridIndexCount = 0;
		s_GridData.GridVertexBufferPtr = s_GridData.GridVertexBufferBase;
	}

	void Grid::Flush()
	{
		// Draw Grid
		if (s_GridData.GridIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_GridData.GridVertexBufferPtr - (uint8_t*)s_GridData.GridVertexBufferBase);
			s_GridData.GridVertexBuffer->SetData(s_GridData.GridVertexBufferBase, dataSize);

			s_GridData.GridShader->Bind();
			RenderCommand::DrawGrid(s_GridData.GridVertexArray, s_GridData.GridIndexCount);
		}
	}

	void Grid::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Grid::DrawGrid()
	{
		//NZ_PROFILE_FUNCTION();
		rtmcpp::Vec3 Translation = { 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 Scale = { 1.0f, 1.0f, 1.0f };
		rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(Rotation));

		//rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ 0.0f, 0.0f, 0.0f }))
		//	* rotation
		//  * rtmcpp::Mat4Cast(rtmcpp::Scale(rtmcpp::Vec3{ 1.0f, 1.0f, 1.0f }));

		rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(Scale))
			* rotation
			* rtmcpp::Mat4Cast(rtmcpp::Translation(Translation));

		DrawGrid(transform);
	}

	void Grid::DrawGrid(float posX, float posY, float posZ)
	{
		rtmcpp::Vec3 Translation = { posX, posY, posZ };
		rtmcpp::Vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 Scale = { 1.0f, 1.0f, 1.0f };
		rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(Rotation));

		//rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ posX, posY, posZ }))
		//	* rotation
		//  * rtmcpp::Mat4Cast(rtmcpp::Scale(rtmcpp::Vec3{ 1.0f, 1.0f, 1.0f }));

		rtmcpp::Mat4 transform = rtmcpp::Mat4Cast(rtmcpp::Scale(Scale))
			* rotation
			* rtmcpp::Mat4Cast(rtmcpp::Translation(Translation));

		DrawGrid(transform);
	}

	void Grid::DrawGrid(const rtmcpp::Mat4& transform, int entityID)
	{
		//NZ_PROFILE_FUNCTION();

		for (size_t i = 0; i < 4; i++)
		{
			s_GridData.GridVertexBufferPtr->Position = s_GridData.GridVertexPositions[i] * transform;
			s_GridData.GridVertexBufferPtr->EntityID = entityID;
			s_GridData.GridVertexBufferPtr->Near = 0.1f;
			s_GridData.GridVertexBufferPtr->Far = 1000.0f;
			s_GridData.GridVertexBufferPtr->View = s_GridData.ViewProjBuffer.View;
			s_GridData.GridVertexBufferPtr->Proj = s_GridData.ViewProjBuffer.Proj;
			s_GridData.GridVertexBufferPtr++;
		}
		
		s_GridData.GridIndexCount += 6;
	}

}