#pragma once

#include <stdint.h>

#include "rtmcpp/Common.hpp"
#include "rtmcpp/Scalar.hpp"

namespace Nutcrackz {

	struct AudioListenerConfig
	{
		float ConeInnerAngle = rtmcpp::Radians(360.0f);
		float ConeOuterAngle = rtmcpp::Radians(360.0f);
		float ConeOuterGain = 0.0f;
	};

	class AudioListener : public RefCounted
	{
	public:
		AudioListener() = default;

		void SetConfig(const AudioListenerConfig& config) const;
		void SetPosition(const rtmcpp::Vec4& position) const;
		void SetDirection(const rtmcpp::Vec3& forward) const;
		void SetVelocity(const rtmcpp::Vec3& velocity) const;
		//void SetPosition(const glm::vec3& position) const;
		//void SetDirection(const glm::vec3& forward) const;
		//void SetVelocity(const glm::vec3& velocity) const;

	private:
		uint32_t m_ListenerIndex = 0;
	};
}
