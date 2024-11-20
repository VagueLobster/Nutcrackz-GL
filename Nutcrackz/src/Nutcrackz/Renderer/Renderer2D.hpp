#pragma once

#include "Nutcrackz/Renderer/Texture.hpp"

#include "Nutcrackz/Renderer/Camera.hpp"
#include "Nutcrackz/Renderer/EditorCamera.hpp"
#include "Nutcrackz/Renderer/Font.hpp"

#include "Nutcrackz/Scene/Components.hpp"

#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, const rtmcpp::Mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void EndScene();
		static void Flush();

		// Legacy Primitives
		//static void DrawTri(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		//static void DrawTri(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawTri(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, int entityID = -1);
		//static void DrawTri(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec2& tilingFactor = glm::vec2(1.0f), const glm::vec4& tintColor = glm::vec4(1.0f));
		//static void DrawTri(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec2& tilingFactor = glm::vec2(1.0f), const glm::vec4& tintColor = glm::vec4(1.0f));
		//static void DrawTri(const glm::mat4& transform, const Ref<Texture2D>& texture, glm::vec2 tilingFactor = glm::vec2(1.0f), const glm::vec4& tintColor = glm::vec4(1.0f), float saturation = 1.0f, int entityID = -1);

		//static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float saturation);
		//static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, float saturation);
		//static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec2& tilingFactor = glm::vec2(1.0f), const glm::vec4& tintColor = glm::vec4(1.0f));
		//static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec2& tilingFactor = glm::vec2(1.0f), const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, rtmcpp::Vec2 tilingFactor = rtmcpp::Vec2(1.0f, 1.0f), const rtmcpp::Vec4& tintColor = rtmcpp::Vec4(1.0f, 1.0f, 1.0f, 1.0f), float saturation = 1.0f, int entityID = -1);

		//static void DrawRotatedTri(const glm::vec2& position, const glm::vec2& size, float& rotation, const glm::vec4& color);
		//static void DrawRotatedTri(const glm::vec3& position, const glm::vec2& size, float& rotation, const glm::vec4& color);
		//static void DrawRotatedTri(const glm::vec2& position, const glm::vec2& size, float& rotation, const Ref<Texture2D>& texture, const glm::vec2& tilingFactor = glm::vec2(1.0f), const glm::vec4& tintColor = glm::vec4(1.0f));
		//static void DrawRotatedTri(const glm::vec3& position, const glm::vec2& size, float& rotation, const Ref<Texture2D>& texture, const glm::vec2& tilingFactor = glm::vec2(1.0f), const glm::vec4& tintColor = glm::vec4(1.0f));

		//static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float& rotation, const glm::vec4& color, float saturation);
		//static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float& rotation, const glm::vec4& color, float saturation);
		//static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float& rotation, const Ref<Texture2D>& texture, const glm::vec2& tilingFactor = glm::vec2(1.0f), const glm::vec4& tintColor = glm::vec4(1.0f));
		//static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float& rotation, const Ref<Texture2D>& texture, const glm::vec2& tilingFactor = glm::vec2(1.0f), const glm::vec4& tintColor = glm::vec4(1.0f));

		// Primitives
		//static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, SpriteRendererComponent& src, const glm::vec4& tintColor = glm::vec4(1.0f));
		//static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, SpriteRendererComponent& src, const glm::vec4& tintColor = glm::vec4(1.0f));
		//static void DrawAnimatedQuad(const glm::vec2& position, const glm::vec2& size, SpriteRendererComponent& src);
		//static void DrawAnimatedQuad(const glm::vec3& position, const glm::vec2& size, SpriteRendererComponent& src);

		static void DrawTri(const rtmcpp::Mat4& transform, const RefPtr<Texture2D>& texture, TriangleRendererComponent& src, int entityID = -1);

		static void DrawQuad(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, float saturation, int entityID = -1);
		static void DrawQuad(const rtmcpp::Mat4& transform, SpriteRendererComponent& src, const Timestep& ts = 0.0f, const rtmcpp::Mat4& cameraProjection = rtmcpp::Mat4(), int entityID = -1);
		static void DrawAnimatedQuad(const rtmcpp::Mat4& transform, SpriteRendererComponent& src, const Timestep& ts = 0.0f, int entityID = -1);

		//static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, SpriteRendererComponent& src, const glm::vec4& tintColor = glm::vec4(1.0f));
		//static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, SpriteRendererComponent& src, const glm::vec4& tintColor = glm::vec4(1.0f));
		//static void DrawAnimatedRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, SpriteRendererComponent& src, const glm::vec4& tintColor = glm::vec4(1.0f));
		//static void DrawAnimatedRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, SpriteRendererComponent& src, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void DrawCircle(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);
		static void DrawCircle(const rtmcpp::Mat4& transform, CircleRendererComponent& src, const Timestep& ts = 0.0f, const rtmcpp::Mat4& cameraProjection = rtmcpp::Mat4(), float thickness = 1.0f, float fade = 0.005f, int entityID = -1);

		static void DrawLine(const rtmcpp::Vec4& p0, const rtmcpp::Vec4& p1, const rtmcpp::Vec4& color, int entityID = -1);
		static void DrawLine(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& p0, const rtmcpp::Vec4& p1, const rtmcpp::Vec4& color, int entityID = -1);

		//static void DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
		static void DrawRect(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, int entityID = -1);

		static void DrawCircleLine(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);

		//static void DrawTriangle(const glm::mat4& transform, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);
		static void DrawTriangle(const rtmcpp::Mat4& transform, TriangleRendererComponent& src, int entityID = -1);

		static void DrawSprite(const rtmcpp::Mat4& transform, SpriteRendererComponent& src, const Timestep& ts, bool UseTextureAnimation, const rtmcpp::Mat4& cameraProjection = rtmcpp::Mat4(), int entityID = -1);
		//static void DrawSprite(const glm::mat4& transform, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);
		static void DrawCircle(const rtmcpp::Mat4& transform, CircleRendererComponent& src, const Timestep& ts, const rtmcpp::Mat4& cameraProjection = rtmcpp::Mat4(), int entityID = -1);

		struct TextParams
		{
			rtmcpp::Vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
			rtmcpp::Vec4 BgColor{ 0.0f, 0.0f, 0.0f, 0.0f };
			float Kerning = 0.0f;
			float LineSpacing = 0.0f;
		};
		static void DrawString(const std::string& string, TextComponent& src, const rtmcpp::Mat4& transform, const TextParams& textParams, rtmcpp::Vec2& textQuadMin, rtmcpp::Vec2& textQuadMax, rtmcpp::Vec2& textCoordMin, rtmcpp::Vec2& textCoordMax, rtmcpp::Vec2& planeMin, rtmcpp::Vec2& planeMax, float& newLineCounter, float& yOffset, int entityID = -1);
		static void DrawString(const std::string& string, const rtmcpp::Mat4& transform, TextComponent& component, int entityID = -1);

		// UI Layouts
		//static void DrawQuadWidget(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float radius, const glm::vec2& dimensions, bool shouldInvertUI);
		//static void DrawQuadWidget(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, float radius, const glm::vec2& dimensions, bool shouldInvertUI);
		//static void DrawRotatedQuadWidget(const glm::vec2& position, const glm::vec2& size, float& rotation, const glm::vec4& color, float radius, const glm::vec2& dimensions, bool shouldInvertUI);
		//static void DrawRotatedQuadWidget(const glm::vec3& position, const glm::vec2& size, float& rotation, const glm::vec4& color, float radius, const glm::vec2& dimensions, bool shouldInvertUI);
		static void DrawQuadWidget(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, float radius, const rtmcpp::Vec2& dimensions, bool shouldInvertUI, int entityID = -1);
		static void DrawQuadWidget(const rtmcpp::Mat4& transform, ButtonWidgetComponent& src, int entityID = -1);
		//static void DrawSpriteWidget(const glm::mat4& transform, const glm::vec4& tintColor = glm::vec4(1.0f), float radius = 0.0f, const glm::vec2& dimensions = glm::vec2(1.0f), bool shouldInvertUI = false, int entityID = -1);
		static void DrawSpriteWidget(const rtmcpp::Mat4& transform, ButtonWidgetComponent& src, int entityID = -1);
		static void DrawSpriteWidget(const rtmcpp::Mat4& transform, CircleWidgetComponent& src, int entityID = -1);
		static void DrawCircleWidget(const rtmcpp::Mat4& transform, const rtmcpp::Vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);
		static void DrawCircleWidget(const rtmcpp::Mat4& transform, CircleWidgetComponent& src, int entityID = -1);

		static float GetLineWidth();
		static void SetLineWidth(float width);

		static void UpdateAnimation(const int& numTiles, const int& startIndexX, const int& startIndexY, const float& animSpeed, float& animationTime, bool useAnimation, bool useCameraParallaxY = false);
		static void UpdateTextureAnimation(const int& numTextures, const int& startIndex, const float& animSpeed, float& animationTime, bool useAnimation);

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t TriangleCount = 0;

			uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() const { return QuadCount * 6; }

			uint32_t GetTotalTriangleVertexCount() const { return TriangleCount * 3; }
			uint32_t GetTotalTriangleIndexCount() const { return TriangleCount * 3; }
		};

		static void ResetStats();
		static Statistics GetStats();

	public:
		static bool s_AnimateInRuntime;
		static bool s_AnimateInEdit;

	private:
		static void StartBatch();
		static void NextBatch();

	private:
		static int tileIndexX;
		static int tileIndexY;
		static float animTime;
	};

}
