#pragma once

#include "Device.hpp"

class DeviceComponent
{
protected:
	Device* mDevice;

	DeviceComponent(Device* device): mDevice(device)
	{ }
};