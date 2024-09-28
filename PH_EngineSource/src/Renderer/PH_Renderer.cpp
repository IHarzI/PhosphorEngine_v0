#include "PH_Renderer.h"
#include "Core/PH_Assert.h"
#include "Containers/PH_StaticArray.h"
#include "Core/PH_Buffer.h"
#include "Core/PH_Profiler.h"

#include "Core/PH_REngineContext.h"

constexpr VkClearColorValue VulkanSurfaceClearColor{ 0.005f,0.01f,0.02f,0.1f };

namespace PhosphorEngine {

	PH_Renderer::PH_Renderer(PH_Window& window) : Window(window)
	{
		recreateSwapChain();
		createCommandBuffers();
	}

	PH_Renderer::~PH_Renderer()
	{
		freeCommandBuffers();
	}

	void PH_Renderer::createCommandBuffers()
	{
		CommandBuffers.resize(PH_SwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = PH_REngineContext::GetDevice().getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(CommandBuffers.size());

		PH_VULKAN_CHECK_MSG(vkAllocateCommandBuffers(PH_REngineContext::GetLogicalDevice(), &allocInfo, CommandBuffers.data()),
			"Failed to allocate command buffers!");
	}

	void PH_Renderer::recreateSwapChain()
	{
		auto extent = Window.getExtent();
		while (extent.width == 0 || extent.height == 0)
		{
			extent = Window.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(PH_REngineContext::GetLogicalDevice());

		if (SwapChain == nullptr) {
			SwapChain = PH_UniqueMemoryHandle<PH_SwapChain>::Create(extent);
		}
		else
		{
			PH_SharedMemoryHandle<PH_SwapChain> oldSwapChain{ SwapChain.RetrieveResourse() };
			SwapChain = PH_UniqueMemoryHandle<PH_SwapChain>::Create(extent, oldSwapChain);

			PH_VULKAN_CHECK_MSG(!oldSwapChain->compareSwapFormats(*SwapChain.Get()),
				"Swap chain image/depth format has changed!");
		}
	}

	void PH_Renderer::freeCommandBuffers()
	{
		vkFreeCommandBuffers(PH_REngineContext::GetLogicalDevice(), PH_REngineContext::GetDevice().getCommandPool(), static_cast<uint32_t>(CommandBuffers.size()), CommandBuffers.data());
		CommandBuffers.clear();
	}

	VkCommandBuffer PH_Renderer::beginFrame() 
	{
		PH_PROFILE_SCOPE(RENDERER_BeginFrame)

		PH_ASSERT_MSG(!IsFrameInProgress(), "Can't call begin frame when frame already started");

		auto result = SwapChain->acquireNextImage(&currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			PH_VULKAN_ERROR("Failed to acquire swap chain image!");
		}

		isFrameStarted = true;
		auto commandBuffer = getCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		PH_VULKAN_CHECK_MSG(vkBeginCommandBuffer(commandBuffer, &beginInfo),
			"Failed to begin command buffer!");

		return commandBuffer;
	}

	void PH_Renderer::endFrame() 
	{
		PH_PROFILE_SCOPE(RENDERER_EndFrame)

		PH_ASSERT_MSG(IsFrameInProgress(), "Can't call endFrame while frame is not in progress");
		auto commandBuffer = getCurrentCommandBuffer();

		PH_VULKAN_CHECK_MSG(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer!");

		{
			PH_PROFILE_SCOPE(RENDERER_EndFrameComndSubmit)

			auto result = SwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || Window.wasWindowResized())
			{
				Window.resetWindowResizedFlag();
				recreateSwapChain();
			}
			else if (result != VK_SUCCESS)
				PH_VULKAN_ERROR("Failed to submit command buffer!");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % PH_SwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void PH_Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) 
	{
		PH_PROFILE_SCOPE(RENDERER_BeginSwapChainRenderPass)

		PH_ASSERT_MSG(isFrameStarted, "Can't call beginSwapChainRenderPass while frame is not in progress");
		PH_ASSERT_MSG(commandBuffer == getCurrentCommandBuffer(), "Can't begin render pass on command buffer from a different frame");

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = SwapChain->getRenderPass();
		renderPassBeginInfo.framebuffer = SwapChain->getFrameBuffer(currentImageIndex);
		renderPassBeginInfo.renderArea.offset = { 0,0 };
		renderPassBeginInfo.renderArea.extent = SwapChain->getSwapChainExtent();

		PH_StaticArray<VkClearValue, 2> clearValues{};
		clearValues[0].color = { VulkanSurfaceClearColor };
		clearValues[1].depthStencil = { 1.f,0 };
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.f;
		viewport.y = 0.f;
		viewport.width = static_cast<float>(SwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(SwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;
		VkRect2D scissor{ {0,0},SwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void PH_Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) 
	{
		PH_PROFILE_SCOPE(RENDERER_EndSwapChainRenderPass)
		PH_ASSERT_MSG(isFrameStarted, "Can't call endSwapChainRenderPass while frame is not in progress");
		PH_ASSERT_MSG(commandBuffer == getCurrentCommandBuffer(), "Can't end render pass on command buffer from a different frame");

		vkCmdEndRenderPass(commandBuffer);
	}

	PH_ImageBuilder::ImageHandle PH_Renderer::CreateTexture(PH_String TextureName, uint32 Width, uint32 Height,
		uint32 ChannelCount, const uint8* Pixels, bool HasTransparency)
	{
		VkDeviceSize image_size = Width * Height * ChannelCount;

		// NOTE: Assumes 8 bits per channel.
		VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

		// Create a staging buffer and load data into it.
		VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VkMemoryPropertyFlags memory_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		PH_Buffer StaginBuffer{image_size,1,usage,memory_prop_flags,true };
		StaginBuffer.map();
		StaginBuffer.writeToBuffer((void*)Pixels, image_size, 0);

		// NOTE: Lots of assumptions here, different texture types will require
		// different options here.
		auto ImageHandle = PH_ImageBuilder::BuildImage(TextureName, Width, Height, ChannelCount);

		PH_ImageBuilder::CreateImageTexture(
			ImageHandle.GetReference(),
			VK_IMAGE_TYPE_2D,
			image_format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true,
			VK_IMAGE_ASPECT_COLOR_BIT);

		auto TempCommandBuffer = PH_REngineContext::GetDevice().beginSingleTimeCommands();

		// Transition the layout from whatever it is currently to optimal for recieving data.
		PH_ImageBuilder::TransitionLayout(ImageHandle.GetReference(), TempCommandBuffer, image_format, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Copy the data from the buffer.
		PH_ImageBuilder::CopyFromBuffer(ImageHandle.GetReference(), StaginBuffer, TempCommandBuffer);

		// Transition from optimal for data reciept to shader-read-only optimal layout.
		PH_ImageBuilder::TransitionLayout(ImageHandle.GetReference(), TempCommandBuffer, image_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		PH_REngineContext::GetDevice().endSingleTimeCommands(TempCommandBuffer);

		// Create a sampler for the texture
		VkSamplerCreateInfo sampler_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

		// TODO: These filters should be configurable.
		sampler_info.magFilter = VK_FILTER_LINEAR;
		sampler_info.minFilter = VK_FILTER_LINEAR;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.anisotropyEnable = VK_TRUE;
		sampler_info.maxAnisotropy = 16;
		sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler_info.unnormalizedCoordinates = VK_FALSE;
		sampler_info.compareEnable = VK_FALSE;
		sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_info.mipLodBias = 0.0f;
		sampler_info.minLod = 0.0f;
		sampler_info.maxLod = 5.0f;

		PH_VULKAN_CHECK_MSG(vkCreateSampler(PH_REngineContext::GetLogicalDevice(), &sampler_info, PH_REngineContext::GetDevice().GetAllocator(), &ImageHandle.GetReference().Sampler),
			"Error creating texture sampler!");

		return ImageHandle.RetrieveResourse();
	}

	void PH_Renderer::DestroyTexture(PH_ImageTexture& TextureHandle)
	{
		PH_ImageBuilder::DestroyImage(TextureHandle);
		vkDestroySampler(PH_REngineContext::GetLogicalDevice(), TextureHandle.Sampler, PH_REngineContext::GetDevice().GetAllocator());
		TextureHandle.Sampler = 0;
	}
}