using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
using Coral.Managed.Interop;

namespace Nutcrackz
{
    // Borrowed from Hazel Dev and from the Coral-Integration branch!
    public class Gamepad
    {
        public static bool IsControllerPresent(int id)
        {
            unsafe { return InternalCalls.Input_IsControllerPresent(id); }
        }

        public static int[] GetConnectedControllerIDs()
        {
            unsafe
            {
                using NativeArray<int> controllerIds = InternalCalls.Input_GetConnectedControllerIDs();
                return controllerIds;
            }
        }

        public static string GetControllerName(int id)
        {
            unsafe { return InternalCalls.Input_GetControllerName(id); }
        }

        /// <summary>
        /// Returns true during the frame that the button was released
        /// </summary>
        public static bool IsControllerButtonPressed(int id, GamepadButton button)
        {
            unsafe { return InternalCalls.Input_IsControllerButtonPressed(id, (int)button); }
        }

        public static bool IsControllerButtonPressed(int id, int button)
        {
            unsafe { return InternalCalls.Input_IsControllerButtonPressed(id, button); }
        }

        /// <summary>
        /// Returns true every frame after the button was initially pressed (returns false when <see cref="Input.IsMouseButtonPressed(MouseButton)"/> returns true)
        /// </summary>
        public static bool IsControllerButtonHeld(int id, GamepadButton button)
        {
            unsafe { return InternalCalls.Input_IsControllerButtonHeld(id, (int)button); }
        }

        public static bool IsControllerButtonHeld(int id, int button)
        {
            unsafe { return InternalCalls.Input_IsControllerButtonHeld(id, button); }
        }

        /// <summary>
        /// Returns true every frame that the button is down. Equivalent to doing <code>Input.IsMouseButtonPressed(key) || Input.IsMouseButtonHeld(key)</code>
        /// </summary>
        public static bool IsControllerButtonDown(int id, GamepadButton button)
        {
            unsafe { return InternalCalls.Input_IsControllerButtonDown(id, (int)button); }
        }

        public static bool IsControllerButtonDown(int id, int button)
        {
            unsafe { return InternalCalls.Input_IsControllerButtonDown(id, button); }
        }

        /// <summary>
        /// Returns true during the frame that the button was released
        /// </summary>
        public static bool IsControllerButtonReleased(int id, GamepadButton button)
        {
            unsafe { return InternalCalls.Input_IsControllerButtonReleased(id, (int)button); }
        }

        public static bool IsControllerButtonReleased(int id, int button)
        {
            unsafe { return InternalCalls.Input_IsControllerButtonReleased(id, button); }
        }

        public static float GetControllerAxis(int id, int axis)
        {
            unsafe { return InternalCalls.Input_GetControllerAxis(id, axis); }
        }

        public static byte GetControllerHat(int id, int hat)
        {
            unsafe { return InternalCalls.Input_GetControllerHat(id, hat); }
        }

        /// <summary>
        /// Getter for the specified controller's axis' deadzone, default value is 0.0f
        /// </summary>
        public static float GetControllerDeadzone(int id, int axis)
        {
            unsafe { return InternalCalls.Input_GetControllerDeadzone(id, axis); }
        }

        /// <summary>
        /// Setter for the specified controller's axis' deadzone, default value is 0.0f
        /// </summary>
        public static void SetControllerDeadzone(int id, int axis, float deadzone)
        {
            unsafe { InternalCalls.Input_SetControllerDeadzone(id, axis, deadzone); }
        }
    }
}
