#pragma once

#include "Nutcrackz/Scene/Components.hpp"

#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	class VideoRenderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, const rtmcpp::Mat4& transform);
		static void BeginScene(const EditorCamera& camera);

		static void EndScene();
		static void Flush();

		static void DrawVideoSprite(TransformComponent& transform, VideoRendererComponent& src, Timestep ts, int entityID = -1);

		static void ResetPacketDuration(VideoRendererComponent& src);

	private:
		static void StartBatch();
		static void NextBatch();

		static void RenderVideo(TransformComponent& transform, VideoRendererComponent& src, Timestep ts, int entityID);
		static void RenderFrame(TransformComponent& transform, VideoRendererComponent& src, int entityID);
	};

};
