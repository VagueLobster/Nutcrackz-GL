#pragma once

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Core/Timestep.hpp"
#include "Nutcrackz/Renderer/EditorCamera.hpp"
#include "Nutcrackz/Renderer/Shader.hpp"
#include "Nutcrackz/Renderer/VertexArray.hpp"
#include "Nutcrackz/Renderer/Texture.hpp"

#include <vector>

//#include <glm/glm.hpp>
#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	struct ParticleProps
	{
		rtmcpp::Vec3 Position;
		rtmcpp::Vec3 Velocity, VelocityVariation;
		rtmcpp::Vec4 ColorBegin, ColorEnd;
		float SizeBegin = 0.0f, SizeEnd = 0.0f, SizeVariation = 0.0f;
		float LifeTime = 1.0f;
	};

	struct TransformComponent;
	struct ParticleSystemComponent;

	class ParticleSystem
	{
	public:
		ParticleSystem(uint32_t maxParticles);

		static void Init();
		static void Shutdown();
		static void BeginScene(const Camera& camera, const rtmcpp::Mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void OnUpdate(const Timestep& ts);
		static void DrawParticle(TransformComponent& transform, bool useBillboard = false, int entityID = -1);
		static void DrawParticle(TransformComponent& comp, const rtmcpp::Mat4& transform, const rtmcpp::Mat4& rotation, float size, const rtmcpp::Vec4& color, bool useBillboard = false, int entityID = -1);
		static void DrawParticle(TransformComponent& transform, ParticleSystemComponent& src, bool useBillboard = false, int entityID = -1);
		static void DrawParticle(TransformComponent& comp, const rtmcpp::Mat4& transform, const rtmcpp::Mat4& rotation, float size, ParticleSystemComponent& src, const rtmcpp::Vec4& color, bool useBillboard = false, int entityID = -1);
		static void StartBatch();
		static void NextBatch();
		static void Flush();
		static void EndScene();

		static void Emit(const ParticleProps& particleProps);
		static void DrawParticles(TransformComponent& transform, ParticleSystemComponent& src, bool useBillboard = false, int entityID = -1);

		static uint32_t GetMaxParticles();
		static void SetMaxParticles(uint32_t maxParticles);

		// Stats
		struct ParticleStats
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;

			uint32_t GetTotalVertexCount() { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() { return QuadCount * 6; }
		};

		static void ResetStats();
		static ParticleStats GetStats();
		
	public:
		static uint32_t s_MaxParticles;

	private:
		static uint32_t s_PoolIndex;
	};

}
