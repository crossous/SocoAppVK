#pragma once

#include "DeviceComponent.h"

class ConstantBuffer : public DeviceComponent
{
public:
	ConstantBuffer(Device* device) : DeviceComponent(device){}
	~ConstantBuffer() { ClearBuffer(); }

	void Init(uint32_t bufferSize)
	{
		mBufferSize = bufferSize;
		VkBufferCreateInfo bufferInfo
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = bufferSize,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};

		ThrowIfFailed(vkCreateBuffer(mDevice->GetDevice(), &bufferInfo, nullptr, &mHostVisibleBuffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mDevice->GetDevice(), mHostVisibleBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = mDevice->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		};

		ThrowIfFailed(vkAllocateMemory(mDevice->GetDevice(), &allocInfo, nullptr, &mHostVisibleBufferMemory));
		ThrowIfFailed(vkBindBufferMemory(mDevice->GetDevice(), mHostVisibleBuffer, mHostVisibleBufferMemory, 0));
	}

	void UpdateBuffer(void* copyData)
	{
		void* data;
		ThrowIfFailed(vkMapMemory(mDevice->GetDevice(), mHostVisibleBufferMemory, 0, mBufferSize, 0, &data));

		memcpy(data, copyData, mBufferSize);

		vkUnmapMemory(mDevice->GetDevice(), mHostVisibleBufferMemory);
	}

	void ClearBuffer()
	{
		if (mHostVisibleBuffer != VK_NULL_HANDLE)
			vkDestroyBuffer(mDevice->GetDevice(), mHostVisibleBuffer, nullptr);
		if (mHostVisibleBufferMemory != VK_NULL_HANDLE)
			vkFreeMemory(mDevice->GetDevice(), mHostVisibleBufferMemory, nullptr);

		mHostVisibleBuffer = VK_NULL_HANDLE;
		mHostVisibleBufferMemory = VK_NULL_HANDLE;
		mBufferSize = 0;
	}

	VkDescriptorBufferInfo GetBufferInfo()
	{
		VkDescriptorBufferInfo bufferInfo{ .buffer{mHostVisibleBuffer}, .offset{0}, .range{mBufferSize} };

		return bufferInfo;
	}

private:
	uint32_t mBufferSize = 0;
	VkBuffer mHostVisibleBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mHostVisibleBufferMemory = VK_NULL_HANDLE;
};