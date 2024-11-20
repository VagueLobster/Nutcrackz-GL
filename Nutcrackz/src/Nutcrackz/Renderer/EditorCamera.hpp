#pragma once

#include "Camera.hpp"
#include "Nutcrackz/Core/Timestep.hpp"
#include "Nutcrackz/Input/InputContext.hpp"
//#include "Nutcrackz/Events/Event.hpp"
//#include "Nutcrackz/Events/MouseEvent.hpp"

//#include <glm/glm.hpp>
#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(Timestep ts, bool control = true, bool isSandbox = false);
		//void OnEvent(Event& e);

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		inline void SetViewportSize(float width, float height, const rtmcpp::Mat4& projMatrix = rtmcpp::Mat4()) { m_ViewportWidth = width; m_ViewportHeight = height; UpdateProjection(projMatrix); };

		const rtmcpp::Mat4& GetViewMatrix() const { return m_ViewMatrix; }
		//rtmcpp::Mat4 GetViewProjection() const { return m_Projection * m_ViewMatrix; }
		rtmcpp::Mat4 GetViewProjection() const { return m_ViewMatrix * m_Projection; }

		rtmcpp::Vec3 GetUpDirection() const;
		rtmcpp::Vec3 GetRightDirection() const;
		virtual rtmcpp::Vec3 GetForwardDirection() const;
		const rtmcpp::Vec3& GetPosition() const { return m_Position; }
		rtmcpp::Quat4 GetOrientation() const;

		const rtmcpp::Vec3& GetFocalPoint() const { return m_FocalPoint; }
		const float GetWidth() const { return m_ViewportWidth; }
		const float GetHeight() const { return m_ViewportHeight; }
		
		const float GetFOV() const { return m_FOV; }
		const float GetAspectRatio() const { return m_AspectRatio; }
		const float GetNearClip() const { return m_NearClip; }
		const float GetFarClip() const { return m_FarClip; }
		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }

		void SetFOV(float fov) { m_FOV = fov; }
		void SetAspectRatio(float aspect) { m_AspectRatio = aspect; }
		void SetNearClip(float nearClip) { m_NearClip = nearClip; }
		void SetFarClip(float farClip) { m_FarClip = farClip; }
		void SetPitch(float value) { m_Pitch = value; }
		void SetYaw(float value) { m_Yaw = value; }

		void SetPosition(const rtmcpp::Vec3& position) { m_Position = position; }
		const rtmcpp::Vec3& GetRotation() { return m_Rotation; }
		void SetRotation(const rtmcpp::Vec3& rotation) { m_Rotation = rotation; }

	private:
		void UpdateProjection(const rtmcpp::Mat4& projMatrix = rtmcpp::Mat4());
		void UpdateView();

		//bool OnMouseScroll(MouseScrolledEvent& e);
		bool OnMouseScroll(float offsetY);

		void MousePan(const rtmcpp::Vec2& delta);
		void MouseRotate(const rtmcpp::Vec2& delta);
		void MouseZoom(float delta);

		rtmcpp::Vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	public:
		bool ViewportFocused = false;
		bool ViewportHovered = false;
		bool IsControllable = true;

	private:
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		rtmcpp::Mat4 m_ViewMatrix;
		rtmcpp::Vec3 m_Position = rtmcpp::Vec3{ 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 m_Rotation = rtmcpp::Vec3{ 0.0f, 0.0f, 0.0f };
		rtmcpp::Vec3 m_FocalPoint = rtmcpp::Vec3{ 0.0f, 0.0f, 0.0f };

		rtmcpp::Vec2 m_InitialMousePosition = rtmcpp::Vec2{ 0.0f, 0.0f };

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1280.0f, m_ViewportHeight = 720.0f;

		bool m_AltKeyPressed = false;
		rtmcpp::Vec2 m_MouseDelta = rtmcpp::Vec2{ 0.0f, 0.0f };
		float m_Timestep = 0.0f;


		InputContext m_AltContext;
		InputContext m_EditorCameraContext;
	};
}
