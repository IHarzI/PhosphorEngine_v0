#include "PH_CORE.h"
#include "PH_Assert.h"
#include "PH_SwapChain.h"
#include "Containers/PH_StaticArray.h"
#include "PH_MATH.h"
#include "Core/PH_Device.h"
#include "Core/PH_REngineContext.h"
#include "PH_Profiler.h"

#include <limits>
#include <cstdlib>
#include <cstring>

#undef max
#undef min

namespace PhosphorEngine {

    using namespace PhosphorEngine::Containers;

    PH_SwapChain::PH_SwapChain( VkExtent2D extent)
        : windowExtent{ extent } {
        init();
    }

    PH_SwapChain::PH_SwapChain(VkExtent2D extent, PH_SharedMemoryHandle<PH_SwapChain> previous)
        : windowExtent{ extent },oldSwapchain(previous) {
        init();

        oldSwapchain = nullptr;
    }

    void PH_SwapChain::init()
    {
        createSwapChain();  
        createImageViews();
        createRenderPass();
        createDepthResources();
        createFramebuffers();
        createSyncObjects();
    }


    PH_SwapChain::~PH_SwapChain() {

        for (auto& swapChainImage : swapChainImages) {
            PH_ImageBuilder::DestroyImageView(*swapChainImage);
        }

        if (swapChain != nullptr) {
            vkDestroySwapchainKHR(PH_REngineContext::GetLogicalDevice(), swapChain, PH_REngineContext::GetDevice().GetAllocator());
            swapChain = nullptr;
        }

        swapChainImages.clear();

        for (auto& depthImage : depthImages) {
            PH_ImageBuilder::DestroyImage(*depthImage);

            PH_ImageBuilder::DestroyImageSampler(*depthImage);
        }

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(PH_REngineContext::GetLogicalDevice(), framebuffer, PH_REngineContext::GetDevice().GetAllocator());
        }

        vkDestroyRenderPass(PH_REngineContext::GetLogicalDevice(), renderPass, PH_REngineContext::GetDevice().GetAllocator());

        // cleanup synchronization objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(PH_REngineContext::GetLogicalDevice(), renderFinishedSemaphores[i], PH_REngineContext::GetDevice().GetAllocator());
            vkDestroySemaphore(PH_REngineContext::GetLogicalDevice(), imageAvailableSemaphores[i], PH_REngineContext::GetDevice().GetAllocator());
            vkDestroyFence(PH_REngineContext::GetLogicalDevice(), inFlightFences[i], PH_REngineContext::GetDevice().GetAllocator());
        }
    }

    VkResult PH_SwapChain::acquireNextImage(uint32_t* imageIndex) {
        const uint64 timeout = 1000000;
        vkWaitForFences(
            PH_REngineContext::GetLogicalDevice(),
            1,
            &inFlightFences[currentFrame],
            VK_TRUE,
            timeout);

        VkResult result = vkAcquireNextImageKHR(
            PH_REngineContext::GetLogicalDevice(),
            swapChain,
            timeout,
            imageAvailableSemaphores[currentFrame],  // must be a not signaled semaphore
            VK_NULL_HANDLE,
            imageIndex);

        return result;
    }

    VkResult PH_SwapChain::submitCommandBuffers(
        const VkCommandBuffer* buffers, uint32_t* imageIndex) {
        PH_PROFILE_SCOPE(SWAPCHAIN_SUBMITCMNDBUFFER)
        if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(PH_REngineContext::GetLogicalDevice(), 1, &imagesInFlight[*imageIndex], VK_TRUE, 1000000);
        }
        imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = buffers;

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(PH_REngineContext::GetLogicalDevice(), 1, &inFlightFences[currentFrame]);
        {
            PH_PROFILE_SCOPE(SWAPCHAIN_SUBMITCMNDBUFFER_QUEUESUBMIT)
            PH_VULKAN_CHECK_MSG(vkQueueSubmit(PH_REngineContext::GetDevice().GetGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]),
                "Failed to submit draw command buffer!");
        };
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = imageIndex;

        PH_PROFILE_SCOPE(SWAPCHAIN_SUBMITCMNDBUFFER_QUEUEPRESENT)
        auto result = vkQueuePresentKHR(PH_REngineContext::GetDevice().GetPresentQueue(), &presentInfo);

        return result;
    }

    void PH_SwapChain::createSwapChain() {
        PH_Device& ph_device = PH_REngineContext::GetDevice();
        SwapChainSupportDetails swapChainSupport = PH_REngineContext::GetDevice().getSwapChainSupport();

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 &&
            imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = PH_REngineContext::GetDevice().GetSurface();

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = PH_REngineContext::GetDevice().findPhysicalQueueFamilies();
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.GetValue(), indices.presentFamily.GetValue() };

        if (indices.graphicsFamily.GetValue() != indices.presentFamily.GetValue()) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;      // Optional
            createInfo.pQueueFamilyIndices = nullptr;  // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = oldSwapchain == nullptr ? VK_NULL_HANDLE : oldSwapchain->swapChain;

        PH_VULKAN_CHECK_MSG(vkCreateSwapchainKHR(ph_device.GetDevice(), &createInfo, ph_device.GetAllocator(), &swapChain),
            "Failed to create swap chain!");

        // Only minimal image count was specified, so Vulkan is allowed to create more images,
        // so need to retrive image count firstly and then resize swapchain images container
        vkGetSwapchainImagesKHR(ph_device.GetDevice(), swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        PH_DynamicArray<VkImage> SwapChainRawImages(imageCount);
        vkGetSwapchainImagesKHR(ph_device.GetDevice(), swapChain, &imageCount, SwapChainRawImages.data());

        for (uint32 i = 0; i < imageCount; i++)
        {
            PH_String ImageName = "SwapChainImage";
            ImageName += i + 30; // ascii number starts from 30
            swapChainImages[i] = PH_ImageBuilder::BuildImage(ImageName, extent.width, extent.height, 4);
            swapChainImages[i]->ImageHandle = SwapChainRawImages[i];
        }

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void PH_SwapChain::createImageViews() {
        for (auto& swapChainImage : swapChainImages) 
        {
            PH_ImageBuilder::CreateImageView(*swapChainImage, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        };
    }

    void PH_SwapChain::createRenderPass() {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = getSwapChainImageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstSubpass = 0;
        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        PH_StaticArray<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        PH_VULKAN_CHECK_MSG(vkCreateRenderPass(PH_REngineContext::GetLogicalDevice(), &renderPassInfo, PH_REngineContext::GetDevice().GetAllocator(), &renderPass),
            "Failed to create render pass!");
    }

    void PH_SwapChain::createFramebuffers() {
        swapChainFramebuffers.resize(imageCount());
        for (size_t i = 0; i < imageCount(); i++) 
        {
            PhosphorEngine::Containers::PH_StaticArray<VkImageView, 2> attachments = 
            { 
                swapChainImages[i]->ImageView, 
                depthImages[i]->ImageView 
            };

            VkExtent2D swapChainExtent = getSwapChainExtent();
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(
                PH_REngineContext::GetLogicalDevice(),
                &framebufferInfo,
                PH_REngineContext::GetDevice().GetAllocator(),
                &swapChainFramebuffers[i]) != VK_SUCCESS) {
                PH_VULKAN_ERROR("Failed to create framebuffer!");
            }
        }
    }

    void PH_SwapChain::createDepthResources() {
        VkFormat depthFormat = findDepthFormat();
        swapChainDepthFormat = depthFormat;
        VkExtent2D swapChainExtent = getSwapChainExtent();

        depthImages.resize(imageCount());

        for (int i = 0; i < depthImages.size(); i++) 
        {
            PH_String ImageName = "DepthImage";
            ImageName += i + 30; // ascii number starts from 30
            depthImages[i] = PH_ImageBuilder::BuildImage(ImageName, swapChainExtent.width, swapChainExtent.height, 4);
            PH_ImageBuilder::CreateImageTexture(*depthImages[i],
                VK_IMAGE_TYPE_2D,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                true,
                VK_IMAGE_ASPECT_DEPTH_BIT);
            PH_ImageBuilder::CreateImageSampler(depthImages[i].GetReference(), VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 0.f, 1.f);
        }
    }

    void PH_SwapChain::createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(PH_REngineContext::GetLogicalDevice(), &semaphoreInfo, PH_REngineContext::GetDevice().GetAllocator(), &imageAvailableSemaphores[i]) !=
                VK_SUCCESS ||
                vkCreateSemaphore(PH_REngineContext::GetLogicalDevice(), &semaphoreInfo, PH_REngineContext::GetDevice().GetAllocator(), &renderFinishedSemaphores[i]) !=
                VK_SUCCESS ||
                vkCreateFence(PH_REngineContext::GetLogicalDevice(), &fenceInfo, PH_REngineContext::GetDevice().GetAllocator(), &inFlightFences[i]) != VK_SUCCESS) {
                PH_VULKAN_ERROR("Failed to create synchronization objects for a frame!");
            }
        }
    }

    VkSurfaceFormatKHR PH_SwapChain::chooseSwapSurfaceFormat(
        const PH_DynamicArray<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR PH_SwapChain::chooseSwapPresentMode(
        const PH_DynamicArray<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
               PH_INFO("Present mode: Mailbox");
               return availablePresentMode;
            }
        }

       //for (const auto &availablePresentMode : availablePresentModes) {
       //  if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
       //    std::cout << "Present mode: Immediate" << std::endl;
       //    return availablePresentMode;
       //  }
       //}
       PH_INFO("Present mode: V-Sync");
       return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D PH_SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            VkExtent2D actualExtent = windowExtent;
            actualExtent.width = Math::max(
                capabilities.minImageExtent.width,
                Math::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = Math::max(
                capabilities.minImageExtent.height,
                Math::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    VkFormat PH_SwapChain::findDepthFormat() {
        return PH_REngineContext::GetDevice().findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

}