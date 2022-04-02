#pragma once

#include "dxUtil.hpp"

#include <vulkan/vulkan.h>
#include <regex>
#include <map>

struct SamplerDesc
{
	VkFilter filter;
	bool anisoEnable;
	uint32_t maxAniso;
	VkSamplerAddressMode addressMode;

	std::strong_ordering operator<=>(const SamplerDesc&) const = default;

//	bool operator<(const SamplerDesc& rhs) const 
//	{
//#define RETURN_TRUE_IF_LESS(var)\
//	if(var < rhs.var)\
//		return true;
//
//		RETURN_TRUE_IF_LESS(filter)
//		RETURN_TRUE_IF_LESS(anisoEnable)
//		RETURN_TRUE_IF_LESS(maxAniso)
//		RETURN_TRUE_IF_LESS(addressMode)
//
//		return false;
//#undef RETURN_TRUE_IF_LESS
//	}
//
//	bool operator<(const SamplerDesc& rhs) const
//	{
//#define RETURN_TRUE_IF_LESS(var)\
//	if(var < rhs.var)\
//		return true;
//
//		RETURN_TRUE_IF_LESS(filter)
//			RETURN_TRUE_IF_LESS(anisoEnable)
//			RETURN_TRUE_IF_LESS(maxAniso)
//			RETURN_TRUE_IF_LESS(addressMode)
//
//			return false;
//#undef RETURN_TRUE_IF_LESS
//	}
};

template<>
struct std::hash<SamplerDesc>
{
	std::size_t operator()(const SamplerDesc& key) const
	{
		using VkFilterUnderlyingType = std::underlying_type_t<VkFilter>;
		using VkSamplerAddressModeUnderlyingType = std::underlying_type_t<VkSamplerAddressMode>;

		size_t res = std::hash<VkFilterUnderlyingType>()(key.filter);
		res ^= std::hash<bool>()(key.anisoEnable) << 4;
		res ^= std::hash<uint32_t>()(key.maxAniso) << 2;
		res ^= std::hash<VkSamplerAddressModeUnderlyingType>()(key.addressMode) << 1;

		return res;
	}
};

class SamplerPool
{
	friend class Device;

public:
	//return std::vector because vector will reallocate when get sampler
	//返回std::vector，因为在获取sampler过程中可能导致重新分配地址, .data()返回的地址并不可靠
	//std::pair<SamplerDesc, std::vector<VkSampler>> Get(const std::string samplerName, uint32_t samplerCount)
	//{
	//	SamplerDesc samplerDesc = ParseSamplerName(samplerName);
	//	return std::make_pair(samplerDesc, Get(samplerDesc, samplerCount));

	//}

	VkSampler Get(const SamplerDesc& samplerDesc)
	{
		auto ite = mSamplerPool.find(samplerDesc);
		VkSampler sampler;
		if (ite != mSamplerPool.end())
		{
			sampler = ite->second;
		}
		else
		{
			sampler = CreateSampler(samplerDesc);
			mSamplerPool.insert(std::make_pair(samplerDesc, sampler));
		}

		return sampler;
	}

	//name regular
	//filter linear\nearest
	//aniso aniso__
	//addressMode repeat mirror clamp edge border
	static SamplerDesc ParseSamplerName(const std::string samplerName)
	{
		std::string lowerSamplerName(samplerName.size(), '_');
		std::transform(samplerName.cbegin(), samplerName.cend(), lowerSamplerName.begin(), (int (*)(int))std::tolower);

		SamplerDesc samplerDesc;
		//filter
		if (lowerSamplerName.find("linear") != std::string::npos)
			samplerDesc.filter = VK_FILTER_LINEAR;
		else
			samplerDesc.filter = VK_FILTER_NEAREST;

		//aniso
		const std::regex anisoRgx("aniso(\\d+)");
		std::smatch res;
		if (std::regex_search(lowerSamplerName.cbegin(), lowerSamplerName.cend(), res, anisoRgx))
		{
			std::string aniso = res[1];
			samplerDesc.anisoEnable = true;
			samplerDesc.maxAniso = std::stoi(aniso);
		}
		else
		{
			samplerDesc.anisoEnable = false;
			samplerDesc.maxAniso = 0;
		}

		//address mode
		samplerDesc.addressMode = GetAddressMode(lowerSamplerName);
		return samplerDesc;
	}

private:
	uint32_t mMaxSamplerAnisotropy;
	VkDevice mDevice;
	std::map<SamplerDesc, VkSampler> mSamplerPool;

	void Clear()
	{
		for (auto mapIte = mSamplerPool.begin(); mapIte != mSamplerPool.end(); ++mapIte)
		{
			vkDestroySampler(mDevice, mapIte->second, nullptr);
		}
		mSamplerPool.clear();
	}

	SamplerPool() = default;

	void Init(VkPhysicalDevice physicalDevice, VkDevice device)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		mMaxSamplerAnisotropy = properties.limits.maxSamplerAnisotropy;
		mDevice = device;
	}

	VkSampler CreateSampler(const SamplerDesc& samplerDesc)
	{
		VkSamplerCreateInfo createInfo = GetDefaultSamplerCreateInfo();
		createInfo.addressModeU = samplerDesc.addressMode;
		createInfo.addressModeV = samplerDesc.addressMode;
		createInfo.addressModeW = samplerDesc.addressMode;
		createInfo.magFilter = samplerDesc.filter;
		createInfo.minFilter = samplerDesc.filter;
		createInfo.mipmapMode = samplerDesc.filter == VK_FILTER_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
		createInfo.anisotropyEnable = samplerDesc.anisoEnable ? VK_TRUE : VK_FALSE;
		createInfo.maxAnisotropy = std::min(samplerDesc.maxAniso, mMaxSamplerAnisotropy);

		VkSampler newSampler;
		ThrowIfFailed(vkCreateSampler(mDevice, &createInfo, nullptr, &newSampler));
		return newSampler;
	}

	constexpr static VkSamplerCreateInfo GetDefaultSamplerCreateInfo()
	{
		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.magFilter = VK_FILTER_NEAREST;
		createInfo.minFilter = VK_FILTER_NEAREST;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		createInfo.mipLodBias = 0;
		createInfo.anisotropyEnable = VK_FALSE;
		createInfo.maxAnisotropy = 0;
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_NEVER;
		createInfo.minLod = 0;
		createInfo.maxLod = VK_LOD_CLAMP_NONE;
		createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;

		return createInfo;
	}

	static VkSamplerAddressMode GetAddressMode(const std::string lowerSamplerName)
	{
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		bool isRepeat = lowerSamplerName.find("repeat") != std::string::npos;
		bool isMirror = lowerSamplerName.find("mirror") != std::string::npos;
		if (isRepeat)
		{
			if (isMirror)
				addressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			//else addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
		else
		{
			bool isClamp = lowerSamplerName.find("clamp") != std::string::npos;
			if (isClamp)
			{
				bool isBorder = lowerSamplerName.find("border") != std::string::npos;
				if (isBorder)
				{
					addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				}
				else
				{
					bool isEdge = lowerSamplerName.find("edge") != std::string::npos;
					if (isEdge)
					{
						if (isMirror)
							addressMode = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
						else
							addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					}
				}

			}
		}

		return addressMode;
	}
};