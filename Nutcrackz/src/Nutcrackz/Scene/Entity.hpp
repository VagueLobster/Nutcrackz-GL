#pragma once

#include "Scene.hpp"
#include "Components.hpp"

#include "flecs/flecs.h"

namespace Nutcrackz {

	class Entity
	{
	public:
		Entity() = default;
		Entity(flecs::entity handle);
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			if (!HasComponent<T>())
				m_EntityHandle.add<T>();

			m_EntityHandle.set<T>({ std::forward<Args>(args)... });
			return m_EntityHandle.ensure<T>();
		}

		template<typename T>
		T& EraseEntity()
		{
			return m_EntityHandle.destruct();
		}

		void DestroyEntity(Entity& handle)
		{
			handle.DestroyEntity();
		}

		template<typename T>
		T& GetComponent()
		{
			if (!HasComponent<T>())
			{
				return AddComponent<T>();
			}

			T& component = (T&)(*m_EntityHandle.get<T>());
			return component;
		}

		template<typename T>
		bool HasComponent()
		{
			return m_EntityHandle.has<T>();
		}

		template<typename T>
		void RemoveComponent()
		{
			m_EntityHandle.remove<T>();
		}

		void DestroyEntity() { m_EntityHandle.destruct(); }

		operator bool() const { return m_EntityHandle != flecs::Empty; }
		operator flecs::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		const std::string& GetName() { return GetComponent<TagComponent>().Tag; }

		uint64_t GetEntityHandle() { return (uint64_t)m_EntityHandle; }

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		flecs::entity m_EntityHandle{ flecs::Empty };

		friend class Scene;
		friend class SceneHierarchyPanel;
		friend class ScriptEngine;
	};

}
