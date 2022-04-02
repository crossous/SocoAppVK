#include "Shader.h"

#include <atlbase.h>        // Common COM helpers.
#include "inc/dxcapi.h"         // Be sure to link with dxcompiler.lib.
#include <d3d12shader.h>    // Shader reflection.
#include "dxUtil.hpp"

#include <vk_format_utils.h>

#include <iostream>
#include <format>

std::unique_ptr<Shader> Shader::LoadFromFile(Device* device, const std::wstring filename, ShaderEntry& entries)
{
	std::unique_ptr<Shader> res(new Shader(device));

	static CComPtr<IDxcUtils> pUtils;
	if (pUtils == nullptr)
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils)));

	//Load Shader
	CComPtr<IDxcBlobEncoding> pSource = nullptr;
	ThrowIfFailed(pUtils->LoadFile(filename.c_str(), nullptr, &pSource));
	if (pSource == nullptr)
		throw std::runtime_error("Load file error");

	DxcBuffer Source;
	Source.Ptr = pSource->GetBufferPointer();
	Source.Size = pSource->GetBufferSize();
	Source.Encoding = DXC_CP_UTF8;

	CComPtr<IDxcIncludeHandler> pIncludeHandler;
	pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);

	//std::vector<std::vector<DescriptorSetLayoutBindingDesc>> layoutBindings;
	//std::vector<std::vector<std::pair<uint32_t, std::vector<VkSampler>>>> tempVkSamplerContainer;
	

	auto InitStage = [&device, filename, &res, &Source, &pIncludeHandler]
		(ShaderStage stage, LPCWSTR entry)
	{
		if (entry == nullptr)
			return;

		auto StageToShaderModel = [](ShaderStage stage) -> LPCWSTR
		{
			switch (stage)
			{
			case ShaderStage::VS:
				return L"vs_6_0";
			case ShaderStage::PS:
				return L"ps_6_0";
			case ShaderStage::DS:
				return L"ds_6_0";
			case ShaderStage::HS:
				return L"hs_6_0";
			case ShaderStage::GS:
				return L"gs_6_0";
			default:
				return L"Error";
			}
		};

		
		LPCWSTR arguments[] = {
			filename.c_str(),
			L"-E", entry,
			L"-T", StageToShaderModel(stage),
			L"-spirv", L"-fspv-reflect"/*, L"-fspv-target-env=vulkan1.2"*/
		};

		static CComPtr<IDxcCompiler3> pCompiler;
		if (pCompiler == nullptr)
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));

		CComPtr<IDxcResult> pResults;
		ThrowIfFailed(pCompiler->Compile(
			&Source,
			arguments,
			_countof(arguments),
			pIncludeHandler,
			IID_PPV_ARGS(&pResults)
		));

		CComPtr<IDxcBlobUtf8> pErrors = nullptr;
		ThrowIfFailed(pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));
		if (pErrors != nullptr && pErrors->GetStringLength() != 0)
			throw std::format_error(std::format("Warnings and Errors:\n{}\n", pErrors->GetStringPointer()));

		HRESULT hrStatus;
		pResults->GetStatus(&hrStatus);
		if (FAILED(hrStatus))
		{
			throw std::format_error("Compilation Failed\n");
		}

		CComPtr<IDxcBlob> pShader = nullptr;
		CComPtr<IDxcBlobUtf16> pShaderName = nullptr;
		ThrowIfFailed(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName));

		//Create ShaderModule and Reflect

		SpvReflectShaderModule reflectShaderModule;
		ThrowIfFailed(spvReflectCreateShaderModule(pShader->GetBufferSize(), pShader->GetBufferPointer(), &reflectShaderModule));
		switch (stage)
		{
		case ShaderStage::VS:
			res->mVertexReflectShaderModule = reflectShaderModule;

			res->mInputVariables.resize(reflectShaderModule.input_variable_count);
			for (int varIndex = 0; varIndex < reflectShaderModule.input_variable_count; ++varIndex)
			{
				SpvReflectInterfaceVariable* inputVar = reflectShaderModule.input_variables[varIndex];
				res->mInputVariables[varIndex] = std::make_pair(inputVar->semantic, inputVar->location);
			}

			break;
		case ShaderStage::PS:
			res->mPixelReflectShaderModule = reflectShaderModule;
			break;
		case ShaderStage::DS:
			res->mDomainReflectShaderModule = reflectShaderModule;
			break;
		case ShaderStage::HS:
			res->mHullReflectShaderModule = reflectShaderModule;
			break;
		case ShaderStage::GS:
			res->mGeometryReflectShaderModule = reflectShaderModule;
			break;
		}

		//std::cout << "----------------------------" << std::endl;
		if (reflectShaderModule.descriptor_set_count > 0)
		{
			uint32_t stageSetCount = reflectShaderModule.descriptor_sets[reflectShaderModule.descriptor_set_count - 1].set + 1;

			if (res->mSetLayoutsDesc.size() < stageSetCount)
			{
				res->mSetLayoutsDesc.resize(stageSetCount);
			}
		}

		for (int i = 0; i < reflectShaderModule.descriptor_set_count; ++i)
		{
			SpvReflectDescriptorSet& desc_set = reflectShaderModule.descriptor_sets[i];
			
			std::vector<DescriptorSetLayoutBindingDesc>& currentSet = res->mSetLayoutsDesc[desc_set.set].pBindings;
			for (int j = 0; j < desc_set.binding_count; j++)
			{
				//https://github.com/microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst#hlsl-register-and-vulkan-binding
				SpvReflectDescriptorBinding* binding = desc_set.bindings[j];

				//resource may be used by multiple stages / 资源可能被多个着色器阶段使用
				std::vector<DescriptorSetLayoutBindingDesc>::iterator findSetLayout
					= std::find_if(currentSet.begin(), currentSet.end(), 
						[&binding](DescriptorSetLayoutBindingDesc& pBinding) { 
							return pBinding.binding == binding->binding;
						});
				DescriptorSetLayoutBindingDesc* bindPtr;
				if (findSetLayout == currentSet.end())
				{
					currentSet.emplace_back();
					bindPtr = &currentSet.back();

					//hlsl have not combined image sampler / hlsl没有combinned image sampler这个概念
					//only get immutable sampler desc in first stage / 只在第一个shader stage获取immutable sampler 描述符
					if (binding->descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER)
					{
						bindPtr->samplerDesc = SamplerPool::ParseSamplerName(binding->name);
					}
				}
				else
				{
					bindPtr = findSetLayout._Ptr;
				}

				bindPtr->name = binding->name;
				bindPtr->binding = binding->binding;
				bindPtr->descriptorCount = binding->count;
				bindPtr->descriptorType = static_cast<VkDescriptorType>(binding->descriptor_type);
				bindPtr->stageFlags |= static_cast<VkShaderStageFlagBits>(reflectShaderModule.shader_stage);

				std::cout << j
					<< " name:" << binding->name
					<< " binding: " << binding->binding
					<< " type: " << magic_enum::enum_name(binding->descriptor_type)
					<< " count: " << binding->count
					<< " set: " << binding->set
					<< std::endl;
			}
			std::cout << "---" << std::endl;
		}

		#pragma region second compile
		//二次编译，第二次不带反射
		pResults = nullptr;
		ThrowIfFailed(pCompiler->Compile(
			&Source,
			arguments,
			_countof(arguments) - 1,
			pIncludeHandler,
			IID_PPV_ARGS(&pResults)
		));

		pErrors = nullptr;
		ThrowIfFailed(pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));
		if (pErrors != nullptr && pErrors->GetStringLength() != 0)
			throw std::format_error(std::format("Warnings and Errors:\n{}\n", pErrors->GetStringPointer()));

		hrStatus;
		pResults->GetStatus(&hrStatus);
		if (FAILED(hrStatus))
		{
			throw std::format_error("Compilation Failed\n");
		}

		pShader = nullptr;
		pShaderName = nullptr;
		ThrowIfFailed(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName));

		#pragma endregion

		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = static_cast<VkShaderStageFlagBits>(reflectShaderModule.shader_stage);
		shaderStageInfo.module = CreateShaderModule(device->GetDevice(), pShader->GetBufferPointer(), pShader->GetBufferSize());
		shaderStageInfo.pName = reflectShaderModule.entry_points->name;

		res->mStageContainer.push_back(shaderStageInfo);
	};

	InitStage(ShaderStage::VS, entries.vs);
	InitStage(ShaderStage::PS, entries.ps);
	InitStage(ShaderStage::DS, entries.ds);
	InitStage(ShaderStage::HS, entries.hs);
	InitStage(ShaderStage::GS, entries.gs);

	res->CreatePipelineLayout();

	return res;
}

void Shader::CreatePipelineLayout()
{
	mPipelineLayout = mDevice->GetPipelineLayoutPool()->Get(mSetLayoutsDesc, mSetLayouts);
}

VkShaderModule Shader::CreateShaderModule(VkDevice device, const void* codebytes, size_t size)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(codebytes);

	VkShaderModule shaderModule;
	ThrowIfFailed(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

	return shaderModule;
}

void Shader::SetupPipelineShaderStageInfo(VkGraphicsPipelineCreateInfo& pipelineInfo) const
{
	pipelineInfo.stageCount = mStageContainer.size();
	pipelineInfo.pStages = mStageContainer.data();
}

const std::vector<Shader::InputVariable> Shader::GetInputVariables() const
{
	return mInputVariables;
}

void Shader::SetupPipelineLayout(VkGraphicsPipelineCreateInfo& pipelineInfo) const
{
	pipelineInfo.layout = mPipelineLayout;
}

bool Shader::IsPipelineLayoutEqual(const Shader& a, const Shader& b)
{
	return a.mSetLayouts == b.mSetLayouts;
}

Shader::BindingPoint Shader::GetBindingPoint(const std::string name) const
{
	
	for (uint32_t setIndex = 0; setIndex < mSetLayoutsDesc.size(); ++setIndex)
	{
		for (const DescriptorSetLayoutBindingDesc& binding : mSetLayoutsDesc[setIndex].pBindings)
		{
			if (binding.name == name)
			{
				return std::make_pair(setIndex, binding.binding);
			}
		}
	}

	return std::make_pair(-1, -1);
}

Shader::Shader(Device* device) : DeviceComponent(device)
{ }

Shader::~Shader()
{
	for (VkPipelineShaderStageCreateInfo& shaderStage : mStageContainer)
	{
		vkDestroyShaderModule(mDevice->GetDevice(), shaderStage.module, nullptr);
	}

	spvReflectDestroyShaderModule(&mVertexReflectShaderModule);
	spvReflectDestroyShaderModule(&mPixelReflectShaderModule);
	spvReflectDestroyShaderModule(&mDomainReflectShaderModule);
	spvReflectDestroyShaderModule(&mHullReflectShaderModule);
	spvReflectDestroyShaderModule(&mGeometryReflectShaderModule);
}