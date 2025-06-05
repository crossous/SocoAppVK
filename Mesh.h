#pragma once

#include <memory>
#include <vector>
#include <utility>
#include <optional>
#include <vulkan/vulkan.h>
#include <string>
#include <set>

#include "DeviceComponent.h"

struct VertexAttributeDesc
{
	std::string semantic;
	VkFormat format;
	uint32_t offset; //In Bytes
	uint32_t binding;

	bool operator==(const VertexAttributeDesc&) const;
	bool operator<(const VertexAttributeDesc&) const;
};

//struct MeshDuplicate
//{
//	std::set<MeshAttributeDesc> attributes;
//	std::vector<std::byte> vertexDatas;
//
//	bool IsInclude(std::vector<MeshAttributeDesc> otherAttributes);
//};



//Mesh只提供Vertex Attribute在顶点数据中的偏移，不保证精度和Component数量
class Mesh : public DeviceComponent
{
public:
	using FormatOffsetPair = std::pair<VkFormat, uint32_t>;

	struct SubmeshGeometry
	{
		uint32_t IndexCount;
		uint32_t StartIndexLocation;
		uint32_t BaseVertexLocation;
	};

	static std::unique_ptr<Mesh> CreateTriangle(Device* device);
	std::optional<VertexAttributeDesc> GetVertexAttribute(const std::string semantic) const;
	uint32_t GetBindingCount() const { return mVertexDatas.size(); }
	uint32_t GetBindingStride(uint32_t binding) const;
	const VkBuffer* GetVertexBuffers() const { return mVertexBuffers.data(); }
	const VkBuffer GetIndexBuffer() const { return mIndexBuffer; }
	const std::vector<SubmeshGeometry>& GetSubmesh() { return mSubmeshes; }
	const VkDeviceSize* GetOffsets() const { return mVertexBufferOffsets.data(); }
	VkIndexType GetIndexType() const { return bIndex32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16; }
	~Mesh() { ReleaseBuffer(); }

protected:
	Mesh(Device* device) : DeviceComponent(device) {}
	
	void BuildBuffer();
	void ReleaseBuffer();

	uint32_t mVertexCount = 0;
	bool bIndex32 = false;

	std::set<VertexAttributeDesc> mAttributes;
	std::vector<std::vector<std::byte>> mVertexDatas;
	std::vector<uint16_t> mIndices16;
	std::vector<uint32_t> mIndices32;

	std::vector<SubmeshGeometry> mSubmeshes;

	std::vector<VkBuffer> mVertexBuffers;
	std::vector<VkDeviceSize> mVertexBufferOffsets;
	VkBuffer mIndexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mDeviceMemory = VK_NULL_HANDLE;
};

struct Vertex
{
	float position[3];
	float normal[3];
	float tangent[4];
	float uv0[2];
	float uv1[2];
	float color[3];
};

//特化的网格体
class FormatMesh : public Mesh
{
public:
	static std::unique_ptr<FormatMesh> CreateTriangle(Device* device);
	static std::unique_ptr<FormatMesh> CreatePlane(Device* device);
	Vertex& GetVertex(int i);
private:
	FormatMesh(Device* device) : Mesh(device) {}
};