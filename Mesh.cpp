#include "Mesh.h"

#include <algorithm>

#include <cassert>

bool VertexAttributeDesc::operator==(const VertexAttributeDesc& rhs) const
{
	if (semantic == rhs.semantic)
		return true;
}

bool VertexAttributeDesc::operator<(const VertexAttributeDesc& rhs) const
{
	return semantic < rhs.semantic;
}

std::unique_ptr<Mesh> Mesh::CreateTriangle(Device* device)
{
	std::unique_ptr<Mesh> res(new Mesh(device));

	//position, color
	float vertices[18] = {
		0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f
	};

	res->mVertexDatas.resize(1);
	res->mVertexDatas[0].resize(sizeof(vertices));
	memcpy(res->mVertexDatas[0].data(), vertices, sizeof(vertices));
	res->mVertexCount = 3;

	//indices
	uint32_t indices[3] = { 0, 1, 2 };
	res->mIndices32.resize(_countof(indices));
	memcpy(res->mIndices32.data(), indices, sizeof(indices));

	res->mIndices16.resize(res->mIndices32.size());
	for (int i = 0; i < res->mIndices32.size(); ++i)
	{
		res->mIndices16[i] = static_cast<uint16_t>(res->mIndices32[i]);
	}

	res->bIndex32 = false;

	res->mAttributes.insert({ "POSITION", VK_FORMAT_R32G32B32_SFLOAT, 0, 0});
	res->mAttributes.insert({ "COLOR", VK_FORMAT_R32G32B32_SFLOAT, 12, 0});

	SubmeshGeometry submesh;
	submesh.IndexCount = 3;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	res->mSubmeshes.push_back(submesh);

	res->BuildBuffer();

	return res;
}

std::optional<VertexAttributeDesc> Mesh::GetVertexAttribute(const std::string semantic) const
{
	auto ite = std::find_if(mAttributes.cbegin(), mAttributes.cend(), [&semantic](const VertexAttributeDesc& attribute) {
		return attribute.semantic == semantic;
	});

	if (ite != mAttributes.cend())
	{
		return std::optional(*ite);
	}

	return std::nullopt;
}

uint32_t Mesh::GetBindingStride(uint32_t binding) const
{
	if (binding < mVertexDatas.size())
	{
		return mVertexDatas[binding].size() / mVertexCount;
	}

	return 0;
}

void Mesh::BuildBuffer()
{
	std::set<uint32_t> queueFamilyIndices = { mDevice->GetGraphicsQueue().index, mDevice->GetTransferQueue().index };
	std::vector<uint32_t> queueFamilyIndicesUnique(queueFamilyIndices.begin(), queueFamilyIndices.end());

	//Build VertexBuffer
	std::vector<VkBuffer> uploadBuffers(mVertexDatas.size() + 1);//1 is indexBuffer
	mVertexBuffers.resize(mVertexDatas.size());

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.sharingMode = queueFamilyIndicesUnique.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	bufferInfo.queueFamilyIndexCount = queueFamilyIndicesUnique.size();
	bufferInfo.pQueueFamilyIndices = queueFamilyIndicesUnique.data();

	for (size_t i = 0; i < mVertexBuffers.size(); ++i)
	{
		bufferInfo.size = mVertexDatas[i].size();

		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		ThrowIfFailed(vkCreateBuffer(mDevice->GetDevice(), &bufferInfo, nullptr, &uploadBuffers[i]));

		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		ThrowIfFailed(vkCreateBuffer(mDevice->GetDevice(), &bufferInfo, nullptr, &mVertexBuffers[i]));
	}
	//Build IndexBuffer
	size_t indexBufferOffset = mVertexBuffers.size();
	VkDeviceSize indexBufferSize = bIndex32 ? (mIndices32.size() * sizeof(uint32_t)) : (mIndices16.size() * sizeof(uint16_t));

	bufferInfo.size = indexBufferSize;

	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	ThrowIfFailed(vkCreateBuffer(mDevice->GetDevice(), &bufferInfo, nullptr, &(uploadBuffers[indexBufferOffset])));
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	ThrowIfFailed(vkCreateBuffer(mDevice->GetDevice(), &bufferInfo, nullptr, &mIndexBuffer));

	//Allocate Memory
	VkDeviceMemory uploadMemory;

	std::vector<uint32_t> memSizeVector(mVertexBuffers.size() + 1);
	uint32_t totalMemorySize = 0;
	uint32_t memoryTypeBits = 0;

	for (size_t i = 0; i < mVertexBuffers.size(); ++i)
	{
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mDevice->GetDevice(), mVertexBuffers[i], &memRequirements);

		totalMemorySize += memRequirements.size;
		memSizeVector[i] = memRequirements.size;
		memoryTypeBits |= memRequirements.memoryTypeBits;
	}

	{
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mDevice->GetDevice(), mIndexBuffer, &memRequirements);

		totalMemorySize += memRequirements.size;
		memSizeVector[indexBufferOffset] = memRequirements.size;
		memoryTypeBits |= memRequirements.memoryTypeBits;
	}

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	allocInfo.allocationSize = totalMemorySize;
	allocInfo.memoryTypeIndex = mDevice->FindMemoryType(memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	ThrowIfFailed(vkAllocateMemory(mDevice->GetDevice(), &allocInfo, nullptr, &mDeviceMemory));

	std::vector<uint32_t> memSizeVectorUploader(mVertexBuffers.size() + 1);
	uint32_t totalMemorySizeUploader = 0;
	uint32_t memoryTypeBitsUploader = 0;

	for (size_t i = 0; i < uploadBuffers.size(); ++i)
	{
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mDevice->GetDevice(), uploadBuffers[i], &memRequirements);

		totalMemorySizeUploader += memRequirements.size;
		memSizeVectorUploader[i] = memRequirements.size;
		memoryTypeBitsUploader |= memRequirements.memoryTypeBits;
	}

	allocInfo.allocationSize = totalMemorySizeUploader;
	allocInfo.memoryTypeIndex = mDevice->FindMemoryType(memoryTypeBitsUploader, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	ThrowIfFailed(vkAllocateMemory(mDevice->GetDevice(), &allocInfo, nullptr, &uploadMemory));

	//Bind Buffer Memory
	uint32_t memOffset = 0;
	uint32_t memOffsetUploader = 0;
	for (size_t i = 0; i < mVertexBuffers.size(); ++i)
	{
		ThrowIfFailed(vkBindBufferMemory(mDevice->GetDevice(), uploadBuffers[i], uploadMemory, memOffsetUploader));
		ThrowIfFailed(vkBindBufferMemory(mDevice->GetDevice(), mVertexBuffers[i], mDeviceMemory, memOffset));
		memOffset += memSizeVector[i];
		memOffsetUploader += memSizeVectorUploader[i];
	}
	ThrowIfFailed(vkBindBufferMemory(mDevice->GetDevice(), uploadBuffers[indexBufferOffset], uploadMemory, memOffsetUploader));
	ThrowIfFailed(vkBindBufferMemory(mDevice->GetDevice(), mIndexBuffer, mDeviceMemory, memOffset));

	//Upload Buffer
	{
		void* data;
		ThrowIfFailed(vkMapMemory(mDevice->GetDevice(), uploadMemory, 0, VK_WHOLE_SIZE, 0, &data));

		memOffsetUploader = 0;
		for (size_t i = 0; i < mVertexDatas.size(); ++i)
		{
			memcpy((void*)((BYTE*)data + memOffsetUploader), mVertexDatas[i].data(), mVertexDatas[i].size());
			memOffsetUploader += memSizeVector[i];
		}

		if (bIndex32)
			memcpy((void*)((BYTE*)data + memOffsetUploader), mIndices32.data(), indexBufferSize);
		else
			memcpy((void*)((BYTE*)data + memOffsetUploader), mIndices16.data(), indexBufferSize);

		vkUnmapMemory(mDevice->GetDevice(), uploadMemory);
	}

	//Copy Buffer
	{
		VkCommandBufferAllocateInfo cmdAllocInfo = {};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdAllocInfo.commandPool = mDevice->GetTransferCommandPool();
		cmdAllocInfo.commandBufferCount = 1;

		VkCommandBuffer transferCmd;
		vkAllocateCommandBuffers(mDevice->GetDevice(), &cmdAllocInfo, &transferCmd);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(transferCmd, &beginInfo);
		{
			VkBufferCopy copyRegion = {};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;

			for (size_t i = 0; i < mVertexBuffers.size(); ++i)
			{
				copyRegion.size = mVertexDatas[i].size();

				vkCmdCopyBuffer(transferCmd, uploadBuffers[i], mVertexBuffers[i], 1, &copyRegion);
			}

			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(transferCmd, uploadBuffers[indexBufferOffset], mIndexBuffer, 1, &copyRegion);
		}
		vkEndCommandBuffer(transferCmd);

		{
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferCmd;

			vkQueueSubmit(mDevice->GetTransferQueue().queue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(mDevice->GetTransferQueue().queue);
		}

		vkFreeCommandBuffers(mDevice->GetDevice(), mDevice->GetTransferCommandPool(), 1, &transferCmd);
	}

	//Release Upload Resource
	for (const VkBuffer buffer : uploadBuffers)
	{
		vkDestroyBuffer(mDevice->GetDevice(), buffer, nullptr);
	}
	vkFreeMemory(mDevice->GetDevice(), uploadMemory, nullptr);
	
	//SetOffset
	mVertexBufferOffsets.resize(mVertexDatas.size());
	for (size_t i = 0; i < mVertexBufferOffsets.size(); ++i)
	{
		mVertexBufferOffsets[i] = static_cast<VkDeviceSize>(0);
	}
}

void Mesh::ReleaseBuffer()
{
	vkFreeMemory(mDevice->GetDevice(), mDeviceMemory, nullptr);

	for (const VkBuffer& buffer : mVertexBuffers)
		vkDestroyBuffer(mDevice->GetDevice(), buffer, nullptr);

	vkDestroyBuffer(mDevice->GetDevice(), mIndexBuffer, nullptr);
}

std::unique_ptr<FormatMesh> FormatMesh::CreateTriangle(Device* device)
{
	std::unique_ptr<FormatMesh> res(new FormatMesh(device));

	Vertex vertices[3] =
	{
		{{0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.0f}, {0.5f, 0.0f}, {0.0f, 0.0f}, {1.0, 0.0, 0.0}},
		{{0.5f, 0.5f, 0.0f,}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, {0.0, 1.0, 0.0}},
		{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {0.0, 0.0, 1.0}},
	};

	res->mVertexDatas.resize(1);
	res->mVertexDatas[0].resize(sizeof(vertices));

	memcpy(res->mVertexDatas[0].data(), vertices, sizeof(vertices));
	res->mVertexCount = 3;

	//indices
	uint32_t indices[3] = { 0, 1, 2 };
	res->mIndices32.resize(_countof(indices));
	memcpy(res->mIndices32.data(), indices, sizeof(indices));

	res->mIndices16.resize(res->mIndices32.size());
	for (int i = 0; i < res->mIndices32.size(); ++i)
	{
		res->mIndices16[i] = static_cast<uint16_t>(res->mIndices32[i]);
	}

	res->bIndex32 = false;

	res->mAttributes.insert({ "POSITION", VK_FORMAT_R32G32B32_SFLOAT, 0, 0 });
	res->mAttributes.insert({ "NORMAL", VK_FORMAT_R32G32B32_SFLOAT, 12, 0 });
	res->mAttributes.insert({ "TANGENT", VK_FORMAT_R32G32B32A32_SFLOAT, 24, 0 });
	res->mAttributes.insert({ "TEXCOORD", VK_FORMAT_R32G32_SFLOAT, 40, 0 });
	res->mAttributes.insert({ "TEXCOORD1", VK_FORMAT_R32G32_SFLOAT, 48, 0 });
	res->mAttributes.insert({ "COLOR", VK_FORMAT_R32G32B32_SFLOAT, 56, 0 });

	SubmeshGeometry submesh;
	submesh.IndexCount = 3;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	res->mSubmeshes.push_back(submesh);

	res->BuildBuffer();

	return res;
}

Vertex& FormatMesh::GetVertex(int i)
{
	assert(sizeof(Vertex) * i < mVertexDatas[0].size());
	return *(static_cast<Vertex*>(static_cast<void*>(mVertexDatas[0].data())) + i);
}