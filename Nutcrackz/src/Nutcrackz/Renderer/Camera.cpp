#include "nzpch.hpp"
#include "Camera.hpp"

namespace Nutcrackz {

	rtmcpp::Vec3 RotateDirection(const rtmcpp::Quat4& q, const rtmcpp::Vec3& v)
	{
		//return q * v; // From glm
		return v * q;
	}

	rtmcpp::Vec3 Camera::GetForwardDirection() const
	{
		return RotateDirection(rtmcpp::FromEuler(rtmcpp::Vec3(m_Yaw, 0.0f, m_Pitch)), rtmcpp::Vec3(0.0f, 0.0f, -1.0f));
	}

}
