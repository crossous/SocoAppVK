#pragma once
#include "Transform.hpp"
#include "ConstantBuffer.hpp"
#include "DeviceComponent.h"

class Camera : public DeviceComponent
{
public:

	Camera(Device* device) : DeviceComponent(device), mPerCameraBuffer(device){}
	~Camera() { ClearBuffer(); }

	inline glm::mat4 GetViewMatrix() const
	{
		glm::vec3 positionWS = mTransform.GetGlobalPosition();
		glm::vec3 forwardWS = mTransform.GetGlobalForward();
		return glm::lookAtLH(positionWS, positionWS + forwardWS, glm::vec3(0, 1, 0));
	}

	inline glm::mat4 GetProjMatrix() const
	{
		glm::mat4 proj = glm::perspectiveLH(mFov, mAspect, mNear, mFar);
		proj[1][1] *= -1;
		return proj;
	}

	inline glm::mat4 GetVPMatrix() const
	{
		return GetProjMatrix() * GetViewMatrix();
	}

	inline Transform* GetTransform()
	{
		return &mTransform;
	}

	inline float GetFov() const { return mFov; }
	inline void SetFov(float fov) { mFov = fov; }
	inline float GetNear() const { return mNear; }
	inline void SetNear(float nearPlane) { mNear = nearPlane; }
	inline float GetFar() const { return mFar; }
	inline void SetFar(float farPlane) { mFar = farPlane; }
	inline float GetAspect() const { return mAspect; }
	inline void SetAspect(float aspect) { mAspect = aspect; }

	void BuildBuffer()
	{
		//VkBufferCreateInfo bufferInfo
		//{
		//	.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		//	.size = sizeof(PerCamera),
		//	.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		//	.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		//};

		//ThrowIfFailed(vkCreateBuffer(mDevice->GetDevice(), &bufferInfo, nullptr, &mPerCameraBuffer));

		//VkMemoryRequirements memRequirements;
		//vkGetBufferMemoryRequirements(mDevice->GetDevice(), mPerCameraBuffer, &memRequirements);

		//VkMemoryAllocateInfo allocInfo
		//{
		//	.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		//	.allocationSize = memRequirements.size,
		//	.memoryTypeIndex = mDevice->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		//};

		//ThrowIfFailed(vkAllocateMemory(mDevice->GetDevice(), &allocInfo, nullptr, &mPerCameraBufferMemory));
		//ThrowIfFailed(vkBindBufferMemory(mDevice->GetDevice(), mPerCameraBuffer, mPerCameraBufferMemory, 0));
		mPerCameraBuffer.Init(sizeof(PerCamera));
	}

	void ClearBuffer()
	{
		//if (mPerCameraBuffer != VK_NULL_HANDLE)
		//	vkDestroyBuffer(mDevice->GetDevice(), mPerCameraBuffer, nullptr);
		//if (mPerCameraBufferMemory != VK_NULL_HANDLE)
		//	vkFreeMemory(mDevice->GetDevice(), mPerCameraBufferMemory, nullptr);

		//mPerCameraBuffer = VK_NULL_HANDLE;
		//mPerCameraBufferMemory = VK_NULL_HANDLE;
		mPerCameraBuffer.ClearBuffer();
	}

	void UpdateBuffer()
	{

		//void* data;
		//ThrowIfFailed(vkMapMemory(mDevice->GetDevice(), mPerCameraBufferMemory, 0, sizeof(PerCamera), 0, &data));

		//memcpy(data, &mPerCameraData, sizeof(PerCamera));

		//vkUnmapMemory(mDevice->GetDevice(), mPerCameraBufferMemory);
		mPerCameraData.WorldToClipMatrix = GetVPMatrix();
		mPerCameraBuffer.UpdateBuffer(&mPerCameraData);
	}

	VkDescriptorBufferInfo GetPerCameraBufferInfo()
	{
		//VkDescriptorBufferInfo bufferInfo{ .buffer{mPerCameraBuffer}, .offset{0}, .range{sizeof(PerCamera)} };

		//return bufferInfo;
		return mPerCameraBuffer.GetBufferInfo();
	}

	ConstantBuffer* GetPerCameraBuffer()
	{
		return &mPerCameraBuffer;
	}

private:

	struct PerCamera
	{
		glm::mat4 WorldToClipMatrix;
	};

	Transform mTransform;

	float mFov = glm::radians(90.0f);
	float mNear = 0.25f;
	float mFar = 1000.0f;
	float mAspect = 800.0f / 600.0f;

	PerCamera mPerCameraData;
	//VkBuffer mPerCameraBuffer = VK_NULL_HANDLE;
	//VkDeviceMemory mPerCameraBufferMemory = VK_NULL_HANDLE;
	ConstantBuffer mPerCameraBuffer;
};