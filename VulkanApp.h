#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <tuple>
#include <vector>
#include <vulkan/vulkan.h>
#include <memory>

#include "Device.hpp"
#include "Shader.h"
#include "PSO.h"

#include "Camera.hpp"

namespace Soco
{
	class TriangleApp {
	public:
		void Run();

	private:

		using ExtensionPair = std::tuple<unsigned int, const char**>;

		void InitWindow();
		const std::vector<const char*> GetRequiredExtension();
		bool CheckValidationLayerSupport();
		VkFormat FindSupportFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CreateInstance();
		void SetupDebugCallback();
		void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
		void CreateSurface();
		void CreateDevice();
		void CreateSwapChain();
		void LoadShader();
		void CreateMesh();
		void CreateConstantBuffer();
		void CreateRenderPass();
		void CreateGraphicsPipeline();

		void CreateCamera();

		void CreateDescriptorPool();
		void CreateDescriptorSet();
		void CreateFrameBuffer();
		void CreateCommandBuffers();
		void CreateSemaphores();

		void CreateImageResource();

		void InitVulkan();
		void OnResize();
		void MainLoop();
		void Cleanup();
		void CleanupSwapChainReferenceResource();

		void OnUpdate();
		void OnUpload();
		void OnRender();

		static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
			VkDebugReportFlagsEXT flags,
			VkDebugReportObjectTypeEXT objType,
			uint64_t obj,
			size_t location,
			int32_t code,
			const char* layerPrefix,
			const char* msg,
			void* userData);

	private:
		GLFWwindow* mWindow;

		int mWidth = 800;
		int mHeight = 600;

		VkInstance mInstance = VK_NULL_HANDLE;
		Device mDevice;

		VkFormat mSwapChainImageFormat;
		VkExtent2D mSwapChainExtent;

		VkDebugReportCallbackEXT mCallback = VK_NULL_HANDLE;

		VkSurfaceKHR mSurface = VK_NULL_HANDLE;

		VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
		std::vector<VkImage> mSwapChainImages;
		std::vector<VkImageView> mSwapChainImageViews;
		std::vector<VkFramebuffer> mSwapChainFrameBuffers;

		
		std::vector<VkCommandBuffer> mCommandBuffers;

		VkSemaphore mImageAvailableSemaphore;
		VkSemaphore mRenderFinishedSemaphore;

		VkDeviceMemory mDepthImageMemory;
		VkImage mDepthImage;
		VkImageView mDepthImageView;

		std::map<std::string, std::unique_ptr<Shader>> mShaders;
		std::map<std::string, std::unique_ptr<Mesh>> mMeshes;
		std::map<std::string, Transform> mTransforms;
		std::map<std::string, std::unique_ptr<ConstantBuffer>> mConstantBuffers;

		VkDescriptorPool mDescriptorPool;
		std::vector<VkDescriptorSet> mDescriptorSets;

		VkRenderPass mRenderPass = VK_NULL_HANDLE;
		PSO mPSO;

		std::unique_ptr<Camera> mCamera;

		const std::vector<const char*> mValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> mDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		const std::vector<const char*> mQueryDeviceExtensions = { VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME, VK_GOOGLE_USER_TYPE_EXTENSION_NAME };
		VkPhysicalDeviceFeatures mDeviceFeatures = {};

#ifdef NDEBUG
		const bool mEnableValidationLayers = false;
#else
		const bool mEnableValidationLayers = true;
#endif
	};
}