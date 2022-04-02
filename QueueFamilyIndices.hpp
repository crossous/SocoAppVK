#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;
	int transferFamily = -1;

	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0 && transferFamily >= 0;
	}

	bool OnlyQueueFamily()
	{
		return FamilyCount() == 1;
	}

	uint32_t FamilyCount()
	{
		std::set<uint32_t> allFamily = AllFamily();

		return static_cast<uint32_t>(allFamily.size());
	}

	std::set<uint32_t> AllFamily() {
		return { static_cast<uint32_t>(graphicsFamily), static_cast<uint32_t>(presentFamily), static_cast<uint32_t>(transferFamily) };
	}

	std::vector<uint32_t> AllFamilyVector() {
		std::set<uint32_t> allFamily = AllFamily();
		return std::vector<uint32_t>(allFamily.begin(), allFamily.end());
	}

	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0)
			{
				if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indices.graphicsFamily = i;
				if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
					indices.transferFamily = i;

				VkBool32 presentSupport = false;
				if (vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport) == VK_SUCCESS && presentSupport)
					indices.presentFamily = i;
			}

			if (indices.isComplete())
				break;

			i++;
		}

		return indices;
	}
};