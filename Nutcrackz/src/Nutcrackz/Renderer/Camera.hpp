#pragma once

#include "Nutcrackz/Core/Ref.hpp"

//#include <glm/glm.hpp>
#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	class Camera : public RefCounted
	{
	public:
		Camera() = default;
		Camera(const rtmcpp::Mat4& projection)
			: m_Projection(projection) {}

		virtual ~Camera() = default;

		virtual rtmcpp::Vec3 GetForwardDirection() const;

		const rtmcpp::Mat4& GetProjection() const { return m_Projection; }

	protected:
		rtmcpp::Mat4 m_Projection = rtmcpp::Mat4();
		float m_Pitch = 0.0f, m_Yaw = 0.0f;
	};

}
