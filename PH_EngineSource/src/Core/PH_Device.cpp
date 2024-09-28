#include "PH_CORE.h"
#include "PH_Device.h"
#include "PH_Logger.h"

#include "Memory/PH_Memory.h"
#include <cstring>
#include "Containers/PH_Set.h"  
#include "Containers/PH_String.h"

namespace PhosphorEngine {

    // local callback functions
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        PH_DynamicArray<char> DebugMsg{};
        DebugMsg.resize(100 + strlen(pCallbackData->pMessage));
        sprintf(DebugMsg.data(), "Vulkan Validation Layer: %s", pCallbackData->pMessage);
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            PH_TRACE(DebugMsg.data());
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            PH_INFO(DebugMsg.data());
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            PH_WARN(DebugMsg.data());
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            PH_VULKAN_ERROR(DebugMsg.data());
            break;
        };
        return VK_FALSE;
    };

    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance_,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance_,
            "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance_, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance_,
        VkDebugUtilsMessengerEXT debugMessenger_,
        const VkAllocationCallbacks* pAllocator) 
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance_,
            "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance_, debugMessenger_, pAllocator);
        }
    }

    // class member functions
    PH_Device::PH_Device(PH_Window& window) : Window{ window } 
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createCommandPool();
    }

    PH_Device::~PH_Device() 
    {
        vkDestroyCommandPool(LogicalDevice, CommandPool, GetAllocator());
        vkDestroyDevice(LogicalDevice, GetAllocator());

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(VulkanInstance, DebugMessenger, GetAllocator());
        }

        vkDestroySurfaceKHR(VulkanInstance, VulkanSufrace, GetAllocator());
        vkDestroyInstance(VulkanInstance, GetAllocator());
    }

    void PH_Device::ToggleRenderFeature(RenderFeaturesType featureType)
    {
        switch (featureType)
        {
        case PhosphorEngine::WireframeRender:
            RenderFeaturesConfig.drawWireframe = (RenderFeaturesConfig.drawWireframe + 1) % 2;
        default:
            break;
        }
    }

    void PH_Device::createInstance()
    {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            PH_ERROR("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Phosphor Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Phosphor Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = RequiredApiVersion;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        PH_VULKAN_CHECK_MSG(vkCreateInstance(&createInfo, GetAllocator(), &VulkanInstance), "Failed to create instance!");

        hasGflwRequiredInstanceExtensions();
    }

    void PH_Device::pickPhysicalDevice() 
    {
        uint32_t deviceCount = 0;
        bool IsDeviceDiscreteGPU = false;

        vkEnumeratePhysicalDevices(VulkanInstance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            PH_ERROR("failed to find GPUs with Vulkan support!");
        }
        std::cout << "Device count: " << deviceCount << std::endl;
        PH_DynamicArray<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(VulkanInstance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                PhysicalDevice = device;
                
                VkPhysicalDeviceProperties deviceProperties;
                VkPhysicalDeviceFeatures deviceFeatures;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);
                vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

                if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    IsDeviceDiscreteGPU = true;
                };

                if (device && IsDeviceDiscreteGPU)
                {
                    break;
                };
            }
        }

        if (PhysicalDevice == VK_NULL_HANDLE) {
            PH_ERROR("failed to find a suitable GPU!");
        }

        vkGetPhysicalDeviceProperties(PhysicalDevice, &properties);
        std::cout << "physical device: " << properties.deviceName << std::endl;
    }

    void PH_Device::createLogicalDevice() 
    {
        QueueFamilyIndices indices = findQueueFamilies(PhysicalDevice);

        if(!indices.isComplete())
            PH_VULKAN_ERROR("Failed to find all needed queue families while creating logical device!")

        PH_DynamicArray<VkDeviceQueueCreateInfo> queueCreateInfos;
        PH_Set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.GetValue(), indices.presentFamily.GetValue() };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures AvailableDeviceFeatures;
        vkGetPhysicalDeviceFeatures(PhysicalDevice, &AvailableDeviceFeatures);

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        if(AvailableDeviceFeatures.fillModeNonSolid)
            deviceFeatures.fillModeNonSolid = VK_TRUE;

        if (AvailableDeviceFeatures.wideLines)
            deviceFeatures.wideLines = VK_TRUE;

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // For compatibility with older implementations of VULKAN(should we have this compatibility?)
        // [VULKAN][DEPRECATED]
        // ====
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }
        // ====

        PH_VULKAN_CHECK_MSG(vkCreateDevice(PhysicalDevice, &createInfo, GetAllocator(), &LogicalDevice),
            "Failed to create logical device!");

        QueuesIndices = indices;

        vkGetDeviceQueue(LogicalDevice, indices.graphicsFamily.GetValue(), 0, &GraphicsQueue);
        vkGetDeviceQueue(LogicalDevice, indices.presentFamily.GetValue(), 0, &PresentQueue);
    }

    void PH_Device::createCommandPool() 
    {
        QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.GetValue();
        poolInfo.flags =
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        PH_VULKAN_CHECK_MSG(vkCreateCommandPool(LogicalDevice, &poolInfo, GetAllocator(), &CommandPool),
            "Failed to create command pool!");
    }

    void PH_Device::createSurface() { Window.createWindowSurface(VulkanInstance, &VulkanSufrace); }

    bool PH_Device::isDeviceSuitable(VkPhysicalDevice device) 
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        if (deviceProperties.apiVersion < RequiredApiVersion)
        {
            return false;
        }

        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate &&
            supportedFeatures.samplerAnisotropy;
    }

    void PH_Device::populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;  // Optional
    }

    void PH_Device::setupDebugMessenger() 
    {
        if (!enableValidationLayers) return;
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);
        PH_VULKAN_CHECK_MSG(CreateDebugUtilsMessengerEXT(VulkanInstance, &createInfo, GetAllocator(), &DebugMessenger), 
            "Failed to set up debug messenger!");
    }

    bool PH_Device::checkValidationLayerSupport() 
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        PH_DynamicArray<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    PH_DynamicArray<const char*> PH_Device::getRequiredExtensions() 
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        PH_DynamicArray<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void PH_Device::hasGflwRequiredInstanceExtensions() {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        PH_DynamicArray<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        PH_INFO("available extensions:");
        PH_Set<PH_String> available;
        for (const auto& extension : extensions) {
            PH_INFO("\t %s", extension.extensionName);
            available.insert(extension.extensionName);
        }

        PH_INFO("required extensions:");
        auto requiredExtensions = getRequiredExtensions();
        for (const auto& required : requiredExtensions) {
            PH_INFO("\t %s", required);
            if (available.find(required) == available.end()) {
                PH_ERROR("Missing required glfw extension");
            }
        }
    }

    bool PH_Device::checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        PH_DynamicArray<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &extensionCount,
            availableExtensions.data());

        PH_Set<PH_String> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices PH_Device::findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        PH_DynamicArray<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        uint32 i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily.SetValue(i);
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, VulkanSufrace, &presentSupport);
            if (queueFamily.queueCount > 0 && presentSupport) {
                indices.presentFamily.SetValue(i);
            }
            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    SwapChainSupportDetails PH_Device::querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, VulkanSufrace, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, VulkanSufrace, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, VulkanSufrace, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, VulkanSufrace, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device,
                VulkanSufrace,
                &presentModeCount,
                details.presentModes.data());
        }
        return details;
    }

    VkFormat PH_Device::findSupportedFormat(
        const PH_DynamicArray<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(PhysicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (
                tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        PH_ERROR("failed to find supported format!");
    }

    uint32_t PH_Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        PH_ERROR("failed to find suitable memory type!");
    }

    VkCommandBuffer PH_Device::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = CommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(LogicalDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void PH_Device::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(GraphicsQueue);

        vkFreeCommandBuffers(LogicalDevice, CommandPool, 1, &commandBuffer);
    }

    void PH_Device::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = srcOffset;  // Optional
        copyRegion.dstOffset = dstOffset;  // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }
}