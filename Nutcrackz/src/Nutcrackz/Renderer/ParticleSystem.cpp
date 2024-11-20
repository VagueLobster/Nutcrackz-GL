#include "nzpch.hpp"
#include "ParticleSystem.hpp"

#include "Random.hpp"
#include "RenderCommand.hpp"
#include "Renderer.hpp"
#include "Renderer2D.hpp"
#include "UniformBuffer.hpp"

#include "Nutcrackz/Scene/Components.hpp"

//#define GLM_ENABLE_EXPERIMENTAL
//#include <glm/gtx/compatibility.hpp>

#include "rtmcpp/Transforms.hpp"
#include "rtmcpp/PackedVector.hpp"
#include "rtmcpp/VectorOps.hpp"
#include "rtmcpp/Scalar.hpp"

#include <cmath>

namespace Nutcrackz {

	struct Particle
	{
		rtmcpp::Vec4 Position;
		rtmcpp::Vec3 Velocity;
		rtmcpp::Vec4 ColorBegin, ColorEnd;
		rtmcpp::Vec3 Rotation;
		float SizeBegin, SizeEnd;

		float LifeTime = 1.0f;
		float LifeRemaining = 0.0f;

		bool Active = false;
	};

	struct ParticleVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;

		// Editor-only
		int EntityID;
	};

	uint32_t ParticleSystem::s_PoolIndex = 0;
	uint32_t ParticleSystem::s_MaxParticles = 0;
	std::vector<Particle> ParticlePool = std::vector<Particle>();

	rtmcpp::Mat4 s_Transform = rtmcpp::Mat4();
	rtmcpp::Vec4 s_Color = rtmcpp::Vec4(1.0f, 1.0f, 1.0f, 1.0f);

	struct Particle2DData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		// Quads
		RefPtr<VertexArray> QuadVertexArray;
		RefPtr<VertexBuffer> QuadVertexBuffer;
		RefPtr<Shader> ParticleShader;
		RefPtr<Texture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		ParticleVertex* QuadVertexBufferBase = nullptr;
		ParticleVertex* QuadVertexBufferPtr = nullptr;

		std::array<RefPtr<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		rtmcpp::Vec4 QuadVertexPositions[4];

		ParticleSystem::ParticleStats Stats;

		struct CameraData
		{
			rtmcpp::Mat4 ViewProjection;
		};

		rtmcpp::Mat4 m_CameraView;
		CameraData CameraBuffer;
		RefPtr<UniformBuffer> CameraUniformBuffer;

		bool UseBillboard = false;
	};

	static Particle2DData s_ParticleData;

	ParticleSystem::ParticleSystem(uint32_t maxParticles)
	{
		s_MaxParticles = maxParticles;
		s_PoolIndex = maxParticles - 1;
		ParticlePool.resize(maxParticles);
	}

	void ParticleSystem::Init()
	{
		s_ParticleData.QuadVertexArray = VertexArray::Create();

		s_ParticleData.QuadVertexBuffer = VertexBuffer::Create(s_ParticleData.MaxVertices * sizeof(ParticleVertex));
		s_ParticleData.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"     },
			{ ShaderDataType::Float4, "a_Color"        },
			{ ShaderDataType::Float2, "a_TexCoord"     },
			{ ShaderDataType::Float2, "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_TexIndex"     },
			{ ShaderDataType::Int,    "a_EntityID"     }
		});
		s_ParticleData.QuadVertexArray->AddVertexBuffer(s_ParticleData.QuadVertexBuffer);
		s_ParticleData.QuadVertexBufferBase = new ParticleVertex[s_ParticleData.MaxVertices];

		uint32_t* quadIndices = new uint32_t[s_ParticleData.MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_ParticleData.MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		RefPtr<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_ParticleData.MaxIndices);
		s_ParticleData.QuadVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		s_ParticleData.WhiteTexture = Texture2D::Create(TextureSpecification());
		uint32_t whiteTextureData = 0xffffffff;
		s_ParticleData.WhiteTexture->SetData(Buffer(&whiteTextureData, sizeof(uint32_t)));

		int32_t samplers[s_ParticleData.MaxTextureSlots];
		for (uint32_t i = 0; i < s_ParticleData.MaxTextureSlots; i++)
			samplers[i] = i;

		s_ParticleData.ParticleShader = Shader::Create("assets/shaders/Renderer2D_Particle.glsl");

		s_ParticleData.TextureSlots[0] = s_ParticleData.WhiteTexture;

		s_ParticleData.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_ParticleData.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		s_ParticleData.QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
		s_ParticleData.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

		s_ParticleData.CameraUniformBuffer = UniformBuffer::Create(sizeof(Particle2DData::CameraData), 0);
	}

	void ParticleSystem::Shutdown()
	{
		//NZ_PROFILE_FUNCTION();

		delete[] s_ParticleData.QuadVertexBufferBase;
	}

	void ParticleSystem::BeginScene(const EditorCamera& camera)
	{
		s_ParticleData.m_CameraView = camera.GetViewMatrix();
		s_ParticleData.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_ParticleData.CameraUniformBuffer->SetData(&s_ParticleData.CameraBuffer, sizeof(Particle2DData::CameraData));

		StartBatch();
	}


	void ParticleSystem::BeginScene(const Camera& camera, const rtmcpp::Mat4& transform)
	{
		//NZ_PROFILE_FUNCTION();

		s_ParticleData.m_CameraView = rtmcpp::Inverse(transform);
		s_ParticleData.CameraBuffer.ViewProjection = rtmcpp::Inverse(transform) * camera.GetProjection();
		s_ParticleData.CameraUniformBuffer->SetData(&s_ParticleData.CameraBuffer, sizeof(Particle2DData::CameraData));

		StartBatch();
	}

	void ParticleSystem::OnUpdate(const Timestep& ts)
	{
		for (auto& particle : ParticlePool)
		{
			if (!particle.Active)
				continue;

			if (particle.LifeRemaining <= 0.0f)
			{
				particle.Active = false;
				continue;
			}

			particle.LifeRemaining -= ts;
			particle.Position += rtmcpp::Vec4(particle.Velocity.X * (float)ts, particle.Velocity.Y * (float)ts, particle.Velocity.Z * (float)ts, 1.0f);
			particle.Rotation += rtmcpp::Vec3(0.01f * ts, 0.01f * ts, 0.01f * ts);
		}
	}

	void ParticleSystem::DrawParticle(TransformComponent& transform, bool useBillboard, int entityID)
	{
		if (s_ParticleData.UseBillboard != useBillboard)
			s_ParticleData.UseBillboard = useBillboard;

		for (auto& particle : ParticlePool)
		{
			if (!particle.Active)
				continue;

			// Fade away particles
			float life = particle.LifeRemaining / particle.LifeTime;
			rtmcpp::Vec4 color = rtm::vector_lerp(
				rtmcpp::Vec4{ particle.ColorBegin.X, particle.ColorBegin.Y, particle.ColorBegin.Z, 1.0f }.Value,
				rtmcpp::Vec4{ particle.ColorEnd.X, particle.ColorEnd.Y, particle.ColorEnd.Z, 1.0f }.Value, life
			); // rtmcpp::Lerp(particle.ColorBegin, particle.ColorEnd, life);

			float size = rtm::scalar_lerp(particle.SizeEnd, particle.SizeBegin, life); // float size = rtmcpp::Lerp(particle.SizeEnd, particle.SizeBegin, life);

			// Render
			rtmcpp::Mat4 posRotScl = rtmcpp::Mat4Cast(rtmcpp::Scale(rtmcpp::Vec3{ size, size, 1.0f }))
				* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(particle.Rotation.Z, { 0.0f, 0.0f, 1.0f }))
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ particle.Position.X, particle.Position.Y, particle.Position.Z }));

			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::AngleAxis(particle.Rotation.X, { 1.0f, 1.0f, 1.0f }))
				* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(particle.Rotation.Y, { 0.0f, 1.0f, 1.0f }))
				* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(particle.Rotation.Z, { 0.0f, 0.0f, 1.0f }));

			// Render
			//glm::mat4 posRotScl = glm::translate(glm::mat4(1.0f), particle.Position)
			//	* glm::rotate(glm::mat4(1.0f), particle.Rotation.z, { 0.0f, 0.0f, 1.0f })
			//	* glm::scale(glm::mat4(1.0f), { size, size, 1.0f });

			//glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), particle.Rotation.x, { 1.0f, 1.0f, 1.0f })
			//	* glm::rotate(glm::mat4(1.0f), particle.Rotation.y, { 0.0f, 1.0f, 1.0f })
			//	* glm::rotate(glm::mat4(1.0f), particle.Rotation.z, { 0.0f, 0.0f, 1.0f });

			DrawParticle(transform, posRotScl, rotation, size, color, useBillboard, entityID);
		}
	}

	void ParticleSystem::DrawParticle(TransformComponent& comp, const rtmcpp::Mat4& transform, const rtmcpp::Mat4& rotation, float size, const rtmcpp::Vec4& color, bool useBillboard, int entityID)
	{
		rtmcpp::Vec2 textureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		if (s_ParticleData.QuadIndexCount >= Particle2DData::MaxIndices)
			NextBatch();

		int textureIndex = 0;
		for (uint32_t i = 1; i < s_ParticleData.TextureSlotIndex; i++)
		{
			if (*s_ParticleData.TextureSlots[i] == *s_ParticleData.WhiteTexture)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_ParticleData.TextureSlotIndex >= Particle2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_ParticleData.TextureSlotIndex;
			s_ParticleData.TextureSlots[s_ParticleData.TextureSlotIndex] = s_ParticleData.WhiteTexture;
			s_ParticleData.TextureSlotIndex++;
		}

		if (useBillboard)
		{
			rtmcpp::Vec4 camRightWS = { s_ParticleData.m_CameraView.Value.x_axis.m128_f32[0], s_ParticleData.m_CameraView.Value.y_axis.m128_f32[0], s_ParticleData.m_CameraView.Value.z_axis.m128_f32[0], s_ParticleData.m_CameraView.Value.w_axis.m128_f32[0] };
			rtmcpp::Vec4 camUpWS = { s_ParticleData.m_CameraView.Value.x_axis.m128_f32[1], s_ParticleData.m_CameraView.Value.y_axis.m128_f32[1], s_ParticleData.m_CameraView.Value.z_axis.m128_f32[1], s_ParticleData.m_CameraView.Value.w_axis.m128_f32[1] };
			rtmcpp::Vec4 position = { transform.Value.w_axis.m128_f32[0] + comp.Translation.X, transform.Value.w_axis.m128_f32[1] + comp.Translation.Y, transform.Value.w_axis.m128_f32[2] + comp.Translation.Z, 1.0f };

			for (size_t i = 0; i < 4; i++)
			{
				//rtmcpp::Vec4 pos = rtmcpp::Vec4{ position + (rtmcpp::Vec4{ (camRightWS.X * (s_ParticleData.QuadVertexPositions[i].X)) * size, (camRightWS.Y * (s_ParticleData.QuadVertexPositions[i].X)) * size, (camRightWS.Z * (s_ParticleData.QuadVertexPositions[i].X)) * size, (camRightWS.W * (s_ParticleData.QuadVertexPositions[i].X)) * size } +
				//	(rtmcpp::Vec4{ (camUpWS.X * s_ParticleData.QuadVertexPositions[i].Y) * size, (camUpWS.Y * s_ParticleData.QuadVertexPositions[i].Y) * size, (camUpWS.Z * s_ParticleData.QuadVertexPositions[i].Y) * size, (camUpWS.W * s_ParticleData.QuadVertexPositions[i].Y) * size }
				//	*rotation)) };
				
				//rtmcpp::Vec4 something1 = rtmcpp::Vec4{ s_ParticleData.QuadVertexPositions[i].X * camRightWS.X, s_ParticleData.QuadVertexPositions[i].X * camRightWS.Y, s_ParticleData.QuadVertexPositions[i].X * camRightWS.Z, s_ParticleData.QuadVertexPositions[i].X * camRightWS.W };
				//rtmcpp::Vec4 something2 = rtmcpp::Vec4{ s_ParticleData.QuadVertexPositions[i].Y * camUpWS.X, s_ParticleData.QuadVertexPositions[i].Y * camUpWS.Y, s_ParticleData.QuadVertexPositions[i].Y * camUpWS.Z, s_ParticleData.QuadVertexPositions[i].Y * camUpWS.W };

				//glm::vec4 something1 = camRightWS * (s_ParticleData.QuadVertexPositions[i].x);
				//glm::vec4 something2 = camUpWS * s_ParticleData.QuadVertexPositions[i].y;

				//s_ParticleData.QuadVertexBufferPtr->Position = position + (something1 * size) * rotation + (something2 * size) * rotation;

				rtmcpp::Vec4 something1 = camUpWS * rtmcpp::Vec4{
					s_ParticleData.QuadVertexPositions[i].Y * size,
					s_ParticleData.QuadVertexPositions[i].Y * size,
					s_ParticleData.QuadVertexPositions[i].Y * size,
					1.0f
				};
				rtmcpp::Vec4 something2 = camRightWS * rtmcpp::Vec4{
					s_ParticleData.QuadVertexPositions[i].X * size,
					s_ParticleData.QuadVertexPositions[i].X * size,
					s_ParticleData.QuadVertexPositions[i].X * size,
					1.0f
				};

				/*s_ParticleData.QuadVertexBufferPtr->Position = camUpWS * rtmcpp::Vec4{
					s_ParticleData.QuadVertexPositions[i].Y * size,
					s_ParticleData.QuadVertexPositions[i].Y * size,
					s_ParticleData.QuadVertexPositions[i].Y * size,
					1.0f
				} + camRightWS * rtmcpp::Vec4{
					s_ParticleData.QuadVertexPositions[i].X * size,
					s_ParticleData.QuadVertexPositions[i].X * size,
					s_ParticleData.QuadVertexPositions[i].X * size,
					1.0f
				} + position;*/

				s_ParticleData.QuadVertexBufferPtr->Position = something1 //rtmcpp::Vec4{ something1.X * size, something1.Y * size, something1.Z * size, 1.0f }
					* rotation + /*rtmcpp::Vec4{something2.X * size, something2.Y * size, something2.Z * size, 1.0f}*/ something2 * rotation + position;
				s_ParticleData.QuadVertexBufferPtr->Color = color;
				s_ParticleData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
				s_ParticleData.QuadVertexBufferPtr->TexIndex = textureIndex;
				s_ParticleData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
				s_ParticleData.QuadVertexBufferPtr->EntityID = entityID;
				s_ParticleData.QuadVertexBufferPtr++;
			}
		}
		else
		{
			rtmcpp::Mat4 finalTransform = transform * comp.GetTransform();
			//glm::mat4 finalTransform = comp.GetTransform() * transform;

			for (size_t i = 0; i < 4; i++)
			{
				s_ParticleData.QuadVertexBufferPtr->Position = s_ParticleData.QuadVertexPositions[i] * finalTransform;
				s_ParticleData.QuadVertexBufferPtr->Color = color;
				s_ParticleData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
				s_ParticleData.QuadVertexBufferPtr->TexIndex = textureIndex;
				s_ParticleData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
				s_ParticleData.QuadVertexBufferPtr->EntityID = entityID;
				s_ParticleData.QuadVertexBufferPtr++;
			}
		}

		s_ParticleData.QuadIndexCount += 6;

		s_ParticleData.Stats.QuadCount++;
	}

	void ParticleSystem::DrawParticle(TransformComponent& transform, ParticleSystemComponent& src, bool useBillboard, int entityID)
	{
		/*if (src.Texture != AssetManager::GetAsset<Texture2D>(src.TextureHandle))
			src.Texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);

		NZ_CORE_VERIFY(src.Texture);*/

		if (s_ParticleData.UseBillboard != useBillboard)
			s_ParticleData.UseBillboard = useBillboard;

		for (auto& particle : ParticlePool)
		{
			if (!particle.Active)
				continue;

			// Fade away particles
			float life = particle.LifeRemaining / particle.LifeTime;
			//glm::vec4 color = glm::lerp(particle.ColorBegin, particle.ColorEnd, life);
			rtmcpp::Vec4 color = rtm::vector_lerp(
				rtmcpp::Vec4{ particle.ColorBegin.X, particle.ColorBegin.Y, particle.ColorBegin.Z, 1.0f }.Value,
				rtmcpp::Vec4{ particle.ColorEnd.X, particle.ColorEnd.Y, particle.ColorEnd.Z, 1.0f }.Value, life
			); // rtmcpp::Lerp(particle.ColorBegin, particle.ColorEnd, life);

			float size = rtm::scalar_lerp(particle.SizeEnd, particle.SizeBegin, life); // float size = rtmcpp::Lerp(particle.SizeEnd, particle.SizeBegin, life);

			// Render
			rtmcpp::Mat4 posRotScl = rtmcpp::Mat4Cast(rtmcpp::Scale(rtmcpp::Vec3{ size, size, 1.0f }))
				* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(particle.Rotation.Z, { 0.0f, 0.0f, 1.0f }))
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ particle.Position.X, particle.Position.Y, particle.Position.Z }));

			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::AngleAxis(particle.Rotation.X, { 1.0f, 1.0f, 1.0f }))
				* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(particle.Rotation.Y, { 0.0f, 1.0f, 1.0f }))
				* rtmcpp::Mat4Cast(rtmcpp::AngleAxis(particle.Rotation.Z, { 0.0f, 0.0f, 1.0f }));

			// Render
			//glm::mat4 posRotScl = glm::translate(glm::mat4(1.0f), particle.Position)
			//	* glm::rotate(glm::mat4(1.0f), particle.Rotation.z, { 0.0f, 0.0f, 1.0f })
			//	* glm::scale(glm::mat4(1.0f), { size, size, 1.0f });

			//glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), particle.Rotation.x, { 1.0f, 1.0f, 1.0f })
			//	* glm::rotate(glm::mat4(1.0f), particle.Rotation.y, { 0.0f, 1.0f, 1.0f })
			//	* glm::rotate(glm::mat4(1.0f), particle.Rotation.z, { 0.0f, 0.0f, 1.0f });

			if (src.TextureHandle != 0)
			{
				DrawParticle(transform, posRotScl, rotation, size, src, color, useBillboard, entityID);
			}
			else
			{
				DrawParticle(transform, posRotScl, rotation, size, color, useBillboard, entityID);
			}
		}
	}

	void ParticleSystem::DrawParticle(TransformComponent& comp, const rtmcpp::Mat4& transform, const rtmcpp::Mat4& rotation, float size, ParticleSystemComponent& src, const rtmcpp::Vec4& color, bool useBillboard, int entityID)
	{
		if (src.Texture != AssetManager::GetAsset<Texture2D>(src.TextureHandle))
			src.Texture = AssetManager::GetAsset<Texture2D>(src.TextureHandle);

		NZ_CORE_VERIFY(src.Texture);

		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		if (s_ParticleData.QuadIndexCount >= Particle2DData::MaxIndices)
			NextBatch();

		int textureIndex = 0;
		for (uint32_t i = 1; i < s_ParticleData.TextureSlotIndex; i++)
		{
			if (*s_ParticleData.TextureSlots[i] == *src.Texture)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_ParticleData.TextureSlotIndex >= Particle2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = s_ParticleData.TextureSlotIndex;
			s_ParticleData.TextureSlots[s_ParticleData.TextureSlotIndex] = src.Texture;
			s_ParticleData.TextureSlotIndex++;
		}

		if (useBillboard)
		{
			/*glm::vec4 camRightWS = {s_ParticleData.m_CameraView[0][0], s_ParticleData.m_CameraView[1][0], s_ParticleData.m_CameraView[2][0], s_ParticleData.m_CameraView[3][0]};
			glm::vec4 camUpWS = { s_ParticleData.m_CameraView[0][1], s_ParticleData.m_CameraView[1][1], s_ParticleData.m_CameraView[2][1], s_ParticleData.m_CameraView[3][1] };
			glm::vec4 position = { transform[3][0] + comp.Translation.x, transform[3][1] + comp.Translation.y, transform[3][2] + comp.Translation.z, 1.0f };

			for (size_t i = 0; i < 4; i++)
			{
				glm::vec4 pos = glm::vec4(position + ((camRightWS * (s_ParticleData.QuadVertexPositions[i].x)) * size) + ((camUpWS * s_ParticleData.QuadVertexPositions[i].y) * size) * rotation);
				
				glm::vec4 something1 = camRightWS * (s_ParticleData.QuadVertexPositions[i].x);
				glm::vec4 something2 = camUpWS * s_ParticleData.QuadVertexPositions[i].y;

				s_ParticleData.QuadVertexBufferPtr->Position = position + (something1 * size) * rotation + (something2 * size) * rotation;
				s_ParticleData.QuadVertexBufferPtr->Color = color;
				s_ParticleData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
				s_ParticleData.QuadVertexBufferPtr->TexIndex = textureIndex;
				s_ParticleData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
				s_ParticleData.QuadVertexBufferPtr->EntityID = entityID;
				s_ParticleData.QuadVertexBufferPtr++;
			}*/

			rtmcpp::Vec4 camRightWS = { s_ParticleData.m_CameraView.Value.x_axis.m128_f32[0], s_ParticleData.m_CameraView.Value.y_axis.m128_f32[0], s_ParticleData.m_CameraView.Value.z_axis.m128_f32[0], s_ParticleData.m_CameraView.Value.w_axis.m128_f32[0] };
			rtmcpp::Vec4 camUpWS = { s_ParticleData.m_CameraView.Value.x_axis.m128_f32[1], s_ParticleData.m_CameraView.Value.y_axis.m128_f32[1], s_ParticleData.m_CameraView.Value.z_axis.m128_f32[1], s_ParticleData.m_CameraView.Value.w_axis.m128_f32[1] };
			rtmcpp::Vec4 position = { transform.Value.w_axis.m128_f32[0] + comp.Translation.X, transform.Value.w_axis.m128_f32[1] + comp.Translation.Y, transform.Value.w_axis.m128_f32[2] + comp.Translation.Z, 1.0f };

			for (size_t i = 0; i < 4; i++)
			{
				/*rtmcpp::Vec4 pos = rtmcpp::Vec4{position + (rtmcpp::Vec4{(camRightWS.X * (s_ParticleData.QuadVertexPositions[i].X)) * size, (camRightWS.Y * (s_ParticleData.QuadVertexPositions[i].X)) * size, (camRightWS.Z * (s_ParticleData.QuadVertexPositions[i].X)) * size, (camRightWS.W * (s_ParticleData.QuadVertexPositions[i].X)) * size} +
					(rtmcpp::Vec4{ (camUpWS.X * s_ParticleData.QuadVertexPositions[i].Y) * size, (camUpWS.Y * s_ParticleData.QuadVertexPositions[i].Y) * size, (camUpWS.Z * s_ParticleData.QuadVertexPositions[i].Y) * size, (camUpWS.W * s_ParticleData.QuadVertexPositions[i].Y) * size }
					*rotation)) };

				rtmcpp::Vec4 something1 = rtmcpp::Vec4{ s_ParticleData.QuadVertexPositions[i].X * camRightWS.X, s_ParticleData.QuadVertexPositions[i].X * camRightWS.Y, s_ParticleData.QuadVertexPositions[i].X * camRightWS.Z, s_ParticleData.QuadVertexPositions[i].X * camRightWS.W };
				rtmcpp::Vec4 something2 = rtmcpp::Vec4{ s_ParticleData.QuadVertexPositions[i].Y * camUpWS.X, s_ParticleData.QuadVertexPositions[i].Y * camUpWS.Y, s_ParticleData.QuadVertexPositions[i].Y * camUpWS.Z, s_ParticleData.QuadVertexPositions[i].Y * camUpWS.W };

				s_ParticleData.QuadVertexBufferPtr->Position = rtmcpp::Vec4{ something1.X * size, something1.Y * size, something1.Z * size, something1.W * size }
				*rotation + rtmcpp::Vec4{ something2.X * size, something2.Y * size, something2.Z * size, something2.W * size } *rotation + position;*/
				rtmcpp::Vec4 something1 = camUpWS * rtmcpp::Vec4{
					s_ParticleData.QuadVertexPositions[i].Y * size,
					s_ParticleData.QuadVertexPositions[i].Y * size,
					s_ParticleData.QuadVertexPositions[i].Y * size,
					1.0f
				};
				rtmcpp::Vec4 something2 = camRightWS * rtmcpp::Vec4{
					s_ParticleData.QuadVertexPositions[i].X * size,
					s_ParticleData.QuadVertexPositions[i].X * size,
					s_ParticleData.QuadVertexPositions[i].X * size,
					1.0f
				};

				/*s_ParticleData.QuadVertexBufferPtr->Position = camUpWS * rtmcpp::Vec4{
					s_ParticleData.QuadVertexPositions[i].Y * size,
					s_ParticleData.QuadVertexPositions[i].Y * size,
					s_ParticleData.QuadVertexPositions[i].Y * size,
					1.0f
				} + camRightWS * rtmcpp::Vec4{
					s_ParticleData.QuadVertexPositions[i].X * size,
					s_ParticleData.QuadVertexPositions[i].X * size,
					s_ParticleData.QuadVertexPositions[i].X * size,
					1.0f
				} + position;*/

				s_ParticleData.QuadVertexBufferPtr->Position = something1 //rtmcpp::Vec4{ something1.X * size, something1.Y * size, something1.Z * size, 1.0f }
					* rotation + /*rtmcpp::Vec4{something2.X * size, something2.Y * size, something2.Z * size, 1.0f}*/ something2 * rotation + position;
				s_ParticleData.QuadVertexBufferPtr->Color = color;
				s_ParticleData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
				s_ParticleData.QuadVertexBufferPtr->TexIndex = textureIndex;
				s_ParticleData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
				s_ParticleData.QuadVertexBufferPtr->EntityID = entityID;
				s_ParticleData.QuadVertexBufferPtr++;
			}
		}
		else
		{
			rtmcpp::Mat4 finalTransform = transform * comp.GetTransform();
			//glm::mat4 finalTransform = comp.GetTransform() * transform;

			for (size_t i = 0; i < 4; i++)
			{
				s_ParticleData.QuadVertexBufferPtr->Position = s_ParticleData.QuadVertexPositions[i] * finalTransform;
				s_ParticleData.QuadVertexBufferPtr->Color = color;
				s_ParticleData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
				s_ParticleData.QuadVertexBufferPtr->TexIndex = textureIndex;
				s_ParticleData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
				s_ParticleData.QuadVertexBufferPtr->EntityID = entityID;
				s_ParticleData.QuadVertexBufferPtr++;
			}
		}

		s_ParticleData.QuadIndexCount += 6;

		s_ParticleData.Stats.QuadCount++;
	}

	void ParticleSystem::StartBatch()
	{
		s_ParticleData.QuadIndexCount = 0;
		s_ParticleData.QuadVertexBufferPtr = s_ParticleData.QuadVertexBufferBase;

		s_ParticleData.TextureSlotIndex = 1;
	}


	void ParticleSystem::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void ParticleSystem::Flush()
	{
		if (s_ParticleData.QuadIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_ParticleData.QuadVertexBufferPtr - (uint8_t*)s_ParticleData.QuadVertexBufferBase);
			s_ParticleData.QuadVertexBuffer->SetData(s_ParticleData.QuadVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_ParticleData.TextureSlotIndex; i++)
				s_ParticleData.TextureSlots[i]->Bind(i);

			s_ParticleData.ParticleShader->Bind();
			RenderCommand::DrawParticles(s_ParticleData.QuadVertexArray, s_ParticleData.QuadIndexCount, s_ParticleData.UseBillboard);
			s_ParticleData.Stats.DrawCalls++;
		}
	}


	void ParticleSystem::EndScene()
	{
		Flush();
	}

	void ParticleSystem::Emit(const ParticleProps& particleProps)
	{
		Particle& particle = ParticlePool[s_PoolIndex];
		particle.Active = true;
		particle.Position = rtmcpp::Vec4{ particleProps.Position, 1.0f };
		particle.Rotation = rtmcpp::Vec3{ Random::Float() * 2.0f * static_cast<float>(M_PI), Random::Float() * 2.0f * static_cast<float>(M_PI), Random::Float() * 2.0f * static_cast<float>(M_PI) };

		// Velocity
		particle.Velocity = particleProps.Velocity;
		particle.Velocity.X += particleProps.VelocityVariation.X * (Random::Float() - 0.5f);
		particle.Velocity.Y += particleProps.VelocityVariation.Y * (Random::Float() - 0.5f);

		// Color
		particle.ColorBegin = particleProps.ColorBegin;
		particle.ColorEnd = particleProps.ColorEnd;

		particle.LifeTime = particleProps.LifeTime;
		particle.LifeRemaining = particleProps.LifeTime;
		particle.SizeBegin = particleProps.SizeBegin + particleProps.SizeVariation * (Random::Float() - 0.5f);
		particle.SizeEnd = particleProps.SizeEnd;

		s_PoolIndex = --s_PoolIndex % ParticlePool.size();
	}

	void ParticleSystem::DrawParticles(TransformComponent& transform, ParticleSystemComponent& src, bool useBillboard, int entityID)
	{
		if (src.TextureHandle != 0)
		{
			DrawParticle(transform, src, useBillboard, entityID);
		}
		else
		{
			DrawParticle(transform, useBillboard, entityID);
		}
	}

	uint32_t ParticleSystem::GetMaxParticles()
	{
		return s_MaxParticles;
	}

	void ParticleSystem::SetMaxParticles(uint32_t maxParticles)
	{
		s_MaxParticles = maxParticles;
		s_PoolIndex = maxParticles - 1;
		ParticlePool.resize(maxParticles);
	}

	void ParticleSystem::ResetStats()
	{
		memset(&s_ParticleData.Stats, 0, sizeof(ParticleStats));
	}

	ParticleSystem::ParticleStats ParticleSystem::GetStats()
	{
		return s_ParticleData.Stats;
	}

}