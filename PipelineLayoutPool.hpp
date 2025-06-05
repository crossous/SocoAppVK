#pragma once

#include "dxUtil.hpp"
#include "SamplerPool.hpp"

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <type_traits>

struct DescriptorSetLayoutBindingDesc
{
	std::string name;
	uint32_t binding;
	VkDescriptorType descriptorType;
	uint32_t descriptorCount;
	VkShaderStageFlags stageFlags;
	SamplerDesc samplerDesc;

	const VkDescriptorSetLayoutBinding ToBind(SamplerPool* pSamplerPool, VkSampler* pSamplerSpace) const
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = this->binding;
		binding.descriptorType = descriptorType;
		binding.descriptorCount = descriptorCount;
		binding.stageFlags = stageFlags;

		if (descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER && descriptorCount != 0)
		{
			VkSampler sampler = pSamplerPool->Get(samplerDesc);
			for (int samplerIndex = 0; samplerIndex < descriptorCount; ++samplerIndex)
				pSamplerSpace[samplerIndex] = sampler;
			binding.pImmutableSamplers = pSamplerSpace;
		}
		else
		{
			binding.pImmutableSamplers = nullptr;
		}

		return binding;
	}

	uint32_t NeedTempVkSamplerSpace() const
	{
		if (descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER)
			return 0;
		else
			return descriptorCount;
	}

	std::strong_ordering operator<=>(const DescriptorSetLayoutBindingDesc&) const = default;
	// bool operator==(const DescriptorSetLayoutBindingDesc& rhs) const
	// {
	// 	return this->binding != rhs.binding
	// 		&& this->descriptorType != rhs.descriptorType
	// 		&& this->descriptorCount != rhs.descriptorCount
	// 		&& this->stageFlags != rhs.stageFlags;
	// }
};

template<>
struct std::hash<DescriptorSetLayoutBindingDesc>
{
	std::size_t operator()(const DescriptorSetLayoutBindingDesc& key) const
	{
		using DescriptorTypeUnderlyingType = std::underlying_type_t<VkDescriptorType>;

		std::size_t res = std::hash<uint32_t>()(key.binding);
		//res ^= std::hash<std::string>()(key.name);
		res ^= std::hash<DescriptorTypeUnderlyingType>()(key.descriptorType) << 1;
		res <<= 1;
		res ^= std::hash<uint32_t>()(key.descriptorCount) >> 1;
		res ^= std::hash<VkShaderStageFlags>()(key.stageFlags) << 1;
		if (key.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER && key.descriptorCount != 0)
		{
			res ^= std::hash<SamplerDesc>()(key.samplerDesc) << 2;
		}

		return res;
	}
};

//auto generator compare use <=> operator
//用<=>操作符自动生成比较
//template<>
//struct std::equal_to<DescriptorSetLayoutBindingDesc>
//{
//	bool operator()(const DescriptorSetLayoutBindingDesc& lhs, const DescriptorSetLayoutBindingDesc& rhs)
//	{
//		RETURN_FALSE_IF_NOT_EQUAL(binding)
//		RETURN_FALSE_IF_NOT_EQUAL(descriptorType)
//		RETURN_FALSE_IF_NOT_EQUAL(descriptorCount)
//		RETURN_FALSE_IF_NOT_EQUAL(stageFlags)
//		RETURN_FALSE_IF_NOT_EQUAL(samplerDesc)
//
//		return true;
//	}
//};

struct DescriptorSetLayoutDesc
{
	VkDescriptorSetLayoutCreateFlags flags;
	std::vector<DescriptorSetLayoutBindingDesc> pBindings;

	const VkDescriptorSetLayoutCreateInfo ToLayout(VkDescriptorSetLayoutBinding* pBindingsSpace, SamplerPool* pSamplerPool, VkSampler** pSamplerSpace) const
	{
		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = pBindings.size();

		if (createInfo.bindingCount != 0)
		{
			createInfo.pBindings = pBindingsSpace;
			for (int bindingIndex = 0; bindingIndex < createInfo.bindingCount; ++bindingIndex)
			{
				pBindingsSpace[bindingIndex] = pBindings[bindingIndex].ToBind(pSamplerPool, pSamplerSpace[bindingIndex]);
			}
		}
		else
		{
			createInfo.pBindings = nullptr;
		}

		return createInfo;
	}

	const uint32_t NeedTempBindingSpace() const
	{
		return pBindings.size();
	}

	bool operator==(const DescriptorSetLayoutDesc& rhs) const
	{
		if (flags != rhs.flags)
			return false;
		
		if (pBindings.size() != rhs.pBindings.size())
		{
			return false;
		}
		else
		{
			for (int bindingIndex = 0; bindingIndex < pBindings.size(); ++bindingIndex)
			{
				if (!std::equal_to<DescriptorSetLayoutBindingDesc>()(pBindings[bindingIndex], rhs.pBindings[bindingIndex]))
					return false;
			}
		}

		return true;
	}
};

template<>
struct std::hash<DescriptorSetLayoutDesc>
{
	std::size_t operator()(const DescriptorSetLayoutDesc& key) const
	{
		std::size_t res = std::hash<VkDescriptorSetLayoutCreateFlags>()(key.flags);
		res ^= std::hash<uint32_t>()(key.pBindings.size()) << 1;
		for (int bindingIndex = 0; bindingIndex < key.pBindings.size(); ++bindingIndex)
		{
			res ^= std::hash<DescriptorSetLayoutBindingDesc>()(key.pBindings[bindingIndex]) << (bindingIndex % 4);
		}

		return res;
	}
};

// can not compile std::unordered_map<std::vector<DescriptorSetLayoutDesc>, value>
// 定义equal_to无法使vector<DescriptorSetLayoutDesc>作为unordered_map的key通过编译，只有operator==可以
//template<>
//struct std::equal_to<DescriptorSetLayoutDesc>
//{
//	bool operator()(const DescriptorSetLayoutDesc& lhs, const DescriptorSetLayoutDesc& rhs) const
//	{
//		if (lhs.flags != rhs.flags)
//			return false;
//		
//		if (lhs.pBindings.size() != rhs.pBindings.size())
//		{
//			return false;
//		}
//		else
//		{
//			for (int bindingIndex = 0; bindingIndex < lhs.pBindings.size(); ++bindingIndex)
//			{
//				if (std::equal_to<DescriptorSetLayoutBindingDesc>()(lhs.pBindings[bindingIndex], rhs.pBindings[bindingIndex]))
//					return false;
//			}
//		}
//
//		return true;
//	}
//};



class DescriptorSetLayoutPool
{
	friend class PipelineLayoutPool;

public:

	VkDescriptorSetLayout Get(const DescriptorSetLayoutDesc& layoutDesc)
	{
		auto ite = mDescriptorSetLayoutPool.find(layoutDesc);
		if (ite == mDescriptorSetLayoutPool.end())
		{
			VkDescriptorSetLayout newSetLayout;

			std::vector<VkDescriptorSetLayoutBinding> tempBindingSpace(layoutDesc.NeedTempBindingSpace());
			std::vector<std::vector<VkSampler>> tempSamplerSpace(layoutDesc.pBindings.size());
			std::vector<VkSampler*> tempSamplerPointerSpace(layoutDesc.pBindings.size());
			for (int bindingIndex = 0; bindingIndex < layoutDesc.pBindings.size(); ++bindingIndex)
			{
				tempSamplerSpace[bindingIndex].resize(layoutDesc.pBindings[bindingIndex].NeedTempVkSamplerSpace());
				tempSamplerPointerSpace[bindingIndex] = tempSamplerSpace[bindingIndex].data();
			}
				

			VkDescriptorSetLayoutCreateInfo createInfo = layoutDesc.ToLayout(tempBindingSpace.data(), mSamplerPool, tempSamplerPointerSpace.data());
			ThrowIfFailed(vkCreateDescriptorSetLayout(mDevice, &createInfo, nullptr, &newSetLayout));

			bool isSuccess;
			std::tie(ite, isSuccess) = mDescriptorSetLayoutPool.insert(std::make_pair(layoutDesc, newSetLayout));
		}

		return ite->second;
	}

private:
	std::unordered_map<DescriptorSetLayoutDesc, VkDescriptorSetLayout> mDescriptorSetLayoutPool;
	VkDevice mDevice;
	SamplerPool* mSamplerPool;

	void Clear()
	{
		for (auto ite = mDescriptorSetLayoutPool.begin(); ite != mDescriptorSetLayoutPool.end(); ++ite)
		{
			vkDestroyDescriptorSetLayout(mDevice, ite->second, nullptr);
		}

		mDescriptorSetLayoutPool.clear();
	}

	void Init(VkDevice device, SamplerPool* pSamplerPool)
	{
		mDevice = device;
		mSamplerPool = pSamplerPool;
	}
};

template<>
struct std::hash<std::vector<DescriptorSetLayoutDesc>>
{
	std::size_t operator()(const std::vector<DescriptorSetLayoutDesc>& key) const
	{
		std::size_t res = 0;
		for (int i = 0; i < key.size(); ++i)
			i ^= std::hash<DescriptorSetLayoutDesc>()(key[i]) << (i % 8);

		return res;
	}
};

template<>
struct std::equal_to<std::vector<DescriptorSetLayoutDesc>>
{
	bool operator()(const std::vector<DescriptorSetLayoutDesc>& lhs, const std::vector<DescriptorSetLayoutDesc>& rhs) const
	{
		return lhs == rhs;
	}
};

class PipelineLayoutPool
{
	friend class Device;

public:

	VkPipelineLayout Get(const std::vector<DescriptorSetLayoutDesc>& setLayoutDescs, std::vector<VkDescriptorSetLayout>& setLayouts)
	{
		auto ite = mPipelineLayoutPool.find(setLayoutDescs);
		if (ite != mPipelineLayoutPool.end())
			return ite->second;
		else
		{
			setLayouts.resize(setLayoutDescs.size());
			for (int setIndex = 0; setIndex < setLayoutDescs.size(); ++setIndex)
			{
				//TODO: there hash calc has problem
				setLayouts[setIndex] = mDescriptorSetLayoutPool.Get(setLayoutDescs[setIndex]);
			}

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.pushConstantRangeCount = 0; // OpCyoutional
			pipelineLayoutInfo.pPushConstantRanges = 0; // Optional
			pipelineLayoutInfo.setLayoutCount = setLayouts.size();
			pipelineLayoutInfo.pSetLayouts = setLayouts.data();

			VkPipelineLayout pipelineLayout;
			ThrowIfFailed(vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));
			mPipelineLayoutPool[setLayoutDescs] = pipelineLayout;
			return pipelineLayout;
		}
	}
private:

	void Clear()
	{
		for (auto ite = mPipelineLayoutPool.begin(); ite != mPipelineLayoutPool.end(); ++ite)
		{
			vkDestroyPipelineLayout(mDevice, ite->second, nullptr);
		}

		mPipelineLayoutPool.clear();
		mDescriptorSetLayoutPool.Clear();
	}

	void Init(VkDevice device, SamplerPool* pSamplerPool)
	{
		mDevice = device;
		mDescriptorSetLayoutPool.Init(device, pSamplerPool);
	}

	std::unordered_map<std::vector<DescriptorSetLayoutDesc>, VkPipelineLayout, 
		std::hash<std::vector<DescriptorSetLayoutDesc>>, std::equal_to<std::vector<DescriptorSetLayoutDesc>>> mPipelineLayoutPool;
	DescriptorSetLayoutPool mDescriptorSetLayoutPool;
	VkDevice mDevice;
};