#pragma once

#include "Nutcrackz/Asset/Asset.hpp"

#include "miniaudio.h"

#include <string>

//#include <glm/glm.hpp>
#include "rtmcpp/Common.hpp"
#include "rtmcpp/Scalar.hpp"

struct ma_sound;

namespace Nutcrackz {

	enum class AttenuationModelType
	{
		None = 0,
		Inverse,
		Linear,
		Exponential
	};

	struct AudioSourceConfig
	{
		float VolumeMultiplier = 1.0f;
		float PitchMultiplier = 1.0f;
		bool PlayOnAwake = true;
		bool Looping = false;

		bool Spatialization = false;
		AttenuationModelType AttenuationModel = AttenuationModelType::Inverse;
		float RollOff = 1.0f;
		float MinGain = 0.0f;
		float MaxGain = 1.0f;
		float MinDistance = 0.3f;
		float MaxDistance = 1000.0f;

		float ConeInnerAngle = rtmcpp::Radians(360.0f);
		float ConeOuterAngle = rtmcpp::Radians(360.0f);
		float ConeOuterGain = 0.0f;

		float DopplerFactor = 1.0f;
	};

	class AudioSource : public Asset
	{
	public:
		AudioSource();
		~AudioSource();

		virtual AssetType GetType() const { return AssetType::Audio; }
		std::unique_ptr<ma_sound>& GetSound() { return m_Sound; }

		void Play();
		void Pause();
		void UnPause();
		void Stop();
		bool IsPlaying();
		uint64_t GetCursorPosition();

		void SetConfig(AudioSourceConfig& config);

		void SetVolume(float volume);
		void SetPitch(float pitch);
		bool IsLooping();
		void SetLooping(bool state);
		void SetSpatialization(bool state);
		void SetAttenuationModel(AttenuationModelType type);
		void SetRollOff(float rollOff);
		void SetMinGain(float minGain);
		void SetMaxGain(float maxGain);
		void SetMinDistance(float minDistance);
		void SetMaxDistance(float maxDistance);
		void SetCone(float innerAngle, float outerAngle, float outerGain);
		void SetDopplerFactor(float factor);

		void SetPosition(const rtmcpp::Vec4& position);
		void SetDirection(const rtmcpp::Vec3& forward);
		void SetVelocity(const rtmcpp::Vec3& velocity);

	private:
		std::unique_ptr<ma_sound> m_Sound;
		bool m_Spatialization = false;
		uint64_t m_CursorPos = 0;
	};
}