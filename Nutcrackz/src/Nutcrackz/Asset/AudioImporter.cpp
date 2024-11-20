#include "nzpch.hpp"
#include "AudioImporter.hpp"

#include "Nutcrackz/Core/Audio/AudioEngine.hpp"

#include "Nutcrackz/Project/Project.hpp"
#include "Nutcrackz/Scene/SceneSerializer.hpp"

namespace Nutcrackz {

	RefPtr<AudioSource> AudioImporter::ImportAudio(AssetHandle handle, const AssetMetadata& metadata)
	{
		//NZ_PROFILE_FUNCTION();

		return LoadAudio(Project::GetActiveAssetDirectory() / metadata.FilePath);
	}

	RefPtr<AudioSource> AudioImporter::LoadAudio(const std::filesystem::path& path)
	{
		NZ_PROFILE_FUNCTION_COLOR("AudioImporter::LoadAudio", 0xD17F8A);
		RefPtr<AudioSource> audioSource = RefPtr<AudioSource>::Create();

		auto* engine = static_cast<ma_engine*>(AudioEngine::GetEngine());

		{
			NZ_PROFILE_SCOPE_COLOR("AudioImporter::LoadAudio Scope", 0x3C7F8A);

			const ma_result result = ma_sound_init_from_file(engine, path.string().c_str(), MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, nullptr, audioSource->GetSound().get());
			if (result != MA_SUCCESS)
				NZ_CORE_ERROR("Failed to initialize sound: {}", path.string());
		}

		return audioSource;
	}
}
