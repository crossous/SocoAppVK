#include "Shader.h"

#include <atlbase.h>        // Common COM helpers.
#include "inc/dxcapi.h"         // Be sure to link with dxcompiler.lib.
#include <d3d12shader.h>    // Shader reflection.
#include "dxUtil.hpp"

#include <vk_format_utils.h>

#include <iostream>
#include <format>

enum class RegisterType : uint8_t
{
	B,//CBV
	T,//SRV
	S,//SAMPLER
	U,//UAV
	Other
};

RegisterType DescriptorTypeToRegisterType(VkDescriptorType descType)
{
	switch (descType)
	{
	case VK_DESCRIPTOR_TYPE_SAMPLER:
		return RegisterType::S;//SamplerState
	case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
	case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE://Texture2D<T>
	case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER://Buffer<T>
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER://tbuffer
		return RegisterType::T;
	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE://RWTexture2D<T>
	case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER://RWBuffer<T>
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
	case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
	case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
		return RegisterType::U;
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER://cbuffer xx
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
	case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
		return RegisterType::B;
	default:
		//VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
		//VK_DESCRIPTOR_TYPE_MUTABLE_VALVE
		return RegisterType::Other;
	}
}

std::wstring RegisterTypeToWString(const RegisterType type)
{
	switch (type)
	{
	case RegisterType::B:
		return L"b";
	case RegisterType::T:
		return L"t";
	case RegisterType::S:
		return L"s";
	case RegisterType::U:
		return L"u";
	default:
		return L"unkown";
	}
}

auto StageToShaderModel = [](VkShaderStageFlagBits stage) -> LPCWSTR
{
	switch (stage)
	{
	case VK_SHADER_STAGE_VERTEX_BIT:
		return L"vs_6_0";
	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return L"ps_6_0";
	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
		return L"ds_6_0";
	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
		return L"hs_6_0";
	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return L"gs_6_0";
	default:
		return L"Error";
	}
};

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

	using SetLayoutIndex = uint8_t;
	using BindingIndex = uint16_t;
	using MinMaxRange = std::pair<BindingIndex, BindingIndex>;
	std::map<SetLayoutIndex, std::map<RegisterType, MinMaxRange>> registerMinMaxRange;

	using RegisterOffset = uint16_t;
	std::vector<std::tuple<RegisterType, SetLayoutIndex, RegisterOffset>> registerOffset;

	auto InitStage = [&device, filename, &res, &Source, &pIncludeHandler, &registerMinMaxRange, &registerOffset]
		(VkShaderStageFlagBits stage, LPCWSTR entry, bool secondCompile)
	{
		if (entry == nullptr)
			return;

		//std::vector<LPCWSTR> arguments;
		std::vector<std::wstring> arguments;
		arguments.push_back(filename.c_str());
		arguments.push_back(L"-E");
		arguments.push_back(entry);
		arguments.push_back(L"-T");
		arguments.push_back(StageToShaderModel(stage));
		arguments.push_back(L"-spirv");
		arguments.push_back(L"-fvk-auto-shift-bindings");

		//open reflection on first compile / 第一次编译开启反射
		if (!secondCompile)
			arguments.push_back(L"-fspv-reflect");
		else
		{
			//close reflection on second compile, and insert offset register argument
			//第二次编译关闭反射，并且偏移编译器指令
			for (auto offsetIte = registerOffset.begin(); offsetIte != registerOffset.end(); ++offsetIte)
			{
				arguments.push_back(std::format(L"-fvk-{}-shift", RegisterTypeToWString(std::get<0>(*offsetIte))));
				arguments.push_back(std::format(L"{}", std::get<2>(*offsetIte)));
				arguments.push_back(std::format(L"{}", std::get<1>(*offsetIte)));
			}

			for (auto ite = arguments.begin(); ite != arguments.end(); ++ite)
			{
				std::wcout << *ite << " ";
			}
			std::cout << std::endl;
		}

		static CComPtr<IDxcCompiler3> pCompiler;
		if (pCompiler == nullptr)
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));

		std::vector<LPCWSTR> charArguments(arguments.size());
		std::transform(arguments.begin(), arguments.end(), charArguments.begin(),
			[](const std::wstring& arg){ return arg.c_str(); });
		
		CComPtr<IDxcResult> pResults;
		ThrowIfFailed(pCompiler->Compile(
			&Source,
			charArguments.data(),
			charArguments.size(),
			pIncludeHandler,
			IID_PPV_ARGS(&pResults)
		));

		CComPtr<IDxcBlobUtf8> pErrors = nullptr;
		ThrowIfFailed(pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));
		if (pErrors != nullptr && pErrors->GetStringLength() != 0)
			throw std::format_error(std::format("Warnings and Errors in {}:\n{}\n",
				(secondCompile ? "Second Compile" : "First Compile"), pErrors->GetStringPointer()));

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
		if (!secondCompile)
		{
			SpvReflectShaderModule reflectShaderModule;
			ThrowIfFailed(spvReflectCreateShaderModule(pShader->GetBufferSize(), pShader->GetBufferPointer(), &reflectShaderModule));
			switch (stage)
			{
			case VK_SHADER_STAGE_VERTEX_BIT:
				res->mVertexReflectShaderModule = reflectShaderModule;

				res->mInputVariables.resize(reflectShaderModule.input_variable_count);
				for (int varIndex = 0; varIndex < reflectShaderModule.input_variable_count; ++varIndex)
				{
					SpvReflectInterfaceVariable* inputVar = reflectShaderModule.input_variables[varIndex];
					res->mInputVariables[varIndex] = std::make_pair(inputVar->semantic, inputVar->location);
				}

				break;
			case VK_SHADER_STAGE_FRAGMENT_BIT:
				res->mPixelReflectShaderModule = reflectShaderModule;
				break;
			case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
				res->mDomainReflectShaderModule = reflectShaderModule;
				break;
			case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
				res->mHullReflectShaderModule = reflectShaderModule;
				break;
			case VK_SHADER_STAGE_GEOMETRY_BIT:
				res->mGeometryReflectShaderModule = reflectShaderModule;
				break;
			}

			//if set not found, create set
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
				std::map<RegisterType, MinMaxRange>* currentSetRange = nullptr;
				auto findSetRangeIte = registerMinMaxRange.find(desc_set.set);
				if (findSetRangeIte == registerMinMaxRange.end())
				{
					const auto[ite, success]
						= registerMinMaxRange.insert(
							std::make_pair(desc_set.set, std::map<RegisterType, MinMaxRange>()));
					currentSetRange = &(ite->second);
				}
				else
				{
					currentSetRange = &(findSetRangeIte->second);
				}
					
				
				for (int j = 0; j < desc_set.binding_count; j++)
				{
					//https://github.com/microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst#hlsl-register-and-vulkan-binding
					SpvReflectDescriptorBinding* binding = desc_set.bindings[j];

					//resource may be used by multiple stages / 资源可能被多个着色器阶段使用
					std::vector<DescriptorSetLayoutBindingDesc>::iterator findSetLayout
						= std::find_if(currentSet.begin(), currentSet.end(), 
							[&binding](DescriptorSetLayoutBindingDesc& pBinding) { 
								return pBinding.binding == binding->binding && pBinding.descriptorType == static_cast<VkDescriptorType>(binding->descriptor_type);
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

						bindPtr->name = binding->name;
						bindPtr->binding = binding->binding;
						bindPtr->descriptorCount = binding->count;
						bindPtr->descriptorType = static_cast<VkDescriptorType>(binding->descriptor_type);
						bindPtr->stageFlags = static_cast<VkShaderStageFlagBits>(reflectShaderModule.shader_stage);

						RegisterType registerType = DescriptorTypeToRegisterType(bindPtr->descriptorType);

						//Record the binding range of each type of register for each set / 记录每个set的各类型寄存器的binding范围
						if (registerType != RegisterType::Other)
						{
							auto findBindingRangeIte = currentSetRange->find(registerType);
							if(findBindingRangeIte == currentSetRange->end())
							{
								MinMaxRange newRange = std::make_pair(binding->binding, binding->binding);
								currentSetRange->insert(std::make_pair(registerType, newRange));
							}
							else
							{
								MinMaxRange currentRange = findBindingRangeIte->second;
								currentRange.first = std::min(currentRange.first, static_cast<uint16_t>(binding->binding));
								currentRange.second = std::max(currentRange.second, static_cast<uint16_t>(binding->binding));
								findBindingRangeIte->second = currentRange;
							}
						}
					}
					else
					{
						bindPtr = findSetLayout._Ptr;
					}

					bindPtr->stageFlags |= static_cast<VkShaderStageFlagBits>(reflectShaderModule.shader_stage);

					std::cout << j
						<< " name:" << binding->name
						<< " binding: " << binding->binding
						<< " type: " << magic_enum::enum_name(binding->descriptor_type)
						<< " array ele count: " << binding->count
						<< " set: " << binding->set
						<< std::endl;
				}
				std::cout << "---" << std::endl;
			}

			VkPipelineShaderStageCreateInfo shaderStageInfo{};
			shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageInfo.stage = static_cast<VkShaderStageFlagBits>(reflectShaderModule.shader_stage);
			//shaderStageInfo.module = CreateShaderModule(device->GetDevice(), pShader->GetBufferPointer(), pShader->GetBufferSize());
			shaderStageInfo.pName = reflectShaderModule.entry_points->name;

			res->mStageContainer.push_back(shaderStageInfo);
		}
		else
		{
			//Create shader module on second compile
			SpvReflectShaderModule reflectShaderModule;
			ThrowIfFailed(spvReflectCreateShaderModule(pShader->GetBufferSize(), pShader->GetBufferPointer(), &reflectShaderModule));
			
			auto stageInfo = std::find_if(res->mStageContainer.begin(), res->mStageContainer.end(),
				[stage](VkPipelineShaderStageCreateInfo& shaderStageInfo){
				return shaderStageInfo.stage == stage;
			});
			stageInfo->module = CreateShaderModule(device->GetDevice(), pShader->GetBufferPointer(), pShader->GetBufferSize());
		}


	};

	InitStage(VK_SHADER_STAGE_VERTEX_BIT, entries.vs, false);
	InitStage(VK_SHADER_STAGE_FRAGMENT_BIT, entries.ps, false);
	InitStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, entries.ds, false);
	InitStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, entries.hs, false);
	InitStage(VK_SHADER_STAGE_GEOMETRY_BIT, entries.gs, false);

	std::vector<std::pair<RegisterType, MinMaxRange>> willSortList;
	for (auto setIte = registerMinMaxRange.begin(); setIte != registerMinMaxRange.end(); ++setIte)
	{
		size_t registerTypeCount = setIte->second.size();
		if (registerTypeCount <= 1)
			continue;

		willSortList.clear();
		willSortList.insert(willSortList.end(), setIte->second.begin(), setIte->second.end());

		std::sort(willSortList.begin(), willSortList.end(),
			[](const std::pair<RegisterType, MinMaxRange>& lhs, const std::pair<RegisterType, MinMaxRange>& rhs)
			{
				MinMaxRange lRange = lhs.second;
				MinMaxRange rRange = rhs.second;
				//first compair min binding, second compair range
				if (lRange.first != rRange.first)
					return lRange.first < rRange.first;
				return (lRange.second - lRange.first) < (rRange.second - rRange.first);
			});

		std::vector<DescriptorSetLayoutBindingDesc>& currentSet = res->mSetLayoutsDesc[setIte->first].pBindings;
		//first element max binding
		uint16_t baseOffset = willSortList[0].second.second + 1;

		for (int registerIndex = 1; registerIndex < willSortList.size(); ++registerIndex)
		{
			const RegisterType type = willSortList[registerIndex].first;
			const MinMaxRange currentRange = willSortList[registerIndex].second;

			//arguments.push_back(std::format(L"-fvk-{}-shift", RegisterTypeToWString(type)));
			//arguments.push_back(std::format(L"{}", baseOffset));
			//arguments.push_back(std::format(L"{}", setIte->first));
			registerOffset.push_back(std::make_tuple(type, setIte->first, baseOffset));

			for (auto bindingIte = currentSet.begin(); bindingIte != currentSet.end(); ++bindingIte)
			{
				DescriptorSetLayoutBindingDesc& binding = *bindingIte;
				if (DescriptorTypeToRegisterType(binding.descriptorType) == type)
				{
					binding.binding += baseOffset;
				}
			}

			baseOffset += (currentRange.second - currentRange.first + 1);
		}
	}

	InitStage(VK_SHADER_STAGE_VERTEX_BIT, entries.vs, true);
	InitStage(VK_SHADER_STAGE_FRAGMENT_BIT, entries.ps, true);
	InitStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, entries.ds, true);
	InitStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, entries.hs, true);
	InitStage(VK_SHADER_STAGE_GEOMETRY_BIT, entries.gs, true);

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