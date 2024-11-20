#pragma once

#include "Nutcrackz/Renderer/Texture.hpp"
#include "Nutcrackz/Renderer/Meshes/ObjModel.hpp"

#include "Nutcrackz/Renderer/Camera.hpp"
#include "Nutcrackz/Renderer/EditorCamera.hpp"

#include "Nutcrackz/Scene/Components.hpp"

#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	class Renderer3D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, const rtmcpp::Mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void EndScene();
		static void Flush();

		// Pyramid
		static void DrawPyramid(const rtmcpp::Mat4& transform, PyramidRendererComponent& src, const rtmcpp::Vec4& color, int entityID, bool spotLightExists);
		static void DrawPyramid(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, PyramidRendererComponent& src, int entityID, bool spotLightExists);

		static void DrawPyramidMesh(const rtmcpp::Mat4& transform, PyramidRendererComponent& src, int entityID, bool spotLightExists);

		// Triangular Prism
		static void DrawTriangularPrism(const rtmcpp::Mat4& transform, TriangularPrismRendererComponent& src, const rtmcpp::Vec4& color, int entityID, bool spotLightExists);
		static void DrawTriangularPrism(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, TriangularPrismRendererComponent& src, int entityID, bool spotLightExists);

		static void DrawTriangularPrismMesh(const rtmcpp::Mat4& transform, TriangularPrismRendererComponent& src, int entityID, bool spotLightExists);

		// 3D Cube
		static void DrawCube(const rtmcpp::Mat4& transform, CubeRendererComponent& src, const rtmcpp::Vec4& color, int entityID, bool spotLightExists);
		static void DrawCube(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, CubeRendererComponent& src, const rtmcpp::Vec2& parallaxScrolling, const Timestep& ts, const rtmcpp::Mat4& cameraProjection, int entityID, bool spotLightExists);

		static void DrawAnimatedCube(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, CubeRendererComponent& src, const Timestep& ts, int entityID, bool spotLightExists);

		static void DrawCubeMesh(const rtmcpp::Mat4& transform, CubeRendererComponent& src, const Timestep& ts, const rtmcpp::Mat4& cameraProjection, int entityID, bool IsRuntime, bool spotLightExists);

		// Plane
		static void DrawPlane(const rtmcpp::Mat4& transform, PlaneRendererComponent& src, const rtmcpp::Vec4& color, int entityID, bool spotLightExists);
		static void DrawPlane(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, PlaneRendererComponent& src, int entityID, bool spotLightExists);

		static void DrawPlaneMesh(const rtmcpp::Mat4& transform, PlaneRendererComponent& src, int entityID, bool spotLightExists);

		// Pyramid
		static void DrawOBJ(const rtmcpp::Mat4& transform, RefPtr<ObjModel>& model, OBJRendererComponent& src, const rtmcpp::Vec4& color, int entityID, bool spotLightExists);
		static void DrawOBJ(const rtmcpp::Mat4& transform, RefPtr<ObjModel>& model, const RefPtr<Texture2D>& texture, OBJRendererComponent& src, int entityID, bool spotLightExists);

		static void DrawOBJMesh(const rtmcpp::Mat4& transform, OBJRendererComponent& src, int entityID, bool spotLightExists);

		static uint32_t GetMeshVerticesSize();
		static uint32_t GetMeshIndicesSize();

		// Update Animations
		static void UpdateAnimation(const int& numTiles, const int& startIndexX, const int& startIndexY, const float& animSpeed, float& animationTime, bool useAnimation, bool useCameraParallaxY);
		static void UpdateTextureAnimation(const int& numTextures, const int& startIndex, const float& animSpeed, float& animationTime, bool useAnimation);

		// Stats
		struct Statistics3D
		{
			uint32_t DrawCalls = 0;
			uint32_t CubeCount = 0;
			uint32_t PyramidCount = 0;
			uint32_t TriangularPrismCount = 0;
			uint32_t MeshCount = 0;

			uint32_t GetTotalVertexCount() const { return CubeCount * 24; }
			uint32_t GetTotalIndexCount() const { return CubeCount * 36; }

			uint32_t GetTotalPyramidVertexCount() const { return PyramidCount * 18; }
			uint32_t GetTotalPyramidIndexCount() const { return PyramidCount * 18; }

			uint32_t GetTotalTriangularPrismVertexCount() const { return TriangularPrismCount * 12; }
			uint32_t GetTotalTriangularPrismIndexCount() const { return TriangularPrismCount * 12; }
		};

		static void Reset3DStats();
		static Statistics3D Get3DStats();

	public:
		static bool s_AnimateInRuntime;
		static bool s_AnimateInEdit;

	private:
		static void StartBatch();
		static void NextBatch();

	private:
		static const size_t s_PyramidSize;
		static const size_t s_TriangleSize;
		static const size_t s_CubeSize;

		static const uint32_t s_PyramidIndicesSize;
		static const uint32_t s_TriangleIndicesSize;
		static const uint32_t s_CubeIndicesSize;

		static int tileIndexX;
		static int tileIndexY;
		static float animTime;

		static rtmcpp::Vec2 m_ScrollingPlusDivision;
	};

}