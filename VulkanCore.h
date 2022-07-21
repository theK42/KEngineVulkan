#pragma once

#if defined(_WIN32) || defined(_WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#include <wtypes.h>
#endif
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <optional>

namespace KEngineVulkan {
	class VulkanCore
	{
	public:
#if defined(_WIN32) || defined(_WINDOWS)
		void Init(const std::string& applicationName, HWND hwnd, HINSTANCE hinstance);
#else
		//void initialize(const std::string& applicationName);  //To be determined/implemented
#endif
		VkDevice getDevice() const;
		VmaAllocator getAllocator() const;
		VkRenderPass getRenderPass() const;
		const VkExtent2D & getFramebufferExtent() const;
		VkSampler getSampler(bool repeat = false) const;

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlagBits memoryProperties, VkBuffer& buffer, VmaAllocation & bufferAllocation);
		void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage& image, VmaAllocation & imageAllocation);
	
		//Loading commands, should create a "loading frame complete" function or something to allow their commands to be combined
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


		template <class DataType>
		void uploadVertexBuffer(const DataType* data, size_t size, VkBuffer& buffer, VmaAllocation& bufferAllocation);

		void uploadIndexBuffer(const uint16_t* data, size_t size, VkBuffer& buffer, VmaAllocation& bufferAllocation);

		VkImageView createImageView(VkImage image, VkFormat format) const;

		void startFrame();
		void endFrame();

		bool inRenderPass() const;
		int  getMaxFramesInFlight() const;
		int  getCurrentFrame() const;
		VkCommandBuffer getCommandBuffer() const;
		VkDescriptorPool getDescriptorPool()const ;

	private:
		void createInstance(const std::string& applicationName);
#if defined(_WIN32) || defined(_WINDOWS)
		void createSurface(HWND hwnd, HINSTANCE hinstance);
#endif
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createAllocator();
		void createSwapChain(int width, int height);
		void createSwapChainImageViews();
		void createRenderPass();
		void createFramebuffers();
		void createCommandPool();
		void createDescriptorPool(int size);
		void createCommandBuffers();
		void createSyncObjects();
		void createTextureSamplers();
		
		std::vector<const char*> getRequiredExtensions() const; 
		bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
		
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
		void setupDebugMessenger();
		bool checkValidationLayerSupport() const;
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;
		

		bool isDeviceSuitable(VkPhysicalDevice device) const;

		struct QueueFamilyIndices {
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			bool isComplete() {
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

		struct SwapChainSupportDetails {
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		//These two need to be reworked into a begin/end loading frame system, probably threaded
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);

		//Single instance fields
		const int MAX_FRAMES_IN_FLIGHT = 2;

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device; //logical device
		VmaAllocator allocator;
		VkQueue graphicsQueue;
		VkQueue presentQueue;
		VkSwapchainKHR swapChain;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		VkRenderPass renderPass;  // One here, maybe many
		VkCommandPool commandPool; // Unclear how to manage these
		VkDescriptorPool descriptorPool; // Unclear how to manage these
		std::vector<VkSampler> textureSamplers;
		int currentFrame{ 0 };
		uint32_t imageIndex{ 0 };
		bool mInRenderPass{ false };
		bool framebufferResized{ false };

		//Per swap chain image
		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;

		//Per in-flight render
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;

#ifdef NDEBUG
		bool enableValidationLayers{ false };
#else
		bool enableValidationLayers{ true };
#endif
		const std::vector<const char*> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

	};
	
	template<class DataType>
	void VulkanCore::uploadVertexBuffer(const DataType* vertices, size_t size, VkBuffer& vertexBuffer, VmaAllocation& vertexBufferAllocation)
	{
		VkDeviceSize bufferSize = sizeof(DataType) * size;

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

		void* data;
		vmaMapMemory(allocator, stagingBufferAllocation, &data);
		memcpy(data, vertices, (size_t)bufferSize);
		vmaUnmapMemory(allocator, stagingBufferAllocation);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, (VmaAllocationCreateFlagBits)0, vertexBuffer, vertexBufferAllocation);

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
	}
}