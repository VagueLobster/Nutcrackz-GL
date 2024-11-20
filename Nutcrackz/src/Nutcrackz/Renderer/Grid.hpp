#pragma once

#include "Nutcrackz/Renderer/Camera.hpp"
#include "Nutcrackz/Renderer/EditorCamera.hpp"

#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	class Grid
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, const rtmcpp::Mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void EndScene();
		static void Flush();

		static void DrawGrid();
		static void DrawGrid(float posX, float posY, float posZ);
		static void DrawGrid(const rtmcpp::Mat4& transform, int entityID = -2);

	private:
		static void StartBatch();
		static void NextBatch();

	//private:
	//	static rtmcpp::Vec3 m_NearPoint;
	//	static rtmcpp::Vec3 m_FarPoint;
	};

}
