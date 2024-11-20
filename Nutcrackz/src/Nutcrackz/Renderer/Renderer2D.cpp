#include "nzpch.hpp"
#include "Renderer2D.hpp"

#include "VertexArray.hpp"
#include "Shader.hpp"
#include "RenderCommand.hpp"
#include "UniformBuffer.hpp"

#include "rtmcpp/PackedVector.hpp"

#include "Nutcrackz/Asset/AssetManager.hpp"

#include "Nutcrackz/Math/Math.hpp"

#include "MSDFData.hpp"

#include <codecvt>

namespace Nutcrackz {

	bool Renderer2D::s_AnimateInRuntime = false;
	bool Renderer2D::s_AnimateInEdit = false;
	int Renderer2D::tileIndexX = 0;
	int Renderer2D::tileIndexY = 0;

	struct TriangleVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;
		float Saturation;

		// Editor-only
		int EntityID;
	};

	struct QuadVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;
		float Saturation;

		// Editor-only
		int EntityID;
	};

	struct CircleVertex
	{
		rtmcpp::PackedVec4 WorldPosition;
		rtmcpp::PackedVec4 LocalPosition;
		rtmcpp::PackedVec4 Color;
		float Thickness;
		float Fade;
		rtmcpp::PackedVec2 WorldScale;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;

		// Editor-only
		int EntityID;
	};

	struct LineVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Color;

		// Editor-only
		int EntityID;
	};

	struct TextVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec4 BgColor;
		rtmcpp::PackedVec2 TexCoord;
		int TexIndex;

		// Editor-only
		int EntityID;
	};

	struct QuadButtonVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;

		float Radius;
		rtmcpp::PackedVec2 Dimensions;
		int ShouldInvertUICorners;

		// Editor-only
		int EntityID;
	};

	struct CircleButtonVertex
	{
		rtmcpp::PackedVec4 WorldPosition;
		rtmcpp::PackedVec4 LocalPosition;
		rtmcpp::PackedVec4 Color;
		float Thickness;
		float Fade;
		rtmcpp::PackedVec2 WorldScale;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;

		// Editor-only
		int EntityID;
	};

	struct Renderer2DData
	{
		static const uint32_t MaxTriangles = 20000;
		static const uint32_t MaxTriangleVertices = MaxTriangles * 4;
		static const uint32_t MaxTriangleIndices = MaxTriangles * 6;
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		RefPtr<VertexArray> TriangleVertexArray;
		RefPtr<VertexBuffer> TriangleVertexBuffer;
		RefPtr<Shader> TriangleShader;
		
		RefPtr<VertexArray> QuadVertexArray;
		RefPtr<VertexBuffer> QuadVertexBuffer;
		RefPtr<Shader> QuadShader;
		RefPtr<Texture2D> WhiteTexture;

		RefPtr<VertexArray> CircleVertexArray;
		RefPtr<VertexBuffer> CircleVertexBuffer;
		RefPtr<Shader> CircleShader;

		RefPtr<VertexArray> LineVertexArray;
		RefPtr<VertexBuffer> LineVertexBuffer;
		RefPtr<Shader> LineShader;

		RefPtr<VertexArray> TextVertexArray;
		RefPtr<VertexBuffer> TextVertexBuffer;
		RefPtr<Shader> TextShader;

		RefPtr<VertexArray> QuadButtonVertexArray;
		RefPtr<VertexBuffer> QuadButtonVertexBuffer;
		RefPtr<Shader> QuadButtonShader;

		RefPtr<VertexArray> CircleButtonVertexArray;
		RefPtr<VertexBuffer> CircleButtonVertexBuffer;
		RefPtr<Shader> CircleButtonShader;

		uint32_t TriangleIndexCount = 0;
		TriangleVertex* TriangleVertexBufferBase = nullptr;
		TriangleVertex* TriangleVertexBufferPtr = nullptr;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		uint32_t CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPtr = nullptr;

		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;

		uint32_t TextIndexCount = 0;
		TextVertex* TextVertexBufferBase = nullptr;
		TextVertex* TextVertexBufferPtr = nullptr;

		uint32_t QuadButtonIndexCount = 0;
		QuadButtonVertex* QuadButtonVertexBufferBase = nullptr;
		QuadButtonVertex* QuadButtonVertexBufferPtr = nullptr;

		uint32_t CircleButtonIndexCount = 0;
		CircleButtonVertex* CircleButtonVertexBufferBase = nullptr;
		CircleButtonVertex* CircleButtonVertexBufferPtr = nullptr;

		float LineWidth = 2.0f;

		std::array<RefPtr<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		std::array<RefPtr<Texture2D>, MaxTextureSlots> FontAtlasTextures;
		uint32_t FontAtlasIndex = 1; // 0 = white texture

		rtmcpp::Vec4 TriangleVertexPositions[3];
		rtmcpp::Vec4 QuadVertexPositions[4];

		Renderer2D::Statistics Stats;

		struct CameraData
		{
			rtmcpp::Mat4 ViewProjection;
		};

		CameraData CameraBuffer;
		RefPtr<UniformBuffer> CameraUniformBuffer;
	};

	static Renderer2DData s_Data;

	void Renderer2D::Init()
	{
		//NZ_PROFILE_FUNCTION();

		// Triangles
		s_Data.TriangleVertexArray = VertexArray::Create();

		s_Data.TriangleVertexBuffer = VertexBuffer::Create(s_Data.MaxTriangleVertices * sizeof(TriangleVertex));
		s_Data.TriangleVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"              },
			{ ShaderDataType::Float4, "a_Color"                 },
			{ ShaderDataType::Float2, "a_TexCoord"              },
			{ ShaderDataType::Float2, "a_TilingFactor"          },
			{ ShaderDataType::Int,    "a_TexIndex"              },
			{ ShaderDataType::Float,  "a_Saturation"            },
			{ ShaderDataType::Int,    "a_EntityID"              }
		});
		s_Data.TriangleVertexArray->AddVertexBuffer(s_Data.TriangleVertexBuffer);

		s_Data.TriangleVertexBufferBase = new TriangleVertex[s_Data.MaxTriangleVertices];

		uint32_t* triangleIndices = new uint32_t[s_Data.MaxTriangleIndices];

		uint32_t triangleOffset = 0;
		for (uint32_t i = 0; i < s_Data.MaxTriangleIndices; i += 3)
		{
			triangleIndices[i + 0] = triangleOffset + 0;
			triangleIndices[i + 1] = triangleOffset + 1;
			triangleIndices[i + 2] = triangleOffset + 2;

			triangleOffset += 3;
		}

		RefPtr<IndexBuffer> triIB = IndexBuffer::Create(triangleIndices, s_Data.MaxTriangleIndices);
		s_Data.TriangleVertexArray->SetIndexBuffer(triIB);
		delete[] triangleIndices;

		// Quads
		s_Data.QuadVertexArray = VertexArray::Create();

		s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));
		s_Data.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"              },
			{ ShaderDataType::Float4, "a_Color"                 },
			{ ShaderDataType::Float2, "a_TexCoord"              },
			{ ShaderDataType::Float2, "a_TilingFactor"          },
			{ ShaderDataType::Int,    "a_TexIndex"              },
			{ ShaderDataType::Float,  "a_Saturation"            },
			{ ShaderDataType::Int,    "a_EntityID"              }
		});
		s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

		s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

		uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];
		
		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		RefPtr<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
		s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		// Circles
		s_Data.CircleVertexArray = VertexArray::Create();

		s_Data.CircleVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(CircleVertex));
		s_Data.CircleVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_WorldPosition" },
			{ ShaderDataType::Float4, "a_LocalPosition" },
			{ ShaderDataType::Float4, "a_Color"         },
			{ ShaderDataType::Float,  "a_Thickness"     },
			{ ShaderDataType::Float,  "a_Fade"          },
			{ ShaderDataType::Float2, "a_WorldScale"    },
			{ ShaderDataType::Float2, "a_TexCoord"      },
			{ ShaderDataType::Float2, "a_TilingFactor"  },
			{ ShaderDataType::Int,    "a_TexIndex"      },
			{ ShaderDataType::Int,    "a_EntityID"      }
		});
		s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);

		s_Data.CircleVertexArray->SetIndexBuffer(quadIB);
		s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxVertices];

		// Lines
		s_Data.LineVertexArray = VertexArray::Create();

		s_Data.LineVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(LineVertex));
		s_Data.LineVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    },
			{ ShaderDataType::Int,    "a_EntityID" }
		});
		s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
		s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxVertices];

		// Text
		s_Data.TextVertexArray = VertexArray::Create();

		s_Data.TextVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(TextVertex));
		s_Data.TextVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"     },
			{ ShaderDataType::Float4, "a_Color"        },
			{ ShaderDataType::Float4, "a_BgColor"      },
			{ ShaderDataType::Float2, "a_TexCoord"     },
			{ ShaderDataType::Int,    "a_TexIndex"     },
			{ ShaderDataType::Int,    "a_EntityID"     }
		});
		s_Data.TextVertexArray->AddVertexBuffer(s_Data.TextVertexBuffer);
		s_Data.TextVertexArray->SetIndexBuffer(quadIB);
		s_Data.TextVertexBufferBase = new TextVertex[s_Data.MaxVertices];

		// UI Buttons
		s_Data.QuadButtonVertexArray = VertexArray::Create();

		s_Data.QuadButtonVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadButtonVertex));
		s_Data.QuadButtonVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"              },
			{ ShaderDataType::Float4, "a_Color"                 },
			{ ShaderDataType::Float2, "a_TexCoord"              },
			{ ShaderDataType::Float2, "a_TilingFactor"          },
			{ ShaderDataType::Int,    "a_TexIndex"              },
			{ ShaderDataType::Float,  "a_Radius"                },
			{ ShaderDataType::Float2, "a_Dimensions"            },
			{ ShaderDataType::Int,    "a_ShouldInvertUICorners" },
			{ ShaderDataType::Int,    "a_EntityID"              }
		});
		s_Data.QuadButtonVertexArray->AddVertexBuffer(s_Data.QuadButtonVertexBuffer);

		s_Data.QuadButtonVertexArray->SetIndexBuffer(quadIB);
		s_Data.QuadButtonVertexBufferBase = new QuadButtonVertex[s_Data.MaxVertices];

		// UI Circles
		s_Data.CircleButtonVertexArray = VertexArray::Create();

		s_Data.CircleButtonVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(CircleVertex));
		s_Data.CircleButtonVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_WorldPosition" },
			{ ShaderDataType::Float4, "a_LocalPosition" },
			{ ShaderDataType::Float4, "a_Color"         },
			{ ShaderDataType::Float,  "a_Thickness"     },
			{ ShaderDataType::Float,  "a_Fade"          },
			{ ShaderDataType::Float2, "a_WorldScale"    },
			{ ShaderDataType::Float2, "a_TexCoord"      },
			{ ShaderDataType::Float2, "a_TilingFactor"  },
			{ ShaderDataType::Int,    "a_TexIndex"      },
			{ ShaderDataType::Int,    "a_EntityID"      }
		});
		s_Data.CircleButtonVertexArray->AddVertexBuffer(s_Data.CircleButtonVertexBuffer);

		s_Data.CircleButtonVertexArray->SetIndexBuffer(quadIB);
		s_Data.CircleButtonVertexBufferBase = new CircleButtonVertex[s_Data.MaxVertices];

		s_Data.WhiteTexture = Texture2D::Create(TextureSpecification());
		s_Data.WhiteTexture->SetLinear(true);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(Buffer(&whiteTextureData, sizeof(uint32_t)));

		//s_Data.QuadShader = Shader::Create("assets/shaders/Renderer2D_Quad.glsl");
		s_Data.QuadShader = Shader::Create("assets/shaders/Renderer2D_Quad_BlackWhite.glsl");
		s_Data.CircleShader = Shader::Create("assets/shaders/Renderer2D_Circle.glsl"); // Doesn't work properly right now!
		s_Data.LineShader = Shader::Create("assets/shaders/Renderer2D_Line.glsl");
		s_Data.TextShader = Shader::Create("assets/shaders/Renderer2D_Text.glsl");
		s_Data.QuadButtonShader = Shader::Create("assets/shaders/Renderer2D_RoundedQuadButton.glsl");
		s_Data.CircleButtonShader = Shader::Create("assets/shaders/Renderer2D_CircleButton.glsl"); // Doesn't work properly right now!

		// Set first texture slot to 0
		s_Data.TextureSlots[0] = s_Data.WhiteTexture;
		s_Data.FontAtlasTextures[0] = s_Data.WhiteTexture;

		s_Data.TriangleVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.TriangleVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.TriangleVertexPositions[2] = {  0.0f,  0.5f, 0.0f, 1.0f };

		s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

		s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DData::CameraData), 0);
	}

	void Renderer2D::Shutdown()
	{
		//NZ_PROFILE_FUNCTION();

		delete[] s_Data.TriangleVertexBufferBase;
		delete[] s_Data.QuadVertexBufferBase;
		delete[] s_Data.CircleVertexBufferBase;
		delete[] s_Data.LineVertexBufferBase;
		delete[] s_Data.TextVertexBufferBase;
		delete[] s_Data.QuadButtonVertexBufferBase;
		delete[] s_Data.CircleButtonVertexBufferBase;
	}

	void Renderer2D::BeginScene(const Camera& camera, const rtmcpp::Mat4& transform)
	{
		//NZ_PROFILE_FUNCTION();

		//s_Data.CameraBuffer.ViewProjection = rtmcpp::Inverse(transform) * camera.GetProjection();
		s_Data.CameraBuffer.ViewProjection = rtmcpp::Inverse(transform) * camera.GetProjection();
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		StartBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		//NZ_PROFILE_FUNCTION();

		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		StartBatch();
	}

	void Renderer2D::EndScene()
	{
		//NZ_PROFILE_FUNCTION();

		Flush();
	}

	void Renderer2D::StartBatch()
	{
		NZ_PROFILE_FUNCTION("Renderer2D::StartBatch");

		s_Data.TriangleIndexCount = 0;
		s_Data.TriangleVertexBufferPtr = s_Data.TriangleVertexBufferBase;

		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.CircleIndexCount = 0;
		s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;

		s_Data.LineVertexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;
		
		s_Data.TextIndexCount = 0;
		s_Data.TextVertexBufferPtr = s_Data.TextVertexBufferBase;
		
		s_Data.QuadButtonIndexCount = 0;
		s_Data.QuadButtonVertexBufferPtr = s_Data.QuadButtonVertexBufferBase;

		s_Data.CircleButtonIndexCount = 0;
		s_Data.CircleButtonVertexBufferPtr = s_Data.CircleButtonVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
		s_Data.FontAtlasIndex = 1;
	}

	void Renderer2D::Flush()
	{
		NZ_PROFILE_FUNCTION("Renderer2D::Flush");

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

			s_Data.QuadShader->Bind();
			RenderCommand::DrawIndexed(s_Data.TriangleVertexArray, s_Data.TriangleIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		// Draw quads
		if (s_Data.QuadIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
			s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			{
				if (s_Data.TextureSlots[i])
					s_Data.TextureSlots[i]->Bind(i);
			}

			s_Data.QuadShader->Bind();
			RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		// Draw circles
		if (s_Data.CircleIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
			s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			{
				if (s_Data.TextureSlots[i])
					s_Data.TextureSlots[i]->Bind(i);
			}

			s_Data.CircleShader->Bind();
			RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		// Draw lines
		if (s_Data.LineVertexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
			s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

			s_Data.LineShader->Bind();
			RenderCommand::SetLineWidth(s_Data.LineWidth);
			RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
			s_Data.Stats.DrawCalls++;
		}

		// Draw text
		if (s_Data.TextIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.TextVertexBufferPtr - (uint8_t*)s_Data.TextVertexBufferBase);
			s_Data.TextVertexBuffer->SetData(s_Data.TextVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.FontAtlasIndex; i++)
			{
				if (s_Data.FontAtlasTextures[i])
					s_Data.FontAtlasTextures[i]->Bind(i);
			}

			s_Data.TextShader->Bind();
			RenderCommand::DrawIndexed(s_Data.TextVertexArray, s_Data.TextIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		// Draw ui buttons
		if (s_Data.QuadButtonIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadButtonVertexBufferPtr - (uint8_t*)s_Data.QuadButtonVertexBufferBase);
			s_Data.QuadButtonVertexBuffer->SetData(s_Data.QuadButtonVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			{
				if (s_Data.TextureSlots[i])
					s_Data.TextureSlots[i]->Bind(i);
			}

			s_Data.QuadButtonShader->Bind();
			RenderCommand::DrawIndexed(s_Data.QuadButtonVertexArray, s_Data.QuadButtonIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		// Draw ui circles
		if (s_Data.CircleButtonIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleButtonVertexBufferPtr - (uint8_t*)s_Data.CircleButtonVertexBufferBase);
			s_Data.CircleButtonVertexBuffer->SetData(s_Data.CircleButtonVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			{
				if (s_Data.TextureSlots[i])
					s_Data.TextureSlots[i]->Bind(i);
			}

			s_Data.CircleButtonShader->Bind();
			RenderCommand::DrawIndexed(s_Data.CircleButtonVertexArray, s_Data.CircleButtonIndexCount);
			s_Data.Stats.DrawCalls++;
		}
	}

	void Renderer2D::NextBatch()
	{
		Flush();
		StartBatch();
	}

#pragma region TriangleRendering

	void Renderer2D::DrawTri(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, int entityID)
	{
		constexpr size_t triangleVertexCount = 3;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f } };
		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		if (s_Data.TriangleIndexCount >= Renderer2DData::MaxTriangleIndices)
			NextBatch();

		for (size_t i = 0; i < triangleVertexCount; i++)
		{
			s_Data.TriangleVertexBufferPtr->Position = s_Data.TriangleVertexPositions[i] * transform;
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.TriangleVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.TriangleVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TriangleVertexBufferPtr->EntityID = entityID;
			s_Data.TriangleVertexBufferPtr++;
		}

		s_Data.TriangleIndexCount += 3;

		s_Data.Stats.TriangleCount++;
	}

	void Renderer2D::DrawTri(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, TriangleRendererComponent& src, int entityID)
	{
		//NZ_PROFILE_FUNCION();
		NZ_CORE_VERIFY(texture);

		constexpr size_t triangleVertexCount = 3;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f } };

		if (s_Data.TriangleIndexCount >= Renderer2DData::MaxTriangleIndices)
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
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;

			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < triangleVertexCount; i++)
		{
			s_Data.TriangleVertexBufferPtr->Position = s_Data.TriangleVertexPositions[i] * transform;
			s_Data.TriangleVertexBufferPtr->Color = src.Color;
			s_Data.TriangleVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.TriangleVertexBufferPtr->TilingFactor = src.UV;
			s_Data.TriangleVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TriangleVertexBufferPtr->Saturation = src.Saturation;
			s_Data.TriangleVertexBufferPtr->EntityID = entityID;
			s_Data.TriangleVertexBufferPtr++;
		}

		s_Data.TriangleIndexCount += 3;

		s_Data.Stats.TriangleCount++;
	}

	void Renderer2D::DrawTriangle(const rtmcpp::Mat4& transform, TriangleRendererComponent& src, int entityID)
	{
		if (src.TextureHandle)
		{
			RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);
			DrawTri(transform, texture, src, entityID);
		}
		else
			DrawTri(transform, src.Color, entityID);
	}

#pragma endregion

#pragma region LegacySpriteRendering

	void Renderer2D::DrawQuad(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, float saturation, int entityID)
	{
		NZ_PROFILE_FUNCTION("Renderer2D::DrawQuad - Color - Legacy");

		constexpr size_t quadVertexCount = 4;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->Saturation = saturation;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, rtmcpp::Vec2 tilingFactor, const rtmcpp::Vec4& tintColor, float saturation, int entityID)
	{
		NZ_PROFILE_FUNCTION("Renderer2D::DrawQuad - Texture - Legacy");
		NZ_CORE_VERIFY(texture);

		constexpr size_t quadVertexCount = 4;
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
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
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;

			if (texture)
				s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			else
				s_Data.TextureSlots[s_Data.TextureSlotIndex] = s_Data.WhiteTexture;

			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->Saturation = saturation;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;

			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

#pragma endregion

#pragma region SpriteRendering

	void Renderer2D::DrawQuad(const rtmcpp::Mat4& transform, SpriteRendererComponent& src, const Timestep& ts, const rtmcpp::Mat4& cameraProjection, int entityID)
	{
		NZ_PROFILE_FUNCTION("Renderer2D::DrawQuad - Texture");
		
		if (src.Texture != AssetManager::GetAsset<Texture2D>(src.TextureHandle))
			src.Texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);

		//NZ_CORE_VERIFY(texture);

		constexpr size_t quadVertexCount = 4;
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		int textureIndex = 0;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (src.Texture)
			{
				if (*s_Data.TextureSlots[i] == *src.Texture)
				{
					textureIndex = i;
					break;
				}
			}
			else
			{
				if (*s_Data.TextureSlots[i] == *s_Data.WhiteTexture)
				{
					textureIndex = i;
					break;
				}
			}
		}

		if (textureIndex == 0)
		{
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;

			if (src.Texture)
				s_Data.TextureSlots[s_Data.TextureSlotIndex] = src.Texture;
			else
				s_Data.TextureSlots[s_Data.TextureSlotIndex] = s_Data.WhiteTexture;

			s_Data.TextureSlotIndex++;
		}

		auto& animSrc = src.m_AnimationData;
		UpdateAnimation(animSrc.NumberOfTiles, animSrc.StartIndexX, animSrc.StartIndexY, animSrc.AnimationSpeed * ts, animSrc.AnimationTime, animSrc.UseTextureAtlasAnimation, animSrc.UseCameraParallaxY);

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			if (s_AnimateInRuntime || s_AnimateInEdit)
			{
				s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[i] * transform;
				s_Data.QuadVertexBufferPtr->Color = src.Color;

				float tmpTileX = fmod((float)tileIndexX, (float)animSrc.Rows);
				float tmpTileY = fmod((float)tileIndexY, (float)animSrc.Columns);

				if (!animSrc.UseParallaxScrolling && animSrc.UseTextureAtlasAnimation && src.UV.X >= 0.01f && src.UV.Y >= 0.01f)
				{
					s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
					s_Data.QuadVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
					s_Data.QuadVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;
				}				
				else if (animSrc.UseParallaxScrolling)
				{
					//rtmcpp::Vec4 pos = cameraProjection.Value.z_axis;
					//rtmcpp::Vec4 localPos = transform.Value.z_axis;
					rtmcpp::Vec4 pos = cameraProjection.Value.w_axis;
					rtmcpp::Vec4 localPos = transform.Value.w_axis;

					float tilingXCoords = textureCoords[i].X + src.UV.X;
					float tilingYCoords = textureCoords[i].Y + src.UV.Y;

					if (!animSrc.UseTextureAtlasAnimation)
					{
						if (animSrc.ParallaxDivision > 0.0f && !animSrc.UseCameraParallaxX && !animSrc.UseCameraParallaxY)
						{
							if (src.m_AnimationData.ParallaxSpeed.X != 0.0f)
							{
								animSrc.ScrollingDivision.X += (src.m_AnimationData.ParallaxSpeed.X / animSrc.ParallaxDivision) * ts;
								s_Data.QuadVertexBufferPtr->TexCoord.X = tilingXCoords + animSrc.ScrollingDivision.X;
							}
							else
							{
								s_Data.QuadVertexBufferPtr->TexCoord.X = textureCoords[i].X;
							}

							if (src.m_AnimationData.ParallaxSpeed.Y != 0.0f)
							{
								animSrc.ScrollingDivision.Y += (src.m_AnimationData.ParallaxSpeed.Y / animSrc.ParallaxDivision) * ts;
								s_Data.QuadVertexBufferPtr->TexCoord.Y = tilingYCoords + animSrc.ScrollingDivision.Y;
							}
							else
							{
								s_Data.QuadVertexBufferPtr->TexCoord.Y = textureCoords[i].Y;
							}
						}
						else if (animSrc.ParallaxDivision > 0.0f && (animSrc.UseCameraParallaxX || animSrc.UseCameraParallaxY))
						{
							if (animSrc.UseCameraParallaxX)
							{
								animSrc.ScrollingDivision.X = (pos.X - localPos.X) / animSrc.ParallaxDivision;
								s_Data.QuadVertexBufferPtr->TexCoord.X = tilingXCoords + animSrc.ScrollingDivision.X;
							}
							else
							{
								s_Data.QuadVertexBufferPtr->TexCoord.X = textureCoords[i].X;
							}

							if (animSrc.UseCameraParallaxY)
							{
								animSrc.ScrollingDivision.Y = (pos.Y - localPos.Y) / animSrc.ParallaxDivision;
								s_Data.QuadVertexBufferPtr->TexCoord.Y = tilingYCoords + animSrc.ScrollingDivision.Y;
							}
							else
							{
								s_Data.QuadVertexBufferPtr->TexCoord.Y = textureCoords[i].Y;
							}
						}

						s_Data.QuadVertexBufferPtr->TilingFactor = src.UV;
					}
					else if (animSrc.UseTextureAtlasAnimation && animSrc.ParallaxDivision > 0.0f)
					{
						if (animSrc.UseCameraParallaxX)
						{
							animSrc.ScrollingDivision.X = (pos.X - localPos.X) / animSrc.ParallaxDivision;
							s_Data.QuadVertexBufferPtr->TexCoord.X = tilingXCoords + animSrc.ScrollingDivision.X;
							s_Data.QuadVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
						}
						else
						{
							s_Data.QuadVertexBufferPtr->TexCoord.X = textureCoords[i].X;
						}

						if (animSrc.UseCameraParallaxY)
						{
							animSrc.ScrollingDivision.Y = (pos.Y - localPos.Y) / animSrc.ParallaxDivision;
							s_Data.QuadVertexBufferPtr->TexCoord.Y = tilingYCoords + animSrc.ScrollingDivision.Y;
							s_Data.QuadVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;
						}
						else
						{
							s_Data.QuadVertexBufferPtr->TexCoord.Y = textureCoords[i].Y;
						}
					}
				}

				if (animSrc.UseTextureAtlasAnimation && src.UV.X >= 0.01f && src.UV.Y >= 0.01f && animSrc.Rows > 0 && animSrc.Columns > 0)
				{
					s_Data.QuadVertexBufferPtr->TilingFactor = src.UV;
					s_Data.QuadVertexBufferPtr->TilingFactor.X = src.UV.X / (float)animSrc.Rows;
					s_Data.QuadVertexBufferPtr->TilingFactor.Y = src.UV.Y / (float)animSrc.Columns;
				}

				s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
				s_Data.QuadVertexBufferPtr->Saturation = src.Saturation;
				s_Data.QuadVertexBufferPtr->EntityID = entityID;
				s_Data.QuadVertexBufferPtr++;
			}
			else if (!s_AnimateInRuntime && !s_AnimateInEdit)
			{
				s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[i] * transform;
				s_Data.QuadVertexBufferPtr->Color = src.Color;

				if (src.UV.X >= 0.01f && src.UV.Y >= 0.01f && animSrc.Rows > 0 && animSrc.Columns > 0)
				{
					float tmpTileX = fmod((float)tileIndexX, (float)animSrc.Rows);
					float tmpTileY = fmod((float)tileIndexY, (float)animSrc.Columns);

					s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
					s_Data.QuadVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
					s_Data.QuadVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;

					if (animSrc.UseParallaxScrolling)// && !animSrc.UseTextureAtlasAnimation)
					{
						rtmcpp::Vec4 pos = cameraProjection.Value.w_axis;
						rtmcpp::Vec4 localPos = transform.Value.w_axis;

						float tilingXCoords = textureCoords[i].X + src.UV.X;
						float tilingYCoords = textureCoords[i].Y + src.UV.Y;

						if (!animSrc.UseTextureAtlasAnimation)
						{
							if (animSrc.ParallaxDivision > 0.0f && !animSrc.UseCameraParallaxX && !animSrc.UseCameraParallaxY)
							{
								if (src.m_AnimationData.ParallaxSpeed.X != 0.0f)
								{
									//animSrc.ScrollingDivision.X += (src.m_AnimationData.ParallaxSpeed.X / animSrc.ParallaxDivision) * ts;
									animSrc.ScrollingDivision.X = (src.m_AnimationData.ParallaxSpeed.X / animSrc.ParallaxDivision) * ts;
									s_Data.QuadVertexBufferPtr->TexCoord.X = tilingXCoords + animSrc.ScrollingDivision.X;
								}
								else
								{
									s_Data.QuadVertexBufferPtr->TexCoord.X = textureCoords[i].X;
								}

								if (src.m_AnimationData.ParallaxSpeed.Y != 0.0f)
								{
									//animSrc.ScrollingDivision.Y += (src.m_AnimationData.ParallaxSpeed.Y / animSrc.ParallaxDivision) * ts;
									animSrc.ScrollingDivision.Y = (src.m_AnimationData.ParallaxSpeed.Y / animSrc.ParallaxDivision) * ts;
									s_Data.QuadVertexBufferPtr->TexCoord.Y = tilingYCoords + animSrc.ScrollingDivision.Y;
								}
								else
								{
									s_Data.QuadVertexBufferPtr->TexCoord.Y = textureCoords[i].Y;
								}
							}
							else if (animSrc.ParallaxDivision > 0.0f && (animSrc.UseCameraParallaxX || animSrc.UseCameraParallaxY))
							{
								if (animSrc.UseCameraParallaxX)
								{
									animSrc.ScrollingDivision.X = (pos.X - localPos.X) / animSrc.ParallaxDivision;
									s_Data.QuadVertexBufferPtr->TexCoord.X = tilingXCoords + animSrc.ScrollingDivision.X;
								}

								if (animSrc.UseCameraParallaxY)
								{
									animSrc.ScrollingDivision.Y = (pos.Y - localPos.Y) / animSrc.ParallaxDivision;
									s_Data.QuadVertexBufferPtr->TexCoord.Y = tilingYCoords + animSrc.ScrollingDivision.Y;
								}
							}

							s_Data.QuadVertexBufferPtr->TilingFactor = src.UV;
						}
						else if (animSrc.UseTextureAtlasAnimation && animSrc.ParallaxDivision > 0.0f)
						{
							if (animSrc.UseCameraParallaxX)
							{
								animSrc.ScrollingDivision.X = (pos.X - localPos.X) / animSrc.ParallaxDivision;
								s_Data.QuadVertexBufferPtr->TexCoord.X = tilingXCoords + animSrc.ScrollingDivision.X;
								//s_Data.QuadVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
								s_Data.QuadVertexBufferPtr->TexCoord.X = tmpTileX / src.UV.X;
							}

							if (animSrc.UseCameraParallaxY)
							{
								animSrc.ScrollingDivision.Y = (pos.Y - localPos.Y) / animSrc.ParallaxDivision;
								s_Data.QuadVertexBufferPtr->TexCoord.Y = tilingYCoords + animSrc.ScrollingDivision.Y;
								//s_Data.QuadVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;
								s_Data.QuadVertexBufferPtr->TexCoord.Y = tmpTileY / src.UV.Y;
							}
						}

						//s_Data.QuadVertexBufferPtr->TilingFactor = src.UV;
					}
					else if (!animSrc.UseParallaxScrolling || animSrc.UseTextureAtlasAnimation)
					{
						s_Data.QuadVertexBufferPtr->TilingFactor = src.UV;
						s_Data.QuadVertexBufferPtr->TilingFactor.X /= (float)animSrc.Rows;
						s_Data.QuadVertexBufferPtr->TilingFactor.Y /= (float)animSrc.Columns;
					}
				}

				animSrc.ScrollingDivision.X = 0.0f;
				animSrc.ScrollingDivision.Y = 0.0f;

				s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
				s_Data.QuadVertexBufferPtr->Saturation = src.Saturation;
				s_Data.QuadVertexBufferPtr->EntityID = entityID;

				s_Data.QuadVertexBufferPtr++;
			}
		}

		s_Data.QuadIndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawAnimatedQuad(const rtmcpp::Mat4& transform, SpriteRendererComponent& src, const Timestep& ts, int entityID)
	{
		NZ_PROFILE_FUNCTION("Renderer2D::DrawAnimatedQuad");

		auto& animSrc = src.m_AnimationData;
		
		constexpr size_t quadVertexCount = 4;
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		int textureIndex = 0;

		UpdateTextureAnimation((int)animSrc.Textures.size(), animSrc.StartIndex, animSrc.AnimationSpeed * ts, animSrc.AnimationTime, src.m_AnimationData.UsePerTextureAnimation);
		
		if (s_AnimateInRuntime || s_AnimateInEdit)
		{
			if (src.m_AnimationData.UsePerTextureAnimation)
			{
				RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(animSrc.Textures[(uint32_t)tileIndexX]);

				//NZ_CORE_VERIFY(texture);

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
					if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
						NextBatch();

					textureIndex = s_Data.TextureSlotIndex;

					if (texture)
						s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
					else
						s_Data.TextureSlots[s_Data.TextureSlotIndex] = s_Data.WhiteTexture;

					s_Data.TextureSlotIndex++;
				}
			}
			else
			{
				RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(animSrc.Textures[(uint32_t)animSrc.StartIndex]);
				
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
					if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
						NextBatch();

					textureIndex = s_Data.TextureSlotIndex;

					if (texture)
						s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
					else
						s_Data.TextureSlots[s_Data.TextureSlotIndex] = s_Data.WhiteTexture;

					s_Data.TextureSlotIndex++;
				}
			}
		}
		else if (!s_AnimateInRuntime && !s_AnimateInEdit)
		{
			RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(animSrc.Textures[(uint32_t)animSrc.StartIndex]);

			//NZ_CORE_VERIFY(texture);

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
				if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
					NextBatch();

				textureIndex = s_Data.TextureSlotIndex;

			if (texture)
				s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			else
				s_Data.TextureSlots[s_Data.TextureSlotIndex] = s_Data.WhiteTexture;

				s_Data.TextureSlotIndex++;
			}
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadVertexBufferPtr->Color = src.Color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TilingFactor = src.UV;

			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;

			s_Data.QuadVertexBufferPtr->Saturation = src.Saturation;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;

			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawSprite(const rtmcpp::Mat4& transform, SpriteRendererComponent& src, const Timestep& ts, bool UseTextureAnimation, const rtmcpp::Mat4& cameraProjection, int entityID)
	{
		NZ_PROFILE_FUNCTION("Renderer2D::DrawSprite");

		if (!UseTextureAnimation)
		{
			if (src.TextureHandle)
				DrawQuad(transform, src, ts, cameraProjection, entityID);
			else
				DrawQuad(transform, src.Color, src.Saturation, entityID);
		}
		else
		{
			if (src.m_AnimationData.Textures[0])
				DrawAnimatedQuad(transform, src, ts, entityID);
			else
				DrawQuad(transform, src.Color, src.Saturation, entityID);
		}
	}

#pragma endregion

#pragma region CircleRendering

	void Renderer2D::DrawCircle(const rtmcpp::Mat4& transform, CircleRendererComponent& src, const Timestep& ts, const rtmcpp::Mat4& cameraProjection, int entityID)
	{
		//NZ_PROFILE_FUNCTION();

		if (src.TextureHandle)
			DrawCircle(transform, src, ts, cameraProjection, src.Thickness, src.Fade, entityID);
		else
			DrawCircle(transform, src.Color, src.Thickness, src.Fade, entityID);
	}

	void Renderer2D::DrawCircle(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, float thickness, float fade, int entityID)
	{
		//NZ_PROFILE_FUNCTION();

		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		// TODO: implement for circles
		if (s_Data.CircleIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		rtmcpp::Vec3 scale;
		Math::DecomposeTransformScale(transform, scale);

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.CircleVertexBufferPtr->WorldPosition = s_Data.QuadVertexPositions[i] * transform;
			s_Data.CircleVertexBufferPtr->LocalPosition = rtmcpp::Vec4{ s_Data.QuadVertexPositions[i].X * 2.0f, s_Data.QuadVertexPositions[i].Y * 2.0f, s_Data.QuadVertexPositions[i].Z * 2.0f, s_Data.QuadVertexPositions[i].W * 2.0f };
			s_Data.CircleVertexBufferPtr->Color = color;
			s_Data.CircleVertexBufferPtr->Thickness = thickness;
			s_Data.CircleVertexBufferPtr->Fade = fade;
			s_Data.CircleVertexBufferPtr->WorldScale = rtmcpp::Vec2{ scale.X, scale.Y };
			s_Data.CircleVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.CircleVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.CircleVertexBufferPtr->TexIndex = textureIndex;
			s_Data.CircleVertexBufferPtr->EntityID = entityID;
			s_Data.CircleVertexBufferPtr++;
		}

		s_Data.CircleIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawCircle(const rtmcpp::Mat4& transform, CircleRendererComponent& src, const Timestep& ts, const rtmcpp::Mat4& cameraProjection, float thickness, float fade, int entityID)
	{
		NZ_PROFILE_FUNCTION("Renderer2D::DrawCircle - Texture");

		if (src.Texture != AssetManager::GetAsset<Texture2D>(src.TextureHandle))
			src.Texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);

		//NZ_CORE_VERIFY(texture);

		constexpr size_t quadVertexCount = 4;
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.CircleIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		int textureIndex = 0;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *src.Texture)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;

			if (src.Texture)
				s_Data.TextureSlots[s_Data.TextureSlotIndex] = src.Texture;
			else
				s_Data.TextureSlots[s_Data.TextureSlotIndex] = s_Data.WhiteTexture;

			s_Data.TextureSlotIndex++;
		}

		rtmcpp::Vec3 scale;
		Math::DecomposeTransformScale(transform, scale);

		auto& animSrc = src.m_AnimationData;
		UpdateAnimation(animSrc.NumberOfTiles, animSrc.StartIndexX, animSrc.StartIndexY, animSrc.AnimationSpeed * ts, animSrc.AnimationTime, animSrc.UseTextureAtlasAnimation, animSrc.UseCameraParallaxY);

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			if (s_AnimateInRuntime || s_AnimateInEdit)
			{
				s_Data.CircleVertexBufferPtr->WorldPosition = s_Data.QuadVertexPositions[i] * transform;
				s_Data.CircleVertexBufferPtr->LocalPosition = rtmcpp::Vec4{ s_Data.QuadVertexPositions[i].X * 2.0f, s_Data.QuadVertexPositions[i].Y * 2.0f, s_Data.QuadVertexPositions[i].Z * 2.0f, s_Data.QuadVertexPositions[i].W * 2.0f };
				s_Data.CircleVertexBufferPtr->Color = src.Color;
				s_Data.CircleVertexBufferPtr->Thickness = thickness;
				s_Data.CircleVertexBufferPtr->Fade = fade;
				s_Data.CircleVertexBufferPtr->WorldScale = rtmcpp::Vec2{ scale.X, scale.Y };

				float tmpTileX = fmod((float)tileIndexX, (float)animSrc.Rows);
				float tmpTileY = fmod((float)tileIndexY, (float)animSrc.Columns);

				if (!animSrc.UseParallaxScrolling && animSrc.UseTextureAtlasAnimation && src.UV.X >= 0.01f && src.UV.Y >= 0.01f)
				{
					s_Data.CircleVertexBufferPtr->TexCoord = textureCoords[i];
					s_Data.CircleVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
					s_Data.CircleVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;
				}
				else if (animSrc.UseParallaxScrolling)
				{
					rtmcpp::Vec4 cameraPos = cameraProjection.Value.w_axis;
					rtmcpp::Vec4 localPos = transform.Value.w_axis;

					float tilingXCoords = textureCoords[i].X + src.UV.X;
					float tilingYCoords = textureCoords[i].Y + src.UV.Y;

					if (!animSrc.UseTextureAtlasAnimation)
					{
						if (animSrc.ParallaxDivision > 0.0f && !animSrc.UseCameraParallaxX && !animSrc.UseCameraParallaxY)
						{
							if (src.m_AnimationData.ParallaxSpeed.X != 0.0f)
							{
								animSrc.ScrollingDivision.X += (src.m_AnimationData.ParallaxSpeed.X / animSrc.ParallaxDivision) * ts;
								s_Data.CircleVertexBufferPtr->TexCoord.X = tilingXCoords + animSrc.ScrollingDivision.X;
							}
							else
							{
								s_Data.CircleVertexBufferPtr->TexCoord.X = textureCoords[i].X;
							}

							if (src.m_AnimationData.ParallaxSpeed.Y != 0.0f)
							{
								animSrc.ScrollingDivision.Y += (src.m_AnimationData.ParallaxSpeed.Y / animSrc.ParallaxDivision) * ts;
								s_Data.CircleVertexBufferPtr->TexCoord.Y = tilingYCoords + animSrc.ScrollingDivision.Y;
							}
							else
							{
								s_Data.CircleVertexBufferPtr->TexCoord.Y = textureCoords[i].Y;
							}
						}
						else if (animSrc.ParallaxDivision > 0.0f && (animSrc.UseCameraParallaxX || animSrc.UseCameraParallaxY))
						{
							if (animSrc.UseCameraParallaxX)
							{
								animSrc.ScrollingDivision.X = (cameraPos.X - localPos.X) / animSrc.ParallaxDivision;
								s_Data.CircleVertexBufferPtr->TexCoord.X = tilingXCoords + animSrc.ScrollingDivision.X;
							}

							if (animSrc.UseCameraParallaxY)
							{
								animSrc.ScrollingDivision.Y = (cameraPos.Y - localPos.Y) / animSrc.ParallaxDivision;
								s_Data.CircleVertexBufferPtr->TexCoord.Y = tilingYCoords + animSrc.ScrollingDivision.Y;
							}
						}

						s_Data.CircleVertexBufferPtr->TilingFactor = src.UV;
					}
					else if (animSrc.UseTextureAtlasAnimation && animSrc.ParallaxDivision > 0.0f)
					{
						if (animSrc.UseCameraParallaxX)
						{
							animSrc.ScrollingDivision.X = (cameraPos.X - localPos.X) / animSrc.ParallaxDivision;
							s_Data.CircleVertexBufferPtr->TexCoord.X = tilingXCoords + animSrc.ScrollingDivision.X;
							s_Data.CircleVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
						}

						if (animSrc.UseCameraParallaxY)
						{
							animSrc.ScrollingDivision.Y = (cameraPos.Y - localPos.Y) / animSrc.ParallaxDivision;
							s_Data.CircleVertexBufferPtr->TexCoord.Y = tilingYCoords + animSrc.ScrollingDivision.Y;
							s_Data.CircleVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;
						}
					}
				}

				if (animSrc.UseTextureAtlasAnimation && src.UV.X >= 0.01f && src.UV.Y >= 0.01f && animSrc.Rows > 0 && animSrc.Columns > 0)
				{
					s_Data.CircleVertexBufferPtr->TilingFactor = src.UV;
					s_Data.CircleVertexBufferPtr->TilingFactor.X = src.UV.X / (float)animSrc.Rows;
					s_Data.CircleVertexBufferPtr->TilingFactor.Y = src.UV.Y / (float)animSrc.Columns;
				}

				s_Data.CircleVertexBufferPtr->TexIndex = textureIndex;
				s_Data.CircleVertexBufferPtr->EntityID = entityID;
				s_Data.CircleVertexBufferPtr++;
			}
			else if (!s_AnimateInRuntime && !s_AnimateInEdit)
			{
				s_Data.CircleVertexBufferPtr->WorldPosition = s_Data.QuadVertexPositions[i] * transform;
				s_Data.CircleVertexBufferPtr->LocalPosition = rtmcpp::Vec4{ s_Data.QuadVertexPositions[i].X * 2.0f, s_Data.QuadVertexPositions[i].Y * 2.0f, s_Data.QuadVertexPositions[i].Z * 2.0f, s_Data.QuadVertexPositions[i].W * 2.0f };
				s_Data.CircleVertexBufferPtr->Color = src.Color;
				s_Data.CircleVertexBufferPtr->Thickness = thickness;
				s_Data.CircleVertexBufferPtr->Fade = fade;
				s_Data.CircleVertexBufferPtr->WorldScale = rtmcpp::Vec2{ scale.X, scale.Y };

				if (src.UV.X >= 0.01f && src.UV.Y >= 0.01f && animSrc.Rows > 0 && animSrc.Columns > 0)
				{
					float tmpTileX = fmod((float)tileIndexX, (float)animSrc.Rows);
					float tmpTileY = fmod((float)tileIndexY, (float)animSrc.Columns);

					s_Data.CircleVertexBufferPtr->TexCoord = textureCoords[i];
					s_Data.CircleVertexBufferPtr->TexCoord.X += tmpTileX / src.UV.X;
					s_Data.CircleVertexBufferPtr->TexCoord.Y += tmpTileY / src.UV.Y;

					if (animSrc.UseParallaxScrolling && !animSrc.UseTextureAtlasAnimation)
					{
						s_Data.CircleVertexBufferPtr->TilingFactor = src.UV;
					}
					else
					{
						s_Data.CircleVertexBufferPtr->TilingFactor = src.UV;
						s_Data.CircleVertexBufferPtr->TilingFactor.X /= (float)animSrc.Rows;
						s_Data.CircleVertexBufferPtr->TilingFactor.Y /= (float)animSrc.Columns;
					}
				}

				animSrc.ScrollingDivision.X = 0.0f;
				animSrc.ScrollingDivision.Y = 0.0f;

				s_Data.CircleVertexBufferPtr->TexIndex = textureIndex;
				s_Data.CircleVertexBufferPtr->EntityID = entityID;

				s_Data.CircleVertexBufferPtr++;
			}
		}

		s_Data.CircleIndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

#pragma endregion

#pragma region MiscRendering

	void Renderer2D::DrawLine(const rtmcpp::Vec4& p0, const rtmcpp::Vec4& p1, const rtmcpp::Vec4& color, int entityID)
	{
		//NZ_PROFILE_FUNCTION();

		s_Data.LineVertexBufferPtr->Position = p0;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr->EntityID = entityID;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexBufferPtr->Position = p1;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr->EntityID = entityID;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexCount += 2;
	}

	void Renderer2D::DrawLine(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& p0, const rtmcpp::Vec4& p1, const rtmcpp::Vec4& color, int entityID)
	{
		//NZ_PROFILE_FUNCTION();
		rtmcpp::Vec4 lineVertices0 = rtmcpp::Vec4{ p0.X, p0.Y, p0.Z, 1.0f } * transform;
		rtmcpp::Vec4 lineVertices1 = rtmcpp::Vec4{ p1.X, p1.Y, p1.Z, 1.0f } * transform;
		
		DrawLine(lineVertices0, lineVertices1, color);
	}

	void Renderer2D::DrawRect(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, int entityID)
	{
		//NZ_PROFILE_FUNCTION();

		rtmcpp::Vec4 lineVertices[4];
		for (size_t i = 0; i < 4; i++)
			lineVertices[i] = s_Data.QuadVertexPositions[i] * transform;

		DrawLine(lineVertices[0], lineVertices[1], color, entityID);
		DrawLine(lineVertices[1], lineVertices[2], color, entityID);
		DrawLine(lineVertices[2], lineVertices[3], color, entityID);
		DrawLine(lineVertices[3], lineVertices[0], color, entityID);
	}

	void Renderer2D::DrawCircleLine(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, float thickness, float fade, int entityID)
	{
		//NZ_PROFILE_FUNCTION();

		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		rtmcpp::Vec3 scale;
		Math::DecomposeTransformScale(transform, scale);

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.CircleVertexBufferPtr->WorldPosition = s_Data.QuadVertexPositions[i] * transform;
			s_Data.CircleVertexBufferPtr->LocalPosition = rtmcpp::Vec4{ s_Data.QuadVertexPositions[i].X * 2.0f, s_Data.QuadVertexPositions[i].Y * 2.0f, s_Data.QuadVertexPositions[i].Z * 2.0f, s_Data.QuadVertexPositions[i].W * 2.0f };
			s_Data.CircleVertexBufferPtr->Color = color;
			s_Data.CircleVertexBufferPtr->Thickness = thickness;
			s_Data.CircleVertexBufferPtr->Fade = fade;
			s_Data.CircleVertexBufferPtr->WorldScale = rtmcpp::Vec2{ scale.X, scale.Y };
			s_Data.CircleVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.CircleVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.CircleVertexBufferPtr->TexIndex = textureIndex;
			s_Data.CircleVertexBufferPtr->EntityID = entityID;
			s_Data.CircleVertexBufferPtr++;
		}

		s_Data.CircleIndexCount += 6;
	}

#pragma endregion

#pragma region UILayouts

	void Renderer2D::DrawQuadWidget(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, float radius, const rtmcpp::Vec2& dimensions, bool invertCorners, int entityID)
	{
		//NZ_PROFILE_FUNCTION();

		constexpr size_t quadVertexCount = 4;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		if (s_Data.QuadButtonIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadButtonVertexBufferPtr->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadButtonVertexBufferPtr->Color = color;
			s_Data.QuadButtonVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadButtonVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadButtonVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadButtonVertexBufferPtr->Radius = radius;
			s_Data.QuadButtonVertexBufferPtr->Dimensions = dimensions;
			s_Data.QuadButtonVertexBufferPtr->ShouldInvertUICorners = invertCorners;
			s_Data.QuadButtonVertexBufferPtr->EntityID = entityID;
			s_Data.QuadButtonVertexBufferPtr++;
		}

		s_Data.QuadButtonIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuadWidget(const rtmcpp::Mat4& transform, ButtonWidgetComponent& src, int entityID)
	{
		//NZ_PROFILE_FUNCTION();

		RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);

		NZ_CORE_VERIFY(texture);

		constexpr size_t quadVertexCount = 4;
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.QuadButtonIndexCount >= Renderer2DData::MaxIndices)
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
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;

			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadButtonVertexBufferPtr->Position = s_Data.QuadVertexPositions[i] * transform;
			s_Data.QuadButtonVertexBufferPtr->Color = src.Color;
			s_Data.QuadButtonVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadButtonVertexBufferPtr->TilingFactor = src.UV;
			s_Data.QuadButtonVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadButtonVertexBufferPtr->Radius = src.Radius;
			s_Data.QuadButtonVertexBufferPtr->Dimensions = src.Dimensions;
			s_Data.QuadButtonVertexBufferPtr->ShouldInvertUICorners = src.InvertCorners;

			s_Data.QuadButtonVertexBufferPtr->EntityID = entityID;
			s_Data.QuadButtonVertexBufferPtr++;
		}

		s_Data.QuadButtonIndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawCircleWidget(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, float thickness, float fade, int entityID)
	{
		//NZ_PROFILE_FUNCTION();

		constexpr size_t quadVertexCount = 4;
		int textureIndex = 0; // White Texture
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		if (s_Data.CircleButtonIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		rtmcpp::Vec3 scale;
		Math::DecomposeTransformScale(transform, scale);

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.CircleButtonVertexBufferPtr->WorldPosition = s_Data.QuadVertexPositions[i] * transform;
			s_Data.CircleButtonVertexBufferPtr->LocalPosition = rtmcpp::Vec4{ s_Data.QuadVertexPositions[i].X * 2.0f, s_Data.QuadVertexPositions[i].Y * 2.0f, s_Data.QuadVertexPositions[i].Z * 2.0f, s_Data.QuadVertexPositions[i].W * 2.0f };
			s_Data.CircleButtonVertexBufferPtr->Color = color;
			s_Data.CircleButtonVertexBufferPtr->Thickness = thickness;
			s_Data.CircleButtonVertexBufferPtr->Fade = fade;
			s_Data.CircleButtonVertexBufferPtr->WorldScale = rtmcpp::Vec2{ scale.X, scale.Y };
			s_Data.CircleButtonVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.CircleButtonVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.CircleButtonVertexBufferPtr->TexIndex = textureIndex;
			s_Data.CircleButtonVertexBufferPtr->EntityID = entityID;
			s_Data.CircleButtonVertexBufferPtr++;
		}

		s_Data.CircleButtonIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawCircleWidget(const rtmcpp::Mat4& transform, CircleWidgetComponent& src, int entityID)
	{
		//NZ_PROFILE_FUNCTION();

		RefPtr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);

		NZ_CORE_VERIFY(texture);

		constexpr size_t quadVertexCount = 4;
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.CircleButtonIndexCount >= Renderer2DData::MaxIndices)
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
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;

			s_Data.TextureSlotIndex++;
		}

		rtmcpp::Vec3 scale;
		Math::DecomposeTransformScale(transform, scale);

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.CircleButtonVertexBufferPtr->WorldPosition = s_Data.QuadVertexPositions[i] * transform;
			s_Data.CircleButtonVertexBufferPtr->LocalPosition = rtmcpp::Vec4{ s_Data.QuadVertexPositions[i].X * 2.0f, s_Data.QuadVertexPositions[i].Y * 2.0f, s_Data.QuadVertexPositions[i].Z * 2.0f, s_Data.QuadVertexPositions[i].W * 2.0f };
			s_Data.CircleButtonVertexBufferPtr->Color = src.Color;
			s_Data.CircleButtonVertexBufferPtr->Thickness = src.Thickness;
			s_Data.CircleButtonVertexBufferPtr->Fade = src.Fade;
			s_Data.CircleButtonVertexBufferPtr->WorldScale = rtmcpp::Vec2{ scale.X, scale.Y };
			s_Data.CircleButtonVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.CircleButtonVertexBufferPtr->TilingFactor = src.UV;
			s_Data.CircleButtonVertexBufferPtr->TexIndex = textureIndex;
			s_Data.CircleButtonVertexBufferPtr->EntityID = entityID;
			s_Data.CircleButtonVertexBufferPtr++;
		}

		s_Data.CircleButtonIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawSpriteWidget(const rtmcpp::Mat4& transform, ButtonWidgetComponent& src, int entityID)
	{
		if (src.TextureHandle)
			DrawQuadWidget(transform, src, entityID);
		else
			DrawQuadWidget(transform, src.Color, src.Radius, src.Dimensions, src.InvertCorners, entityID);
	}

	void Renderer2D::DrawSpriteWidget(const rtmcpp::Mat4& transform, CircleWidgetComponent& src, int entityID)
	{
		if (src.TextureHandle)
			DrawCircleWidget(transform, src, entityID);
		else
			DrawCircleWidget(transform, src.Color, src.Thickness, src.Fade, entityID);
	}

#pragma endregion

#pragma region TextRendering

	// From https://stackoverflow.com/questions/31302506/stdu32string-conversion-to-from-stdstring-and-stdu16string
	static std::u32string To_UTF32(const std::string& s)
	{		
		std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
		return conv.from_bytes(s);
	}

	void Renderer2D::DrawString(const std::string& string, TextComponent& src, const rtmcpp::Mat4& transform, const TextParams& textParams, rtmcpp::Vec2& texQuadMin, rtmcpp::Vec2& texQuadMax, rtmcpp::Vec2& texCoordMin, rtmcpp::Vec2& texCoordMax, rtmcpp::Vec2& planeMin, rtmcpp::Vec2& planeMax, float& newLineCounter, float& yOffset, int entityID)
	{
		RefPtr<Font> fontAsset = nullptr;

		if (src.FontHandle != 0)
			fontAsset = AssetManager::GetAsset<Font>(src.FontHandle);
		else
			fontAsset = Font::GetDefault();

		if (!fontAsset)
		{
			NZ_CORE_ERROR("Font asset is null!");
			return;
		}

		texQuadMin = rtmcpp::Vec2(0.0f, 0.0f);
		texQuadMax = rtmcpp::Vec2(0.0f, 0.0f);

		texCoordMin = rtmcpp::Vec2(0.0f, 0.0f);
		texCoordMax = rtmcpp::Vec2(0.0f, 0.0f);

		if (string.empty())
			return;

		std::u32string utf32string = To_UTF32(string);

		const auto& fontGeometry = fontAsset->GetMSDFData()->FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();

		RefPtr<Texture2D> fontAtlas = fontAsset->GetAtlasTexture();

		int textureIndex = 0;

		for (uint32_t i = 1; i < s_Data.FontAtlasIndex; i++)
		{
			if (*s_Data.FontAtlasTextures[i] == *fontAtlas)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_Data.FontAtlasIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_Data.FontAtlasIndex;
			s_Data.FontAtlasTextures[s_Data.FontAtlasIndex] = fontAtlas;

			s_Data.FontAtlasIndex++;
		}

		double x = 0.0;
		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
		double y = 0.0;

		const double spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();
		float newlines = 0.0f;

		for (size_t i = 0; i < utf32string.size(); i++)
		{
			char32_t character = utf32string[i];
			if (character == '\r')
				continue;

			if (character == '\n')
			{
				newlines++;
				x = 0;
				y -= fsScale * metrics.lineHeight + textParams.LineSpacing;
				continue;
			}

			auto glyph = fontGeometry.getGlyph(character);
			if (!glyph)
				glyph = fontGeometry.getGlyph('?');
			if (!glyph)
				return;

			if (character == ' ')
			{
				float advance = (float)spaceGlyphAdvance;
				if (i < utf32string.size() - 1)
				{
					char nextCharacter = (char)utf32string[i + 1];
					double dAdvance;
					fontGeometry.getAdvance(dAdvance, character, nextCharacter);
					advance = (float)dAdvance;
				}

				x += fsScale * advance + textParams.Kerning;
				continue;
			}

			if (character == '\t')
			{
				x += 4.0f * (fsScale * spaceGlyphAdvance + textParams.Kerning);
				continue;
			}

			double al, ab, ar, at;
			glyph->getQuadAtlasBounds(al, ab, ar, at);
			rtmcpp::Vec2 textCoordMin(static_cast<float>(al), static_cast<float>(ab));
			rtmcpp::Vec2 textCoordMax(static_cast<float>(ar), static_cast<float>(at));
			texCoordMin = rtmcpp::Vec2(static_cast<float>(al), static_cast<float>(ab));
			texCoordMax = rtmcpp::Vec2(static_cast<float>(ar), static_cast<float>(at));

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);
			rtmcpp::Vec2 textQuadMin(static_cast<float>(pl), static_cast<float>(pb));
			rtmcpp::Vec2 textQuadMax(static_cast<float>(pr), static_cast<float>(pt));
			texQuadMin = rtmcpp::Vec2(static_cast<float>(pl), static_cast<float>(pb));
			texQuadMax = rtmcpp::Vec2(static_cast<float>(pr), static_cast<float>(pt));

			textQuadMin *= rtmcpp::Vec2{ static_cast<float>(fsScale), static_cast<float>(fsScale) }, texQuadMax *= rtmcpp::Vec2{ static_cast<float>(fsScale), static_cast<float>(fsScale) };
			textQuadMin += rtmcpp::Vec2(static_cast<float>(x), static_cast<float>(y));
			textQuadMax += rtmcpp::Vec2(static_cast<float>(x), static_cast<float>(y));
			texQuadMin *= rtmcpp::Vec2{ static_cast<float>(fsScale), static_cast<float>(fsScale) }, texQuadMax *= rtmcpp::Vec2{ static_cast<float>(fsScale), static_cast<float>(fsScale) };
			texQuadMin += rtmcpp::Vec2(static_cast<float>(x), static_cast<float>(y));
			texQuadMax += rtmcpp::Vec2(static_cast<float>(x), static_cast<float>(y));

			float texelWidth = 1.0f / fontAtlas->GetWidth();
			float texelHeight = 1.0f / fontAtlas->GetHeight();
			textCoordMin *= rtmcpp::Vec2(texelWidth, texelHeight);
			textCoordMax *= rtmcpp::Vec2(texelWidth, texelHeight);
			texCoordMin *= rtmcpp::Vec2(texelWidth, texelHeight);
			texCoordMax *= rtmcpp::Vec2(texelWidth, texelHeight);

			planeMin = rtmcpp::Vec2{ texCoordMin.X + texCoordMin.X, texCoordMin.Y + texCoordMin.Y };
			planeMax = rtmcpp::Vec2{ texCoordMax.X + texCoordMax.X, texCoordMax.Y + texCoordMax.Y };

			// render here
			s_Data.TextVertexBufferPtr->Position = rtmcpp::Vec4(textQuadMin.X, textQuadMin.Y, 0.0f, 1.0f) * transform;
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->BgColor = textParams.BgColor;
			s_Data.TextVertexBufferPtr->TexCoord = textCoordMin;
			s_Data.TextVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TextVertexBufferPtr->EntityID = entityID;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = rtmcpp::Vec4(textQuadMin.X, textQuadMax.Y, 0.0f, 1.0f) * transform;
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->BgColor = textParams.BgColor;
			s_Data.TextVertexBufferPtr->TexCoord = rtmcpp::Vec2{ textCoordMin.X, textCoordMax.Y };
			s_Data.TextVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TextVertexBufferPtr->EntityID = entityID;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = rtmcpp::Vec4(textQuadMax.X, textQuadMax.Y, 0.0f, 1.0f) * transform;
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->BgColor = textParams.BgColor;
			s_Data.TextVertexBufferPtr->TexCoord = textCoordMax;
			s_Data.TextVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TextVertexBufferPtr->EntityID = entityID;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = rtmcpp::Vec4(textQuadMax.X, textQuadMin.Y, 0.0f, 1.0f) * transform;
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->BgColor = textParams.BgColor;
			s_Data.TextVertexBufferPtr->TexCoord = rtmcpp::Vec2{ textCoordMax.X, textCoordMin.Y };
			s_Data.TextVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TextVertexBufferPtr->EntityID = entityID;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextIndexCount += 6;
			s_Data.Stats.QuadCount++;

			if (i < utf32string.size() - 1)
			{
				double advance = glyph->getAdvance();
				char nextCharacter = (char)utf32string[i + 1];
				fontGeometry.getAdvance(advance, character, nextCharacter);

				x += fsScale * advance + textParams.Kerning;
			}
		}

		newLineCounter = newlines;
	}

	void Renderer2D::DrawString(const std::string& string, const rtmcpp::Mat4& transform, TextComponent& component, int entityID)
	{
		DrawString(string, component, transform, { component.Color, component.BgColor, component.Kerning, component.LineSpacing },
			component.m_TextQuadMin, component.m_TextQuadMax, component.m_TexCoordMin, component.m_TexCoordMax,
			component.m_PlaneMin, component.m_PlaneMax, component.m_NewLineCounter, component.m_YMinOffset, entityID);
	}

#pragma endregion

	float Renderer2D::GetLineWidth()
	{
		return s_Data.LineWidth;
	}

	void Renderer2D::SetLineWidth(float width)
	{
		s_Data.LineWidth = width;
	}

	void Renderer2D::UpdateAnimation(const int& numTiles, const int& startIndexX, const int& startIndexY, const float& animSpeed, float& animationTime, bool useAnimation, bool useCameraParallaxY)
	{
		//NZ_PROFILE_FUNCTION();

		if (useAnimation && (s_AnimateInRuntime || s_AnimateInEdit))
			animationTime += animSpeed;
		else if (!useAnimation)
			animationTime = 0.0f;

		tileIndexX = startIndexX + (int)animationTime % numTiles;

		if (useCameraParallaxY)
			tileIndexY = startIndexY + (int)animationTime % numTiles;
		else
			tileIndexY = startIndexY + (int)animationTime % 1;
	}

	void Renderer2D::UpdateTextureAnimation(const int& numTextures, const int& startIndex, const float& animSpeed, float& animationTime, bool useAnimation)
	{
		NZ_PROFILE_FUNCTION("Renderer2D::UpdateTextureAnimation");

		int numberOfTextures = numTextures;

		if (numberOfTextures > 0)
			numberOfTextures = numTextures - startIndex;

		if (useAnimation)
			animationTime += animSpeed;
		else
			animationTime = 0.0f;

		tileIndexX = startIndex + (int)animationTime % numberOfTextures;
	}

	void Renderer2D::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data.Stats;
	}

}
