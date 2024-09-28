#pragma once

#include "vulkan/vulkan.h"

#include "Memory/PH_Memory.h"
#include "Containers/PH_DynamicArray.h"

#include "Renderer/PH_ImageTexture.h"

namespace PhosphorEngine {

	using namespace PhosphorEngine::Containers;
	using namespace PhosphorEngine::Memory;

	class PH_SwapChain {
	public:
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		PH_SwapChain(VkExtent2D windowExtent);
		PH_SwapChain(VkExtent2D windowExtent, PH_SharedMemoryHandle<PH_SwapChain> previous);
		~PH_SwapChain();

		PH_SwapChain(const PH_SwapChain&) = delete;
		PH_SwapChain& operator=(const PH_SwapChain&) = delete;

		VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
		VkRenderPass getRenderPass() { return renderPass; }
		VkImageView getImageView(int index) { return swapChainImages[index]->ImageView; }
		size_t imageCount() { return swapChainImages.size(); }
		VkSampler GetDepthSampler(int index) { return depthImages[index]->Sampler; };
		VkImageView GetDepthView(int index) { return depthImages[index]->ImageView; };
		VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
		VkExtent2D getSwapChainExtent() { return swapChainExtent; }
		uint32_t width() { return swapChainExtent.width; }
		uint32_t height() { return swapChainExtent.height; }

		float extentAspectRatio() {
			return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
		}
		VkFormat findDepthFormat();

		VkResult acquireNextImage(uint32_t* imageIndex);
		VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

		const bool compareSwapFormats(const PH_SwapChain& swapChain) const {
			return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
				swapChain.swapChainImageFormat == swapChainImageFormat;
		}

	private:
		void init();
		void createSwapChain();
		void createImageViews();
		void createDepthResources();
		void createRenderPass();
		void createFramebuffers();
		void createSyncObjects();

		// Helper functions
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(
			const PH_DynamicArray<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(
			const PH_DynamicArray<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		VkFormat swapChainImageFormat;
		VkFormat swapChainDepthFormat;
		VkExtent2D swapChainExtent;

		PH_DynamicArray<VkFramebuffer> swapChainFramebuffers;
		VkRenderPass renderPass;

		PH_DynamicArray<PH_ImageBuilder::ImageHandle> depthImages;
		PH_DynamicArray<PH_ImageBuilder::ImageHandle> swapChainImages;

		VkExtent2D windowExtent;

		VkSwapchainKHR swapChain;
		PH_SharedMemoryHandle<PH_SwapChain> oldSwapchain;

		PH_DynamicArray<VkSemaphore> imageAvailableSemaphores;
		PH_DynamicArray<VkSemaphore> renderFinishedSemaphores;
		PH_DynamicArray<VkFence> inFlightFences;
		PH_DynamicArray<VkFence> imagesInFlight;
		size_t currentFrame = 0;
	};

}
