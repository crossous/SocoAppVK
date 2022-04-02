#pragma once
#include <string>
#include <map>

//class Device;

class SystemInfo
{
public:
	static bool IsVulkanDeviceSupport(const std::string& extension)
	{
		return mVkExtension.contains(extension);
	}

private:

	friend class Device;
	static std::map<std::string, bool> mVkExtension;
};

