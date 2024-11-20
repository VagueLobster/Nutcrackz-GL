#pragma once

#include "Nutcrackz/Input/InputDevice.hpp"

namespace Nutcrackz {

	class GenericInputDeviceProvider : public InputProvider
	{
	public:
		void Init(InputDeviceRegistry registry) override;
		void Update() override;

	private:
		InputDeviceRegistry m_DeviceRegistry;
	};

}
