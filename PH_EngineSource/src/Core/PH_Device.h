#pragma once

#include "PH_CORE.h"
#include <vulkan/vulkan.h>

#include "PH_Window.h"
#include "PH_Optional.h"
#include "Containers/PH_DynamicArray.h"

namespace PhosphorEngine {

    using namespace PhosphorEngine::Containers;

    struct QueueFamilyIndices {
        PH_Optional<uint32> graphicsFamily{};
        PH_Optional<uint32> presentFamily{};
        bool isComplete() { return graphicsFamily.IsSet() && presentFamily.IsSet(); }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        PH_DynamicArray<VkSurfaceFormatKHR> formats;
        PH_DynamicArray<VkPresentModeKHR> presentModes;
    };

    enum RenderFeaturesType : uint32
    {
        WireframeRender
    };

	class PH_Device
	{
    public:
#ifndef _PH_DEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif

        PH_Device(PH_Window& window);
        ~PH_Device();

        PH_Device(const PH_Device&) = delete;
        PH_Device& operator=(const PH_Device&) = delete;
        PH_Device(PH_Device&&) = delete;
        PH_Device& operator=(PH_Device&&) = delete;

        VkCommandPool getCommandPool() { return CommandPool; }
        VkInstance GetInstance() { return VulkanInstance; }
        VkPhysicalDevice GetPhysicalDevice() { return PhysicalDevice; };
        VkDevice GetDevice() { return LogicalDevice; }
        VkSurfaceKHR GetSurface() { return VulkanSufrace; }
        VkQueue GetGraphicsQueue() { return GraphicsQueue; }
        VkQueue GetPresentQueue() { return PresentQueue; }

        uint32 GetGraphicsQueueIndex() { return QueuesIndices.graphicsFamily.GetValue(); }
        uint32 GetPresentQueueIndex() { return QueuesIndices.presentFamily.GetValue(); }

        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(PhysicalDevice); }

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(PhysicalDevice); }

        VkFormat findSupportedFormat(
            const PH_DynamicArray<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        // Buffer Helper Functions

        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

        VkPhysicalDeviceProperties properties;

        VkAllocationCallbacks* GetAllocator() { return nullptr; };
        void ToggleRenderFeature(RenderFeaturesType featureType);
    private:
        // Creation helpers
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();

        // Helper functions over Vulkan objects
        bool isDeviceSuitable(VkPhysicalDevice device);
        PH_DynamicArray<const char*> getRequiredExtensions();
        bool checkValidationLayerSupport();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void hasGflwRequiredInstanceExtensions();
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        // Data
        VkInstance VulkanInstance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        PH_Window& Window;
        VkCommandPool CommandPool;

        uint32 RequiredApiVersion = VK_API_VERSION_1_1;

        struct RenderFeatures
        {
            RenderFeatures()
            {
                drawWireframe = 0;
            };

            uint8 drawWireframe : 1;
        };

        RenderFeatures RenderFeaturesConfig;

        VkDevice LogicalDevice;
        VkSurfaceKHR VulkanSufrace;
        VkQueue GraphicsQueue;
        VkQueue PresentQueue;
        QueueFamilyIndices QueuesIndices;

        const PH_DynamicArray<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        const PH_DynamicArray<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    public:
        struct RenderFeatures GetRenderFeaturesConfig() const {
            return RenderFeaturesConfig;
        }
	};

}