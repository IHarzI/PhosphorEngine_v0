#pragma once

#include "Core/PH_CORE.h"
#include "Memory/PH_Memory.h"
#include "Core/PH_Device.h"
#include "Renderer/PH_Renderer.h"
#include "Core/PH_Window.h"

#include "Core/PH_Descriptors.h"
#include "Core/PH_Pipeline.h"
#include "Core/PH_FrameInfo.h"

namespace PhosphorEngine
{

	class PH_REngineContext
	{
	public:
		PH_REngineContext() {};
		PH_STATIC_LOCAL void Initialize(PH_Window& appWindow);
		PH_STATIC_LOCAL void Destroy();

		PH_STATIC_LOCAL PH_Device& GetDevice() {
			return GetRenderEngineContext().device.GetReference();
		}
		// Vulkan Logical Device
		PH_STATIC_LOCAL VkDevice GetLogicalDevice() {
			return GetRenderEngineContext().device->GetDevice();
		}
		PH_STATIC_LOCAL PH_DescriptorPoolAllocator& GetDescriptorPoolAllocator() { return GetRenderEngineContext().globalPool.GetReference(); }
		PH_STATIC_LOCAL PH_DescriptorLayoutCache& GetDescriptorLayoutCache() { return GetRenderEngineContext().descriptorLayoutCache.GetReference(); }
		PH_STATIC_LOCAL PH_PipelinesLayoutCache& GetPipelinesLayoutCache() { return GetRenderEngineContext().pipelinesLayousCache.GetReference(); }
		PH_STATIC_LOCAL PH_PipelinesCache& GetPipelinesCache() { return GetRenderEngineContext().pipelinesCache.GetReference(); }
		PH_STATIC_LOCAL PH_Renderer& GetRenderer() { return GetRenderEngineContext().renderer.GetReference(); }

		PH_STATIC_LOCAL PH_ImageBuilder::Map& GetTexturesMap() { return GetRenderEngineContext().textures; }

		PH_STATIC_LOCAL void UpdateFrameInfo(PH_FrameInfo FrameInfo) { 
			if (!GetRenderEngineContext().frameInfo.IsValid())
			{
				GetRenderEngineContext().frameInfo = GetRenderEngineContext().frameInfo.Create(FrameInfo);
			}
			*(GetRenderEngineContext().frameInfo) = FrameInfo; 
		}

		PH_STATIC_LOCAL PH_FrameInfo& GetCurrentFrameInfo() { return GetRenderEngineContext().frameInfo.GetReference(); }

		PH_STATIC_LOCAL bool IsInitialized() {
			return sInstanceREngineContext.IsValid();
		}
		PH_STATIC_LOCAL PH_REngineContext& GetRenderEngineContext() {
			PH_ASSERT(sInstanceREngineContext.IsValid());
			return sInstanceREngineContext.GetReference();
		}

		PH_STATIC_LOCAL PH_Pipeline& GetCullPipeline() { return GetRenderEngineContext().cullPipeline.GetReference();}
		PH_STATIC_LOCAL VkPipelineLayout& GetCullLayout() { return GetRenderEngineContext().cullLayout.GetReference(); }
		PH_STATIC_LOCAL PH_Pipeline& GetDepthReducePipeline() { return GetRenderEngineContext().depthReducePipeline.GetReference(); }
		PH_STATIC_LOCAL VkPipelineLayout& GetDepthReduceLayout() { return GetRenderEngineContext().depthReduceLayout.GetReference(); }
		PH_STATIC_LOCAL PH_Pipeline& GetSparseUploadPipeline() { return GetRenderEngineContext().sparseUploadPipeline.GetReference(); }
		PH_STATIC_LOCAL VkPipelineLayout& GetSparseUploadLayout() { return GetRenderEngineContext().sparseUploadLayout.GetReference(); }
		PH_STATIC_LOCAL PH_Pipeline& GetBlitPipeline() { return GetRenderEngineContext().blitPipeline.GetReference(); }
		PH_STATIC_LOCAL VkPipelineLayout& GetBlitLayout() {	return GetRenderEngineContext().blitLayout.GetReference();}
		PH_STATIC_LOCAL PH_DynamicArray<VkBufferMemoryBarrier>& GetUploadBarriers() { return GetRenderEngineContext().uploadBarriers; }
		PH_STATIC_LOCAL PH_DynamicArray<VkBufferMemoryBarrier>& GetCullReadyBarriers() { return GetRenderEngineContext().cullReadyBarriers; }
		PH_STATIC_LOCAL PH_DynamicArray<VkBufferMemoryBarrier>& GetPostCullBarriers() { return GetRenderEngineContext().postCullBarriers; }

	private:
		PH_UniqueMemoryHandle<PH_Device> device{};
		PH_UniqueMemoryHandle<PH_Renderer> renderer{};

		PH_UniqueMemoryHandle<PH_DescriptorPoolAllocator> globalPool{};
		PH_UniqueMemoryHandle<PH_DescriptorLayoutCache> descriptorLayoutCache{};
		PH_UniqueMemoryHandle<PH_PipelinesLayoutCache> pipelinesLayousCache{};
		PH_UniqueMemoryHandle<PH_PipelinesCache> pipelinesCache{};
		PH_UniqueMemoryHandle<PH_FrameInfo> frameInfo{};

		PH_UniqueMemoryHandle<PH_Pipeline> cullPipeline{};
		PH_UniqueMemoryHandle<VkPipelineLayout> cullLayout{};
		PH_UniqueMemoryHandle<PH_Pipeline> depthReducePipeline;
		PH_UniqueMemoryHandle<VkPipelineLayout> depthReduceLayout;
		PH_UniqueMemoryHandle<PH_Pipeline> sparseUploadPipeline;
		PH_UniqueMemoryHandle<VkPipelineLayout> sparseUploadLayout;
		PH_UniqueMemoryHandle<PH_Pipeline> blitPipeline;
		PH_UniqueMemoryHandle<VkPipelineLayout> blitLayout;

		PH_DynamicArray<VkBufferMemoryBarrier> uploadBarriers;

		PH_DynamicArray<VkBufferMemoryBarrier> cullReadyBarriers;

		PH_DynamicArray<VkBufferMemoryBarrier> postCullBarriers;

		PH_ImageBuilder::Map textures;

		PH_STATIC_LOCAL PH_INLINE PH_UniqueMemoryHandle<PH_REngineContext> sInstanceREngineContext{};
	};

}