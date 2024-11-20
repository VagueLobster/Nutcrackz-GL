#include "nzpch.hpp"
#include "VideoRenderer.hpp"

#include "Nutcrackz/Core/Application.hpp"
#include "Nutcrackz/Video/VideoTexture.hpp"

#include "Nutcrackz/Project/Project.hpp"

#include "VertexArray.hpp"
#include "Shader.hpp"
#include "RenderCommand.hpp"
#include "UniformBuffer.hpp"

#include "rtmcpp/PackedVector.hpp"

#include <timeapi.h>
#include <thread>
#include <chrono>
#include <future>

namespace Nutcrackz {

	struct VideoVertex
	{
		rtmcpp::PackedVec4 Position;
		rtmcpp::PackedVec4 Color;
		rtmcpp::PackedVec2 TexCoord;
		rtmcpp::PackedVec2 TilingFactor;
		int TexIndex;
		float Saturation;

		// Editor-only
		int EntityID;
	};

	struct VideoRendererData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		RefPtr<VertexArray> VideoVertexArray;
		RefPtr<VertexBuffer> VideoVertexBuffer;
		RefPtr<Shader> VideoShader;

		uint32_t VideoIndexCount = 0;
		VideoVertex* VideoVertexBufferBase = nullptr;
		VideoVertex* VideoVertexBufferPtr = nullptr;

		std::array<RefPtr<VideoTexture>, MaxTextureSlots> VideoTextureSlots;

		RefPtr<VideoTexture> WhiteVideoTexture;
		uint32_t VideoTextureSlotIndex = 1; // 0 = white texture

		rtmcpp::Vec4 QuadVertexPositions[4];

		struct CameraData
		{
			rtmcpp::Mat4 ViewProjection;
		};

		rtmcpp::Mat4 m_CameraView;
		CameraData CameraBuffer;
		RefPtr<UniformBuffer> CameraUniformBuffer;

		bool UseBillboard = false;
		bool IsPlayingVideo = false;
	};

	static VideoRendererData s_VideoData;

	void VideoRenderer::Init()
	{
		s_VideoData.VideoVertexArray = VertexArray::Create();

		s_VideoData.VideoVertexBuffer = VertexBuffer::Create(s_VideoData.MaxVertices * sizeof(VideoVertex));
		s_VideoData.VideoVertexBuffer->SetLayout({
			{ ShaderDataType::Float4, "a_Position"     },
			{ ShaderDataType::Float4, "a_Color"        },
			{ ShaderDataType::Float2, "a_TexCoord"     },
			{ ShaderDataType::Float2, "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_TexIndex"     },
			{ ShaderDataType::Float,  "a_Saturation"   },
			{ ShaderDataType::Int,    "a_EntityID"     }
		});
		s_VideoData.VideoVertexArray->AddVertexBuffer(s_VideoData.VideoVertexBuffer);

		s_VideoData.VideoVertexBufferBase = new VideoVertex[s_VideoData.MaxVertices];

		uint32_t* quadIndices = new uint32_t[s_VideoData.MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_VideoData.MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		RefPtr<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_VideoData.MaxIndices);
		s_VideoData.VideoVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		s_VideoData.WhiteVideoTexture = VideoTexture::Create(TextureSpecification());
		uint32_t whiteVideoTextureData = 0xffffffff;
		s_VideoData.WhiteVideoTexture->SetData(Buffer(&whiteVideoTextureData, sizeof(uint32_t)));

		//s_VideoData.VideoShader = Shader::Create("assets/shaders/Renderer2D_Quad.glsl");
		s_VideoData.VideoShader = Shader::Create("assets/shaders/Renderer2D_Video_BlackWhite.glsl");

		// Set first texture slot to 0
		s_VideoData.VideoTextureSlots[0] = s_VideoData.WhiteVideoTexture;

		s_VideoData.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_VideoData.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		s_VideoData.QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
		s_VideoData.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

		s_VideoData.CameraUniformBuffer = UniformBuffer::Create(sizeof(VideoRendererData::CameraData), 0);
	}

	void VideoRenderer::Shutdown()
	{
		//NZ_PROFILE_FUNCTION();

		delete[] s_VideoData.VideoVertexBufferBase;
	}

	void VideoRenderer::BeginScene(const Camera& camera, const rtmcpp::Mat4& transform)
	{
		//NZ_PROFILE_FUNCTION();

		s_VideoData.m_CameraView = rtmcpp::Inverse(transform); // glm::inverse(transform);
		s_VideoData.CameraBuffer.ViewProjection = rtmcpp::Inverse(transform) * camera.GetProjection();
		s_VideoData.CameraUniformBuffer->SetData(&s_VideoData.CameraBuffer, sizeof(VideoRendererData::CameraData));

		StartBatch();
	}

	void VideoRenderer::BeginScene(const EditorCamera& camera)
	{
		//NZ_PROFILE_FUNCTION();

		s_VideoData.m_CameraView = camera.GetViewMatrix();
		s_VideoData.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_VideoData.CameraUniformBuffer->SetData(&s_VideoData.CameraBuffer, sizeof(VideoRendererData::CameraData));

		StartBatch();
	}

	void VideoRenderer::EndScene()
	{
		//NZ_PROFILE_FUNCTION();

		Flush();
	}

	void VideoRenderer::Flush()
	{
		if (s_VideoData.VideoIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_VideoData.VideoVertexBufferPtr - (uint8_t*)s_VideoData.VideoVertexBufferBase);
			s_VideoData.VideoVertexBuffer->SetData(s_VideoData.VideoVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_VideoData.VideoTextureSlotIndex; i++)
			{
				if (s_VideoData.VideoTextureSlots[i])
					s_VideoData.VideoTextureSlots[i]->Bind(i, s_VideoData.IsPlayingVideo);
			}

			s_VideoData.VideoShader->Bind();
			RenderCommand::DrawIndexed(s_VideoData.VideoVertexArray, s_VideoData.VideoIndexCount, s_VideoData.UseBillboard);
		}
	}

	void VideoRenderer::StartBatch()
	{
		s_VideoData.VideoIndexCount = 0;
		s_VideoData.VideoVertexBufferPtr = s_VideoData.VideoVertexBufferBase;

		s_VideoData.VideoTextureSlotIndex = 1;
	}

	void VideoRenderer::NextBatch()
	{
		Flush();
		StartBatch();
	}

#pragma region FromGLFW

	// Because of the way ImGui uses GLFW's SetTime(),
	// i couldn't use it in here! I had to copy the entire Timer structure from GLFW!
	struct VideoTimer
	{
		uint64_t Offset;

		struct VideoTimerWin32
		{
			bool HasPC;
			uint64_t Frequency;

		} win32;

	} timer;

	void InitTimerWin32()
	{
		uint64_t frequency;

		if (QueryPerformanceFrequency((LARGE_INTEGER*)&frequency))
		{
			timer.win32.HasPC = true;
			timer.win32.Frequency = frequency;
		}
		else
		{
			timer.win32.HasPC = false;
			timer.win32.Frequency = 1000;
		}
	}

	uint64_t PlatformGetTimerValue()
	{
		if (timer.win32.HasPC)
		{
			uint64_t value;
			QueryPerformanceCounter((LARGE_INTEGER*)&value);
			return value;
		}
		else
			return (uint64_t)timeGetTime();
	}

	uint64_t PlatformGetTimerFrequency()
	{
		return timer.win32.Frequency;
	}

	double GetTime()
	{
		return (double)(PlatformGetTimerValue() - timer.Offset) / PlatformGetTimerFrequency();
	}

	void SetTime(double time)
	{
		if (time != time || time < 0.0 || time > 18446744073.0)
		{
			NZ_CORE_TRACE("Invalid time: {0}", time);
			return;
		}

		timer.Offset = PlatformGetTimerValue() - (uint64_t)(time * PlatformGetTimerFrequency());
	}

#pragma endregion

	void VideoRenderer::RenderVideo(TransformComponent& transform, VideoRendererComponent& src/*, VideoData& data*/, int entityID)
	{
		if (src.Video != 0)
		{
			if (src.Texture == nullptr || src.Texture != AssetManager::GetAsset<VideoTexture>(src.Video))
				src.Texture = AssetManager::GetAsset<VideoTexture>(src.Video);
		}

		//if (src.Video)
		//{
		//	NZ_CORE_VERIFY(src.Texture);
		//}

		if (!src.m_VideoData.HasInitializedTimer)
		{
			InitTimerWin32();
			src.m_VideoData.HasInitializedTimer = true;
		}

		constexpr size_t videoVertexCount = 4;
		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f }, };

		int textureIndex = 0;
		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		// This if statement is very much required!
		// Without it, this function creates a big memory leak!
		if (src.m_VideoData.VideoRendererID)
			src.Texture->DeleteRendererID(src.m_VideoData.VideoRendererID);

		if (src.Texture)
		{
			if (!src.m_VideoData.UseExternalAudio)
				{
					src.Texture->SetVolumeFactor(src.m_VideoData.Volume);
					src.Texture->ReadAndPlayAudio(&src.Texture->GetVideoState(), src.m_VideoData.FramePosition, src.m_VideoData.SeekAudio, src.m_VideoData.PauseVideo, Project::GetActiveAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetFilePath(src.Video));
				}

				if (!src.m_VideoData.IsRenderingVideo)
				{
					SetTime(0.0);
					src.m_VideoData.IsRenderingVideo = true;
				}

				if (src.m_VideoData.FramePosition != 0)
				{
					SetTime(src.m_VideoData.FramePosition / src.Texture->GetVideoState().Framerate);

					if (!src.Texture->VideoReaderSeekFrame(&src.Texture->GetVideoState(), src.m_VideoData.FramePosition))
					{
						NZ_CORE_WARN("Could not seek video back to start frame!");
						return;
					}

					src.m_VideoData.PresentationTimeStamp = src.m_VideoData.FramePosition;
					src.m_VideoData.FramePosition = 0;
				}

				src.m_VideoData.VideoRendererID = src.Texture->GetIDFromTexture(src.m_VideoData.VideoFrameData, &src.m_VideoData.PresentationTimeStamp, src.m_VideoData.PauseVideo, Project::GetActiveAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetFilePath(src.Video));
				src.Texture->SetRendererID(src.m_VideoData.VideoRendererID);

				if (src.m_VideoData.PauseVideo)
				{
					if (!src.m_VideoData.UseExternalAudio)
					{
						if (!src.Texture->AVReaderSeekFrame(&src.Texture->GetVideoState(), src.m_VideoData.PresentationTimeStamp))
						{
							NZ_CORE_WARN("Could not seek a/v back to start frame!");
							return;
						}
					}
					else
					{
						if (!src.Texture->VideoReaderSeekFrame(&src.Texture->GetVideoState(), src.m_VideoData.PresentationTimeStamp))
						{
							NZ_CORE_WARN("Could not seek video back to start frame!");
							return;
						}
					}

					SetTime(0.0);
				}
				else if (!src.m_VideoData.PauseVideo)
				{
					if (src.m_VideoData.RestartPointFromPause > GetTime())
						SetTime(src.m_VideoData.RestartPointFromPause);

					if (src.m_VideoData.RestartPointFromPause < GetTime())
						src.m_VideoData.RestartPointFromPause = GetTime();

					int hoursToSeconds = src.Texture->GetVideoState().Hours * 3600;
					int minutesToSeconds = src.Texture->GetVideoState().Mins * 60;

					if (src.m_VideoData.VideoDuration != hoursToSeconds + minutesToSeconds + src.Texture->GetVideoState().Secs + (0.01 * ((100 * src.Texture->GetVideoState().Us) / AV_TIME_BASE)))
						src.m_VideoData.VideoDuration = hoursToSeconds + minutesToSeconds + src.Texture->GetVideoState().Secs + (0.01 * ((100 * src.Texture->GetVideoState().Us) / AV_TIME_BASE));

					if (src.m_VideoData.Hours != src.Texture->GetVideoState().Hours)
						src.m_VideoData.Hours = src.Texture->GetVideoState().Hours;

					if (src.m_VideoData.Minutes != src.Texture->GetVideoState().Mins)
						src.m_VideoData.Minutes = src.Texture->GetVideoState().Mins;

					if (src.m_VideoData.Seconds != src.Texture->GetVideoState().Secs)
						src.m_VideoData.Seconds = src.Texture->GetVideoState().Secs;

					if (src.m_VideoData.Milliseconds != src.Texture->GetVideoState().Us)
						src.m_VideoData.Milliseconds = src.Texture->GetVideoState().Us;

					src.m_VideoData.PresentationTimeInSeconds = src.m_VideoData.PresentationTimeStamp * ((double)src.Texture->GetVideoState().TimeBase.num / (double)src.Texture->GetVideoState().TimeBase.den);

					//std::async([=]() mutable
					//{
						while (src.m_VideoData.PresentationTimeInSeconds > GetTime())
						{
							std::this_thread::sleep_for(std::chrono::seconds((long long)(src.m_VideoData.PresentationTimeInSeconds - GetTime())));
							//Sleep(data.PresentationTimeInSeconds - GetTime());
						}
					//});

					if (src.m_VideoData.RepeatVideo && GetTime() >= src.m_VideoData.VideoDuration)
					{
						if (!src.m_VideoData.UseExternalAudio)
						{
							if (!src.Texture->AVReaderSeekFrame(&src.Texture->GetVideoState(), 0, true))
							{
								NZ_CORE_WARN("Could not seek audio/video back to start frame!");
								return;
							}

							src.Texture->CloseAudio(&src.Texture->GetVideoState());
						}
						else
						{
							if (!src.Texture->VideoReaderSeekFrame(&src.Texture->GetVideoState(), 0))
							{
								NZ_CORE_WARN("Could not seek video back to start frame!");
								return;
							}
						}

						src.m_VideoData.SeekAudio = true;
						src.Texture->DeleteRendererID(src.m_VideoData.VideoRendererID);
						src.Texture->CloseVideo(&src.Texture->GetVideoState());

						std::filesystem::path filepath = Project::GetActiveAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetFilePath(src.Video);
						src.m_VideoData.VideoRendererID = src.Texture->GetIDFromTexture(src.m_VideoData.VideoFrameData, &src.m_VideoData.PresentationTimeStamp, src.m_VideoData.PauseVideo, filepath);

						src.Texture->SetRendererID(src.m_VideoData.VideoRendererID);

						src.m_VideoData.PresentationTimeStamp = 0;
						SetTime(0.0);
						src.m_VideoData.RestartPointFromPause = 0.0;
					}
				}

			if (s_VideoData.VideoIndexCount >= VideoRendererData::MaxIndices)
				NextBatch();

			for (uint32_t i = 1; i < s_VideoData.VideoTextureSlotIndex; i++)
			{
				if (*s_VideoData.VideoTextureSlots[i] == *src.Texture)
				{
					textureIndex = i;
					break;
				}
			}

			if (textureIndex == 0)
			{
				if (s_VideoData.VideoTextureSlotIndex >= VideoRendererData::MaxTextureSlots)
					NextBatch();

				textureIndex = s_VideoData.VideoTextureSlotIndex;

				if (src.Texture != nullptr)
					s_VideoData.VideoTextureSlots[s_VideoData.VideoTextureSlotIndex] = src.Texture;
				else
					s_VideoData.VideoTextureSlots[s_VideoData.VideoTextureSlotIndex] = s_VideoData.WhiteVideoTexture;

				s_VideoData.VideoTextureSlotIndex++;
			}
		}

		if (src.m_VideoData.UseBillboard)
		{
			rtmcpp::Vec4 camRightWS = { s_VideoData.m_CameraView.Value.x_axis.m128_f32[0], s_VideoData.m_CameraView.Value.y_axis.m128_f32[0], s_VideoData.m_CameraView.Value.z_axis.m128_f32[0], 1.0f };
			rtmcpp::Vec4 camUpWS = { s_VideoData.m_CameraView.Value.x_axis.m128_f32[1], s_VideoData.m_CameraView.Value.y_axis.m128_f32[1], s_VideoData.m_CameraView.Value.z_axis.m128_f32[1], 1.0f };
			rtmcpp::Vec4 position = { transform.Translation.X, transform.Translation.Y, transform.Translation.Z, 1.0f };

			for (size_t i = 0; i < videoVertexCount; i++)
			{
				//rtmcpp::Vec4 positionFinal = rtmcpp::Vec4{
				//	camRightWS.X * s_VideoData.QuadVertexPositions[i].X * transform.Scale.X + camUpWS.X * s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y + position.X,
				//	camRightWS.Y * s_VideoData.QuadVertexPositions[i].X * transform.Scale.X + camUpWS.Y * s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y + position.Y,
				//	camRightWS.Z * s_VideoData.QuadVertexPositions[i].X * transform.Scale.X + camUpWS.Z * s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y + position.Z,
				//	camRightWS.W * s_VideoData.QuadVertexPositions[i].X * transform.Scale.X + camUpWS.W * s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y + position.W
				//};

				//s_VideoData.VideoVertexBufferPtr->Position = position + camRightWS * (s_VideoData.QuadVertexPositions[i].X) * transform.Scale.x + camUpWS * s_VideoData.QuadVertexPositions[i].y * transform.Scale.y;
				//s_VideoData.VideoVertexBufferPtr->Position = rtmcpp::Vec4{ s_VideoData.QuadVertexPositions[i].X, s_VideoData.QuadVertexPositions[i].X,
				//	s_VideoData.QuadVertexPositions[i].X, s_VideoData.QuadVertexPositions[i].X } * camRightWS
				//	* rtmcpp::Vec4{ transform.Scale.X, transform.Scale.X, transform.Scale.X, transform.Scale.X }
				//	+ rtmcpp::Vec4{ s_VideoData.QuadVertexPositions[i].Y, s_VideoData.QuadVertexPositions[i].Y,
				//	s_VideoData.QuadVertexPositions[i].Y, s_VideoData.QuadVertexPositions[i].Y } * camUpWS
				//	* rtmcpp::Vec4{ transform.Scale.Y, transform.Scale.Y, transform.Scale.Y, transform.Scale.Y } + position;
				//s_VideoData.VideoVertexBufferPtr->Position = positionFinal;
				/*s_VideoData.VideoVertexBufferPtr->Position = position + rtmcpp::Vec4{
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					1.0f
				}
				* camRightWS + rtmcpp::Vec4{
					s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
					s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
					s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
					1.0f
				}
				* camUpWS;*/

				s_VideoData.VideoVertexBufferPtr->Position = camUpWS * rtmcpp::Vec4{
						s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
						s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
						s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
						1.0f
				} + camRightWS * rtmcpp::Vec4{
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					1.0f
				} + position;

				s_VideoData.VideoVertexBufferPtr->Color = src.Color;
				s_VideoData.VideoVertexBufferPtr->TexCoord = textureCoords[i];
				s_VideoData.VideoVertexBufferPtr->TilingFactor = tilingFactor;
				s_VideoData.VideoVertexBufferPtr->TexIndex = textureIndex;
				s_VideoData.VideoVertexBufferPtr->Saturation = src.Saturation;
				s_VideoData.VideoVertexBufferPtr->EntityID = entityID;
				s_VideoData.VideoVertexBufferPtr++;
			}
		}
		else
		{
			for (size_t i = 0; i < videoVertexCount; i++)
			{
				s_VideoData.VideoVertexBufferPtr->Position = s_VideoData.QuadVertexPositions[i] * transform.GetTransform();
				s_VideoData.VideoVertexBufferPtr->Color = src.Color;
				s_VideoData.VideoVertexBufferPtr->TexCoord = textureCoords[i];
				s_VideoData.VideoVertexBufferPtr->TilingFactor = tilingFactor;
				s_VideoData.VideoVertexBufferPtr->TexIndex = textureIndex;
				s_VideoData.VideoVertexBufferPtr->Saturation = src.Saturation;
				s_VideoData.VideoVertexBufferPtr->EntityID = entityID;
				s_VideoData.VideoVertexBufferPtr++;
			}
		}

		s_VideoData.VideoIndexCount += 6;
		//s_Data.Stats.QuadCount++;
	}

	void VideoRenderer::RenderStartFrame(TransformComponent& transform, VideoRendererComponent& src, VideoData& data, int entityID)
	{
		if (src.Texture == nullptr || src.Texture != AssetManager::GetAsset<VideoTexture>(src.Video))
			src.Texture = AssetManager::GetAsset<VideoTexture>(src.Video);

		//NZ_CORE_VERIFY(texture);

		constexpr size_t videoVertexCount = 4;
		int textureIndex = 0;

		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f } };

		if (src.Texture)
		{
			if (data.FramePosition != data.FramePosition * src.Texture->GetVideoState().VideoPacketDuration)
				data.FramePosition = data.FramePosition * src.Texture->GetVideoState().VideoPacketDuration;

			SetTime(0.0);

			if (data.NumberOfFrames != src.Texture->GetVideoState().NumberOfFrames)
				data.NumberOfFrames = src.Texture->GetVideoState().NumberOfFrames;

			if (data.Hours != src.Texture->GetVideoState().Hours)
				data.Hours = src.Texture->GetVideoState().Hours;

			if (data.Minutes != src.Texture->GetVideoState().Mins)
				data.Minutes = src.Texture->GetVideoState().Mins;

			if (data.Seconds != src.Texture->GetVideoState().Secs)
				data.Seconds = src.Texture->GetVideoState().Secs;

			if (data.Milliseconds != src.Texture->GetVideoState().Us)
				data.Milliseconds = src.Texture->GetVideoState().Us;

			if (s_VideoData.VideoIndexCount >= VideoRendererData::MaxIndices)
				NextBatch();

			for (uint32_t i = 1; i < s_VideoData.VideoTextureSlotIndex; i++)
			{
				if (*s_VideoData.VideoTextureSlots[i] == *src.Texture)
				{
					textureIndex = i;
					break;
				}
			}

			if (textureIndex == 0)
			{
				if (s_VideoData.VideoTextureSlotIndex >= VideoRendererData::MaxTextureSlots)
					NextBatch();

				textureIndex = s_VideoData.VideoTextureSlotIndex;
				s_VideoData.VideoTextureSlots[s_VideoData.VideoTextureSlotIndex] = src.Texture;

				s_VideoData.VideoTextureSlotIndex++;
			}
		}

		if (data.UseBillboard)
		{
			/*glm::vec3 camRightWS = {s_VideoData.m_CameraView[0][0], s_VideoData.m_CameraView[1][0], s_VideoData.m_CameraView[2][0]};
			glm::vec3 camUpWS = { s_VideoData.m_CameraView[0][1], s_VideoData.m_CameraView[1][1], s_VideoData.m_CameraView[2][1] };
			glm::vec3 position = { transform.Translation.x, transform.Translation.y, transform.Translation.z };

			for (size_t i = 0; i < videoVertexCount; i++)
			{
				s_VideoData.VideoVertexBufferPtr->Position = position + camRightWS * (s_VideoData.QuadVertexPositions[i].x) * transform.Scale.x + camUpWS * s_VideoData.QuadVertexPositions[i].y * transform.Scale.y;*/
			rtmcpp::Vec4 camRightWS = { s_VideoData.m_CameraView.Value.x_axis.m128_f32[0], s_VideoData.m_CameraView.Value.y_axis.m128_f32[0], s_VideoData.m_CameraView.Value.z_axis.m128_f32[0], 1.0f };
			rtmcpp::Vec4 camUpWS = { s_VideoData.m_CameraView.Value.x_axis.m128_f32[1], s_VideoData.m_CameraView.Value.y_axis.m128_f32[1], s_VideoData.m_CameraView.Value.z_axis.m128_f32[1], 1.0f };
			rtmcpp::Vec4 position = { transform.Translation.X, transform.Translation.Y, transform.Translation.Z, 1.0f };

			for (size_t i = 0; i < videoVertexCount; i++)
			{
				//s_VideoData.VideoVertexBufferPtr->Position = position + camRightWS * (s_VideoData.QuadVertexPositions[i].X) * transform.Scale.x + camUpWS * s_VideoData.QuadVertexPositions[i].y * transform.Scale.y;
				/*s_VideoData.VideoVertexBufferPtr->Position = rtmcpp::Vec4{s_VideoData.QuadVertexPositions[i].X, s_VideoData.QuadVertexPositions[i].X,
					s_VideoData.QuadVertexPositions[i].X, s_VideoData.QuadVertexPositions[i].X } *camRightWS
					* rtmcpp::Vec4{ transform.Scale.X, transform.Scale.X, transform.Scale.X, transform.Scale.X }
					+ rtmcpp::Vec4{ s_VideoData.QuadVertexPositions[i].Y, s_VideoData.QuadVertexPositions[i].Y,
					s_VideoData.QuadVertexPositions[i].Y, s_VideoData.QuadVertexPositions[i].Y } *camUpWS
					* rtmcpp::Vec4{ transform.Scale.Y, transform.Scale.Y, transform.Scale.Y, transform.Scale.Y } + position;*/

				s_VideoData.VideoVertexBufferPtr->Position = camUpWS * rtmcpp::Vec4{
					s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
					s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
					s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
					1.0f
				} + camRightWS * rtmcpp::Vec4{
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					1.0f
				} + position;

				/*s_VideoData.VideoVertexBufferPtr->Position = position + camRightWS
					* rtmcpp::Vec4{ s_VideoData.QuadVertexPositions[i].X, s_VideoData.QuadVertexPositions[i].X,
					s_VideoData.QuadVertexPositions[i].X, s_VideoData.QuadVertexPositions[i].X }
					* rtmcpp::Vec4{ transform.Scale.X, transform.Scale.X, transform.Scale.X, transform.Scale.X }
					+ camUpWS * rtmcpp::Vec4{ s_VideoData.QuadVertexPositions[i].Y, s_VideoData.QuadVertexPositions[i].Y,
					s_VideoData.QuadVertexPositions[i].Y, s_VideoData.QuadVertexPositions[i].Y }
					* rtmcpp::Vec4{ transform.Scale.Y, transform.Scale.Y, transform.Scale.Y, transform.Scale.Y };*/
				s_VideoData.VideoVertexBufferPtr->Color = src.Color;
				s_VideoData.VideoVertexBufferPtr->TexCoord = textureCoords[i];
				s_VideoData.VideoVertexBufferPtr->TilingFactor = tilingFactor;
				s_VideoData.VideoVertexBufferPtr->TexIndex = textureIndex;
				s_VideoData.VideoVertexBufferPtr->Saturation = src.Saturation;
				s_VideoData.VideoVertexBufferPtr->EntityID = entityID;
				s_VideoData.VideoVertexBufferPtr++;
			}
		}
		else
		{
			for (size_t i = 0; i < videoVertexCount; i++)
			{
				s_VideoData.VideoVertexBufferPtr->Position = s_VideoData.QuadVertexPositions[i] * transform.GetTransform();
				s_VideoData.VideoVertexBufferPtr->Color = src.Color;
				s_VideoData.VideoVertexBufferPtr->TexCoord = textureCoords[i];
				s_VideoData.VideoVertexBufferPtr->TilingFactor = tilingFactor;
				s_VideoData.VideoVertexBufferPtr->TexIndex = textureIndex;
				s_VideoData.VideoVertexBufferPtr->Saturation = src.Saturation;
				s_VideoData.VideoVertexBufferPtr->EntityID = entityID;
				s_VideoData.VideoVertexBufferPtr++;
			}
		}

		s_VideoData.VideoIndexCount += 6;
	}

	void VideoRenderer::SeekToFrame(TransformComponent& transform, VideoRendererComponent& src, VideoData& data, int entityID)
	{
		if (src.Texture == nullptr || src.Texture != AssetManager::GetAsset<VideoTexture>(src.Video))
			src.Texture = AssetManager::GetAsset<VideoTexture>(src.Video);

		NZ_CORE_VERIFY(src.Texture);

		constexpr size_t videoVertexCount = 4;
		int textureIndex = 0;

		const rtmcpp::Vec2 tilingFactor(1.0f, 1.0f);

		rtmcpp::Vec2 textureCoords[] = { { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f } };

		if (src.Texture)
		{
			if (data.NumberOfFrames != src.Texture->GetVideoState().NumberOfFrames)
				data.NumberOfFrames = src.Texture->GetVideoState().NumberOfFrames;

			if (data.Hours != src.Texture->GetVideoState().Hours)
				data.Hours = src.Texture->GetVideoState().Hours;

			if (data.Minutes != src.Texture->GetVideoState().Mins)
				data.Minutes = src.Texture->GetVideoState().Mins;

			if (data.Seconds != src.Texture->GetVideoState().Secs)
				data.Seconds = src.Texture->GetVideoState().Secs;

			if (data.Milliseconds != src.Texture->GetVideoState().Us)
				data.Milliseconds = src.Texture->GetVideoState().Us;

			if (data.FramePosition != data.FramePosition * src.Texture->GetVideoState().VideoPacketDuration)
			{
				if (src.Texture->HasLoadedAudio())
				{
					if (!data.UseExternalAudio)
					{
						src.Texture->CloseAudio(&src.Texture->GetVideoState());

						if (!src.Texture->AVReaderSeekFrame(&src.Texture->GetVideoState(), 0, true))
						{
							NZ_CORE_WARN("Could not seek a/v back to start frame!");
							return;
						}
					}

					data.SeekAudio = true;
					SetTime(0.0);
					data.RestartPointFromPause = 0.0;
				}

				if (data.IsRenderingVideo)
				{
					SetTime(0.0);
					data.RestartPointFromPause = 0.0;
					data.IsRenderingVideo = false;
				}

				data.FramePosition = data.FramePosition * src.Texture->GetVideoState().VideoPacketDuration;

				if (!src.Texture->VideoReaderSeekFrame(&src.Texture->GetVideoState(), data.FramePosition))
				{
					NZ_CORE_WARN("Could not seek video back to start frame!");
					return;
				}

				data.PresentationTimeStamp = data.FramePosition;
				src.Texture->DeleteRendererID(data.VideoRendererID);

				std::filesystem::path filepath = Project::GetActiveAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetFilePath(src.Video);
				data.VideoRendererID = src.Texture->GetIDFromTexture(data.VideoFrameData, &data.PresentationTimeStamp, data.PauseVideo, filepath);

				src.Texture->SetRendererID(data.VideoRendererID);
			}

			if (s_VideoData.VideoIndexCount >= VideoRendererData::MaxIndices)
				NextBatch();

			for (uint32_t i = 1; i < s_VideoData.VideoTextureSlotIndex; i++)
			{
				if (*s_VideoData.VideoTextureSlots[i] == *src.Texture)
				{
					textureIndex = i;
					break;
				}
			}

			if (textureIndex == 0)
			{
				if (s_VideoData.VideoTextureSlotIndex >= VideoRendererData::MaxTextureSlots)
					NextBatch();

				textureIndex = s_VideoData.VideoTextureSlotIndex;
				s_VideoData.VideoTextureSlots[s_VideoData.VideoTextureSlotIndex] = src.Texture;

				s_VideoData.VideoTextureSlotIndex++;
			}
		}

		if (data.UseBillboard)
		{
			/*glm::vec3 camRightWS = {s_VideoData.m_CameraView[0][0], s_VideoData.m_CameraView[1][0], s_VideoData.m_CameraView[2][0]};
			glm::vec3 camUpWS = { s_VideoData.m_CameraView[0][1], s_VideoData.m_CameraView[1][1], s_VideoData.m_CameraView[2][1] };
			glm::vec3 position = { transform.Translation.x, transform.Translation.y, transform.Translation.z };

			for (size_t i = 0; i < videoVertexCount; i++)
			{
				s_VideoData.VideoVertexBufferPtr->Position = position + camRightWS * (s_VideoData.QuadVertexPositions[i].x) * transform.Scale.x + camUpWS * s_VideoData.QuadVertexPositions[i].y * transform.Scale.y;*/

			rtmcpp::Vec4 camRightWS = { s_VideoData.m_CameraView.Value.x_axis.m128_f32[0], s_VideoData.m_CameraView.Value.y_axis.m128_f32[0], s_VideoData.m_CameraView.Value.z_axis.m128_f32[0] };
			rtmcpp::Vec4 camUpWS = { s_VideoData.m_CameraView.Value.x_axis.m128_f32[1], s_VideoData.m_CameraView.Value.y_axis.m128_f32[1], s_VideoData.m_CameraView.Value.z_axis.m128_f32[1] };
			rtmcpp::Vec4 position = { transform.Translation.X, transform.Translation.Y, transform.Translation.Z, 1.0f };

			for (size_t i = 0; i < videoVertexCount; i++)
			{
				s_VideoData.VideoVertexBufferPtr->Position = position + rtmcpp::Vec4{
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					s_VideoData.QuadVertexPositions[i].X * transform.Scale.X,
					1.0f
				}
					*camRightWS + rtmcpp::Vec4{
						s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
						s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
						s_VideoData.QuadVertexPositions[i].Y * transform.Scale.Y,
						1.0f
				}
				*camUpWS;
				s_VideoData.VideoVertexBufferPtr->Position.W = 1.0f;
				s_VideoData.VideoVertexBufferPtr->Color = src.Color;
				s_VideoData.VideoVertexBufferPtr->TexCoord = textureCoords[i];
				s_VideoData.VideoVertexBufferPtr->TilingFactor = tilingFactor;
				s_VideoData.VideoVertexBufferPtr->TexIndex = textureIndex;
				s_VideoData.VideoVertexBufferPtr->Saturation = src.Saturation;
				s_VideoData.VideoVertexBufferPtr->EntityID = entityID;
				s_VideoData.VideoVertexBufferPtr++;
			}
		}
		else
		{
			for (size_t i = 0; i < videoVertexCount; i++)
			{
				s_VideoData.VideoVertexBufferPtr->Position = s_VideoData.QuadVertexPositions[i] * transform.GetTransform();
				s_VideoData.VideoVertexBufferPtr->Color = src.Color;
				s_VideoData.VideoVertexBufferPtr->TexCoord = textureCoords[i];
				s_VideoData.VideoVertexBufferPtr->TilingFactor = tilingFactor;
				s_VideoData.VideoVertexBufferPtr->TexIndex = textureIndex;
				s_VideoData.VideoVertexBufferPtr->Saturation = src.Saturation;
				s_VideoData.VideoVertexBufferPtr->EntityID = entityID;
				s_VideoData.VideoVertexBufferPtr++;
			}
		}

		s_VideoData.VideoIndexCount += 6;
	}

	void VideoRenderer::DrawVideoSprite(TransformComponent& transform, VideoRendererComponent& src, VideoData& data, int entityID)
	{
		//NZ_CORE_CRITICAL("Filepath: {}", Project::GetActiveAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetFilePath(src.Video));

		s_VideoData.IsPlayingVideo = data.PlayVideo;
		if (data.PlayVideo)
		{
			//m_Transform = transform;
			//m_Source = src;
			//m_EntityID = entityID;

			//TransformComponent m_Transform = transform;
			//VideoRendererComponent m_Source = src;
			//int m_EntityID = entityID;

			//std::async([=]() mutable
			//{
			//	RenderVideo(m_Transform, m_Source, m_EntityID);
			//});

			//transform = m_Transform;
			//src = m_Source;

			RenderVideo(transform, src/*, data*/, entityID);
		}
		else if (!data.PlayVideo && data.FramePosition == 0)
		{
			RenderStartFrame(transform, src, data, entityID);
		}
		else if (!data.PlayVideo && data.FramePosition != 0)
		{
			SeekToFrame(transform, src, data, entityID);
		}
	}

	void VideoRenderer::ResetPacketDuration(VideoRendererComponent& src)
	{
		if (src.Texture == nullptr || src.Texture != AssetManager::GetAsset<VideoTexture>(src.Video))
			src.Texture = AssetManager::GetAsset<VideoTexture>(src.Video);

		NZ_CORE_VERIFY(src.Texture);

		src.Texture->ResetAudioPacketDuration(&src.Texture->GetVideoState());
	}

}
