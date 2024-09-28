#include "PH_ImageTexture.h"
#include "Core/PH_Device.h"
#include "Renderer/PH_Renderer.h"
#include "Core/PH_Buffer.h"

#include "Core/PH_REngineContext.h"

namespace PhosphorEngine {

	PH_ImageBuilder::ImageHandle PH_ImageBuilder::BuildImage(PH_String TextureName, uint32 Width, uint32 Height, uint32 ChannelCount)
	{
		auto ImageToReturn = PH_ImageBuilder::ImageHandle::Create(TextureName,Width, Height, ChannelCount);

		PH_ASSERT(ImageToReturn.IsValid());

		ImageToReturn.Get()->ID = CurrentIDForNextImage++;
		return ImageToReturn;
	}

	void PH_ImageBuilder::CreateImageTexture(PH_ImageTexture& ImageTextureHandle, VkImageType type,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memoryFlags,
		bool shouldCreateView,
		VkImageAspectFlags viewAspectFlags)
	{
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = ImageTextureHandle.Width;
		imageCreateInfo.extent.height = ImageTextureHandle.Height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 4;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = tiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = usage;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		PH_VULKAN_CHECK_MSG(
			vkCreateImage(PH_REngineContext::GetLogicalDevice(), &imageCreateInfo, PH_REngineContext::GetDevice().GetAllocator(), &ImageTextureHandle.ImageHandle),
			"Failed to create image!");

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(PH_REngineContext::GetLogicalDevice(), ImageTextureHandle.ImageHandle, &memoryRequirements);

		int32 memoryTypeIndex = PH_REngineContext::GetDevice().findMemoryType(memoryRequirements.memoryTypeBits, memoryFlags);
		if (memoryTypeIndex == -1) {
			PH_ERROR("Required memory type not found. Image not valid.");
		}

		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
		PH_VULKAN_CHECK_MSG(
			vkAllocateMemory(PH_REngineContext::GetLogicalDevice(), &memoryAllocateInfo, PH_REngineContext::GetDevice().GetAllocator(), &ImageTextureHandle.ImageMemory),
			"Failed to allocate memory for image.");

		PH_VULKAN_CHECK_MSG(
			vkBindImageMemory(PH_REngineContext::GetLogicalDevice(), ImageTextureHandle.ImageHandle, ImageTextureHandle.ImageMemory, 0),
			"Failed to bind image memory");

		// Create view
		if (shouldCreateView) {
			ImageTextureHandle.ImageView = {};
			CreateImageView(ImageTextureHandle, format, viewAspectFlags);
		}
	}

	void PH_ImageBuilder::CreateImageSampler(PH_ImageTexture& ImageTextureHandle,
		VkFilter Filter,
		VkSamplerMipmapMode MipMap,
		VkSamplerAddressMode addressMode, float minLod, float MaxLod, VkBorderColor BorderColor, bool compareEnable, VkCompareOp CompareOp)
	{
		VkSamplerCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.pNext = nullptr;

		info.mipmapMode = MipMap;
		info.magFilter = Filter;
		info.minFilter = Filter;
		info.addressModeU = addressMode;
		info.addressModeV = addressMode;
		info.addressModeW = addressMode;
		info.compareEnable = compareEnable;
		info.compareOp = CompareOp;
		info.borderColor = BorderColor;
		info.minLod = minLod;
		info.maxLod = MaxLod;

		PH_VULKAN_CHECK_MSG(vkCreateSampler(PH_REngineContext::GetLogicalDevice(), &info, PH_REngineContext::GetDevice().GetAllocator(), &ImageTextureHandle.Sampler), 
			"Failed to create image sampler");
	}

	void PH_ImageBuilder::CreateImageView(PH_ImageTexture& ImageTextureHandle,
		VkFormat format, VkImageAspectFlags aspect_flags)
	{
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = ImageTextureHandle.ImageHandle;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange.aspectMask = aspect_flags;

		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		PH_VULKAN_CHECK_MSG(vkCreateImageView(PH_REngineContext::GetLogicalDevice(), &viewCreateInfo, PH_REngineContext::GetDevice().GetAllocator(), &ImageTextureHandle.ImageView),
			"Failed to create image view.");
	}

	void PH_ImageBuilder::TransitionLayout(PH_ImageTexture& ImageTextureHandle,
		VkCommandBuffer& CommandBuffer, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout)
	{
		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = OldLayout;
		barrier.newLayout = NewLayout;
		barrier.srcQueueFamilyIndex = PH_REngineContext::GetDevice().GetGraphicsQueueIndex();
		barrier.dstQueueFamilyIndex = PH_REngineContext::GetDevice().GetGraphicsQueueIndex();
		barrier.image = ImageTextureHandle.ImageHandle;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags source_stage;
		VkPipelineStageFlags dest_stage;

		// Don't care about the old layout - transition to optimal layout (for the underlying implementation).
		if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			// Don't care what stage the pipeline is in at the start.
			source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			// Used for copying
			dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			// Transitioning from a transfer destination layout to a shader-readonly layout.
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			// From a copying stage to...
			source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			// The fragment stage.
			dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			PH_FATAL("Unsupported layout transition!");
			return;
		}

		vkCmdPipelineBarrier(
			CommandBuffer,
			source_stage, dest_stage,
			0,
			0, 0,
			0, 0,
			1, &barrier);
	}

	void PH_ImageBuilder::CopyFromBuffer(PH_ImageTexture& ImageTextureHandle, PH_Buffer& Buffer, VkCommandBuffer& CommandBuffer)
	{
		// Region to copy
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0,0,0 };
		region.imageExtent.width = ImageTextureHandle.Width;
		region.imageExtent.height = ImageTextureHandle.Height;
		region.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(
			CommandBuffer,
			Buffer.getBuffer(),
			ImageTextureHandle.ImageHandle,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
	}

	void PH_ImageBuilder::DestroyImage(PH_ImageTexture& ImageTextureHandle)
	{
		DestroyImageView(ImageTextureHandle);

		if (ImageTextureHandle.ImageMemory) {
			vkFreeMemory(PH_REngineContext::GetLogicalDevice(), ImageTextureHandle.ImageMemory, PH_REngineContext::GetDevice().GetAllocator());
			ImageTextureHandle.ImageMemory = 0;
		}

		if (ImageTextureHandle.ImageHandle) {
			vkDestroyImage(PH_REngineContext::GetLogicalDevice(), ImageTextureHandle.ImageHandle, PH_REngineContext::GetDevice().GetAllocator());
			ImageTextureHandle.ImageHandle = 0;
		}
	}

	void PH_ImageBuilder::DestroyImageSampler(PH_ImageTexture& ImageTextureHandle)
	{
		if(ImageTextureHandle.Sampler)
			vkDestroySampler(PH_REngineContext::GetLogicalDevice(), ImageTextureHandle.Sampler, PH_REngineContext::GetDevice().GetAllocator());

		ImageTextureHandle.Sampler = 0;
	}

	void PH_ImageBuilder::DestroyImageView(PH_ImageTexture& ImageTextureHandle)
	{
		if (ImageTextureHandle.ImageView) {
			vkDestroyImageView(PH_REngineContext::GetLogicalDevice(), ImageTextureHandle.ImageView, PH_REngineContext::GetDevice().GetAllocator());
			ImageTextureHandle.ImageView = 0;
		}
	}
};