#include "nzpch.hpp"
#include "Renderer3D.hpp"

#include "VertexArray.hpp"
#include "Shader.hpp"
#include "RenderCommand.hpp"
#include "UniformBuffer.hpp"

#include "Nutcrackz/Math/Math.hpp"

#include "rtmcpp/PackedVector.hpp"
#include "rtmcpp/Transforms.hpp"
#include "rtmcpp/Scalar.hpp"

namespace Nutcrackz {

	const size_t Renderer3D::s_PyramidSize = 18;
	const size_t Renderer3D::s_TriangleSize = 12;
	const size_t Renderer3D::s_CubeSize = 24;

	const uint32_t Renderer3D::s_PyramidIndicesSize = 18;
	const uint32_t Renderer3D::s_TriangleIndicesSize = 12;
	const uint32_t Renderer3D::s_CubeIndicesSize = 36;

	bool Renderer3D::s_AnimateInRuntime = false;
	bool Renderer3D::s_AnimateInEdit = false;

	int Renderer3D::tileIndexX = 0;
	int Renderer3D::tileIndexY = 0;

	rtmcpp::Vec2 Renderer3D::m_ScrollingPlusDivision = { 0.0f, 0.0f };

	struct PyramidVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Normal;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;

		rtmcpp::PackedVec3 MaterialSpecular;
		float MaterialShininess;

		rtmcpp::PackedVec3 ViewPos;

		// Editor-only
		int EntityID;
	};

	struct TriangularPrismVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Normal;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;

		rtmcpp::PackedVec3 MaterialSpecular;
		float MaterialShininess;

		rtmcpp::PackedVec3 ViewPos;

		// Editor-only
		int EntityID;
	};

	struct CubeVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Normal;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;

		rtmcpp::PackedVec3 MaterialSpecular;
		float MaterialShininess;

		rtmcpp::PackedVec3 ViewPos;

		// Editor-only
		int EntityID;
	};

	struct PlaneVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Normal;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;

		rtmcpp::PackedVec3 MaterialSpecular;
		float MaterialShininess;

		rtmcpp::PackedVec3 ViewPos;

		// Editor-only
		int EntityID;
	};

	struct MeshVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Normal;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;

		rtmcpp::PackedVec3 MaterialSpecular;
		float MaterialShininess;

		rtmcpp::PackedVec3 ViewPos;

		// Editor-only
		int EntityID;
	};

	struct Renderer3DData
	{
		static const uint32_t MaxPyramids = 20000;
		static const uint32_t MaxPyramidVertices = MaxPyramids * 12;
		static const uint32_t MaxPyramidIndices = MaxPyramids * 18;

		static const uint32_t MaxTriangles = 20000;
		static const uint32_t MaxTriangleVertices = MaxTriangles * 6;
		static const uint32_t MaxTriangleIndices = MaxTriangles * 9;

		static const uint32_t MaxCubes = 20000;
		static const uint32_t MaxCubeVertices = MaxCubes * 4;
		static const uint32_t MaxCubeIndices = MaxCubes * 6;
		
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxQuadVertices = MaxQuads * 4;
		static const uint32_t MaxQuadIndices = MaxQuads * 6;

		static const uint32_t MaxMeshes = 2000000;
		static const uint32_t MaxMeshVertices = MaxMeshes * 3;
		static const uint32_t MaxMeshIndices = MaxMeshes * 3;

		static const uint32_t MaxSkyboxes = 10000;
		static const uint32_t MaxSkyboxVertices = MaxSkyboxes * 24;
		static const uint32_t MaxSkyboxIndices = MaxSkyboxes * 36;

		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		RefPtr<VertexArray> PyramidVertexArray;
		RefPtr<VertexBuffer> PyramidVertexBuffer;

		RefPtr<VertexArray> TriangleVertexArray;
		RefPtr<VertexBuffer> TriangleVertexBuffer;

		RefPtr<VertexArray> CubeVertexArray;
		RefPtr<VertexBuffer> CubeVertexBuffer;
		RefPtr<Shader> CubeShader;

		RefPtr<VertexArray> PlaneVertexArray;
		RefPtr<VertexBuffer> PlaneVertexBuffer;

		RefPtr<VertexArray> MeshVertexArray;
		RefPtr<VertexBuffer> MeshVertexBuffer;

		RefPtr<Texture2D> WhiteTexture;

		uint32_t PyramidIndexCount = 0;
		PyramidVertex* PyramidVertexBufferBase = nullptr;
		PyramidVertex* PyramidVertexBufferPtr = nullptr;

		uint32_t TriangleIndexCount = 0;
		TriangularPrismVertex* TriangleVertexBufferBase = nullptr;
		TriangularPrismVertex* TriangleVertexBufferPtr = nullptr;

		uint32_t CubeIndexCount = 0;
		CubeVertex* CubeVertexBufferBase = nullptr;
		CubeVertex* CubeVertexBufferPtr = nullptr;

		uint32_t PlaneIndexCount = 0;
		PlaneVertex* PlaneVertexBufferBase = nullptr;
		PlaneVertex* PlaneVertexBufferPtr = nullptr;

		uint32_t MeshIndexCount = 0;
		MeshVertex* MeshVertexBufferBase = nullptr;
		MeshVertex* MeshVertexBufferPtr = nullptr;

		std::array<RefPtr<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		rtmcpp::Vec4 PyramidVertexPositions[18];
		rtmcpp::Vec4 PyramidVertexNormals[18];

		rtmcpp::Vec4 TriangleVertexPositions[12];
		rtmcpp::Vec4 TriangleVertexNormals[12];

		rtmcpp::Vec4 CubeVertexPositions[36];
		rtmcpp::Vec4 CubeVertexNormals[36];

		rtmcpp::Vec4 PlaneVertexPositions[4];
		rtmcpp::Vec4 PlaneVertexNormals[4];

		uint32_t MeshVerticesSize = 0;
		uint32_t MeshIndicesSize = 0;

		Renderer3D::Statistics3D Stats;

		struct CameraData
		{
			rtmcpp::Mat4 ViewProjection;
		};

		CameraData CameraBuffer;
		RefPtr<UniformBuffer> CameraUniformBuffer;

		// Editor only!
		struct ViewProjData
		{
			rtmcpp::Mat4 View;
			rtmcpp::Mat4 Proj;
		};
		ViewProjData ViewProjBuffer;

		struct CamPosData
		{
			rtmcpp::Vec3 CamPosition;
		};
		CamPosData CamPosBuffer;
	};

	static Renderer3DData s_Data;

	void Renderer3D::Init()
	{
		//NZ_PROFILE_FUNCTION();

		// Pyramids
		s_Data.PyramidVertexArray = VertexArray::Create();

		s_Data.PyramidVertexBuffer = VertexBuffer::Create(s_Data.MaxPyramidVertices * sizeof(PyramidVertex));
		s_Data.PyramidVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"        },
			{ ShaderDataType::Float4, "a_Normal"          },
			{ ShaderDataType::Float4, "a_Color"           },
			{ ShaderDataType::Float2, "a_TexCoord"        },
			{ ShaderDataType::Float2, "a_TilingFactor"    },
			{ ShaderDataType::Int,    "a_TexIndex"        },
			{ ShaderDataType::Float3, "a_MatSpecular"     },
			{ ShaderDataType::Float,  "a_MatShininess"    },
			{ ShaderDataType::Float3, "a_ViewPos"         },
			{ ShaderDataType::Int,    "a_EntityID"        }
		});

		s_Data.PyramidVertexArray->AddVertexBuffer(s_Data.PyramidVertexBuffer);

		s_Data.PyramidVertexBufferBase = new PyramidVertex[s_Data.MaxPyramidVertices];

		uint32_t* pyramidIndices = new uint32_t[s_Data.MaxPyramidIndices];

		uint32_t pyramidOffset = 0;
		for (uint32_t i = 0; i < s_Data.MaxPyramidIndices; i += 3)
		{
			pyramidIndices[i + 0] = pyramidOffset + 0;
			pyramidIndices[i + 1] = pyramidOffset + 1;
			pyramidIndices[i + 2] = pyramidOffset + 2;

			pyramidOffset += 3;
		}

		RefPtr<IndexBuffer> pyramidIB = IndexBuffer::Create(pyramidIndices, s_Data.MaxPyramidIndices);
		s_Data.PyramidVertexArray->SetIndexBuffer(pyramidIB);
		delete[] pyramidIndices;

		// Triangles
		s_Data.TriangleVertexArray = VertexArray::Create();

		s_Data.TriangleVertexBuffer = VertexBuffer::Create(s_Data.MaxTriangleVertices * sizeof(TriangularPrismVertex));
		s_Data.TriangleVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"        },
			{ ShaderDataType::Float4, "a_Normal"          },
			{ ShaderDataType::Float4, "a_Color"           },
			{ ShaderDataType::Float2, "a_TexCoord"        },
			{ ShaderDataType::Float2, "a_TilingFactor"    },
			{ ShaderDataType::Int,    "a_TexIndex"        },
			{ ShaderDataType::Float3, "a_MatSpecular"     },
			{ ShaderDataType::Float,  "a_MatShininess"    },
			{ ShaderDataType::Float3, "a_ViewPos"         },
			{ ShaderDataType::Int,    "a_EntityID"        }
		});
		s_Data.TriangleVertexArray->AddVertexBuffer(s_Data.TriangleVertexBuffer);
		s_Data.TriangleVertexBufferBase = new TriangularPrismVertex[s_Data.MaxTriangleVertices];

		uint32_t* triangleIndices = new uint32_t[s_Data.MaxTriangleIndices];

		uint32_t triangleOffset = 0;
		for (uint32_t i = 0; i < s_Data.MaxTriangleIndices; i += 3)
		{
			triangleIndices[i + 0] = triangleOffset + 0;
			triangleIndices[i + 1] = triangleOffset + 1;
			triangleIndices[i + 2] = triangleOffset + 2;

			triangleOffset += 3;
		}

		RefPtr<IndexBuffer> triangleIB = IndexBuffer::Create(triangleIndices, s_Data.MaxTriangleIndices);
		s_Data.TriangleVertexArray->SetIndexBuffer(triangleIB);
		delete[] triangleIndices;
		
		// Cubes
		s_Data.CubeVertexArray = VertexArray::Create();

		s_Data.CubeVertexBuffer = VertexBuffer::Create(s_Data.MaxCubeVertices * sizeof(CubeVertex));
		s_Data.CubeVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"        },
			{ ShaderDataType::Float4, "a_Normal"          },
			{ ShaderDataType::Float4, "a_Color"           },
			{ ShaderDataType::Float2, "a_TexCoord"        },
			{ ShaderDataType::Float2, "a_TilingFactor"    },
			{ ShaderDataType::Int,    "a_TexIndex"        },
			{ ShaderDataType::Float3, "a_MatSpecular"     },
			{ ShaderDataType::Float,  "a_MatShininess"    },
			{ ShaderDataType::Float3, "a_ViewPos"         },
			{ ShaderDataType::Int,    "a_EntityID"        }
		});
		s_Data.CubeVertexArray->AddVertexBuffer(s_Data.CubeVertexBuffer);

		s_Data.CubeVertexBufferBase = new CubeVertex[s_Data.MaxCubeVertices];

		uint32_t* cubeIndices = new uint32_t[s_Data.MaxCubeIndices];

		uint32_t cubeOffset = 0;
		for (uint32_t i = 0; i < s_Data.MaxCubeIndices; i += 6)
		{
			cubeIndices[i + 0] = cubeOffset + 0;
			cubeIndices[i + 1] = cubeOffset + 1;
			cubeIndices[i + 2] = cubeOffset + 2;

			cubeIndices[i + 3] = cubeOffset + 2;
			cubeIndices[i + 4] = cubeOffset + 3;
			cubeIndices[i + 5] = cubeOffset + 0;

			cubeOffset += 4;
		}

		RefPtr<IndexBuffer> cubeIB = IndexBuffer::Create(cubeIndices, s_Data.MaxCubeIndices);
		s_Data.CubeVertexArray->SetIndexBuffer(cubeIB);
		delete[] cubeIndices;

		// Planes
		s_Data.PlaneVertexArray = VertexArray::Create();

		s_Data.PlaneVertexBuffer = VertexBuffer::Create(s_Data.MaxQuadVertices * sizeof(PlaneVertex));
		s_Data.PlaneVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"        },
			{ ShaderDataType::Float4, "a_Normal"          },
			{ ShaderDataType::Float4, "a_Color"           },
			{ ShaderDataType::Float2, "a_TexCoord"        },
			{ ShaderDataType::Float2, "a_TilingFactor"    },
			{ ShaderDataType::Int,    "a_TexIndex"        },
			{ ShaderDataType::Float3, "a_MatSpecular"     },
			{ ShaderDataType::Float,  "a_MatShininess"    },
			{ ShaderDataType::Float3, "a_ViewPos"         },
			{ ShaderDataType::Int,    "a_EntityID"        }
		});

		s_Data.PlaneVertexArray->AddVertexBuffer(s_Data.PlaneVertexBuffer);

		s_Data.PlaneVertexBufferBase = new PlaneVertex[s_Data.MaxQuadVertices];

		uint32_t* planeIndices = new uint32_t[s_Data.MaxQuadIndices];

		uint32_t planeOffset = 0;
		for (uint32_t i = 0; i < s_Data.MaxQuadIndices; i += 6)
		{
			planeIndices[i + 0] = planeOffset + 0;
			planeIndices[i + 1] = planeOffset + 1;
			planeIndices[i + 2] = planeOffset + 2;

			planeIndices[i + 3] = planeOffset + 2;
			planeIndices[i + 4] = planeOffset + 3;
			planeIndices[i + 5] = planeOffset + 0;

			planeOffset += 4;
		}

		RefPtr<IndexBuffer> planeIB = IndexBuffer::Create(planeIndices, s_Data.MaxQuadIndices);
		s_Data.PlaneVertexArray->SetIndexBuffer(planeIB);
		delete[] planeIndices;

		// Meshes
		s_Data.MeshVertexArray = VertexArray::Create();

		s_Data.MeshVertexBuffer = VertexBuffer::Create(s_Data.MaxMeshVertices * sizeof(MeshVertex));
		s_Data.MeshVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"        },
			{ ShaderDataType::Float4, "a_Normal"          },
			{ ShaderDataType::Float4, "a_Color"           },
			{ ShaderDataType::Float2, "a_TexCoord"        },
			{ ShaderDataType::Float2, "a_TilingFactor"    },
			{ ShaderDataType::Int,    "a_TexIndex"        },
			{ ShaderDataType::Float3, "a_MatSpecular"     },
			{ ShaderDataType::Float,  "a_MatShininess"    },
			{ ShaderDataType::Float3, "a_ViewPos"         },
			{ ShaderDataType::Int,    "a_EntityID"        }
		});
		s_Data.MeshVertexArray->AddVertexBuffer(s_Data.MeshVertexBuffer);

		s_Data.MeshVertexBufferBase = new MeshVertex[s_Data.MaxMeshVertices];

		uint32_t* meshIndices = new uint32_t[s_Data.MaxMeshIndices];

		uint32_t meshOffset = 0;
		for (uint32_t i = 0; i < s_Data.MaxMeshIndices; i += 3)
		{
			meshIndices[i + 0] = meshOffset + 0;
			meshIndices[i + 1] = meshOffset + 1;
			meshIndices[i + 2] = meshOffset + 2;

			meshOffset += 3;
		}

		RefPtr<IndexBuffer> meshIB = IndexBuffer::Create(meshIndices, s_Data.MaxMeshIndices);
		s_Data.MeshVertexArray->SetIndexBuffer(meshIB);
		delete[] meshIndices;

		s_Data.WhiteTexture = Texture2D::Create(TextureSpecification());
		s_Data.WhiteTexture->SetLinear(true);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(Buffer(&whiteTextureData, sizeof(uint32_t)));

		s_Data.CubeShader = Shader::Create("assets/shaders/Renderer3D_Primitive.glsl");
		
		// Set first texture slot to 0
		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		// 3D Pyramid Vertex Positions + Normals
		{
			// Positions Bottom side (all 6 form a quad)
			s_Data.PyramidVertexPositions[ 0] = { -0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.PyramidVertexPositions[ 1] = {  0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.PyramidVertexPositions[ 2] = {  0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.PyramidVertexPositions[ 3] = {  0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.PyramidVertexPositions[ 4] = { -0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.PyramidVertexPositions[ 5] = { -0.5f, -0.5f, -0.5f, 1.0f };

			// Positions Front side
			s_Data.PyramidVertexPositions[ 6] = { -0.5f, -0.5f, 0.5f, 1.0f };
			s_Data.PyramidVertexPositions[ 7] = {  0.5f, -0.5f, 0.5f, 1.0f };
			s_Data.PyramidVertexPositions[ 8] = {  0.0f,  0.5f, 0.0f, 1.0f };

			// Positions Right side
			s_Data.PyramidVertexPositions[ 9] = { 0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.PyramidVertexPositions[10] = { 0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.PyramidVertexPositions[11] = { 0.0f,  0.5f,  0.0f, 1.0f };

			// Positions Back side
			s_Data.PyramidVertexPositions[12] = {  0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.PyramidVertexPositions[13] = { -0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.PyramidVertexPositions[14] = {  0.0f,  0.5f,  0.0f, 1.0f };

			// Positions Left side
			s_Data.PyramidVertexPositions[15] = { -0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.PyramidVertexPositions[16] = { -0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.PyramidVertexPositions[17] = {  0.0f,  0.5f,  0.0f, 1.0f };

			// Normals Bottom side
			s_Data.PyramidVertexNormals[ 0] = { 0.0f, -1.0f, 0.0f, 1.0f };
			s_Data.PyramidVertexNormals[ 1] = { 0.0f, -1.0f, 0.0f, 1.0f };
			s_Data.PyramidVertexNormals[ 2] = { 0.0f, -1.0f, 0.0f, 1.0f };
			s_Data.PyramidVertexNormals[ 3] = { 0.0f, -1.0f, 0.0f, 1.0f };
			s_Data.PyramidVertexNormals[ 4] = { 0.0f, -1.0f, 0.0f, 1.0f };
			s_Data.PyramidVertexNormals[ 5] = { 0.0f, -1.0f, 0.0f, 1.0f };

			// Normals Front side
			s_Data.PyramidVertexNormals[ 6] = { 0.0f, 0.0f, 1.0f, 1.0f };
			s_Data.PyramidVertexNormals[ 7] = { 0.0f, 0.0f, 1.0f, 1.0f };
			s_Data.PyramidVertexNormals[ 8] = { 0.0f, 0.0f, 1.0f, 1.0f };

			// Normals Right side
			s_Data.PyramidVertexNormals[ 9] = { 1.0f, 0.0f, 0.0f, 1.0f };
			s_Data.PyramidVertexNormals[10] = { 1.0f, 0.0f, 0.0f, 1.0f };
			s_Data.PyramidVertexNormals[11] = { 1.0f, 0.0f, 0.0f, 1.0f };

			// Normals Back side
			s_Data.PyramidVertexNormals[12] = { 0.0f, 0.0f, -1.0f, 1.0f };
			s_Data.PyramidVertexNormals[13] = { 0.0f, 0.0f, -1.0f, 1.0f };
			s_Data.PyramidVertexNormals[14] = { 0.0f, 0.0f, -1.0f, 1.0f };

			// Normals Left side
			s_Data.PyramidVertexNormals[15] = { -1.0f, 0.0f, 0.0f, 1.0f };
			s_Data.PyramidVertexNormals[16] = { -1.0f, 0.0f, 0.0f, 1.0f };
			s_Data.PyramidVertexNormals[17] = { -1.0f, 0.0f, 0.0f, 1.0f };
		}

		// 3D Triangular Prism Vertex Positions + Normals
		{
			// Positions Bottom side
			s_Data.TriangleVertexPositions[ 0] = {  0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.TriangleVertexPositions[ 1] = { -0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.TriangleVertexPositions[ 2] = {  0.0f, -0.5f, -0.5f, 1.0f };

			// Positions Front side
			s_Data.TriangleVertexPositions[ 3] = { -0.5f, -0.5f, 0.5f, 1.0f };
			s_Data.TriangleVertexPositions[ 4] = {  0.5f, -0.5f, 0.5f, 1.0f };
			s_Data.TriangleVertexPositions[ 5] = {  0.0f,  0.5f, 0.0f, 1.0f };

			// Positions Right side
			s_Data.TriangleVertexPositions[ 6] = { 0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.TriangleVertexPositions[ 7] = { 0.0f, -0.5f, -0.5f, 1.0f };
			s_Data.TriangleVertexPositions[ 8] = { 0.0f,  0.5f,  0.0f, 1.0f };
			
			// Positions Left side
			s_Data.TriangleVertexPositions[ 9] = {  0.0f, -0.5f, -0.5f, 1.0f };
			s_Data.TriangleVertexPositions[10] = { -0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.TriangleVertexPositions[11] = {  0.0f,  0.5f,  0.0f, 1.0f };

			// Bottom side
			s_Data.TriangleVertexNormals[ 0] = { 0.0f, -1.0f, 0.0f, 1.0f };
			s_Data.TriangleVertexNormals[ 1] = { 0.0f, -1.0f, 0.0f, 1.0f };
			s_Data.TriangleVertexNormals[ 2] = { 0.0f, -1.0f, 0.0f, 1.0f };

			// Normals Front side
			s_Data.TriangleVertexNormals[ 3] = { 0.0f, 0.0f, 1.0f, 1.0f };
			s_Data.TriangleVertexNormals[ 4] = { 0.0f, 0.0f, 1.0f, 1.0f };
			s_Data.TriangleVertexNormals[ 5] = { 0.0f, 0.0f, 1.0f, 1.0f };

			// Normals Right side
			s_Data.TriangleVertexNormals[ 6] = { 1.0f, 0.0f, -1.0f, 1.0f };
			s_Data.TriangleVertexNormals[ 7] = { 1.0f, 0.0f, -1.0f, 1.0f };
			s_Data.TriangleVertexNormals[ 8] = { 1.0f, 0.0f, -1.0f, 1.0f };

			// Normals Left side
			s_Data.TriangleVertexNormals[ 9] = { -1.0f, 0.0f, -1.0f, 1.0f };
			s_Data.TriangleVertexNormals[10] = { -1.0f, 0.0f, -1.0f, 1.0f };
			s_Data.TriangleVertexNormals[11] = { -1.0f, 0.0f, -1.0f, 1.0f };
		}

		// 3D Cube Vertex Positions + Normals
		{
			// Positions Front side
			s_Data.CubeVertexPositions[ 0] = { -0.5f, -0.5f, 0.5f, 1.0f };
			s_Data.CubeVertexPositions[ 1] = {  0.5f, -0.5f, 0.5f, 1.0f };
			s_Data.CubeVertexPositions[ 2] = {  0.5f,  0.5f, 0.5f, 1.0f };
			s_Data.CubeVertexPositions[ 3] = { -0.5f,  0.5f, 0.5f, 1.0f };

			// Positions Right side
			s_Data.CubeVertexPositions[ 4] = { 0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.CubeVertexPositions[ 5] = { 0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.CubeVertexPositions[ 6] = { 0.5f,  0.5f, -0.5f, 1.0f };
			s_Data.CubeVertexPositions[ 7] = { 0.5f,  0.5f,  0.5f, 1.0f };

			// Positions Back side
			s_Data.CubeVertexPositions[ 8] = {  0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.CubeVertexPositions[ 9] = { -0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.CubeVertexPositions[10] = { -0.5f,  0.5f, -0.5f, 1.0f };
			s_Data.CubeVertexPositions[11] = {  0.5f,  0.5f, -0.5f, 1.0f };

			// Positions Left side
			s_Data.CubeVertexPositions[12] = { -0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.CubeVertexPositions[13] = { -0.5f,  0.5f,  0.5f, 1.0f };
			s_Data.CubeVertexPositions[14] = { -0.5f,  0.5f, -0.5f, 1.0f };
			s_Data.CubeVertexPositions[15] = { -0.5f, -0.5f, -0.5f, 1.0f };

			// Positions Top side
			s_Data.CubeVertexPositions[16] = {  0.5f, 0.5f, -0.5f, 1.0f };
			s_Data.CubeVertexPositions[17] = { -0.5f, 0.5f, -0.5f, 1.0f };
			s_Data.CubeVertexPositions[18] = { -0.5f, 0.5f,  0.5f, 1.0f };
			s_Data.CubeVertexPositions[19] = {  0.5f, 0.5f,  0.5f, 1.0f };

			// Positions Bottom side
			s_Data.CubeVertexPositions[20] = { -0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.CubeVertexPositions[21] = {  0.5f, -0.5f, -0.5f, 1.0f };
			s_Data.CubeVertexPositions[22] = {  0.5f, -0.5f,  0.5f, 1.0f };
			s_Data.CubeVertexPositions[23] = { -0.5f, -0.5f,  0.5f, 1.0f };

			// Normals Front side
			s_Data.CubeVertexNormals[ 0] = { 0.0f, 0.0f, 1.0f, 1.0f };
			s_Data.CubeVertexNormals[ 1] = { 0.0f, 0.0f, 1.0f, 1.0f };
			s_Data.CubeVertexNormals[ 2] = { 0.0f, 0.0f, 1.0f, 1.0f };
			s_Data.CubeVertexNormals[ 3] = { 0.0f, 0.0f, 1.0f, 1.0f };

			// Normals Right side
			s_Data.CubeVertexNormals[ 4] = { 1.0f, 0.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[ 5] = { 1.0f, 0.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[ 6] = { 1.0f, 0.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[ 7] = { 1.0f, 0.0f, 0.0f, 1.0f };

			// Normals Back side
			s_Data.CubeVertexNormals[ 8] = { 0.0f, 0.0f, -1.0f, 1.0f };
			s_Data.CubeVertexNormals[ 9] = { 0.0f, 0.0f, -1.0f, 1.0f };
			s_Data.CubeVertexNormals[10] = { 0.0f, 0.0f, -1.0f, 1.0f };
			s_Data.CubeVertexNormals[11] = { 0.0f, 0.0f, -1.0f, 1.0f };

			// Normals Left side
			s_Data.CubeVertexNormals[12] = { -1.0f, 0.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[13] = { -1.0f, 0.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[14] = { -1.0f, 0.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[15] = { -1.0f, 0.0f, 0.0f, 1.0f };

			// Normals Top side
			s_Data.CubeVertexNormals[16] = { 0.0f, 1.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[17] = { 0.0f, 1.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[18] = { 0.0f, 1.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[19] = { 0.0f, 1.0f, 0.0f, 1.0f };

			// Normals Bottom side
			s_Data.CubeVertexNormals[20] = { 0.0f, -1.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[21] = { 0.0f, -1.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[22] = { 0.0f, -1.0f, 0.0f, 1.0f };
			s_Data.CubeVertexNormals[23] = { 0.0f, -1.0f, 0.0f, 1.0f };
		}

		// 3D Plane Vertex Positions + Normals
		{
			// Positions
			s_Data.PlaneVertexPositions[0] = {  0.5f, 0.0f, -0.5f, 1.0f };
			s_Data.PlaneVertexPositions[1] = { -0.5f, 0.0f, -0.5f, 1.0f };
			s_Data.PlaneVertexPositions[2] = { -0.5f, 0.0f,  0.5f, 1.0f };
			s_Data.PlaneVertexPositions[3] = {  0.5f, 0.0f,  0.5f, 1.0f };

			// Normals
			s_Data.PlaneVertexNormals[0] = { 0.0f, 1.0f, 0.0f, 1.0f };
			s_Data.PlaneVertexNormals[1] = { 0.0f, 1.0f, 0.0f, 1.0f };
			s_Data.PlaneVertexNormals[2] = { 0.0f, 1.0f, 0.0f, 1.0f };
			s_Data.PlaneVertexNormals[3] = { 0.0f, 1.0f, 0.0f, 1.0f };
		}

		s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::CameraData), 0);
	}

	void Renderer3D::Shutdown()
	{
		//NZ_PROFILE_FUNCTION();

		delete[] s_Data.PyramidVertexBufferBase;
		delete[] s_Data.TriangleVertexBufferBase;
		delete[] s_Data.CubeVertexBufferBase;
		delete[] s_Data.PlaneVertexBufferBase;
		delete[] s_Data.MeshVertexBufferBase;
	}

	rtmcpp::Mat4 LookAtRH(const rtmcpp::Vec3& eye, const rtmcpp::Vec3& center, const rtmcpp::Vec3& up)
	{
		const rtmcpp::Vec3 f = rtmcpp::Vec3{ rtm::vector_sub(rtm::vector_normalize3(center.Value), rtm::vector_normalize3(eye.Value)) };// normalize(center - eye));
		const rtmcpp::Vec3 s(rtm::vector_normalize3(rtm::vector_cross3(rtmcpp::Vec4(f.X, f.Y, f.Z, 1.0f).Value, rtmcpp::Vec4(up.X, up.Y, up.Z, 1.0f).Value)));//  normalize(glm::cross(f, up)));
		const rtmcpp::Vec3 u(rtm::vector_cross3(rtmcpp::Vec4(s.X, s.Y, s.Z, 1.0f).Value, rtmcpp::Vec4(f.X, f.Y, f.Z, 1.0f).Value));

		//rtmcpp::Mat4 Result(1);
		rtmcpp::Mat4 result;

		result.Value.x_axis = rtm::vector_set(s.X, u.X, -f.X, 1.0f);
		result.Value.y_axis = rtm::vector_set(s.Y, u.Y, -f.Y, 1.0f);
		result.Value.z_axis = rtm::vector_set(s.Z, u.Z, -f.Z, 1.0f);
		result.Value.w_axis = rtm::vector_set(-s.Dot(eye), -u.Dot(eye), f.Dot(eye), 1.0f);

		//Result[0][0] = s.x;
		//Result[1][0] = s.y;
		//Result[2][0] = s.z;
		//Result[0][1] = u.x;
		//Result[1][1] = u.y;
		//Result[2][1] = u.z;
		//Result[0][2] = -f.x;
		//Result[1][2] = -f.y;
		//Result[2][2] = -f.z;
		//Result[3][0] = -glm::dot(s, eye);
		//Result[3][1] = -dot(u, eye);
		//Result[3][2] = dot(f, eye);
		return result;
	}

	void Renderer3D::BeginScene(const Camera& camera, const rtmcpp::Mat4& transform)
	{
		//NZ_PROFILE_FUNCTION();

		s_Data.CamPosBuffer.CamPosition = transform.Value.w_axis;

		s_Data.CameraBuffer.ViewProjection = rtmcpp::Inverse(transform) * camera.GetProjection();
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

		rtmcpp::Vec3 cameraPosition = rtmcpp::Inverse(transform).Value.w_axis;
		s_Data.ViewProjBuffer.View = rtmcpp::Mat4(LookAtRH(cameraPosition, rtmcpp::Vec3{ cameraPosition.X + 0.0f, cameraPosition.Y + 0.0f, cameraPosition.Z + -1.0f }, rtmcpp::Vec3(0.0f, 1.0f, 0.0f)));
		s_Data.ViewProjBuffer.Proj = camera.GetProjection();

		StartBatch();
	}

	void Renderer3D::BeginScene(const EditorCamera& camera)
	{
		//NZ_PROFILE_FUNCTION();

		//s_Data.CamPosBuffer.CamPosition = camera.GetPosition();
		
		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

		//s_Data.ViewProjBuffer.View = rtmcpp::Mat4(LookAtRH(camera.GetPosition(), rtmcpp::Vec3{ camera.GetPosition().X + camera.GetForwardDirection().X, camera.GetPosition().Y + camera.GetForwardDirection().Y, camera.GetPosition().Z + camera.GetForwardDirection().Z }, camera.GetUpDirection()));
		//s_Data.ViewProjBuffer.Proj = rtmcpp::Mat4::PerspectiveInfReversedZ(rtmcpp::Radians(camera.GetFOV()), camera.GetWidth() / camera.GetHeight(), camera.GetNearClip());

		StartBatch();
	}

	void Renderer3D::EndScene()
	{
		//NZ_PROFILE_FUNCTION();

		Flush();
	}

	void Renderer3D::StartBatch()
	{
		s_Data.PyramidIndexCount = 0;
		s_Data.PyramidVertexBufferPtr = s_Data.PyramidVertexBufferBase;

		s_Data.TriangleIndexCount = 0;
		s_Data.TriangleVertexBufferPtr = s_Data.TriangleVertexBufferBase;

		s_Data.CubeIndexCount = 0;
		s_Data.CubeVertexBufferPtr = s_Data.CubeVertexBufferBase;

		s_Data.PlaneIndexCount = 0;
		s_Data.PlaneVertexBufferPtr = s_Data.PlaneVertexBufferBase;

		s_Data.MeshIndexCount = 0;
		s_Data.MeshVertexBufferPtr = s_Data.MeshVertexBufferBase;

		s_Data.MeshVerticesSize = 0;
		s_Data.MeshIndicesSize = 0;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer3D::Flush()
	{
		// Draw pyramids
		if (s_Data.PyramidIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.PyramidVertexBufferPtr - (uint8_t*)s_Data.PyramidVertexBufferBase);
			s_Data.PyramidVertexBuffer->SetData(s_Data.PyramidVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			{
				if (s_Data.TextureSlots[i])
					s_Data.TextureSlots[i]->Bind(i);
			}

			s_Data.CubeShader->Bind();
			RenderCommand::DrawMeshIndexed(s_Data.PyramidVertexArray, s_Data.PyramidIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		// Draw triangles
		if (s_Data.TriangleIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.TriangleVertexBufferPtr - (uint8_t*)s_Data.TriangleVertexBufferBase);
			s_Data.TriangleVertexBuffer->SetData(s_Data.TriangleVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			{
				if (s_Data.TextureSlots[i])
					s_Data.TextureSlots[i]->Bind(i);
			}

			s_Data.CubeShader->Bind();
			RenderCommand::DrawMeshIndexed(s_Data.TriangleVertexArray, s_Data.TriangleIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		// Draw cubes
		if (s_Data.CubeIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CubeVertexBufferPtr - (uint8_t*)s_Data.CubeVertexBufferBase);
			s_Data.CubeVertexBuffer->SetData(s_Data.CubeVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			{
				if (s_Data.TextureSlots[i])
					s_Data.TextureSlots[i]->Bind(i);
			}

			s_Data.CubeShader->Bind();
			RenderCommand::DrawMeshIndexed(s_Data.CubeVertexArray, s_Data.CubeIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		// Draw planes
		if (s_Data.PlaneIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.PlaneVertexBufferPtr - (uint8_t*)s_Data.PlaneVertexBufferBase);
			s_Data.PlaneVertexBuffer->SetData(s_Data.PlaneVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			{
				if (s_Data.TextureSlots[i])
					s_Data.TextureSlots[i]->Bind(i);
			}

			s_Data.CubeShader->Bind();
			RenderCommand::DrawMeshIndexed(s_Data.PlaneVertexArray, s_Data.PlaneIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		// Draw Obj meshes
		if (s_Data.MeshIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.MeshVertexBufferPtr - (uint8_t*)s_Data.MeshVertexBufferBase);
			s_Data.MeshVertexBuffer->SetData(s_Data.MeshVertexBufferBase, dataSize);
		
			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			{
				if (s_Data.TextureSlots[i])
					s_Data.TextureSlots[i]->Bind(i);
			}
		
			s_Data.CubeShader->Bind();		
			RenderCommand::DrawMeshIndexed(s_Data.MeshVertexArray, s_Data.MeshIndexCount);
			s_Data.Stats.DrawCalls++;
		}
	}

	void Renderer3D::NextBatch()
	{
		Flush();
		StartBatch();
	}

#pragma region PyramidRendering

	void Renderer3D::DrawPyramid(const rtmcpp::Mat4& transform, PyramidRendererComponent& src, const rtmcpp::Vec4& color, int entityID, bool spotLightExists)
	{
		constexpr size_t pyramidVertexCount = s_PyramidSize;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = {
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, // UVs Bottom side
			{ 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f }, // UVs Bottom side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Front side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Right side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Back side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }  // UVs Left side
		};

		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		if (s_Data.PyramidIndexCount >= Renderer3DData::MaxPyramidIndices)
			NextBatch();

		for (size_t i = 0; i < pyramidVertexCount; i++)
		{
			s_Data.PyramidVertexBufferPtr->Position = s_Data.PyramidVertexPositions[i] * transform;
			s_Data.PyramidVertexBufferPtr->Normal = s_Data.PyramidVertexNormals[i];
			s_Data.PyramidVertexBufferPtr->Color = color;
			s_Data.PyramidVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.PyramidVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.PyramidVertexBufferPtr->TexIndex = textureIndex;

			s_Data.PyramidVertexBufferPtr->MaterialSpecular = src.MaterialSpecular;
			s_Data.PyramidVertexBufferPtr->MaterialShininess = src.MaterialShininess;
			s_Data.PyramidVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

			s_Data.PyramidVertexBufferPtr->EntityID = entityID;
			s_Data.PyramidVertexBufferPtr++;
		}

		s_Data.PyramidIndexCount += s_PyramidIndicesSize;

		s_Data.Stats.PyramidCount++;
	}

	void Renderer3D::DrawPyramid(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, PyramidRendererComponent& src, int entityID, bool spotLightExists)
	{
		NZ_CORE_VERIFY(texture);

		constexpr size_t pyramidVertexCount = s_PyramidSize;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = {
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, // UVs Bottom side 1st triangle
			{ 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f }, // UVs Bottom side 2nd triangle
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Front side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Right side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Back side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }  // UVs Left side
		};

		if (s_Data.PyramidIndexCount >= Renderer3DData::MaxPyramidIndices)
			NextBatch();

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;

			s_Data.TextureSlotIndex++;
		};

		for (size_t i = 0; i < pyramidVertexCount; i++)
		{
			s_Data.PyramidVertexBufferPtr->Position = s_Data.PyramidVertexPositions[i] * transform;
			s_Data.PyramidVertexBufferPtr->Normal = s_Data.PyramidVertexNormals[i];
			s_Data.PyramidVertexBufferPtr->Color = src.Color;
			s_Data.PyramidVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.PyramidVertexBufferPtr->TilingFactor = src.UV;
			s_Data.PyramidVertexBufferPtr->TexIndex = textureIndex;

			s_Data.PyramidVertexBufferPtr->MaterialSpecular = src.MaterialSpecular;
			s_Data.PyramidVertexBufferPtr->MaterialShininess = src.MaterialShininess;
			s_Data.PyramidVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

			s_Data.PyramidVertexBufferPtr->EntityID = entityID;
			s_Data.PyramidVertexBufferPtr++;
		}

		s_Data.PyramidIndexCount += s_PyramidIndicesSize;

		s_Data.Stats.PyramidCount++;
	}

	void Renderer3D::DrawPyramidMesh(const rtmcpp::Mat4& transform, PyramidRendererComponent& src, int entityID, bool spotLightExists)
	{
		if (src.TextureHandle)
		{
			RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);
			DrawPyramid(transform, texture, src, entityID, spotLightExists);
		}
		else
			DrawPyramid(transform, src, src.Color, entityID, spotLightExists);
	}

#pragma endregion

#pragma region TriangularPrismRendering

	void Renderer3D::DrawTriangularPrism(const rtmcpp::Mat4& transform, TriangularPrismRendererComponent& src, const rtmcpp::Vec4& color, int entityID, bool spotLightExists)
	{
		constexpr size_t triangleVertexCount = s_TriangleSize;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = {
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Bottom side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Front side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Right side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }  // UVs Left side
		};

		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		if (s_Data.TriangleIndexCount >= Renderer3DData::MaxTriangleIndices)
			NextBatch();

		for (size_t i = 0; i < triangleVertexCount; i++)
		{
			s_Data.TriangleVertexBufferPtr->Position = s_Data.TriangleVertexPositions[i] * transform;
			s_Data.TriangleVertexBufferPtr->Normal = s_Data.TriangleVertexNormals[i];
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.TriangleVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.TriangleVertexBufferPtr->TexIndex = textureIndex;

			s_Data.TriangleVertexBufferPtr->MaterialSpecular = src.MaterialSpecular;
			s_Data.TriangleVertexBufferPtr->MaterialShininess = src.MaterialShininess;
			s_Data.TriangleVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

			s_Data.TriangleVertexBufferPtr->EntityID = entityID;
			s_Data.TriangleVertexBufferPtr++;
		}

		s_Data.TriangleIndexCount += s_TriangleIndicesSize;

		s_Data.Stats.TriangularPrismCount++;
	}

	void Renderer3D::DrawTriangularPrism(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, TriangularPrismRendererComponent& src, int entityID, bool spotLightExists)
	{
		NZ_CORE_VERIFY(texture);

		constexpr size_t triangleVertexCount = s_TriangleSize;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = {
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Bottom side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Front side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, // UVs Right side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }  // UVs Left side
		};

		if (s_Data.TriangleIndexCount >= Renderer3DData::MaxTriangleIndices)
			NextBatch();

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;

			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < triangleVertexCount; i++)
		{
			s_Data.TriangleVertexBufferPtr->Position = s_Data.TriangleVertexPositions[i] * transform;
			s_Data.TriangleVertexBufferPtr->Normal = s_Data.TriangleVertexNormals[i];
			s_Data.TriangleVertexBufferPtr->Color = src.Color;
			s_Data.TriangleVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.TriangleVertexBufferPtr->TilingFactor = src.UV;
			s_Data.TriangleVertexBufferPtr->TexIndex = textureIndex;

			s_Data.TriangleVertexBufferPtr->MaterialSpecular = src.MaterialSpecular;
			s_Data.TriangleVertexBufferPtr->MaterialShininess = src.MaterialShininess;
			s_Data.TriangleVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

			s_Data.TriangleVertexBufferPtr->EntityID = entityID;
			s_Data.TriangleVertexBufferPtr++;
		}

		s_Data.TriangleIndexCount += s_TriangleIndicesSize;

		s_Data.Stats.TriangularPrismCount++;
	}

	void Renderer3D::DrawTriangularPrismMesh(const rtmcpp::Mat4& transform, TriangularPrismRendererComponent& src, int entityID, bool spotLightExists)
	{
		if (src.TextureHandle)
		{
			RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);
			DrawTriangularPrism(transform, texture, src, entityID, spotLightExists);
		}
		else
			DrawTriangularPrism(transform, src, src.Color, entityID, spotLightExists);
	}

#pragma endregion

#pragma region CubeRendering

	void Renderer3D::DrawCube(const rtmcpp::Mat4& transform, CubeRendererComponent& src, const rtmcpp::Vec4& color, int entityID, bool spotLightExists)
	{
		//NZ_PROFILE_FUNCTION();

		constexpr size_t cubeVertexCount = s_CubeSize;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = {
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, // UVs Front side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, // UVs Right side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, // UVs Back side
			{ 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f }, // UVs Left side
			{ 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, // UVs Top side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }  // UVs Bottom side
		};

		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		if (s_Data.CubeIndexCount >= Renderer3DData::MaxCubeIndices)
			NextBatch();

		for (size_t i = 0; i < cubeVertexCount; i++)
		{
			s_Data.CubeVertexBufferPtr->Position = s_Data.CubeVertexPositions[i] * transform;
			s_Data.CubeVertexBufferPtr->Normal = s_Data.CubeVertexNormals[i];
			s_Data.CubeVertexBufferPtr->Color = color;
			s_Data.CubeVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.CubeVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.CubeVertexBufferPtr->TexIndex = textureIndex;

			s_Data.CubeVertexBufferPtr->MaterialSpecular = src.MaterialSpecular;
			s_Data.CubeVertexBufferPtr->MaterialShininess = src.MaterialShininess;
			s_Data.CubeVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

			s_Data.CubeVertexBufferPtr->EntityID = entityID;
			s_Data.CubeVertexBufferPtr++;
		}

		s_Data.CubeIndexCount += s_CubeIndicesSize;

		s_Data.Stats.CubeCount++;
	}

	void Renderer3D::DrawCube(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, CubeRendererComponent& src, const rtmcpp::Vec2& parallaxScrolling, const Timestep& ts, const rtmcpp::Mat4& cameraProjection, int entityID, bool spotLightExists)
	{
		//NZ_PROFILE_FUNCTION();

		NZ_CORE_VERIFY(texture);

		constexpr size_t cubeVertexCount = s_CubeSize;
		rtmcpp::Vec2 textureCoords[] = {
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, // UVs Front side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, // UVs Right side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, // UVs Back side
			{ 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f }, // UVs Left side
			{ 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, // UVs Top side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }  // UVs Bottom side
		};

		if (s_Data.CubeIndexCount >= Renderer3DData::MaxCubeIndices)
			NextBatch();

		int textureIndex = 0;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;

			s_Data.TextureSlotIndex++;
		}

		auto& animSrc = src.m_AnimationData;

		bool useCamX = animSrc.UseCameraParallaxX;

		UpdateAnimation(animSrc.NumberOfTiles, animSrc.StartIndexX, animSrc.StartIndexY, animSrc.AnimationSpeed * ts, animSrc.AnimationTime, animSrc.UseTextureAtlasAnimation, animSrc.UseCameraParallaxY);

		if (s_AnimateInRuntime || s_AnimateInEdit)
		{
			for (size_t i = 0; i < cubeVertexCount; i++)
			{
				s_Data.CubeVertexBufferPtr->Position = s_Data.CubeVertexPositions[i] * transform;
				s_Data.CubeVertexBufferPtr->Normal = s_Data.CubeVertexNormals[i];
				s_Data.CubeVertexBufferPtr->Color = src.Color;

				if (!animSrc.UseParallaxScrolling && !animSrc.UseCameraParallaxX && !animSrc.UseCameraParallaxY)
				{
					if (src.UV.X >= 0.01f && src.UV.Y >= 0.01f && animSrc.Rows > 0 && animSrc.Columns > 0)
					{
						float tmpTileX = fmod((float)tileIndexX, (float)animSrc.Rows);
						float tmpTileY = fmod((float)tileIndexY, (float)animSrc.Columns);

						//src.m_Animation2D.CurrentSpriteIndex = glm::ivec2{ (int)tmpTileX, (int)tmpTileY };
						//src.m_Animation2D.CurrentSpriteIndex = glm::vec2{ tmpTileX, tmpTileY };
						
						s_Data.CubeVertexBufferPtr->TexCoord = textureCoords[i];
						s_Data.CubeVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
						s_Data.CubeVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;

						s_Data.CubeVertexBufferPtr->TilingFactor = src.UV;
						s_Data.CubeVertexBufferPtr->TilingFactor.X /= (float)animSrc.Rows;
						s_Data.CubeVertexBufferPtr->TilingFactor.Y /= (float)animSrc.Columns;
					}
				}
				else if (animSrc.UseParallaxScrolling && !animSrc.UseCameraParallaxX && !animSrc.UseCameraParallaxY && !animSrc.UseTextureAtlasAnimation)
				{
					if (parallaxScrolling.X != 0.0f && animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling)
					{
						float coordsAndTiling = textureCoords[i].X + src.UV.X;

						m_ScrollingPlusDivision.X += parallaxScrolling.X / animSrc.ParallaxDivision;
						s_Data.CubeVertexBufferPtr->TexCoord.X = coordsAndTiling + m_ScrollingPlusDivision.X;
					}
					else if (parallaxScrolling.X == 0.0f && animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling)
					{
						s_Data.CubeVertexBufferPtr->TexCoord.X = textureCoords[i].X;
					}

					if (parallaxScrolling.Y != 0.0f && animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling)
					{
						float coordsAndTiling = textureCoords[i].Y + src.UV.Y;

						m_ScrollingPlusDivision.Y += parallaxScrolling.Y / animSrc.ParallaxDivision;
						s_Data.CubeVertexBufferPtr->TexCoord.Y = coordsAndTiling + m_ScrollingPlusDivision.Y;
					}
					else if (parallaxScrolling.Y == 0.0f && animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling)
					{
						s_Data.CubeVertexBufferPtr->TexCoord.Y = textureCoords[i].Y;
					}

					s_Data.CubeVertexBufferPtr->TilingFactor = src.UV;
				}
				else if (animSrc.UseParallaxScrolling && !animSrc.UseTextureAtlasAnimation)
				{
					rtmcpp::Vec4 pos = cameraProjection.Value.w_axis;
					rtmcpp::Vec4 localPos = transform.Value.w_axis;

					if (animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling && animSrc.UseCameraParallaxX)
					{
						float coordsAndTiling = textureCoords[i].X + src.UV.X;

						m_ScrollingPlusDivision.X = (pos.X - localPos.X) / animSrc.ParallaxDivision;
						s_Data.CubeVertexBufferPtr->TexCoord.X = coordsAndTiling + m_ScrollingPlusDivision.X;
					}

					if (animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling && animSrc.UseCameraParallaxY)
					{
						float coordsAndTiling = textureCoords[i].Y + src.UV.Y;

						m_ScrollingPlusDivision.Y = (pos.Y - localPos.Y) / animSrc.ParallaxDivision;
						s_Data.CubeVertexBufferPtr->TexCoord.Y = coordsAndTiling + m_ScrollingPlusDivision.Y;
					}

					s_Data.CubeVertexBufferPtr->TilingFactor = src.UV;
				}
				else if (animSrc.UseParallaxScrolling && !animSrc.UseCameraParallaxX && !animSrc.UseCameraParallaxY && animSrc.UseTextureAtlasAnimation)
				{
					float tmpTileX = fmod((float)tileIndexX, (float)animSrc.Rows);
					float tmpTileY = fmod((float)tileIndexY, (float)animSrc.Columns);

					//src.m_Animation2D.CurrentSpriteIndex = glm::ivec2{ (int)tmpTileX, (int)tmpTileY };
					//src.m_Animation2D.CurrentSpriteIndex = glm::vec2{ tmpTileX, tmpTileY };

					if (parallaxScrolling.X != 0.0f && animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling)
					{
						float coordsAndTiling = textureCoords[i].X + src.UV.X;

						m_ScrollingPlusDivision.X += parallaxScrolling.X / animSrc.ParallaxDivision;
						s_Data.CubeVertexBufferPtr->TexCoord.X = coordsAndTiling + m_ScrollingPlusDivision.X;
						s_Data.CubeVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
					}
					else if (parallaxScrolling.X == 0.0f && animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling)
					{
						s_Data.CubeVertexBufferPtr->TexCoord.X = textureCoords[i].X;
					}

					if (parallaxScrolling.Y != 0.0f && animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling)
					{
						float coordsAndTiling = textureCoords[i].Y + src.UV.Y;

						m_ScrollingPlusDivision.Y += parallaxScrolling.Y / animSrc.ParallaxDivision;
						s_Data.CubeVertexBufferPtr->TexCoord.Y = coordsAndTiling + m_ScrollingPlusDivision.Y;
						s_Data.CubeVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;
					}
					else if (parallaxScrolling.Y == 0.0f && animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling)
					{
						s_Data.CubeVertexBufferPtr->TexCoord.Y = textureCoords[i].Y;
					}

					s_Data.CubeVertexBufferPtr->TilingFactor = src.UV;
					s_Data.CubeVertexBufferPtr->TilingFactor.X = src.UV.X / (float)animSrc.Rows;
					s_Data.CubeVertexBufferPtr->TilingFactor.Y = src.UV.Y / (float)animSrc.Columns;
				}
				else if (animSrc.UseParallaxScrolling && animSrc.UseTextureAtlasAnimation)
				{
					float tmpTileX = fmod((float)tileIndexX, (float)animSrc.Rows);
					float tmpTileY = fmod((float)tileIndexY, (float)animSrc.Columns);

					//src.m_Animation2D.CurrentSpriteIndex = glm::ivec2{ (int)tmpTileX, (int)tmpTileY };
					//src.m_Animation2D.CurrentSpriteIndex = glm::vec2{ tmpTileX, tmpTileY };

					rtmcpp::Vec4 pos = cameraProjection.Value.w_axis;
					rtmcpp::Vec4 localPos = transform.Value.w_axis;

					if (animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling && animSrc.UseCameraParallaxX && !animSrc.UseCameraParallaxY)
					{
						float coordsAndTiling = textureCoords[i].X + src.UV.X;

						m_ScrollingPlusDivision.X = (pos.X - localPos.X) / animSrc.ParallaxDivision;
						s_Data.CubeVertexBufferPtr->TexCoord.X = coordsAndTiling + m_ScrollingPlusDivision.X;
						s_Data.CubeVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
					}
					else if (animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling && !animSrc.UseCameraParallaxX && animSrc.UseCameraParallaxY)
					{
						float coordsAndTiling = textureCoords[i].Y + src.UV.Y;

						m_ScrollingPlusDivision.Y = (pos.Y - localPos.Y) / animSrc.ParallaxDivision;
						s_Data.CubeVertexBufferPtr->TexCoord.Y = coordsAndTiling + m_ScrollingPlusDivision.Y;
						s_Data.CubeVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;
					}
					else if (animSrc.ParallaxDivision > 0.0f && animSrc.UseParallaxScrolling && animSrc.UseCameraParallaxX && animSrc.UseCameraParallaxY)
					{
						float coordsAndTilingX = textureCoords[i].X + src.UV.X;

						m_ScrollingPlusDivision.X = (pos.X - localPos.X) / animSrc.ParallaxDivision;
						s_Data.CubeVertexBufferPtr->TexCoord.X = coordsAndTilingX + m_ScrollingPlusDivision.X;
						s_Data.CubeVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;

						float coordsAndTilingY = textureCoords[i].Y + src.UV.Y;

						m_ScrollingPlusDivision.Y = (pos.Y - localPos.Y) / animSrc.ParallaxDivision;
						s_Data.CubeVertexBufferPtr->TexCoord.Y = coordsAndTilingY + m_ScrollingPlusDivision.Y;
						s_Data.CubeVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;
					}

					s_Data.CubeVertexBufferPtr->TilingFactor = src.UV;
					s_Data.CubeVertexBufferPtr->TilingFactor.X = src.UV.X / (float)animSrc.Rows;
					s_Data.CubeVertexBufferPtr->TilingFactor.Y = src.UV.Y / (float)animSrc.Columns;
				}

				s_Data.CubeVertexBufferPtr->TexIndex = textureIndex;

				s_Data.CubeVertexBufferPtr->MaterialSpecular = src.MaterialSpecular;
				s_Data.CubeVertexBufferPtr->MaterialShininess = src.MaterialShininess;
				s_Data.CubeVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

				s_Data.CubeVertexBufferPtr->EntityID = entityID;
				s_Data.CubeVertexBufferPtr++;
			}

			s_Data.CubeIndexCount += s_CubeIndicesSize;

			s_Data.Stats.CubeCount++;
		}
		else if (!s_AnimateInRuntime && !s_AnimateInEdit)
		{
			for (size_t i = 0; i < cubeVertexCount; i++)
			{
				s_Data.CubeVertexBufferPtr->Position = s_Data.CubeVertexPositions[i] * transform;
				s_Data.CubeVertexBufferPtr->Normal = s_Data.CubeVertexNormals[i];
				s_Data.CubeVertexBufferPtr->Color = src.Color;

				if (!animSrc.UseParallaxScrolling && !animSrc.UseTextureAtlasAnimation)// && !s_Reset)
				{
					if (src.UV.X >= 0.01f && src.UV.Y >= 0.01f && animSrc.Rows > 0 && animSrc.Columns > 0)
					{
						rtmcpp::Vec2 rowsAndColumns = rtmcpp::Vec2((float)animSrc.Rows, (float)animSrc.Columns);

						float tmpTileX = fmod((float)tileIndexX, rowsAndColumns.X);
						float tmpTileY = fmod((float)tileIndexY, rowsAndColumns.Y);

						//src.m_Animation2D.CurrentSpriteIndex = glm::ivec2{ (int)tmpTileX, (int)tmpTileY };

						s_Data.CubeVertexBufferPtr->TexCoord = textureCoords[i];
						s_Data.CubeVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
						s_Data.CubeVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;

						s_Data.CubeVertexBufferPtr->TilingFactor = src.UV;
						s_Data.CubeVertexBufferPtr->TilingFactor.X /= rowsAndColumns.X;
						s_Data.CubeVertexBufferPtr->TilingFactor.Y /= rowsAndColumns.Y;
					}
				}
				else if (!animSrc.UseParallaxScrolling && animSrc.UseTextureAtlasAnimation)// && !s_Reset)
				{
					if (src.UV.X >= 0.01f && src.UV.Y >= 0.01f && animSrc.Rows > 0 && animSrc.Columns > 0)
					{
						float tmpTileX = fmod((float)tileIndexX, (float)animSrc.Rows);
						float tmpTileY = fmod((float)tileIndexY, (float)animSrc.Columns);

						//src.m_Animation2D.CurrentSpriteIndex = glm::ivec2{ (int)tmpTileX, (int)tmpTileY };

						s_Data.CubeVertexBufferPtr->TexCoord = textureCoords[i];
						s_Data.CubeVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
						s_Data.CubeVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;

						s_Data.CubeVertexBufferPtr->TilingFactor = src.UV;
						s_Data.CubeVertexBufferPtr->TilingFactor.X /= (float)animSrc.Rows;
						s_Data.CubeVertexBufferPtr->TilingFactor.Y /= (float)animSrc.Columns;
					}
				}
				else if (animSrc.UseParallaxScrolling && !animSrc.UseTextureAtlasAnimation)
				{
					s_Data.CubeVertexBufferPtr->TilingFactor = src.UV;
				}
				else if (animSrc.UseParallaxScrolling && animSrc.UseTextureAtlasAnimation)
				{
					if (src.UV.X >= 0.01f && src.UV.Y >= 0.01f && animSrc.Rows > 0 && animSrc.Columns > 0)
					{
						float tmpTileX = fmod((float)tileIndexX, (float)animSrc.Rows);
						float tmpTileY = fmod((float)tileIndexY, (float)animSrc.Columns);

						//src.m_Animation2D.CurrentSpriteIndex = glm::ivec2{ (int)tmpTileX, (int)tmpTileY };

						s_Data.CubeVertexBufferPtr->TexCoord = textureCoords[i];
						s_Data.CubeVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
						s_Data.CubeVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;

						s_Data.CubeVertexBufferPtr->TilingFactor = src.UV;
						s_Data.CubeVertexBufferPtr->TilingFactor.X = src.UV.X / (float)animSrc.Rows;
						s_Data.CubeVertexBufferPtr->TilingFactor.Y = src.UV.Y / (float)animSrc.Columns;
					}
				}

				m_ScrollingPlusDivision.X = 0.0f;
				m_ScrollingPlusDivision.Y = 0.0f;

				s_Data.CubeVertexBufferPtr->TexIndex = textureIndex;

				s_Data.CubeVertexBufferPtr->MaterialSpecular = src.MaterialSpecular;
				s_Data.CubeVertexBufferPtr->MaterialShininess = src.MaterialShininess;
				s_Data.CubeVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

				s_Data.CubeVertexBufferPtr->EntityID = entityID;
				s_Data.CubeVertexBufferPtr++;
			}

			s_Data.CubeIndexCount += s_CubeIndicesSize;

			s_Data.Stats.CubeCount++;
		}
	}

	void Renderer3D::DrawAnimatedCube(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, CubeRendererComponent& src, const Timestep& ts, int entityID, bool spotLightExists)
	{
		//NZ_PROFILE_FUNCTION();

		//NZ_CORE_VERIFY(texture);

		constexpr size_t cubeVertexCount = s_CubeSize;
		rtmcpp::Vec2 textureCoords[] = {
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, // UVs Front side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, // UVs Right side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, // UVs Back side
			{ 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f }, // UVs Left side
			{ 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, // UVs Top side
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }  // UVs Bottom side
		};

		if (s_Data.CubeIndexCount >= Renderer3DData::MaxCubeIndices)
			NextBatch();

		int textureIndex = 0;

		auto& animSrc = src.m_AnimationData;

		UpdateTextureAnimation((int)animSrc.Textures.size(), animSrc.StartIndex, animSrc.AnimationSpeed * ts, animSrc.AnimationTime, src.m_AnimationData.UsePerTextureAnimation);

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;

			RefPtr<Texture2D> textureHandle = AssetManager::GetAsset<Texture2D>(src.TextureHandle);
			textureHandle = texture;

			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;

			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < cubeVertexCount; i++)
		{
			s_Data.CubeVertexBufferPtr->Position = s_Data.CubeVertexPositions[i] * transform;
			s_Data.CubeVertexBufferPtr->Normal = s_Data.CubeVertexNormals[i];
			s_Data.CubeVertexBufferPtr->Color = src.Color;
			s_Data.CubeVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.CubeVertexBufferPtr->TilingFactor = src.UV;
			s_Data.CubeVertexBufferPtr->TexIndex = textureIndex;

			s_Data.CubeVertexBufferPtr->MaterialSpecular = src.MaterialSpecular;
			s_Data.CubeVertexBufferPtr->MaterialShininess = src.MaterialShininess;
			s_Data.CubeVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

			s_Data.CubeVertexBufferPtr->EntityID = entityID;
			s_Data.CubeVertexBufferPtr++;
		}

		s_Data.CubeIndexCount += s_CubeIndicesSize;

		s_Data.Stats.CubeCount++;
	}

	void Renderer3D::DrawCubeMesh(const rtmcpp::Mat4& transform, CubeRendererComponent& src, const Timestep& ts, const rtmcpp::Mat4& cameraProjection, int entityID, bool IsRuntime, bool spotLightExists)
	{
		//NZ_PROFILE_FUNCTION();

		if (!IsRuntime)
		{
			if (src.TextureHandle)
			{
				RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);
				DrawCube(transform, texture, src, rtmcpp::Vec2(src.m_AnimationData.ParallaxSpeed.X * ts, src.m_AnimationData.ParallaxSpeed.Y * ts), ts, cameraProjection, entityID, spotLightExists);
			}
			else
				DrawCube(transform, src, src.Color, entityID, spotLightExists);
		}
		else
		{
			if (src.m_AnimationData.Textures[0])
			{
				if (src.m_AnimationData.UsePerTextureAnimation)
				{
					RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(src.m_AnimationData.Textures[(uint32_t)tileIndexX]);
					DrawAnimatedCube(transform, texture, src, ts, entityID, spotLightExists);
				}
				else
				{
					RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(src.m_AnimationData.Textures[(uint32_t)src.m_AnimationData.StartIndex]);
					DrawAnimatedCube(transform, texture, src, ts, entityID, spotLightExists);
				}
			}
			else
				DrawCube(transform, src, src.Color, entityID, spotLightExists);
		}
	}

#pragma endregion

#pragma region PlaneRendering

	void Renderer3D::DrawPlane(const rtmcpp::Mat4& transform, PlaneRendererComponent& src, const rtmcpp::Vec4& color, int entityID, bool spotLightExists)
	{
		constexpr size_t planeVertexCount = 4;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		if (s_Data.PlaneIndexCount >= Renderer3DData::MaxQuadIndices)
			NextBatch();

		for (size_t i = 0; i < planeVertexCount; i++)
		{
			s_Data.PlaneVertexBufferPtr->Position = s_Data.PlaneVertexPositions[i] * transform;
			s_Data.PlaneVertexBufferPtr->Normal = s_Data.PlaneVertexNormals[i];
			s_Data.PlaneVertexBufferPtr->Color = color;
			s_Data.PlaneVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.PlaneVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.PlaneVertexBufferPtr->TexIndex = textureIndex;

			s_Data.PlaneVertexBufferPtr->MaterialSpecular = src.MaterialSpecular;
			s_Data.PlaneVertexBufferPtr->MaterialShininess = src.MaterialShininess;
			s_Data.PlaneVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

			s_Data.PlaneVertexBufferPtr->EntityID = entityID;
			s_Data.PlaneVertexBufferPtr++;
		}

		s_Data.PlaneIndexCount += 6;
	}

	void Renderer3D::DrawPlane(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, PlaneRendererComponent& src, int entityID, bool spotLightExists)
	{
		NZ_CORE_VERIFY(texture);

		constexpr size_t planeVertexCount = 4;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.PlaneIndexCount >= Renderer3DData::MaxQuadIndices)
			NextBatch();

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;

			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < planeVertexCount; i++)
		{
			s_Data.PlaneVertexBufferPtr->Position = s_Data.PlaneVertexPositions[i] * transform;
			s_Data.PlaneVertexBufferPtr->Normal = s_Data.PlaneVertexNormals[i];
			s_Data.PlaneVertexBufferPtr->Color = src.Color;
			s_Data.PlaneVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.PlaneVertexBufferPtr->TilingFactor = src.UV;
			s_Data.PlaneVertexBufferPtr->TexIndex = textureIndex;

			s_Data.PlaneVertexBufferPtr->MaterialSpecular = src.MaterialSpecular;
			s_Data.PlaneVertexBufferPtr->MaterialShininess = src.MaterialShininess;
			s_Data.PlaneVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

			s_Data.PlaneVertexBufferPtr->EntityID = entityID;
			s_Data.PlaneVertexBufferPtr++;
		}

		s_Data.PlaneIndexCount += 6;
	}

	void Renderer3D::DrawPlaneMesh(const rtmcpp::Mat4& transform, PlaneRendererComponent& src, int entityID, bool spotLightExists)
	{
		if (src.TextureHandle)
		{
			RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);
			DrawPlane(transform, texture, src, entityID, spotLightExists);
		}
		else
			DrawPlane(transform, src, src.Color, entityID, spotLightExists);
	}

#pragma endregion

#pragma region MeshLoading

	void Renderer3D::DrawOBJ(const rtmcpp::Mat4& transform, RefPtr<ObjModel>& model, OBJRendererComponent& src, const rtmcpp::Vec4& color, int entityID, bool spotLightExists)
	{
		const size_t meshVertexCount = model->GetMeshPositionIndices().size();
		s_Data.MeshVerticesSize = (uint32_t)model->GetMeshPositionVertices().size();
		int textureIndex = 0; // White Texture

		if (s_Data.MeshIndexCount >= Renderer3DData::MaxMeshIndices)
			NextBatch();

		NZ_CORE_WARN("New Vertex Positions: {0}", meshVertexCount);

		for (size_t i = 0; i < meshVertexCount; i++)
		{
			s_Data.MeshVertexBufferPtr->Position = rtmcpp::Vec4(model->GetMeshPositionVertices()[i].X, model->GetMeshPositionVertices()[i].Y, model->GetMeshPositionVertices()[i].Z, 1.0f) * transform;
			s_Data.MeshVertexBufferPtr->Position.W = 1.0f;
			
			if (model->GetMeshNormalVertices().size() > 0)
			{
				s_Data.MeshVertexBufferPtr->Normal = rtmcpp::Vec4{ model->GetMeshNormalVertices()[i], 1.0f };
			}
			else
			{
				s_Data.MeshVertexBufferPtr->Normal = rtmcpp::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
			}

			s_Data.MeshVertexBufferPtr->Color = src.Color;

			s_Data.MeshVertexBufferPtr->TexCoord = model->GetMeshUVVertices()[i];

			s_Data.MeshVertexBufferPtr->TilingFactor = src.UV;
			s_Data.MeshVertexBufferPtr->TexIndex = textureIndex;

			s_Data.MeshVertexBufferPtr->MaterialSpecular = model->GetMaterialSpecular();
			s_Data.MeshVertexBufferPtr->MaterialShininess = model->GetMaterialShininess();
			s_Data.MeshVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

			s_Data.MeshVertexBufferPtr->EntityID = entityID;
			s_Data.MeshVertexBufferPtr++;
		}

		s_Data.MeshIndexCount += (uint32_t)model->GetMeshPositionIndices().size();
		s_Data.MeshIndicesSize = (uint32_t)model->GetMeshPositionIndices().size();

		s_Data.Stats.MeshCount++;
	}

	void Renderer3D::DrawOBJ(const rtmcpp::Mat4& transform, RefPtr<ObjModel>& model, const RefPtr<Texture2D>& texture, OBJRendererComponent& src, int entityID, bool spotLightExists)
	{
		const size_t meshVertexCount = model->GetMeshPositionIndices().size();
		s_Data.MeshVerticesSize = (uint32_t)model->GetMeshPositionVertices().size();
		int textureIndex = 0; // White Texture

		if (s_Data.MeshIndexCount >= Renderer3DData::MaxMeshIndices)
			NextBatch();

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;

			s_Data.TextureSlotIndex++;
		};

		for (size_t i = 0; i < meshVertexCount; i++)
		{
			s_Data.MeshVertexBufferPtr->Position = rtmcpp::Vec4(model->GetMeshPositionVertices()[i].X, model->GetMeshPositionVertices()[i].Y, model->GetMeshPositionVertices()[i].Z, 1.0f) * transform;

			if (model->GetMeshNormalVertices().size() > 0)
			{
				s_Data.MeshVertexBufferPtr->Normal = rtmcpp::Vec4{ model->GetMeshNormalVertices()[i], 1.0f };
			}
			else
			{
				s_Data.MeshVertexBufferPtr->Normal = rtmcpp::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
			}

			s_Data.MeshVertexBufferPtr->Color = src.Color;

			s_Data.MeshVertexBufferPtr->TexCoord = model->GetMeshUVVertices()[i];

			s_Data.MeshVertexBufferPtr->TilingFactor = src.UV;
			s_Data.MeshVertexBufferPtr->TexIndex = textureIndex;

			s_Data.MeshVertexBufferPtr->MaterialSpecular = model->GetMaterialSpecular();
			s_Data.MeshVertexBufferPtr->MaterialShininess = model->GetMaterialShininess();
			s_Data.MeshVertexBufferPtr->ViewPos = s_Data.CamPosBuffer.CamPosition;

			s_Data.MeshVertexBufferPtr->EntityID = entityID;
			s_Data.MeshVertexBufferPtr++;
		}

		s_Data.MeshIndexCount += (uint32_t)model->GetMeshPositionIndices().size();
		s_Data.MeshIndicesSize = (uint32_t)model->GetMeshPositionIndices().size();

		s_Data.Stats.MeshCount++;
	}

	void Renderer3D::DrawOBJMesh(const rtmcpp::Mat4& transform, OBJRendererComponent& src, int entityID, bool spotLightExists)
	{
		if (src.ModelHandle)
		{
			RefPtr<ObjModel> obj = AssetManager::GetAsset<ObjModel>(src.ModelHandle);
			if (obj)
			{
				if (obj->GetTextureHandle())
				{
					RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(obj->GetTextureHandle());
					DrawOBJ(transform, obj, texture, src, entityID, spotLightExists);
				}
				else
					DrawOBJ(transform, obj, src, src.Color, entityID, spotLightExists);
			}
		}
	}

	uint32_t Renderer3D::GetMeshVerticesSize()
	{
		return (uint32_t)s_Data.MeshVerticesSize;
	}

	uint32_t Renderer3D::GetMeshIndicesSize()
	{
		return (uint32_t)s_Data.MeshIndicesSize;
	}

#pragma endregion

	void Renderer3D::UpdateAnimation(const int& numTiles, const int& startIndexX, const int& startIndexY, const float& animSpeed, float& animationTime, bool useAnimation, bool useCameraParallaxY)
	{
		//NZ_PROFILE_FUNCTION();

		if (s_AnimateInRuntime || s_AnimateInEdit)
		{
			if (useAnimation)
			{
				animationTime += animSpeed;
				tileIndexX = startIndexX + (int)animationTime % numTiles;

				if (useCameraParallaxY)
					tileIndexY = startIndexY + (int)animationTime % numTiles;
				else
					tileIndexY = startIndexY + (int)animationTime % 1;
			}
			else
			{
				animationTime = 0.0f;
				tileIndexX = startIndexX + (int)animationTime % numTiles;

				if (useCameraParallaxY)
					tileIndexY = startIndexY + (int)animationTime % numTiles;
				else
					tileIndexY = startIndexY + (int)animationTime % 1;
			}
		}
		else
		{
			if (useAnimation)
			{
				animationTime = 0.0f;
				tileIndexX = startIndexX + (int)animationTime % numTiles;

				if (useCameraParallaxY)
					tileIndexY = startIndexY + (int)animationTime % numTiles;
				else
					tileIndexY = startIndexY + (int)animationTime % 1;
			}
			else if (!useAnimation)
			{
				animationTime = 0.0f;
				tileIndexX = startIndexX + (int)animationTime % numTiles;

				if (useCameraParallaxY)
					tileIndexY = startIndexY + (int)animationTime % numTiles;
				else
					tileIndexY = startIndexY + (int)animationTime % 1;
			}
		}
	}

	void Renderer3D::UpdateTextureAnimation(const int& numTextures, const int& startIndex, const float& animSpeed, float& animationTime, bool useAnimation)
	{
		//NZ_PROFILE_FUNCTION();

		int numberOfTextures = numTextures;

		if (numberOfTextures > 0)
		{
			numberOfTextures = numTextures - startIndex;
		}

		if (useAnimation)
		{
			animationTime += animSpeed;
			tileIndexX = startIndex + (int)animationTime % numberOfTextures;
		}
		else
		{
			animationTime = 0.0f;
			tileIndexX = startIndex + (int)animationTime % numberOfTextures;
		}
	}

	void Renderer3D::Reset3DStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics3D));
	}

	Renderer3D::Statistics3D Renderer3D::Get3DStats()
	{
		return s_Data.Stats;
	}

}