#pragma once

#include "Core/PH_Device.h"
#include "Core/PH_Window.h"
#include "Core/PH_SwapChain.h"
#include "Containers/PH_DynamicArray.h"

#include "Memory/PH_Memory.h"
#include <cassert>

#include "Core/PH_Buffer.h"
#include "PH_ImageTexture.h"

namespace PhosphorEngine {

	using namespace Containers;

	struct IndirectDrawCmd {
		VkDrawIndexedIndirectCommand command;
		uint32_t objectID;
		uint32_t batchID;
	};

	class PH_Renderer 
	{
	public:
		PH_Renderer(PH_Window& window);
		~PH_Renderer();

		PH_Renderer(const PH_Renderer&) = delete;
		PH_Renderer& operator=(const PH_Renderer&) = delete;

		const VkRenderPass getSwapChainRenderPass() { return SwapChain->getRenderPass(); }

		float32 getAspectRatio() const { return SwapChain->extentAspectRatio(); }

		const bool IsFrameInProgress() { return isFrameStarted; }

		const VkCommandBuffer getCurrentCommandBuffer() {
			PH_ASSERT(IsFrameInProgress() && " Cannot get command buffer because frame not in progress");
			return CommandBuffers[currentFrameIndex];
		}

		const int getFrameIndex() {
			PH_ASSERT(IsFrameInProgress() && "Frame processing not started yet!");
			return currentFrameIndex;
		}
		PH_SwapChain& GetSwapchain() { return SwapChain.GetReference(); };
		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

		PH_ImageBuilder::ImageHandle CreateTexture(PH_String TextureName, uint32 Width, uint32 Height, uint32 ChannelCount,
			const uint8* Pixels, bool HasTransparency);

		void DestroyTexture(PH_ImageTexture& TextureHandle);
	private:
		void createCommandBuffers();
		void recreateSwapChain();
		void freeCommandBuffers();

		PH_Window& Window;
		
		PH_UniqueMemoryHandle<PH_SwapChain> SwapChain;
		PH_DynamicArray<VkCommandBuffer> CommandBuffers;

		uint32 currentImageIndex = 0;
		int32 currentFrameIndex = 0;
		bool isFrameStarted = false;
	};

}