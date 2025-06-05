#pragma once

#include <vulkan/vulkan.h>

#include "Shader.h"

struct BlendState
{
    VkPipelineColorBlendStateCreateInfo blendState;
    VkPipelineColorBlendAttachmentState attachmentBlendState[8];
};

struct RenderState
{
    VkPipelineRasterizationStateCreateInfo rasterizeState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    BlendState blendState;
};

class Material
{
public:
    Material(Shader* shader, RenderState* renderState = nullptr);

private:
    Shader* mShader;
    
};
