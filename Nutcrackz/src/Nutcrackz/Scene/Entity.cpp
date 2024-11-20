#include "nzpch.hpp"
#include "Entity.hpp"
#include "Components.hpp"

namespace Nutcrackz {

	Entity::Entity(flecs::entity handle)
		: m_EntityHandle(handle)
	{
	}

}
