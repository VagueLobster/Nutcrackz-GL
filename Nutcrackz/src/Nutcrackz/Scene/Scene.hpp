#pragma once

#include "Nutcrackz/Core/UUID.hpp"
#include "Nutcrackz/Asset/Asset.hpp"
#include "Nutcrackz/Core/Timestep.hpp"
#include "Nutcrackz/Renderer/EditorCamera.hpp"
#include "Nutcrackz/Renderer/Framebuffer.hpp"
#include "Nutcrackz/Scene/Components.hpp"

#include "Nutcrackz/Scripting/ScriptEntityStorage.hpp"

#include "flecs/flecs.h"

#include "rtmcpp/Common.hpp"

class b2World;

namespace Nutcrackz {

	class Entity;

	using EntityMap = std::unordered_map<uint64_t, flecs::entity>;

	class Scene : public Asset
	{
	public:
		Scene();
		~Scene();

		static RefPtr<Scene> Copy(RefPtr<Scene> other);

		virtual AssetType GetType() const { return AssetType::Scene; }

		AssetHandle GetAssetHandle() { return m_SceneHandle; }
		void SetAssetHandle(AssetHandle handle) { m_SceneHandle = handle; }

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithID(uint64_t id, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnSimulationStart();
		void OnSimulationStop();
				
		void OnUpdateRuntime(Timestep ts);
		void OnUpdateSimulation(Timestep ts, const EditorCamera& camera);
		void OnUpdateEditor(Timestep ts, const EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);

		float GetFPS() { return m_FPS; }
		float GetFrameTime() { return m_FrameTime; }
		float GetMinFrameTime() { return m_MinFrameTime; }
		void SetMinFrameTime(float value) { m_MinFrameTime = value; }
		float GetMaxFrameTime() { return m_MaxFrameTime; }
		void SetMaxFrameTime(float value) { m_MaxFrameTime = value; }

		Entity GetPrimaryCameraEntity();

		bool IsPaused() const { return m_IsPaused; }

		void SetPaused(bool paused) { m_IsPaused = paused; }

		void Step(int frames = 1);

		Entity DuplicateEntity(Entity entity);
		
		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByID(uint64_t id);
		Entity TryGetEntityWithID(uint64_t id) const;

		Entity FindEntityByTag(const std::string& tag);

		void OnSceneTransition(AssetHandle handle);

		rtmcpp::Vec2 GetPhysics2DGravity();
		void SetPhysics2DGravity(const rtmcpp::Vec2& gravity);

		void RenderHoveredEntityOutline(Entity entity, rtmcpp::Vec4 color);
		void RenderSelectedEntityOutline(Entity entity, rtmcpp::Vec4 color);

		flecs::world& GetECS() { return m_ECS; }

		void SetName(const std::string& name) { m_Name = name; }
		const std::string& GetName() const { return m_Name; }

		void SetSceneTransitionCallback(const std::function<void(AssetHandle)>& callback) { m_OnSceneTransitionCallback = callback; }

		void ShouldGameBePaused(bool shouldPause) { s_SetPaused = shouldPause; }
		bool IsGamePaused() { return s_SetPaused; }
						
		// Frustum culling (maybe?)
		bool IsSpriteVisibleToCamera(bool isEditorCamera, const rtmcpp::Vec4& spritePosition, const rtmcpp::Vec3& spriteSize, const rtmcpp::Vec4& cameraPosition, const rtmcpp::Vec3& cameraSize = rtmcpp::Vec3(1.0f, 1.0f, 1.0f));

		ScriptStorage& GetScriptStorage() { return m_ScriptStorage; }
		const ScriptStorage& GetScriptStorage() const { return m_ScriptStorage; }

	private:
		void OnPhysics2DStart();
		void OnPhysics2DStop();

		void RenderScene(Timestep ts, const EditorCamera& camera);

		void SortQuadEntitiesDepth(float cameraDepth);

		void OnScriptComponentDestroy(flecs::world& registry, flecs::entity entity);

	private:
		flecs::world m_ECS;
		AssetHandle m_SceneHandle;

		std::function<void(AssetHandle)> m_OnSceneTransitionCallback;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		std::string m_Name = "Untitled";

		bool m_IsPaused = false;
		int m_StepFrames = 0;

		Entity* m_Physics2DBodyEntityBuffer = nullptr;

		static bool s_SetPaused;
		static rtmcpp::Vec2 s_Gravity;

		b2World* m_PhysicsWorld = nullptr;

		EntityMap m_EntityMap;
		
		// Frustum culling (maybe?)
		rtmcpp::Vec4 m_EditorCameraPosition;

		rtmcpp::Vec4 m_SceneCameraPosition;
		rtmcpp::Vec3 m_SceneCameraSize;

		ScriptStorage m_ScriptStorage;

		// Frame rate counter
		float m_Deltatime = 0.0f;
		float m_FPS = 0.0f;
		int m_MinFrameTimeCounter = 0;
		float m_FrameTime = 0.0f;
		float m_MinFrameTime = 1000000.0f;
		float m_MaxFrameTime = 0.0f;
		float m_SecondsElapsed = 0.0f;
		float m_LastFrameTime = 0.0f;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}
