#pragma once

#include "Nutcrackz/Core/KeyCodes.hpp"
#include "Nutcrackz/Core/MouseCodes.hpp"
#include "Nutcrackz/Core/GamepadCodes.hpp"
#include "Nutcrackz/Events/Event.hpp"
#include "Nutcrackz/Core/Window.hpp"

//#include <glm/glm.hpp>
#include "rtmcpp/Common.hpp"
//#include "rtmcpp/Transforms.hpp"

#include <map>

namespace Nutcrackz {

	struct ControllerButtonData
	{
		int Button;
		KeyState State = KeyState::None;
		KeyState OldState = KeyState::None;
	};

	struct Controller
	{
		int ID;
		std::string Name;
		std::map<int, bool> ButtonDown;
		std::map<int, ControllerButtonData> ButtonStates;
		std::map<GamepadAxis, float> AxisStates;
		std::map<GamepadAxis, float> DeadZones;
		std::map<int, uint8_t> HatStates;
	};

	class Input
	{
	public:		
		static bool IsKeyPressed(KeyCodeHazel keycode);
		static bool IsKeyReleased(KeyCodeHazel keycode);

		static bool IsMouseButtonPressed(MouseCodeHazel button);
		static bool PressMouseButton(MouseCodeHazel button);
		static bool ReleaseMouseButton(MouseCodeHazel button);
		static rtmcpp::Vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
		static rtmcpp::Vec2 GetViewportMousePosition();
		static float GetViewportMouseX();
		static float GetViewportMouseY();

		static void Update();

		static void TransitionPressedButtons();
		static void ClearReleasedKeys();

		// Controllers
		static void UpdateControllerButtonState(int controllerID, int button, KeyState newState);

		static bool IsControllerPresent(int id);
		static std::vector<int> GetConnectedControllerIDs();
		static const Controller* GetController(int id);
		static std::string_view GetControllerName(int id);

		static bool IsControllerButtonPressed(int controllerID, int button);
		static bool IsControllerButtonHeld(int controllerID, int button);
		static bool IsControllerButtonDown(int controllerID, int button);
		static bool IsControllerButtonReleased(int controllerID, int button);

		static float GetControllerAxis(int controllerID, GamepadAxis axis);
		static uint8_t GetControllerHat(int controllerID, int hat);

		static float GetControllerDeadzone(int controllerID, GamepadAxis axis);
		static void SetControllerDeadzone(int controllerID, GamepadAxis axis, float deadzone);

		static const std::map<int, Controller>& GetControllers() { return s_Controllers; }

	public:
		static rtmcpp::Vec2 s_MousePos;
		static rtmcpp::Vec2 s_ViewportSize;
		static rtmcpp::Vec2 s_ViewportBounds;
		static int s_MouseX;
		static int s_MouseY;

	private:
		inline static std::map<int, Controller> s_Controllers;
	};

}
