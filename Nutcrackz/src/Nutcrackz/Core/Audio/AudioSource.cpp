#include "nzpch.hpp"
#include "AudioSource.hpp"

#include "AudioEngine.hpp"
#include "Nutcrackz/Asset/AudioImporter.hpp"
#include "Nutcrackz/Project/Project.hpp"

namespace Nutcrackz {

	AudioSource::AudioSource()
	{
		//NZ_PROFILE_FUNCTION();

		m_Sound = std::make_unique<ma_sound>();
	}

	AudioSource::~AudioSource()
	{
		//NZ_PROFILE_FUNCTION();
		
		if (!AudioEngine::ShuttingDownEngine())
		{
			if (IsPlaying())
			{
				if (ma_sound_stop(m_Sound.get()) != MA_SUCCESS)
				{
					NZ_CORE_ERROR("Failed to stop playback device!");
					ma_sound_uninit(m_Sound.get());
				}

				ma_sound_uninit(m_Sound.get());
				m_Sound = nullptr;
			}
		}
	}

	void AudioSource::Play()
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_start(m_Sound.get());
		}
	}

	void AudioSource::Pause()
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_stop(m_Sound.get());
		}
	}

	void AudioSource::UnPause()
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_start(m_Sound.get());
		}
	}

	void AudioSource::Stop()
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_stop(m_Sound.get());
			ma_sound_seek_to_pcm_frame(m_Sound.get(), 0);

			m_CursorPos = 0;
		}
	}

	bool AudioSource::IsPlaying()
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound.get())
			return ma_sound_is_playing(m_Sound.get());

		return false;
	}

	uint64_t AudioSource::GetCursorPosition()
	{
		if (m_Sound.get())
		{
			//uint64_t cursorPos = 0;
			ma_sound_get_cursor_in_pcm_frames(m_Sound.get(), &m_CursorPos);
			return m_CursorPos;
		}

		return 0;
	}

	static ma_attenuation_model GetAttenuationModel(AttenuationModelType model)
	{
		//NZ_PROFILE_FUNCTION();

		switch (model)
		{
		case AttenuationModelType::None:		return ma_attenuation_model_none;
		case AttenuationModelType::Inverse:		return ma_attenuation_model_inverse;
		case AttenuationModelType::Linear:		return ma_attenuation_model_linear;
		case AttenuationModelType::Exponential: return ma_attenuation_model_exponential;
		}

		return ma_attenuation_model_none;
	}

	void AudioSource::SetConfig(AudioSourceConfig& config)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound* sound = m_Sound.get();
			ma_sound_set_volume(sound, config.VolumeMultiplier);
			ma_sound_set_pitch(sound, config.PitchMultiplier);

			if (sound)
			{
				if (config.Looping)
					ma_sound_set_looping(sound, MA_TRUE);
				else
					ma_sound_set_looping(sound, MA_FALSE);
			}

			if (m_Spatialization != config.Spatialization)
			{
				m_Spatialization = config.Spatialization;
				ma_sound_set_spatialization_enabled(sound, config.Spatialization);
			}

			if (config.Spatialization)
			{
				ma_sound_set_attenuation_model(sound, GetAttenuationModel(config.AttenuationModel));
				ma_sound_set_rolloff(sound, config.RollOff);
				ma_sound_set_min_gain(sound, config.MinGain);
				ma_sound_set_max_gain(sound, config.MaxGain);
				ma_sound_set_min_distance(sound, config.MinDistance);
				ma_sound_set_max_distance(sound, config.MaxDistance);

				ma_sound_set_cone(sound, config.ConeInnerAngle, config.ConeOuterAngle, config.ConeOuterGain);
				ma_sound_set_doppler_factor(sound, std::max(config.DopplerFactor, 0.0f));
			}
			else
			{
				ma_sound_set_attenuation_model(sound, ma_attenuation_model_none);
			}
		}
	}

	void AudioSource::SetVolume(float volume)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_volume(m_Sound.get(), volume);
		}
	}

	void AudioSource::SetPitch(float pitch)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_pitch(m_Sound.get(), pitch);
		}
	}

	bool AudioSource::IsLooping()
	{
		if (m_Sound)
			return ma_sound_is_looping(m_Sound.get());

		return false;
	}

	void AudioSource::SetLooping(bool state)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			if (state)
				ma_sound_set_looping(m_Sound.get(), MA_TRUE);
			else
				ma_sound_set_looping(m_Sound.get(), MA_FALSE);
		}
	}

	void AudioSource::SetSpatialization(bool state)
	{
		//NZ_PROFILE_FUNCTION();

		m_Spatialization = state;
		if (m_Sound)
		{
			ma_sound_set_spatialization_enabled(m_Sound.get(), state);
		}
	}

	void AudioSource::SetAttenuationModel(AttenuationModelType type)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			if (m_Spatialization)
				ma_sound_set_attenuation_model(m_Sound.get(), GetAttenuationModel(type));
			else
				ma_sound_set_attenuation_model(m_Sound.get(), GetAttenuationModel(AttenuationModelType::None));
		}
	}

	void AudioSource::SetRollOff(float rollOff)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_rolloff(m_Sound.get(), rollOff);
		}
	}

	void AudioSource::SetMinGain(float minGain)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_min_gain(m_Sound.get(), minGain);
		}
	}

	void AudioSource::SetMaxGain(float maxGain)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_max_gain(m_Sound.get(), maxGain);
		}
	}

	void AudioSource::SetMinDistance(float minDistance)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_min_distance(m_Sound.get(), minDistance);
		}
	}

	void AudioSource::SetMaxDistance(float maxDistance)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_max_distance(m_Sound.get(), maxDistance);
		}
	}

	void AudioSource::SetCone(float innerAngle, float outerAngle, float outerGain)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_cone(m_Sound.get(), innerAngle, outerAngle, outerGain);
		}
	}

	void AudioSource::SetDopplerFactor(float factor)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_doppler_factor(m_Sound.get(), std::max(factor, 0.0f));
		}
	}

	void AudioSource::SetPosition(const rtmcpp::Vec4& position)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_position(m_Sound.get(), position.X, position.Y, position.Z);
		}
	}

	void AudioSource::SetDirection(const rtmcpp::Vec3& forward)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_direction(m_Sound.get(), forward.X, forward.Y, forward.Z);
		}
	}

	void AudioSource::SetVelocity(const rtmcpp::Vec3& velocity)
	{
		//NZ_PROFILE_FUNCTION();

		if (m_Sound)
		{
			ma_sound_set_velocity(m_Sound.get(), velocity.X, velocity.Y, velocity.Z);
		}
	}

}
