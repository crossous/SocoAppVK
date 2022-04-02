#include "VulkanApp.h"
#include <stdexcept>
#include <vector>
#include <set>
#include <iostream>

#include "QueueFamilyIndices.hpp"
#include "SwapChainSupportDetails.hpp"

#include "dxUtil.hpp"

namespace Soco {
	void TriangleApp::Run()
	{
		InitWindow();
		InitVulkan();
		MainLoop();
		Cleanup();
	}

	void TriangleApp::InitWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		mWindow = glfwCreateWindow(mWidth, mHeight, "Vulkan Window", nullptr, nullptr);

		glfwSetWindowUserPointer(mWindow, this);
		glfwSetWindowSizeCallback(mWindow, [](GLFWwindow* window, int width, int height) {
			if (width == 0 || height == 0)
				return;

			TriangleApp* app = reinterpret_cast<TriangleApp*>(glfwGetWindowUserPointer(window));
			if (width == app->mWidth && height == app->mHeight)
				return;

			//glfwSetWindowSize(app->mWindow, width, height);

			app->mWidth = width;
			app->mHeight = height;
			app->OnResize();
			});
	}

	const std::vector<const char*> TriangleApp::GetRequiredExtension()
	{

		unsigned int glfwExtensionCount;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::cout << "glfw required instance extensions:" << std::endl;
		for (unsigned int i = 0; i < glfwExtensionCount; ++i)
		{
			std::cout << "\t" << *(glfwExtensions + i) << std::endl;
		}

		unsigned int extensionCount = glfwExtensionCount;
		extensionCount += (mEnableValidationLayers ? 1 : 0);

		std::vector<const char*> extensions(extensionCount);

		for (unsigned int i = 0; i < glfwExtensionCount; ++i)
		{
			extensions[i] = glfwExtensions[i];
		}

		if (mEnableValidationLayers)
			extensions[glfwExtensionCount] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;

		//查询所有拓展
		{
			uint32_t extensionCount = 0;
			ThrowIfFailed(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
			std::vector<VkExtensionProperties> extensions(extensionCount);
			ThrowIfFailed(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()));

			std::cout << "available instance extensions(" << extensionCount << "):" << std::endl;
			for (const VkExtensionProperties& extension : extensions)
				std::cout << "\t" << extension.extensionName << std::endl;
		}

		return extensions;
	}

	bool TriangleApp::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		ThrowIfFailed(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

		std::vector<VkLayerProperties> availableLayers(layerCount);
		ThrowIfFailed(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

		std::cout << "available layers(" << availableLayers.size() << "):" << std::endl;
		for (const VkLayerProperties& layer : availableLayers)
			std::cout << "\t" << layer.layerName << " : " << layer.description << std::endl;

		for (const char* layerName : mValidationLayers)
		{
			bool layerFound = false;

			for (const VkLayerProperties& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	VkFormat TriangleApp::FindSupportFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(mDevice.GetPhysicalDevice(), format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				return format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				return format;
		}

		throw std::runtime_error("failed to find supported format!");
	}

	void TriangleApp::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	}

	void TriangleApp::CreateInstance()
	{
		VkApplicationInfo appInfo = {};

		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		if (mEnableValidationLayers && !CheckValidationLayerSupport())
		{
			throw std::runtime_error("failed to check validation layer");
		};

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		const std::vector<const char*> extensions = GetRequiredExtension();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		createInfo.enabledLayerCount = 0;

		if (mEnableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
			createInfo.ppEnabledLayerNames = mValidationLayers.data();
		}

		ThrowIfFailed(vkCreateInstance(&createInfo, nullptr, &mInstance));
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL TriangleApp::VulkanDebugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData)
	{
		std::cerr << "validation layer: " << msg << std::endl;
		return VK_FALSE;
	}

	void TriangleApp::SetupDebugCallback()
	{
		if (!mEnableValidationLayers)
			return;

		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = VulkanDebugCallback;

		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugReportCallbackEXT");

		VkResult res = VK_ERROR_EXTENSION_NOT_PRESENT;
		if (func != nullptr) {
			res = func(mInstance, &createInfo, nullptr, &mCallback);
		}
		if (res != VK_SUCCESS){
			throw std::runtime_error("failed to setup debug callback!");
		}
	}

	void TriangleApp::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		if (func != nullptr) {
			func(instance, callback, pAllocator);
		}
	}

	void TriangleApp::CreateSurface()
	{
		if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void TriangleApp::CreateDevice()
	{
		mDeviceFeatures.samplerAnisotropy = true;

		mDevice.Init(mInstance, mSurface, mDeviceExtensions, mQueryDeviceExtensions, mValidationLayers, mDeviceFeatures);
	}

	void TriangleApp::CreateSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = SwapChainSupportDetails::QuerySwapChainSupport(mDevice.GetPhysicalDevice(), mSurface);

		VkSurfaceFormatKHR surfaceFormat = SwapChainSupportDetails::ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = SwapChainSupportDetails::ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = SwapChainSupportDetails::ChooseSwapExtent(swapChainSupport.capabilities, mWidth, mHeight);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if(swapChainSupport.capabilities.maxImageCount > 0)
			imageCount = std::clamp(imageCount, 0u, swapChainSupport.capabilities.maxImageCount);

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = mSurface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices queueIndices = QueueFamilyIndices::FindQueueFamilies(mDevice.GetPhysicalDevice(), mSurface);
		if (queueIndices.OnlyQueueFamily())
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

			std::vector<uint32_t> allFamilyIndicesVec = queueIndices.AllFamilyVector();
			createInfo.queueFamilyIndexCount = static_cast<uint32_t>(allFamilyIndicesVec.size());
			createInfo.pQueueFamilyIndices = allFamilyIndicesVec.data();
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = mSwapChain;

		ThrowIfFailed(vkCreateSwapchainKHR(mDevice.GetDevice(), &createInfo, nullptr, &mSwapChain));

		if (createInfo.oldSwapchain != VK_NULL_HANDLE)
		{
			CleanupSwapChainReferenceResource();
			vkDestroySwapchainKHR(mDevice.GetDevice(), createInfo.oldSwapchain, nullptr);
		}

		uint32_t swapChainImageCount;
		ThrowIfFailed(vkGetSwapchainImagesKHR(mDevice.GetDevice(), mSwapChain, &swapChainImageCount, nullptr));
		mSwapChainImages.resize(swapChainImageCount);
		ThrowIfFailed(vkGetSwapchainImagesKHR(mDevice.GetDevice(), mSwapChain, &swapChainImageCount, mSwapChainImages.data()));

		mSwapChainImageViews.resize(swapChainImageCount);
		for (uint32_t i = 0; i < swapChainImageCount; ++i)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = mSwapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = surfaceFormat.format;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			ThrowIfFailed(vkCreateImageView(mDevice.GetDevice(), &createInfo, nullptr, &mSwapChainImageViews[i]));
		}

		mSwapChainImageFormat = surfaceFormat.format;
		mSwapChainExtent = extent;
	}

	void TriangleApp::LoadShader()
	{
		ShaderEntry entries;
		entries.vs = L"vert";
		entries.ps = L"frag";

		mShaders["Shaders/unlit.hlsl"] = Shader::LoadFromFile(&mDevice, L"Shaders/unlit.hlsl", entries);
	}

	void TriangleApp::CreateMesh()
	{
		Transform triangleTransform;
		mMeshes["Triangle"] = FormatMesh::CreateTriangle(&mDevice);
		mTransforms["Triangle"] = triangleTransform;
	}

	struct PerObject
	{
		glm::mat4 ObjectToWorldMatrix;
		glm::mat4 WorldToObjectMatrix;
	};

	void TriangleApp::CreateConstantBuffer()
	{
		mConstantBuffers["Triangle"] = std::make_unique<ConstantBuffer>(&mDevice);
		mConstantBuffers["Triangle"]->Init(sizeof(PerObject));
	}

	void TriangleApp::CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = mSwapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef
		{
			.attachment=0, 
			.layout= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL 
		};

		VkSubpassDescription subpass
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1, // SV_TARGET0 in shader
			.pColorAttachments = &colorAttachmentRef
		};

		VkSubpassDependency dependency
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,

			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,

			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		};

		VkRenderPassCreateInfo renderPassInfo
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &colorAttachment,

			.subpassCount = 1,
			.pSubpasses = &subpass,

			.dependencyCount = 1,
			.pDependencies = &dependency
		};

		ThrowIfFailed(vkCreateRenderPass(mDevice.GetDevice(), &renderPassInfo, nullptr, &mRenderPass));

	}

	void TriangleApp::CreateGraphicsPipeline()
	{
		mPSO.Init(mDevice.GetDevice(), *mShaders["Shaders/unlit.hlsl"].get(), *mMeshes["Triangle"].get(), mSwapChainExtent, mRenderPass, 0);
	}

	void TriangleApp::CreateDescriptorPool()
	{
		//Shader* shader = mShaders["Shaders/unlit.hlsl"].get();

		VkDescriptorPoolSize poolSizes[] = { 
			{.type{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}, .descriptorCount{1000}},
			{.type{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE}, .descriptorCount{1000}}
		};
		VkDescriptorPoolCreateInfo poolInfo{ 
			.sType{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO}, 
			.flags{VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT}, 
			.maxSets{100}, 
			.poolSizeCount{_countof(poolSizes)}, 
			.pPoolSizes{poolSizes} 
		};

		ThrowIfFailed(vkCreateDescriptorPool(mDevice.GetDevice(), &poolInfo, nullptr, &mDescriptorPool));
	}

	void TriangleApp::CreateDescriptorSet()
	{
		const std::vector<VkDescriptorSetLayout>& setLayouts = mShaders["Shaders/unlit.hlsl"]->GetDescriptorSetLayout();
		mDescriptorSets.resize(setLayouts.size());

		VkDescriptorSetAllocateInfo allocInfo
		{
			.sType{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO},
			.descriptorPool{mDescriptorPool},
			.descriptorSetCount{static_cast<uint32_t>(setLayouts.size())},
			.pSetLayouts{setLayouts.data()}
		};

		ThrowIfFailed(vkAllocateDescriptorSets(mDevice.GetDevice(), &allocInfo, mDescriptorSets.data()));
	}

	void TriangleApp::CreateFrameBuffer()
	{
		mSwapChainFrameBuffers.resize(mSwapChainImageViews.size());

		for (size_t i = 0; i < mSwapChainImageViews.size(); ++i)
		{
			VkImageView attachments[] = {
				mSwapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = mRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = mSwapChainExtent.width;
			framebufferInfo.height = mSwapChainExtent.height;
			framebufferInfo.layers = 1;

			ThrowIfFailed(vkCreateFramebuffer(mDevice.GetDevice(), &framebufferInfo, nullptr, &mSwapChainFrameBuffers[i]));
		}
	}

	void TriangleApp::CreateCommandBuffers()
	{
		mCommandBuffers.resize(mSwapChainFrameBuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mDevice.GetGraphicsCommandPool();
		//VK_COMMAND_BUFFER_LEVEL_PRIMARY: 可以提交到队列执行，但不能从其他的命令缓冲区调用。
		//VK_COMMAND_BUFFER_LEVEL_SECONDARY : 无法直接提交，但是可以从主命令缓冲区调用。
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mCommandBuffers.size();

		ThrowIfFailed(vkAllocateCommandBuffers(mDevice.GetDevice(), &allocInfo, mCommandBuffers.data()));
	}

	void TriangleApp::CreateSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		ThrowIfFailed(vkCreateSemaphore(mDevice.GetDevice(), &semaphoreInfo, nullptr, &mRenderFinishedSemaphore));
		ThrowIfFailed(vkCreateSemaphore(mDevice.GetDevice(), &semaphoreInfo, nullptr, &mImageAvailableSemaphore));
	}

	void TriangleApp::CreateImageResource()
	{
		auto AllocAndBindImageMemory = [&](VkImage image, VkDeviceMemory memory, VkMemoryPropertyFlags properties)
		{
			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(mDevice.GetDevice(), image, &memRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = mDevice.FindMemoryType(memRequirements.memoryTypeBits, properties);
			ThrowIfFailed(vkAllocateMemory(mDevice.GetDevice(), &allocInfo, nullptr, &memory));

			vkBindImageMemory(mDevice.GetDevice(), image, memory, 0);
		};

		auto CreateImage2DView = [&](VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlag)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspectFlag;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			ThrowIfFailed(vkCreateImageView(mDevice.GetDevice(), &viewInfo, nullptr, &imageView));

			return imageView;
		};

		auto CreateTex2D = [&](uint32_t width, uint32_t height, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlagBits aspectFlag,
			VkImage& image, VkDeviceMemory& imageMemory, VkImageView& view)
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1; 
			imageInfo.arrayLayers = 1;
			imageInfo.format = format;
			imageInfo.tiling = tiling;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = usage;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.queueFamilyIndexCount = 0;
			imageInfo.pQueueFamilyIndices = nullptr;

			ThrowIfFailed(vkCreateImage(mDevice.GetDevice(), &imageInfo, nullptr, &image));
			AllocAndBindImageMemory(image, imageMemory, properties);
			CreateImage2DView(image, format, aspectFlag);
		};

		//Depth
		VkFormat depthFormat = FindSupportFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
		
		CreateTex2D(mSwapChainExtent.width, mSwapChainExtent.height, depthFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
			mDepthImage, mDepthImageMemory, mDepthImageView
		);
	}

	void TriangleApp::CreateCamera()
	{
		mCamera = std::make_unique<Camera>(&mDevice);

		Transform* cameraTransform = mCamera->GetTransform();
		cameraTransform->SetLocalPosition(glm::vec3(0, 0, 5));
		cameraTransform->SetLocalRotation(glm::quat(glm::vec3(0, glm::radians(180.0f), 0)));

		mCamera->SetFov(glm::radians(90.0f));
		mCamera->SetAspect((float)mWidth / (float)mHeight);
		mCamera->SetNear(0.25f);
		mCamera->SetFar(1000.0f);

		mCamera->BuildBuffer();
	}

	void TriangleApp::InitVulkan()
	{
		CreateInstance();
		CreateSurface();
		SetupDebugCallback();
		CreateDevice();
		CreateSwapChain();
		LoadShader();
		CreateMesh();
		CreateConstantBuffer();
		CreateRenderPass();
		CreateGraphicsPipeline();

		CreateCamera();

		CreateDescriptorPool();
		CreateDescriptorSet();
		CreateFrameBuffer();
		CreateCommandBuffers();
		CreateSemaphores();
		CreateConstantBuffer();
	}

	void TriangleApp::OnResize()
	{
		vkDeviceWaitIdle(mDevice.GetDevice());

		//Create Resource
		CreateSwapChain();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFrameBuffer();

		mCamera->SetAspect((float)mWidth / (float)mHeight);
	}

	void TriangleApp::MainLoop()
	{
		while (!glfwWindowShouldClose(mWindow)) {
			glfwPollEvents();

			OnUpdate();
			OnUpload();
			OnRender();
		}

		vkDeviceWaitIdle(mDevice.GetDevice());
	}

	void TriangleApp::Cleanup()
	{
		mCamera->ClearBuffer();

		vkDestroySemaphore(mDevice.GetDevice(), mRenderFinishedSemaphore, nullptr);
		vkDestroySemaphore(mDevice.GetDevice(), mImageAvailableSemaphore, nullptr);

		vkFreeCommandBuffers(mDevice.GetDevice(), mDevice.GetGraphicsCommandPool(), mCommandBuffers.size(), mCommandBuffers.data());

		for (const VkFramebuffer& swapChainFrameBuffer : mSwapChainFrameBuffers)
			vkDestroyFramebuffer(mDevice.GetDevice(), swapChainFrameBuffer, nullptr);

		vkFreeDescriptorSets(mDevice.GetDevice(), mDescriptorPool, mShaders["Shaders/unlit.hlsl"]->GetDescriptorSetLayout().size(), mDescriptorSets.data());
		vkDestroyDescriptorPool(mDevice.GetDevice(), mDescriptorPool, nullptr);

		mConstantBuffers.clear();
		mMeshes.clear();
		mShaders.clear();


		vkDestroyRenderPass(mDevice.GetDevice(), mRenderPass, nullptr);
		mPSO.Clear();

		for (const VkImageView& image : mSwapChainImageViews)
			vkDestroyImageView(mDevice.GetDevice(), image, nullptr);

		vkDestroySwapchainKHR(mDevice.GetDevice(), mSwapChain, nullptr);
		mDevice.Destroy();
		DestroyDebugReportCallbackEXT(mInstance, mCallback, nullptr);
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		vkDestroyInstance(mInstance, nullptr);
		glfwDestroyWindow(mWindow);
		glfwTerminate();
	}

	void TriangleApp::CleanupSwapChainReferenceResource()
	{
		for (const VkFramebuffer& swapChainFrameBuffer : mSwapChainFrameBuffers)
			vkDestroyFramebuffer(mDevice.GetDevice(), swapChainFrameBuffer, nullptr);

		vkDestroyRenderPass(mDevice.GetDevice(), mRenderPass, nullptr);
		mPSO.Clear();

		for (const VkImageView& image : mSwapChainImageViews)
			vkDestroyImageView(mDevice.GetDevice(), image, nullptr);
	}

	void TriangleApp::OnUpdate()
	{
		Transform& triangleTransform = mTransforms["Triangle"];
		glm::quat triangleRotation = triangleTransform.GetLocalRotation();
		triangleRotation *= glm::quat(glm::vec3(0, 0, glm::radians(5.0f)));
		triangleTransform.SetLocalRotation(triangleRotation);
	}

	void TriangleApp::OnUpload()
	{
		vkQueueWaitIdle(mDevice.GetPresentQueue().queue);

		//PerCamera Buffer
		mCamera->UpdateBuffer();

		VkDescriptorBufferInfo camearBufferInfo = mCamera->GetPerCameraBufferInfo();

		auto [cameraBufferSetIndex, cameraBufferBinding] = mShaders["Shaders/unlit.hlsl"]->GetBindingPoint("PerCamera");

		VkWriteDescriptorSet camearBufferDescriptorWrite
		{
			.sType{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET},
			.dstSet{mDescriptorSets[cameraBufferSetIndex]},
			.dstBinding{cameraBufferBinding},
			.dstArrayElement{0},
			.descriptorCount{1},
			.descriptorType{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
			.pImageInfo{nullptr},
			.pBufferInfo{&camearBufferInfo},
			.pTexelBufferView{nullptr}
		};

		//PerObject Buffer
		Transform& triangleTransform = mTransforms["Triangle"];


		auto [objectBufferSetIndex, objectBufferBinding] = mShaders["Shaders/unlit.hlsl"]->GetBindingPoint("PerObject");

		PerObject trianglePerObjectBuffer;
		trianglePerObjectBuffer.ObjectToWorldMatrix = triangleTransform.GetGlobalMatrix();
		trianglePerObjectBuffer.WorldToObjectMatrix = glm::inverse(trianglePerObjectBuffer.ObjectToWorldMatrix);

		//Update perObjectBuffer
		ConstantBuffer* perObjectBuffer = mConstantBuffers["Triangle"].get();
		perObjectBuffer->UpdateBuffer(&trianglePerObjectBuffer);
		VkDescriptorBufferInfo objectBufferInfo = perObjectBuffer->GetBufferInfo();

		VkWriteDescriptorSet objectBufferDescriptorWrite
		{
			.sType{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET},
			.dstSet{mDescriptorSets[objectBufferSetIndex]},
			.dstBinding{objectBufferBinding},
			.dstArrayElement{0},
			.descriptorCount{1},
			.descriptorType{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
			.pImageInfo{nullptr},
			.pBufferInfo{&objectBufferInfo},
			.pTexelBufferView{nullptr}
		};

		VkWriteDescriptorSet writeSets[] = {camearBufferDescriptorWrite, objectBufferDescriptorWrite};
		vkUpdateDescriptorSets(mDevice.GetDevice(), _countof(writeSets), writeSets, 0, nullptr);
	}

	void TriangleApp::OnRender()
	{
		uint32_t imageIndex;
		ThrowIfFailed(vkAcquireNextImageKHR(mDevice.GetDevice(), mSwapChain, std::numeric_limits<uint64_t>::max(), mImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex));

		VkCommandBuffer currentCommandBuffer = mCommandBuffers[imageIndex];

		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: 命令缓冲区将在执行一次后立即重新记录。
		//VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 这是一个辅助缓冲区，它限制在在一个渲染通道中。
		//VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : 命令缓冲区也可以重新提交，同时它也在等待执行。
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		cmdBeginInfo.pInheritanceInfo = nullptr; // Optional

		ThrowIfFailed(vkBeginCommandBuffer(currentCommandBuffer, &cmdBeginInfo));

		vkCmdSetViewport(currentCommandBuffer, 0, 1, &(mPSO.GetViewport()));

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = mRenderPass;
		renderPassBeginInfo.framebuffer = mSwapChainFrameBuffers[imageIndex];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = mSwapChainExtent;
		
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(currentCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		Shader* shader = mShaders["Shaders/unlit.hlsl"].get();

		vkCmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPSO.GetPipeline());
		vkCmdBindDescriptorSets(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetPipelineLayout(), 0, mDescriptorSets.size(), mDescriptorSets.data(), 0, nullptr);

		for (auto& [name, mesh] : mMeshes)
		{
			vkCmdBindIndexBuffer(currentCommandBuffer, mesh->GetIndexBuffer(), 0, mesh->GetIndexType());
			vkCmdBindVertexBuffers(currentCommandBuffer, 0, mesh->GetBindingCount(), mesh->GetVertexBuffers(), mesh->GetOffsets());

			for (Mesh::SubmeshGeometry submesh : mesh->GetSubmesh())
			{
				vkCmdDrawIndexed(currentCommandBuffer, submesh.IndexCount, 1, submesh.StartIndexLocation, submesh.BaseVertexLocation, 0);
			}
		}

		vkCmdEndRenderPass(currentCommandBuffer);
		ThrowIfFailed(vkEndCommandBuffer(currentCommandBuffer));

		VkSemaphore waitSemaphores[] = { mImageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphore };

		//Submit
		{

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = _countof(waitSemaphores);
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];

			submitInfo.signalSemaphoreCount = _countof(signalSemaphores);
			submitInfo.pSignalSemaphores = signalSemaphores;

			ThrowIfFailed(vkQueueSubmit(mDevice.GetGraphicsQueue().queue, 1, &submitInfo, VK_NULL_HANDLE));
		}

		//Presentation
		{
			VkSwapchainKHR swapChains[] = { mSwapChain };

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;

			ThrowIfFailed(vkQueuePresentKHR(mDevice.GetPresentQueue().queue, &presentInfo));
		}


	}
}