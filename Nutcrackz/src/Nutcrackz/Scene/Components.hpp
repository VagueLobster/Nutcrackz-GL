#pragma once

#include "Nutcrackz/Core/UUID.hpp"
#include "Nutcrackz/Core/Audio/AudioListener.hpp"
#include "Nutcrackz/Core/Audio/AudioSource.hpp"
#include "Nutcrackz/Renderer/Texture.hpp"
#include "Nutcrackz/Video/VideoTexture.hpp"
#include "Nutcrackz/Renderer/Font.hpp"
#include "Nutcrackz/Renderer/ParticleSystem.hpp"
#include "SceneCamera.hpp"

#include "Nutcrackz/Asset/AssetManager.hpp"
#include "Nutcrackz/Scripting/CSharpObject.hpp"

#include "rtmcpp/Common.hpp"
#include "rtmcpp/Transforms.hpp"

#include <fstream>

namespace Nutcrackz {

	struct AnimationData
	{
		bool UseParallaxScrolling = false;
		rtmcpp::Vec2 ParallaxSpeed{ 0.0f, 0.0f };
		float ParallaxDivision = 0.0f;
		bool UseTextureAtlasAnimation = false;
		float AnimationSpeed = 7.5f;
		float AnimationTime = 0.0f;
		float AnimationTimePaused = 0.0f;
		int NumberOfTiles = 1;
		int StartIndexX = 0;
		int StartIndexY = 0;
		int Rows = 0;
		int Columns = 0;

		bool UseLinear = false;
		bool UseCameraParallaxX = false;
		bool UseCameraParallaxY = false;

		// For Texture Animation:
		std::vector<AssetHandle> Textures;
		bool UsePerTextureAnimation = false;
		int StartIndex = 0;
		uint32_t NumberOfTextures = 0; // Used for storing the size of Textures for the texture animation!
		rtmcpp::Vec2 ScrollingDivision = { 0.0f, 0.0f };
	};

	struct VideoData
	{
		bool UseBillboard = false;
		bool UseExternalAudio = false;
		bool PlayVideo = false;
		bool RepeatVideo = false;
		bool PauseVideo = false;
		bool HasInitializedTimer = false;
		bool IsRenderingVideo = false;
		bool SeekAudio = true;

		uint8_t* VideoFrameData = nullptr;
		uint32_t VideoRendererID = 0;

		int64_t PresentationTimeStamp = 0;
		int64_t NumberOfFrames = 0;
		int64_t FramePosition = 0;
		int64_t Hours = 0;
		int64_t Minutes = 0;
		int64_t Seconds = 0;
		int64_t Milliseconds = 0;

		float Volume = 100.0f;

		double RestartPointFromPause = 0.0;
		double PresentationTimeInSeconds = 0.0;
		double VideoDuration = 0.0;
	};

	struct AudioData // For audio sources only!
	{
		std::vector<AssetHandle> Playlist;
		bool UsePlaylist = false;
		bool RepeatPlaylist = false;
		bool RepeatAfterSpecificTrackPlays = false;
		bool PlayingCurrentIndex = false;
		uint32_t NumberOfAudioSources = 0;
		uint32_t OldIndex = 0;
		uint32_t CurrentIndex = 0;
		uint32_t StartIndex = 0;

		// For Scene:
		bool HasPlayedAudioSource = false;

		// Copies
		std::vector<AssetHandle> PlaylistCopy;
	};

	struct IDComponent
	{
		uint64_t ID = 0;

		IDComponent() = default;
		IDComponent(const uint64_t& id)
		{
			ID = id;
		}
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		//TagComponent(const TagComponent& other) = default;
		TagComponent(const TagComponent& other)
		{
			Tag = other.Tag;
		}
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	struct TransformComponent
	{
		bool Enabled = true;
		bool OverrideSelectedEntitysOutline = false;
		rtmcpp::Vec4 Translation = rtmcpp::Vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
		rtmcpp::Vec3 Rotation = rtmcpp::Vec3{ 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 Scale = rtmcpp::Vec3{ 1.0f, 1.0f, 1.0f };
		
		TransformComponent() = default;
		//TransformComponent(const TransformComponent& other) = default;
		TransformComponent(const TransformComponent& other)
		{
			Enabled = other.Enabled;
			OverrideSelectedEntitysOutline = other.OverrideSelectedEntitysOutline;
			Translation = other.Translation;
			Rotation = other.Rotation;
			Scale = other.Scale;
		}
		TransformComponent(const rtmcpp::Vec4& translation)
			: Translation(translation) {}

		/*TransformComponent operator=(const TransformComponent& other)
		{
			Enabled = other.Enabled;
			OverrideSelectedEntitysOutline = other.OverrideSelectedEntitysOutline;
			Translation = other.Translation;
			Rotation = other.Rotation;
			Scale = other.Scale;
		}*/

		rtmcpp::Mat4 GetTransform() const
		{
			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(Rotation));
			//rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(rtmcpp::Vec3{ Rotation.X, Rotation.Y, Rotation.Z }));
			//rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(rtmcpp::Vec3{ Rotation.Y, Rotation.Z, Rotation.X }));
			//rtmcpp::Vec3 tl = rtmcpp::Vec3{ Translation.X, Translation.Y, Translation.Z };
			//rtmcpp::Mat4 rotation = rtmcpp::Rotation(rtmcpp::FromEuler(Rotation));
			//rtmcpp::Mat4 translation = rtmcpp::Translation(tl);

			return rtmcpp::Mat4Cast(rtmcpp::Scale(Scale))
				* rotation
				//* rtmcpp::Mat4Cast(rtmcpp::FromEuler(Rotation))
				//* rtmcpp::Mat4Cast(rtmcpp::Translation(tl));
				//* translation;
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ Translation.X, Translation.Y, Translation.Z }));

			//return rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ Translation.X, Translation.Y, Translation.Z }))
			//	* rotation
			//	* rtmcpp::Mat4Cast(rtmcpp::Scale(Scale));
		}
	};

	struct SpriteRendererComponent
	{
		rtmcpp::Vec4 SpriteTranslation = rtmcpp::Vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
		rtmcpp::Vec3 SpriteRotation = rtmcpp::Vec3{ 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 SpriteScale = rtmcpp::Vec3{ 1.0f, 1.0f, 1.0f };

		float Saturation = 1.0f;
		rtmcpp::Vec4 Color = rtmcpp::Vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec2 UV = rtmcpp::Vec2{ 1.0f, 1.0f };
		AssetHandle TextureHandle = 0;
		AnimationData m_AnimationData;
		RefPtr<Texture2D> Texture = nullptr;

		SpriteRendererComponent() = default;
		//SpriteRendererComponent(const SpriteRendererComponent& other) = default;
		SpriteRendererComponent(const SpriteRendererComponent& other)
		{
			SpriteTranslation = other.SpriteTranslation;
			SpriteRotation = other.SpriteRotation;
			SpriteScale = other.SpriteScale;

			Saturation = other.Saturation;
			Color = other.Color;
			UV = other.UV;
			TextureHandle = other.TextureHandle;
			m_AnimationData = other.m_AnimationData;
			Texture = other.Texture;
		}
		SpriteRendererComponent(const rtmcpp::Vec4& color)
			: Color(color) {}
		
		RefPtr<Texture2D> GetAnimatedTexture(uint32_t index) const { return AssetManager::GetAsset<Texture2D>(m_AnimationData.Textures[index]); }
		void SetAnimatedTexture(uint32_t index) { TextureHandle = m_AnimationData.Textures[index]; }
		
		void AddAnimationTexture(AssetHandle& texture)
		{
			m_AnimationData.Textures.emplace_back(texture);
			m_AnimationData.NumberOfTextures = (uint32_t)m_AnimationData.Textures.size();
		}

		void RemoveAnimationTexture(uint32_t index)
		{
			m_AnimationData.Textures.erase(m_AnimationData.Textures.begin() + index);
			m_AnimationData.Textures.shrink_to_fit();
			m_AnimationData.NumberOfTextures = (uint32_t)m_AnimationData.Textures.size();
		}

		void RemoveAnimationTexture(AssetHandle& texture)
		{
			uint32_t index = 0;

			for (uint32_t i = 0; i < m_AnimationData.Textures.size(); i++)
			{
				AssetHandle animTexture = m_AnimationData.Textures[i];

				if (animTexture == texture)
				{
					index = i;
				}
			}

			m_AnimationData.Textures.erase(m_AnimationData.Textures.begin() + index);
			m_AnimationData.Textures.shrink_to_fit();
			m_AnimationData.NumberOfTextures = (uint32_t)m_AnimationData.Textures.size();
		}

		rtmcpp::Mat4 GetTransform() const
		{
			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(SpriteRotation));

			return rtmcpp::Mat4Cast(rtmcpp::Scale(SpriteScale))
				* rotation
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ SpriteTranslation.X, SpriteTranslation.Y, SpriteTranslation.Z }));

			//return rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ SpriteTranslation.X, SpriteTranslation.Y, SpriteTranslation.Z }))
			//	* rotation
			//	* rtmcpp::Mat4Cast(rtmcpp::Scale(SpriteScale));
		}
	};

	struct CircleRendererComponent
	{
		rtmcpp::Vec4 CircleTranslation = rtmcpp::Vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
		rtmcpp::Vec3 CircleRotation = rtmcpp::Vec3{ 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 CircleScale = rtmcpp::Vec3{ 1.0f, 1.0f, 1.0f };

		rtmcpp::Vec4 Color = rtmcpp::Vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec2 UV = rtmcpp::Vec2{ 1.0f, 1.0f };
		float Thickness = 1.0f;
		float OldThickness = 1.0f;
		float Fade = 0.005f;
		AssetHandle TextureHandle = 0;
		RefPtr<Texture2D> Texture = nullptr;
		AnimationData m_AnimationData;
		bool MakeSolid = false;

		CircleRendererComponent() = default;
		//CircleRendererComponent(const CircleRendererComponent& other) = default;
		CircleRendererComponent(const CircleRendererComponent& other)
		{
			CircleTranslation = other.CircleTranslation;
			CircleRotation = other.CircleRotation;
			CircleScale = other.CircleScale;

			Color = other.Color;
			UV = other.UV;
			Thickness = other.Thickness;
			OldThickness = other.OldThickness;
			Fade = other.Fade;
			TextureHandle = other.TextureHandle;
			Texture = other.Texture;
			m_AnimationData = other.m_AnimationData;
			MakeSolid = other.MakeSolid;
		}
		CircleRendererComponent(const rtmcpp::Vec4& color)
			: Color(color) {}

		/*glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(CircleRotation));

			return glm::translate(glm::mat4(1.0f), CircleTranslation)
				* rotation
				* glm::scale(glm::mat4(1.0f), CircleScale);
		}*/

		rtmcpp::Mat4 GetTransform() const
		{
			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(CircleRotation));

			return rtmcpp::Mat4Cast(rtmcpp::Scale(CircleScale))
				* rotation
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ CircleTranslation.X, CircleTranslation.Y, CircleTranslation.Z }));

			//return rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ CircleTranslation.X, CircleTranslation.Y, CircleTranslation.Z }))
			//	* rotation
			//	* rtmcpp::Mat4Cast(rtmcpp::Scale(CircleScale));
		}
	};

	struct TriangleRendererComponent
	{
		rtmcpp::Vec4 TriangleTranslation = { 0.0f, 0.0f, 0.0f, 1.0f };
		rtmcpp::Vec3 TriangleRotation = { 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 TriangleScale = { 1.0f, 1.0f, 1.0f };
		float Saturation = 1.0f;
		rtmcpp::Vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec2 UV{ 1.0f, 1.0f };
		AssetHandle TextureHandle = 0;

		TriangleRendererComponent() = default;
		//TriangleRendererComponent(const TriangleRendererComponent& other) = default;
		TriangleRendererComponent(const TriangleRendererComponent& other)
		{
			TriangleTranslation = other.TriangleTranslation;
			TriangleRotation = other.TriangleRotation;
			TriangleScale = other.TriangleScale;
			Saturation = other.Saturation;
			Color = other.Color;
			UV = other.UV;
			TextureHandle = other.TextureHandle;
		}
		TriangleRendererComponent(const rtmcpp::Vec4& color)
			: Color(color) {}

		rtmcpp::Mat4 GetTransform() const
		{
			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(TriangleRotation));

			return rtmcpp::Mat4Cast(rtmcpp::Scale(TriangleScale))
				* rotation
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ TriangleTranslation.X, TriangleTranslation.Y, TriangleTranslation.Z }));

			//return rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ TriangleTranslation.X, TriangleTranslation.Y, TriangleTranslation.Z }))
			//	* rotation
			//	* rtmcpp::Mat4Cast(rtmcpp::Scale(TriangleScale));
		}
	};

	struct LineRendererComponent
	{
		float LineThickness = 1.0f;
		std::vector<rtmcpp::Vec4> Colors;
		std::vector<rtmcpp::Vec4> Translations;
		uint32_t NumberOfColors = 0;
		uint32_t NumberOfTranslations = 0;

		LineRendererComponent() = default;
		//LineRendererComponent(const LineRendererComponent& other) = default;
		LineRendererComponent(const LineRendererComponent& other)
		{
			LineThickness = other.LineThickness;
			Colors = other.Colors;
			Translations = other.Translations;
			NumberOfColors = other.NumberOfColors;
			NumberOfTranslations = other.NumberOfTranslations;
		}

		void AddColor(const rtmcpp::Vec4& color)
		{
			Colors.emplace_back(color);
			NumberOfColors = (uint32_t)Colors.size();
		}

		void RemoveColor(uint32_t index)
		{
			Colors.erase(Colors.begin() + index);
			Colors.shrink_to_fit();
			NumberOfColors = (uint32_t)Colors.size();
		}

		rtmcpp::Vec4 GetColor(uint32_t index)
		{
			return Colors[index];
		}

		void AddTranslation(const rtmcpp::Vec4& translation)
		{
			Translations.emplace_back(translation);
			NumberOfTranslations = (uint32_t)Translations.size();
		}

		void RemoveTranslation(uint32_t index)
		{
			Translations.erase(Translations.begin() + index);
			Translations.shrink_to_fit();
			NumberOfTranslations = (uint32_t)Translations.size();
		}

		rtmcpp::Vec4 GetTranslation(uint32_t index)
		{
			return Translations[index];
		}
	};

	struct TextComponent
	{
		std::string TextString = "";

		// Font
		rtmcpp::Vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec4 BgColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		float LineSpacing = 0.0f;
		float Kerning = 0.0f;
		bool UseLinear = false;
		AssetHandle FontHandle = 0;
		bool LinearFilteringOn = true;

		rtmcpp::Vec2 m_TextQuadMin = rtmcpp::Vec2(0.0f, 0.0f);
		rtmcpp::Vec2 m_TextQuadMax = rtmcpp::Vec2(0.0f, 0.0f);

		rtmcpp::Vec2 m_TexCoordMin = rtmcpp::Vec2(0.0f, 0.0f);
		rtmcpp::Vec2 m_TexCoordMax = rtmcpp::Vec2(0.0f, 0.0f);

		rtmcpp::Vec2 m_PlaneMin = rtmcpp::Vec2(0.0f, 0.0f);
		rtmcpp::Vec2 m_PlaneMax = rtmcpp::Vec2(0.0f, 0.0f);

		float m_NewLineCounter = 0.0f;
		float m_YMinOffset = 0.0f;
		float m_YMaxOffset = 0.0f;

		TextComponent() = default;
		//TextComponent(const TextComponent& other) = default;
		TextComponent(const TextComponent& other)
		{
			TextString = other.TextString;

			// Font
			Color = other.Color;
			BgColor = other.BgColor;
			LineSpacing = other.LineSpacing;
			Kerning = other.Kerning;
			UseLinear = other.UseLinear;
			FontHandle = other.FontHandle;
			LinearFilteringOn = other.LinearFilteringOn;

			m_TextQuadMin = other.m_TextQuadMin;
			m_TextQuadMax = other.m_TextQuadMax;

			m_TexCoordMin = other.m_TexCoordMin;
			m_TexCoordMax = other.m_TexCoordMax;

			m_PlaneMin = other.m_PlaneMin;
			m_PlaneMax = other.m_PlaneMax;

			m_NewLineCounter = other.m_NewLineCounter;
			m_YMinOffset = other.m_YMinOffset;
			m_YMaxOffset = other.m_YMaxOffset;
		}
		TextComponent(const rtmcpp::Vec4& color)
			: Color(color) {}

		const rtmcpp::Vec2& GetTextQuadMin() { return m_TextQuadMin; }
		const rtmcpp::Vec2& GetTextQuadMax() { return m_TextQuadMax; }

		const rtmcpp::Vec2& GetTexCoordMin() { return m_TexCoordMin; }
		const rtmcpp::Vec2& GetTexCoordMax() { return m_TexCoordMax; }

		const rtmcpp::Vec2& GetPlaneMin() { return m_PlaneMin; }
		const rtmcpp::Vec2& GetPlaneMax() { return m_PlaneMax; }

		float GetNewLines() { return m_NewLineCounter; }
		float GetYMinOffset() { return m_YMinOffset; }
		float GetYMaxOffset() { return m_YMaxOffset; }
	};

	struct ParticleSystemComponent
	{
		rtmcpp::Vec3 Velocity = { 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 VelocityVariation = { 3.0f, 1.0f, 0.0f };
		rtmcpp::Vec4 ColorBegin = { 254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f };
		rtmcpp::Vec4 ColorEnd = { 254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f };
		float SizeBegin = 0.5f;
		float SizeEnd = 0.0f;
		float SizeVariation = 0.3f;
		float LifeTime = 1.0f;
		int ParticleSize = 5;
		bool UseLinear = false;
		AssetHandle TextureHandle = 0;
		RefPtr<Texture2D> Texture = nullptr;
		ParticleSystem m_ParticleSystem = ParticleSystem(1000);
		bool UseBillboard = false;

		ParticleSystemComponent() = default;
		//ParticleSystemComponent(const ParticleSystemComponent& other) = default;
		ParticleSystemComponent(const ParticleSystemComponent& other)
		{
			Velocity = other.Velocity;
			VelocityVariation = other.VelocityVariation;
			ColorBegin = other.ColorBegin;
			ColorEnd = other.ColorEnd;
			SizeBegin = other.SizeBegin;
			SizeEnd = other.SizeEnd;
			SizeVariation = other.SizeVariation;
			LifeTime = other.LifeTime;
			ParticleSize = other.ParticleSize;
			UseLinear = other.UseLinear;
			TextureHandle = other.TextureHandle;
			Texture = other.Texture;
			m_ParticleSystem = other.m_ParticleSystem;
			UseBillboard = other.UseBillboard;
		}

		ParticleProps Particle;

		void UpdateParticleProps()
		{
			Particle.ColorBegin = ColorBegin;
			Particle.ColorEnd = ColorEnd;
			Particle.SizeBegin = SizeBegin;
			Particle.SizeVariation = SizeVariation;
			Particle.SizeEnd = SizeEnd;
			Particle.LifeTime = LifeTime;
			Particle.Velocity = Velocity;
			Particle.VelocityVariation = VelocityVariation;
		}
	};

	struct CubeRendererComponent
	{
		rtmcpp::Vec4 CubeTranslation = { 0.0f, 0.0f, 0.0f, 1.0f };
		rtmcpp::Vec3 CubeRotation = { 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 CubeScale = { 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec2 UV{ 1.0f, 1.0f };
		AssetHandle TextureHandle = 0;
		rtmcpp::Vec3 MaterialSpecular = rtmcpp::Vec3(0.5f, 0.5f, 0.5f);
		float MaterialShininess = 64.0f;
		AnimationData m_AnimationData;

		CubeRendererComponent() = default;
		//CubeRendererComponent(const CubeRendererComponent& other) = default;
		CubeRendererComponent(const CubeRendererComponent& other)
		{
			CubeTranslation = other.CubeTranslation;
			CubeRotation = other.CubeRotation;
			CubeScale = other.CubeScale;
			Color = other.Color;
			UV = other.UV;
			TextureHandle = other.TextureHandle;
			MaterialSpecular = other.MaterialSpecular;
			MaterialShininess = other.MaterialShininess;
			m_AnimationData = other.m_AnimationData;
		}
		CubeRendererComponent(const rtmcpp::Vec4& color)
			: Color(color) {}

		AssetHandle GetAnimatedTexture(uint32_t index) const { return m_AnimationData.Textures[index]; }
		void SetAnimatedTexture(uint32_t index) { TextureHandle = m_AnimationData.Textures[index]; }

		void AddAnimationTexture(AssetHandle& texture)
		{
			m_AnimationData.Textures.emplace_back(texture);
			m_AnimationData.NumberOfTextures = (uint32_t)m_AnimationData.Textures.size();
		}

		void RemoveAnimationTexture(uint32_t index)
		{
			m_AnimationData.Textures.erase(m_AnimationData.Textures.begin() + index);
			m_AnimationData.Textures.shrink_to_fit();
			m_AnimationData.NumberOfTextures = (uint32_t)m_AnimationData.Textures.size();
		}

		void RemoveAnimationTexture(AssetHandle& texture)
		{
			uint32_t index = 0;

			for (uint32_t i = 0; i < m_AnimationData.Textures.size(); i++)
			{
				if (m_AnimationData.Textures[i] == texture)
				{
					index = i;
				}
			}

			m_AnimationData.Textures.erase(m_AnimationData.Textures.begin() + index);
			m_AnimationData.Textures.shrink_to_fit();
			m_AnimationData.NumberOfTextures = (uint32_t)m_AnimationData.Textures.size();
		}

		/*glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(CubeRotation));

			return glm::translate(glm::mat4(1.0f), CubeTranslation)
				* rotation
				* glm::scale(glm::mat4(1.0f), CubeScale);
		}*/

		rtmcpp::Mat4 GetTransform() const
		{
			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(CubeRotation));

			return rtmcpp::Mat4Cast(rtmcpp::Scale(CubeScale))
				* rotation
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ CubeTranslation.X, CubeTranslation.Y, CubeTranslation.Z }));
			
			//return rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ CubeTranslation.X, CubeTranslation.Y, CubeTranslation.Z }))
			//	* rotation
			//	* rtmcpp::Mat4Cast(rtmcpp::Scale(CubeScale));
		}
	};
	
	struct PyramidRendererComponent
	{
		rtmcpp::Vec4 PyramidTranslation = { 0.0f, 0.0f, 0.0f, 1.0f };
		rtmcpp::Vec3 PyramidRotation = { 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 PyramidScale = { 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec2 UV{ 1.0f, 1.0f };
		AssetHandle TextureHandle = 0;
		rtmcpp::Vec3 MaterialSpecular = rtmcpp::Vec3(0.5f, 0.5f, 0.5f);
		float MaterialShininess = 64.0f;

		PyramidRendererComponent() = default;
		//PyramidRendererComponent(const PyramidRendererComponent& other) = default;
		PyramidRendererComponent(const PyramidRendererComponent& other)
		{
			PyramidTranslation = other.PyramidTranslation;
			PyramidRotation = other.PyramidRotation;
			PyramidScale = other.PyramidScale;
			Color = other.Color;
			UV = other.UV;
			TextureHandle = other.TextureHandle;
			MaterialSpecular = other.MaterialSpecular;
			MaterialShininess = other.MaterialShininess;
		}
		PyramidRendererComponent(const rtmcpp::Vec4& color)
			: Color(color) {}

		/*glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(PyramidRotation));

			return glm::translate(glm::mat4(1.0f), PyramidTranslation)
				* rotation
				* glm::scale(glm::mat4(1.0f), PyramidScale);
		}*/

		rtmcpp::Mat4 GetTransform() const
		{
			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(PyramidRotation));

			return rtmcpp::Mat4Cast(rtmcpp::Scale(PyramidScale))
				* rotation
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ PyramidTranslation.X, PyramidTranslation.Y, PyramidTranslation.Z }));

			//return rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ PyramidTranslation.X, PyramidTranslation.Y, PyramidTranslation.Z }))
			//	* rotation
			//	* rtmcpp::Mat4Cast(rtmcpp::Scale(PyramidScale));
		}
	};
	
	struct TriangularPrismRendererComponent
	{
		rtmcpp::Vec4 TriangleTranslation = { 0.0f, 0.0f, 0.0f, 1.0f };
		rtmcpp::Vec3 TriangleRotation = { 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 TriangleScale = { 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec2 UV{ 1.0f, 1.0f };
		AssetHandle TextureHandle = 0;
		rtmcpp::Vec3 MaterialSpecular = rtmcpp::Vec3(0.5f, 0.5f, 0.5f);
		float MaterialShininess = 64.0f;

		TriangularPrismRendererComponent() = default;
		//TriangularPrismRendererComponent(const TriangularPrismRendererComponent& other) = default;
		TriangularPrismRendererComponent(const TriangularPrismRendererComponent& other)
		{
			TriangleTranslation = other.TriangleTranslation;
			TriangleRotation = other.TriangleRotation;
			TriangleScale = other.TriangleScale;
			Color = other.Color;
			UV = other.UV;
			TextureHandle = other.TextureHandle;
			MaterialSpecular = other.MaterialSpecular;
			MaterialShininess = other.MaterialShininess;
		}
		TriangularPrismRendererComponent(const rtmcpp::Vec4& color)
			: Color(color) {}

		/*glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(TriangleRotation));

			return glm::translate(glm::mat4(1.0f), TriangleTranslation)
				* rotation
				* glm::scale(glm::mat4(1.0f), TriangleScale);
		}*/

		rtmcpp::Mat4 GetTransform() const
		{
			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(TriangleRotation));

			return rtmcpp::Mat4Cast(rtmcpp::Scale(TriangleScale))
				* rotation
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ TriangleTranslation.X, TriangleTranslation.Y, TriangleTranslation.Z }));

			//return rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ TriangleTranslation.X, TriangleTranslation.Y, TriangleTranslation.Z }))
			//	* rotation
			//	* rtmcpp::Mat4Cast(rtmcpp::Scale(TriangleScale));
		}
	};

	struct PlaneRendererComponent
	{
		rtmcpp::Vec4 PlaneTranslation = { 0.0f, 0.0f, 0.0f, 1.0f };
		rtmcpp::Vec3 PlaneRotation = { 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 PlaneScale = { 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec2 UV{ 1.0f, 1.0f };
		AssetHandle TextureHandle = 0;
		rtmcpp::Vec3 MaterialSpecular = rtmcpp::Vec3(0.5f, 0.5f, 0.5f);
		float MaterialShininess = 64.0f;

		PlaneRendererComponent() = default;
		//PlaneRendererComponent(const PlaneRendererComponent& other) = default;
		PlaneRendererComponent(const PlaneRendererComponent& other)
		{
			PlaneTranslation = other.PlaneTranslation;
			PlaneRotation = other.PlaneRotation;
			PlaneScale = other.PlaneScale;
			Color = other.Color;
			UV = other.UV;
			TextureHandle = other.TextureHandle;
			MaterialSpecular = other.MaterialSpecular;
			MaterialShininess = other.MaterialShininess;
		}
		PlaneRendererComponent(const rtmcpp::Vec4& color)
			: Color(color) {}

		/*glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(PlaneRotation));

			return glm::translate(glm::mat4(1.0f), PlaneTranslation)
				* rotation
				* glm::scale(glm::mat4(1.0f), PlaneScale);
		}*/

		rtmcpp::Mat4 GetTransform() const
		{
			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(PlaneRotation));

			return rtmcpp::Mat4Cast(rtmcpp::Scale(PlaneScale))
				* rotation
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ PlaneTranslation.X, PlaneTranslation.Y, PlaneTranslation.Z }));

			//return rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ PlaneTranslation.X, PlaneTranslation.Y, PlaneTranslation.Z }))
			//	* rotation
			//	* rtmcpp::Mat4Cast(rtmcpp::Scale(PlaneScale));
		}
	};

	struct OBJRendererComponent
	{
		rtmcpp::Vec4 ObjTranslation = { 0.0f, 0.0f, 0.0f, 1.0f };
		rtmcpp::Vec3 ObjRotation = { 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 ObjScale = { 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec2 UV{ 1.0f, 1.0f };
		AssetHandle ModelHandle = 0;

		OBJRendererComponent() = default;
		//OBJRendererComponent(const OBJRendererComponent& other) = default;
		OBJRendererComponent(const OBJRendererComponent& other)
		{
			ObjTranslation = other.ObjTranslation;
			ObjRotation = other.ObjRotation;
			ObjScale = other.ObjScale;
			Color = other.Color;
			UV = other.UV;
			ModelHandle = other.ModelHandle;
		}
		OBJRendererComponent(const rtmcpp::Vec4& color)
			: Color(color) {}

		/*glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(ObjRotation));

			return glm::translate(glm::mat4(1.0f), ObjTranslation)
				* rotation
				* glm::scale(glm::mat4(1.0f), ObjScale);
		}*/

		rtmcpp::Mat4 GetTransform() const
		{
			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(ObjRotation));

			return rtmcpp::Mat4Cast(rtmcpp::Scale(ObjScale))
				* rotation
				* rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ ObjTranslation.X, ObjTranslation.Y, ObjTranslation.Z }));

			//return rtmcpp::Mat4Cast(rtmcpp::Translation(rtmcpp::Vec3{ ObjTranslation.X, ObjTranslation.Y, ObjTranslation.Z }))
			//	* rotation
			//	* rtmcpp::Mat4Cast(rtmcpp::Scale(ObjScale));
		}
	};

	struct ButtonWidgetComponent
	{
		rtmcpp::Vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec2 UV{ 1.0f, 1.0f };
		float Radius = 0.0f;
		rtmcpp::Vec2 Dimensions{ 1.0f, 1.0f };
		bool InvertCorners = false;
		AssetHandle TextureHandle = 0;
		bool UseLinear = false;

		ButtonWidgetComponent() = default;
		//ButtonWidgetComponent(const ButtonWidgetComponent& other) = default;
		ButtonWidgetComponent(const ButtonWidgetComponent& other)
		{
			Color = other.Color;
			UV = other.UV;
			Radius = other.Radius;
			Dimensions = other.Dimensions;
			InvertCorners = other.InvertCorners;
			TextureHandle = other.TextureHandle;
			UseLinear = other.UseLinear;
		}
		ButtonWidgetComponent(const rtmcpp::Vec4& color)
			: Color(color) {}
	};

	struct CircleWidgetComponent
	{
		rtmcpp::Vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		rtmcpp::Vec2 UV{ 1.0f, 1.0f };
		float Thickness = FLT_MAX;
		float Fade = 0.005f;
		float Radius = 1.0f;
		AssetHandle TextureHandle = 0;
		bool UseLinear = false;

		CircleWidgetComponent() = default;
		//CircleWidgetComponent(const CircleWidgetComponent& other) = default;
		CircleWidgetComponent(const CircleWidgetComponent& other)
		{
			Color = other.Color;
			UV = other.UV;
			Thickness = other.Thickness;
			Fade = other.Fade;
			Radius = other.Radius;
			TextureHandle = other.TextureHandle;
			UseLinear = other.UseLinear;
		}
		CircleWidgetComponent(const rtmcpp::Vec4& color)
			: Color(color) {}
	};

	struct CameraComponent
	{
		RefPtr<SceneCamera> Camera;
		bool Primary = true; // TODO: think about moving to Scene
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		//CameraComponent(const CameraComponent& other) = default;
		CameraComponent(const CameraComponent& other)
		{
			Camera = other.Camera;
			Primary = other.Primary;
			FixedAspectRatio = other.FixedAspectRatio;
		}

		operator SceneCamera& () { return *Camera.Raw(); }
		operator const SceneCamera& () const { return *Camera.Raw(); }
	};
	
	struct ScriptComponent
	{
		AssetHandle ScriptHandle = 0;
		CSharpObject Instance;
		std::vector<uint32_t> FieldIDs;
		bool HasInitializedScript = false;

		// NOTE: Gets set to true when OnCreate has been called for this entity
		bool IsRuntimeInitialized = false;

		ScriptComponent() = default;
		//ScriptComponent(const ScriptComponent& other) = default;
		ScriptComponent(const ScriptComponent& other)
		{
			ScriptHandle = other.ScriptHandle;
			Instance = other.Instance;
			FieldIDs = other.FieldIDs;
			HasInitializedScript = other.HasInitializedScript;
			IsRuntimeInitialized = other.IsRuntimeInitialized;
		}
	};

	struct Rigidbody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type;
		bool FixedRotation = false;
		bool SetEnabled = true;

		// Storage for runtime
		void* RuntimeBody = nullptr;

		Rigidbody2DComponent() = default;
		//Rigidbody2DComponent(const Rigidbody2DComponent& other) = default;
		Rigidbody2DComponent(const Rigidbody2DComponent& other)
		{
			Type = other.Type;
			FixedRotation = other.FixedRotation;
			SetEnabled = other.SetEnabled;
			RuntimeBody = other.RuntimeBody;
		}
	};

	struct BoxCollider2DComponent
	{
		rtmcpp::Vec2 CollisionRay = { 0.0f, 0.0f };
		rtmcpp::Vec2 Offset = { 0.0f, 0.0f };
		rtmcpp::Vec2 BoxOffset = { 0.0f, 0.0f };
		rtmcpp::Vec2 Size = { 0.5f, 0.5f };
		rtmcpp::Vec2 BoxScaleSize = { 0.5f, 0.5f };
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;
		bool Awake = true;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		BoxCollider2DComponent() = default;
		//BoxCollider2DComponent(const BoxCollider2DComponent& other) = default;
		BoxCollider2DComponent(const BoxCollider2DComponent& other)
		{
			CollisionRay = other.CollisionRay;
			Offset = other.Offset;
			BoxOffset = other.BoxOffset;
			Size = other.Size;
			BoxScaleSize = other.BoxScaleSize;
			Density = other.Density;
			Friction = other.Friction;
			Restitution = other.Restitution;
			RestitutionThreshold = other.RestitutionThreshold;
			RuntimeFixture = other.RuntimeFixture;
		}
	};

	struct CircleCollider2DComponent
	{
		rtmcpp::Vec2 CollisionRay = { 0.0f, 0.0f };
		rtmcpp::Vec2 Offset = { 0.0f, 0.0f };
		float Radius = 0.5f;
		float CircleScaleRadius = 0.5f;
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;
		bool Awake = true;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		CircleCollider2DComponent() = default;
		//CircleCollider2DComponent(const CircleCollider2DComponent& other) = default;
		CircleCollider2DComponent(const CircleCollider2DComponent& other)
		{
			CollisionRay = other.CollisionRay;
			Offset = other.Offset;
			Radius = other.Radius;
			CircleScaleRadius = other.CircleScaleRadius;
			Density = other.Density;
			Friction = other.Friction;
			Restitution = other.Restitution;
			RestitutionThreshold = other.RestitutionThreshold;
			RuntimeFixture = other.RuntimeFixture;
		}
	};

	struct TriangleCollider2DComponent
	{
		rtmcpp::Vec2 CollisionRay = { 0.0f, 0.0f };
		rtmcpp::Vec2 Offset = { 0.0f, 0.0f };
		rtmcpp::Vec2 TriangleOffset = { 0.0f, 0.0f };
		rtmcpp::Vec2 Size = { 0.5f, 0.5f };
		rtmcpp::Vec2 TriangleScaleSize = { 0.5f, 0.5f };
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;
		bool Awake = true;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		TriangleCollider2DComponent() = default;
		//TriangleCollider2DComponent(const TriangleCollider2DComponent& other) = default;
		TriangleCollider2DComponent(const TriangleCollider2DComponent& other)
		{
			CollisionRay = other.CollisionRay;
			Offset = other.Offset;
			TriangleOffset = other.TriangleOffset;
			Size = other.Size;
			TriangleScaleSize = other.TriangleScaleSize;
			Density = other.Density;
			Friction = other.Friction;
			Restitution = other.Restitution;
			RestitutionThreshold = other.RestitutionThreshold;
			RuntimeFixture = other.RuntimeFixture;
		}
	};

	struct CapsuleCollider2DComponent
	{
		rtmcpp::Vec2 CollisionRay = { 0.0f, 0.0f };
		rtmcpp::Vec2 Offset = { 0.0f, 0.0f };
		rtmcpp::Vec2 CapsuleOffset = { 0.0f, 0.0f };
		rtmcpp::Vec2 Size = { 0.5f, 0.5f };
		rtmcpp::Vec2 CapsuleScaleSize = { 0.5f, 0.5f };
		float Rotation = 0.0f;
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;
		bool Awake = true;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		CapsuleCollider2DComponent() = default;
		//CapsuleCollider2DComponent(const CapsuleCollider2DComponent& other) = default;
		CapsuleCollider2DComponent(const CapsuleCollider2DComponent& other)
		{
			CollisionRay = other.CollisionRay;
			Offset = other.Offset;
			CapsuleOffset = other.CapsuleOffset;
			Size = other.Size;
			CapsuleScaleSize = other.CapsuleScaleSize;
			Density = other.Density;
			Friction = other.Friction;
			Restitution = other.Restitution;
			RestitutionThreshold = other.RestitutionThreshold;
			RuntimeFixture = other.RuntimeFixture;
		}
	};

	struct MeshCollider2DComponent
	{
		rtmcpp::Vec2 CollisionRay = { 0.0f, 0.0f };
		rtmcpp::Vec2 Offset = { 0.0f, 0.0f };
		rtmcpp::Vec2 MeshOffset = { 0.0f, 0.0f };
		rtmcpp::Vec2 Size = { 0.5f, 0.5f };
		rtmcpp::Vec2 MeshScaleSize = { 0.5f, 0.5f };
		float Rotation = 0.0f;
		std::vector<rtmcpp::Vec2> Positions;
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;
		bool Awake = true;
		uint32_t NumberOfPositions = 0;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		MeshCollider2DComponent() = default;
		//MeshCollider2DComponent(const MeshCollider2DComponent& other) = default;
		MeshCollider2DComponent(const MeshCollider2DComponent& other)
		{
			CollisionRay = other.CollisionRay;
			Offset = other.Offset;
			MeshOffset = other.MeshOffset;
			Size = other.Size;
			MeshScaleSize = other.MeshScaleSize;
			Rotation = other.Rotation;
			Positions = other.Positions;
			Density = other.Density;
			Friction = other.Friction;
			Restitution = other.Restitution;
			RestitutionThreshold = other.RestitutionThreshold;
			NumberOfPositions = other.NumberOfPositions;
			RuntimeFixture = other.RuntimeFixture;
		}

		const rtmcpp::Vec2& GetPosition(uint32_t index) const { return Positions[index]; }

		void AddPosition(const rtmcpp::Vec2& position)
		{
			Positions.emplace_back(position);
			NumberOfPositions = (uint32_t)Positions.size();
		}

		void RemoveAllPositions()
		{
			Positions.erase(Positions.begin(), Positions.end());
			Positions.shrink_to_fit();
		}

		void RemovePosition(uint32_t index)
		{
			Positions.erase(Positions.begin() + index);
			Positions.shrink_to_fit();

			NumberOfPositions = (uint32_t)Positions.size();
		}
	};

	struct AudioSourceComponent
	{
		AudioSourceConfig Config;

		AssetHandle Audio = 0;
		AudioData AudioSourceData;
		
		bool Paused = false;
		bool Seek = false;
		uint64_t SeekPosition = 0;

		RefPtr<AudioSource> GetAudioSource(uint32_t index) const { return AssetManager::GetAsset<AudioSource>(AudioSourceData.Playlist[index]); }
		void SetAudioSource(uint32_t index) { Audio = AudioSourceData.Playlist[index]; }

		void AddAudioSource(AssetHandle& audio)
		{
			AudioSourceData.Playlist.emplace_back(audio);
			AudioSourceData.NumberOfAudioSources = (uint32_t)AudioSourceData.Playlist.size();
		}

		void RemoveAudioSource(uint32_t index)
		{
			AudioSourceData.Playlist.erase(AudioSourceData.Playlist.begin() + index);
			AudioSourceData.Playlist.shrink_to_fit();
			AudioSourceData.NumberOfAudioSources = (uint32_t)AudioSourceData.Playlist.size();
		}

		void RemoveAudioSource(AssetHandle& audio)
		{
			uint32_t index = 0;

			for (uint32_t i = 0; i < AudioSourceData.Playlist.size(); i++)
			{
				AssetHandle audioSource = AudioSourceData.Playlist[i];

				if (audioSource == audio)
				{
					index = i;
				}
			}

			AudioSourceData.Playlist.erase(AudioSourceData.Playlist.begin() + index);
			AudioSourceData.Playlist.shrink_to_fit();
			AudioSourceData.NumberOfAudioSources = (uint32_t)AudioSourceData.Playlist.size();
		}
	};

	struct AudioListenerComponent
	{
		bool Active = true;
		AudioListenerConfig Config;

		RefPtr<AudioListener> Listener;
	};

	struct VideoRendererComponent
	{
		rtmcpp::Vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float Saturation = 1.0f;
		AssetHandle Video = 0;
		VideoData m_VideoData;

		RefPtr<VideoTexture> Texture = nullptr;

		VideoRendererComponent() = default;
		//VideoRendererComponent(const VideoRendererComponent& other) = default;
		VideoRendererComponent(const VideoRendererComponent& other)
		{
			Color = other.Color;
			Saturation = other.Saturation;
			Video = other.Video;
			m_VideoData = other.m_VideoData;

			Texture = other.Texture;
		}
		VideoRendererComponent(const rtmcpp::Vec4& color)
			: Color(color) {}
	};

	template<typename... Component>
	struct ComponentGroup
	{
	};

	using AllComponents = ComponentGroup
	<
		TransformComponent,
		SpriteRendererComponent,
		CircleRendererComponent,
		TriangleRendererComponent,
		LineRendererComponent,
		TextComponent,
		ParticleSystemComponent,
		CubeRendererComponent,
		PyramidRendererComponent,
		TriangularPrismRendererComponent,
		PlaneRendererComponent,
		OBJRendererComponent,
		ButtonWidgetComponent,
		CircleWidgetComponent,
		CameraComponent,
		Rigidbody2DComponent,
		BoxCollider2DComponent,
		CircleCollider2DComponent,
		TriangleCollider2DComponent,
		CapsuleCollider2DComponent,
		MeshCollider2DComponent,
		ScriptComponent,
		AudioSourceComponent,
		AudioListenerComponent,
		VideoRendererComponent
	>;
}
