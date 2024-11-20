#pragma once

#include "InputAction.hpp"
#include "InputContext.hpp"
#include "ExternalInputChannel.hpp"

namespace Nutcrackz {

	struct InputSystem : Handle<InputSystem>
	{
		InputContext CreateContext();
		InputAction RegisterAction(const InputActionData& actionData);
	};

}
