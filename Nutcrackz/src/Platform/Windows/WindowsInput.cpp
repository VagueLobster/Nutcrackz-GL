#include "nzpch.hpp"
#include "Nutcrackz/Core/Input.hpp"
#include "WindowsWindow.hpp"
#include "Nutcrackz/Core/Application.hpp"

#include <GLFW/glfw3.h>

#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	rtmcpp::Vec2 Input::s_MousePos = rtmcpp::Vec2(0.0f, 0.0f);
	rtmcpp::Vec2 Input::s_ViewportSize = rtmcpp::Vec2(0.0f, 0.0f);
	rtmcpp::Vec2 Input::s_ViewportBounds = rtmcpp::Vec2(0.0f, 0.0f);
	int Input::s_MouseX = 0;
	int Input::s_MouseY = 0;

	const float DeadzoneX = 0.05f;
	const float DeadzoneY = 0.02f;

	struct GamepadState
	{
		bool IsConnected;
		std::unordered_map<GamepadCode, bool> Buttons;
		rtmcpp::Vec2 LeftJoystick;
		rtmcpp::Vec2 RightJoystick;
		rtmcpp::Vec2 Triggers;
		rtmcpp::Vec2 Vibration;
	};

	static std::array<GamepadState, MaxGamepadCount> s_Gamepads;

	bool Input::IsKeyPressed(const KeyCodeHazel key)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetKey(window, static_cast<int32_t>(key));
		return state == GLFW_PRESS;
	}

	bool Input::IsKeyReleased(const KeyCodeHazel key)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetKey(window, static_cast<int32_t>(key));
		return state == GLFW_RELEASE;
	}

	bool Input::IsMouseButtonPressed(const MouseCodeHazel button)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	bool Input::PressMouseButton(MouseCodeHazel button)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	bool Input::ReleaseMouseButton(MouseCodeHazel button)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_RELEASE;
	}

	rtmcpp::Vec2 Input::GetMousePosition()
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}

	float Input::GetMouseX()
	{
		return GetMousePosition().X;
	}

	float Input::GetMouseY()
	{
		return GetMousePosition().Y;
	}

	rtmcpp::Vec2 Input::GetViewportMousePosition()
	{
		s_MousePos.X -= s_ViewportBounds.X;
		s_MousePos.Y -= s_ViewportBounds.Y;
		rtmcpp::Vec2 viewportSize = s_ViewportSize;
		s_MousePos.Y = viewportSize.Y - s_MousePos.Y;
		//int mouseX = (int)s_MousePos.X;
		//int mouseY = (int)s_MousePos.Y;

		//return { (float)mouseX, (float)mouseY };
		return s_MousePos;
	}

	float Input::GetViewportMouseX()
	{
		return GetViewportMousePosition().X;
	}

	float Input::GetViewportMouseY()
	{
		return GetViewportMousePosition().Y;
	}

	void Input::Update()
	{
		// Cleanup disconnected controller
		for (auto it = s_Controllers.begin(); it != s_Controllers.end(); )
		{
			int id = it->first;
			if (glfwJoystickPresent(id) != GLFW_TRUE)
				it = s_Controllers.erase(it);
			else
				it++;
		}

		// Update controllers
		for (int id = GLFW_JOYSTICK_1; id < GLFW_JOYSTICK_LAST; id++)
		{
			if (glfwJoystickPresent(id) == GLFW_TRUE)
			{
				Controller& controller = s_Controllers[id];
				controller.ID = id;
				controller.Name = glfwGetJoystickName(id);

				int buttonCount;
				const unsigned char* buttons = glfwGetJoystickButtons(id, &buttonCount);
				for (int i = 0; i < buttonCount; i++)
				{
					if (buttons[i] == GLFW_PRESS && !controller.ButtonDown[i])
						controller.ButtonStates[i].State = KeyState::Pressed;
					else if (buttons[i] == GLFW_RELEASE && controller.ButtonDown[i])
						controller.ButtonStates[i].State = KeyState::Released;

					controller.ButtonDown[i] = buttons[i] == GLFW_PRESS;
				}

				int axisCount;
				const float* axes = glfwGetJoystickAxes(id, &axisCount);
				for (int i = 0; i < axisCount; i++)
					controller.AxisStates[i] = abs(axes[i]) > controller.DeadZones[i] ? axes[i] : 0.0f;

				int hatCount;
				const unsigned char* hats = glfwGetJoystickHats(id, &hatCount);
				for (int i = 0; i < hatCount; i++)
					controller.HatStates[i] = hats[i];

				//NZ_CORE_TRACE("Controller id: {}", id);
			}

		}
	}

	void Input::TransitionPressedButtons()
	{
		for (const auto& [id, controller] : s_Controllers)
		{
			for (const auto& [button, buttonStates] : controller.ButtonStates)
			{
				if (buttonStates.State == KeyState::Pressed)
					UpdateControllerButtonState(id, button, KeyState::Held);
			}
		}
	}

	void Input::ClearReleasedKeys()
	{
		for (const auto& [id, controller] : s_Controllers)
		{
			for (const auto& [button, buttonStates] : controller.ButtonStates)
			{
				if (buttonStates.State == KeyState::Released)
					UpdateControllerButtonState(id, button, KeyState::None);
			}
		}
	}

	// Controllers
	void Input::UpdateControllerButtonState(int controllerID, int button, KeyState newState)
	{
		auto& controllerButtonData = s_Controllers.at(controllerID).ButtonStates.at(button);
		controllerButtonData.Button = button;
		controllerButtonData.OldState = controllerButtonData.State;
		controllerButtonData.State = newState;
	}

	bool Input::IsControllerPresent(int id)
	{
		return s_Controllers.find(id) != s_Controllers.end();
	}

	std::vector<int> Input::GetConnectedControllerIDs()
	{
		std::vector<int> ids;
		ids.reserve(s_Controllers.size());
		for (auto [id, controller] : s_Controllers)
			ids.emplace_back(id);

		return ids;
	}

	const Controller* Input::GetController(int id)
	{
		if (!Input::IsControllerPresent(id))
			return nullptr;

		return &s_Controllers.at(id);
	}

	std::string_view Input::GetControllerName(int id)
	{
		if (!Input::IsControllerPresent(id))
			return {};

		return s_Controllers.at(id).Name;
	}

	bool Input::IsControllerButtonPressed(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return false;

		auto& contoller = s_Controllers.at(controllerID);
		return contoller.ButtonStates.find(button) != contoller.ButtonStates.end() && contoller.ButtonStates[button].State == KeyState::Pressed;
	}

	bool Input::IsControllerButtonHeld(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return false;

		auto& contoller = s_Controllers.at(controllerID);
		return contoller.ButtonStates.find(button) != contoller.ButtonStates.end() && contoller.ButtonStates[button].State == KeyState::Held;
	}

	bool Input::IsControllerButtonDown(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return false;

		const Controller& controller = s_Controllers.at(controllerID);
		if (controller.ButtonDown.find(button) == controller.ButtonDown.end())
			return false;

		return controller.ButtonDown.at(button);
	}

	bool Input::IsControllerButtonReleased(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return true;

		auto& contoller = s_Controllers.at(controllerID);
		return contoller.ButtonStates.find(button) != contoller.ButtonStates.end() && contoller.ButtonStates[button].State == KeyState::Released;
	}

	float Input::GetControllerAxis(int controllerID, GamepadAxis axis)
	{
		if (!Input::IsControllerPresent(controllerID))
			return 0.0f;

		const Controller& controller = s_Controllers.at(controllerID);
		if (controller.AxisStates.find(axis) == controller.AxisStates.end())
			return 0.0f;

		return controller.AxisStates.at(axis);
	}

	uint8_t Input::GetControllerHat(int controllerID, int hat)
	{
		if (!Input::IsControllerPresent(controllerID))
			return 0;

		const Controller& controller = s_Controllers.at(controllerID);
		if (controller.HatStates.find(hat) == controller.HatStates.end())
			return 0;

		return controller.HatStates.at(hat);
	}

	float Input::GetControllerDeadzone(int controllerID, GamepadAxis axis)
	{
		if (!Input::IsControllerPresent(controllerID))
			return 0.0f;

		const Controller& controller = s_Controllers.at(controllerID);
		return controller.DeadZones.at(axis);
	}

	void Input::SetControllerDeadzone(int controllerID, GamepadAxis axis, float deadzone)
	{
		if (!Input::IsControllerPresent(controllerID))
			return;

		Controller& controller = s_Controllers.at(controllerID);
		controller.DeadZones[axis] = deadzone;
	}

}
