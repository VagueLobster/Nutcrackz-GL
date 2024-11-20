#pragma once

#include "RenderCommand.hpp"

#include "Camera.hpp"
#include "Shader.hpp"

namespace Nutcrackz {

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);

		//static void BeginScene(const OrthographicCamera& camera);
		//static void EndScene();

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
		static void SetAPI(RendererAPI::API api) { RendererAPI::SetAPI(api); }

	/*private:
		struct SceneData
		{
			rtmcpp::Mat4 ViewProjectionMatrix;
		};

		static Scope<SceneData> s_SceneData;*/
	};

}
