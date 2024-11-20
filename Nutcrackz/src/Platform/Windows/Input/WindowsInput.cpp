#include "nzpch.hpp"
#include "Nutcrackz/Input/InputSystemImpl.hpp"

#include "GameInputProvider.hpp"

namespace Nutcrackz {

	void RegisterPlatformInputProviders(InputSystem inputSystem)
	{
		inputSystem->RegisterProvider<GameInputProvider>();
	}

}
