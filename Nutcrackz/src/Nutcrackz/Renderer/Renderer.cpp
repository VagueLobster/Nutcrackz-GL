#include "nzpch.hpp"
#include "Renderer.hpp"
#include "Grid.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"
#include "VideoRenderer.hpp"
#include "ParticleSystem.hpp"

#include "Nutcrackz/Core/Audio/AudioEngine.hpp"

namespace Nutcrackz {

	//Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();

	void Renderer::Init()
	{
		//NZ_PROFILE_FUNCTION();
		
		AudioEngine::Init();

		RenderCommand::Init();
		Grid::Init();
		ParticleSystem::Init();
		Renderer2D::Init();
		Renderer3D::Init();
		VideoRenderer::Init();
	}

	void Renderer::Shutdown()
	{
		VideoRenderer::Shutdown();
		Renderer3D::Shutdown();
		Renderer2D::Shutdown();
		ParticleSystem::Shutdown();
		Grid::Shutdown();

		AudioEngine::Shutdown();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	/*void Renderer::BeginScene(const OrthographicCamera& camera)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetProjectionMatrix();
	}

	void Renderer::EndScene()
	{
	}*/

}