#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <set>
#include <vector>

#include <iostream>
#include <algorithm>

#include "SystemInfo.h"

#include "SwapChainSupportDetails.hpp"
#include "QueueFamilyIndices.hpp"

#include "SamplerPool.hpp"
#include "PipelineLayoutPool.hpp"

class Device
{
public:
	Device() = default;

	void Init(VkInstance instance, VkSurfaceKHR surface,
		std::vector<const char*> deviceExtensions, 
		std::vector<const char*> queryDeviceExtensions, 
		std::vector<const char*> validationLayers,
		VkPhysicalDeviceFeatures deviceFeatures)
	{
		mInstance = instance;
		mSurface = surface;
		mDeviceExtensions = deviceExtensions;
		mQueryDeviceExtensions = queryDeviceExtensions;
		mValidationLayers = validationLayers;
		mDeviceFeatures = deviceFeatures;

		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPool();

		mSamplerPool.Init(mPhysicalDevice, mDevice);
		mPipelineLayoutPool.Init(mDevice, &mSamplerPool);
	}

	void Destroy()
	{
		vkDestroyCommandPool(mDevice, mGraphicsCommandPool, nullptr);
		vkDestroyCommandPool(mDevice, mTransferCommandPool, nullptr);
		mSamplerPool.Clear();
		mPipelineLayoutPool.Clear();
		vkDestroyDevice(mDevice, nullptr);
	}

	VkDevice GetDevice() const
	{
		return mDevice;
	}

	VkPhysicalDevice GetPhysicalDevice() const
	{
		return mPhysicalDevice;
	}

	SamplerPool* GetSamplerPool()
	{
		return &mSamplerPool;
	}

	PipelineLayoutPool* GetPipelineLayoutPool()
	{
		return &mPipelineLayoutPool;
	}

	struct QueueIndexPair
	{
		uint32_t index;
		VkQueue queue;
	};

	QueueIndexPair GetGraphicsQueue() const
	{
		return mGraphicsQueue;
	}

	QueueIndexPair GetPresentQueue() const
	{
		return mPresentQueue;
	}

	QueueIndexPair GetTransferQueue() const
	{
		return mTransferQueue;
	}

	VkCommandPool GetGraphicsCommandPool() const
	{
		return mGraphicsCommandPool;
	}

	VkCommandPool GetTransferCommandPool() const
	{
		return mTransferCommandPool;
	}

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

private:
	VkInstance mInstance = VK_NULL_HANDLE;
	VkSurfaceKHR mSurface = VK_NULL_HANDLE;

	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;

	std::vector<const char*> mDeviceExtensions;
	std::vector<const char*> mQueryDeviceExtensions;
	std::vector<const char*> mValidationLayers;
	VkPhysicalDeviceFeatures mDeviceFeatures;

	QueueIndexPair mGraphicsQueue;
	QueueIndexPair mPresentQueue;
	QueueIndexPair mTransferQueue;

	VkCommandPool mGraphicsCommandPool;
	VkCommandPool mTransferCommandPool;

	SamplerPool mSamplerPool;
	PipelineLayoutPool mPipelineLayoutPool;

#ifdef NDEBUG
	const bool mEnableValidationLayers = false;
#else
	const bool mEnableValidationLayers = true;
#endif

	bool CheckDeviceExtensionSupport(VkPhysicalDevice device, int* queryExtensionCount)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(mDeviceExtensions.begin(), mDeviceExtensions.end());
		std::set<std::string> queryExtensions(mQueryDeviceExtensions.begin(), mQueryDeviceExtensions.end());

		for (const VkExtensionProperties& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
			queryExtensions.erase(extension.extensionName);
		}

		static bool print = false;
		if (!print)
		{
			print = true;

			std::cout << "available extension(" << availableExtensions.size() << "):" << std::endl;
			for (const auto& extension : availableExtensions)
			{
				std::cout << "\t" << extension.extensionName << std::endl;
			}
		}
		if (queryExtensionCount != nullptr)
			*queryExtensionCount = mQueryDeviceExtensions.size() - queryExtensions.size();

		return requiredExtensions.empty();
	}

	void PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);

		if (deviceCount == 0)
			throw std::runtime_error("failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

		auto IsDeviceSuitable = [this](VkPhysicalDevice device, int* queryExtensionCount)->bool {
			QueueFamilyIndices queueIndices = QueueFamilyIndices::FindQueueFamilies(device, mSurface);

			bool extensionsSupported = CheckDeviceExtensionSupport(device, queryExtensionCount);

			SwapChainSupportDetails swapChainSupport = SwapChainSupportDetails::QuerySwapChainSupport(device, mSurface);
			bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

			return queueIndices.isComplete() && extensionsSupported && swapChainAdequate;
		};

		int maxQueryExtensionCount = -1;
		int currentQueryExtensionCount;

		for (const VkPhysicalDevice& device : devices) {
			if (IsDeviceSuitable(device, &currentQueryExtensionCount) && currentQueryExtensionCount > maxQueryExtensionCount) {
				mPhysicalDevice = device;
				maxQueryExtensionCount = currentQueryExtensionCount;
			}
		}

		//Register extension to SystemInfo
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &extensionCount, availableExtensions.data());

		for (const std::string& extensionName : mQueryDeviceExtensions)
		{
			if (std::find_if(availableExtensions.cbegin(), availableExtensions.cend(), 
				[&extensionName](const VkExtensionProperties& extensionProp) {
					return std::string(extensionProp.extensionName) == extensionName;
				})
				!= availableExtensions.cend())
			{
				SystemInfo::mVkExtension[extensionName] = true;
			}
		}

		if (mPhysicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("failed to find a suitable GPU!");
	}

	void CreateLogicalDevice()
	{
		QueueFamilyIndices queueIndices = QueueFamilyIndices::FindQueueFamilies(mPhysicalDevice, mSurface);
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = queueIndices.AllFamily();

		float queuePriority = 1.0f;

		for (int queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

		createInfo.pEnabledFeatures = &mDeviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(mDeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = mDeviceExtensions.data();

		if (mEnableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
			createInfo.ppEnabledLayerNames = mValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}

		mGraphicsQueue.index = queueIndices.graphicsFamily;
		mPresentQueue.index = queueIndices.presentFamily;
		mTransferQueue.index = queueIndices.transferFamily;

		vkGetDeviceQueue(mDevice, queueIndices.graphicsFamily, 0, &mGraphicsQueue.queue);
		vkGetDeviceQueue(mDevice, queueIndices.presentFamily, 0, &mPresentQueue.queue);
		vkGetDeviceQueue(mDevice, queueIndices.transferFamily, 0, &mTransferQueue.queue);
	}

	void CreateCommandPool()
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = mGraphicsQueue.index;
		//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: 提示命令缓冲区非常频繁的重新记录新命令(可能会改变内存分配行为)
		//VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 允许命令缓冲区单独重新记录，没有这个标志，所有的命令缓冲区都必须一起重置
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

		ThrowIfFailed(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mGraphicsCommandPool));

		poolInfo.queueFamilyIndex = mTransferQueue.index;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // Optional

		ThrowIfFailed(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mTransferCommandPool));
	}
};