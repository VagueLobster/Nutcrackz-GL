#pragma once

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Core/Timestep.hpp"
#include "Nutcrackz/Events/Event.hpp"

namespace Nutcrackz {

	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}