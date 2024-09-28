#pragma once

#include "PH_CORE.h"
#include "PH_Device.h"
#include "Memory/PH_Memory.h"
#include "Containers/PH_Map.h"
#include "Containers/PH_DynamicArray.h"

namespace PhosphorEngine {

    using namespace PhosphorEngine::Containers;

    struct DescriptorLayoutInfo {
        //good idea to turn this into a inlined array
        PH_DynamicArray<VkDescriptorSetLayoutBinding> bindings;

        bool operator==(const DescriptorLayoutInfo& other) const;

        uint64 hash() const;
    };

    class  PH_DescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder& addBinding(
                uint32 binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32 count = 1);
            Builder& addBinding(VkDescriptorSetLayoutBinding binding) { bindings.push_back(binding); return *this; }
            PH_UniqueMemoryHandle<PH_DescriptorSetLayout> build() const;
            PH_UniqueMemoryHandle<PH_DescriptorSetLayout> buildFromCache() const;
            void buildRawVulkanLayout(VkDescriptorSetLayout& DescriptorSetLayout) const;
            VkDescriptorSetLayoutCreateInfo GetDescriptorRawVulkanInfo() const;
            DescriptorLayoutInfo GetDescriptorLayoutInfo() const;
            Builder& SetLayoutSetNumber(uint32 SetNumber) { DescriptorSetNumber = SetNumber; return *this; };
            uint32 GetLayoutSetNumber() const { return DescriptorSetNumber; };
            PH_DynamicArray<VkDescriptorSetLayoutBinding>& GetBindings() { return bindings; };
            VkDescriptorSetLayoutBinding& GetBinding(uint32 index) { return bindings[index]; };
        private:
            PH_DynamicArray<VkDescriptorSetLayoutBinding> bindings{};
            uint32 DescriptorSetNumber = 0;
        };

        PH_DescriptorSetLayout(PH_DynamicArray<VkDescriptorSetLayoutBinding> bindings, uint32 setNumber = 0);
        PH_DescriptorSetLayout(VkDescriptorSetLayout SetLayout, PH_DynamicArray<VkDescriptorSetLayoutBinding> bindings, uint32 setNumber = 0)
            : descriptorSetLayout(SetLayout), bindings(bindings), setNumber(setNumber) {}
        ~PH_DescriptorSetLayout();
        PH_DescriptorSetLayout(const PH_DescriptorSetLayout&) = delete;
        PH_DescriptorSetLayout& operator=(const PH_DescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
        uint32 GetSetNumber() const { return setNumber; };
    private:

        VkDescriptorSetLayout descriptorSetLayout;
        PH_DynamicArray<VkDescriptorSetLayoutBinding> bindings;
        uint32 setNumber = 0;
        uint8 destroyOnDestruct : 1;
        friend class PH_DescriptorWriter;
    };

    class  PH_DescriptorPoolAllocator {
    public:
        class Builder {
        public:
            Builder& addPoolSize(VkDescriptorType descriptorType, uint32 count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            PH_UniqueMemoryHandle<PH_DescriptorPoolAllocator> build() const;

        private:
            PH_DynamicArray<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };
        
        PH_DescriptorPoolAllocator(uint32 maxSets, VkDescriptorPoolCreateFlags flags, PH_DynamicArray<VkDescriptorPoolSize> poolSizes);
        ~PH_DescriptorPoolAllocator();
        PH_DescriptorPoolAllocator(const PH_DescriptorPoolAllocator&) = delete;
        PH_DescriptorPoolAllocator& operator=(const PH_DescriptorPoolAllocator&) = delete;

        bool allocateDescriptor(
            const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptorSet);

        void resetPools();

        void cleanup();

        const PH_DynamicArray<VkDescriptorPoolSize>& GetPoolSizes() const { return poolSizes; };
        uint32 GetMaxSets() const { return maxSets; };
        VkDescriptorPoolCreateFlags GetPoolFlags() const { return flags; };
    private:
        VkDescriptorPool currentPool{ VK_NULL_HANDLE };

        VkDescriptorPool getNextFreePool();

        PH_DynamicArray<VkDescriptorPool> usedPools;
        PH_DynamicArray<VkDescriptorPool> freePools;

        PH_DynamicArray<VkDescriptorPoolSize> poolSizes{};
        uint32 maxSets = 1000;
        VkDescriptorPoolCreateFlags flags;
        friend class PH_DescriptorWriter;
    };

    class PH_DescriptorWriter {
    public:
        PH_DescriptorWriter(PH_DescriptorSetLayout& setLayout);

        PH_DescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        PH_DescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

        bool build(VkDescriptorSet& set);
        void overwrite(VkDescriptorSet& set);

    private:
        PH_DescriptorSetLayout& setLayout;
        PH_DynamicArray<VkWriteDescriptorSet> writes;
    };

    class PH_DescriptorLayoutCache {
    public:
        void cleanup();

        VkDescriptorSetLayout CreateSetLayout(const PH_DescriptorSetLayout::Builder& SetBuilder);


    private:

        struct DescriptorLayoutHash
        {
            uint64 operator()(const DescriptorLayoutInfo& k) const
            {
                return k.hash();
            }
        };

        PH_Map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> layoutCache;
    };
}