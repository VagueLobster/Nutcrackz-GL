#include "nzpch.hpp"

#include "WootingInputProvider.hpp"

#include "Nutcrackz/Input/InputDeviceImpl.hpp"

#include "Nutcrackz/Core/Stack.hpp"

#include <wooting-analog-wrapper.h>

#include <stdlib.h>

namespace Nutcrackz {

	// NOTE(Peter): Hate to do this, but Wooting doesn't allow us to set a context variable for their device callbacks
	static WootingInputProvider* s_WootingContext = nullptr;

	static void DeviceCallback(WootingAnalog_DeviceEventType type, WootingAnalog_DeviceInfo_FFI* deviceInfo);

	using enum WootingAnalog_DeviceEventType;
	using enum WootingAnalog_KeycodeType;

	void WootingInputProvider::Init(InputDeviceRegistry registry)
	{
		//NutcrackzStackPoint();

		s_WootingContext = this;
		m_DeviceRegistry = registry;

		NutcrackzUnused(_putenv("RUST_LOG=off"));
		int status = wooting_analog_initialise();

		if (status < 0)
		{
			NZ_CORE_ERROR("Failed to initialize the Wooting Input Provider!");

			using enum WootingAnalogResult;

			switch (static_cast<WootingAnalogResult>(status))
			{
			case WootingAnalogResult_NoPlugins:
			{
				NZ_CORE_ERROR("Reason: No plugins discovered");
				break;
			}
			case WootingAnalogResult_FunctionNotFound:
			{
				NZ_CORE_ERROR("Reason: Failed to find a required function");
				break;
			}
			case WootingAnalogResult_DLLNotFound:
			{
				NZ_CORE_ERROR("Reason: Failed to find the Wooting DLL");
				break;
			}
			}

			return;
		}

		NZ_CORE_ERROR("Found {} Wooting devices.", status);

		wooting_analog_set_keycode_mode(WootingAnalog_KeycodeType_VirtualKey);
		wooting_analog_set_device_event_cb(DeviceCallback);

		auto deviceInfos = Nutcrackz::StackAlloc<WootingAnalog_DeviceInfo_FFI*>(status);
		wooting_analog_get_connected_devices_info(deviceInfos.Data(), status);

		for (auto deviceInfo : deviceInfos)
		{
			DeviceCallback(WootingAnalog_DeviceEventType_Connected, deviceInfo);
		}
	}

	void WootingInputProvider::Update()
	{
		static std::array<uint16_t, s_MaxKeyCount> s_KeyCodes;
		static std::array<float32_t, s_MaxKeyCount> s_KeyValues;

		for (auto [deviceID, device] : m_Devices)
		{
			auto inputDevice = m_DeviceRegistry.GetDevice(deviceID);

			int32_t keyCount = wooting_analog_read_full_buffer_device(s_KeyCodes.data(), s_KeyValues.data(), s_MaxKeyCount, device);

			if (keyCount < 0)
			{
				continue;
			}

			for (int32_t i = 0; i < keyCount; i++)
			{
				inputDevice->WriteChannelValue(static_cast<uint32_t>(s_KeyCodes[i]), s_KeyValues[i]);
			}
		}
	}

	extern uint32_t g_DummyID;

	void WootingInputProvider::RegisterWootingDevice(WootingAnalog_DeviceInfo_FFI* deviceInfo)
	{
		InputDeviceID deviceID = g_DummyID++;
		auto inputDevice = m_DeviceRegistry->CreateDevice(deviceID);
		inputDevice->Name = deviceInfo->device_name;
		inputDevice->ManufacturerName = deviceInfo->manufacturer_name;
		inputDevice->Type = InputDevice::Type::Keyboard;
		inputDevice->Channels.resize(s_MaxKeyCount, { 0.0f, 0.0f });

		m_Devices[deviceID] = deviceInfo->device_id;
	}

	void WootingInputProvider::UnregisterWootingDevice(WootingAnalog_DeviceInfo_FFI* deviceInfo)
	{
		std::erase_if(m_Devices, [deviceInfo](const auto& keyValue)
		{
			return keyValue.second == deviceInfo->device_id;
		});
	}

	void DeviceCallback(WootingAnalog_DeviceEventType type, WootingAnalog_DeviceInfo_FFI* deviceInfo)
	{
		switch (type)
		{
		case WootingAnalog_DeviceEventType_Connected:
		{
			s_WootingContext->RegisterWootingDevice(deviceInfo);
			break;
		}
		case WootingAnalog_DeviceEventType_Disconnected:
		{
			s_WootingContext->UnregisterWootingDevice(deviceInfo);
			break;
		}
		}
	}

}
