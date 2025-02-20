#include "nzpch.hpp"
#include "TextureImporter.hpp"

#include "Nutcrackz/Project/Project.hpp"

#include <stb_image.h>

namespace Nutcrackz {

	RefPtr<Texture2D> TextureImporter::ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata)
	{
		//NZ_PROFILE_FUNCTION();

		return LoadTexture2D(Project::GetActiveAssetDirectory() / metadata.FilePath);
	}

	RefPtr<Texture2D> TextureImporter::LoadTexture2D(const std::filesystem::path& path)
	{
		//NZ_PROFILE_FUNCTION();
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		Buffer data;

		{
			//NZ_PROFILE_SCOPE("stbi_load - TextureImporter::ImportTexture2D");
			std::string pathStr = path.string();
			data.Data = stbi_load(pathStr.c_str(), &width, &height, &channels, 4);
			channels = 4;
		}

		if (data.Data == nullptr)
		{
			NZ_CORE_ERROR("TextureImporter::ImportTexture2D - Could not load texture from filepath: {}", path.string());
			return nullptr;
		}

		// TODO: think about this
		data.Size = width * height * channels;

		TextureSpecification spec;
		spec.Width = width;
		spec.Height = height;
		switch (channels)
		{
		case 3:
			spec.Format = ImageFormat::RGB8;
			break;
		case 4:
			spec.Format = ImageFormat::RGBA8;
			break;
		}

		RefPtr<Texture2D> texture = Texture2D::Create(spec, data);
		data.Release();
		return texture;
	}

	RefPtr<VideoTexture> TextureImporter::ImportVideoTexture(AssetHandle handle, const AssetMetadata& metadata)
	{
		//NZ_PROFILE_FUNCTION();

		std::filesystem::path filepath = Project::GetActiveAssetDirectory() / metadata.FilePath;
		Buffer data;
		VideoReaderState videoState;

		{
			//NZ_PROFILE_SCOPE("stbi_load - TextureImporter::ImportVideoTexture");

			if (!VideoTexture::VideoReaderOpen(&videoState, filepath))
			{
				NZ_CORE_ERROR("TextureImporter::ImportVideoTexture - Could not open video from filepath: {}", filepath.string());
				return nullptr;
			}

			if (!VideoTexture::AudioReaderOpen(&videoState, filepath))
				NZ_CORE_ERROR("TextureImporter::ImportVideoTexture - Could not open audio in video from filepath: {}", filepath.string());
		}

		TextureSpecification spec;
		spec.Width = videoState.Width;
		spec.Height = videoState.Height;
		spec.Format = ImageFormat::RGBA8;

		data.Data = new uint8_t[videoState.Width * videoState.Height * 4];
		data.Size = videoState.Width * videoState.Height * 4;

		if (data.Data == nullptr)
		{
			NZ_CORE_ERROR("TextureImporter::ImportVideoTexture - Could not load video from filepath: {}", filepath.string());
			return nullptr;
		}

		RefPtr<VideoTexture> texture = VideoTexture::Create(spec, data, videoState);
		data.Release();
		return texture;
	}

}
