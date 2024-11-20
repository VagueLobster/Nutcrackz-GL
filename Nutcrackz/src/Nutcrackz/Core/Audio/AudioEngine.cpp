#include "nzpch.hpp"
#include "AudioEngine.hpp"
#include "AudioSource.hpp"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c" // Enables Vorbis decoding.

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c" // Enables Vorbis decoding.

namespace Nutcrackz {

	ma_engine* AudioEngine::s_Engine;

	void AudioEngine::Init()
	{
		//NZ_PROFILE_FUNCTION();

		s_ShuttingDown = false;
		ma_engine_config engineConfig = ma_engine_config_init();
		engineConfig.listenerCount = 1;

		s_Engine = new ma_engine();
		ma_result result = ma_engine_init(&engineConfig, s_Engine);
		if (result != MA_SUCCESS)
		{
			ma_engine_uninit(s_Engine);
			delete s_Engine;

			NZ_CORE_ERROR("Failed to initialize audio engine!");
		}
	}

	void AudioEngine::Shutdown()
	{
		//NZ_PROFILE_FUNCTION();
		
		s_ShuttingDown = true;
		ma_engine_stop(s_Engine);
		ma_engine_uninit(s_Engine);
		delete s_Engine;
	}

}
