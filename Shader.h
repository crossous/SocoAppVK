#pragma once
#include "DeviceComponent.h"
#include "Mesh.h"

#include <string>
#include <memory>
#include <windows.h>
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>


enum class ShaderStage: int
{
	VS,
	PS,
	DS,
	HS,
	GS
};

struct ShaderEntry
{
	LPCWSTR vs = nullptr;
	LPCWSTR ps = nullptr;
	LPCWSTR ds = nullptr;
	LPCWSTR hs = nullptr;
	LPCWSTR gs = nullptr;
};

class Shader : public DeviceComponent
{
public:

	static std::unique_ptr<Shader> LoadFromFile(Device* device, const std::wstring filename, ShaderEntry& entries);

	//void SetupInputLayout(VkGraphicsPipelineCreateInfo& pipelineInfo, const Mesh& mesh) const;
	using InputVariable = std::pair<std::string, uint32_t>;
	const std::vector<InputVariable> GetInputVariables() const;
	void SetupPipelineShaderStageInfo(VkGraphicsPipelineCreateInfo& pipelineInfo) const;
	void SetupPipelineLayout(VkGraphicsPipelineCreateInfo& pipelineInfo) const;

	static bool IsPipelineLayoutEqual(const Shader& a, const Shader& b);

	using BindingPoint = std::pair<uint32_t, uint32_t>;
	BindingPoint GetBindingPoint(const std::string bufferName) const;

	VkPipelineLayout GetPipelineLayout() const { return mPipelineLayout; }
	const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayout() const { return mSetLayouts; }

	~Shader();
private:

	SpvReflectShaderModule mVertexReflectShaderModule;
	SpvReflectShaderModule mPixelReflectShaderModule;
	SpvReflectShaderModule mDomainReflectShaderModule;
	SpvReflectShaderModule mHullReflectShaderModule;
	SpvReflectShaderModule mGeometryReflectShaderModule;

	std::vector<VkPipelineShaderStageCreateInfo> mStageContainer;
	std::vector<DescriptorSetLayoutDesc> mSetLayoutsDesc;

	std::vector<InputVariable> mInputVariables;

	VkPipelineLayout mPipelineLayout;
	std::vector<VkDescriptorSetLayout> mSetLayouts;

	Shader(Device* device);
	void CreatePipelineLayout();

	static VkShaderModule CreateShaderModule(VkDevice device, const void* codebytes, size_t size);
};