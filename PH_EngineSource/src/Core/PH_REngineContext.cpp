#include "PH_REngineContext.h"

#include "Renderer/PH_Shader.h"

namespace PhosphorEngine {

    void PH_REngineContext::Initialize(PH_Window& appWindow)
    {
        PH_ASSERT(!IsInitialized());
        sInstanceREngineContext = PH_UniqueMemoryHandle<PH_REngineContext>::Create();
        auto& REngineContext = *sInstanceREngineContext;
        REngineContext.globalPool = PH_DescriptorPoolAllocator::Builder().setMaxSets(1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, PH_SwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, PH_SwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

        REngineContext.device = PH_UniqueMemoryHandle<PH_Device>::Create(appWindow);
        REngineContext.renderer = PH_UniqueMemoryHandle<PH_Renderer>::Create(appWindow);

        REngineContext.descriptorLayoutCache = PH_UniqueMemoryHandle<PH_DescriptorLayoutCache>::Create();
        REngineContext.pipelinesLayousCache = PH_UniqueMemoryHandle<PH_PipelinesLayoutCache>::Create();
        REngineContext.pipelinesCache = PH_UniqueMemoryHandle< PH_PipelinesCache>::Create();

        REngineContext.cullPipeline = REngineContext.cullPipeline.Create();
        REngineContext.cullLayout = REngineContext.cullLayout.Create();
        REngineContext.depthReducePipeline = REngineContext.depthReducePipeline.Create();
        REngineContext.depthReduceLayout = REngineContext.depthReduceLayout.Create();
        REngineContext.sparseUploadPipeline = REngineContext.sparseUploadPipeline.Create();
        REngineContext.sparseUploadLayout = REngineContext.sparseUploadLayout.Create();
        REngineContext.blitPipeline = REngineContext.blitPipeline.Create();
        REngineContext.blitLayout = REngineContext.blitLayout.Create();

        loadComputeShader("shaders/indirect_cull.comp.spv", REngineContext.cullPipeline.GetReference(), REngineContext.cullLayout.GetReference());
        loadComputeShader("shaders/depthReduce.comp.spv", REngineContext.depthReducePipeline.GetReference(), REngineContext.depthReduceLayout.GetReference());
        loadComputeShader("shaders/sparse_upload.comp.spv", REngineContext.sparseUploadPipeline.GetReference(), REngineContext.sparseUploadLayout.GetReference());
    }

    void PH_REngineContext::Destroy()
    {
        PH_ASSERT(IsInitialized());
        auto& REngineContext = GetRenderEngineContext();
        for (auto& imageTexture : REngineContext.textures)
        {
            REngineContext.renderer->DestroyTexture(imageTexture.second.GetReference());
        }

        REngineContext.descriptorLayoutCache->cleanup();
        REngineContext.pipelinesLayousCache->cleanup();
        REngineContext.globalPool->cleanup();
        REngineContext.pipelinesCache->cleanup();

        REngineContext.descriptorLayoutCache.Release();
        REngineContext.pipelinesLayousCache.Release();
        REngineContext.globalPool.Release();
        REngineContext.pipelinesCache.Release();

        sInstanceREngineContext.Release();
    }

}