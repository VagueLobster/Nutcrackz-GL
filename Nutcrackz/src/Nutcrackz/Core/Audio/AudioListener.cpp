#include "nzpch.hpp"
#include "AudioListener.hpp"

#include <miniaudio.h>

#include "AudioEngine.hpp"

namespace Nutcrackz {

	void AudioListener::SetConfig(const AudioListenerConfig& config) const
	{
		//NZ_PROFILE_FUNCTION();

		auto* engine = static_cast<ma_engine*>(AudioEngine::GetEngine());
		ma_engine_listener_set_cone(engine, m_ListenerIndex, config.ConeInnerAngle, config.ConeOuterAngle, config.ConeOuterGain);
	}
	
	void AudioListener::SetPosition(const rtmcpp::Vec4& position) const
	{
		//NZ_PROFILE_FUNCTION();

		auto* engine = static_cast<ma_engine*>(AudioEngine::GetEngine());
		ma_engine_listener_set_position(engine, m_ListenerIndex, position.X, position.Y, position.Z);

		static bool setupWorldUp = false;
		if (!setupWorldUp)
		{
			ma_engine_listener_set_world_up(engine, m_ListenerIndex, 0, 1, 0);
			setupWorldUp = true;
		}
	}

	void AudioListener::SetDirection(const rtmcpp::Vec3& forward) const
	{
		//NZ_PROFILE_FUNCTION();

		auto* engine = static_cast<ma_engine*>(AudioEngine::GetEngine());
		ma_engine_listener_set_direction(engine, m_ListenerIndex, forward.X, forward.Y, forward.Z);
	}

	void AudioListener::SetVelocity(const rtmcpp::Vec3& velocity) const
	{
		//NZ_PROFILE_FUNCTION();

		auto* engine = static_cast<ma_engine*>(AudioEngine::GetEngine());
		ma_engine_listener_set_velocity(engine, m_ListenerIndex, velocity.X, velocity.Y, velocity.Z);
	}
}
