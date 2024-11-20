#include "nzpch.hpp"
#include "EditorCamera.hpp"

#include "Nutcrackz/Core/Input.hpp"
#include "Nutcrackz/Core/KeyCodes.hpp"
#include "Nutcrackz/Core/MouseCodes.hpp"

#include "Nutcrackz/Core/Application.hpp"
#include "Nutcrackz/Input/InputDevice.hpp"

#include <glfw/glfw3.h>

//#define GLM_ENABLE_EXPERIMENTAL
//#include <glm/gtx/quaternion.hpp>

#include "rtmcpp/Scalar.hpp"
#include "rtmcpp/Transforms.hpp"

namespace Nutcrackz {

	rtmcpp::Vec3 Rotate(const rtmcpp::Quat4& q, const rtmcpp::Vec3& v)
	{
		//return q * v; // From glm
		return v * q;
	}

	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
		//: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip))
		: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), Camera(rtmcpp::Mat4::PerspectiveInfReversedZ(rtmcpp::Radians(fov), aspectRatio, nearClip))
		//: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), Camera(rtmcpp::Mat4::PerspectiveInfReversedZ(fov, aspectRatio, nearClip))
	{
		UpdateView();

		using enum TriggerEventType;

		m_EditorCameraContext = Application::Get().GetInputSystem().CreateContext();
		m_AltContext = Application::Get().GetInputSystem().CreateContext();

		auto activateAltAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::LeftAlt, OnHeld }, 1.0f },
						{ { GenericKeyboard, KeyCode::LeftAlt, OnReleased }, 0.0f }
					}
				}
			}
		});

		m_EditorCameraContext.BindAction(activateAltAction, [&](const InputReading& reading)
		{
			auto [val] = reading.Read<1>();

			if (val > 0.0f)
			{
				if ((ViewportFocused || ViewportHovered) && IsControllable)
				{
					const rtmcpp::Vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
					rtmcpp::Vec2 mousePos = mouse - m_InitialMousePosition;
					m_MouseDelta = rtmcpp::Vec2(mousePos.X * 0.003f, mousePos.Y * 0.003f);
					m_InitialMousePosition = mouse;
				}

				m_AltContext.Activate();
			}
			else
			{
				m_AltContext.Deactivate();
			}
		});

		auto mouseAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericMouse, MouseCode::ButtonLeft, OnHeld }, 1.0f },
						{ { GenericMouse, MouseCode::ButtonLeft, OnReleased }, 0.0f }
					}
				},
				{
					.Bindings = {
						{ { GenericMouse, MouseCode::ButtonMiddle, OnHeld }, 1.0f },
						{ { GenericMouse, MouseCode::ButtonMiddle, OnReleased }, 0.0f }
					}
				},
				{
					.Bindings = {
						{ { GenericMouse, MouseCode::ButtonRight, OnHeld }, 1.0f },
						{ { GenericMouse, MouseCode::ButtonRight, OnReleased }, 0.0f }
					}
				}
			}
		});

		m_AltContext.BindAction(mouseAction, [&](const InputReading& reading)
		{
			auto [left, middle, right] = reading.Read<3>();

			//NZ_CORE_WARN("Left mouse button pressed = {}", left);
			//NZ_CORE_WARN("Left mouse button pressed = {},{}", m_MousePos.x, m_MousePos.y);

			if ((ViewportFocused || ViewportHovered) && IsControllable)
			{
				if (middle > 0.0f)
					MousePan(rtmcpp::Vec2{ m_MouseDelta.X, m_MouseDelta.Y });
					//MousePan(rtmcpp::Vec2{ m_MouseDelta.X * 0.075f, m_MouseDelta.Y * 0.075f });
				else if (left > 0.0f)
					MouseRotate(m_MouseDelta);
				else if (right > 0.0f)
					MouseZoom(m_MouseDelta.Y);

				UpdateView();
			}
		});

		auto mouseMovementAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::LeftArrow, OnHeld }, 1.0f },
						{ { GenericKeyboard, KeyCode::RightArrow, OnHeld }, -1.0f },
						{ { GenericKeyboard, KeyCode::LeftArrow, OnReleased }, 0.0f },
						{ { GenericKeyboard, KeyCode::RightArrow, OnReleased }, 0.0f }
					}
				},
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::UpArrow, OnHeld }, 1.0f },
						{ { GenericKeyboard, KeyCode::DownArrow, OnHeld }, -1.0f },
						{ { GenericKeyboard, KeyCode::UpArrow, OnReleased }, 0.0f },
						{ { GenericKeyboard, KeyCode::DownArrow, OnReleased }, 0.0f }
					}
				}
			}
		});

		m_EditorCameraContext.BindAction(mouseMovementAction, [&](const InputReading& reading)
		{
			auto [x, y] = reading.Read<2>();

			if ((ViewportFocused || ViewportHovered) && IsControllable)
			{
				auto [xSpeed, ySpeed] = PanSpeed();

				if (x > 0.0f)
				{
					m_FocalPoint += rtmcpp::Vec3((-GetRightDirection().X * m_Timestep * xSpeed * m_Distance) * 0.075f, (-GetRightDirection().Y * m_Timestep * xSpeed * m_Distance) * 0.075f, (-GetRightDirection().Z * m_Timestep * xSpeed * m_Distance) * 0.075f);
				}
				else if (x < 0.0f)
				{
					m_FocalPoint -= rtmcpp::Vec3((-GetRightDirection().X * m_Timestep * xSpeed * m_Distance) * 0.075f, (-GetRightDirection().Y * m_Timestep * xSpeed * m_Distance) * 0.075f, (-GetRightDirection().Z * m_Timestep * xSpeed * m_Distance) * 0.075f);
				}

				if (y > 0.0f)
				{
					m_FocalPoint += rtmcpp::Vec3((GetUpDirection().X * m_Timestep * ySpeed * m_Distance) * 0.075f, (GetUpDirection().Y * m_Timestep * ySpeed * m_Distance) * 0.075f, (GetUpDirection().Z * m_Timestep * ySpeed * m_Distance) * 0.075f);
				}
				else if (y < 0.0f)
				{
					m_FocalPoint -= rtmcpp::Vec3((GetUpDirection().X * m_Timestep * ySpeed * m_Distance) * 0.075f, (GetUpDirection().Y * m_Timestep * ySpeed * m_Distance) * 0.075f, (GetUpDirection().Z * m_Timestep * ySpeed * m_Distance) * 0.075f);
				}

				UpdateView();
			}
		});

		auto mouseScrollAction = Application::Get().GetInputSystem().RegisterAction({
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericMouse, MouseCode::WheelScrollY, OnPressed }, 1.0f }
					}
				}
			}
		});

		m_EditorCameraContext.BindAction(mouseScrollAction, [&](const InputReading& reading)
		{
			auto [x] = reading.Read<1>();

			if (ViewportHovered && IsControllable)
			{
				OnMouseScroll(x);
			}
		});

		m_EditorCameraContext.Activate();
	}

	void EditorCamera::UpdateProjection(const rtmcpp::Mat4& projMatrix)
	{
		if (IsControllable)
		{
			m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
			//m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip); // Old
			m_Projection = rtmcpp::Mat4::PerspectiveInfReversedZ(rtmcpp::Radians(m_FOV), m_AspectRatio, m_NearClip);
			//m_Projection = rtmcpp::Mat4::PerspectiveInfReversedZ(m_FOV, m_AspectRatio, m_NearClip);
		}
		else
		{
			m_Projection = projMatrix;
		}
	}

	void EditorCamera::UpdateView()
	{
		// m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
		m_Position = CalculatePosition();

		rtmcpp::Quat4 orientation = GetOrientation();
		//m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation); // Old
		m_ViewMatrix = rtmcpp::Mat4Cast(orientation) * rtmcpp::Mat4Cast(rtmcpp::Translation(m_Position));
		m_ViewMatrix = rtmcpp::Inverse(m_ViewMatrix);
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 0.8f;
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCamera::OnUpdate(Timestep ts, bool control, bool isSandbox)
	{
		m_Timestep = ts * 2.0f;

		static bool updatedView = false;

		if (!updatedView && IsControllable)
		{
			UpdateView();
			updatedView = true;
		}
		else if (!IsControllable)
		{
			rtmcpp::Mat4 rotation = rtmcpp::Mat4Cast(rtmcpp::FromEuler(rtmcpp::Vec3{ m_Rotation.Y, m_Rotation.Z, m_Rotation.X }));

			m_ViewMatrix = rotation * rtmcpp::Mat4Cast(rtmcpp::Translation(m_Position));
			m_ViewMatrix = rtmcpp::Inverse(m_ViewMatrix);
		}
	}

	/*void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(NZ_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
	}*/

	//bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	bool EditorCamera::OnMouseScroll(float offsetY)
	{
		float delta = (offsetY / 120.0f) * 0.1f;
		MouseZoom(delta);
		UpdateView();
		return false;
	}

	void EditorCamera::MousePan(const rtmcpp::Vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		//m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance; // Old
		//m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;     // Old
		
		m_FocalPoint += rtmcpp::Vec3(-GetRightDirection().X * delta.X * xSpeed * m_Distance, -GetRightDirection().Y * delta.X * xSpeed * m_Distance, -GetRightDirection().Z * delta.X * xSpeed * m_Distance);
		m_FocalPoint += rtmcpp::Vec3(GetUpDirection().X * delta.Y * ySpeed * m_Distance, GetUpDirection().Y * delta.Y * ySpeed * m_Distance, GetUpDirection().Z * delta.Y * ySpeed * m_Distance);
	}

	void EditorCamera::MouseRotate(const rtmcpp::Vec2& delta)
	{
		float yawSign = GetUpDirection().Y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.X * RotationSpeed();
		m_Pitch += delta.Y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();

		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	rtmcpp::Vec3 EditorCamera::GetUpDirection() const
	{
		return Rotate(GetOrientation(), rtmcpp::Vec3(0.0f, 1.0f, 0.0f));
	}

	rtmcpp::Vec3 EditorCamera::GetRightDirection() const
	{
		return Rotate(GetOrientation(), rtmcpp::Vec3(1.0f, 0.0f, 0.0f));
	}

	rtmcpp::Vec3 EditorCamera::GetForwardDirection() const
	{
		return Rotate(GetOrientation(), rtmcpp::Vec3(0.0f, 0.0f, -1.0f));
	}

	rtmcpp::Vec3 EditorCamera::CalculatePosition() const
	{
		//return m_FocalPoint - GetForwardDirection() * m_Distance; // Old
		
		//rtmcpp::Vec3 result = rtmcpp::Vec3(m_FocalPoint - GetForwardDirection());
		//return rtmcpp::Vec3(result.X * m_Distance, result.Y * m_Distance, result.Z * m_Distance);
		return rtmcpp::Vec3(m_FocalPoint.X - GetForwardDirection().X * m_Distance, m_FocalPoint.Y - GetForwardDirection().Y * m_Distance, m_FocalPoint.Z - GetForwardDirection().Z * m_Distance);
	}

	rtmcpp::Quat4 EditorCamera::GetOrientation() const
	{
		//return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f)); // Old
		return rtmcpp::FromEuler(rtmcpp::Vec3(m_Yaw, 0.0f, m_Pitch));
	}
}