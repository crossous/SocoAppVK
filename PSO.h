#pragma once

#include <vulkan/vulkan.h>

#include "Shader.h"

class PSO
{
public:
	PSO();

	void Init(VkDevice device, const Shader& shader, const Mesh& mesh, const VkExtent2D& viewport2D, const VkRenderPass renderPass, uint32_t subPassIndex);
	void Clear();

	VkPipeline GetPipeline() { return mGraphicsPipeline; }
	VkViewport& GetViewport() { return mViewport; }

private:
	void SetupShaderStageAndPipelineLayout(VkGraphicsPipelineCreateInfo& pipelineInfo, const Shader& shader);
	void SetupRenderPass(VkGraphicsPipelineCreateInfo& pipelineInfo, const VkRenderPass renderPass, uint32_t subPassIndex);
	void SetupInputLayout(VkGraphicsPipelineCreateInfo& pipelineInfo, const Shader& shader,  const Mesh& mesh);
	void SetupViewport(VkGraphicsPipelineCreateInfo& pipelineInfo, const VkExtent2D& viewport2D);
	void SetupRasterizerState(VkGraphicsPipelineCreateInfo& pipelineInfo);
	void SetupMultisamplingState(VkGraphicsPipelineCreateInfo& pipelineInfo);
	void SetupDepthStencilState(VkGraphicsPipelineCreateInfo& pipelineInfo);
	void SetupBlendState(VkGraphicsPipelineCreateInfo& pipelineInfo);
	void SetupDynamicState(VkGraphicsPipelineCreateInfo& pipelineInfo);

	VkDevice mDevice;
	VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;

	const VkDynamicState mDynamicStates[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineVertexInputStateCreateInfo mVertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo mInputAssembly;
	std::vector<VkVertexInputBindingDescription> mVertexInputBindings;
	std::vector<VkVertexInputAttributeDescription> mVertexAttributes;
	VkViewport mViewport;
	VkRect2D mScissor;
	VkPipelineViewportStateCreateInfo mViewportState;
	VkPipelineRasterizationStateCreateInfo mRasterizer;
	VkPipelineMultisampleStateCreateInfo mMultisampling;
	VkPipelineDepthStencilStateCreateInfo mDepthStencil;
	VkPipelineColorBlendAttachmentState mColorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo mColorBlending;
	VkPipelineDynamicStateCreateInfo mDynamicState;
};