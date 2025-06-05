#include "PSO.h"
#include "dxUtil.hpp"

PSO::PSO(){}

void PSO::Init(VkDevice device, const Shader& shader, const Mesh& mesh, const VkExtent2D& viewport2D, const VkRenderPass renderPass, uint32_t subPassIndex)
{
	mDevice = device;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	SetupShaderStageAndPipelineLayout(pipelineInfo, shader);
	SetupRenderPass(pipelineInfo, renderPass, subPassIndex);

	SetupInputLayout(pipelineInfo, shader, mesh);
	SetupViewport(pipelineInfo, viewport2D);
	SetupRasterizerState(pipelineInfo);
	SetupMultisamplingState(pipelineInfo);
	SetupDepthStencilState(pipelineInfo);
	SetupBlendState(pipelineInfo);
	SetupDynamicState(pipelineInfo);

	ThrowIfFailed(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline));
}

void PSO::Clear()
{
	vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);
}

void PSO::SetupShaderStageAndPipelineLayout(VkGraphicsPipelineCreateInfo& pipelineInfo, const Shader& shader)
{
	shader.SetupPipelineShaderStageInfo(pipelineInfo);
	shader.SetupPipelineLayout(pipelineInfo);
}

void PSO::SetupRenderPass(VkGraphicsPipelineCreateInfo& pipelineInfo, const VkRenderPass renderPass, uint32_t subPassIndex)
{
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = subPassIndex;
}

void PSO::SetupInputLayout(VkGraphicsPipelineCreateInfo& pipelineInfo, const Shader& shader, const Mesh& mesh)
{
	const std::vector<Shader::InputVariable>& shaderInputVariables = shader.GetInputVariables();
	mVertexAttributes.resize(shaderInputVariables.size());

	for (int i = 0; i < shaderInputVariables.size(); ++i)
	{
		auto [semantic, location] = shaderInputVariables[i];
		VkVertexInputAttributeDescription& vertexAttribute = mVertexAttributes[i];
		vertexAttribute.location = location;

		auto attri = mesh.GetVertexAttribute(semantic);
		if (attri.has_value())
		{
			vertexAttribute.binding = attri->binding;
			vertexAttribute.format = attri->format;
			vertexAttribute.offset = attri->offset;
		}
		else
		{
			vertexAttribute.binding = 0;
			vertexAttribute.format = VK_FORMAT_UNDEFINED;
			vertexAttribute.offset = 0;
		}
	}

	mVertexInputBindings.resize(mesh.GetBindingCount());

	for (int i = 0; i < mVertexInputBindings.size(); ++i)
	{
		VkVertexInputBindingDescription& vertexInputBinding = mVertexInputBindings[i];

		vertexInputBinding.binding = i;
		vertexInputBinding.stride = mesh.GetBindingStride(i);
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	}

	mVertexInputInfo = {};
	mVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	mVertexInputInfo.vertexBindingDescriptionCount = mVertexInputBindings.size();
	mVertexInputInfo.pVertexBindingDescriptions = mVertexInputBindings.data();
	mVertexInputInfo.vertexAttributeDescriptionCount = mVertexAttributes.size();
	mVertexInputInfo.pVertexAttributeDescriptions = mVertexAttributes.data();

	mInputAssembly = {};
	mInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	mInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	mInputAssembly.primitiveRestartEnable = VK_FALSE;

	pipelineInfo.pVertexInputState = &mVertexInputInfo;
	pipelineInfo.pInputAssemblyState = &mInputAssembly;
}

void PSO::SetupViewport(VkGraphicsPipelineCreateInfo& pipelineInfo, const VkExtent2D& viewport2D)
{
	mViewport = {};
	mViewport.x = 0.0f;
	mViewport.y = 0.0f;
	mViewport.width = (float)viewport2D.width;
	mViewport.height = (float)viewport2D.height;
	mViewport.minDepth = 0.0f;
	mViewport.maxDepth = 1.0f;

	mScissor = {};
	mScissor.offset = { 0, 0 };
	mScissor.extent = viewport2D;

	mViewportState = {};
	mViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	mViewportState.viewportCount = 1;
	mViewportState.pViewports = &mViewport;
	mViewportState.scissorCount = 1;
	mViewportState.pScissors = &mScissor;

	pipelineInfo.pViewportState = &mViewportState;
}

void PSO::SetupRasterizerState(VkGraphicsPipelineCreateInfo& pipelineInfo)
{
	mRasterizer = {};
	mRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	mRasterizer.depthClampEnable = VK_FALSE;
	mRasterizer.rasterizerDiscardEnable = VK_FALSE;
	mRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	mRasterizer.lineWidth = 1.0f;
	mRasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	mRasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	mRasterizer.depthBiasEnable = VK_FALSE;
	mRasterizer.depthBiasConstantFactor = 0.0f; // Optional
	mRasterizer.depthBiasClamp = 0.0f; // Optional
	mRasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	pipelineInfo.pRasterizationState = &mRasterizer;
}

void PSO::SetupMultisamplingState(VkGraphicsPipelineCreateInfo& pipelineInfo)
{
	mMultisampling = {};
	mMultisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	mMultisampling.sampleShadingEnable = VK_FALSE;
	mMultisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	mMultisampling.minSampleShading = 1.0f; // Optional
	mMultisampling.pSampleMask = nullptr; // Optional
	mMultisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	mMultisampling.alphaToOneEnable = VK_FALSE; // Optional

	pipelineInfo.pMultisampleState = &mMultisampling;
}

void PSO::SetupDepthStencilState(VkGraphicsPipelineCreateInfo& pipelineInfo)
{
	mDepthStencil = {};

	mDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	mDepthStencil.depthTestEnable = VK_TRUE;
	mDepthStencil.depthWriteEnable = VK_TRUE;
	mDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	mDepthStencil.depthBoundsTestEnable = VK_FALSE;
	mDepthStencil.minDepthBounds = 0.0f; // Optional
	mDepthStencil.maxDepthBounds = 1.0f; // Optional
	mDepthStencil.stencilTestEnable = VK_FALSE;
	mDepthStencil.front = {}; // Optional
	mDepthStencil.back = {}; // Optional

	pipelineInfo.pDepthStencilState = &mDepthStencil;
}

void PSO::SetupBlendState(VkGraphicsPipelineCreateInfo& pipelineInfo)
{
	mColorBlendAttachment = {};
	mColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	mColorBlendAttachment.blendEnable = VK_FALSE;
	mColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	mColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	mColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	mColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	mColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	mColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	mColorBlending = {};
	mColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	mColorBlending.logicOpEnable = VK_FALSE;
	mColorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	mColorBlending.attachmentCount = 1;
	mColorBlending.pAttachments = &mColorBlendAttachment;
	mColorBlending.blendConstants[0] = 0.0f; // Optional
	mColorBlending.blendConstants[1] = 0.0f; // Optional
	mColorBlending.blendConstants[2] = 0.0f; // Optional
	mColorBlending.blendConstants[3] = 0.0f; // Optional

	pipelineInfo.pColorBlendState = &mColorBlending;
}

void PSO::SetupDynamicState(VkGraphicsPipelineCreateInfo& pipelineInfo)
{
	mDynamicState = {};
	mDynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	mDynamicState.dynamicStateCount = 2;
	mDynamicState.pDynamicStates = mDynamicStates;

	pipelineInfo.pDynamicState = &mDynamicState;
}