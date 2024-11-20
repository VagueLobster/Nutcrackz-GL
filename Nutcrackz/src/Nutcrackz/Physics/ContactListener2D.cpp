#include "nzpch.hpp"
#include "ContactListener2D.hpp"

#include "Nutcrackz/Scene/Entity.hpp"
#include "Nutcrackz/Scripting/ScriptEngine.hpp"

namespace Nutcrackz {

	void ContactListener2D::BeginContact(b2Contact* contact)
	{
		if (m_IsPlaying)
		{
			Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
			Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

			if (a.HasComponent<ScriptComponent>() && a.HasComponent<Rigidbody2DComponent>())
			{
				auto& scriptComponent = a.GetComponent<ScriptComponent>();
				scriptComponent.Instance.Invoke("OnCollisionBegin");
			}

			if (b.HasComponent<ScriptComponent>() && b.HasComponent<Rigidbody2DComponent>())
			{
				auto& scriptComponent = b.GetComponent<ScriptComponent>();
				scriptComponent.Instance.Invoke("OnCollisionBegin");
			}
		}
	}

	/// Called when two fixtures cease to touch.
	void ContactListener2D::EndContact(b2Contact* contact)
	{
		if (m_IsPlaying)
		{
			Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
			Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;
		
			if (a.HasComponent<ScriptComponent>() && a.HasComponent<Rigidbody2DComponent>())
			{
				auto& scriptComponent = a.GetComponent<ScriptComponent>();
				scriptComponent.Instance.Invoke("OnCollisionEnd");
			}
		
			if (b.HasComponent<ScriptComponent>() && b.HasComponent<Rigidbody2DComponent>())
			{
				auto& scriptComponent = b.GetComponent<ScriptComponent>();
				scriptComponent.Instance.Invoke("OnCollisionEnd");
			}
		}
	}

}